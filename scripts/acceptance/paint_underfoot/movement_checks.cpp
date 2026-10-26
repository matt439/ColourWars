#include "game/pch.h"
#include "scripts/acceptance/paint_underfoot/fixtures.h"
#include <cstdlib>
#include <iostream>

namespace paint_underfoot_acceptance
{
    namespace
    {
        using namespace mattmath;
        std::vector<std::uint32_t> trace(World& world, int mode)
        {
            if (mode == 0) { world.configure(false); }
            else if (mode == 1)
            { world.tuning = world.variant([](rapidjson::Document& value) { value["player"].RemoveMember("paint_underfoot"); }); }
            else { world.configure(true, 1.0f, 1.0f, 1.0f, 1.0f); }
            std::srand(451u);
            Course course(world);
            auto& surface = course.surface();
            world.paint(surface, surface.shape()->bounding_box(), PlayerTeam::a, 8);
            course.settle();
            std::vector<std::uint32_t> result;
            for (int tick = 0; tick < 180; ++tick)
            {
                const float movement = tick < 30 ? 0.25f : tick < 70 ? 1.0f : tick < 110 ? -0.5f : 0.0f;
                course.tick(input(movement, tick == 80, tick % 3 == 0));
                const Player& player = *course.player;
                for (float value : { player.center().x, player.center().y, player.velocity().x,
                    player.velocity().y, player.health(), player.weapon_ammo(), player.respawn_timer() })
                { result.push_back(bits(value)); }
                result.push_back(static_cast<std::uint32_t>(player.move_state()));
            }
            result.push_back(static_cast<std::uint32_t>(std::rand()));
            return result;
        }
        float speed(World& world, PlayerTeam player_team, PlayerTeam paint_team,
            float direction, float multiplier, int updates)
        {
            world.configure(true);
            Course course(world, player_team);
            auto& surface = course.surface();
            if (paint_team != PlayerTeam::none)
            { world.paint(surface, surface.shape()->bounding_box(), paint_team, paint_team == PlayerTeam::a ? 0 : -1); }
            course.settle();
            course.tick(input(direction));
            require(bits(course.player->velocity().x) == bits(world.tuning.player().x_initial_velocity * multiplier * direction),
                "Ground initial speed did not use the single paint factor in the existing operand order");
            for (int tick = 1; tick < updates; ++tick)
            {
                const float prior = course.player->velocity().x;
                course.tick(input(direction));
                if (tick == 1)
                {
                    const float acceleration = world.tuning.player().x_acceleration * multiplier;
                    const float expected = prior + (acceleration * dt) * direction;
                    require(acceleration > 0.0f && std::fabs(expected) < world.tuning.player().max_velocity.x * multiplier,
                        "Early acceleration control must be positive and strictly below its cap");
                    require(bits(course.player->velocity().x) == bits(expected),
                        "Second uncapped movement step did not scale acceleration in the existing operand order");
                }
            }
            return course.player->velocity().x;
        }
    }
    void check_movement(World& world)
    {
        const auto disabled = trace(world, 0);
        require(trace(world, 1) == disabled && trace(world, 2) == disabled,
            "Missing old block/disabled trial factors/enabled neutral factors changed matched public state or RNG");
        // These are new-build metamorphic controls. Root's frozen old-build trace
        // and all-tool report rows separately establish the required old-vs-off proof.
        for (PlayerTeam player_team : { PlayerTeam::a, PlayerTeam::b })
        {
            const PlayerTeam enemy = player_team == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a;
            for (float direction : { -1.0f, 1.0f })
            {
                const float cap = world.tuning.player().max_velocity.x;
                require(bits(speed(world, player_team, player_team, direction, 1.10f, 150)) == bits(cap * 1.10f * direction),
                    "Friendly paint did not scale the horizontal cap for both teams/directions");
                require(bits(speed(world, player_team, enemy, direction, 0.85f, 150)) == bits(cap * 0.85f * direction),
                    "Enemy paint did not scale the horizontal cap for both teams/directions");
                require(bits(speed(world, player_team, PlayerTeam::none, direction, 1.0f, 150)) == bits(cap * direction),
                    "Neutral ground changed the original cap");
                (void)speed(world, player_team, player_team, direction * 0.25f, 1.10f, 2);
                (void)speed(world, player_team, enemy, direction * 0.25f, 0.85f, 2);
            }
        }
        world.configure(true);
        Course own(world), neutral(world);
        auto& painted = own.surface(); neutral.surface();
        world.paint(painted, painted.shape()->bounding_box(), PlayerTeam::a, 0);
        own.settle(); neutral.settle();
        for (int tick = 0; tick < 150; ++tick) { own.tick(input(1)); neutral.tick(input(1)); }
        const float own_before = own.player->velocity().x, neutral_before = neutral.player->velocity().x;
        own.tick(); neutral.tick();
        require(std::fabs((own_before - own.player->velocity().x) -
            world.tuning.player().x_deacceleration * 1.10f * dt) < 0.0002f &&
            std::fabs((neutral_before - neutral.player->velocity().x) - world.tuning.player().x_deacceleration * dt) < 0.0002f,
            "Release braking did not follow the same factor as speed/acceleration");
        own.tick(input(-0.25f)); neutral.tick(input(-0.25f));
        require(own.player->velocity().x < own_before && neutral.player->velocity().x < neutral_before,
            "Opposite analog input failed to brake");
        // A supported jump can consume that step's context; later airborne steps cannot.
        own.tick(input(0, true)); neutral.tick(input(0, true));
        require(bits(own.player->velocity().y) == bits(neutral.player->velocity().y) &&
            bits(own.player->center().y) == bits(neutral.player->center().y), "Paint changed jump launch/vertical movement");
        require(own.player->paint_underfoot_state().support_team == PlayerTeam::none,
            "Takeoff retained a grounded support snapshot");
        own.tick(); neutral.tick();
        require(own.player->paint_underfoot_state().step_team == PlayerTeam::none &&
            bits(own.player->velocity().y) == bits(neutral.player->velocity().y), "Airborne paint context or vertical boost persisted");
        own.player->place_at({ -world.tuning.player().size.x / 2.0f, -world.tuning.player().size.y });
        own.settle(); world.configure(false);
        require(own.player->paint_underfoot_state() == PlayerPaintUnderfootState{}, "Disabled query exposed stale active context");
        own.tick(input(1));
        require(bits(own.player->velocity().x) == bits(world.tuning.player().x_initial_velocity),
            "Live disabled switch did not immediately stop fresh multipliers from matched rest");
        std::cout << "PASS underfoot movement: disabled/missing/neutral controls, teams/directions/analog, cap/brake and unchanged jump\n";
    }
}
