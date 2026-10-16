#include "game/pch.h"
#include "engine/app/application.h"
#include "game/input_slots.h"
#include "game/objects/level_builder.h"
#include "game/objects/projectile_spray.h"
#include "game/objects/weapon_feedback.h"
#include "game/objects/blocked_shot_feedback.h"
#include "game/objects/player_badge.h"
#include "game/objects/countdown_art.h"
#include "game/objects/reveal_art.h"
#include "game/states/menu_panel.h"
#include "game/states/game_states.h"
#include "game/states/end_menu.h"
#include "game/states/pause_menu.h"
#include "game/states/results_menu.h"
#include "game/states/main_menu.h"
#include "game/states/controls_settings_menu.h"
#include "game/states/stick_settings_menu.h"
#include "game/objects/stick_response.h"
#include "game/build_info.h"
#include "game/save.h"
#include "game/debug/dev_tools.h"
#if COLOURWARS_DEV_TOOLS
#include "game/debug/match_fingerprint.h"
#include "game/debug/debug_director_impl.h"
#endif

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

#if defined(COLOURWARS_PAINTER_SCRIPTED_PADS)
// This focused probe supplies the concrete input backend at the link seam.
// Normal painter binaries retain Labrador's XInput implementation.
namespace painter_scripted_pads
{
    std::array<labrador::GamepadState, labrador::Gamepads::max_count> states{};
}
namespace labrador
{
    struct GamepadReader::Impl
    {
        std::array<GamepadState, Gamepads::max_count> states{};
        bool suspended = false;
    };
    GamepadReader::GamepadReader() : impl_(std::make_unique<Impl>()) {}
    GamepadReader::~GamepadReader() = default;
    GamepadReader::GamepadReader(GamepadReader&&) noexcept = default;
    GamepadReader& GamepadReader::operator=(GamepadReader&&) noexcept = default;
    void GamepadReader::sample()
    {
        this->impl_->states = this->impl_->suspended ?
            std::array<GamepadState, Gamepads::max_count>{} : painter_scripted_pads::states;
    }
    GamepadState GamepadReader::read(int slot) const
    {
        return slot >= 0 && slot < Gamepads::max_count ?
            this->impl_->states[static_cast<size_t>(slot)] : GamepadState{};
    }
    void GamepadReader::suspend() { this->impl_->suspended = true; }
    void GamepadReader::resume() { this->impl_->suspended = false; }
}
#endif

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

    std::vector<float> blocked_observation(const BlockedShotFeedback& feedback)
    {
        std::vector<float> values{ static_cast<float>(feedback.cues().size()) };
        for (const auto& cue : feedback.cues())
        {
            const auto& contact = cue.contact;
            values.insert(values.end(), { contact.position.x, contact.position.y,
                contact.normal.x, contact.normal.y, contact.team_colour.r,
                contact.team_colour.g, contact.team_colour.b, contact.team_colour.a,
                static_cast<float>(contact.player_num), cue.age });
        }
        for (int slot = 0; slot < input_slots::count; ++slot)
        {
            values.push_back(feedback.repeat_remaining(slot));
        }
        return values;
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
            const RectangleF identity = level.player_identity_bounds(view);
            const QuietCharacter::State& quiet = player.quiet_character().state();
            values.insert(values.end(), { body.x, body.y, body.width, body.height,
                player.velocity().x, player.velocity().y, player.health(), player.weapon_ammo(),
                player.respawn_timer(), static_cast<float>(player.state()),
                static_cast<float>(player.move_state()), player.facing_right() ? 1.0f : 0.0f,
                tool.rotation(), tool.nozzle_pos().x, tool.nozzle_pos().y,
                static_cast<float>(player.animation_state()), player.action_age(),
                static_cast<float>(player.action_frame()),
                static_cast<float>(tool.visual_pose()), static_cast<float>(tool.readiness()), mechanism.pressure,
                mechanism.valve, mechanism.travel_phase, mechanism.lift,
                mechanism.compression, mechanism.bucket_swing, mechanism.hose_sway,
                mechanism.mist_tail_remaining, weapon_feedback.muzzle_remaining,
                weapon_feedback.empty_remaining, weapon_feedback.recovered_remaining, combat.hurt_remaining(),
                combat.elimination_remaining(), death.x, death.y, death.width, death.height,
                quiet.breath, quiet.lean, quiet.blinking ? 1.0f : 0.0f });
            values.insert(values.end(), { level.player_identity_visible(view) ? 1.0f : 0.0f,
                level.player_identity_emphasis(view), identity.x, identity.y, identity.width, identity.height });
            const IdleDrip* drip = level.idle_drip(view);
            values.push_back(drip != nullptr ? 1.0f : 0.0f);
            if (drip != nullptr)
            {
                const RectangleF bead = drip->bead_rectangle();
                values.insert(values.end(), { drip->attached() ? 1.0f : 0.0f,
                    drip->bead_age(), drip->interval_remaining(), bead.x, bead.y,
                    bead.width, bead.height, static_cast<float>(drip->drops().size()) });
                for (const auto& drop : drip->drops())
                {
                    values.insert(values.end(), { drop.position.x, drop.position.y, drop.age });
                }
            }
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
        const auto blocked = blocked_observation(level.blocked_shot_feedback());
        values.insert(values.end(), blocked.begin(), blocked.end());
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
        explicit Canvas(RenderResources* resources, const Colour& colour = Colour(0.72f, 0.75f, 0.78f)) :
            TextureObject("sprite_sheet_1", "pixel", resources, colour) {}
        using TextureObject::draw;
    };

    Frame isolated(const Player* player, const Player& camera_player,
        const Viewport& viewport, Renderer& renderer, const Canvas& canvas,
        const IdleDrip* drip = nullptr)
    {
        renderer.begin_frame();
        renderer.set_view_count(1);
        DrawList list = renderer.view(0);
        list.set_viewport(viewport);
        list.set_camera(camera_player.camera());
        list.set_filter(TextureFilter::point);
        canvas.draw(list, camera_player.camera().visible_rectangle(viewport));
        if (player != nullptr) { player->draw(list); }
        if (drip != nullptr) { drip->draw(list); }
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
    size_t check_projected_bounds(const Frame& image, const Frame& blank,
        RectangleF bounds, const Viewport& viewport, Frame& crop)
    {
        const RectangleI pane = viewport.pixel_rect();
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

    size_t check_bounds(const Frame& image, const Frame& blank, const Player& player,
        const Viewport& viewport, Frame& crop)
    {
        return check_projected_bounds(image, blank, player.camera().calculate_view_rectangle(
            player.cull_bounds(1.0f / player.camera().scale)), viewport, crop);
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
                << player.combat_feedback().death_rectangle().height << ','
                << player.quiet_character().state().breath << ','
                << player.quiet_character().state().lean << ','
                << player.quiet_character().state().blinking << '\n';
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
        WeaponType tool = WeaponType::none, OpeningMode opening = OpeningMode::off)
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
        auto level = builder.build_level(settings, palette, opening);
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

    Frame blocked_context(const Player& player, const BlockedShotFeedback& feedback,
        const Camera& camera, const Viewport& viewport, const Vector2F& contact,
        const Vector2F& normal, Renderer& renderer, const Canvas& canvas, const Canvas& cover)
    {
        renderer.begin_frame();
        renderer.set_view_count(1);
        DrawList list = renderer.view(0);
        list.set_viewport(viewport);
        list.set_camera(camera);
        list.set_filter(TextureFilter::point);
        canvas.draw(list, camera.visible_rectangle(viewport));
        player.draw(list);
        // The solid begins at the resolved face and overlaps the actual tool
        // nozzle. Drawing feedback last matches Level's world-overlay order.
        const Vector2F size = std::abs(normal.x) > 0.5f ? Vector2F(28, 56) : Vector2F(56, 28);
        const Vector2F center = contact - normal * 14.0f;
        cover.draw(list, RectangleF(center - size * 0.5f, size));
        feedback.draw(list);
        return read(renderer);
    }

    // Retain surrounding cover and the tool at native playing size. The
    // difference crop alone would lose the overlap that this cue must explain.
    Frame blocked_context_crop(const Frame& frame, const Camera& camera,
        const Viewport& viewport, const Vector2F& contact)
    {
        const RectangleI pane = viewport.pixel_rect();
        const Vector2F projected = camera.calculate_view_position(contact);
        const int center_x = pane.x + static_cast<int>(projected.x);
        const int center_y = pane.y + static_cast<int>(projected.y);
        const int left = std::max(pane.x, center_x - 90);
        const int top = std::max(pane.y, center_y - 70);
        const int right = std::min(pane.right(), center_x + 90);
        const int bottom = std::min(pane.bottom(), center_y + 70);
        Frame crop{ right - left, bottom - top, {} };
        for (int y = top; y < bottom; ++y)
        {
            const size_t start = (static_cast<size_t>(y) * static_cast<size_t>(frame.width) +
                static_cast<size_t>(left)) * 4;
            crop.pixels.insert(crop.pixels.end(), frame.pixels.begin() + start,
                frame.pixels.begin() + start + static_cast<size_t>(crop.width) * 4);
        }
        return crop;
    }

    void blocked_level_capture(Application& app, Level& level, Presentation& presentation,
        const std::array<Vector2F, 4>& normals, const std::filesystem::path& output,
        const std::string& stem)
    {
        auto& look = const_cast<BlockedShotPresentation&>(presentation.blocked_shot());
        auto& feedback = const_cast<BlockedShotFeedback&>(level.blocked_shot_feedback());
        feedback.clear();
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Player& player = level.player(view);
            feedback.emit({ player.primary_weapon().nozzle_pos(), normals.at(static_cast<size_t>(view)),
                player.team_colour(), player.player_num() });
        }
        require(feedback.cues().size() == 4, "Level blocked-shot fixture did not retain four contacts");
        const RectangleF bounds = feedback.bounds();
        const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
        const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
        const Frame enabled = render(level, *app.renderer());
        require(enabled.pixels == render(level, *app.renderer()).pixels,
            "Frozen Level blocked-shot redraw changed pixels");
        look.enabled = false;
        const Frame disabled = render(level, *app.renderer());
        require(disabled.pixels == render(level, *app.renderer()).pixels,
            "Disabled Level blocked-shot redraw changed pixels");
        look.enabled = true;
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            Frame crop{};
            require(check_projected_bounds(enabled, disabled,
                level.view_camera(view).calculate_view_rectangle(bounds), viewport, crop) > 0,
                "Level did not draw detached blocked-shot contacts in a live world pane");
            save(crop, output / (stem + "-player" + std::to_string(view + 1) + ".ppm"));
        }
        require(before == observation(level), "Level blocked-shot drawing changed simulation or cue timers");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
            "Level blocked-shot drawing changed replay fingerprint");
