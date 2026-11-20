#include "game/pch.h"
#include "scripts/acceptance/umbrella/fixtures.h"

namespace umbrella_acceptance
{
    void check_match(World& world)
    {
        const auto settings = roster();
        auto level = world.build(settings);
        auto inputs = physical_inputs();
        level->start_without_countdown();
        const auto step = [&] { level->update(inputs, dt); };
        for (int tick = 0; tick < 240 && level->state() != LevelState::active; ++tick) { step(); }
        require(level->state() == LevelState::active, "Umbrella fixture did not enter live play");
        const auto& holder = level->player(0);
        const auto& roller = level->player(1);
        require(holder.primary_weapon().type() == WeaponType::umbrella &&
            !holder.canopy() && !roller.canopy(), "Level did not equip the chosen tools with closed canopies");
        require(level->place_player(0, { 285, 1000 }), "Could not place holder on Crossfeed's dock");
        for (int i = 0; i < 30; ++i) { step(); }
        const float initial_health = holder.health();
        const int full = world.assets.tuning.weapon(WeaponType::umbrella).canopy.hits;
        const auto fire = [&](Vector2F at, Vector2F velocity, PlayerTeam team)
        { level->admit_projectile(straight_shot(world, at, velocity, team)); };
        const auto hits = [&] { return holder.primary_weapon().canopy_state().hits_left; };
        // Shoot and the enemy's incoming shot arrive in the same live step.
        inputs[0] = hold();
        const auto centre = holder.center();
        fire(centre + Vector2F(84, 0), { -4800, 0 }, PlayerTeam::b);
        step();
        require(holder.canopy() && hits() == full - 1 && holder.health() == initial_health,
            "A same-step held canopy failed to stop the incoming shot before its owner");
        fire(centre + Vector2F(84, 0), { -4800, 0 }, PlayerTeam::a);
        step();
        require(hits() == full - 1 && holder.health() == initial_health, "Allied travel charged a canopy hit");
        fire(centre - Vector2F(38, 0), { 4800, 0 }, PlayerTeam::b);
        step();
        require(hits() == full - 1 && holder.health() < initial_health, "A shot from behind failed to hit the holder");

        require(level->place_player(1, { holder.rectangle().left() + 145, holder.rectangle().top() }),
            "Cannot place the opposing roller on the dock");
        inputs[1].x_movement = -1;
        bool crossed = false;
        for (int tick = 0; tick < 80 && !crossed; ++tick)
        {
            step();
            require(holder.canopy().has_value() && roller.state() == PlayerState::alive,
                "Walk-through witness lost its open canopy or opposing player");
            crossed = roller.rectangle().right() < holder.canopy()->bounds.left();
        }
        require(crossed, "An opposing player could not walk through the open canopy");

        require(level->place_player(1, { holder.rectangle().left() + 38, holder.rectangle().top() }),
            "Cannot position the opposing roller to make contact");
        inputs[1] = hold(3.14f);
        inputs[1].x_movement = -1;
        const float before_roller = holder.health();
        for (int i = 0; i < 45 && holder.health() == before_roller; ++i) { step(); }
        require(holder.health() < before_roller && hits() == full - 1, "A live roller failed to hurt the guarded player");
        require(level->kill_player(0), "Cannot eliminate the canopy holder");
        step();
        require(holder.state() == PlayerState::dead && !holder.canopy(), "An eliminated player's canopy stayed live");
        require(level->respawn_player(0) && !holder.canopy() && hits() == full,
            "Respawn did not clear and repair the canopy");

        // Compare held and released versions of the same opening run using the same
        // roster, spawn and seed. Drawing/opening the canopy changes no speed.
        const auto distance = [&](bool held)
        {
            level.reset();
            level = world.build(settings);
            inputs = physical_inputs();
            level->start_without_countdown();
            for (int tick = 0; tick < 240 && level->state() != LevelState::active; ++tick) { step(); }
            inputs[0] = hold();
            inputs[0].primary_shoot = held;
            inputs[0].x_movement = 1;
            for (int i = 0; i < 32; ++i) { step(); }
            require(level->player(0).canopy().has_value() == held, "Invalid speed comparison");
            return level->player(0).center();
        };
        const auto open_run = distance(true);
        require(open_run == distance(false), "Holding the canopy changed the player's movement");

        // A spent canopy cannot absorb a second shot later in the same tick.
        const Player& fresh = level->player(0);
        inputs[0] = hold();
        step();
        const auto shield = fresh.canopy()->bounds;
        const auto hp = fresh.health();
        for (int i = 0; i <= full; ++i)
        {
            fire({ shield.right() + 4.0f, shield.center().y }, { -4800, 0 }, PlayerTeam::b);
        }
        step();
        require(!fresh.canopy() && fresh.health() < hp && fresh.primary_weapon().canopy_state().hits_left == 0,
            "After the breaking hit, later shots in the same sweep batch must reach the player");

        // A short round with only the Umbrella firing proves its actual
        // pellets enter the scene, paint the dock and credit the holder.
        level.reset();
        auto brief = settings;
        brief.set_duration(2.0f);
        level = world.build(brief);
        inputs = physical_inputs();
        inputs[0] = hold(1.25f);
        level->start_without_countdown();
        for (int tick = 0; tick < 240 && level->state() != LevelState::active; ++tick) { step(); }
        require(level->state() == LevelState::active, "The scoring round did not start");
        for (int tick = 0; tick < 240 && level->state() == LevelState::active; ++tick) { step(); }
        const auto result = level->level_end_info();
        require(level->state() != LevelState::active && result.team_a_area > 0 &&
            result.team_b_area == 0 && result.laid_areas[0] >= result.team_a_area &&
            result.player_areas[0] == result.team_a_area, "Umbrella pellets did not score for their owner in a full round");
    }
}
