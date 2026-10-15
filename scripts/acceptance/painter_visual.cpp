#include "game/pch.h"
#include "engine/app/application.h"
#include "game/input_slots.h"
#include "game/objects/level_builder.h"
#include "game/objects/projectile_spray.h"
#include "game/debug/dev_tools.h"
#if COLOURWARS_DEV_TOOLS
#include "game/debug/match_fingerprint.h"
#endif

#include <filesystem>
#include <fstream>
#include <iostream>

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

    Frame read(Renderer& renderer)
    {
        renderer.submit();
        const Vector2F size = renderer.back_buffer_size();
        Frame frame{ static_cast<int>(size.x), static_cast<int>(size.y), {} };
        renderer.read_back_buffer(frame.pixels);
        require(frame.pixels.size() == static_cast<size_t>(frame.width) *
            static_cast<size_t>(frame.height) * 4, "readback size mismatch");
        return frame;
    }

    Frame render(const Level& level, Renderer& renderer)
    {
        renderer.begin_frame();
        level.draw(renderer);
        return read(renderer);
    }

    void save(const Frame& frame, const std::filesystem::path& path)
    {
        std::ofstream file(path, std::ios::binary);
        file << "P6\n" << frame.width << ' ' << frame.height << "\n255\n";
        std::vector<unsigned char> rgb(frame.pixels.size() / 4 * 3);
        size_t target = 0;
        for (size_t source = 0; source < frame.pixels.size(); source += 4)
        {
            for (size_t channel = 0; channel < 3; ++channel) { rgb[target++] = frame.pixels[source + channel]; }
        }
        file.write(reinterpret_cast<const char*>(rgb.data()), static_cast<std::streamsize>(rgb.size()));
        require(file.good(), "capture write failed");
    }

    // The same public simulation evidence used by the replay fingerprint, plus
    // aim, facing and movement effects. It is also available in Release.
    std::vector<float> observation(const Level& level)
    {
        const LevelEndInfo info = level.level_end_info();
        std::vector<float> values{ static_cast<float>(level.state()),
            info.team_a_area, info.team_b_area, info.total_area };
        for (const float area : info.player_areas) { values.push_back(area); }
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Player& player = level.player(view);
            const RectangleF body = player.rectangle();
            const Weapon& tool = player.primary_weapon();
            const ToolMotionState& mechanism = tool.motion_state();
            values.insert(values.end(), { body.x, body.y, body.width, body.height,
                player.velocity().x, player.velocity().y, player.health(), player.weapon_ammo(),
                player.respawn_timer(), static_cast<float>(player.state()),
                static_cast<float>(player.move_state()), player.facing_right() ? 1.0f : 0.0f,
                tool.rotation(), tool.nozzle_pos().x, tool.nozzle_pos().y,
                static_cast<float>(player.animation_state()), player.action_age(),
                static_cast<float>(player.action_frame()),
                static_cast<float>(tool.visual_pose()), mechanism.pressure,
                mechanism.valve, mechanism.travel_phase, mechanism.lift,
                mechanism.compression, mechanism.bucket_swing, mechanism.hose_sway,
                mechanism.mist_tail_remaining });
            const auto effects = player.movement_feedback().effects();
            values.push_back(static_cast<float>(effects.size()));
            for (const auto& effect : effects)
            {
                values.insert(values.end(), { static_cast<float>(effect.kind),
                    effect.position.x, effect.position.y, effect.age, effect.strength });
            }
        }
        return values;
    }

    void resize(Application& app, Level& level, const Vector2I& size)
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        level.refresh_layout();
    }

    // A contrasting canvas makes black outlines observable too. The player is
    // still the real builder-owned object, drawn with its actual pane/camera.
    struct Canvas : TextureObject
    {
        explicit Canvas(RenderResources* resources) :
            TextureObject("sprite_sheet_1", "pixel", resources, Colour(0.72f, 0.75f, 0.78f)) {}
        using TextureObject::draw;
    };

    Frame isolated(const Player* player, const Player& camera_player,
        const Viewport& viewport, Renderer& renderer, const Canvas& canvas)
    {
        renderer.begin_frame();
        renderer.set_view_count(1);
        DrawList list = renderer.view(0);
        list.set_viewport(viewport);
        list.set_camera(camera_player.camera());
        list.set_filter(TextureFilter::point);
        canvas.draw(list, camera_player.camera().visible_rectangle(viewport));
        if (player != nullptr) { player->draw(list); }
        return read(renderer);
    }

    // Pin only the tool's pivot in pane pixels, at the real gameplay zoom.
    // This removes body animation, camera follow and travelled distance from
    // the comparison, so a changed raster must come from the tool itself.
    Frame isolated_tool(const Player& player, const Viewport& viewport,
        Renderer& renderer, const Canvas& canvas)
    {
        const Weapon& tool = player.primary_weapon();
        const float scale = player.camera().scale;
        const RectangleI pane = viewport.pixel_rect();
        const Camera camera(tool.draw_pos() - Vector2F(
            static_cast<float>(pane.width) / (2 * scale),
            static_cast<float>(pane.height) / (2 * scale)), scale);
        renderer.begin_frame();
        renderer.set_view_count(1);
        DrawList list = renderer.view(0);
        list.set_viewport(viewport);
        list.set_camera(camera);
        list.set_filter(TextureFilter::point);
        canvas.draw(list, camera.visible_rectangle(viewport));
        tool.draw(list);
        return read(renderer);
    }

    // This checks raster output, rather than reproducing the arms/accent
    // geometry in the test. Keep one pixel for rasterized partial edges.
    size_t check_bounds(const Frame& image, const Frame& blank, const Player& player,
        const Viewport& viewport, Frame& crop)
    {
        const RectangleI pane = viewport.pixel_rect();
        RectangleF bounds = player.camera().calculate_view_rectangle(
            player.cull_bounds(1.0f / player.camera().scale));
        bounds.x += static_cast<float>(pane.x) - 1;
        bounds.y += static_cast<float>(pane.y) - 1;
        bounds.width += 2;
        bounds.height += 2;
        int left = pane.right(), top = pane.bottom(), right = pane.x, bottom = pane.y;
        size_t changed = 0;
        for (int y = pane.y; y < pane.bottom(); ++y)
        {
            for (int x = pane.x; x < pane.right(); ++x)
            {
                const size_t offset = (static_cast<size_t>(y) * static_cast<size_t>(image.width) +
                    static_cast<size_t>(x)) * 4;
                if (image.pixels[offset] == blank.pixels[offset] &&
                    image.pixels[offset + 1] == blank.pixels[offset + 1] &&
                    image.pixels[offset + 2] == blank.pixels[offset + 2]) { continue; }
                require(static_cast<float>(x) >= bounds.x && static_cast<float>(x) <= bounds.right() &&
                    static_cast<float>(y) >= bounds.y && static_cast<float>(y) <= bounds.bottom(),
                    "rendered painter/tool pixels escaped projected Scene culling bounds");
                ++changed;
                left = std::min(left, x); top = std::min(top, y);
                right = std::max(right, x + 1); bottom = std::max(bottom, y + 1);
            }
        }
        if (changed == 0) { return 0; }
        left = std::max(pane.x, left - 8); top = std::max(pane.y, top - 8);
        right = std::min(pane.right(), right + 8); bottom = std::min(pane.bottom(), bottom + 8);
        crop = { right - left, bottom - top, {} };
        for (int y = top; y < bottom; ++y)
        {
            const size_t start = (static_cast<size_t>(y) * static_cast<size_t>(image.width) +
                static_cast<size_t>(left)) * 4;
            crop.pixels.insert(crop.pixels.end(), image.pixels.begin() + start,
                image.pixels.begin() + start + static_cast<size_t>(crop.width) * 4);
        }
        return changed;
    }

    void capture(Application& app, Level& level, const Canvas& canvas,
        const std::filesystem::path& output, const std::string& stem,
        std::ofstream& records, bool expect_visible = true)
    {
        const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
        const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
        const Frame first = render(level, *app.renderer());
        for (int repeat = 0; repeat < 3; ++repeat)
        {
            require(first.pixels == render(level, *app.renderer()).pixels,
                "frozen redraw changed painter or another visible state");
        }
        save(first, output / (stem + ".ppm"));
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Player& player = level.player(view);
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            const Frame blank = isolated(nullptr, player, viewport, *app.renderer(), canvas);
            const Frame image = isolated(&player, player, viewport, *app.renderer(), canvas);
            Frame crop{};
            const size_t changed = check_bounds(image, blank, player, viewport, crop);
            require(expect_visible ? changed > 0 : changed == 0,
                "painter visibility did not match death/redraw phase");
            if (changed > 0) { save(crop, output / (stem + "-player" + std::to_string(view + 1) + ".ppm")); }
            records << stem << ',' << view + 1 << ',' << player.player_num() << ',' << changed << ','
                << player.facing_right() << ',' << player.primary_weapon().rotation() << ','
                << player.respawn_progress() << ',' << static_cast<int>(player.animation_state()) << ','
                << player.action_age() << ',' << player.action_frame() << ','
                << player.primary_weapon().visual_pose() << ','
                << player.primary_weapon().motion_state().pressure << ','
                << player.primary_weapon().motion_state().valve << ','
                << player.primary_weapon().motion_state().travel_phase << ','
                << player.primary_weapon().motion_state().mist_tail_remaining << '\n';
        }
        require(before == observation(level), "drawing changed public simulation state");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
            "drawing changed the replay fingerprint");