#endif
        feedback.clear();
    }

    // CPU acceptance exercises real blocked launches and resolved contacts.
    // Here the event age and cover orientation are controlled so hardware
    // rasters can establish visibility, projection and draw-only invariance.
    void blocked_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        Presentation& presentation, const std::filesystem::path& output)
    {
        auto& look = const_cast<BlockedShotPresentation&>(presentation.blocked_shot());
        const RestoreFlag restore{ look.enabled, look.enabled };
        look.enabled = true;
        const Canvas cover(app.render_resources(), Colour(0.26f, 0.29f, 0.32f));
        const std::array<Vector2F, 4> normals{ Vector2F(-1, 0), Vector2F(1, 0),
            Vector2F(0, -1), Vector2F(0, 1) };
        const std::array<const char*, 4> normal_names{ "left", "right", "up", "down" };
        std::ofstream records(output / "blocked-shots.csv");
        records << "capture,view,input_slot,normal_x,normal_y,age,changed_pixels,bounds_x,bounds_y,bounds_width,bounds_height\n";
        for (const TeamColourPair palette : { TeamColourPair::blue_orange,
            TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            auto level = build(app, builder, 4, palette);
            const auto inputs = neutral_inputs();
            settle(*level, inputs);
            for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
            {
                if (size.x != 1280 && palette != TeamColourPair::blue_orange) { continue; }
                resize(app, *level, size);
                const std::string prefix = std::string("blocked-4p-") + TeamColourTools::palette_key(palette) +
                    '-' + std::to_string(size.x);
                blocked_level_capture(app, *level, presentation, normals, output, prefix + "-level");
                const auto level_before = observation(*level);
                for (int view = 0; view < level->player_count(); ++view)
                {
                    const Player& player = level->player(view);
                    const Viewport& viewport = app.viewport_manager()->player_viewport(view);
                    const RectangleI pane = viewport.pixel_rect();
                    const Vector2F point = player.primary_weapon().nozzle_pos();
                    const Vector2F normal = normals.at(static_cast<size_t>(view));
                    // Keep the actual gameplay scale while framing the fixed
                    // contact identically; no screenshot crop is enlarged.
                    const float scale = player.camera().scale;
                    const Camera camera(point - Vector2F(static_cast<float>(pane.width) / (2.0f * scale),
                        static_cast<float>(pane.height) / (2.0f * scale)), scale);
                    BlockedShotFeedback feedback(&look, app.render_resources());
                    Frame initial{};
                    for (int phase = 0; phase < 3; ++phase)
                    {
                        const float age = look.duration * static_cast<float>(phase) * 0.5f;
                        feedback.clear();
                        feedback.emit({ point, normal, player.team_colour(), player.player_num() });
                        require(feedback.cues().size() == 1, "Controlled blocked-shot contact was not retained");
                        const RectangleF cue_bounds = feedback.cue_bounds(feedback.cues().front());
                        require(feedback.bounds() == cue_bounds, "Single blocked-shot aggregate omitted its cue bounds");
                        feedback.update(age);
                        const bool active = phase < 2;
                        require(feedback.cues().empty() != active, "Blocked-shot expiry did not match its authored duration");
                        const auto before = blocked_observation(feedback);
                        const auto draw = [&]()
                        {
                            return blocked_context(player, feedback, camera, viewport, point, normal,
                                *app.renderer(), canvas, cover);
                        };
                        const Frame enabled = draw();
                        for (int repeat = 0; repeat < 3; ++repeat)
                        {
                            require(enabled.pixels == draw().pixels,
                                "Frozen blocked-shot context redraw changed pixels");
                        }
                        look.enabled = false;
                        const Frame disabled = draw();
                        require(disabled.pixels == draw().pixels,
                            "Disabled blocked-shot context redraw changed pixels");
                        look.enabled = true;
                        Frame crop{};
                        const size_t changed = check_projected_bounds(enabled, disabled,
                            camera.calculate_view_rectangle(cue_bounds), viewport, crop);
                        require(active ? changed > 0 : changed == 0,
                            "Blocked-shot cover/nozzle overlap visibility did not match its lifetime");
                        const char* phase_name = phase == 0 ? "initial" : phase == 1 ? "middle" : "expired";
                        const std::string stem = prefix + '-' + normal_names.at(static_cast<size_t>(view)) + '-' + phase_name;
                        const std::string suffix = "-player" + std::to_string(view + 1) + ".ppm";
                        if (active) { save(crop, output / (stem + suffix)); }
                        save(blocked_context_crop(enabled, camera, viewport, point), output / (stem + "-context" + suffix));
                        if (phase == 0) { initial = enabled; }
                        if (phase == 1)
                        {
                            require(initial.pixels != enabled.pixels,
                                "Blocked-shot contact did not visibly rebound/fade at midlife");
                        }
                        records << stem << ',' << view + 1 << ',' << player.player_num() << ',' << normal.x << ','
                            << normal.y << ',' << age << ',' << changed << ',' << cue_bounds.x << ',' << cue_bounds.y << ','
                            << cue_bounds.width << ',' << cue_bounds.height << '\n';
                        require(before == blocked_observation(feedback),
                            "Blocked-shot drawing/toggling changed contact snapshot or timers");
                    }
                }
                require(level_before == observation(*level),
                    "Controlled blocked-shot raster fixture changed its real player or Level state");
            }
        }
        require(records.good(), "Blocked-shot capture record write failed");
        std::cout << "PASS detached blocked-shot Level on/off rasters, native 4P cover/nozzle overlap; "
            "four surface normals, three palettes at 720p and representative 1080p/QHD; "
            "initial/middle/expired, projected cue/aggregate bounds, frozen redraw/contact ages and replay fingerprint\n";
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

    // The decoration is a separate Scene object, so its falling footprint must
    // be checked separately from the player's culling box. Keep an additional
    // combined crop for judging the bead at its real nozzle and playing size.
    bool raster_adjacent(const Frame& decoration, const Frame& painter,
        const Frame& blank, const Viewport& viewport)
    {
        const RectangleI pane = viewport.pixel_rect();
        const auto ink = [&](const Frame& image, int x, int y)
        {
            const size_t offset = (static_cast<size_t>(y) * static_cast<size_t>(image.width) +
                static_cast<size_t>(x)) * 4;
            return image.pixels[offset] != blank.pixels[offset] ||
                image.pixels[offset + 1] != blank.pixels[offset + 1] ||
                image.pixels[offset + 2] != blank.pixels[offset + 2];
        };
        for (int y = pane.y; y < pane.bottom(); ++y)
        {
            for (int x = pane.x; x < pane.right(); ++x)
            {
                if (!ink(decoration, x, y)) { continue; }
                for (int neighbour_y = std::max(pane.y, y - 1);
                    neighbour_y <= std::min(pane.bottom() - 1, y + 1); ++neighbour_y)
                {
                    for (int neighbour_x = std::max(pane.x, x - 1);
                        neighbour_x <= std::min(pane.right() - 1, x + 1); ++neighbour_x)
                    {
                        if (ink(painter, neighbour_x, neighbour_y)) { return true; }
                    }
                }
            }
        }
        return false;
    }

    void quiet_capture(Application& app, Level& level, const Canvas& canvas,
        Presentation& presentation, const std::filesystem::path& output,
        const std::string& stem, std::ofstream& records, std::ofstream& quiet_records)
    {
        capture(app, level, canvas, output, stem, records);
        const auto before = observation(level);
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Player& player = level.player(view);
            auto& look = const_cast<IdleDripPresentation&>(presentation.idle_drip(player.primary_weapon().type()));
            const RestoreFlag restore{ look.enabled, look.enabled };
            const IdleDrip* drip = level.idle_drip(view);
            require(drip != nullptr, "wet tool builder omitted its Scene-owned idle drip");
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            const Frame blank = isolated(nullptr, player, viewport, *app.renderer(), canvas);
            look.enabled = true;
            const Frame image = isolated(nullptr, player, viewport, *app.renderer(), canvas, drip);
            require(image.pixels == isolated(nullptr, player, viewport, *app.renderer(), canvas, drip).pixels,
                "frozen idle drip changed during repeated drawing");
            Frame crop{};
            const size_t pixels = check_projected_bounds(image, blank,
                player.camera().calculate_view_rectangle(drip->cull_bounds(1.0f / player.camera().scale)),
                viewport, crop);
            require((pixels > 0) == (drip->attached() || !drip->drops().empty()),
                "idle drip raster visibility did not match its bead/drop state");
            if (drip->attached() && drip->bead_age() >= look.grow_time)
            {
                const Frame painter = isolated(&player, player, viewport, *app.renderer(), canvas);
                require(raster_adjacent(image, painter, blank, viewport),
                    "mature attached idle bead did not meet the rendered wet lip within one pixel");
            }
            look.enabled = false;
            require(blank.pixels == isolated(nullptr, player, viewport, *app.renderer(), canvas, drip).pixels,
                "disabled idle drip still drew pixels");
            look.enabled = true;
            const Frame combined = isolated(&player, player, viewport, *app.renderer(), canvas, drip);
            RectangleF bounds = player.cull_bounds(1.0f / player.camera().scale);
            if (pixels > 0)
            {
                bounds = RectangleF::union_of(bounds, drip->cull_bounds(1.0f / player.camera().scale));
            }
            require(check_projected_bounds(combined, blank,
                player.camera().calculate_view_rectangle(RectangleF(bounds)), viewport, crop) > 0,
                "quiet detail crop omitted its painter");
            save(crop, output / (stem + "-detail-player" + std::to_string(view + 1) + ".ppm"));
            quiet_records << stem << ',' << view + 1 << ',' << drip->attached() << ','
                << drip->bead_age() << ',' << drip->interval_remaining() << ','
                << drip->drops().size() << ',' << pixels << ','
                << player.quiet_character().state().breath << ','
                << player.quiet_character().state().lean << ','
                << player.quiet_character().state().blinking << '\n';
        }
        require(before == observation(level), "quiet detail drawing changed observed simulation or cosmetic state");
    }

    void compare_quiet(Application& app, Level& level, const Canvas& canvas,
        Presentation& presentation, bool blinking)
    {
        const auto before = observation(level);
        auto& enabled = const_cast<bool&>(presentation.figure().quiet.enabled);
        const RestoreFlag restore{ enabled, enabled };
        bool changed = false;
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Player& player = level.player(view);
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            enabled = true;
            const Frame active = isolated(&player, player, viewport, *app.renderer(), canvas);
            enabled = false;
            changed |= active.pixels != isolated(&player, player, viewport, *app.renderer(), canvas).pixels;
        }
        require(changed, "quiet breathing/weight shift produced no visible playing-size raster change");
        enabled = true;
        if (blinking)
        {
            const Player& player = level.player(0);
            auto& blink = const_cast<bool&>(player.quiet_character().state().blinking);
            const RestoreFlag restore_blink{ blink, blink };
            require(blink, "quiet blink fixture missed its closed-eye phase");
            const Viewport& viewport = app.viewport_manager()->player_viewport(0);
            const Frame closed = isolated(&player, player, viewport, *app.renderer(), canvas);
            blink = false;
            require(closed.pixels != isolated(&player, player, viewport, *app.renderer(), canvas).pixels,
                "quiet blink did not change the eyes at playing size");
        }
        require(before == observation(level), "quiet on/off comparison changed observed simulation or cosmetic state");
    }

    void quiet_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        Presentation& presentation, const std::filesystem::path& output, std::ofstream& records)
    {
        std::ofstream quiet_records(output / "quiet-detail.csv");
        quiet_records << "capture,view,attached,bead_age,interval_remaining,drops,drip_pixels,breath,lean,blinking\n";
        const auto& look = presentation.idle_drip();
        for (const int count : { 1, 4 })
        {
            for (const TeamColourPair palette : { TeamColourPair::blue_orange,
                TeamColourPair::pink_green, TeamColourPair::blue_yellow })
            {
                auto level = build(app, builder, count, palette, false, false, WeaponType::sprayer);
                auto inputs = neutral_inputs();
                settle(*level, inputs);
                resize(app, *level, { 1280, 720 });
                for (int view = 0; view < count; ++view)
                {
                    require(level->place_player(view, Vector2F(1200.0f + 200.0f * static_cast<float>(view), 1275)),
                        "could not place the quiet fixture on the recovery floor");
                }
                for (int tick = 0; tick < 60; ++tick) { level->update(inputs, dt); }
                const std::string prefix = std::to_string(count) + "p-quiet-" + TeamColourTools::palette_key(palette);
                const IdleDrip* drip = level->idle_drip(0);
                require(drip != nullptr, "quiet fixture has no sprayer drip");
                const auto advance_until = [&](const auto& condition, const char* message)
                {
                    for (int tick = 0; tick < 900 && !condition(); ++tick) { level->update(inputs, dt); }
                    require(condition(), message);
                    require(level->state() == LevelState::active, "quiet fixture exhausted its real match clock");
                };
                const auto phase = [&](const std::string& name, bool extents = false)
                {
                    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
                    {
                        if (size.x != 1280 && (!extents || palette != TeamColourPair::blue_orange)) { continue; }
                        resize(app, *level, size);
                        quiet_capture(app, *level, canvas, presentation, output,
                            prefix + '-' + name + '-' + std::to_string(size.x), records, quiet_records);
                    }
                    resize(app, *level, { 1280, 720 });
                };
                advance_until([&] { return level->player(0).quiet_character().state().breath >=
                    presentation.figure().quiet.breath_height * 0.9f; },
                    "settled character never reached its breathing crest");
                compare_quiet(app, *level, canvas, presentation, false);
                if (count == 4)
                {
                    require(level->player(0).quiet_character().state() != level->player(1).quiet_character().state(),
                        "separate roster members shared one quiet character phase");
                }
                phase("breath");
                advance_until([&] { return drip->attached() && drip->bead_age() >= look.grow_time * 0.5f &&
                    drip->bead_age() < look.grow_time; }, "settled sprayer never grew a muzzle bead");
                phase("grow");
                advance_until([&] { return drip->attached() &&
                    drip->bead_age() >= look.grow_time + look.stretch_time * 0.5f; },
                    "settled sprayer never stretched its muzzle bead");
                phase("stretch", true);
                advance_until([&] { return !drip->drops().empty() && drip->drops()[0].age >= dt * 2.0f; },
                    "settled sprayer never released a falling cosmetic drop");
                phase("fall");
                const float drop_y = drip->drops()[0].position.y;
                level->update(inputs, dt);
                require(drip->drops().empty() || drip->drops()[0].position.y > drop_y,
                    "released idle drop did not fall downwards");
                advance_until([&] { return level->player(0).quiet_character().state().blinking; },
                    "settled character never blinked");
                compare_quiet(app, *level, canvas, presentation, true);
                phase("blink");
                advance_until([&] { return drip->attached() && drip->bead_age() >= look.grow_time * 0.5f; },
                    "idle bead did not return for the interruption check");
                for (auto& input : inputs) { input.x_movement = -1.0f; }
                level->update(inputs, dt);
                require(!drip->attached(), "starting movement did not remove the attached idle bead");
                phase("interrupted");
                for (int tick = 0; tick < 6; ++tick) { level->update(inputs, dt); }
                phase("lean-left");
            }
        }
        require(quiet_records.good(), "quiet detail capture record write failed");
        std::cout << "PASS actual settled 1P/4P sprayer grow/stretch/fall/interruption and quiet breath/blink/lean; "
            "three 720p palettes, representative 1080p/QHD; frozen redraw, on/off rasters and Scene culling bounds\n";
    }

    void wet_tool_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        Presentation& presentation, PlaySettings& preferences,
        const std::filesystem::path& output, std::ofstream& records)
    {
        const RestoreFlag restore_reduced{ preferences.reduce_effects, preferences.reduce_effects };
        preferences.reduce_effects = false;
        std::ofstream details(output / "quiet-tools-detail.csv");
        details << "capture,view,attached,bead_age,interval_remaining,drops,drip_pixels,breath,lean,blinking\n";
        for (const auto& tool : std::array<std::pair<WeaponType, const char*>, 2>{{
            { WeaponType::sniper, "sniper" }, { WeaponType::bucket, "bucket" } }})
        {
            const auto& look = presentation.idle_drip(tool.first);
            for (const int count : { 1, 4 })
            {
                for (const TeamColourPair palette : { TeamColourPair::blue_orange,
                    TeamColourPair::pink_green, TeamColourPair::blue_yellow })
                {
                    auto level = build(app, builder, count, palette, false, false, tool.first);
                    auto inputs = neutral_inputs();
                    settle(*level, inputs);
                    resize(app, *level, { 1280, 720 });
                    for (int view = 0; view < count; ++view)
                    {
                        require(level->place_player(view,
                            Vector2F(1200.0f + 200.0f * static_cast<float>(view), 1275)),
                            "could not place the wet-tool fixture on the recovery floor");
                    }
                    for (int tick = 0; tick < 60; ++tick) { level->update(inputs, dt); }
                    const IdleDrip* drip = level->idle_drip(0);
                    require(drip != nullptr, "wet-tool fixture omitted its Scene-owned drip");
                    const std::string prefix = std::to_string(count) + "p-quiet-" + tool.second + '-' +
                        TeamColourTools::palette_key(palette);
                    const auto advance_until = [&](const auto& condition, const char* message)
                    {
                        for (int tick = 0; tick < 900 && !condition(); ++tick) { level->update(inputs, dt); }
                        require(condition(), message);
                        require(level->state() == LevelState::active, "wet-tool fixture exhausted its real match clock");
                    };
                    const auto phase = [&](const std::string& name, bool extents = false)
                    {
                        for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
                        {
                            if (size.x != 1280 && (!extents || palette != TeamColourPair::blue_orange)) { continue; }
                            resize(app, *level, size);
                            quiet_capture(app, *level, canvas, presentation, output,
                                prefix + '-' + name + '-' + std::to_string(size.x), records, details);
                        }
                        resize(app, *level, { 1280, 720 });
                    };
                    const auto reduced = [&](const char* name)
                    {
                        const auto before = observation(*level);
#if COLOURWARS_DEV_TOOLS
                        const MatchFingerprint fingerprint = MatchFingerprint::of(*level, 0);
#endif
                        for (int view = 0; view < count; ++view)
                        {
                            const Player& player = level->player(view);
                            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
                            const IdleDrip* decoration = level->idle_drip(view);
                            const Frame blank = isolated(nullptr, player, viewport, *app.renderer(), canvas);
                            const Frame normal = isolated(nullptr, player, viewport, *app.renderer(), canvas, decoration);
                            if (view == 0) { require(normal.pixels != blank.pixels, "wet-tool reduction fixture has no visible liquid"); }
                            preferences.reduce_effects = true;
                            const Frame hidden = isolated(nullptr, player, viewport, *app.renderer(), canvas, decoration);
                            require(hidden.pixels == blank.pixels && hidden.pixels ==
                                isolated(nullptr, player, viewport, *app.renderer(), canvas, decoration).pixels,
                                "reduce effects did not hide the frozen wet-tool bead/drop");
                            preferences.reduce_effects = false;
                            require(normal.pixels == isolated(nullptr, player, viewport, *app.renderer(), canvas, decoration).pixels,
                                "normal wet-tool liquid did not return after reducing effects");
                        }
                        preferences.reduce_effects = true;
                        capture(app, *level, canvas, output, prefix + '-' + name + "-reduced-1280", records);
                        preferences.reduce_effects = false;
                        require(before == observation(*level), "wet-tool reduced draws changed simulation/cosmetic state");
#if COLOURWARS_DEV_TOOLS
                        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(*level, 0)).empty(),
                            "wet-tool reduced draws changed the replay fingerprint");
#endif
                    };
                    advance_until([&] { return drip->attached() && drip->bead_age() >= look.grow_time * 0.5f &&
                        drip->bead_age() < look.grow_time; }, "settled wet tool never grew a bead");
                    phase("grow");
                    advance_until([&] { return drip->attached() &&
                        drip->bead_age() >= look.grow_time + look.stretch_time * 0.5f; },
                        "settled wet tool never stretched its bead");
                    phase("stretch", true);
                    reduced("stretch");
                    // Change the real stick aim while this same mature bead is
                    // still attached. The art's local wet point is measured
                    // from the left-centre draw pivot, independently of the
                    // projectile launch point used by the runtime attachment.
                    int quadrant = 0;
                    for (const float angle : { PI / 4.0f, 3.0f * PI / 4.0f, -3.0f * PI / 4.0f, -PI / 4.0f })
                    {
                        const float age = drip->bead_age();
                        for (auto& input : inputs)
                        {
                            input.shoot_angle = angle;
                            input.shoot_direction = Vector2F::unit_vec_from_angle(angle);
                        }
                        level->update(inputs, dt);
                        require(drip->attached() && drip->bead_age() > age,
                            "continuous wet-tool aim replaced or released the attached bead");
                        for (int view = 0; view < count; ++view)
                        {
                            const IdleDrip* attached = level->idle_drip(view);
                            if (!attached->attached()) { continue; }
                            const Weapon& weapon = level->player(view).primary_weapon();
                            const float side = std::abs(angle) > PI / 2.0f ? -1.0f : 1.0f;
                            const Vector2F local = tool.first == WeaponType::sniper ? Vector2F(99, 3) : Vector2F(46, 8);
                            const Vector2F expected = weapon.draw_pos() + Vector2F::rotate_vector(
                                Vector2F(local.x, side * local.y), weapon.rotation());
                            const RectangleF bead = attached->bead_rectangle();
                            require(std::abs(bead.center().x - expected.x) < 0.01f && std::abs(bead.y - expected.y) < 0.01f,
                                "wet bead missed the authored muzzle lip/bucket runoff stripe after aiming/flipping");
                        }
                        phase("aim" + std::to_string(++quadrant));
                    }
                    advance_until([&] { return !drip->drops().empty() && drip->drops()[0].age >= dt * 2.0f; },
                        "settled wet tool never released a falling cosmetic drop");
                    phase("fall");
                    reduced("fall");
                    const float drop_y = drip->drops()[0].position.y;
                    level->update(inputs, dt);
                    require(drip->drops().empty() || drip->drops()[0].position.y > drop_y,
                        "released wet-tool drop did not fall downwards");
                    for (auto& input : inputs) { input.x_movement = -1.0f; }
                    level->update(inputs, dt);
                    require(!drip->attached(), "wet-tool movement did not interrupt its bead");
                    for (auto& input : inputs)
                    {
                        input.x_movement = 0;
                        input.shoot_angle = PI;
                        input.shoot_direction = Vector2F(-1, 0);
                    }
                    advance_until([&] { return drip->attached() && drip->bead_age() >= look.grow_time; },
                        "left-facing settled wet tool never regrew a mature bead");
                    require(!level->player(0).facing_right(), "wet-tool left-facing fixture stayed right-facing");
                    phase("left-attached", true);
                }
            }
        }
        require(details.good(), "wet-tool detail capture record write failed");
        std::cout << "PASS actual 1P/4P sniper residual bead and bucket rim runoff: grow/stretch/fall, "
            "four continuous aim quadrants, flipped lip/stripe attachment and both body facings; "
            "all 720p palettes, representative 1080p/QHD; frozen redraw, bounds and reduced-effects suppression\n";
    }

    bool same_pixel(const Frame& first, const Frame& second, size_t offset)
    {
        return std::equal(first.pixels.begin() + offset, first.pixels.begin() + offset + 4,
            second.pixels.begin() + offset);
    }

    size_t pane_delta(const Frame& first, const Frame& second, const RectangleI& pane)
    {
        size_t changed = 0;
        for (int y = pane.y; y < pane.bottom(); ++y)
        {
            for (int x = pane.x; x < pane.right(); ++x)
            {
                const size_t offset = (static_cast<size_t>(y) * static_cast<size_t>(first.width) +
                    static_cast<size_t>(x)) * 4;
                if (!same_pixel(first, second, offset)) { ++changed; }
            }
        }
        return changed;
    }

    Frame isolated_marker(const Player& player, const Viewport& viewport,
        Renderer& renderer, const Canvas& canvas, bool visible);

    Frame isolated_status(const Player& player, const Viewport& viewport,
        Renderer& renderer, const Canvas& canvas, bool reduced)
    {
        renderer.begin_frame();
        renderer.set_view_count(1);
        DrawList list = renderer.view(0);
        list.set_viewport(viewport);
        list.set_camera(player.camera());
        list.set_filter(TextureFilter::point);
        canvas.draw(list, player.camera().visible_rectangle(viewport));
        const Weapon& tool = player.primary_weapon();
        tool.feedback()->draw(list, tool.nozzle_pos(), tool.draw_pos(),
            tool.rotation(), tool.type(), player.team_colour(), reduced);
        return read(renderer);
    }

    // The live builder borrows this same saved preference object. Compare its
    // two draw paths without advancing a match, then restore the exact normal
    // raster; rendering a reduced frame must not consume a cosmetic event.
    void reduced_effects_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        Presentation& presentation, PlaySettings& preferences,
        const std::filesystem::path& output, std::ofstream& records)
    {
        const RestoreFlag restore{ preferences.reduce_effects, preferences.reduce_effects };
        const RestoreFlag restore_marker{ preferences.aim_marker, preferences.aim_marker };
        preferences.reduce_effects = false;
        preferences.aim_marker = true;
        std::ofstream effects_records(output / "reduced-effects.csv");
        effects_records << "capture,view,changed_pixels\n";
        const auto compare = [&](Level& level, const std::string& stem, bool every_pane)
        {
            const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
            const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
            preferences.reduce_effects = false;
            capture(app, level, canvas, output, stem + "-normal", records);
            const Frame normal = render(level, *app.renderer());
            preferences.reduce_effects = true;
            capture(app, level, canvas, output, stem + "-reduced", records);
            const Frame reduced = render(level, *app.renderer());
            bool changed = false;
            for (int view = 0; view < level.player_count(); ++view)
            {
                const size_t pixels = pane_delta(normal, reduced,
                    app.viewport_manager()->player_viewport(view).pixel_rect());
                require(!every_pane || pixels > 0,
                    "reduce-effects preference did not remove the live muzzle flash in every pane");
                changed |= pixels > 0;
                effects_records << stem << ',' << view + 1 << ',' << pixels << '\n';
            }
            require(changed, "reduce-effects preference produced no visible playing-size difference");
            preferences.reduce_effects = false;
            require(normal.pixels == render(level, *app.renderer()).pixels,
                "normal raster did not return after the same-state reduce-effects toggle");
            require(before == observation(level),
                "reduce-effects drawing or preference toggles changed observed match state");
#if COLOURWARS_DEV_TOOLS
            require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
                "reduce-effects drawing or preference toggles changed the replay fingerprint");
#endif
        };
        const auto compare_markers = [&](Level& level, bool initially_ready)
        {
            const auto before = observation(level);
            bool saw_recovering = false;
            for (int view = 0; view < level.player_count(); ++view)
            {
                const Player& player = level.player(view);
                const Viewport& viewport = app.viewport_manager()->player_viewport(view);
                const WeaponReadiness readiness = player.primary_weapon().readiness();
                require(!initially_ready || readiness == WeaponReadiness::ready,
                    "reduced readiness fixture was not initially ready");
                saw_recovering |= readiness == WeaponReadiness::recovering;
                const Frame blank = isolated_marker(player, viewport, *app.renderer(), canvas, false);
                preferences.reduce_effects = false;
                require(level.aim_marker_visible(view), "normal reduce-effects fixture lost its readiness marker");
                const Frame normal = isolated_marker(player, viewport, *app.renderer(), canvas, true);
                preferences.reduce_effects = true;
                require(level.aim_marker_visible(view) && normal.pixels != blank.pixels &&
                    normal.pixels == isolated_marker(player, viewport, *app.renderer(), canvas, true).pixels &&
                    player.primary_weapon().readiness() == readiness,
                    "reduce effects hid or changed the real aim/readiness marker raster");
            }
            preferences.reduce_effects = false;
            require(initially_ready || saw_recovering,
                "reduced readiness fixture did not exercise a recovering tool");
            require(before == observation(level), "reduced readiness comparison changed observed match state");
        };
        for (const int count : { 1, 4 })
        {
            auto level = build(app, builder, count, TeamColourPair::blue_orange,
                false, false, count == 1 ? WeaponType::mister : WeaponType::none);
            auto inputs = neutral_inputs();
            settle(*level, inputs);
            resize(app, *level, { 1280, 720 });
            compare_markers(*level, true);
            for (int view = 0; view < count; ++view)
            {
                const Player& player = level->player(view);
                Weapon& tool = const_cast<Weapon&>(player.primary_weapon());
                auto& input = inputs[static_cast<size_t>(player.player_num())];
                input.primary_shoot = true;
                const float ammo = tool.ammo();
                // Retain real paid projectiles locally, as in the weapon
                // matrix, so the four fixtures cannot hit each other.
                const auto shots = tool.update_and_get_projectiles(input,
                    player.rectangle().center(), player.velocity(), player.facing_right(), dt);
                require(!shots.empty() && tool.ammo() < ammo &&
                    tool.feedback()->state().muzzle_remaining > 0.0f,
                    "reduce-effects weapon fixture did not produce a real paid muzzle flash");
            }
            const std::string prefix = std::to_string(count) + "p-reduce-effects-720";
            compare(*level, prefix + "-paid-shot", true);
            compare_markers(*level, false);
            for (int view = 0; view < count; ++view)
            {
                const Player& player = level->player(view);
                const Weapon& tool = player.primary_weapon();
                auto& feedback = const_cast<WeaponFeedback&>(*tool.feedback());
                const Viewport& viewport = app.viewport_manager()->player_viewport(view);
                const RectangleI pane = viewport.pixel_rect();
                const Frame blank = isolated_marker(player, viewport, *app.renderer(), canvas, false);
                for (const bool recovered : { false, true })
                {
                    feedback.clear();
                    if (recovered) { feedback.recovered(); }
                    else { feedback.empty(); }
                    const auto before = observation(*level);
                    const Frame normal = isolated_status(player, viewport, *app.renderer(), canvas, false);
                    const Frame reduced = isolated_status(player, viewport, *app.renderer(), canvas, true);
                    const size_t normal_pixels = pane_delta(normal, blank, pane);
                    const size_t reduced_pixels = pane_delta(reduced, blank, pane);
                    require(reduced.pixels == isolated_status(player, viewport, *app.renderer(), canvas, true).pixels,
                        "frozen reduced status cue changed during repeated drawing");
                    require(reduced_pixels > 0 && (recovered ? reduced_pixels < normal_pixels :
                        reduced.pixels == normal.pixels),
                        "reduced effects removed a useful dry/refill cue or retained the refill rays");
                    const std::string stem = prefix + (recovered ? "-refill" : "-dry");
                    for (const auto& sample : { std::pair{ &normal, "-normal" }, std::pair{ &reduced, "-reduced" } })
                    {
                        Frame crop{};
                        require(check_projected_bounds(*sample.first, blank,
                            player.camera().calculate_view_rectangle(tool.draw_bounds()), viewport, crop) > 0,
                            "reduced status crop omitted its dry/refill cue");
                        save(crop, output / (stem + sample.second + "-player" + std::to_string(view + 1) + ".ppm"));
                    }
                    effects_records << stem << ',' << view + 1 << ',' << pane_delta(normal, reduced, pane) << '\n';
                    require(before == observation(*level), "reduced status drawing changed observed match state");
                }
                feedback.clear();
                if (tool.type() == WeaponType::mister)
                {
                    Weapon& mister = const_cast<Weapon&>(tool);
                    auto& input = inputs[static_cast<size_t>(player.player_num())];
                    input.primary_shoot = false;
                    const auto shots = mister.update_and_get_projectiles(input,
                        player.rectangle().center(), player.velocity(), player.facing_right(), dt);
                    require(shots.empty() && mister.motion_state().mist_tail_remaining > 0.0f,
                        "reduce-effects mister fixture did not release a real cosmetic tail");
                    const auto before = observation(*level);
                    preferences.reduce_effects = false;
                    const Frame normal = isolated_tool(player, viewport, *app.renderer(), canvas, presentation);
                    preferences.reduce_effects = true;
                    const Frame reduced = isolated_tool(player, viewport, *app.renderer(), canvas, presentation);
                    require(reduced.pixels == isolated_tool(player, viewport, *app.renderer(), canvas, presentation).pixels,
                        "frozen reduced mister tool changed during repeated drawing");
                    preferences.reduce_effects = false;
                    auto& motion = const_cast<ToolMotionState&>(mister.motion_state());
                    const ToolMotionState released = motion;
                    motion.mist_tail_remaining = 0.0f;
                    const Frame tool_only = isolated_tool(player, viewport, *app.renderer(), canvas, presentation);
                    motion = released;
                    require(normal.pixels != reduced.pixels && reduced.pixels == tool_only.pixels,
                        "reduce effects failed to hide only the real mister release tail");
                    require(before == observation(*level), "reduced mist drawing changed observed match state");
                }
            }
            compare(*level, prefix + "-mist-release", false);

            auto quiet = build(app, builder, count, TeamColourPair::blue_orange,
                false, false, WeaponType::sprayer);
            inputs = neutral_inputs();
            settle(*quiet, inputs);
            resize(app, *quiet, { 1280, 720 });
            for (int view = 0; view < count; ++view)
            {
                require(quiet->place_player(view,
                    Vector2F(1200.0f + 200.0f * static_cast<float>(view), 1275)),
                    "could not place the reduce-effects idle fixture on the recovery floor");
            }
            for (int tick = 0; tick < 60; ++tick) { quiet->update(inputs, dt); }
            const IdleDrip* drip = quiet->idle_drip(0);
            require(drip != nullptr, "reduce-effects idle fixture omitted its sprayer drip");
            const auto visible_bead = [&] { return drip->attached() &&
                drip->bead_age() >= presentation.idle_drip().grow_time; };
            for (int tick = 0; tick < 900 && !visible_bead(); ++tick) { quiet->update(inputs, dt); }
            require(visible_bead(), "reduce-effects fixture never produced a mature idle bead");
            compare(*quiet, prefix + "-idle", false);
        }
        require(effects_records.good(), "reduce-effects capture record write failed");
        std::cout << "PASS saved reduce-effects preference at 1P/4P 720p; real paid muzzle flashes and idle detail; "
            "preserved dry/refill and ready/cooldown marker rasters, fewer refill rays and hidden mister release tail; "
            "normal/reduced/restored rasters, frozen redraw and unchanged match observation/replay fingerprint\n";
    }

    // Draw the actual HUD over two contrasting backgrounds. Pixels independent
    // of both backgrounds are opaque HUD ink; full-match samples that match that
    // ink must stay fixed while the scene behind them moves. Translucent panel
    // pixels intentionally retain the world beneath them and are not compared.
    Frame shake_hud(Application& app, const Level& level, const GameTuning& tuning,
        const Presentation& presentation, const Colour& background)
    {
        const InterfaceGameplay hud(app.render_resources(), &presentation.hud(), &presentation.player_identity());
        const Canvas canvas(app.render_resources(), background);
        Renderer& renderer = *app.renderer();
        renderer.begin_frame();
        renderer.set_view_count(level.player_count());
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Player& player = level.player(view);
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            DrawList list = renderer.view(view);
            list.set_viewport(viewport);
            list.set_camera(Camera::DEFAULT_CAMERA);
            list.set_filter(TextureFilter::point);
            canvas.draw(list, RectangleF(Vector2F::ZERO, viewport.size()));
            // Identity and meters use the real frozen values. Any timer ink
            // selected below must also match the full frame's current clock.
            hud.draw_gameplay_interface(list, viewport.size(), view, player.player_num(),
                player.team(), player.health() / tuning.player().starting_health,
                player.weapon_ammo() / tuning.starting_ammo(), 100.0f, player.team_colour());
        }
        return read(renderer);
    }

    void shake_capture(Application& app, Level& level, PlaySettings& preferences,
        const Frame& dark_hud, const Frame& light_hud, const std::filesystem::path& output,
        const std::string& stem, std::ofstream& records, bool active)
    {
        const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
        const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
        std::vector<Camera> cameras;
        std::vector<Vector2F> aims;
        std::vector<Vector2F> offsets;
        std::vector<float> remaining;
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Player& player = level.player(view);
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            cameras.push_back(player.camera());
            const RectangleI pane = viewport.pixel_rect();
            const Vector2F cursor(static_cast<float>(pane.x) + viewport.width * 0.7f,
                static_cast<float>(pane.y) + viewport.height * 0.3f);
            aims.push_back(PlayerCamera::mouse_direction(player.center(), cursor, viewport, player.camera()));
            offsets.push_back(player.view_shake().offset());
            remaining.push_back(player.view_shake().remaining());
            require((view == 0 && active) == (player.view_shake().remaining() > 0.0f),
                "view shake was absent from the affected player or leaked into another seat");
        }

        preferences.view_shake = false;
        level.refresh_layout();
        for (int view = 0; view < level.player_count(); ++view)
        {
            require(level.view_camera(view) == cameras[static_cast<size_t>(view)],
                "disabled view shake displaced a render camera");
        }
        const Frame disabled = render(level, *app.renderer());
        preferences.view_shake = true;
        level.refresh_layout();
        const Frame enabled = render(level, *app.renderer());
        require(enabled.pixels == render(level, *app.renderer()).pixels,
            "frozen redraw aged view shake or changed the match raster");
        preferences.view_shake = false;
        level.refresh_layout();
        require(disabled.pixels == render(level, *app.renderer()).pixels,
            "repeated frozen setting toggle changed the disabled match raster");
        preferences.view_shake = true;
        level.refresh_layout();

        save(enabled, output / (stem + ".ppm"));
        if (active && enabled.width == 1280) { save(disabled, output / (stem + "-off.ppm")); }
        for (int view = 0; view < level.player_count(); ++view)
        {
            const size_t index = static_cast<size_t>(view);
            const Player& player = level.player(view);
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            const RectangleI pane = viewport.pixel_rect();
            const size_t changed = pane_delta(enabled, disabled, pane);
            require(view == 0 && active ? changed > 0 : changed == 0,
                "view shake failed to move its pane or changed an unaffected pane");
            require((level.view_camera(view) != cameras[index]) == (view == 0 && active),
                "render camera displacement did not stay local to the affected view");
            size_t hud_pixels = 0;
            for (int y = pane.y; y < pane.bottom(); ++y)
            {
                for (int x = pane.x; x < pane.right(); ++x)
                {
                    const size_t offset = (static_cast<size_t>(y) * static_cast<size_t>(enabled.width) +
                        static_cast<size_t>(x)) * 4;
                    if (same_pixel(dark_hud, light_hud, offset) && same_pixel(dark_hud, disabled, offset))
                    {
                        require(same_pixel(enabled, disabled, offset), "view shake moved opaque HUD pixels");
                        ++hud_pixels;
                    }
                }
            }
            require(hud_pixels > 100, "HUD shake comparison did not find enough real opaque HUD pixels");
            const Vector2F cursor(static_cast<float>(pane.x) + viewport.width * 0.7f,
                static_cast<float>(pane.y) + viewport.height * 0.3f);
            require(player.camera() == cameras[index] &&
                PlayerCamera::mouse_direction(player.center(), cursor, viewport, player.camera()) == aims[index],
                "view shake setting changed the base follow camera or mouse aim");
            require(player.view_shake().offset() == offsets[index] &&
                player.view_shake().remaining() == remaining[index],
                "drawing or changing the view shake preference advanced cosmetic state");
            records << stem << ',' << view + 1 << ',' << player.player_num() << ',' << changed << ','
                << hud_pixels << ',' << offsets[index].x << ',' << offsets[index].y << ','
                << remaining[index] << '\n';
        }
        require(before == observation(level), "view shake draw/toggle changed observed match state");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
            "view shake draw/toggle changed the replay fingerprint");
