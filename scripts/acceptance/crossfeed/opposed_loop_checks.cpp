#include "game/pch.h"
#include "scripts/acceptance/crossfeed/fixtures.h"
#include <cstdlib>

namespace crossfeed_acceptance
{
    namespace
    {
        constexpr int active_ticks = 600;
        constexpr int tail_limit = 720;

        const char* tool_name(WeaponType type)
        {
            switch (type)
            {
            case WeaponType::sprayer: return "sprayer";
            case WeaponType::sniper: return "sniper";
            case WeaponType::roller: return "roller";
            case WeaponType::mister: return "mister";
            case WeaponType::bucket: return "bucket";
            default: throw std::runtime_error("Crossfeed opposed loops need a concrete primary");
            }
        }

        int flying_objects(const Run& run)
        {
            int count = 0;
            for (const CollisionObject* object : run.scene.collision_objects())
            {
                count += is_projectile(to_collision_type(object->tag())) && !object->for_deletion() ? 1 : 0;
            }
            return count;
        }

        float steer(const Player& player, float goal, const PlayerTuning& tuning)
        {
            const float error = goal - player.center().x;
            const float velocity = player.velocity().x;
            const float stopping = velocity * velocity / (2.0f * tuning.x_deacceleration) +
                std::fabs(velocity) * dt + 4.0f;
            if (std::fabs(error) < 5.0f || (velocity != 0.0f &&
                (velocity > 0.0f) == (error > 0.0f) && std::fabs(error) <= stopping)) { return 0.0f; }
            return error > 0.0f ? 1.0f : -1.0f;
        }

        struct HealthObservation
        {
            float net_tick_loss = 0.0f;
            float minimum = 0.0f;
            int first_loss_tick = -1;
            int lethal_events = 0;
            int respawns = 0;
            int inactive_ticks = 0;

            void observe(const Player& player, float before, PlayerState previous, int tick)
            {
                const float loss = std::max(0.0f, before - player.health());
                this->net_tick_loss += loss;
                this->minimum = std::min(this->minimum, player.health());
                if (loss > 0.0f && this->first_loss_tick < 0) { this->first_loss_tick = tick; }
                this->lethal_events += before > 0.0f && player.health() <= 0.0f ? 1 : 0;
                this->respawns += previous == PlayerState::dead && player.state() == PlayerState::alive ? 1 : 0;
                this->inactive_ticks += player.state() != PlayerState::alive || player.health() <= 0.0f ? 1 : 0;
            }
        };

        struct Result
        {
            std::vector<std::array<float, 6>> trajectory;
            HealthObservation subject;
            HealthObservation opponent;
            int paid = 0;
            int opponent_paid = 0;
            int turns = 0;
            int lower_landing = -1;
            int home_exit = -1;
            int plinth_visit = -1;
        };

