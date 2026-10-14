#include "game/pch.h"
#include "game/game_data.h"
#include "game/input_slots.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

// Inspect the real Scene-owned roster without adding test-only game APIs.
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

    void resize(Application& app, Level& level, const Vector2I& size)
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        level.refresh_layout();
    }

    std::vector<float> simulation_state(const Level& level)
    {
        const LevelEndInfo paint = level.level_end_info();
        std::vector<float> state{ level.timer_, paint.total_area,
            paint.team_a_area, paint.team_b_area };
        state.insert(state.end(), paint.player_areas.begin(), paint.player_areas.end());
        for (const Player* player : level.players_)
        {
            const RectangleF& rectangle = player->rectangle();
            state.insert(state.end(), { rectangle.x, rectangle.y, rectangle.width,
                rectangle.height, player->velocity().x, player->velocity().y,
                player->health(), player->weapon_ammo(), static_cast<float>(player->state()) });
        }
        return state;
    }

    ScreenLayout layout(int count)
    {
        switch (count)
        {
        case 1: return ScreenLayout::one_player;
        case 2: return ScreenLayout::two_player;
        case 3: return ScreenLayout::three_player;
        default: return ScreenLayout::four_player;
        }
    }

    const char* mode_name(PlayerVisualMode mode)
    {
        switch (mode)
        {
        case PlayerVisualMode::original: return "original";
        case PlayerVisualMode::continuous: return "continuous";
        default: return "authored";
        }
    }

    std::vector<PlayerInputData> actions(int tick, int count)
    {
        std::vector<PlayerInputData> result(input_slots::count);
        for (int index = 0; index < count; ++index)
        {
            PlayerInputData& input = result[index];
            input.connected = true;
            input.shoot_direction_requested = true;
            input.shoot_angle = static_cast<float>(tick) * 2.0f * PI / 90.0f;
            if (index % 2 != 0) { input.shoot_angle = PI - input.shoot_angle; }
            input.shoot_angle = std::atan2(std::sin(input.shoot_angle), std::cos(input.shoot_angle));
            input.shoot_direction = Vector2F::unit_vec_from_angle(input.shoot_angle);
            input.jump_pressed = tick == 16;
            input.jump_held = tick >= 16 && tick < 46;
            // Move and reverse within the spawn approach, then stop. No RNG-dependent
            // spray during parity: firing is exercised separately after this sequence.
            input.x_movement = tick < 8 ? 0.4f : tick < 16 ? -0.4f : 0.0f;
        }
        return result;
    }

    std::unique_ptr<Level> build(LevelBuilder& builder, int count,
        PlayerVisualMode mode, TeamColourPair palette, const std::string& stage = "movement_course")
    {
        MenuLevelSettings settings;
        settings.set_player_count(count);
        settings.set_stage(stage);
        settings.set_screen_layout(layout(count));
        settings.set_workshop_visual_mode(mode);
        settings.begin_input_claims(0);
        for (int index = 1; index < count; ++index)
        {
            require(settings.claim_input_slot(index), "input claim failed");
        }
        for (int index = 0; index < count; ++index)
        {
            // Same-team pairs test P1/P2 and P3/P4 identity, not just team colour.
            settings.set_player_team(index, index < 2 ? PlayerTeam::a : PlayerTeam::b);
            settings.set_player_weapon(index, index % 2 == 0 ? WeaponType::sprayer : WeaponType::roller);
        }
        return builder.build_level(settings, palette, OpeningMode::off);
    }

    void settle(Level& level, int count)
    {
        std::vector<PlayerInputData> input = actions(0, count);
        for (PlayerInputData& action : input) { action.x_movement = 0; }
        for (int tick = 0; tick < 600 && level.state() != LevelState::active; ++tick)
        {
            level.update(input, dt);
        }
        require(level.state() == LevelState::active, "countdown did not finish");
        for (int tick = 0; tick < 60; ++tick) { level.update(input, dt); }
        for (const Player* player : level.players_)
        {
            require(player->state() == PlayerState::alive && player->velocity().y == 0,
                "authored spawn did not settle alive");
        }
    }

    struct Rgb
    {
        int r;
        int g;
        int b;
    };

    // The course background is data, so the colour that means "nothing drawn
    // here" is read from the same object the level draws.
    Rgb background_colour(const LevelLoadedInfo& info)
    {
        const JsonValue objects = info.non_collision_objects_json();
        for (size_t index = 0; index < objects.size(); ++index)
        {
            const JsonValue object = objects.at(index);
            if (object.string("name") != "course_background") { continue; }
            // A CSS name or RRGGBB, as the stage loader reads it.
            const std::string name = object.string("colour");
            std::optional<Colour> colour = colour_from_name(name);
            if (!colour && name.size() == 6) { colour = Colour(name); }
            require(colour.has_value(), "course background colour is not a colour");
            return { static_cast<int>(std::lround(colour->r * 255.0f)),
                static_cast<int>(std::lround(colour->g * 255.0f)),
                static_cast<int>(std::lround(colour->b * 255.0f)) };
        }
        throw std::runtime_error("Movement Course has no course_background object");
    }

    struct ContactSummary
    {
        int checked = 0;
        int skipped = 0;
        // Settled palette frames by frame width; these may never be skipped.
        std::map<int, int> settled_checked;
        int settled_skipped = 0;
    };

    // Grounded contact has no background gap: the collision bottom, projected
    // through the player's own view, is a drawn row, and so is every row up to
    // the first one above it. "Drawn" means some pixel across the projected
    // collision width is not course background. The check does not attribute
    // that pixel to the character's boots, so another drawn thing in the row
    // (a tool, paint, a stacked player) would also satisfy it. Grounded is
    // world state, never a tick number, and the support must be an authored
    // surface.
    void check_contact(const Frame& frame, const Level& level, Application& app,
        const LevelLoadedInfo& course, const Rgb& background, const std::string& label,
        std::ofstream& records, ContactSummary& summary, bool settled = false)
    {
        const auto is_background = [&](int x, int y) {
            const size_t offset = (static_cast<size_t>(y) * static_cast<size_t>(frame.width) +
                static_cast<size_t>(x)) * 4;
            return std::abs(frame.pixels[offset] - background.r) <= 2 &&
                std::abs(frame.pixels[offset + 1] - background.g) <= 2 &&
                std::abs(frame.pixels[offset + 2] - background.b) <= 2;
        };
        const JsonValue surfaces = course.collision_objects_json();
        for (size_t view = 0; view < level.players_.size(); ++view)
        {
            const Player& player = *level.players_[view];
            if (player.state() != PlayerState::alive ||
                (player.move_state() != PlayerMoveState::on_ground &&
                 player.move_state() != PlayerMoveState::on_drop_down_ground))
            {
                continue;
            }
            const RectangleF& body = player.rectangle();
            bool supported = false;
            for (size_t index = 0; index < surfaces.size() && !supported; ++index)
            {
                const JsonValue shape = surfaces.at(index).object("rectangle");
                const float x = shape.number("x");
                supported = std::abs(shape.number("y") - body.bottom()) <= 0.05f &&
                    x < body.right() && x + shape.number("width") > body.left();
            }
            require(supported, "a grounded player stands on no authored surface top");

            const RectangleI pane = app.viewport_manager()->player_viewport(
                static_cast<int>(view)).pixel_rect();
            const RectangleF projected = player.camera().calculate_view_rectangle(body);
            const int ground = pane.y + static_cast<int>(std::trunc(projected.bottom()));
            const int reach = static_cast<int>(std::ceil(projected.height / 4.0f));
            const int left = pane.x + std::max(0, static_cast<int>(std::ceil(projected.x)) + 1);
            const int right = pane.x + std::min(pane.width,
                static_cast<int>(std::floor(projected.right())) - 1);
            if (ground - reach < pane.y || ground + 2 > pane.bottom() || right - left < 4)
            {
                ++summary.skipped;
                summary.settled_skipped += settled ? 1 : 0;
                continue;
            }
            const auto empty_row = [&](int y) {
                for (int x = left; x < right; ++x)
                {
                    if (!is_background(x, y)) { return false; }
                }
                return true;
            };
            int gap = 0;
            while (gap < reach && empty_row(ground - 1 - gap)) { ++gap; }
            records << label << ',' << view + 1 << ',' << body.bottom() << ',' << ground << ','
                << left << ',' << right << ',' << gap << '\n';
            require(!empty_row(ground) && !empty_row(ground + 1),
                "the platform rows under a grounded player are background");
            if (gap != 0)
            {
                throw std::runtime_error(label + " view " + std::to_string(view + 1) + ": " +
                    std::to_string(gap) + " background row(s) between the player and platform row " +
                    std::to_string(ground) + " in columns " + std::to_string(left) + "-" +
                    std::to_string(right - 1));
            }
            ++summary.checked;
            summary.settled_checked[frame.width] += settled ? 1 : 0;
        }
    }

    void paused_draw(Level& level, Renderer& renderer)
    {
        level.suspend();
        const std::vector<float> before = simulation_state(level);
        const Frame first = render(level, renderer);
        for (int repeat = 0; repeat < 3; ++repeat)
        {
            require(render(level, renderer).pixels == first.pixels,
                "paused rendering changed pixels");
        }
        require(simulation_state(level) == before, "rendering changed simulation");
        level.resume();
    }
}

