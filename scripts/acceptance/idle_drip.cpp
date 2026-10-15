#include "game/pch.h"
#include "game/input_slots.h"
#include "game/objects/idle_drip.h"
#include "game/objects/level_builder.h"
#include "game/objects/structure.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/step_timer.h"
#include "engine/math/triangle.h"
#include "engine/render/sprite_font_file.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <type_traits>

using namespace labrador;
using namespace mattmath;

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    bool nearly_equal(float first, float second)
    {
        return std::abs(first - second) < 0.0001f;
    }

    const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));

    auto observation(const IdleDrip& drip)
    {
        return std::tuple{ drip.attached(), drip.bead_age(), drip.interval_remaining(),
            drip.bead_rectangle(), drip.bounds(),
            std::vector<IdleDrip::Drop>(drip.drops().begin(), drip.drops().end()) };
    }

    void attach(IdleDrip& drip, const Vector2F& nozzle = Vector2F::ZERO,
        float angle = 0.0f)
    {
        drip.advance(true, nozzle, angle, drip.interval_remaining() + 0.001f, {});
        require(drip.attached() && drip.bead_age() == 0.0f,
            "Quiet interval did not begin a fresh attached bead");
    }

    void detach(IdleDrip& drip, const IdleDripPresentation& look,
        const Vector2F& nozzle = Vector2F::ZERO)
    {
        attach(drip, nozzle);
        drip.advance(true, nozzle, 0.0f, look.grow_time + look.stretch_time + 0.001f, {});
        require(!drip.attached() && !drip.drops().empty() && drip.drops().back().age == 0.0f,
            "Grown and stretched bead did not detach into a fresh falling drop");
    }

    // Only metadata is loaded. These objects never create a device or a save.
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
            for (const char* sheet : { "sprite_sheet_1", "painter", "menu_paint", "weapon_paint", "paint_fx" })
            {
                this->render.add_sprite_sheet(sheet, read_sprite_sheet(
                    (content / "textures" / (std::string(sheet) + ".json")).string().c_str(), TextureHandle{}));
            }
            for (const char* font : { "gill_sans_ultra_bold_144", "gill_sans_mt_bold_24",
                "gill_sans_mt_bold_36", "gill_sans_mt_bold_48", "gill_sans_mt_bold_72",
                "gill_sans_mt_bold_144", "courier_new_bold_16" })
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
                &this->presentation, &this->resolution, nullptr, &this->partitioner, &this->mouse);
        }

        std::unique_ptr<Level> build(float duration = 300.0f)
        {
            MenuLevelSettings settings;
            settings.set_stage("movement_course");
            settings.set_player_count(4);
            settings.set_screen_layout(ScreenLayout::four_player);
            settings.set_duration(duration);
            constexpr std::array tools{ WeaponType::sprayer, WeaponType::sniper,
                WeaponType::roller, WeaponType::mister };
            for (int index = 0; index < 4; ++index)
            {
                settings.set_player_team(index, PlayerTeam::a);
                settings.set_player_weapon(index, tools.at(static_cast<size_t>(index)));
            }
            settings.begin_input_claims(0);
            for (int slot = 1; slot < 4; ++slot) { require(settings.claim_input_slot(slot), "Could not claim CPU pad slot"); }
            this->viewports.set_layout(settings.screen_layout());
            return this->builder->build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
        }

        std::unique_ptr<Structure> structure(std::unique_ptr<Shape> shape,
            CollisionObjectType type = CollisionObjectType::structure)
        {
            const RectangleF rectangle = shape->bounding_box();
            return std::make_unique<Structure>("sprite_sheet_1", "square_white_4", rectangle,
                std::move(shape), &this->render, type);
        }
    };

    void check_quiet_intervals(World& world)
    {
        const auto& look = world.presentation.idle_drip();
        IdleDrip first(&look, &world.render, Colour::white, 0);
        const float initial = first.interval_remaining();
        for (int tick = 0; tick < 1200; ++tick)
        {
            first.advance(false, Vector2F::ZERO, 0.0f, dt, {});
            require(!first.attached() && first.drops().empty() && first.interval_remaining() == initial,
                "Unsettled time consumed the next quiet interval or created a drip");
        }
        first.advance(true, Vector2F::ZERO, 0.0f, initial * 0.5f, {});
        require(!first.attached() && first.interval_remaining() < initial,
            "Settled sprayer skipped the wait or failed to age its quiet interval");
        first.advance(false, Vector2F::ZERO, 0.0f, dt, {});
        const float restarted = first.interval_remaining();
        require(restarted >= look.interval_min && restarted <= look.interval_max,
            "Activity did not schedule a complete new quiet interval");
        first.advance(true, Vector2F::ZERO, 0.0f, restarted * 0.5f, {});
        require(!first.attached(), "Interrupted quiet time leaked into the next idle interval");

        std::srand(8123);
        const int expected_random = std::rand();
        std::srand(8123);
        IdleDrip same_a(&look, &world.render, Colour::white, 3);
        IdleDrip same_b(&look, &world.render, Colour::white, 3);
        IdleDrip different(&look, &world.render, Colour::white, 4);
        bool varied = false;
        bool distinct = false;
        float previous = same_a.interval_remaining();
        for (int cycle = 0; cycle < 12; ++cycle)
        {
            require(observation(same_a) == observation(same_b),
                "Equal roster identities did not reproduce the local cosmetic sequence");
            distinct = distinct || same_a.interval_remaining() != different.interval_remaining();
            detach(same_a, look);
            detach(same_b, look);
            detach(different, look);
            const float interval = same_a.interval_remaining();
            require(interval >= look.interval_min && interval <= look.interval_max,
                "Local drip interval escaped its presentation range");
            varied = varied || interval != previous;
            previous = interval;
        }
        require(varied && distinct && std::rand() == expected_random,
            "Drip timing repeated uniformly, shared identities or consumed gameplay rand()");
        for (int cycle = 0; cycle < 6; ++cycle)
        {
            detach(same_a, look);
            std::ignore = std::rand();
            detach(same_b, look);
            require(observation(same_a) == observation(same_b),
                "Gameplay random draws changed the drip's private interval sequence");
        }
        std::cout << "PASS idle drip: settled waits, interrupted waits, varied per-seat intervals and isolated RNG\n";
    }

    void check_attachment_and_falling(World& world)
    {
        const auto& look = world.presentation.idle_drip();
        const Vector2F nozzle(120.0f, 80.0f);
        IdleDrip right(&look, &world.render, Colour::white, 0);
        IdleDrip left(&look, &world.render, Colour::white, 0);
        attach(right, nozzle);
        attach(left, nozzle, PI);
        require(nearly_equal(right.bead_rectangle().center().x, nozzle.x - look.nozzle_inset) &&
            nearly_equal(left.bead_rectangle().center().x, nozzle.x + look.nozzle_inset) &&
            nearly_equal(right.bead_rectangle().y, nozzle.y + look.nozzle_underside) &&
            nearly_equal(left.bead_rectangle().y, right.bead_rectangle().y),
            "Facing flip moved the residual bead off the tool's nozzle underside");

        const RectangleF initial_bead = right.bead_rectangle();
        right.advance(true, nozzle, 0.0f, look.grow_time * 0.5f, {});
        const RectangleF growing = right.bead_rectangle();
        require(growing.width > initial_bead.width && nearly_equal(growing.height, growing.width) &&
            nearly_equal(growing.y, initial_bead.y), "Attached bead did not grow from its nozzle attachment");
        right.advance(true, nozzle, 0.0f, look.grow_time * 0.5f + look.stretch_time * 0.5f, {});
        const RectangleF stretched = right.bead_rectangle();
        require(nearly_equal(stretched.width, look.size) && stretched.height > stretched.width &&
            nearly_equal(stretched.y, initial_bead.y), "Grown bead did not stretch downwards before release");
        const Vector2F moved_nozzle(140.0f, 50.0f);
        const float angle = PI * 0.25f;
        left.advance(true, moved_nozzle, angle, look.grow_time * 0.25f, {});
        const Vector2F expected_anchor = moved_nozzle +
            Vector2F::rotate_vector(Vector2F(-look.nozzle_inset, look.nozzle_underside), angle);
        require(nearly_equal(left.bead_rectangle().center().x, expected_anchor.x) &&
            nearly_equal(left.bead_rectangle().y, expected_anchor.y),
            "Continuous aim or translation left an attached bead behind the nozzle");

        right.advance(true, nozzle, 0.0f, look.stretch_time, {});
        require(!right.attached() && right.drops().size() == 1, "Stretched bead failed to release");
        const Vector2F released = right.drops()[0].position;
        const float fall_step = std::min(0.05f, look.lifetime * 0.1f);
        right.advance(false, Vector2F(900, -900), PI, fall_step, {});
        require(right.drops().size() == 1 && right.drops()[0].position.x == released.x &&
            right.drops()[0].position.y > released.y,
            "Detached paint followed later aim or failed to fall under gravity");
        const float first_fall = right.drops()[0].position.y - released.y;
        const float before_second = right.drops()[0].position.y;
        right.advance(false, Vector2F(-900, 900), -PI * 0.5f, fall_step, {});
        require(right.drops()[0].position.y - before_second > first_fall,
            "Falling residual paint did not accelerate under gravity");

        left.advance(false, nozzle, 0.0f, dt, {});
        require(!left.attached() && left.drops().empty(), "Activity failed to cancel an attached bead");
        right.advance(false, nozzle, 0.0f, look.lifetime, {});
        require(right.drops().empty(), "Falling residual paint exceeded its bounded lifetime");
        std::cout << "PASS idle drip: both facings, aimed attachment, growth/stretch, independent gravity and cancellation\n";
    }

    void check_terrain(World& world)
    {
        const auto& look = world.presentation.idle_drip();
        IdleDrip thin(&look, &world.render, Colour::white, 0);
        detach(thin, look);
        const float x = thin.drops()[0].position.x;
        const float top = thin.drops()[0].position.y + 20.0f;
        auto platform = world.structure(std::make_unique<RectangleF>(x - 20.0f, top, 40.0f, 0.1f),
            CollisionObjectType::structure_jump_through);
        const std::array<CollisionObject*, 1> platform_targets{ platform.get() };
        thin.advance(false, Vector2F::ZERO, 0.0f, 0.5f, platform_targets);
        require(thin.drops().empty(), "Residual paint tunnelled through a thin jump-through platform");

        IdleDrip ramp(&look, &world.render, Colour::white, 0);
        detach(ramp, look, Vector2F(10.0f + look.nozzle_inset, 0.0f));
        auto slope = world.structure(std::make_unique<Triangle>(0.0f, 100.0f, 100.0f, 0.0f, 100.0f, 100.0f),
            CollisionObjectType::structure_ramp_right);
        const std::array<CollisionObject*, 1> ramp_targets{ slope.get() };
        ramp.advance(false, Vector2F::ZERO, 0.0f, 0.35f, ramp_targets);
        require(ramp.drops().size() == 1,
            "Ramp bounding box retired a drop still above its actual sloping surface");
        ramp.advance(false, Vector2F::ZERO, 0.0f, 0.25f, ramp_targets);
        require(ramp.drops().empty(), "Residual paint passed through the ramp's actual surface");

        IdleDrip blocked(&look, &world.render, Colour::white, 0);
        auto wall = world.structure(std::make_unique<RectangleF>(-20.0f, 0.0f, 40.0f, 30.0f));
        const std::array<CollisionObject*, 1> wall_targets{ wall.get() };
        blocked.advance(true, Vector2F::ZERO, 0.0f, blocked.interval_remaining() + 0.001f, wall_targets);
        require(!blocked.attached() && blocked.drops().empty(),
            "Attached paint grew inside terrain instead of disappearing on contact");

        IdleDrip ignored(&look, &world.render, Colour::white, 0);
        auto projectile_tag = world.structure(std::make_unique<RectangleF>(-20.0f, 0.0f, 40.0f, 30.0f),
            CollisionObjectType::projectile_spray_team_a);
        const std::array<CollisionObject*, 1> ignored_targets{ projectile_tag.get() };
        ignored.advance(true, Vector2F::ZERO, 0.0f, ignored.interval_remaining() + 0.001f, ignored_targets);
        require(ignored.attached(), "Residual paint treated a non-terrain collision object as scenery");
        std::cout << "PASS idle drip: swept thin-platform contact, true ramp geometry, attached contact and terrain filtering\n";
    }

    void check_freeze_disable_and_capacity(World& world)
    {
        static_assert(!std::is_base_of_v<CollisionObject, IdleDrip>);
        static_assert(std::is_same_v<decltype(&IdleDrip::draw), void (IdleDrip::*)(DrawList&) const>);
        auto look = world.presentation.idle_drip();
        look.lifetime = 100.0f;
        IdleDrip drip(&look, &world.render, Colour::white, 0);
        detach(drip, look);
        attach(drip);
        const auto before = observation(drip);
        for (float elapsed : { 0.0f, -1.0f, std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::quiet_NaN() })
        {
            drip.advance(false, Vector2F(900, 900), PI, elapsed, {});
            drip.update(elapsed);
            require(observation(drip) == before, "Frozen or invalid delta advanced residual-paint state");
        }

        const auto falling = drip.drops()[0];
        drip.advance(false, Vector2F(900, 900), PI, dt, {});
        require(!drip.attached() && drip.drops().size() == 1 &&
            drip.drops()[0].position.x == falling.position.x &&
            drip.drops()[0].position.y > falling.position.y,
            "Activity cancelled an existing falling drop or retained its attached bead");
        attach(drip);
        const auto before_draw = observation(drip);

        look.enabled = false;
        DrawList unused;
        for (int view = 0; view < 4; ++view) { drip.draw(unused); }
        require(observation(drip) == before_draw, "Disabled draw fan-out mutated simulation state");
        drip.advance(true, Vector2F::ZERO, 0.0f, dt, {});
        require(!drip.attached() && drip.drops().empty(), "Disabled presentation retained active residual paint");
        for (int tick = 0; tick < 600; ++tick) { drip.advance(true, Vector2F::ZERO, 0.0f, dt, {}); }
        require(!drip.attached() && drip.drops().empty(), "Disabled presentation emitted residual paint");

        look.enabled = true;
        look.interval_min = 0.001f;
        look.interval_max = 0.002f;
        look.grow_time = 0.001f;
        look.stretch_time = 0.001f;
        look.lifetime = 1000.0f;
        look.gravity = 0.0f;
        IdleDrip crowded(&look, &world.render, Colour::white, 0);
        for (int tick = 0; tick < 1000; ++tick)
        {
            crowded.advance(true, Vector2F::ZERO, 0.0f, dt, {});
            require(crowded.drops().size() <= 4, "Residual paint exceeded its fixed four-drop storage");
        }
        require(crowded.drops().size() == 4, "Capacity fixture never exercised full residual-paint storage");
        attach(crowded);
        crowded.clear();
        require(!crowded.attached() && crowded.drops().empty() && crowded.bounds() == RectangleF::ZERO,
            "Explicit scene-ending clear retained attached or falling residual paint");
        crowded.clear();
        require(!crowded.attached() && crowded.drops().empty(), "Residual-paint clear was not idempotent");
        std::cout << "PASS idle drip: frozen deltas, const/disabled draw, feature switch and fixed capacity\n";
    }

    PlayerInputData input()
    {
        PlayerInputData result;
        result.connected = true;
        result.shoot_direction_requested = true;
        result.shoot_direction = Vector2F(1, 0);
        return result;
    }

    void compare_gameplay(const Level& first, const Level& second)
    {
        require(first.state() == second.state() && first.projectile_count() == second.projectile_count() &&
            first.collision_object_count() == second.collision_object_count(),
            "Idle-drip feature switch changed level state or gameplay-object counts");
        for (int view = 0; view < first.player_count(); ++view)
        {
            const Player& a = first.player(view);
            const Player& b = second.player(view);
            require(a.rectangle() == b.rectangle() && a.velocity() == b.velocity() &&
                a.state() == b.state() && a.move_state() == b.move_state() && a.health() == b.health() &&
                a.weapon_ammo() == b.weapon_ammo() &&
                a.primary_weapon().ready_to_shoot() == b.primary_weapon().ready_to_shoot() &&
                a.primary_weapon().rotation() == b.primary_weapon().rotation() &&
                a.primary_weapon().nozzle_pos() == b.primary_weapon().nozzle_pos(),
                "Idle-drip feature switch changed movement, damage, paint, shot readiness or muzzle");
        }
        const auto a = first.level_end_info();
        const auto b = second.level_end_info();
        require(a.player_areas == b.player_areas && a.team_a_area == b.team_a_area &&
            a.team_b_area == b.team_b_area && a.total_area == b.total_area,
            "Cosmetic idle drips captured a surface or changed the paint score");
    }

    void check_level_integration(World& enabled_world, const std::filesystem::path& content)
    {
        World disabled_world(content);
        // The original Presentation is mutable; only its read-only accessor is
        // exposed to clients. Alter the fixture's switch before building Level.
        const_cast<IdleDripPresentation&>(disabled_world.presentation.idle_drip()).enabled = false;
        std::srand(753);
        auto enabled = enabled_world.build();
        std::srand(753);
        auto disabled = disabled_world.build();
        require(enabled->idle_drip(0) != nullptr && enabled->idle_drip(1) == nullptr &&
            enabled->idle_drip(2) == nullptr && enabled->idle_drip(3) == nullptr,
            "Level attached residual-paint decoration to a non-sprayer tool");
        std::vector<PlayerInputData> inputs(input_slots::count, input());
        const auto tick = [&]
        {
            enabled->update(inputs, dt);
            disabled->update(inputs, dt);
            compare_gameplay(*enabled, *disabled);
            require(!disabled->idle_drip(0)->attached() && disabled->idle_drip(0)->drops().empty(),
                "Disabled Level emitted a cosmetic drip");
        };
        const auto wait_for_bead = [&]
        {
            for (int step = 0; step < 1200 && !enabled->idle_drip(0)->attached(); ++step) { tick(); }
            require(enabled->idle_drip(0)->attached(), "Real Level never delivered settled player time to its drip");
        };
        wait_for_bead();
        const auto paused = observation(*enabled->idle_drip(0));
        for (int step = 0; step < 10; ++step) { enabled->update(inputs, 0.0f); }
        require(observation(*enabled->idle_drip(0)) == paused,
            "Frozen Level update advanced residual-paint state");

        bool fell = false;
        const auto untouched_score = enabled->level_end_info().player_areas;
        const float untouched_ammo = enabled->player(0).weapon_ammo();
        for (int step = 0; step < 600; ++step)
        {
            tick();
            fell = fell || !enabled->idle_drip(0)->drops().empty();
        }
        require(fell && enabled->level_end_info().player_areas == untouched_score &&
            enabled->player(0).weapon_ammo() == untouched_ammo,
            "Real idle drip failed to fall, spent ammunition or captured a surface");

        wait_for_bead();
        inputs[0].x_movement = 1.0f;
        tick();
        require(!enabled->idle_drip(0)->attached(), "Resolved walking failed to cancel the attached bead");
        for (int step = 0; step < 12; ++step) { tick(); }
        inputs[0] = input();
        wait_for_bead();
        inputs[0].primary_shoot = true;
        tick();
        require(!enabled->idle_drip(0)->attached(), "Firing failed to cancel the attached bead");
        inputs[0] = input();
        wait_for_bead();
        inputs[0].jump_pressed = true;
        inputs[0].jump_held = true;
        tick();
        require(!enabled->idle_drip(0)->attached(), "Jumping failed to cancel the attached bead");
        inputs[0] = input();
        wait_for_bead();
        require(enabled->kill_player(0) && disabled->kill_player(0), "Could not eliminate the drip fixture player");
        tick();
        require(!enabled->idle_drip(0)->attached(), "Elimination retained an attached residual bead");
        require(enabled->respawn_player(0) && disabled->respawn_player(0), "Could not respawn the drip fixture player");
        tick();
        require(!enabled->idle_drip(0)->attached(), "Respawn immediately re-used an old residual bead");
        std::cout << "PASS idle drip: real Level contacts, sprayer-only ownership, pause/action/death cancellation and gameplay equivalence\n";
    }

    void check_round_end(const std::filesystem::path& content)
    {
        World world(content);
        auto& look = const_cast<IdleDripPresentation&>(world.presentation.idle_drip());
        look.interval_min = 0.01f;
        look.interval_max = 0.02f;
        look.grow_time = 0.01f;
        look.stretch_time = 0.01f;
        look.lifetime = 100.0f;
        look.gravity = 0.0f;
        auto level = world.build(0.5f);
        const std::vector<PlayerInputData> inputs(input_slots::count, input());
        bool saw_attached = false;
        bool saw_falling = false;
        for (int step = 0; step < 600 && level->state() != LevelState::zoom_out; ++step)
        {
            level->update(inputs, dt);
            saw_attached = saw_attached || level->idle_drip(0)->attached();
            saw_falling = saw_falling || !level->idle_drip(0)->drops().empty();
        }
        require(saw_attached && saw_falling && level->state() == LevelState::zoom_out,
            "Round-end fixture failed to exercise attached and falling paint before time-up");
        require(!level->idle_drip(0)->attached() && level->idle_drip(0)->drops().empty(),
            "Time-up froze residual paint on screen instead of clearing it with the tool");
        for (int step = 0; step < 60; ++step) { level->update(inputs, dt); }
        require(!level->idle_drip(0)->attached() && level->idle_drip(0)->drops().empty(),
            "Round reveal restarted residual paint after its tool disappeared");
        std::cout << "PASS idle drip: time-up clears and reveal cannot restart attached or falling paint\n";
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: idle_drip.exe <content directory>");
        const std::filesystem::path content(argv[1]);
        World world(content);
        check_quiet_intervals(world);
        check_attachment_and_falling(world);
        check_terrain(world);
        check_freeze_disable_and_capacity(world);
        check_level_integration(world, content);
        check_round_end(content);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL idle drip: " << error.what() << '\n';
        return 1;
    }
}
