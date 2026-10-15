#include "game/pch.h"
#include "engine/app/application.h"
#include "game/input_slots.h"
#include "game/objects/level_builder.h"
#include "game/objects/projectile_spray.h"
#include "game/objects/weapon_feedback.h"
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

    struct RestoreFlag
    {
        bool& setting;
        bool value;
        ~RestoreFlag() { this->setting = this->value; }
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
            require(tool.feedback() != nullptr, "real builder omitted weapon feedback");
            const WeaponFeedbackState& weapon_feedback = tool.feedback()->state();
            const CombatFeedback& combat = player.combat_feedback();
            const RectangleF death = combat.death_rectangle();
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
                mechanism.mist_tail_remaining, weapon_feedback.muzzle_remaining,
                weapon_feedback.empty_remaining, weapon_feedback.recovered_remaining, combat.hurt_remaining(),
                combat.elimination_remaining(), death.x, death.y, death.width, death.height });
            const auto effects = player.movement_feedback().effects();
            values.push_back(static_cast<float>(effects.size()));
            for (const auto& effect : effects)
            {
                const Colour paint = effect.paint_colour.value_or(Colour());
                values.insert(values.end(), { static_cast<float>(effect.kind),
                    effect.position.x, effect.position.y, effect.age, effect.strength,
                    effect.direction, effect.paint_colour.has_value() ? 1.0f : 0.0f,
                    paint.r, paint.g, paint.b, paint.a });
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
        Renderer& renderer, const Canvas& canvas, Presentation& presentation)
    {
        // A paid-shot pulse must not stand in for a changed mechanism pose.
        auto& look = const_cast<WeaponFeedbackPresentation&>(presentation.weapon_feedback());
        const RestoreFlag restore{ look.enabled, look.enabled };
        look.enabled = false;
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
                << player.primary_weapon().motion_state().mist_tail_remaining << ','
                << player.primary_weapon().feedback()->state().muzzle_remaining << ','
                << player.primary_weapon().feedback()->state().empty_remaining << ','
                << player.primary_weapon().feedback()->state().recovered_remaining << ','
                << player.combat_feedback().hurt_remaining() << ','
                << player.combat_feedback().elimination_remaining() << ','
                << player.combat_feedback().death_rectangle().x << ','
                << player.combat_feedback().death_rectangle().y << ','
                << player.combat_feedback().death_rectangle().width << ','
                << player.combat_feedback().death_rectangle().height << '\n';
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
        const Viewport& viewport, Renderer& renderer, const Canvas& canvas,
        Presentation& presentation)
    {
        auto& look = const_cast<CombatFeedbackPresentation&>(presentation.combat_feedback());
        const RestoreFlag restore{ look.enabled, look.enabled };
        look.enabled = false;
        const Camera live_camera = player.camera();
        player.set_pose({ anchor.position() - player.rectangle().position(), 0, false, false });
        player.set_camera(camera);
        const Frame result = isolated(&player, player, viewport, renderer, canvas);
        player.set_camera(live_camera);
        player.set_pose({});
        return result;
    }

    // Compare only the draw switch at exactly the same simulation state, so
    // action frames, camera following and the respawn pen cannot supply the
    // visible difference. Restore the authored setting even on a failed check.
    void compare_combat(Application& app, Level& level, const Canvas& canvas,
        Presentation& presentation, bool expect_delta)
    {
        auto& look = const_cast<CombatFeedbackPresentation&>(presentation.combat_feedback());
        const RestoreFlag restore{ look.enabled, look.enabled };
        const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
        const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Player& player = level.player(view);
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            look.enabled = true;
            const Frame enabled = isolated(&player, player, viewport, *app.renderer(), canvas);
            require(enabled.pixels == isolated(&player, player, viewport, *app.renderer(), canvas).pixels,
                "frozen combat feedback changed during repeated drawing");
            look.enabled = false;
            const Frame disabled = isolated(&player, player, viewport, *app.renderer(), canvas);
            require(disabled.pixels == isolated(&player, player, viewport, *app.renderer(), canvas).pixels,
                "disabled combat feedback changed during repeated drawing");
            require((enabled.pixels != disabled.pixels) == expect_delta,
                "combat feedback draw switch did not match active/expired or whole-story state");
        }
        require(before == observation(level), "combat feedback drawing changed observed simulation/timers");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
            "combat feedback drawing changed the replay fingerprint");
