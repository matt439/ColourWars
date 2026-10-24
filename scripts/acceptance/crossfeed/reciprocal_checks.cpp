#include "game/pch.h"
#include "scripts/acceptance/crossfeed/fixtures.h"
#include <cstdlib>

namespace crossfeed_acceptance
{
    namespace
    {
        float reciprocal_aim(const Services& services, const Player& attacker, const Player& target)
        {
            const bool right = target.center().x >= attacker.center().x;
            const Vector2F offset = services.tuning.weapon(attacker.primary_weapon().type()).details.offset;
            const Vector2F origin = attacker.center() + Vector2F(right ? offset.x : -offset.x, offset.y);
            const Vector2F delta = target.center() - origin;
            return std::atan2(delta.y, delta.x);
        }

        void reciprocal_trial(Services& services, PlayerTeam team, WeaponType type, bool primary_fire, bool opponent_fire)
        {
            const bool right = team == PlayerTeam::a;
            const RectangleF bounds = services.map.info.out_of_bounds_rectangle();
            const auto x = [&](float canonical) { return right ? canonical : bounds.left() + bounds.right() - canonical; };
            const Surface& floor = services.map.named(right ? "floor_paint_a" : "floor_paint_b");
            Run run(services, standing_on(services, floor, x(1050.0f)), team, type);
            Player* opponent = run.add_opponent(standing_on(services, floor, x(1180.0f)), type);
            run.settle(90);
            const int start = run.ticks;
            const Vector2F primary_start = run.player->center(), opponent_start = opponent->center();
            const float starting_health = services.tuning.player().starting_health;
            const float starting_ammo = services.tuning.starting_ammo();
            require(run.player->primary_weapon().ready_to_shoot() && opponent->primary_weapon().ready_to_shoot(),
                "Crossfeed reciprocal positive control did not prepare both real weapons");
            run.death_expected = true;
            run.weapon_aim_after_movement = [&] { return reciprocal_aim(services, *run.player, *opponent); };
            run.set_opponent_weapon_aim(opponent, [&] { return reciprocal_aim(services, *opponent, *run.player); });
            run.set_opponent_input(opponent, [opponent_fire](int)
            {
                PlayerInputData actions = input();
                actions.primary_shoot = opponent_fire;
                return actions;
            });
            std::srand(2401031);
            float primary_damage = 0.0f, opponent_damage = 0.0f;
            int primary_first_damage = -1, opponent_first_damage = -1;
            for (int tick = 1; tick <= 150; ++tick)
            {
                const float primary_before = run.player->health(), opponent_before = opponent->health();
                PlayerInputData actions = input();
                actions.primary_shoot = primary_fire;
                run.tick(actions);
                run.require_clear(*opponent, "reciprocal opponent");
                const float primary_received = std::max(0.0f, primary_before - run.player->health());
                const float opponent_received = std::max(0.0f, opponent_before - opponent->health());
                primary_damage += primary_received;
                opponent_damage += opponent_received;
                if (primary_received > 0.0f && primary_first_damage < 0) { primary_first_damage = tick; }
                if (opponent_received > 0.0f && opponent_first_damage < 0) { opponent_first_damage = tick; }
            }
            const Run::OpponentWeaponStats& stats = run.opponent_weapon_stats(opponent);
            require(approximately(run.player->center().x, primary_start.x) &&
                approximately(opponent->center().x, opponent_start.x),
                "Crossfeed reciprocal stationary control moved horizontally");
            require(primary_fire ? (run.paid_events > 0 && run.emitted_objects + run.launch_retired_objects > 0 && opponent_damage > 0.0f &&
                run.minimum_ammo < starting_ammo) : (run.paid_events == 0 && run.emitted_objects == 0 &&
                    opponent_damage == 0.0f && approximately(run.player->weapon_ammo(), starting_ammo)),
                "Crossfeed reciprocal primary firing/no-fire control failed its paid projectile and damage gate");
            require(opponent_fire ? (stats.paid_events > 0 && stats.emitted_objects + stats.launch_retired_objects > 0 && primary_damage > 0.0f &&
                stats.minimum_ammo < starting_ammo) : (stats.paid_events == 0 && stats.emitted_objects == 0 &&
                    primary_damage == 0.0f && approximately(opponent->weapon_ammo(), starting_ammo)),
                "Crossfeed reciprocal opponent firing/no-fire control failed its paid projectile and damage gate");
            require(opponent_fire || approximately(run.player->health(), starting_health),
                "Crossfeed reciprocal no-opposing-fire control lost primary health");
            require(primary_fire || approximately(opponent->health(), starting_health),
                "Crossfeed reciprocal no-primary-fire control lost opponent health");
            std::cout << "MEASURE Crossfeed reciprocal tool=" << (type == WeaponType::sprayer ? "sprayer" : "sniper") <<
                " team=" << (right ? 'a' : 'b') << " primary_fire=" << primary_fire << " opponent_fire=" << opponent_fire <<
                " active_ticks=150 prepared_ticks=" << start << " primary_paid=" << run.paid_events <<
                " opponent_paid=" << stats.paid_events << " primary_first_paid_tick=" <<
                (run.first_paid_tick < 0 ? -1 : run.first_paid_tick - start) << " opponent_first_paid_tick=" <<
                (stats.first_paid_tick < 0 ? -1 : stats.first_paid_tick - start) <<
                " primary_emitted=" << run.emitted_objects << " opponent_emitted=" << stats.emitted_objects <<
                " primary_launch_retired=" << run.launch_retired_objects << " opponent_launch_retired=" << stats.launch_retired_objects <<
                " primary_damage=" << fixed(primary_damage, 4) << " opponent_damage=" << fixed(opponent_damage, 4) <<
                " primary_first_damage_tick=" << primary_first_damage << " opponent_first_damage_tick=" << opponent_first_damage <<
                " primary_health=" << fixed(run.player->health(), 4) << " opponent_health=" << fixed(opponent->health(), 4) <<
                " primary_min_ammo=" << fixed(run.minimum_ammo, 6) << " opponent_min_ammo=" << fixed(stats.minimum_ammo, 6) <<
                " scope=prepared-stationary-lower-130-current-centre-tracking-no-lead-no-respawn\n";
        }