int wmain(int argc, wchar_t** argv)
{
    try
    {
        require(argc == 4, "usage: workshop_playable_visual <content> <output> <sequence frames>");
        const std::filesystem::path content = std::filesystem::absolute(argv[1]);
        const std::filesystem::path output = std::filesystem::absolute(argv[2]);
        const int sequence_frames = std::stoi(argv[3]);
        require(sequence_frames >= 0 && sequence_frames <= 180, "invalid sequence length");
        std::filesystem::create_directories(output);
        Registry<LevelLoadedInfo> levels("Level");
        GameTuning tuning((content / "tuning.json").string().c_str());
        Presentation presentation((content / "presentation.json").string().c_str());
        PlaySettings preferences;
        preferences.music_volume = 0;
        preferences.sfx_volume = 0;
        ApplicationOptions options;
        options.window_class_name = L"ColourWarsWorkshopPlayableProbe";
        options.window_title = L"ColourWars workshop playable comparison";
        options.resolution = ScreenResolution::s_1280_720;
        options.fullscreen = false;
        options.max_threads = 4;
        options.visible = false;
        Application app(options);
        app.initialize();
        const RenderDeviceInfo& device = app.renderer()->device_info();
        std::cout << "DEVICE " << device.backend << ' ' << device.api << ' ' << device.device_name << '\n';
        require(device.backend == "d3d11" && device.kind == RenderDeviceKind::hardware,
            "playable captures require D3D11 hardware");
        app.resource_loader()->register_kind("level", {
            [&](const std::string& directory, const std::string& name, bool) {
                auto info = std::make_unique<LevelLoadedInfo>((directory + name + ".json").c_str());
                info->validate();
                levels.add(name, std::move(info));
            }, nullptr });
        app.load_manifest((content / "manifest.json").string().c_str());
        presentation.validate_resources(*app.render_resources(), *app.audio_resources());
        LevelBuilder builder(app.viewport_manager(), app.render_resources(), app.audio_resources(),
            &preferences, &levels, &tuning, &presentation, app.resolution_manager(),
            app.thread_pool(), app.partitioner(), app.mouse());

        const std::array<PlayerVisualMode, 3> modes{ PlayerVisualMode::original,
            PlayerVisualMode::continuous, PlayerVisualMode::authored };
        std::ofstream records(output / "checks.csv");
        records << "players,mode,palette,width,height,phase\n";
        const LevelLoadedInfo& course = *levels.get("movement_course");
        const Rgb background = background_colour(course);
        std::ofstream contact(output / "contact.csv");
        contact << "frame,view,feet_y,platform_row,first_column,end_column,gap_rows\n";
        ContactSummary contacts;
        for (int count : { 1, 2, 3, 4 })
        {
            app.viewport_manager()->set_layout(layout(count));
            std::vector<std::vector<float>> baseline;
            for (PlayerVisualMode mode : modes)
            {
                auto level = build(builder, count, mode, TeamColourPair::blue_orange);
                settle(*level, count);
                resize(app, *level, { 1280, 720 });
                const std::string prefix = std::to_string(count) + "p-" + mode_name(mode);
                const std::filesystem::path sequence = output / (prefix + "-60fps");
                if (sequence_frames > 0 && (count == 1 || count == 4))
                {
                    std::filesystem::create_directories(sequence);
                }
                const int ticks = std::max(120, sequence_frames);
                for (int tick = 0; tick < ticks; ++tick)
                {
                    level->update(actions(tick, count), dt);
                    const std::vector<float> state = simulation_state(*level);
                    if (mode == PlayerVisualMode::original) { baseline.push_back(state); }
                    else { require(state == baseline.at(static_cast<size_t>(tick)),
                        "workshop visual mode changed movement, health, ammo or paint"); }
                    // Every tick is drawn for the contact check; stills and the
                    // sequence save that same frame.
                    const Frame frame = render(*level, *app.renderer());
                    check_contact(frame, *level, app, course, background,
                        prefix + "-blue_orange-1280-tick" + std::to_string(tick), contact, contacts);
                    if (tick == 0 || tick == 16 || tick == 42 || tick == 77 || tick == 100)
                    {
                        save(frame, output / (prefix + "-tick" + std::to_string(tick) + ".ppm"));
                        paused_draw(*level, *app.renderer());
                        records << count << ',' << mode_name(mode) << ",blue_orange,1280,720,tick" << tick << '\n';
                    }
                    if (tick < sequence_frames && (count == 1 || count == 4))
                    {
                        std::ostringstream filename;
                        filename << "frame-" << std::setw(3) << std::setfill('0') << tick << ".ppm";
                        save(frame, sequence / filename.str());
                    }
                }
                // Actual firing after the deterministic movement comparison. This is
                // not equality evidence because gameplay spray uses its own RNG.
                for (int tick = 0; tick < 120; ++tick)
                {
                    std::vector<PlayerInputData> firing = actions(100, count);
                    for (PlayerInputData& action : firing)
                    {
                        action.primary_shoot = true;
                        action.shoot_angle = PI / 2.0f;
                        action.shoot_direction = Vector2F::unit_vec_from_angle(action.shoot_angle);
                    }
                    level->update(firing, dt);
                }
                const LevelEndInfo painted = level->level_end_info();
                require(painted.team_a_area + painted.team_b_area > 0.0f,
                    "firing fixture did not paint a surface");
                std::cout << prefix << " fired: paint=" << painted.team_a_area + painted.team_b_area
                    << '/' << painted.total_area << '\n';
                const Frame painted_frame = render(*level, *app.renderer());
                check_contact(painted_frame, *level, app, course, background,
                    prefix + "-blue_orange-1280-painted", contact, contacts);
                save(painted_frame, output / (prefix + "-painted.ppm"));
            }
            for (TeamColourPair palette : { TeamColourPair::blue_orange,
                TeamColourPair::pink_green, TeamColourPair::blue_yellow })
            {
                for (PlayerVisualMode mode : modes)
                {
                    auto level = build(builder, count, mode, palette);
                    settle(*level, count);
                    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
                    {
                        resize(app, *level, size);
                        const std::string stem = std::to_string(count) + "p-" + mode_name(mode) +
                            "-palette" + std::to_string(static_cast<int>(palette)) + "-" + std::to_string(size.x);
                        const Frame frame = render(*level, *app.renderer());
                        check_contact(frame, *level, app, course, background, stem, contact, contacts,
                            true);
                        save(frame, output / (stem + ".ppm"));
                        paused_draw(*level, *app.renderer());
                        records << count << ',' << mode_name(mode) << ',' << static_cast<int>(palette)
                            << ',' << size.x << ',' << size.y << ",settled\n";
                    }
                }
            }
        }
        // Remembering a comparison choice must never reskin a competitive stage.
        app.viewport_manager()->set_layout(ScreenLayout::one_player);
        auto original = build(builder, 1, PlayerVisualMode::original, TeamColourPair::blue_orange, "close_quarters");
        auto authored = build(builder, 1, PlayerVisualMode::authored, TeamColourPair::blue_orange, "close_quarters");
        settle(*original, 1);
        settle(*authored, 1);
        require(render(*original, *app.renderer()).pixels == render(*authored, *app.renderer()).pixels,
            "course comparison leaked into Close Quarters");
        require(records.good() && contact.good(), "could not write check record");
        // Settled palette frames are 3 modes x 3 palettes x 10 views at each
        // size, every one grounded and checked, none skipped at a pane edge.
        require(contacts.settled_skipped == 0, "a settled contact was skipped at a pane edge");
        for (const int width : { 1280, 1920, 2560 })
        {
            require(contacts.settled_checked[width] == 90,
                "a resolution did not check every settled grounded contact");
        }
        std::cout << "PASS grounded contact, no background row: " << contacts.checked
            << " player-view checks (90 settled at each of 720p/1080p/QHD); " << contacts.skipped
            << " movement-frame checks skipped at a pane edge\n";
        std::cout << "PASS real Scene comparison; exact movement/resource/paint parity at 60Hz; "
            "1-4 views; 3 palettes; 720p/1080p/QHD; paused redraw; grounded contact; course-only selection\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