#endif
        std::cout << "PASS " << stem << " local world displacement, fixed HUD/aim and frozen redraw\n";
    }

    Frame isolated_marker(const Player& player, const Viewport& viewport,
        Renderer& renderer, const Canvas& canvas, bool visible)
    {
        renderer.begin_frame();
        renderer.set_view_count(1);
        DrawList list = renderer.view(0);
        list.set_viewport(viewport);
        list.set_camera(player.camera());
        list.set_filter(TextureFilter::point);
        canvas.draw(list, player.camera().visible_rectangle(viewport));
        if (visible) { player.primary_weapon().draw_aim_marker(list); }
        return read(renderer);
    }

    // On/off is rendered from exactly the same live Level. Every changed pixel
    // must belong to that pane's own current marker; another player's marker
    // in this pane cannot hide behind a whole-frame "changed" assertion.
    std::vector<Frame> marker_capture(Application& app, Level& level, PlaySettings& preferences,
        const Canvas& canvas, const std::filesystem::path& output,
        const std::string& stem, std::ofstream& records)
    {
        const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
        const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
        preferences.aim_marker = true;
        const Frame enabled = render(level, *app.renderer());
        require(enabled.pixels == render(level, *app.renderer()).pixels,
            "Frozen marker Level redraw changed pixels");
        preferences.aim_marker = false;
        const Frame disabled = render(level, *app.renderer());
        require(disabled.pixels == render(level, *app.renderer()).pixels,
            "Disabled marker Level redraw changed pixels");
        preferences.aim_marker = true;
        save(enabled, output / (stem + ".ppm"));
        std::vector<Frame> marker_rasters;
        for (int view = 0; view < level.player_count(); ++view)
        {
            const auto& player = level.player(view);
            const auto& tool = player.primary_weapon();
            const auto& viewport = app.viewport_manager()->player_viewport(view);
            Frame crop{};
            const size_t changed = check_projected_bounds(enabled, disabled,
                level.view_camera(view).calculate_view_rectangle(tool.aim_marker_bounds()), viewport, crop);
            require(changed > 0 && level.aim_marker_visible(view),
                "Active own-view marker produced no on/off raster delta");
            // The public marker bounds cover its independent world overlay,
            // even if its owner's body were culled by Scene.
            const Frame blank = isolated_marker(player, viewport, *app.renderer(), canvas, false);
            const Frame marker = isolated_marker(player, viewport, *app.renderer(), canvas, true);
            require(marker.pixels == isolated_marker(player, viewport, *app.renderer(), canvas, true).pixels,
                "Frozen isolated marker redraw changed pixels");
            Frame isolated_crop{};
            require(check_projected_bounds(marker, blank,
                player.camera().calculate_view_rectangle(tool.aim_marker_bounds()), viewport, isolated_crop) > 0,
                "Marker overlay omitted its independent raster");
            save(isolated_crop, output / (stem + "-player" + std::to_string(view + 1) + ".ppm"));
            marker_rasters.push_back(std::move(isolated_crop));
            records << stem << ',' << view + 1 << ',' << player.player_num() << ','
                << static_cast<int>(tool.type()) << ',' << static_cast<int>(tool.readiness()) << ','
                << tool.rotation() << ',' << changed << '\n';
        }
        require(before == observation(level), "Marker drawing/toggling changed simulation or feedback timers");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
            "Marker drawing/toggling changed replay fingerprint");