        void lethal_contact(Services& services, Player& victim)
        {
            const PlayerTeam enemy = victim.team() == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a;
            const ProjectileSpray projectile(RectangleF(0, 0, 10, 10), Vector2F::ZERO, enemy, 1,
                Colour::white, &services.render,
                resolve_projectile_strip(services.tuning.projectile(ProjectileType::spray), services.render),
                &services.tuning.projectile(ProjectileType::spray));
            for (int hit = 0; hit < 1000 && victim.health() > 0.0f; ++hit)
            {
                victim.on_contact(projectile, Vector2F::ZERO, 0.0f);
            }
            require(victim.state() == PlayerState::alive && victim.health() <= 0.0f,
                "Crossfeed lethal-contact control did not establish health-zero before the next Player update");
        }

        void launch_guard_trial(Services& services, PlayerTeam team, bool guard_primary)
        {
            const bool right = team == PlayerTeam::a;
            const RectangleF bounds = services.map.info.out_of_bounds_rectangle();
            const auto x = [&](float canonical) { return right ? canonical : bounds.left() + bounds.right() - canonical; };
            const Surface& floor = services.map.named(right ? "floor_paint_a" : "floor_paint_b");
            Run run(services, standing_on(services, floor, x(1050.0f)), team, WeaponType::sniper);
            Player* opponent = run.add_opponent(standing_on(services, floor, x(1180.0f)), WeaponType::sniper);
            run.settle(90);
            Player* victim = guard_primary ? run.player : opponent;
            require(victim->primary_weapon().ready_to_shoot() && victim->weapon_ammo() >=
                services.tuning.weapon(WeaponType::sniper).details.ammo_usage,
                "Crossfeed launch guard did not prepare an otherwise payable weapon");
            run.death_expected = true;
            run.set_opponent_input(opponent, [guard_primary](int)
            {
                PlayerInputData actions = input();
                actions.primary_shoot = !guard_primary;
                return actions;
            });
            bool injected = false;
            run.after_scene_update = [&]
            {
                if (!injected)
                {
                    // A controlled real projectile-contact callback occupies
                    // the same interval as fatal swept contacts in Level.
                    // It is a launch-guard check, not a damage-rate trial.
                    lethal_contact(services, *victim);
                    injected = true;
                }
            };
            PlayerInputData actions = input();
            actions.primary_shoot = guard_primary;
            const float ammo = victim->weapon_ammo();
            run.tick(actions);
            require(injected && victim->state() == PlayerState::alive && victim->health() <= 0.0f &&
                approximately(victim->weapon_ammo(), ammo) && run.paid_events == 0 &&
                run.opponent_weapon_stats(opponent).paid_events == 0 && run.emitted_objects == 0 &&
                run.opponent_weapon_stats(opponent).emitted_objects == 0,
                "Crossfeed health-zero player paid or launched before its dead-state update");
            run.after_scene_update = {};
            run.tick(actions);
            require(victim->state() == PlayerState::dead && approximately(victim->weapon_ammo(), ammo) &&
                run.paid_events == 0 && run.opponent_weapon_stats(opponent).paid_events == 0 &&
                run.emitted_objects == 0 && run.opponent_weapon_stats(opponent).emitted_objects == 0,
                "Crossfeed dead-state player paid or launched under held fire");
            std::cout << "MEASURE Crossfeed reciprocal_launch_guard team=" << (right ? 'a' : 'b') <<
                " guarded_player=" << (guard_primary ? "primary" : "opponent") <<
                " health_zero_before_launch=1 dead_next_tick=1 paid_events=0 emitted_objects=0" <<
                " scope=controlled-projectile-contact-between-update-and-weapon-phase\n";
        }
    }

    void reciprocal_checks(Services& services)
    {
        require(150.0f * dt < services.tuning.player().respawn_delay,
            "Crossfeed reciprocal damage checks must end before a lethal player can respawn");
        std::cout << "NOTE Crossfeed reciprocal fixture qualification: mirrored stationary lower-130 Sprayer/Sniper, "
            "four primary/opponent fire combinations, both real weapon phases, ammunition and health. "
            "Separate controlled fatal contacts check zero-health and dead launch suppression for each player order. "
            "No human aim, route safety or competitive balance is established\n";
        for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
        {
            for (const WeaponType type : { WeaponType::sprayer, WeaponType::sniper })
            {
                for (const bool primary_fire : { false, true })
                {
                    for (const bool opponent_fire : { false, true })
                    {
                        reciprocal_trial(services, team, type, primary_fire, opponent_fire);
                    }
                }
            }
            launch_guard_trial(services, team, true);
            launch_guard_trial(services, team, false);
        }
        std::cout << "PASS Crossfeed reciprocal fixture: sixteen mirrored paid-fire/no-fire controls and four "
            "health-zero/dead launch guards for both player orders\n";
    }
}