        Result trial(Services& services, WeaponType type, PlayerTeam team,
            bool advancing, bool fire, bool opponent_fire)
        {
            const bool right = team == PlayerTeam::a;
            const PlayerTeam enemy = right ? PlayerTeam::b : PlayerTeam::a;
            const RectangleF bounds = services.map.info.out_of_bounds_rectangle();
            const auto x = [&](float canonical) { return right ? canonical : bounds.left() + bounds.right() - canonical; };
            const auto spawns = right ? services.map.info.team_a_spawns() : services.map.info.team_b_spawns();
            const Surface& home = services.map.named(right ? "spawn_a_floor" : "spawn_b_floor");
            const Surface& floor = services.map.named(right ? "floor_paint_a" : "floor_paint_b");
            const Surface& plinth = services.map.named("centre_plinth");
            Run run(services, spawns.at(0), team, type);
            Player* opponent = run.add_opponent(standing_on(services, floor, x(1150.0f)), WeaponType::sprayer);
            require(run.ticks == 0 && run.support() == home.name &&
                !run.player->primary_weapon().ready_to_shoot() && !opponent->primary_weapon().ready_to_shoot() &&
                approximately(run.player->weapon_ammo(), services.tuning.starting_ammo()) &&
                approximately(opponent->weapon_ammo(), services.tuning.starting_ammo()),
                "Crossfeed opposed loops did not start with cold weapons at their named positions");
            run.death_expected = true;
            run.set_opponent_input(opponent, [opponent_fire](int)
            {
                PlayerInputData actions = input();
                actions.primary_shoot = opponent_fire;
                return actions;
            });
            run.set_opponent_weapon_aim(opponent, [&]
            {
                const Vector2F offset = services.tuning.weapon(WeaponType::sprayer).details.offset;
                const bool facing_right = run.player->center().x >= opponent->center().x;
                const Vector2F origin = opponent->center() + Vector2F(facing_right ? offset.x : -offset.x, offset.y);
                const Vector2F to_target = run.player->center() - origin;
                return std::atan2(to_target.y, to_target.x);
            });
            std::srand(2401027);
            Result result;
            result.subject.minimum = run.player->health();
            result.opponent.minimum = opponent->health();
            const float total = run.total_area();
            bool dropping = true;
            bool far_goal = false;
            float previous_own = 0.0f;
            float previous_enemy = 0.0f;
            const auto check_opponent = [&]
            {
                run.require_clear(*opponent, "opposed loop opponent");
                require(approximately(opponent->center().x, x(1150.0f)) &&
                    approximately(opponent->rectangle().bottom(), floor.rectangle.top()) &&
                    (opponent->state() != PlayerState::alive || opponent->health() <= 0.0f || supported(*opponent)),
                    "Crossfeed stationary pressure opponent left its named floor station");
            };
            const auto score = [&]
            {
                const float own = run.team_area(team), other = run.team_area(enemy);
                const float neutral = run.total_area() - own - other;
                require(approximately(run.total_area(), total) && own >= 0.0f && other >= 0.0f && neutral >= -tolerance,
                    "Crossfeed opposed loop exceeded its conserved scored surface budget");
                require(fire || own == 0.0f, "Crossfeed no-fire subject gained team paint");
                require(opponent_fire || other == 0.0f, "Crossfeed no-fire opponent gained team paint");
            };
            for (int tick = 1; tick <= active_ticks; ++tick)
            {
                const float goal = x(advancing ? (far_goal ? 2450.0f : 1150.0f) : (far_goal ? 500.0f : 120.0f));
                const float direction = goal >= run.player->center().x ? 1.0f : -1.0f;
                PlayerInputData actions = aimed(direction > 0.0f ? PI / 3.0f : 2.0f * PI / 3.0f,
                    input(dropping ? 0.0f : steer(*run.player, goal, services.tuning.player()), false, false, dropping));
                actions.primary_shoot = fire;
                const float before = run.player->health(), opponent_before = opponent->health();
                const PlayerState state = run.player->state(), opponent_state = opponent->state();
                run.tick(actions);
                check_opponent();
                result.subject.observe(*run.player, before, state, tick);
                result.opponent.observe(*opponent, opponent_before, opponent_state, tick);
                if (state == PlayerState::dead && run.player->state() == PlayerState::alive)
                {
                    // Respawn is production-owned. Restart departure on the next
                    // input tick; neither the clock nor retained paint is reset.
                    dropping = true;
                    far_goal = false;
                }
                const RectangleF body = run.player->rectangle();
                const bool alive = run.player->state() == PlayerState::alive && run.player->health() > 0.0f;
                result.trajectory.push_back({ actions.x_movement, actions.drop_through ? 1.0f : 0.0f,
                    run.player->center().x, run.player->center().y, run.player->velocity().x, run.player->velocity().y });
                require(advancing || (body.left() >= home.rectangle.left() - tolerance &&
                    body.right() <= home.rectangle.right() + tolerance), "Crossfeed opposed home loop left its footprint");
                if (alive && dropping && supported(*run.player) && approximately(body.bottom(), floor.rectangle.top()))
                {
                    dropping = false;
                    if (result.lower_landing < 0) { result.lower_landing = tick; }
                }
                if (alive && result.home_exit < 0 &&
                    (right ? body.left() >= home.rectangle.right() : body.right() <= home.rectangle.left()))
                { result.home_exit = tick; }
                if (alive && result.plinth_visit < 0 && supported(*run.player) &&
                    approximately(body.bottom(), plinth.rectangle.top()) &&
                    body.right() > plinth.rectangle.left() && body.left() < plinth.rectangle.right())
                { result.plinth_visit = tick; }
                if (alive && !dropping && std::fabs(run.player->center().x - goal) <= 8.0f &&
                    run.player->velocity().x == 0.0f && supported(*run.player))
                {
                    ++result.turns;
                    far_goal = !far_goal;
                }
                score();
                if (tick == 180 || tick == 360 || tick == active_ticks)
                {
                    const float own = run.team_area(team), other = run.team_area(enemy);
                    std::cout << "MEASURE Crossfeed opposed_loop tool=" << tool_name(type) <<
                        " team=" << (right ? 'a' : 'b') << " route=" << (advancing ? "advancing" : "home") <<
                        " fire=" << fire << " opponent_fire=" << opponent_fire << " checkpoint_tick=" << tick <<
                        " owned_area=" << fixed(own) << " enemy_area=" << fixed(other) << " neutral_area=" << fixed(total - own - other) <<
                        " net_owned_change=" << fixed(own - previous_own) << " net_enemy_change=" << fixed(other - previous_enemy) <<
                        " score_difference=" << fixed(own - other) <<
                        " paid_events=" << run.paid_events << " opponent_paid_events=" << run.opponent_weapon_stats(opponent).paid_events <<
                        " reserve=" << fixed(run.player->weapon_ammo(), 6) << " opponent_reserve=" << fixed(opponent->weapon_ammo(), 6) <<
                        " health=" << fixed(run.player->health(), 4) << " opponent_health=" << fixed(opponent->health(), 4) <<
                        " observed_health_loss=" << fixed(result.subject.net_tick_loss, 4) <<
                        " opponent_observed_health_loss=" << fixed(result.opponent.net_tick_loss, 4) <<
                        " first_damage_tick=" << result.subject.first_loss_tick << " opponent_first_damage_tick=" << result.opponent.first_loss_tick <<
                        " lethal_events=" << result.subject.lethal_events << " opponent_lethal_events=" << result.opponent.lethal_events <<
                        " respawns=" << result.subject.respawns << " opponent_respawns=" << result.opponent.respawns <<
                        " inactive_ticks=" << result.subject.inactive_ticks << " opponent_inactive_ticks=" << result.opponent.inactive_ticks <<
                        " lower_landing_tick=" << result.lower_landing << " full_body_home_exit_tick=" << result.home_exit <<
                        " first_supported_plinth_tick=" << result.plinth_visit << " stopped_endpoint_count=" << result.turns <<
                        " x=" << fixed(run.player->center().x) << " y=" << fixed(run.player->center().y) <<
                        " flying_objects=" << flying_objects(run) << " scope=stationary-lower-approach-sprayer-current-centre-no-lead\n";
                    previous_own = own;
                    previous_enemy = other;
                }
            }
            result.paid = run.paid_events;
            result.opponent_paid = run.opponent_weapon_stats(opponent).paid_events;
            require((fire ? result.paid > 0 : result.paid == 0) &&
                (opponent_fire ? result.opponent_paid > 0 : result.opponent_paid == 0),
                "Crossfeed opposed-loop payment did not follow each player's fire control");
            require(opponent_fire || (result.subject.net_tick_loss == 0.0f && result.subject.lethal_events == 0),
                "Crossfeed subject took damage without opposing fire");
            require(fire || (result.opponent.net_tick_loss == 0.0f && result.opponent.lethal_events == 0),
                "Crossfeed opponent took damage without subject fire");
            require(result.lower_landing > 0 && (opponent_fire || (result.turns >= 3 &&
                (!advancing || (result.home_exit > 0 && result.plinth_visit > 0)))),
                "Crossfeed opposed loop missed its required unopposed route control");
            const float active_own = run.team_area(team), active_enemy = run.team_area(enemy);
            run.set_opponent_input(opponent, {});
            int tail = 0;
            while (flying_objects(run) > 0 && tail < tail_limit)
            {
                run.tick(input());
                check_opponent();
                ++tail;
                score();
                require(run.paid_events == result.paid && run.opponent_weapon_stats(opponent).paid_events == result.opponent_paid,
                    "Crossfeed opposed-loop tail paid another shot");
            }
            require(flying_objects(run) == 0, "Crossfeed opposed-loop flight tail exceeded its finite limit");
            std::cout << "MEASURE Crossfeed opposed_loop_settled tool=" << tool_name(type) << " team=" << (right ? 'a' : 'b') <<
                " route=" << (advancing ? "advancing" : "home") << " fire=" << fire << " opponent_fire=" << opponent_fire <<
                " tail_ticks=" << tail << " active_owned_area=" << fixed(active_own) << " active_enemy_area=" << fixed(active_enemy) <<
                " settled_owned_area=" << fixed(run.team_area(team)) << " settled_enemy_area=" << fixed(run.team_area(enemy)) <<
                " scope=release-both-players-tail-excluded-from-active-results\n";
            return result;
        }
    }