#endif
        return marker_rasters;
    }

    void marker_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        const GameTuning& tuning, PlaySettings& preferences, const std::filesystem::path& output)
    {
        const RestoreFlag restore{ preferences.aim_marker, preferences.aim_marker };
        preferences.aim_marker = true;
        std::ofstream records(output / "aim-marker.csv");
        records << "capture,view,input_slot,tool,readiness,angle,changed_pixels\n";
        for (const auto palette : { TeamColourPair::blue_orange,
            TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            auto level = build(app, builder, 4, palette);
            const auto inputs = neutral_inputs();
            settle(*level, inputs);
            for (int view = 0; view < 4; ++view)
            {
                require(level->place_player(view, Vector2F(1200.0f + 400.0f * static_cast<float>(view), 1275)),
                    "Could not place marker fixture on recovery floor");
            }
            for (int tick = 0; tick < 60; ++tick) { level->update(inputs, dt); }
            for (int direction = 0; direction < 4; ++direction)
            {
                const float angle = -3.0f * PI / 4.0f + static_cast<float>(direction) * PI / 2.0f;
                std::array<std::vector<Frame>, 3> ready_rasters;
                std::array<std::vector<Frame>, 3> recovering_rasters;
                for (int phase = 0; phase < 3; ++phase)
                {
                    const auto readiness = phase == 0 ? WeaponReadiness::ready :
                        phase == 1 ? WeaponReadiness::recovering : WeaponReadiness::empty;
                    for (int view = 0; view < 4; ++view)
                    {
                        Player& player = const_cast<Player&>(level->player(view));
                        Weapon& tool = const_cast<Weapon&>(player.primary_weapon());
                        const float cooldown = tuning.weapon(tool.type()).details.shoot_interval + dt;
                        // Aim around the actual player's pivot, then keep that
                        // transform during the controlled firing-only updates.
                        tool.reset_ammo();
                        PlayerInputData aim;
                        aim.connected = true;
                        aim.shoot_direction_requested = true;
                        aim.shoot_angle = angle;
                        aim.shoot_direction = Vector2F::unit_vec_from_angle(angle);
                        std::ignore = tool.update_and_get_projectiles(aim, player.rectangle().center(),
                            player.velocity(), player.facing_right(), cooldown);
                        const auto fire = [&](bool held, float elapsed)
                        {
                            aim.primary_shoot = held;
                            return tool.update_and_get_projectiles(aim, player.rectangle().center(),
                                player.velocity(), player.facing_right(), elapsed);
                        };
                        if (phase == 1)
                        {
                            require(!fire(true, 0.0f).empty(), "Marker cooldown fixture did not pay a shot");
                        }
                        else if (phase == 2)
                        {
                            for (int shot = 0; shot < 1000 && tool.readiness() != WeaponReadiness::empty; ++shot)
                            {
                                require(!fire(true, 0.0f).empty(), "Marker depletion fixture refused an affordable shot");
                                if (tool.readiness() != WeaponReadiness::empty) { std::ignore = fire(false, cooldown); }
                            }
                        }
                        require(tool.readiness() == readiness, "Actual weapon did not enter requested marker readiness");
                    }
                    for (const auto size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
                    {
                        if (size.x != 1280 && (palette != TeamColourPair::blue_orange || direction != 0)) { continue; }
                        resize(app, *level, size);
                        const char* state = phase == 0 ? "ready" : phase == 1 ? "recovering" : "empty";
                        const std::string stem = std::string("4p-aim-marker-") + TeamColourTools::palette_key(palette) +
                            "-q" + std::to_string(direction + 1) + "-" + state + "-" + std::to_string(size.x);
                        const auto rasters = marker_capture(app, *level, preferences, canvas, output, stem, records);
                        const size_t resolution = size.x == 1280 ? 0u : size.x == 1920 ? 1u : 2u;
                        if (phase == 0) { ready_rasters[resolution] = rasters; }
                        else
                        {
                            for (size_t view = 0; view < rasters.size(); ++view)
                            {
                                require(rasters[view].pixels != ready_rasters[resolution][view].pixels,
                                    "Ready and unavailable marker shapes produced identical playing-size rasters");
                                if (phase == 2)
                                {
                                    require(rasters[view].pixels != recovering_rasters[resolution][view].pixels,
                                        "Recovering and empty marker shapes produced identical playing-size rasters");
                                }
                            }
                            if (phase == 1) { recovering_rasters[resolution] = rasters; }
                        }
                    }
                    if (phase == 1)
                    {
                        const auto before = observation(*level);
                        const Frame paused = render(*level, *app.renderer());
                        level->suspend();
                        level->refresh_layout();
                        require(before == observation(*level) && paused.pixels == render(*level, *app.renderer()).pixels,
                            "Suspending the frozen match changed marker state or its raster");
                        level->resume();
                    }
                }
            }
            if (palette == TeamColourPair::blue_orange)
            {
                // Different states and directions in one frozen roster catch
                // a shared/global marker accidentally reused by another pane.
                for (int view = 0; view < 4; ++view)
                {
                    const auto& player = level->player(view);
                    auto& tool = const_cast<Weapon&>(player.primary_weapon());
                    const float cooldown = tuning.weapon(tool.type()).details.shoot_interval + dt;
                    PlayerInputData aim;
                    aim.connected = true;
                    aim.shoot_direction_requested = true;
                    aim.shoot_angle = -PI / 4.0f + static_cast<float>(view) * PI / 2.0f;
                    aim.shoot_direction = Vector2F::unit_vec_from_angle(aim.shoot_angle);
                    const auto fire = [&](bool held, float elapsed)
                    {
                        aim.primary_shoot = held;
                        return tool.update_and_get_projectiles(aim, player.rectangle().center(),
                            player.velocity(), player.facing_right(), elapsed);
                    };
                    tool.reset_ammo();
                    std::ignore = fire(false, cooldown);
                    if (view == 1) { require(!fire(true, 0.0f).empty(), "Mixed marker cooldown fixture did not fire"); }
                    if (view == 2)
                    {
                        for (int shot = 0; shot < 1000 && tool.readiness() != WeaponReadiness::empty; ++shot)
                        {
                            require(!fire(true, 0.0f).empty(), "Mixed marker empty fixture did not fire");
                            if (tool.readiness() != WeaponReadiness::empty) { std::ignore = fire(false, cooldown); }
                        }
                    }
                    require(tool.readiness() == (view == 1 ? WeaponReadiness::recovering :
                        view == 2 ? WeaponReadiness::empty : WeaponReadiness::ready),
                        "Mixed roster did not retain independent readiness");
                }
                resize(app, *level, { 1280, 720 });
                std::ignore = marker_capture(app, *level, preferences, canvas, output,
                    "4p-aim-marker-mixed-own-1280", records);
            }
        }
        require(records.good(), "Marker capture record write failed");
        std::cout << "PASS aim marker: actual ready/recovering/empty tools in all palettes and quadrants, four own panes at 720p, representative 1080p/QHD, independent marker raster bounds, pause, frozen redraw and unchanged fingerprint\n";
    }

    Frame identity_context_crop(const Frame& frame, const Level& level, int view,
        const Viewport& viewport)
    {
        const RectangleF body = level.player(view).rectangle();
        const RectangleF badge = level.player_identity_bounds(view);
        const float left = std::min(body.left(), badge.left());
        const float top = std::min(body.top(), badge.top());
        const float right = std::max(body.right(), badge.right());
        const float bottom = std::max(body.bottom(), badge.bottom());
        const RectangleF projected = level.view_camera(view).calculate_view_rectangle(
            RectangleF(left, top, right - left, bottom - top));
        const RectangleI pane = viewport.pixel_rect();
        const int x0 = std::max(pane.x, pane.x + static_cast<int>(std::floor(projected.x)) - 12);
        const int y0 = std::max(pane.y, pane.y + static_cast<int>(std::floor(projected.y)) - 12);
        const int x1 = std::min(pane.right(), pane.x + static_cast<int>(std::ceil(projected.right())) + 12);
        const int y1 = std::min(pane.bottom(), pane.y + static_cast<int>(std::ceil(projected.bottom())) + 12);
        require(x1 > x0 && y1 > y0, "Identity context did not intersect its own pane");
        Frame crop{ x1 - x0, y1 - y0, {} };
        for (int y = y0; y < y1; ++y)
        {
            const size_t start = (static_cast<size_t>(y) * static_cast<size_t>(frame.width) +
                static_cast<size_t>(x0)) * 4;
            crop.pixels.insert(crop.pixels.end(), frame.pixels.begin() + start,
                frame.pixels.begin() + start + static_cast<size_t>(crop.width) * 4);
        }
        return crop;
    }

    void identity_capture(Application& app, Level& level, Presentation& presentation,
        const std::filesystem::path& output, const std::string& stem, std::ofstream& records)
    {
        const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
        const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
        const Frame enabled = render(level, *app.renderer());
        require(enabled.pixels == render(level, *app.renderer()).pixels,
            "Frozen identity Level redraw changed pixels");
        auto& look = const_cast<PlayerIdentityPresentation&>(presentation.player_identity());
        // Hide only the world badge at the identical state. The same shared
        // renderer still draws the pane's UI badge, so every delta below must
        // belong to the owner above the figure, not its HUD or another player.
        const Vector2F world_size = look.world_size;
        look.world_size = Vector2F::ZERO;
        const Frame disabled = render(level, *app.renderer());
        look.world_size = world_size;
        require(enabled.pixels == render(level, *app.renderer()).pixels,
            "Restoring world identity size did not restore its raster");
        save(enabled, output / (stem + ".ppm"));
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            const RectangleF bounds = level.player_identity_bounds(view);
            Frame crop{};
            const size_t changed = check_projected_bounds(enabled, disabled,
                level.view_camera(view).calculate_view_rectangle(bounds), viewport, crop);
            require(level.player_identity_visible(view) ? changed > 0 : changed == 0,
                "World identity visibility or own-pane raster bounds did not match lifecycle");
            if (changed > 0)
            {
                save(identity_context_crop(enabled, level, view, viewport), output /
                    (stem + "-context-player" + std::to_string(view + 1) + ".ppm"));
                save(crop, output / (stem + "-badge-player" + std::to_string(view + 1) + ".ppm"));
            }
            records << stem << ',' << view + 1 << ',' << level.player(view).player_num() << ','
                << level.player_identity_visible(view) << ',' << level.player_identity_emphasis(view) << ','
                << changed << ',' << bounds.x << ',' << bounds.y << ',' << bounds.width << ',' << bounds.height << '\n';
        }
        require(before == observation(level), "Identity draws changed simulation or cosmetic timers");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
            "Identity draws changed replay fingerprint");
#endif
    }

    void identity_matrix(Application& app, LevelBuilder& builder, Presentation& presentation,
        PlaySettings& preferences, const std::filesystem::path& output)
    {
        const RestoreFlag restore_reduced{ preferences.reduce_effects, preferences.reduce_effects };
        preferences.reduce_effects = false;
        std::ofstream records(output / "player-identity.csv");
        records << "capture,view,input_slot,visible,emphasis,changed_pixels,bounds_x,bounds_y,bounds_width,bounds_height\n";
        const auto inputs = neutral_inputs();
        for (const TeamColourPair palette : { TeamColourPair::blue_orange,
            TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            auto level = build(app, builder, 4, palette, false, true, WeaponType::sprayer);
            settle(*level, inputs);
            resize(app, *level, { 1280, 720 });
            // All four identical same-team tools overlap. Only roster identity
            // and the existing head accents can distinguish the figures.
            for (int view = 0; view < 4; ++view)
            {
                require(level->place_player(view, { 1400.0f + 12.0f * static_cast<float>(view), 1275.0f }),
                    "Could not overlap same-team identity fixtures");
            }
            for (int tick = 0; tick < 600 && level->player_identity_emphasis(0) > 0.0f; ++tick)
            {
                level->update(inputs, dt);
            }
            require(level->player_identity_emphasis(0) == 0.0f, "Identity emphasis did not settle");
            const std::string prefix = std::string("4p-identity-") + TeamColourTools::palette_key(palette);
            identity_capture(app, *level, presentation, output, prefix + "-overlap-quiet-1280", records);
            const RectangleF quiet = level->player_identity_bounds(0);
            if (palette == TeamColourPair::blue_orange)
            {
                // Pin the shared renderer at exactly one pane-local position
                // and native gameplay size. Team tint, bodies and subpixel
                // camera differences cannot make two roster digits differ.
                const Viewport& viewport = app.viewport_manager()->player_viewport(0);
                const RectangleF projected = level->view_camera(0).calculate_view_rectangle(quiet);
                const RectangleF box(Vector2F(24, 24), projected.size());
                const Canvas canvas(app.render_resources());
                const PlayerBadge badge(&presentation.player_identity(), app.render_resources());
                const auto glyph = [&](int roster)
                {
                    Renderer& renderer = *app.renderer();
                    renderer.begin_frame();
                    renderer.set_view_count(1);
                    DrawList list = renderer.view(0);
                    list.set_viewport(viewport);
                    list.set_camera(Camera::DEFAULT_CAMERA);
                    canvas.draw(list, RectangleF(Vector2F::ZERO, viewport.size()));
                    if (roster >= 0) { badge.draw(list, box, roster); }
                    return read(renderer);
                };
                const Frame blank = glyph(-1);
                std::vector<Frame> digits;
                for (int roster = 0; roster < 4; ++roster)
                {
                    const Frame image = glyph(roster);
                    Frame crop{};
                    require(check_projected_bounds(image, blank, box, viewport, crop) > 0,
                        "Shared roster badge produced no native-size glyph");
                    for (const Frame& earlier : digits)
                    {
                        require(crop.pixels != earlier.pixels,
                            "Two roster badge numbers have identical native-size rasters");
                    }
                    save(crop, output / ("4p-identity-glyph-quiet-1280-player" +
                        std::to_string(roster + 1) + ".ppm"));
                    digits.push_back(std::move(crop));
                }
            }
            level->emphasise_players();
            require(level->player_identity_emphasis(0) == 1.0f &&
                level->player_identity_bounds(0).width > quiet.width,
                "Requested identity emphasis did not expand the world badge");
            identity_capture(app, *level, presentation, output, prefix + "-overlap-emphasis-1280", records);
            preferences.reduce_effects = true;
            require(level->player_identity_visible(0) && level->player_identity_emphasis(0) == 0.0f &&
                level->player_identity_bounds(0).width == quiet.width,
                "Reduce effects hid identity or retained its animated emphasis");
            identity_capture(app, *level, presentation, output, prefix + "-overlap-reduced-1280", records);
            preferences.reduce_effects = false;
            if (palette != TeamColourPair::blue_orange) { continue; }
            for (const Vector2I size : { Vector2I(1920, 1080), Vector2I(2560, 1440) })
            {
                resize(app, *level, size);
                identity_capture(app, *level, presentation, output,
                    prefix + "-overlap-emphasis-" + std::to_string(size.x), records);
            }
            resize(app, *level, { 1280, 720 });
            auto crossing = inputs;
            require(level->place_player(0, { 1400, 1275 }) && level->place_player(1, { 1512, 1275 }),
                "Could not place teammate crossing fixture");
            crossing[static_cast<size_t>(level->player(0).player_num())].x_movement = 1.0f;
            crossing[static_cast<size_t>(level->player(1).player_num())].x_movement = -1.0f;
            for (int tick = 0; tick < 60 &&
                level->player(0).rectangle().x <= level->player(1).rectangle().x; ++tick)
            {
                level->update(crossing, dt);
            }
            require(level->player(0).rectangle().x > level->player(1).rectangle().x,
                "Same-team scripted players did not cross");
            identity_capture(app, *level, presentation, output, prefix + "-crossed-1280", records);
            level->suspend();
            identity_capture(app, *level, presentation, output, prefix + "-paused-1280", records);
            level->resume();
            require(level->kill_player(0), "Could not kill identity fixture");
            level->update(inputs, dt);
            require(!level->player_identity_visible(0), "Dead player retained an owner badge");
            identity_capture(app, *level, presentation, output, prefix + "-dead-1280", records);
            for (int tick = 0; tick < 600 && level->player(0).state() != PlayerState::alive; ++tick)
            {
                level->update(inputs, dt);
            }
            require(level->player(0).state() == PlayerState::alive &&
                level->player_identity_visible(0) && level->player_identity_emphasis(0) > 0.0f,
                "Respawn did not restore and emphasise the owning identity");
            identity_capture(app, *level, presentation, output, prefix + "-respawn-1280", records);
        }
        auto sparse = build(app, builder, 2, TeamColourPair::blue_orange, false, true);
        settle(*sparse, inputs);
        resize(app, *sparse, { 1280, 720 });
        identity_capture(app, *sparse, presentation, output, "2p-identity-sparse-slots-1280", records);
        auto story = build(app, builder, 4, TeamColourPair::blue_orange,
            false, true, WeaponType::sprayer, OpeningMode::quick);
        resize(app, *story, { 1280, 720 });
        require(story->intro_playing() && !story->player_identity_visible(0),
            "Story fixture did not suppress world identity");
        identity_capture(app, *story, presentation, output, "4p-identity-story-hidden-1280", records);
        require(records.good(), "Identity record write failed");
        std::cout << "PASS player identity: three 4P720 palettes, same-team overlap/crossing, sparse/reordered input slots, quiet/emphasis/reduced effects, 1080p/QHD bounds, dead/respawn/story/pause, own-pane world-only raster deltas, frozen cosmetic state and fingerprint\n";
    }

    void shake_matrix(Application& app, LevelBuilder& builder, const GameTuning& tuning,
        const Presentation& presentation, PlaySettings& preferences, const std::filesystem::path& output)
    {
        const RestoreFlag restore{ preferences.view_shake, preferences.view_shake };
        preferences.view_shake = true;
        std::ofstream records(output / "shake-detail.csv");
        records << "capture,view,input_slot,changed_pixels,fixed_hud_pixels,offset_x,offset_y,remaining\n";
        for (const int count : { 1, 4 })
        {
            for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
            {
                auto level = build(app, builder, count, TeamColourPair::blue_orange);
                auto inputs = neutral_inputs();
                settle(*level, inputs);
                resize(app, *level, size);
                for (int view = 0; view < count; ++view)
                {
                    require(level->place_player(view, Vector2F(1200.0f + 400.0f * static_cast<float>(view), 1275)),
                        "could not place the view shake fixture on the recovery floor");
                }
                for (int tick = 0; tick < 60; ++tick) { level->update(inputs, dt); }
                const Frame dark_hud = shake_hud(app, *level, tuning, presentation, Colour::black);
                const Frame light_hud = shake_hud(app, *level, tuning, presentation, Colour::white);
                const auto phase = [&](const char* name, bool active)
                {
                    shake_capture(app, *level, preferences, dark_hud, light_hud, output,
                        std::to_string(count) + "p-shake-" + name + '-' + std::to_string(size.x), records, active);
                };
                const auto expire = [&]()
                {
                    for (int tick = 0; tick < 120 && level->player(0).view_shake().remaining() > 0.0f; ++tick)
                    {
                        level->update(inputs, dt);
                    }
                    require(level->player(0).view_shake().remaining() == 0.0f,
                        "real match view shake did not expire within 120 ticks");
                };
                phase("before", false);
                Player& player = const_cast<Player&>(level->player(0));
                const ProjectileSpray hit(player.rectangle(), Vector2F::ZERO, PlayerTeam::b,
                    0, Colour::white, app.render_resources(), &tuning.projectile(ProjectileType::spray));
                const float health = player.health();
                player.on_contact(hit, Vector2F::ZERO, 0);
                require(player.health() < health && player.state() == PlayerState::alive,
                    "view shake fixture did not take a real nonfatal enemy projectile hit");
                phase("hurt", true);
                level->suspend();
                level->refresh_layout();
                for (int view = 0; view < count; ++view)
                {
                    require(level->player(view).view_shake().remaining() == 0.0f &&
                        level->view_camera(view) == level->player(view).camera(),
                        "suspending a hurt match retained a displaced render camera");
                }
                const Frame suspended = render(*level, *app.renderer());
                preferences.view_shake = false;
                level->refresh_layout();
                require(suspended.pixels == render(*level, *app.renderer()).pixels,
                    "suspended match still rendered a view shake accent");
                preferences.view_shake = true;
                level->resume();
                level->refresh_layout();
                require(player.view_shake().remaining() == 0.0f &&
                    suspended.pixels == render(*level, *app.renderer()).pixels,
                    "resuming a suspended match restored a stale view shake accent");

                // Establish real support before walking off the runway. A
                // direct airborne placement is spawn settling and must stay
                // silent. Stop in the gap before its x2700 recovery platforms.
                require(level->place_player(0, Vector2F(2380, 875)), "could not place the strong-landing fixture");
                for (int tick = 0; tick < 30; ++tick) { level->update(inputs, dt); }
                require(player.move_state() == PlayerMoveState::on_ground,
                    "strong-landing fixture did not establish runway support");
                PlayerInputData& input = inputs[static_cast<size_t>(player.player_num())];
                input.x_movement = 1.0f;
                for (int tick = 0; tick < 120 && player.rectangle().x < 2482.0f; ++tick)
                {
                    level->update(inputs, dt);
                }
                input.x_movement = 0.0f;
                require(player.rectangle().x >= 2482.0f && player.rectangle().right() < 2700.0f,
                    "strong-landing fixture did not leave the runway into the open gap");
                for (int tick = 0; tick < 180 && player.view_shake().remaining() == 0.0f; ++tick)
                {
                    level->update(inputs, dt);
                }
                require(player.state() == PlayerState::alive && player.move_state() == PlayerMoveState::on_ground &&
                    player.view_shake().remaining() > 0.0f, "resolved strong landing did not start local view shake");
                phase("landing", true);
                expire();
                require(level->kill_player(0), "could not request local view shake elimination");
                level->update(inputs, dt);
                require(player.state() == PlayerState::dead, "view shake elimination fixture stayed alive");
                phase("elimination", true);
                expire();
                phase("expired", false);
            }
        }
        require(records.good(), "view shake capture record write failed");
        std::cout << "PASS real enemy projectile hurt, resolved strong landing and elimination; "
            "local 1P/4P world shake at 720p/1080p/QHD, unchanged other panes and opaque HUD ink; "
            "same-state on/off, frozen redraw, suspend/resume reset, expiry, stable follow camera/mouse aim and replay fingerprint\n";
    }
#include "scripts/acceptance/painter_visual/monitor_checks.h"
#include "scripts/acceptance/painter_visual/countdown_checks.h"
#include "scripts/acceptance/painter_visual/reveal_checks.h"
#include "scripts/acceptance/painter_visual/hud_checks.h"
#include "scripts/acceptance/painter_visual/supply_checks.h"
#include "scripts/acceptance/painter_visual/resume_checks.h"
#include "scripts/acceptance/painter_visual/end_menu_checks.h"
#include "scripts/acceptance/painter_visual/end_heading_checks.h"
#include "scripts/acceptance/painter_visual/pause_menu_checks.h"
#include "scripts/acceptance/painter_visual/pause_question_checks.h"
#include "scripts/acceptance/painter_visual/results_menu_checks.h"
#include "scripts/acceptance/painter_visual/main_menu_checks.h"
#include "scripts/acceptance/painter_visual/setup_paper_checks.h"
#include "scripts/acceptance/painter_visual/weapon_catalog_checks.h"
#include "scripts/acceptance/painter_visual/menu_text_checks.h"
#include "scripts/acceptance/painter_visual/stage_random_text_checks.h"
#include "scripts/acceptance/painter_visual/stage_briefing_text_checks.h"
#include "scripts/acceptance/painter_visual/stage_cue_text_checks.h"
#include "scripts/acceptance/painter_visual/save_status_checks.h"
#include "scripts/acceptance/painter_visual/settings_value_checks.h"
#include "scripts/acceptance/painter_visual/settings_menu_checks.h"
#include "scripts/acceptance/painter_visual/controls_help_checks.h"
#include "scripts/acceptance/painter_visual/controls_menu_checks.h"
#include "scripts/acceptance/painter_visual/stick_settings_checks.h"
#include "scripts/acceptance/painter_visual/menu_motion_checks.h"
#if defined(COLOURWARS_PAINTER_SCRIPTED_PADS)
#include "scripts/acceptance/painter_visual/team_setup_checks.h"
#include "scripts/acceptance/painter_visual/setup_motion_checks.h"
#include "scripts/acceptance/painter_visual/join_stage_motion_checks.h"
#include "scripts/acceptance/painter_visual/join_paper_checks.h"
#include "scripts/acceptance/painter_visual/team_paper_checks.h"
#include "scripts/acceptance/painter_visual/weapon_paper_checks.h"
#include "scripts/acceptance/painter_visual/stage_paper_checks.h"
#endif
}