#endif
    }

    void action_frames(Application& app, Level& level, const Canvas& canvas,
        const std::filesystem::path& output, const std::string& stem,
        std::ofstream& records, const std::vector<PlayerInputData>& inputs,
        PlayerAnimationState expected, Presentation& presentation)
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
                app.viewport_manager()->player_viewport(view), *app.renderer(), canvas, presentation));
        }
        capture(app, level, canvas, output, stem + "-frame0", records);
        if (expected == PlayerAnimationState::hurt || expected == PlayerAnimationState::elimination)
        {
            compare_combat(app, level, canvas, presentation, true);
        }
        for (int tick = 0; tick < 30 && level.player(0).animation_state() == expected &&
            level.player(0).action_frame() == 0; ++tick) { level.update(inputs, dt); }
        for (int view = 0; view < level.player_count(); ++view)
        {
            Player& player = const_cast<Player&>(level.player(view));
            require(player.animation_state() == expected && player.action_frame() == 1,
                "action skipped its second frame");
            const Frame second = pinned_body(player, anchors[static_cast<size_t>(view)],
                cameras[static_cast<size_t>(view)], app.viewport_manager()->player_viewport(view),
                *app.renderer(), canvas, presentation);
            require(second.pixels != first[static_cast<size_t>(view)].pixels,
                "first and second action body frames produced identical playing-size raster output");
        }
        capture(app, level, canvas, output, stem + "-frame1", records);
    }

    void action_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        const GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output,
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
            PlayerAnimationState::taking_off, presentation);
        for (int tick = 0; tick < 150 &&
            level->player(0).animation_state() != PlayerAnimationState::landing; ++tick)
        {
            level->update(inputs, dt);
        }
        action_frames(app, *level, canvas, output, prefix + "land", records, inputs,
            PlayerAnimationState::landing, presentation);
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
            PlayerAnimationState::firing, presentation);
        for (int view = 0; view < count; ++view)
        {
            Player& player = const_cast<Player&>(level->player(view));
            const ProjectileSpray hit(RectangleF(0, 0, 10, 10), Vector2F::ZERO,
                player.team() == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a,
                0, Colour::white, app.render_resources(), &tuning.projectile(ProjectileType::spray));
            player.on_contact(hit, Vector2F::ZERO, 0);
        }
        action_frames(app, *level, canvas, output, prefix + "hurt", records, inputs,
            PlayerAnimationState::hurt, presentation);
        for (int view = 0; view < count; ++view)
        {
            require(level->kill_player(view), "could not request action-matrix elimination");
        }
        level->update(inputs, dt);
        action_frames(app, *level, canvas, output, prefix + "elimination", records, inputs,
            PlayerAnimationState::elimination, presentation);
    }

    void combat_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        const GameTuning& tuning, Presentation& presentation,
        const std::filesystem::path& output, std::ofstream& records)
    {
        auto level = build(app, builder, 4, TeamColourPair::blue_orange);
        const auto inputs = neutral_inputs();
        settle(*level, inputs);
        const auto phase = [&](const char* name, bool active)
        {
            for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
            {
                resize(app, *level, size);
                compare_combat(app, *level, canvas, presentation, active);
                capture(app, *level, canvas, output, std::string("4p-combat-") + name + "-" +
                    std::to_string(size.x), records,
                    active || level->player(0).state() == PlayerState::alive);
            }
        };
        for (int view = 0; view < level->player_count(); ++view)
        {
            Player& player = const_cast<Player&>(level->player(view));
            const ProjectileSpray hit(RectangleF(0, 0, 10, 10), Vector2F::ZERO,
                player.team() == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a,
                0, Colour::white, app.render_resources(), &tuning.projectile(ProjectileType::spray));
            player.on_contact(hit, Vector2F::ZERO, 0);
        }
        phase("hurt-initial", true);
        while (level->player(0).combat_feedback().hurt_remaining() >
            presentation.combat_feedback().hurt_duration * 0.5f) { level->update(inputs, dt); }
        require(level->player(0).combat_feedback().hurt_remaining() > 0.0f,
            "hurt recovery fixture expired before its comparison");
        phase("hurt-recovery", true);
        while (level->player(0).combat_feedback().hurt_remaining() > 0.0f) { level->update(inputs, dt); }
        compare_combat(app, *level, canvas, presentation, false);

        for (int view = 0; view < level->player_count(); ++view)
        {
            require(level->kill_player(view), "could not request combat-matrix elimination");
        }
        level->update(inputs, dt);
        phase("elimination-initial", true);
        for (int view = 0; view < level->player_count(); ++view)
        {
            const_cast<Player&>(level->player(view)).set_pose({ Vector2F::ZERO, 0, false, true });
        }
        compare_combat(app, *level, canvas, presentation, false);
        for (int view = 0; view < level->player_count(); ++view)
        {
            const_cast<Player&>(level->player(view)).set_pose({});
        }
        while (level->player(0).combat_feedback().elimination_remaining() >
            presentation.combat_feedback().elimination_duration * 0.5f) { level->update(inputs, dt); }
        phase("elimination-middle", true);
        while (level->player(0).combat_feedback().elimination_remaining() >
            presentation.combat_feedback().elimination_duration * 0.15f) { level->update(inputs, dt); }
        require(level->player(0).combat_feedback().elimination_remaining() > 0.0f,
            "elimination late fixture expired before its comparison");
        phase("elimination-late", true);
        while (level->player(0).combat_feedback().elimination_remaining() > 0.0f) { level->update(inputs, dt); }
        phase("elimination-expired", false);
        std::cout << "PASS controlled 4P combat on/off rasters at 720p/1080p/QHD; "
            "hurt initial/recovery and expiry; elimination initial/middle/late and expiry; whole-story suppression\n";
    }

    void compare_movement(Application& app, Level& level, const Canvas& canvas,
        Presentation& presentation, bool expect_delta)
    {
        auto& look = const_cast<MovementFeedbackPresentation&>(presentation.movement_feedback());
        const RestoreFlag restore{ look.enabled, look.enabled };
        const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
        const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Player& player = level.player(view);
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            look.enabled = true;
            const Frame enabled = isolated(&player, player, viewport, *app.renderer(), canvas);
            require(enabled.pixels == isolated(&player, player, viewport, *app.renderer(), canvas).pixels,
                "frozen movement feedback changed during repeated drawing");
            look.enabled = false;
            const Frame disabled = isolated(&player, player, viewport, *app.renderer(), canvas);
            require(disabled.pixels == isolated(&player, player, viewport, *app.renderer(), canvas).pixels,
                "disabled movement feedback changed during repeated drawing");
            require((enabled.pixels != disabled.pixels) == expect_delta,
                "movement contact draw switch did not match the active/expired phase");
        }
        require(before == observation(level), "movement drawing changed direction, tint or simulation state");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
            "movement drawing changed the replay fingerprint");
