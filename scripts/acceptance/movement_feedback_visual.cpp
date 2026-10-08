#include "game/pch.h"
#include "game/game_data.h"
#include "game/input_slots.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

// Inspect the real Scene-owned roster without adding test-only game APIs. No
// state, position, velocity, contact or effect is injected by this probe.
#define private public
#include "game/objects/level_builder.h"
#undef private

using namespace labrador;
using namespace mattmath;

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));

    struct Frame
    {
        int width;
        int height;
        std::vector<unsigned char> pixels;
    };

    Frame render(Level& level, Renderer& renderer)
    {
        renderer.begin_frame();
        level.draw(renderer);
        renderer.submit();
        const Vector2F size = renderer.back_buffer_size();
        Frame frame{ static_cast<int>(size.x), static_cast<int>(size.y), {} };
        renderer.read_back_buffer(frame.pixels);
        require(frame.pixels.size() ==
            static_cast<size_t>(frame.width) * static_cast<size_t>(frame.height) * 4,
            "readback size mismatch");
        return frame;
    }

    void save(const Frame& frame, const std::filesystem::path& path)
    {
        std::ofstream file(path, std::ios::binary);
        file << "P6\n" << frame.width << ' ' << frame.height << "\n255\n";
        std::vector<unsigned char> rgb(frame.pixels.size() / 4 * 3);
        for (size_t source = 0, target = 0; source < frame.pixels.size(); source += 4)
        {
            for (size_t channel = 0; channel < 3; ++channel)
            {
                rgb[target++] = frame.pixels[source + channel];
            }
        }
        file.write(reinterpret_cast<const char*>(rgb.data()),
            static_cast<std::streamsize>(rgb.size()));
        require(file.good(), "capture write failed");
    }

    size_t differences(const Frame& enabled, const Frame& disabled,
        const Viewport& viewport, const Level& level, int view)
    {
        require(enabled.width == disabled.width && enabled.height == disabled.height,
            "comparison frame sizes differ");
        const RectangleI rectangle = viewport.pixel_rect();
        std::vector<RectangleF> effect_bounds;
        for (const Player* player : level.players_)
        {
            RectangleF projected = level.players_[view]->camera().calculate_view_rectangle(player->bounds());
            // Rasterization can cover the last partial pixel at an edge.
            projected.x += static_cast<float>(rectangle.x) - 1;
            projected.y += static_cast<float>(rectangle.y) - 1;
            projected.width += 2;
            projected.height += 2;
            effect_bounds.push_back(projected);
        }
        size_t changed = 0;
        for (int y = rectangle.y; y < rectangle.bottom(); ++y)
        {
            for (int x = rectangle.x; x < rectangle.right(); ++x)
            {
                const size_t offset = (static_cast<size_t>(y) *
                    static_cast<size_t>(enabled.width) + static_cast<size_t>(x)) * 4;
                if (enabled.pixels[offset] != disabled.pixels[offset] ||
                    enabled.pixels[offset + 1] != disabled.pixels[offset + 1] ||
                    enabled.pixels[offset + 2] != disabled.pixels[offset + 2])
                {
                    ++changed;
                    const Vector2F pixel(static_cast<float>(x), static_cast<float>(y));
                    require(std::any_of(effect_bounds.begin(), effect_bounds.end(),
                        [&](const RectangleF& bounds) {
                            return pixel.x >= bounds.x && pixel.x <= bounds.right() &&
                                pixel.y >= bounds.y && pixel.y <= bounds.bottom();
                        }),
                        "feedback pixels escaped the Scene culling bounds");
                }
            }
        }
        return changed;
    }

    std::vector<float> simulation_state(const Level& level)
    {
        std::vector<float> state{ level.timer_ };
        for (const Player* player : level.players_)
        {
            const RectangleF& rectangle = player->rectangle();
            state.insert(state.end(), { rectangle.x, rectangle.y, rectangle.width,
                rectangle.height, player->velocity().x, player->velocity().y });
            const auto effects = player->movement_feedback().effects();
            state.push_back(static_cast<float>(effects.size()));
            for (const auto& effect : effects)
            {
                state.insert(state.end(), { static_cast<float>(effect.kind),
                    effect.position.x, effect.position.y, effect.age, effect.strength });
            }
        }
        return state;
    }

    void resize(Application& app, Level& level, const Vector2I& size)
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        level.refresh_layout();
    }

    bool grounded(const Player& player)
    {
        return player.move_state() == PlayerMoveState::on_ground ||
            player.move_state() == PlayerMoveState::on_drop_down_ground ||
            player.move_state() == PlayerMoveState::on_ramp_left ||
            player.move_state() == PlayerMoveState::on_ramp_right;
    }

    void capture_phase(Application& app, Level& level, Presentation& presentation,
        const std::filesystem::path& output, int count, const std::string& phase,
        int tick, std::ofstream& records, bool expect_difference = true)
    {
        auto& feedback = const_cast<MovementFeedbackPresentation&>(
            presentation.movement_feedback());
        const auto before = simulation_state(level);
        Vector2F reference_extent;
        for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080),
            Vector2I(2560, 1440) })
        {
            resize(app, level, size);
            const Vector2F extent = level.players_.front()->camera().visible_rectangle(
                app.viewport_manager()->player_viewport(0)).size();
            if (reference_extent == Vector2F::ZERO) { reference_extent = extent; }
            require((extent - reference_extent).length() < 0.01f,
                "world framing changed across resolutions");
            const std::string stem = std::to_string(count) + "p-" + phase + "-" +
                std::to_string(size.x) + "x" + std::to_string(size.y);

            // The same simulation snapshot is drawn twice. The presentation
            // switch only hides the existing effects; no intervening update.
            feedback.enabled = true;
            const Frame enabled = render(level, *app.renderer());
            feedback.enabled = false;
            const Frame disabled = render(level, *app.renderer());
            feedback.enabled = true;
            save(enabled, output / (stem + "-on.ppm"));
            save(disabled, output / (stem + "-off.ppm"));
            for (int view = 0; view < count; ++view)
            {
                const size_t changed = differences(enabled, disabled,
                    app.viewport_manager()->player_viewport(view), level, view);
                require(expect_difference ? changed > 0 : changed == 0,
                    "movement feedback visibility did not match the expected phase");
                const auto pane = app.viewport_manager()->player_viewport(view).pixel_rect();
                require(changed * 100 < static_cast<size_t>(pane.width) * static_cast<size_t>(pane.height),
                    "movement feedback changed more than one percent of a pane");
                records << count << ',' << phase << ',' << tick << ',' << tick * dt << ','
                    << size.x << ',' << size.y << ',' << view + 1 << ',' << changed << '\n';
            }
            std::cout << "PASS " << stem << " matched feedback on/off\n";
        }
        resize(app, level, { 1280, 720 });
        require(before == simulation_state(level),
            "on/off redraws or resize advanced movement, effects or the match clock");
    }

    void check_suspended_draws(Level& level, Renderer& renderer,
        const std::filesystem::path& output, int count)
    {
        level.suspend();
        const auto before = simulation_state(level);
        const Frame initial = render(level, renderer);
        for (int frame = 0; frame < 12; ++frame)
        {
            const Frame repeated = render(level, renderer);
            require(initial.pixels == repeated.pixels,
                "suspended redraw changed feedback or another visible state");
        }
        save(initial, output / (std::to_string(count) + "p-paused-landing.ppm"));
        require(before == simulation_state(level), "suspended drawing advanced simulation state");
        level.resume();
        std::cout << "PASS " << count << "p landing frozen across 12 suspended draws\n";
    }
}