int wmain(int argc, wchar_t** argv)
{
    try
    {
        require(argc == 3 || (argc == 4 && (std::wstring(argv[3]) == L"identity" ||
            std::wstring(argv[3]) == L"quiet" || std::wstring(argv[3]) == L"monitor" ||
            std::wstring(argv[3]) == L"countdown" || std::wstring(argv[3]) == L"closing" ||
            std::wstring(argv[3]) == L"supply" || std::wstring(argv[3]) == L"hud" ||
            std::wstring(argv[3]) == L"resume" || std::wstring(argv[3]) == L"end-menu" ||
            std::wstring(argv[3]) == L"pause-menu" || std::wstring(argv[3]) == L"results-menu" ||
            std::wstring(argv[3]) == L"main-menu" || std::wstring(argv[3]) == L"settings-menu" ||
            std::wstring(argv[3]) == L"controls-menu" || std::wstring(argv[3]) == L"stick-settings" ||
            std::wstring(argv[3]) == L"team-setup" || std::wstring(argv[3]) == L"weapon-catalog" ||
            std::wstring(argv[3]) == L"menu-text" || std::wstring(argv[3]) == L"menu-motion" ||
            std::wstring(argv[3]) == L"setup-motion" || std::wstring(argv[3]) == L"join-stage-motion" ||
            std::wstring(argv[3]) == L"setup-paper" || std::wstring(argv[3]) == L"join-paper" || std::wstring(argv[3]) == L"team-paper" || std::wstring(argv[3]) == L"weapon-paper" || std::wstring(argv[3]) == L"stage-paper")),
            "usage: painter_visual <staged-content> <output> [identity|quiet|monitor|countdown|closing|supply|hud|resume|end-menu|pause-menu|results-menu|main-menu|settings-menu|controls-menu|stick-settings|team-setup|weapon-catalog|menu-text|menu-motion|setup-motion|join-stage-motion|setup-paper|join-paper|team-paper|weapon-paper|stage-paper]");
        const bool identity_only = argc == 4 && std::wstring(argv[3]) == L"identity";
        const bool quiet_only = argc == 4 && std::wstring(argv[3]) == L"quiet";
        const bool monitor_only = argc == 4 && std::wstring(argv[3]) == L"monitor";
        const bool countdown_only = argc == 4 && std::wstring(argv[3]) == L"countdown";
        const bool closing_only = argc == 4 && std::wstring(argv[3]) == L"closing";
        const bool supply_only = argc == 4 && std::wstring(argv[3]) == L"supply";
        const bool hud_only = argc == 4 && std::wstring(argv[3]) == L"hud";
        const bool resume_only = argc == 4 && std::wstring(argv[3]) == L"resume";
        const bool end_menu_only = argc == 4 && std::wstring(argv[3]) == L"end-menu";
        const bool pause_menu_only = argc == 4 && std::wstring(argv[3]) == L"pause-menu";
        const bool results_menu_only = argc == 4 && std::wstring(argv[3]) == L"results-menu";
        const bool main_menu_only = argc == 4 && std::wstring(argv[3]) == L"main-menu";
        const bool weapon_catalog_only = argc == 4 && std::wstring(argv[3]) == L"weapon-catalog";
        const bool menu_text_only = argc == 4 && std::wstring(argv[3]) == L"menu-text";
        const bool menu_motion_only = argc == 4 && std::wstring(argv[3]) == L"menu-motion";
        const bool settings_menu_only = argc == 4 && std::wstring(argv[3]) == L"settings-menu";
        const bool controls_menu_only = argc == 4 && std::wstring(argv[3]) == L"controls-menu";
        const bool stick_settings_only = argc == 4 && std::wstring(argv[3]) == L"stick-settings";
#if defined(COLOURWARS_PAINTER_SCRIPTED_PADS)
        require(argc == 4 && (std::wstring(argv[3]) == L"team-setup" || menu_text_only ||
            std::wstring(argv[3]) == L"setup-motion" || std::wstring(argv[3]) == L"join-stage-motion" || std::wstring(argv[3]) == L"join-paper" || std::wstring(argv[3]) == L"team-paper" || std::wstring(argv[3]) == L"weapon-paper" || std::wstring(argv[3]) == L"stage-paper"),
            "The scripted gamepad backend is restricted to focused team/setup/menu-text acceptance");
#else
        require(argc != 4 || (std::wstring(argv[3]) != L"team-setup" &&
            std::wstring(argv[3]) != L"setup-motion" && std::wstring(argv[3]) != L"join-stage-motion" && std::wstring(argv[3]) != L"join-paper" && std::wstring(argv[3]) != L"team-paper" && std::wstring(argv[3]) != L"weapon-paper" && std::wstring(argv[3]) != L"stage-paper"),
            "Use check-painter.ps1 focused scripted input switches to compile the scripted input seam");
#endif
        std::cout << std::unitbuf;
        const std::filesystem::path content = std::filesystem::absolute(argv[1]);
        const std::filesystem::path output = std::filesystem::absolute(argv[2]);
        std::filesystem::create_directories(output);
        std::ofstream records(output / "painter.csv");
        records << "capture,view,input_slot,painter_pixels,facing_right,tool_angle,respawn_progress,animation_state,action_age,action_frame,tool_pose,pressure,valve,travel_phase,mist_tail_remaining,muzzle_remaining,empty_remaining,recovered_remaining,hurt_remaining,elimination_remaining,death_x,death_y,death_width,death_height,breath,lean,blinking\n";
        Registry<LevelLoadedInfo> levels("Level");
        GameTuning tuning((content / "tuning.json").string().c_str());
        Presentation presentation((content / "presentation.json").string().c_str());
        Save saved_preferences;
        saved_preferences.load_save_file();
        require(!saved_preferences.play_settings().aim_marker, "Isolated visual save did not preserve aim_marker=false");
        PlaySettings preferences = saved_preferences.play_settings();
        preferences.music_volume = 0;
        preferences.sfx_volume = 0;
        // Existing comparisons isolate their own visual feature. The marker
        // matrix enables its preference explicitly at identical match state.
        preferences.aim_marker = false;
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
        if (argc == 4 && std::wstring(argv[3]) == L"setup-paper")
        {
            setup_paper_matrix(app, canvas, tuning, presentation, output);
            std::cout << "PASS setup-paper-only actual pages; normal reader, keyboard-driven only\n";
            return 0;
        }
#if defined(COLOURWARS_PAINTER_SCRIPTED_PADS)
        if (argc == 4 && std::wstring(argv[3]) == L"stage-paper")
        {
            stage_paper_matrix(app, levels, canvas, tuning, presentation, output);
            std::cout << "PASS stage-paper-only actual pages; scripted reader, no physical-pad acceptance\n";
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"weapon-paper")
        {
            weapon_paper_matrix(app, levels, canvas, tuning, presentation, output);
            std::cout << "PASS weapon-paper-only actual pages; scripted reader, no physical-pad acceptance\n";
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"team-paper")
        {
            team_paper_matrix(app, levels, canvas, tuning, presentation, output);
            std::cout << "PASS team-paper-only actual pages; scripted reader, no physical-pad acceptance\n";
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"join-paper")
        {
            join_paper_matrix(app, levels, canvas, tuning, presentation, output);
            std::cout << "PASS join-paper-only actual pages; scripted reader, no physical-pad acceptance\n";
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"join-stage-motion")
        {
            join_stage_motion_matrix(app, levels, canvas, tuning, presentation, output);
            std::cout << "PASS join-stage-motion-only actual pages; scripted backend, no physical-pad acceptance\n";
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"setup-motion")
        {
            setup_motion_matrix(app, levels, canvas, tuning, presentation, output);
            std::cout << "PASS setup-motion-only actual pages; scripted backend, no physical-pad acceptance\n";
            return 0;
        }
        if (argc == 4 && std::wstring(argv[3]) == L"team-setup")
        {
            team_setup_matrix(app, levels, tuning, presentation, output);
            std::cout << "PASS team-setup-only actual menu hardware check; scripted backend, no physical-pad acceptance\n";
            return 0;
        }
#endif
        if (menu_motion_only)
        {
            menu_motion_matrix(app, canvas, tuning, presentation, output);
            std::cout << "PASS menu-motion-only hardware check; other painter matrices were not run\n";
            return 0;
        }
        if (menu_text_only)
        {
            menu_text_matrix(app, levels, canvas, tuning, presentation, output);
            std::cout << "PASS menu-text-only hardware check; other painter matrices were not run\n";
            return 0;
        }
        if (weapon_catalog_only)
        {
            weapon_catalog_matrix(app, levels, tuning, presentation, output);
            std::cout << "PASS weapon-catalog-only hardware check; other painter matrices were not run\n";
            return 0;
        }
        if (stick_settings_only)
        {
            stick_settings_matrix(app, canvas, tuning, presentation, output);
            std::cout << "PASS stick-settings-only hardware check; other painter matrices were not run\n";
            return 0;
        }
        if (controls_menu_only)
        {
            controls_menu_matrix(app, levels, canvas, tuning, presentation, content, output);
            stick_settings_matrix(app, canvas, tuning, presentation, output);
            std::cout << "PASS controls-menu-only hardware check; other painter matrices were not run\n";
            return 0;
        }
        if (settings_menu_only)
        {
            settings_menu_matrix(app, canvas, tuning, presentation, output);
            std::cout << "PASS settings-menu-only hardware check; other painter matrices were not run\n";
            return 0;
        }
        if (main_menu_only)
        {
            main_menu_matrix(app, canvas, tuning, presentation, output);
            std::cout << "PASS main-menu-only hardware check; other painter matrices were not run\n";
            return 0;
        }
        if (results_menu_only)
        {
            results_menu_matrix(app, canvas, tuning, presentation, output);
            std::cout << "PASS results-menu-only hardware check; other painter matrices were not run\n";
            return 0;
        }
        if (pause_menu_only)
        {
            pause_menu_matrix(app, canvas, tuning, presentation, output);
            std::cout << "PASS pause-menu-only hardware check; other painter matrices were not run\n";
            return 0;
        }
        if (end_menu_only)
        {
            end_menu_matrix(app, canvas, tuning, presentation, output);
            std::cout << "PASS end-menu-only hardware check; other painter matrices were not run\n";
            return 0;
        }
        if (resume_only)
        {
            resume_matrix(app, levels, tuning, presentation, content, output);
            std::cout << "PASS resume-only actual GameLevel hardware check\n";
            return 0;
        }
        if (hud_only)
        {
            hud_matrix(app, builder, canvas, presentation, output);
            std::cout << "PASS HUD-only visual check; other painter matrices were not run\n";
            return 0;
        }
        if (supply_only)
        {
            supply_art_checks(app, builder, canvas, tuning, presentation, preferences, output);
            std::cout << "PASS supply-only visual check; other painter matrices were not run\n";
            return 0;
        }
        if (closing_only)
        {
            closing_matrix(app, builder, canvas, tuning, presentation, preferences, output);
            require(records.good(), "Capture record write failed");
            std::cout << "PASS closing-only visual check; other painter matrices were not run\n";
            return 0;
        }
        if (countdown_only)
        {
            countdown_matrix(app, builder, canvas, tuning, presentation, preferences, output);
            require(records.good(), "Capture record write failed");
            std::cout << "PASS countdown-only visual check; other painter matrices were not run\n";
            return 0;
        }
        if (monitor_only)
        {
            monitor_matrix(app, builder, canvas, tuning, presentation, preferences, output);
            require(records.good(), "Capture record write failed");
            std::cout << "PASS monitor-only visual check; other painter matrices were not run\n";
            return 0;
        }
        if (identity_only)
        {
            identity_matrix(app, builder, presentation, preferences, output);
            require(records.good(), "Capture record write failed");
            std::cout << "PASS identity-only visual check; other painter matrices were not run\n";
            return 0;
        }
        if (quiet_only)
        {
            quiet_matrix(app, builder, canvas, presentation, output, records);
            wet_tool_matrix(app, builder, canvas, presentation, preferences, output, records);
            require(records.good(), "Capture record write failed");
            std::cout << "PASS quiet-only visual check; other painter matrices were not run\n";
            return 0;
        }
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
        blocked_matrix(app, builder, canvas, presentation, output);
        contact_matrix(app, builder, canvas, tuning, presentation, output, records);
        quiet_matrix(app, builder, canvas, presentation, output, records);
        wet_tool_matrix(app, builder, canvas, presentation, preferences, output, records);
        reduced_effects_matrix(app, builder, canvas, presentation, preferences, output, records);
        shake_matrix(app, builder, tuning, presentation, preferences, output);
        identity_matrix(app, builder, presentation, preferences, output);
        monitor_matrix(app, builder, canvas, tuning, presentation, preferences, output);
        countdown_matrix(app, builder, canvas, tuning, presentation, preferences, output);
        closing_matrix(app, builder, canvas, tuning, presentation, preferences, output);
        hud_matrix(app, builder, canvas, presentation, output, false);
        supply_art_checks(app, builder, canvas, tuning, presentation, preferences, output);
        {
            // Depletion pays hundreds of shots without wall-clock audio
            // updates. A silent bank keeps this renderer check independent of
            // the hardware one-shot voice pool, as in CPU acceptance.
            AudioResources marker_audio;
            marker_audio.add_sound_bank("paint_exe", SoundBank::silent());
            LevelBuilder marker_builder(app.viewport_manager(), app.render_resources(),
                &marker_audio, &preferences, &levels, &tuning, &presentation,
                app.resolution_manager(), app.thread_pool(), app.partitioner(), app.mouse());
            marker_matrix(app, marker_builder, canvas, tuning, preferences, output);
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
        resume_matrix(app, levels, tuning, presentation, content, output);
        end_menu_matrix(app, canvas, tuning, presentation, output);
        pause_menu_matrix(app, canvas, tuning, presentation, output);
        results_menu_matrix(app, canvas, tuning, presentation, output);
        settings_menu_matrix(app, canvas, tuning, presentation, output);
        controls_menu_matrix(app, levels, canvas, tuning, presentation, content, output);
        stick_settings_matrix(app, canvas, tuning, presentation, output);
        menu_text_matrix(app, levels, canvas, tuning, presentation, output);
        weapon_catalog_matrix(app, levels, tuning, presentation, output);
        menu_motion_matrix(app, canvas, tuning, presentation, output);
        main_menu_matrix(app, canvas, tuning, presentation, output);
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