#endif
    }

    // Compare only the new draw switch, with the actual tool and player still
    // supplying the pose, tint, pivot and nozzle. Existing action/mechanism
    // animation cannot supply this raster delta.
    void compare_weapon_feedback(Application& app, Level& level, const Canvas& canvas,
        Presentation& presentation, bool active, bool muzzle)
    {
        auto& look = const_cast<WeaponFeedbackPresentation&>(presentation.weapon_feedback());
        const RestoreFlag restore{ look.enabled, look.enabled };
        const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
        const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Player& player = level.player(view);
            const Weapon& tool = player.primary_weapon();
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            look.enabled = true;
            const Frame enabled = isolated(&player, player, viewport, *app.renderer(), canvas);
            require(enabled.pixels == isolated(&player, player, viewport, *app.renderer(), canvas).pixels,
                "frozen weapon feedback changed during repeated drawing");
            look.enabled = false;
            const Frame disabled = isolated(&player, player, viewport, *app.renderer(), canvas);
            require(disabled.pixels == isolated(&player, player, viewport, *app.renderer(), canvas).pixels,
                "disabled weapon feedback changed during repeated drawing");
            const bool visible = active && (!muzzle || tool.type() != WeaponType::roller);
            require((enabled.pixels != disabled.pixels) == visible,
                "weapon feedback draw switch did not match active/expired or roller state");
            if (visible && muzzle)
            {
                const RectangleI pane = viewport.pixel_rect();
                const Vector2F nozzle = player.camera().calculate_view_position(tool.nozzle_pos()) +
                    Vector2F(static_cast<float>(pane.x), static_cast<float>(pane.y));
                // The pulse is brief and local to the current continuous
                // nozzle, including when aim has changed since its paid shot.
                const float reach = look.muzzle_sizes.at(static_cast<size_t>(tool.type())).length() *
                    player.camera().scale * 2.0f + look.keyline_width * player.camera().scale * 2.0f + 2.0f;
                for (int y = pane.y; y < pane.bottom(); ++y)
                {
                    for (int x = pane.x; x < pane.right(); ++x)
                    {
                        const size_t offset = (static_cast<size_t>(y) * static_cast<size_t>(enabled.width) +
                            static_cast<size_t>(x)) * 4;
                        if (enabled.pixels[offset] == disabled.pixels[offset] &&
                            enabled.pixels[offset + 1] == disabled.pixels[offset + 1] &&
                            enabled.pixels[offset + 2] == disabled.pixels[offset + 2]) { continue; }
                        require(std::abs(static_cast<float>(x) - nozzle.x) <= reach &&
                            std::abs(static_cast<float>(y) - nozzle.y) <= reach,
                            "paid-shot raster did not follow the current actual nozzle");
                    }
                }
            }
        }
        require(before == observation(level), "weapon drawing changed feedback timers or simulation state");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
            "weapon feedback drawing changed the replay fingerprint");