int wmain(int argc, wchar_t** argv)
{
    try
    {
        require(argc == 4,
            "usage: movement_feedback_visual <staged-content> <output> <sequence-frames>");
        const std::filesystem::path content = std::filesystem::absolute(argv[1]);
        const std::filesystem::path output = std::filesystem::absolute(argv[2]);
        const int sequence_frames = std::stoi(argv[3]);
        require(sequence_frames >= 0 && sequence_frames <= 180,
            "sequence frame count must be between 0 and 180");
        std::filesystem::create_directories(output);
        std::ofstream records(output / "phases.csv");
        records << "players,phase,tick,seconds,width,height,view,changed_pixels\n";

        Registry<LevelLoadedInfo> levels("Level");
        GameTuning tuning((content / "tuning.json").string().c_str());
        Presentation presentation((content / "presentation.json").string().c_str());
        require(presentation.movement_feedback().enabled,
            "the visual probe requires feedback enabled in staged presentation.json");
        PlaySettings preferences;
        preferences.music_volume = 0;
        preferences.sfx_volume = 0;
        ApplicationOptions options;
        options.window_class_name = L"ColourWarsMovementFeedbackProbe";
        options.window_title = L"ColourWars movement feedback validation";
        options.resolution = ScreenResolution::s_1280_720;
        options.fullscreen = false;
        options.max_threads = 4;
        Application app(options);
        app.initialize(GetModuleHandleW(nullptr), SW_HIDE);
        const auto& device = app.renderer()->device_info();
        std::cout << "DEVICE backend=" << device.backend << " api=" << device.api
            << " name=" << device.device_name << " vendor=" << device.vendor_id
            << " device=" << device.device_id << '\n';
        require(device.backend == "d3d11" && device.kind == RenderDeviceKind::hardware,
            "visual evidence requires a D3D11 hardware device");
        app.resource_loader()->register_kind("level", {
            [&](const std::string& directory, const std::string& name, bool) {
                auto info = std::make_unique<LevelLoadedInfo>((directory + name + ".json").c_str());
                info->validate();
                levels.add(name, std::move(info));
            }, nullptr });
        app.load_manifest((content / "manifest.json").string().c_str());
        presentation.validate_resources(*app.render_resources(), *app.audio_resources());
        LevelBuilder builder(app.viewport_manager(), app.render_resources(),
            app.audio_resources(), &preferences, &levels, &tuning, &presentation,
            app.resolution_manager(), app.thread_pool(), app.partitioner(), app.mouse());
        for (int count : { 1, 4 })
        {
            MenuLevelSettings settings;
            settings.set_player_count(count);
            settings.set_stage("movement_course");
            settings.set_screen_layout(count == 1 ? ScreenLayout::one_player : ScreenLayout::four_player);
            settings.begin_input_claims(0);
            for (int player = 1; player < count; ++player)
            {
                require(settings.claim_input_slot(player), "input claim failed");
            }
            for (int player = 0; player < count; ++player)
            {
                settings.set_player_team(player, player % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
                settings.set_player_weapon(player, player % 2 == 0 ? WeaponType::sprayer : WeaponType::roller);
            }
            app.viewport_manager()->set_layout(settings.screen_layout());
            auto level = builder.build_level(settings, TeamColourPair::blue_orange);
            std::vector<PlayerInputData> inputs(input_slots::count);
            for (auto& input : inputs)
            {
                input.connected = true;
                input.shoot_direction = Vector2F(1, 0);
                input.shoot_direction_requested = true;
            }
            // Let the actual countdown, gravity and collision settle authored
            // spawn points. The limit deliberately refuses an unready scene.
            for (int tick = 0; tick < 600 && level->state() != LevelState::active; ++tick)
            {
                level->update(inputs, dt);
            }
            require(level->state() == LevelState::active, "countdown did not reach active play");
            for (int tick = 0; tick < 60; ++tick) { level->update(inputs, dt); }
            for (const Player* player : level->players_)
            {
                require(player->state() == PlayerState::alive && grounded(*player),
                    "authored spawn did not settle alive on support");
            }

            const auto sequence = output / (std::to_string(count) + "p-sequence-60fps");
            if (sequence_frames > 0) { std::filesystem::create_directories(sequence); }
            bool landed = false;
            int landing_tick = -1;
            const int takeoff_mid = static_cast<int>(std::ceil(
                presentation.movement_feedback().takeoff.duration / (2 * dt)));
            const int landing_mid = static_cast<int>(std::ceil(
                presentation.movement_feedback().landing.duration / (2 * dt)));
            const int landing_expiry = static_cast<int>(std::ceil(
                presentation.movement_feedback().landing.duration / dt)) + 1;
            bool expired = false;
            for (int tick = 0; tick < 180; ++tick)
            {
                for (auto& input : inputs)
                {
                    input.jump_pressed = tick == 0;
                    input.jump_held = tick < 30;
                }
                level->update(inputs, dt);
                for (const Player* player : level->players_)
                {
                    require(player->state() == PlayerState::alive, "scripted jump killed a player");
                }
                if (tick == 0)
                {
                    for (const Player* player : level->players_)
                    {
                        require(!grounded(*player) && player->velocity().y < 0,
                            "scripted jump did not take off");
                    }
                    capture_phase(app, *level, presentation, output, count, "takeoff", tick, records);
                }
                if (tick == takeoff_mid)
                {
                    capture_phase(app, *level, presentation, output, count, "takeoff-mid", tick, records);
                }
                if (!landed && tick > 0 && std::all_of(level->players_.begin(), level->players_.end(),
                    [](const Player* player) { return grounded(*player); }))
                {
                    landed = true;
                    landing_tick = tick;
                    capture_phase(app, *level, presentation, output, count, "landing", tick, records);
                    check_suspended_draws(*level, *app.renderer(), output, count);
                }
                if (landed && tick == landing_tick + landing_mid)
                {
                    capture_phase(app, *level, presentation, output, count, "landing-mid", tick, records);
                }
                if (landed && tick == landing_tick + landing_expiry)
                {
                    expired = true;
                    for (const Player* player : level->players_)
                    {
                        require(player->movement_feedback().effects().empty(),
                            "movement feedback did not expire");
                    }
                    capture_phase(app, *level, presentation, output, count, "expired", tick, records, false);
                }
                if (tick < sequence_frames)
                {
                    std::ostringstream filename;
                    filename << "frame-" << std::setw(3) << std::setfill('0') << tick << ".ppm";
                    save(render(*level, *app.renderer()), sequence / filename.str());
                }
                if (expired && tick + 1 >= sequence_frames) { break; }
            }
            require(landed, "scripted jump did not land within 180 ticks");
            require(expired, "movement feedback did not expire within 180 ticks");
        }
        require(records.good(), "phase record write failed");
        std::cout << "PASS real LevelBuilder/Scene takeoff and landing; 1/4 views; "
            "720p/1080p/QHD; feedback on/off; paused redraw; scripted inputs, no physical-device claim\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