#endif
        std::cout << "PASS " << stem << " frozen redraw, fingerprint and raster bounds\n";
    }

    std::unique_ptr<Level> build(Application& app, LevelBuilder& builder,
        int count, TeamColourPair palette, bool roller = false, bool same_team = false,
        WeaponType tool = WeaponType::none)
    {
        MenuLevelSettings settings;
        settings.set_player_count(count);
        settings.set_stage("movement_course");
        settings.set_screen_layout(count == 1 ? ScreenLayout::one_player : ScreenLayout::four_player);
        constexpr int slots[] = { 3, 1, 2, 0 };
        constexpr WeaponType tools[] = { WeaponType::sprayer, WeaponType::sniper,
            WeaponType::mister, WeaponType::bucket };
        settings.begin_input_claims(slots[0]);
        for (int view = 1; view < count; ++view)
        {
            require(settings.claim_input_slot(slots[view]), "input claim failed");
        }
        for (int view = 0; view < count; ++view)
        {
            settings.set_player_team(view, same_team || view % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
            settings.set_player_weapon(view, tool != WeaponType::none ? tool :
                (roller ? WeaponType::roller : tools[view]));
        }
        app.viewport_manager()->set_layout(settings.screen_layout());
        auto level = builder.build_level(settings, palette, OpeningMode::off);
        require(level->player_count() == count, "builder changed the roster count");
        for (int view = 0; view < count; ++view)
        {
            require(level->player(view).player_num() == slots[view], "builder changed the sparse input roster");
        }
        return level;
    }

    std::vector<PlayerInputData> neutral_inputs()
    {
        std::vector<PlayerInputData> inputs(input_slots::count);
        for (auto& input : inputs)
        {
            input.connected = true;
            input.shoot_direction = Vector2F(1, 0);
            input.shoot_direction_requested = true;
        }
        return inputs;
    }

    void settle(Level& level, const std::vector<PlayerInputData>& inputs)
    {
        for (int tick = 0; tick < 600 && level.state() != LevelState::active; ++tick) { level.update(inputs, dt); }
        require(level.state() == LevelState::active, "countdown did not reach active play");
        for (int tick = 0; tick < 60; ++tick) { level.update(inputs, dt); }
        for (int view = 0; view < level.player_count(); ++view)
        {
            require(level.player(view).state() == PlayerState::alive, "authored spawn did not settle alive");
        }
    }

    // Compare the two actual body rasters at the same location and camera,
    // without a rotating tool or movement effects supplying a false change.
    // The normal captures still include each real tool, viewport and cull box.
    Frame pinned_body(Player& player, const RectangleF& anchor, const Camera& camera,
        const Viewport& viewport, Renderer& renderer, const Canvas& canvas)
    {
        const Camera live_camera = player.camera();
        player.set_pose({ anchor.position() - player.rectangle().position(), 0, false, false });
        player.set_camera(camera);
        const Frame result = isolated(&player, player, viewport, renderer, canvas);
        player.set_camera(live_camera);
        player.set_pose({});
        return result;
    }

    void action_frames(Application& app, Level& level, const Canvas& canvas,
        const std::filesystem::path& output, const std::string& stem,
        std::ofstream& records, const std::vector<PlayerInputData>& inputs,
        PlayerAnimationState expected)
    {
        std::vector<RectangleF> anchors;
        std::vector<Camera> cameras;
        std::vector<Frame> first;
        for (int view = 0; view < level.player_count(); ++view)
        {
            Player& player = const_cast<Player&>(level.player(view));
            require(player.animation_state() == expected && player.action_frame() == 0 &&
                player.action_age() == 0.0f, "action did not begin at its first frame on the event tick");
            anchors.push_back(player.rectangle());
            cameras.push_back(player.camera());
            first.push_back(pinned_body(player, anchors.back(), cameras.back(),
                app.viewport_manager()->player_viewport(view), *app.renderer(), canvas));
        }
        capture(app, level, canvas, output, stem + "-frame0", records);
        for (int tick = 0; tick < 30 && level.player(0).animation_state() == expected &&
            level.player(0).action_frame() == 0; ++tick) { level.update(inputs, dt); }
        for (int view = 0; view < level.player_count(); ++view)
        {
            Player& player = const_cast<Player&>(level.player(view));
            require(player.animation_state() == expected && player.action_frame() == 1,
                "action skipped its second frame");
            const Frame second = pinned_body(player, anchors[static_cast<size_t>(view)],
                cameras[static_cast<size_t>(view)], app.viewport_manager()->player_viewport(view),
                *app.renderer(), canvas);
            require(second.pixels != first[static_cast<size_t>(view)].pixels,
                "first and second action body frames produced identical playing-size raster output");
        }
        capture(app, level, canvas, output, stem + "-frame1", records);
    }

    void action_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        const GameTuning& tuning, const std::filesystem::path& output,
        std::ofstream& records, int count, TeamColourPair palette, bool right)
    {
        auto level = build(app, builder, count, palette);
        auto inputs = neutral_inputs();
        settle(*level, inputs);
        resize(app, *level, { 1280, 720 });
        for (auto& input : inputs) { input.x_movement = right ? 1.0f : -1.0f; }
        level->update(inputs, dt);
        for (auto& input : inputs) { input.x_movement = 0.0f; }
        for (int tick = 0; tick < 20; ++tick) { level->update(inputs, dt); }
        const std::string prefix = std::to_string(count) + "p-action-" +
            TeamColourTools::palette_key(palette) + (right ? "-right-" : "-left-");
        for (auto& input : inputs) { input.jump_pressed = true; }
        level->update(inputs, dt);
        for (auto& input : inputs) { input.jump_pressed = false; }
        action_frames(app, *level, canvas, output, prefix + "takeoff", records, inputs,
            PlayerAnimationState::taking_off);
        for (int tick = 0; tick < 150 &&
            level->player(0).animation_state() != PlayerAnimationState::landing; ++tick)
        {
            level->update(inputs, dt);
        }
        action_frames(app, *level, canvas, output, prefix + "land", records, inputs,
            PlayerAnimationState::landing);
        for (int tick = 0; tick < 20; ++tick) { level->update(inputs, dt); }
        for (auto& input : inputs)
        {
            input.primary_shoot = true;
            input.shoot_angle = -PI / 2.0f;
            input.shoot_direction = Vector2F::unit_vec_from_angle(input.shoot_angle);
        }
        level->update(inputs, dt);
        for (auto& input : inputs) { input.primary_shoot = false; }
        action_frames(app, *level, canvas, output, prefix + "fire", records, inputs,
            PlayerAnimationState::firing);
        for (int view = 0; view < count; ++view)
        {
            Player& player = const_cast<Player&>(level->player(view));
            const ProjectileSpray hit(RectangleF(0, 0, 10, 10), Vector2F::ZERO,
                player.team() == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a,
                0, Colour::white, app.render_resources(), &tuning.projectile(ProjectileType::spray));
            player.on_contact(hit, Vector2F::ZERO, 0);
        }
        action_frames(app, *level, canvas, output, prefix + "hurt", records, inputs,
            PlayerAnimationState::hurt);
        for (int view = 0; view < count; ++view)
        {
            require(level->kill_player(view), "could not request action-matrix elimination");
        }
        level->update(inputs, dt);
        action_frames(app, *level, canvas, output, prefix + "elimination", records, inputs,
            PlayerAnimationState::elimination);
    }

    // Real paid shots and resolved movement select the tool poses. Capture the
    // release and airborne phrases too: additional render views must not age
    // either one, or leave cosmetic pixels outside the scene's cull bounds.
    void tool_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        const std::filesystem::path& output, std::ofstream& records, int count,
        TeamColourPair palette, bool right, WeaponType tool, const std::string& name)
    {
        auto level = build(app, builder, count, palette, false, false, tool);
        auto inputs = neutral_inputs();
        settle(*level, inputs);
        resize(app, *level, { 1280, 720 });
        for (auto& input : inputs)
        {
            input.x_movement = right ? 1.0f : -1.0f;
            input.shoot_angle = right ? -PI / 4.0f : -3.0f * PI / 4.0f;
            input.shoot_direction = Vector2F::unit_vec_from_angle(input.shoot_angle);
        }
        level->update(inputs, dt);
        for (auto& input : inputs) { input.x_movement = 0; }
        for (int tick = 0; tick < 25; ++tick) { level->update(inputs, dt); }
        const std::string prefix = std::to_string(count) + "p-tool-" + name + "-" +
            TeamColourTools::palette_key(palette) + (right ? "-right-" : "-left-");
        std::vector<Frame> idle_tools;
        for (int view = 0; view < count; ++view)
        {
            idle_tools.push_back(isolated_tool(level->player(view),
                app.viewport_manager()->player_viewport(view), *app.renderer(), canvas));
        }
        const auto check_mechanisms = [&]()
        {
            for (int view = 0; view < count; ++view)
            {
                const Player& player = level->player(view);
                if (player.primary_weapon().visual_pose() == 0) { continue; }
                const Frame image = isolated_tool(player,
                    app.viewport_manager()->player_viewport(view), *app.renderer(), canvas);
                require(image.pixels != idle_tools[static_cast<size_t>(view)].pixels,
                    "tool mechanism pose is indistinguishable from idle at gameplay size");
            }
        };
        capture(app, *level, canvas, output, prefix + "idle", records);
        for (auto& input : inputs) { input.primary_shoot = true; }
        level->update(inputs, dt);
        check_mechanisms();
        capture(app, *level, canvas, output, prefix + "paid-shot", records);
        for (int tick = 0; tick < 5; ++tick) { level->update(inputs, dt); }
        check_mechanisms();
        capture(app, *level, canvas, output, prefix + "held-fire", records);
        for (auto& input : inputs) { input.primary_shoot = false; }
        level->update(inputs, dt);
        capture(app, *level, canvas, output, prefix + "release", records);
        level->suspend();
        capture(app, *level, canvas, output, prefix + "paused", records);
        level->resume();
        for (int tick = 0; tick < 8; ++tick) { level->update(inputs, dt); }
        check_mechanisms();
        capture(app, *level, canvas, output, prefix + "recovery", records);
        if (tool == WeaponType::sniper || tool == WeaponType::none)
        {
            for (int tick = 0; tick < 30; ++tick) { level->update(inputs, dt); }
            check_mechanisms();
            capture(app, *level, canvas, output, prefix + "gauge-recovery", records);
        }
        if (tool == WeaponType::roller || tool == WeaponType::sprayer ||
            tool == WeaponType::bucket || tool == WeaponType::none)
        {
            for (auto& input : inputs) { input.x_movement = right ? 1.0f : -1.0f; }
            for (int tick = 0; tick < 12; ++tick) { level->update(inputs, dt); }
            check_mechanisms();
            capture(app, *level, canvas, output, prefix + "travel", records);
        }
        if (tool == WeaponType::roller)
        {
            for (auto& input : inputs) { input.jump_pressed = true; }
            level->update(inputs, dt);
            for (auto& input : inputs) { input.jump_pressed = false; input.x_movement = 0; }
            for (int tick = 0; tick < 3; ++tick) { level->update(inputs, dt); }
            check_mechanisms();
            capture(app, *level, canvas, output, prefix + "lift", records);
        }
        if (count == 4)
        {
            for (const Vector2I size : { Vector2I(1920, 1080), Vector2I(2560, 1440) })
            {
                resize(app, *level, size);
                capture(app, *level, canvas, output,
                    prefix + "extent-" + std::to_string(size.x), records);
            }
        }
    }
}