#endif
    }

    // Paid shots prove the producer for every tool. Turning on the following
    // tick must move the still-live pulse with continuous aim and sprite flip.
    void weapon_fire_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        Presentation& presentation, const std::filesystem::path& output,
        std::ofstream& records, int count, TeamColourPair palette,
        WeaponType selected, const char* name)
    {
        auto level = build(app, builder, count, palette, false, false, selected);
        auto inputs = neutral_inputs();
        settle(*level, inputs);
        resize(app, *level, { 1280, 720 });
        for (int quadrant = 0; quadrant < 4; ++quadrant)
        {
            for (int view = 0; view < count; ++view)
            {
                auto& input = inputs[static_cast<size_t>(level->player(view).player_num())];
                input.primary_shoot = false;
                input.shoot_angle = -3.0f * PI / 4.0f + PI * 0.5f *
                    static_cast<float>((quadrant + view) % 4);
                input.shoot_direction = Vector2F::unit_vec_from_angle(input.shoot_angle);
            }
            const auto ready = [&]()
            {
                for (int view = 0; view < count; ++view)
                {
                    const Weapon& tool = level->player(view).primary_weapon();
                    if (!tool.ready_to_shoot() || tool.feedback()->state().muzzle_remaining > 0.0f)
                    {
                        return false;
                    }
                }
                return true;
            };
            for (int tick = 0; tick < 600 && !ready(); ++tick) { level->update(inputs, dt); }
            require(ready(), "weapon visual fixture did not become ready for its paid shot");
            for (int view = 0; view < count; ++view)
            {
                const Player& player = level->player(view);
                Weapon& tool = const_cast<Weapon&>(player.primary_weapon());
                auto& input = inputs[static_cast<size_t>(player.player_num())];
                input.primary_shoot = true;
                const float ammo = tool.ammo();
                // Exercise the actual shared firing workflow but retain its
                // projectiles locally: later quadrants cannot shoot another
                // fixture player or paint the floor under its camera.
                const auto shots = tool.update_and_get_projectiles(input,
                    player.rectangle().center(), player.velocity(), player.facing_right(), dt);
                require(!shots.empty() && tool.ammo() < ammo && tool.shooting_this_update(),
                    "weapon visual fixture did not pay its shot");
                require((tool.feedback()->state().muzzle_remaining > 0.0f) ==
                    (tool.type() != WeaponType::roller), "paid-shot muzzle timer did not match the tool");
            }
            compare_weapon_feedback(app, *level, canvas, presentation, true, true);
            const std::string prefix = std::to_string(count) + "p-weapon-" + name + '-' +
                TeamColourTools::palette_key(palette) + "-q" + std::to_string(quadrant + 1);
            capture(app, *level, canvas, output, prefix + "-paid-shot", records);
            for (auto& input : inputs)
            {
                input.primary_shoot = false;
                input.shoot_angle = std::atan2(std::sin(input.shoot_angle + PI),
                    std::cos(input.shoot_angle + PI));
                input.shoot_direction = Vector2F::unit_vec_from_angle(input.shoot_angle);
            }
            for (int view = 0; view < count; ++view)
            {
                const Player& player = level->player(view);
                Weapon& tool = const_cast<Weapon&>(player.primary_weapon());
                const auto shots = tool.update_and_get_projectiles(
                    inputs[static_cast<size_t>(player.player_num())], player.rectangle().center(),
                    player.velocity(), player.facing_right(), dt);
                require(shots.empty(), "released visual fixture unexpectedly launched a projectile");
            }
            compare_weapon_feedback(app, *level, canvas, presentation, true, true);
            capture(app, *level, canvas, output, prefix + "-aim-follow", records);
        }
    }

    // The paid-shot matrix above proves live producers; this fixture controls
    // only the cosmetic event age so all views/resolutions see identical
    // initial, fading and expired pulses. Empty/recovery gameplay edges have
    // their own CPU acceptance checks.
    void weapon_feedback_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        Presentation& presentation, const std::filesystem::path& output, std::ofstream& records)
    {
        auto& look = const_cast<WeaponFeedbackPresentation&>(presentation.weapon_feedback());
        const RestoreFlag restore{ look.enabled, look.enabled };
        look.enabled = true;
        for (const auto& tool : std::array<std::pair<WeaponType, const char*>, 5>{{
            { WeaponType::sprayer, "sprayer" }, { WeaponType::sniper, "sniper" },
            { WeaponType::roller, "roller" }, { WeaponType::mister, "mister" },
            { WeaponType::bucket, "bucket" } }})
        {
            weapon_fire_matrix(app, builder, canvas, presentation, output, records,
                1, TeamColourPair::blue_orange, tool.first, tool.second);
        }
        for (const TeamColourPair palette : { TeamColourPair::blue_orange,
            TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            weapon_fire_matrix(app, builder, canvas, presentation, output, records,
                4, palette, WeaponType::none, "mixed");
        }
        auto level = build(app, builder, 4, TeamColourPair::blue_orange);
        auto inputs = neutral_inputs();
        settle(*level, inputs);
        for (int view = 0; view < level->player_count(); ++view)
        {
            auto& input = inputs[static_cast<size_t>(level->player(view).player_num())];
            input.shoot_angle = -3.0f * PI / 4.0f + PI * 0.5f * static_cast<float>(view);
            input.shoot_direction = Vector2F::unit_vec_from_angle(input.shoot_angle);
        }
        level->update(inputs, dt);
        for (int event = 0; event < 3; ++event)
        {
            const char* name = event == 0 ? "muzzle" : event == 1 ? "empty" : "recovered";
            const float duration = event == 0 ? look.muzzle_duration :
                event == 1 ? look.empty_duration : look.recovered_duration;
            const auto emit = [&](float age)
            {
                for (int view = 0; view < level->player_count(); ++view)
                {
                    const Weapon& tool = level->player(view).primary_weapon();
                    auto& feedback = const_cast<WeaponFeedback&>(*tool.feedback());
                    feedback.clear();
                    if (event == 0) { feedback.shot(tool.type()); }
                    else if (event == 1) { feedback.empty(); }
                    else { feedback.recovered(); }
                    feedback.update(age);
                }
            };
            for (int phase = 0; phase < 3; ++phase)
            {
                emit(duration * static_cast<float>(phase) * 0.5f);
                const bool active = phase < 2;
                const char* phase_name = phase == 0 ? "initial" : phase == 1 ? "middle" : "expired";
                for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
                {
                    resize(app, *level, size);
                    compare_weapon_feedback(app, *level, canvas, presentation, active, event == 0);
                    const std::string prefix = std::string("4p-weapon-controlled-") + name + '-' + phase_name +
                        '-' + std::to_string(size.x);
                    capture(app, *level, canvas, output, prefix + "-on", records);
                    look.enabled = false;
                    capture(app, *level, canvas, output, prefix + "-off", records);
                    look.enabled = true;
                }
            }
        }
        std::cout << "PASS paid-shot 1P five tools and 4P three palettes; four aim quadrants and live aim/flip follow; "
            "controlled 4P muzzle/empty/recovered initial/middle/expired on/off rasters at 720p/1080p/QHD; "
            "frozen feedback timers, redraw and culling bounds\n";
    }

    // A real walk and reversal exercise the completed Scene contact phase.
    // Use the broad, unpainted recovery floor rather than a spawn near a ledge.
    void resolved_contacts(Application& app, LevelBuilder& builder, const Canvas& canvas,
        const GameTuning& tuning, const std::filesystem::path& output, std::ofstream& records)
    {
        auto level = build(app, builder, 4, TeamColourPair::blue_orange);
        auto inputs = neutral_inputs();
        settle(*level, inputs);
        resize(app, *level, { 1280, 720 });
        for (int view = 0; view < level->player_count(); ++view)
        {
            require(level->place_player(view, Vector2F(1200.0f + 200.0f * static_cast<float>(view), 1275)),
                "could not place the resolved-contact fixture on the recovery floor");
        }
        for (int tick = 0; tick < 30; ++tick) { level->update(inputs, dt); }
        for (int view = 0; view < level->player_count(); ++view)
        {
            inputs[static_cast<size_t>(level->player(view).player_num())].x_movement =
                view % 2 == 0 ? 1.0f : -1.0f;
        }
        const auto all_emitted = [&](MovementFeedbackKind kind)
        {
            for (int view = 0; view < level->player_count(); ++view)
            {
                const auto effects = level->player(view).movement_feedback().effects();
                if (!std::any_of(effects.begin(), effects.end(), [kind](const auto& effect)
                    { return effect.kind == kind && !effect.paint_colour.has_value(); })) { return false; }
            }
            return true;
        };
        for (int tick = 0; tick < 60 && !all_emitted(MovementFeedbackKind::step); ++tick)
        {
            level->update(inputs, dt);
        }
        require(all_emitted(MovementFeedbackKind::step), "resolved walking did not produce four dry contact particles");
        capture(app, *level, canvas, output, "4p-contact-resolved-dry-step", records);
        const auto all_at_skid_speed = [&]()
        {
            for (int view = 0; view < level->player_count(); ++view)
            {
                if (std::abs(level->player(view).velocity().x) < tuning.player().movement_sounds.skid_min_speed)
                {
                    return false;
                }
            }
            return true;
        };
        for (int tick = 0; tick < 60 && !all_at_skid_speed(); ++tick) { level->update(inputs, dt); }
        require(all_at_skid_speed(), "resolved walking never reached the skid threshold");
        for (auto& input : inputs) { input.x_movement = -input.x_movement; }
        level->update(inputs, dt);
        require(all_emitted(MovementFeedbackKind::skid), "resolved reversal did not produce four dry skid particles");
        capture(app, *level, canvas, output, "4p-contact-resolved-dry-skid", records);
    }

    // Only this geometry/timing matrix emits directly: the real walk/reversal
    // above proves the producer. Each pane retains its real player, camera,
    // tool and culling path; views 1/2 are dry right/left, 3/4 painted right/left.
    void contact_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        const GameTuning& tuning, Presentation& presentation,
        const std::filesystem::path& output, std::ofstream& records)
    {
        auto& look = const_cast<MovementFeedbackPresentation&>(presentation.movement_feedback());
        const RestoreFlag restore{ look.enabled, look.enabled };
        look.enabled = true;
        resolved_contacts(app, builder, canvas, tuning, output, records);
        for (const TeamColourPair palette : { TeamColourPair::blue_orange,
            TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            auto level = build(app, builder, 4, palette);
            const auto inputs = neutral_inputs();
            settle(*level, inputs);
            for (const MovementFeedbackKind kind : { MovementFeedbackKind::step, MovementFeedbackKind::skid })
            {
                const float duration = kind == MovementFeedbackKind::step ? look.step.duration : look.skid.duration;
                const auto emit = [&](int view, bool reverse, bool swap_paint, float age)
                {
                    const Player& player = level->player(view);
                    auto& feedback = const_cast<MovementFeedback&>(player.movement_feedback());
                    const RectangleF body = player.rectangle();
                    const float direction = (view % 2 == 0 ? 1.0f : -1.0f) * (reverse ? -1.0f : 1.0f);
                    const bool painted = (view >= 2) != swap_paint;
                    feedback.clear();
                    feedback.contact(kind, Vector2F(body.x + body.width * 0.5f, body.bottom()), direction,
                        painted ? std::optional<Colour>(player.team_colour()) : std::nullopt);
                    if (age > 0.0f) { feedback.update(age); }
                };
                for (int view = 0; view < level->player_count(); ++view) { emit(view, false, false, 0.0f); }
                const std::string prefix = std::string("4p-contact-") +
                    (kind == MovementFeedbackKind::step ? "step-" : "skid-") + TeamColourTools::palette_key(palette);
                const auto phase = [&](const char* name, bool active)
                {
                    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
                    {
                        resize(app, *level, size);
                        compare_movement(app, *level, canvas, presentation, active);
                        capture(app, *level, canvas, output, prefix + '-' + name + '-' + std::to_string(size.x), records);
                    }
                };
                phase("initial", true);
                for (int view = 0; view < level->player_count(); ++view) { emit(view, false, false, duration * 0.5f); }
                phase("middle", true);
                // Compare at the exact same pose, tint, camera and age. A
                // different character, body frame or pane cannot supply either delta.
                for (int view = 0; view < level->player_count(); ++view)
                {
                    const Player& player = level->player(view);
                    const Viewport& viewport = app.viewport_manager()->player_viewport(view);
                    const Frame normal = isolated(&player, player, viewport, *app.renderer(), canvas);
                    emit(view, true, false, duration * 0.5f);
                    const Frame reversed = isolated(&player, player, viewport, *app.renderer(), canvas);
                    require(normal.pixels != reversed.pixels, "contact particle direction produced no visible raster change");
                    emit(view, false, true, duration * 0.5f);
                    const Frame other_surface = isolated(&player, player, viewport, *app.renderer(), canvas);
                    require(normal.pixels != other_surface.pixels, "dry and painted contact particles were indistinguishable");
                    emit(view, false, false, duration);
                    require(player.movement_feedback().effects().empty(), "contact particles did not retire at their duration");
                }
                phase("expired", false);
            }
        }
        std::cout << "PASS resolved dry walk/reversal; controlled 4P step/skid on/off rasters at 720p/1080p/QHD; "
            "three palettes, dry/painted and left/right; initial/middle/expired; frozen draw and culling bounds\n";
    }

    // Real paid shots and resolved movement select the tool poses. Capture the
    // release and airborne phrases too: additional render views must not age
    // either one, or leave cosmetic pixels outside the scene's cull bounds.
    void tool_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        const std::filesystem::path& output, std::ofstream& records, int count,
        TeamColourPair palette, bool right, WeaponType tool, const std::string& name,
        Presentation& presentation)
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
                app.viewport_manager()->player_viewport(view), *app.renderer(), canvas, presentation));
        }
        const auto check_mechanisms = [&]()
        {
            for (int view = 0; view < count; ++view)
            {
                const Player& player = level->player(view);
                if (player.primary_weapon().visual_pose() == 0) { continue; }
                const Frame image = isolated_tool(player,
                    app.viewport_manager()->player_viewport(view), *app.renderer(), canvas, presentation);
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
        records << "capture,view,input_slot,painter_pixels,facing_right,tool_angle,respawn_progress,animation_state,action_age,action_frame,tool_pose,pressure,valve,travel_phase,mist_tail_remaining,muzzle_remaining,empty_remaining,recovered_remaining,hurt_remaining,elimination_remaining,death_x,death_y,death_width,death_height\n";
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
                    TeamColourPair::blue_orange, right, tool.first, tool.second, presentation);
            }
        }
        for (const TeamColourPair palette : { TeamColourPair::blue_orange,
            TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            tool_matrix(app, builder, canvas, output, records, 4, palette, true,
                WeaponType::none, "mixed", presentation);
            tool_matrix(app, builder, canvas, output, records, 4, palette, false,
                WeaponType::roller, "roller", presentation);
        }
        for (const bool right : { true, false })
        {
            action_matrix(app, builder, canvas, tuning, presentation, output, records, 1,
                TeamColourPair::blue_orange, right);
        }
        for (const TeamColourPair palette : { TeamColourPair::blue_orange,
            TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            action_matrix(app, builder, canvas, tuning, presentation, output, records, 4, palette,
                palette != TeamColourPair::blue_orange);
        }
        combat_matrix(app, builder, canvas, tuning, presentation, output, records);
        weapon_feedback_matrix(app, builder, canvas, presentation, output, records);
        contact_matrix(app, builder, canvas, tuning, presentation, output, records);
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
        for (int tick = 0; tick < 60 && (player.animation_state() == PlayerAnimationState::elimination ||
            player.combat_feedback().elimination_remaining() > 0.0f); ++tick)
        {
            level->update(inputs, dt);
        }
        require(player.animation_state() == PlayerAnimationState::idle &&
            player.combat_feedback().elimination_remaining() == 0.0f,
            "elimination pose/burst did not finish before the outline redraw");
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
