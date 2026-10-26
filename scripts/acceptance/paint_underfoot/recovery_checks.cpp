#include "game/pch.h"
#include "scripts/acceptance/paint_underfoot/fixtures.h"
#include <cstdlib>
#include <iostream>

namespace paint_underfoot_acceptance
{
    namespace
    {
        using namespace mattmath;
        constexpr WeaponType tools[]{ WeaponType::sprayer, WeaponType::sniper, WeaponType::roller,
            WeaponType::mister, WeaponType::bucket };
        auto update(Weapon& weapon, bool fire, float factor, bool default_argument = false, float elapsed = dt)
        {
            if (default_argument)
            { return weapon.update_and_get_projectiles(input(0, false, fire), Vector2F::ZERO, Vector2F::ZERO, true, elapsed); }
            return weapon.update_and_get_projectiles(input(0, false, fire), Vector2F::ZERO, Vector2F::ZERO, true, elapsed, factor);
        }
        void deplete(Weapon& weapon)
        {
            std::size_t paid = 0;
            for (int tick = 0; tick < 2000; ++tick)
            {
                paid += update(weapon, true, 0.0f).size();
                if (paid > 0 && weapon.readiness() == WeaponReadiness::empty) { return; }
            }
            throw std::runtime_error("Actual tool did not reach paid full-shot depletion");
        }
        std::vector<std::uint32_t> direct_trace(World& world, WeaponType tool, float factor, bool default_argument)
        {
            std::srand(741u);
            auto weapon = world.weapon(tool);
            std::vector<std::uint32_t> result;
            for (int tick = 0; tick < 240; ++tick)
            {
                const auto shots = update(*weapon, tick < 100 || tick > 190, factor, default_argument);
                result.push_back(static_cast<std::uint32_t>(shots.size()));
                result.push_back(bits(weapon->ammo()));
                result.push_back(static_cast<std::uint32_t>(weapon->readiness()));
                for (const auto& shot : shots) { result.push_back(static_cast<std::uint32_t>(shot->projectile_type())); }
            }
            result.push_back(static_cast<std::uint32_t>(std::rand()));
            return result;
        }
        void deplete(Course& course, StructurePaintable& floor)
        {
            for (int tick = 0; tick < 2000; ++tick)
            {
                course.begin(input(0, false, true));
                // A real roller can paint during launch obstruction before its
                // returned projectile is retired. Restore the controlled enemy
                // top with a real later paint callback, never a Player hook.
                course.world.paint(floor, floor.shape()->bounding_box(), PlayerTeam::b, -1);
                course.scene.resolve(); course.finish();
                if (course.player->weapon_ammo() < course.world.tuning.starting_ammo() &&
                    course.player->primary_weapon().readiness() == WeaponReadiness::empty) { return; }
            }
            throw std::runtime_error("Player's actual tool did not deplete on enemy paint");
        }
    }
    void check_recovery(World& world)
    {
        world.configure(true);
        for (WeaponType tool : tools)
        {
            require(direct_trace(world, tool, 1.0f, true) == direct_trace(world, tool, 1.0f, false),
                "Standalone default recovery changed paid-shot/ammo/readiness/type/RNG state");
            auto enemy = world.weapon(tool), neutral = world.weapon(tool), own = world.weapon(tool);
            deplete(*enemy); deplete(*neutral); deplete(*own);
            require(bits(enemy->ammo()) == bits(neutral->ammo()) && bits(own->ammo()) == bits(neutral->ammo()),
                "Matched actual-tool depletion did not establish identical reserves");
            const float spent = enemy->ammo();
            const int delay_ticks = static_cast<int>(std::ceil(world.tuning.ammo_regen_delay() / dt)) + 3;
            for (int tick = 0; tick < delay_ticks; ++tick)
            { require(update(*enemy, false, 0.0f).empty() && update(*neutral, false, 0.0f).empty() &&
                update(*own, false, 0.0f).empty(), "Released tool fired during recovery delay"); }
            require(bits(enemy->ammo()) == bits(spent), "Enemy paint recovered ammo or altered its empty reserve");
            update(*enemy, false, 0.0f); update(*neutral, false, 1.0f); update(*own, false, 1.25f);
            require(bits(enemy->ammo()) == bits(spent) && neutral->ammo() > spent && own->ammo() > neutral->ammo(),
                "Enemy recovery suppression, normal recovery or own recovery boost failed for an actual tool");
            require(bits(neutral->ammo()) == bits(spent + world.tuning.ammo_regen_rate() * dt),
                "Neutral recovery lost the original exact rate*dt expression");
            for (int tick = 0; tick < 600; ++tick) { update(*own, false, 1.25f); }
            require(bits(own->ammo()) == bits(world.tuning.starting_ammo()), "Boosted recovery escaped the full-tank clamp");
            require(update(*enemy, true, 0.0f).empty() && enemy->readiness() == WeaponReadiness::empty,
                "Held empty enemy-paint request bypassed the full-shot gate");
            // The zero factor kept delay/cooldown clocks running: leaving enemy
            // paint recovers immediately instead of restarting the delay.
            update(*enemy, false, 1.0f);
            require(enemy->ammo() > spent, "Zero recovery factor froze/restarted the ordinary delay timer");
            auto first = world.weapon(tool), second = world.weapon(tool);
            // Before regen delay expires, changing only recovery has no effect
            // on paid shot ticks, interval, cost or projectile kinds/counts.
            const int window = std::max(1, static_cast<int>(world.tuning.ammo_regen_delay() / dt) - 1);
            for (int tick = 0; tick < window; ++tick)
            {
                const auto a = update(*first, true, 0.0f), b = update(*second, true, 1.25f);
                require(a.size() == b.size() && bits(first->ammo()) == bits(second->ammo()),
                    "Recovery factor altered pre-delay shot timing/cost instead of only regeneration");
                for (std::size_t index = 0; index < a.size(); ++index)
                { require(a[index]->projectile_type() == b[index]->projectile_type(), "Recovery context changed emitted projectile kind"); }
            }
            Course player(world, PlayerTeam::a, tool);
            auto& floor = player.surface();
            world.paint(floor, floor.shape()->bounding_box(), PlayerTeam::b, -1);
            player.settle(); deplete(player, floor);
            for (int tick = 0; tick < delay_ticks; ++tick) { player.tick(); }
            const float player_spent = player.player->weapon_ammo();
            player.tick(input(0, false, true));
            require(bits(player.player->weapon_ammo()) == bits(player_spent), "Player forwarding granted enemy-paint recovery");
            world.paint(floor, floor.shape()->bounding_box(), PlayerTeam::a, 8);
            player.tick();
            require(bits(player.player->weapon_ammo()) == bits(player_spent), "Player recovery used unfinished repaint contact context");
            player.tick();
            require(bits(player.player->weapon_ammo()) == bits(player_spent +
                (world.tuning.ammo_regen_rate() * world.tuning.player().paint_underfoot.own_ammo_regen_multiplier) * dt),
                "Player did not forward the same frozen own class to actual weapon recovery");
            const auto before_frozen = player.player->paint_underfoot_state();
            const auto before_ammo = bits(player.player->weapon_ammo());
            const auto before_x = bits(player.player->center().x);
            player.scene.update(0.0f);
            require(player.player->paint_underfoot_state() == before_frozen && bits(player.player->weapon_ammo()) == before_ammo &&
                bits(player.player->center().x) == before_x, "Frozen Scene update refreshed or advanced terrain gameplay context");
            player.player->set_infinite_ammo(true);
            bool paid_action = false;
            for (int tick = 0; tick < 100; ++tick)
            {
                player.tick(input(0, false, true));
                paid_action |= player.player->animation_state() == PlayerAnimationState::firing;
                require(bits(player.player->weapon_ammo()) == bits(world.tuning.starting_ammo()),
                    "Prototype recovery context changed explicit infinite-ammo behavior");
            }
            require(paid_action, "Infinite-ammo control reached no actual paid firing action");
            require(bits(player.player->health()) == bits(world.tuning.player().starting_health),
                "Enemy paint harmed health");
        }
        std::cout << "PASS underfoot recovery: all5 actual tools, paid depletion/delay/shot cost, clamp and shared Player context\n";
    }
}
