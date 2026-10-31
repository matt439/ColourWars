#include "game/pch.h"
#include "game/input_slots.h"
#include "game/objects/level_builder.h"
#include "game/objects/player_badge.h"
#include "game/presentation.h"
#include "game/debug/dev_tools.h"
#if COLOURWARS_DEV_TOOLS
#include "game/debug/match_fingerprint.h"
#endif
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/step_timer.h"
#include "engine/render/sprite_font_file.h"
#include "scripts/acceptance/support/probe.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <type_traits>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include "scripts/acceptance/support/slot_seats.h"

using namespace labrador;
using namespace mattmath;

namespace
{
    using acceptance::require;

    bool close_to(float first, float second)
    {
        return std::abs(first - second) < 0.001f;
    }

    const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));

    // Metadata only: no graphics/audio device, window or player's save.
    struct World
    {
        RenderResources render;
        AudioResources audio;
        PlaySettings preferences;
        GameTuning tuning;
        Presentation presentation;
        Registry<LevelLoadedInfo> levels{ "Level" };
        ResolutionManager resolution;
        ViewportManager viewports{ &resolution };
        labrador::Mouse mouse;
        Partitioner partitioner;
        std::unique_ptr<LevelBuilder> builder;

        explicit World(const std::filesystem::path& content) :
            tuning((content / "tuning.json").string().c_str()),
            presentation((content / "presentation.json").string().c_str())
        {
            for (const char* sheet : { "sprite_sheet_1", "painter", "menu_paint", "weapon_paint", "paint_fx", "terrain" })
            {
                this->render.add_sprite_sheet(sheet, read_sprite_sheet(
                    (content / "textures" / (std::string(sheet) + ".json")).string().c_str(), TextureHandle{}));
            }
            for (const char* font : { "bowlby_one_144", "lato_black_24",
                "lato_black_36", "lato_black_48", "lato_black_72",
                "lato_black_144", "courier_prime_bold_16" })
            {
                const SpriteFontFile file = read_sprite_font_file(
                    (content / "fonts" / (std::string(font) + ".spritefont")).string());
                auto loaded = std::make_unique<Font>(TextureHandle{}, file.glyphs, file.line_spacing);
                if (loaded->contains(U'?')) { loaded->set_stand_in(U'?'); }
                this->render.add_font(font, std::move(loaded));
            }
            this->audio.add_sound_bank("paint_exe", SoundBank::silent());
            auto info = std::make_unique<LevelLoadedInfo>(
                (content / "levels/movement_course.json").string().c_str());
            info->validate();
            this->levels.add("movement_course", std::move(info));
            this->builder = std::make_unique<LevelBuilder>(&this->viewports, &this->render,
                &this->audio, &this->preferences, &this->levels, &this->tuning,
                &this->presentation, &this->resolution, nullptr, &this->partitioner);
        }

        std::unique_ptr<Level> build(int count = 4, OpeningMode opening = OpeningMode::off,
            float duration = 300.0f)
        {
            MatchSetup settings;
            settings.set_stage("movement_course");
            settings.set_player_count(count);
            settings.set_screen_layout(count == 1 ? ScreenLayout::one_player :
                count == 2 ? ScreenLayout::two_player : ScreenLayout::four_player);
            settings.set_duration(duration);
            constexpr std::array slots{ 3, 1, 2, 0 };
            constexpr std::array tools{ WeaponType::sprayer, WeaponType::sniper,
                WeaponType::mister, WeaponType::bucket };
            settings.begin_input_claims(slots[0]);
            for (int view = 1; view < count; ++view)
            {
                require(settings.claim_input_slot(slots.at(static_cast<size_t>(view))),
                    "Could not claim reordered identity slot");
            }
            for (int view = 0; view < count; ++view)
            {
                settings.set_player_team(view, PlayerTeam::a);
                settings.set_player_weapon(view, tools.at(static_cast<size_t>(view)));
            }
            this->viewports.set_layout(settings.screen_layout());
            return this->builder->build_level(settings, TeamColourPair::blue_orange, opening, MatchRandom());
        }
    };

    PlayerInputData input(bool fire = false, float angle = 0.0f)
    {
        PlayerInputData result;
        result.connected = true;
        result.primary_shoot = fire;
        result.shoot_direction_requested = true;
        result.shoot_angle = angle;
        result.shoot_direction = Vector2F::unit_vec_from_angle(angle);
        return result;
    }

    void activate(Level& level, const std::vector<PlayerInputData>& inputs)
    {
        for (int tick = 0; tick < 600 && level.state() != LevelState::active; ++tick)
        {
            level.update(acceptance::slot_seats(level, inputs), dt);
        }
        require(level.state() == LevelState::active, "Identity fixture did not reach live play");
    }

    void expire_emphasis(Level& level, const std::vector<PlayerInputData>& inputs)
    {
        for (int tick = 0; tick < 600 && level.player_identity_emphasis(0) > 0.0f; ++tick)
        {
            level.update(acceptance::slot_seats(level, inputs), dt);
        }
        require(level.player_identity_emphasis(0) == 0.0f,
            "Identity emphasis failed to expire within its bounded duration");
    }

    void check_roster_and_bounds(World& world)
    {
        static_assert(std::is_same_v<decltype(&Level::draw), void (Level::*)(Renderer&) const>);
        static_assert(std::is_same_v<decltype(&Level::player_identity_visible), bool (Level::*)(int) const>);
        static_assert(std::is_same_v<decltype(&Level::player_identity_bounds), RectangleF (Level::*)(int) const>);
        static_assert(std::is_same_v<decltype(&Level::player_identity_emphasis), float (Level::*)(int) const>);
        const auto& look = world.presentation.player_identity();
        constexpr std::array slots{ 3, 1, 2, 0 };
        for (const int count : { 1, 2, 4 })
        {
            auto level = world.build(count);
            require(!level->player_identity_visible(-1) && !level->player_identity_visible(count),
                "Invalid view acquired another local player's badge");
            for (int view = 0; view < count; ++view)
            {
                const auto index = static_cast<size_t>(view);
                require(level->player(view).player_num() == slots[index] &&
                    PlayerBadge::label(view) == L"P" + std::to_wstring(view + 1),
                    "Player badge identity followed device slot instead of the roster");
                require(level->place_player(view, { static_cast<float>(view * 250),
                    static_cast<float>(-view * 100) }), "Could not place identity player");
                require(level->player_identity_visible(view), "Countdown hid an alive local identity");
                const auto body = level->player(view).rectangle();
                const auto bounds = level->player_identity_bounds(view);
                const float emphasis = level->player_identity_emphasis(view);
                const float scale = 1.0f + (look.emphasis_scale - 1.0f) * emphasis;
                require(close_to(bounds.width, look.world_size.x * scale) &&
                    close_to(bounds.height, look.world_size.y * scale) &&
                    close_to(bounds.center().x, body.center().x + look.world_offset.x) &&
                    close_to(bounds.bottom(), body.top() + look.world_offset.y),
                    "Local badge bounds lost authored size or followed another view's figure");
            }
        }
        std::cout << "PASS player identity: shared P1-P4 roster labels and own-figure authored bounds at 1/2/4 players with reordered device slots\n";
    }

    void check_lifecycle(World& world)
    {
        const std::vector<PlayerInputData> inputs(input_slots::count, input());
        auto opening = world.build(4, OpeningMode::full);
        for (int view = 0; view < opening->player_count(); ++view)
        {
            require(!opening->player_identity_visible(view), "Story opening showed a local world badge");
        }
        opening.reset();
        auto level = world.build();
        const auto countdown_emphasis = level->player_identity_emphasis(0);
        require(close_to(countdown_emphasis, 1.0f), "Countdown did not begin with emphasized identity");
        level->update(acceptance::slot_seats(*level, inputs), dt);
        require(close_to(level->player_identity_emphasis(0), countdown_emphasis),
            "Countdown consumed the live identity emphasis budget");
        activate(*level, inputs);
        require(level->player_identity_emphasis(0) > 0.9f,
            "Entering live play did not start the identity emphasis");
        const auto first = level->player_identity_emphasis(0);
        level->update(acceptance::slot_seats(*level, inputs), dt);
        require(level->player_identity_emphasis(0) < first,
            "Positive live update did not age identity emphasis");

        level->suspend();
        const auto frozen = level->player_identity_emphasis(0);
        const auto frozen_bounds = level->player_identity_bounds(0);
        for (int repeat = 0; repeat < 20; ++repeat)
        {
            level->refresh_layout();
            level->update(acceptance::slot_seats(*level, inputs), repeat % 2 == 0 ? 0.0f : -dt);
            require(level->player_identity_visible(0) && level->player_identity_emphasis(0) == frozen &&
                level->player_identity_bounds(0) == frozen_bounds,
                "Frozen redraw or layout refresh consumed identity emphasis or moved its bounds");
        }
        level->resume();
        expire_emphasis(*level, inputs);
        const auto quiet = level->player_identity_bounds(0);
        level->emphasise_players();
        require(close_to(level->player_identity_emphasis(0), 1.0f) &&
            level->player_identity_bounds(0).width > quiet.width,
            "Resume hook failed to re-emphasize the quiet local badge");
        world.preferences.reduce_effects = true;
        require(level->player_identity_visible(0) && level->player_identity_emphasis(0) == 0.0f &&
            close_to(level->player_identity_bounds(0).width, quiet.width) &&
            close_to(level->player_identity_bounds(0).height, quiet.height),
            "Reduced effects hid identity or retained the emphasis enlargement");
        world.preferences.reduce_effects = false;
        // ROAD#7: view 0 holds Pad 4. Its own Reduce effects calms its badge;
        // another pane's viewer still sees the emphasis, and the reverse holds.
        const int own = level->player(0).player_num();
        const int other = own == 0 ? 1 : 0;
        require(own == 3, "Identity fixture did not seat Pad 4 in view 0");
        world.preferences.pad_comfort[static_cast<size_t>(own)].reduce_effects = SeatSwitch::on;
        require(level->player_identity_emphasis(0) == 0.0f &&
            close_to(level->player_identity_bounds(0).width, quiet.width) &&
            close_to(level->player_identity_emphasis_for(0, other), 1.0f) &&
            level->player_identity_bounds_for(0, other).width > quiet.width,
            "One pad's Reduce effects calmed another viewer's badge or missed its own");
        world.preferences.reduce_effects = true;
        world.preferences.pad_comfort[static_cast<size_t>(own)].reduce_effects = SeatSwitch::off;
        require(close_to(level->player_identity_emphasis(0), 1.0f) &&
            level->player_identity_emphasis_for(0, other) == 0.0f &&
            level->player_identity_emphasis_for(0, input_slots::keyboard_and_mouse) == 0.0f &&
            level->player_identity_emphasis_for(0, -1) == 0.0f,
            "A pad's Reduce effects Off did not win over the shared switch, or leaked to other viewers");
        world.preferences.reduce_effects = false;
        world.preferences.pad_comfort = {};
        auto& look = const_cast<PlayerIdentityPresentation&>(world.presentation.player_identity());
        look.enabled = false;
        require(!level->player_identity_visible(0), "Disabled identity presentation retained a world badge");
        look.enabled = true;

        expire_emphasis(*level, inputs);
        require(level->respawn_player(0) && level->player_identity_emphasis(0) == 0.0f,
            "Respawn request for an already-alive player invented a new identity event");
        require(level->kill_player(0) && !level->player_identity_visible(0),
            "Fatal health edge left the figure's identity visible before the dead-state tick");
        level->update(acceptance::slot_seats(*level, inputs), dt);
        require(level->player(0).state() == PlayerState::dead && !level->player_identity_visible(0),
            "Dead player's redraw carried an ordinary world identity badge");
        require(level->respawn_player(0) && level->player_identity_visible(0) &&
            close_to(level->player_identity_emphasis(0), 1.0f) && level->player_identity_emphasis(1) == 0.0f,
            "Actual developer respawn lost its emphasis or re-emphasized another pane");
        require(level->kill_player(0), "Could not eliminate natural-respawn identity player");
        level->update(acceptance::slot_seats(*level, inputs), dt);
        bool respawned = false;
        for (int tick = 0; tick < 600; ++tick)
        {
            level->update(acceptance::slot_seats(*level, inputs), dt);
            if (level->player(0).state() == PlayerState::alive)
            {
                require(level->player_identity_visible(0) && level->player_identity_emphasis(0) > 0.9f,
                    "Natural dead-to-alive transition failed to re-emphasize identity");
                respawned = true;
                break;
            }
            require(!level->player_identity_visible(0), "Identity became visible during unfinished redraw");
        }
        require(respawned, "Identity lifecycle probe missed natural respawn");

        auto short_round = world.build(4, OpeningMode::off, 0.5f);
        activate(*short_round, inputs);
        for (int tick = 0; tick < 120 && short_round->state() == LevelState::active; ++tick)
        {
            short_round->update(acceptance::slot_seats(*short_round, inputs), dt);
        }
        require(short_round->state() == LevelState::zoom_out, "Short identity fixture did not reach time-up");
        short_round->emphasise_players();
        for (int view = 0; view < short_round->player_count(); ++view)
        {
            require(!short_round->player_identity_visible(view),
                "Time-up overview retained a local identity or resume hook restored it");
        }
        std::cout << "PASS player identity: story/countdown/live/time-up visibility, age/frozen redraw, resume, reduced effects, fatal edge, redraw and separate natural/developer respawn\n";
    }

    void compare_gameplay(const Level& first, const Level& second)
    {
        require(first.state() == second.state() && first.projectile_count() == second.projectile_count() &&
            first.collision_object_count() == second.collision_object_count(),
            "Identity changed match state or scene object counts");
        for (int view = 0; view < first.player_count(); ++view)
        {
            const auto& a = first.player(view);
            const auto& b = second.player(view);
            require(a.rectangle() == b.rectangle() && a.velocity() == b.velocity() && a.health() == b.health() &&
                a.state() == b.state() && a.move_state() == b.move_state() && a.weapon_ammo() == b.weapon_ammo() &&
                a.primary_weapon().readiness() == b.primary_weapon().readiness() &&
                a.primary_weapon().rotation() == b.primary_weapon().rotation() &&
                a.primary_weapon().nozzle_pos() == b.primary_weapon().nozzle_pos(),
                "Identity changed movement, aim/muzzle, ammo/cadence, health, collisions or respawn");
        }
        const auto a = first.level_end_info(), b = second.level_end_info();
        require(a.player_areas == b.player_areas && a.team_a_area == b.team_a_area &&
            a.team_b_area == b.team_b_area && a.total_area == b.total_area,
            "Identity changed painted coverage or score");
    }

    void check_gameplay(World& enabled_world, const std::filesystem::path& content)
    {
        World disabled_world(content);
        auto& look = const_cast<PlayerIdentityPresentation&>(disabled_world.presentation.player_identity());
        look.enabled = false;
        std::srand(734);
        auto enabled = enabled_world.build();
        std::srand(734);
        auto disabled = disabled_world.build();
        std::vector<PlayerInputData> inputs(input_slots::count, input());
        activate(*enabled, inputs);
        activate(*disabled, inputs);
        bool shots = false;
        for (int tick = 0; tick < 360; ++tick)
        {
            for (auto& actions : inputs)
            {
                actions = input(tick < 150, static_cast<float>(tick % 16) * PI / 8.0f);
                actions.x_movement = tick < 40 ? 1.0f : 0.0f;
                actions.jump_pressed = tick == 15;
                actions.jump_held = tick >= 15 && tick < 30;
            }
            if (tick == 170)
            {
                require(enabled->kill_player(0) && disabled->kill_player(0),
                    "Could not eliminate identity-equivalence player");
            }
            if (tick % 30 == 0) { enabled->emphasise_players(); }
            const unsigned seed = static_cast<unsigned>(tick + 123);
            std::srand(seed); enabled->update(acceptance::slot_seats(*enabled, inputs), dt);
            std::srand(seed); disabled->update(acceptance::slot_seats(*disabled, inputs), dt);
            compare_gameplay(*enabled, *disabled);
            shots = shots || enabled->projectile_count() > 0;
            require(!disabled->player_identity_visible(0),
                "Disabled identity presentation reappeared during gameplay");
        }
        require(shots && enabled->player(0).state() == PlayerState::alive,
            "Identity comparison missed paid fire or natural respawn");
        std::cout << "PASS player identity on/off: real movement, aim, paid shots, ammo, collisions, paint/score, health/death and respawn unchanged with repeated visual emphasis\n";
    }

    void check_camera_integration(World& world, const std::filesystem::path& content)
    {
        auto& framing = const_cast<CameraPresentation&>(world.presentation.camera());
        struct Restore
        {
            CameraPresentation& live; CameraPresentation saved;
            ResolutionManager& resolution; Vector2I size;
            ~Restore() { this->live = this->saved; this->resolution.set_resolution_exactly(this->size); }
        } restore{ framing, framing, world.resolution, world.resolution.resolution_ivec() };
        framing.minimum_world_width = 1280.0f;
        framing.horizontal_lead_seconds = 0.25f; framing.horizontal_lead_max_distance = 200.0f;
        world.resolution.set_resolution_exactly({ 1280, 720 });
        const LevelLoadedInfo source((content / "levels/movement_course.json").string().c_str());
        const auto rectangle = [&](const char* name)
        {
            const JsonValue objects = source.collision_objects_json();
            for (size_t i = 0; i < objects.size(); ++i)
            {
                const JsonValue object = objects.at(i);
                if (object.string("name") != name) { continue; }
                const JsonValue r = object.object("rectangle");
                return RectangleF(r.number("x"), r.number("y"), r.number("width"), r.number("height"));
            }
            throw std::runtime_error("Camera integration lost its authored Course station");
        };
        const RectangleF runway = rectangle("long_landing_running_takeoff");
        const RectangleF bounds = source.camera_bounds_rectangle();
        const Vector2F size = world.tuning.player().size;
        std::vector<PlayerInputData> neutral(input_slots::count, input()), moving = neutral;
        auto level = world.build(); activate(*level, neutral);
        require(level->place_player(0, { runway.left() + size.x, runway.top() - size.y }),
            "Could not start camera integration on the actual interior runway");
        for (int tick = 0; tick < 8; ++tick) { level->update(acceptance::slot_seats(*level, neutral), dt); }
        moving[static_cast<size_t>(level->player(0).player_num())].x_movement = 1.0f;
        for (int tick = 0; tick < 90 && level->player(0).velocity().x < world.tuning.player().max_velocity.x; ++tick)
        { level->update(acceptance::slot_seats(*level, moving), dt); }
        const Player& player = level->player(0);
        const float retained_velocity = player.velocity().x;
        require(retained_velocity == world.tuning.player().max_velocity.x &&
            player.rectangle().right() < runway.right() && close_to(player.rectangle().bottom(), runway.top()),
            "Camera integration did not naturally accelerate on the authored runway");
        const PlayerCamera camera_policy{};
        const auto refresh = [&](const Vector2I& resolution, bool zero_lead)
        {
            std::array<Camera, 4> expected;
            world.resolution.set_resolution_exactly(resolution);
            for (int view = 0; view < 4; ++view)
            {
                const Player& p = level->player(view);
                expected[static_cast<size_t>(view)] = camera_policy.follow(p.camera_focus(),
                    world.viewports.player_viewport(view), static_cast<float>(resolution.y), framing,
                    p.camera(), bounds, zero_lead ? 0.0f : p.velocity().x);
            }
            level->refresh_layout();
            for (int view = 0; view < 4; ++view)
            { require(level->player(view).camera() == expected[static_cast<size_t>(view)],
                "Level camera refresh disagrees with the public follow policy or expected zero lead"); }
        };
        level->suspend();
        const auto simulation = [&]
        {
            const auto seat = [&](int view)
            {
                const Player& p = level->player(view);
                return std::tuple(p.rectangle(), p.velocity(), p.health(), p.weapon_ammo(), p.respawn_timer(),
                    p.state(), p.move_state(), p.animation_state(), p.primary_weapon().readiness(),
                    p.primary_weapon().rotation(), p.primary_weapon().nozzle_pos(),
                    level->player_identity_emphasis(view), level->player_identity_bounds(view));
            };
            const auto score = level->level_end_info();
            return std::tuple(level->state(), level->projectile_count(), level->collision_object_count(),
                score.player_areas, score.team_a_area, score.team_b_area, score.total_area,
                std::array{ seat(0), seat(1), seat(2), seat(3) });
        };
        const auto frozen = simulation();
#if COLOURWARS_DEV_TOOLS
        const auto fingerprint = MatchFingerprint::of(*level, 0);
#endif
        for (const Vector2I resolution : { Vector2I(1920, 1080), Vector2I(2560, 1440), Vector2I(1281, 721),
            Vector2I(1600, 1200), Vector2I(2560, 720), Vector2I(1280, 720) })
        {
            refresh(resolution, false);
            for (int view = 0; view < 4; ++view)
            {
                const Player& p = level->player(view);
                const Viewport pane(world.viewports.player_viewport(view).pixel_rect());
                const Camera camera = p.camera();
                const RectangleF visible = camera.visible_rectangle(pane);
                require(visible.contains(p.rectangle()) && visible.contains(level->player_identity_bounds(view)),
                    "Suspended C2 resize lost the actual character or its current badge bounds");
                for (const Vector2F offset : { Vector2F(100, -80), Vector2F(-100, 80) })
                {
                    const Vector2F cursor = camera.calculate_view_position(p.center() + offset) + pane.position();
                    const Vector2F aim = PlayerCamera::mouse_direction(p.center(), cursor, pane, camera);
                    require(close_to(aim.x, offset.normalized().x) && close_to(aim.y, offset.normalized().y),
                        "Suspended C2 resize projected mouse aim from the virtual lead focus");
                }
                const Vector2F cursor = camera.calculate_view_position(p.center()) + pane.position();
                require(PlayerCamera::mouse_direction(p.center(), cursor, pane, camera) == Vector2F::ZERO,
                    "Suspended C2 resize invented aim at the physical player centre");
                level->refresh_layout();
                require(p.camera() == camera, "Repeated suspended resize/restore refresh drifted the camera");
            }
            require(simulation() == frozen, "Suspended C2 resize advanced public simulation or badge state");
#if COLOURWARS_DEV_TOOLS
            require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(*level, 0)).empty(),
                "Suspended C2 resize changed the frozen match fingerprint");