    void opposed_loop_checks(Services& services)
    {
        std::cout << "NOTE Crossfeed opposed loops: same 600-tick cold-spawn-zero home/advancing endpoints and downward aim as "
            "home_loop. A cold stationary Sprayer occupies our lower approach at canonical x=1150 (mirror 2450), "
            "tracking current centre after movement without lead or gravity compensation. Primary launches first, as in Level. "
            "All four subject/opponent fire controls are sampled. Respawn restarts the subject's drop/first endpoint next tick "
            "without resetting clock or paint; opponent respawns at its constructed station. Observed health loss sums net "
            "tick decreases and may undercount damage concurrent with regeneration; it is separate from final health. "
            "The shot-free tail releases both players and is excluded from active results. No safety or balance ranking follows\n";
        int trials = 0;
        int advancing_damage_samples = 0;
        for (const WeaponType type : { WeaponType::sprayer, WeaponType::sniper, WeaponType::roller, WeaponType::mister, WeaponType::bucket })
        {
            for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
            {
                for (const bool advancing : { false, true })
                {
                    const Result neither = trial(services, type, team, advancing, false, false);
                    const Result subject = trial(services, type, team, advancing, true, false);
                    require(neither.trajectory == subject.trajectory && neither.turns == subject.turns &&
                        neither.lower_landing == subject.lower_landing && neither.home_exit == subject.home_exit &&
                        neither.plinth_visit == subject.plinth_visit,
                        "Crossfeed inactive-opponent controls changed subject movement under paid fire");
                    const Result opponent = trial(services, type, team, advancing, false, true);
                    const Result reciprocal = trial(services, type, team, advancing, true, true);
                    if (advancing)
                    {
                        advancing_damage_samples += opponent.subject.net_tick_loss > 0.0f ? 1 : 0;
                        advancing_damage_samples += reciprocal.subject.net_tick_loss > 0.0f ? 1 : 0;
                    }
                    trials += 4;
                }
            }
        }
        require(trials == 80, "Crossfeed opposed-loop matrix changed its bounded trial count");
        require(advancing_damage_samples > 0,
            "Crossfeed lower-approach pressure policy never caused observed health loss in an advancing sample");
        std::cout << "PASS Crossfeed opposed loops: eighty mirrored five-tool home/advancing trials, four independent "
            "fire controls, both-player payment/health/respawn observations and bounded released-flight tails\n";
    }
}