int wmain(int argc, wchar_t** argv)
{
    try
    {
        require(argc == 3, "usage: painter_visual <staged-content> <output>");
        const std::filesystem::path content = std::filesystem::absolute(argv[1]);
        const std::filesystem::path output = std::filesystem::absolute(argv[2]);
        std::filesystem::create_directories(output);
        std::ofstream records(output / "painter.csv");
        records << "capture,view,input_slot,painter_pixels,facing_right,tool_angle,respawn_progress,animation_state,action_age,action_frame,tool_pose,pressure,valve,travel_phase,mist_tail_remaining\n";
        Registry<LevelLoadedInfo> levels("Level");
        GameTuning tuning((content / "tuning.json").string().c_str());
        Presentation presentation((content / "presentation.json").string().c_str());
        PlaySettings preferences;
        preferences.music_volume = 0;
        preferences.sfx_volume = 0;
        ApplicationOptions options;
        options.window_class_name = L"ColourWarsPainterProbe";
        options.window_title = L"ColourWars painter validation";
        options.resolution = ScreenResolution::s_1280_720;
        options.fullscreen = false;
        options.max_threads = 4;
        options.visible = false;
        Application app(options);
        app.initialize();
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
        tuning.validate_resources(*app.render_resources(), *app.audio_resources());
        presentation.validate_resources(*app.render_resources(), *app.audio_resources());
        LevelBuilder builder(app.viewport_manager(), app.render_resources(),
            app.audio_resources(), &preferences, &levels, &tuning, &presentation,
            app.resolution_manager(), app.thread_pool(), app.partitioner(), app.mouse());
        const Canvas canvas(app.render_resources());
        // Base captures below keep their normal movement effects. Disable them
        // only while checking action-body frame differences in this matrix.
        presentation.set_movement_feedback_enabled(false);
        for (const bool right : { true, false })
        {
            for (const auto& tool : std::array<std::pair<WeaponType, const char*>, 5>{{
                { WeaponType::sprayer, "sprayer" }, { WeaponType::sniper, "sniper" },
                { WeaponType::roller, "roller" }, { WeaponType::mister, "mister" },
                { WeaponType::bucket, "bucket" } }})
            {
                tool_matrix(app, builder, canvas, output, records, 1,
                    TeamColourPair::blue_orange, right, tool.first, tool.second);
            }
        }
        for (const TeamColourPair palette : { TeamColourPair::blue_orange,
            TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            tool_matrix(app, builder, canvas, output, records, 4, palette, true,
                WeaponType::none, "mixed");
            tool_matrix(app, builder, canvas, output, records, 4, palette, false,
                WeaponType::roller, "roller");
        }
        for (const bool right : { true, false })
        {
            action_matrix(app, builder, canvas, tuning, output, records, 1,
                TeamColourPair::blue_orange, right);
        }
        for (const TeamColourPair palette : { TeamColourPair::blue_orange,
            TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            action_matrix(app, builder, canvas, tuning, output, records, 4, palette,
                palette != TeamColourPair::blue_orange);
        }
        presentation.set_movement_feedback_enabled(true);
        for (int count : { 1, 4 })
        {
            auto level = build(app, builder, count, TeamColourPair::blue_orange);
            auto inputs = neutral_inputs();
            settle(*level, inputs);
            for (int phase = 0; phase < 8; ++phase)
            {
                const bool right = phase < 4;
                for (auto& input : inputs)
                {
                    input.x_movement = right ? 1.0f : -1.0f;
                }
                for (int tick = 0; tick < 8; ++tick)
                {
                    const float angle = PI * (-0.25f + 0.5f * static_cast<float>(phase % 4) +
                        0.5f * static_cast<float>(tick + 1) / 8.0f);
                    for (auto& input : inputs)
                    {
                        input.shoot_angle = std::atan2(std::sin(angle), std::cos(angle));
                        input.shoot_direction = Vector2F::unit_vec_from_angle(input.shoot_angle);
                    }
                    level->update(inputs, dt);
                }
                for (int view = 0; view < count; ++view)
                {
                    require(level->player(view).state() == PlayerState::alive &&
                        level->player(view).facing_right() == right,
                        "scripted moving aim sweep did not preserve a live facing");
                }
                for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
                {
                    if (size.x != 1280 && phase != 1 && phase != 5) { continue; }
                    resize(app, *level, size);
                    const std::string stem = std::to_string(count) + "p-blue_orange-" +
                        (right ? "right" : "left") + "-q" + std::to_string(phase % 4 + 1) + "-" +
                        std::to_string(size.x) + "x" + std::to_string(size.y);
                    capture(app, *level, canvas, output, stem, records);
                }
            }
            resize(app, *level, { 1280, 720 });
            level->suspend();
            capture(app, *level, canvas, output, std::to_string(count) + "p-paused", records);
            level->resume();
        }
        for (const TeamColourPair palette : { TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            auto level = build(app, builder, 4, palette);
            const auto inputs = neutral_inputs();
            settle(*level, inputs);
            resize(app, *level, { 1280, 720 });
            capture(app, *level, canvas, output, std::string("4p-") + TeamColourTools::palette_key(palette), records);
        }
        {
            auto teammates = build(app, builder, 4, TeamColourPair::blue_orange, false, true);
            auto teammate_inputs = neutral_inputs();
            settle(*teammates, teammate_inputs);
            resize(app, *teammates, { 1280, 720 });
            for (bool right : { true, false })
            {
                const float angle = right ? PI * 0.25f : PI * 0.75f;
                for (auto& input : teammate_inputs)
                {
                    input.x_movement = right ? 1.0f : -1.0f;
                    input.shoot_angle = angle;
                    input.shoot_direction = Vector2F::unit_vec_from_angle(angle);
                }
                for (int tick = 0; tick < 8; ++tick) { teammates->update(teammate_inputs, dt); }
                for (int view = 0; view < 4; ++view)
                {
                    require(teammates->player(view).team() == PlayerTeam::a &&
                        teammates->player(view).state() == PlayerState::alive &&
                        teammates->player(view).facing_right() == right,
                        "same-team roster did not preserve four live teammate facings");
                }
                capture(app, *teammates, canvas, output,
                    right ? "4p-same-team-right" : "4p-same-team-left", records);
            }
        }
        auto level = build(app, builder, 1, TeamColourPair::blue_orange, true);
        const auto inputs = neutral_inputs();
        settle(*level, inputs);
        resize(app, *level, { 1280, 720 });
        capture(app, *level, canvas, output, "1p-roller", records);
        Player& player = const_cast<Player&>(level->player(0));
        const auto before_pose = observation(*level);
        player.set_pose({ Vector2F(4, 0), 0.15f, false, false });
        capture(app, *level, canvas, output, "1p-story-tool-free-squash", records);
        require(before_pose == observation(*level), "story pose changed simulation state");
        player.set_pose({});
        require(level->kill_player(0), "could not request elimination");
        level->update(inputs, dt);
        require(player.state() == PlayerState::dead, "elimination did not enter death");
        const Camera whole_camera = player.camera();
        player.set_pose({ Vector2F::ZERO, 0, false, true });
        const Frame early_whole = isolated(&player, player,
            app.viewport_manager()->player_viewport(0), *app.renderer(), canvas);
        player.set_pose({});
        for (int tick = 0; tick < 30 && player.animation_state() == PlayerAnimationState::elimination; ++tick)
        {
            level->update(inputs, dt);
        }
        require(player.animation_state() == PlayerAnimationState::idle,
            "elimination did not finish before the outline redraw");
        const Camera live_camera = player.camera();
        player.set_camera(whole_camera);
        player.set_pose({ Vector2F::ZERO, 0, false, true });
        const Frame settled_whole = isolated(&player, player,
            app.viewport_manager()->player_viewport(0), *app.renderer(), canvas);
        require(early_whole.pixels == settled_whole.pixels,
            "whole-dead story figure inherited an elimination frame instead of its idle figure");
        player.set_pose({});
        player.set_camera(live_camera);
        capture(app, *level, canvas, output, "1p-death", records, false);
        const float outline_at = (presentation.respawn_redraw().draw_from +
            presentation.respawn_redraw().fill_at) * 0.5f;
        while (player.state() == PlayerState::dead && player.respawn_progress() < outline_at) { level->update(inputs, dt); }
        require(player.state() == PlayerState::dead, "outline phase missed the respawn wait");
        capture(app, *level, canvas, output, "1p-redraw-outline", records);
        player.set_pose({ Vector2F::ZERO, 0.0f, false, true });
        capture(app, *level, canvas, output, "1p-story-dead-whole", records);
        player.set_pose({});
        while (player.state() == PlayerState::dead && player.respawn_progress() < presentation.respawn_redraw().fill_at)
        {
            level->update(inputs, dt);
        }
        require(player.state() == PlayerState::dead, "filled phase missed the respawn wait");
        capture(app, *level, canvas, output, "1p-redraw-filled", records);
        for (int tick = 0; tick < 600 && player.state() == PlayerState::dead; ++tick) { level->update(inputs, dt); }
        require(player.state() == PlayerState::alive, "redraw did not respawn within 600 ticks");
        capture(app, *level, canvas, output, "1p-respawn", records);
        require(records.good(), "capture record write failed");
        std::cout << "PASS real builder roster; 1P/4P 720p/1080p/QHD; three palettes; "
            "five action clips and five tool mechanisms, distinct pinned body/tool rasters; "
            "paid fire/hold/release/recovery, hose/bucket travel, roller lift/travel; "
            "moving aim quadrants and both facings; four same-team accents; frozen motion/redraw; "
            "raster culling bounds; death/outline/fill/respawn; tool-free and whole story poses\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