#endif
        }
        level->resume();
        require(level->kill_player(0) && player.state() == PlayerState::alive && player.health() == 0.0f &&
            player.velocity().x == retained_velocity, "Known fatal health signal did not retain the moving pose");
        const Vector2F death_centre = player.center();
        const Camera previous = player.camera();
        world.resolution.set_resolution_exactly({ 2560, 720 });
        const Viewport wide = world.viewports.player_viewport(0);
        const Camera zero = camera_policy.follow(player.camera_focus(), wide, 720.0f, framing, previous, bounds);
        const Camera stale = camera_policy.follow(player.camera_focus(), wide, 720.0f, framing, previous, bounds, retained_velocity);
        require(zero != stale, "Fatal camera guard fixture was hidden by the follow deadzone or arena bounds");
        refresh({ 2560, 720 }, true); // Public resize sees health zero before a Player dead-state tick.
        bool saw_hold = false, saw_spawn = false, respawned = false;
        for (int tick = 0; tick < 600; ++tick)
        {
            level->update(acceptance::slot_seats(*level, neutral), dt);
            if (player.state() == PlayerState::alive)
            {
                require(player.health() == world.tuning.player().starting_health && player.velocity() == Vector2F::ZERO &&
                    player.rectangle() == player.spawn_rectangle(), "Natural camera respawn did not reset the actual player");
                refresh({ 1920, 1080 }, false); respawned = true; break;
            }
            const bool hold = world.tuning.player().respawn_delay - player.respawn_timer() <
                world.presentation.respawn_redraw().camera_hold;
            require(player.velocity().x == retained_velocity &&
                player.camera_focus() == (hold ? death_centre : player.spawn_rectangle().center()),
                "Dead camera lost its retained velocity control or body-to-spawn focus transition");
            if ((hold && !saw_hold) || (!hold && !saw_spawn))
            { refresh(hold ? Vector2I(1281, 721) : Vector2I(1280, 720), true); }
            saw_hold = saw_hold || hold; saw_spawn = saw_spawn || !hold;
        }
        require(saw_hold && saw_spawn && respawned, "Camera lifecycle missed dead hold, spawn focus or natural respawn");

        // A separate station uses the real 60-unit short_step side wall. The
        // current Level refresh precedes late contacts; only the next public
        // refresh is compared with resolved zero velocity, not the contact frame.
        const RectangleF approach = rectangle("running_landing"), wall = rectangle("short_step");
        require(approach.right() == wall.left() && wall.top() < approach.top() &&
            level->place_player(0, { approach.left() + size.x, approach.top() - size.y }),
            "Camera wall trial lost its authored runway/step connection");
        bool hit_wall = false;
        for (int tick = 0; tick < 180; ++tick)
        {
            const float before = player.velocity().x; level->update(acceptance::slot_seats(*level, moving), dt);
            if (before > 0.0f && player.velocity().x == 0.0f && close_to(player.rectangle().right(), wall.left()))
            { hit_wall = true; break; }
        }
        require(hit_wall, "Input-driven camera wall trial did not make an actual authored side contact");
        refresh({ 1281, 721 }, false);
        std::cout << "PASS C2 Level camera integration: input-only runway acceleration, resolved authored wall, "
            "known fatal health-zero/retained-velocity guard, dead hold/spawn focus/natural respawn; suspended 4P "
            "720p/1080p/QHD/odd/narrow/wide aspect refreshes preserve public state and Debug fingerprint, "
            "body/badge space and physical-player mouse rays. Late endpoint-fatal contact frames and human comfort remain unqualified\n";
    }

    void check_validation(const std::filesystem::path& content, const std::filesystem::path& scratch)
    {
        std::ifstream input_file(content / "presentation.json");
        require(input_file.good(), "Cannot read identity presentation");
        const std::string source(std::istreambuf_iterator<char>(input_file), {});
        const auto path = scratch / "player-identity-invalid.json";
        struct Cleanup
        {
            std::filesystem::path path;
            ~Cleanup() { std::error_code ignored; std::filesystem::remove(this->path, ignored); }
        } cleanup{ path };
        const auto reject = [&](auto edit)
        {
            rapidjson::Document json;
            json.Parse(source.c_str());
            require(!json.HasParseError(), "Cannot parse identity presentation fixture");
            edit(json["player_identity"]);
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            json.Accept(writer);
            { std::ofstream variant(path); variant << buffer.GetString(); require(variant.good(), "Cannot write identity variant"); }
            bool rejected = false;
            try { const Presentation invalid(path.string().c_str()); }
            catch (const std::exception& error)
            {
                rejected = std::string(error.what()).find("player_identity") != std::string::npos;
            }
            require(rejected, "Malformed identity presentation was accepted or lost its section name");
        };
        reject([](rapidjson::Value& look) { look.RemoveMember("enabled"); });
        reject([](rapidjson::Value& look) { look["enabled"].SetInt(1); });
        for (const char* field : { "world_size", "world_offset", "ui_size", "bot_label_size" })
        {
            reject([field](rapidjson::Value& look) { look.RemoveMember(field); });
            reject([field](rapidjson::Value& look) { look[field].SetString("small"); });
            reject([field](rapidjson::Value& look) { look[field].RemoveMember("x"); });
            reject([field](rapidjson::Value& look) { look[field]["y"].SetString("small"); });
        }
        for (const char* field : { "world_size", "ui_size", "bot_label_size" })
        {
            for (const char* axis : { "x", "y" })
            {
                reject([field, axis](rapidjson::Value& look) { look[field][axis].SetFloat(0.0f); });
                reject([field, axis](rapidjson::Value& look) { look[field][axis].SetFloat(-1.0f); });
                reject([field, axis](rapidjson::Value& look) {
                    look[field][axis].SetDouble(std::numeric_limits<double>::max()); });
            }
        }
        for (const char* field : { "emphasis_duration", "emphasis_scale", "ui_gap", "border", "bot_label_gap" })
        {
            reject([field](rapidjson::Value& look) { look.RemoveMember(field); });
            reject([field](rapidjson::Value& look) { look[field].SetString("small"); });
            reject([field](rapidjson::Value& look) { look[field].SetFloat(-1.0f); });
            reject([field](rapidjson::Value& look) {
                look[field].SetDouble(std::numeric_limits<double>::max()); });
        }
        for (const char* field : { "emphasis_duration", "ui_gap", "border" })
        {
            reject([field](rapidjson::Value& look) { look[field].SetFloat(0.0f); });
        }
        reject([](rapidjson::Value& look) { look["emphasis_scale"].SetFloat(0.9f); });
        reject([](rapidjson::Value& look) { look["emphasis_scale"].SetFloat(2.01f); });
        reject([](rapidjson::Value& look) {
            look["world_offset"]["x"].SetDouble(std::numeric_limits<double>::max()); });
        for (const char* field : { "background_colour", "ink_colour" })
        {
            reject([field](rapidjson::Value& look) { look.RemoveMember(field); });
            reject([field](rapidjson::Value& look) { look[field].SetInt(3); });
            reject([field](rapidjson::Value& look) { look[field].SetString("not-a-colour"); });
            reject([field](rapidjson::Value& look) { look[field].SetString("11223380"); });
        }
        reject([](rapidjson::Value& look) {
            look["ink_colour"].SetString(rapidjson::StringRef(look["background_colour"].GetString())); });
        reject([](rapidjson::Value& look) { look["border"].SetFloat(look["ui_size"]["y"].GetFloat() / 2.0f); });
        std::cout << "PASS player identity content: required finite typed geometry, positive sizes/gap/duration, bounded emphasis, distinct opaque colours and visible badge interior\n";
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("player identity", [&]
    {
        require(argc == 2, "Usage: player_identity.exe <content directory>");
        const std::filesystem::path content(argv[1]);
        World world(content);
        check_roster_and_bounds(world);
        check_lifecycle(world);
        check_gameplay(world, content);
        check_camera_integration(world, content);
        check_validation(content, std::filesystem::absolute(argv[0]).parent_path());
        return 0;
    });
}
