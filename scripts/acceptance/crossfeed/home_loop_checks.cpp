#include "game/pch.h"
#include "scripts/acceptance/crossfeed/fixtures.h"
#include <cstdlib>

namespace crossfeed_acceptance
{
    namespace
    {
        constexpr int active_ticks = 600;
        constexpr int flight_tail_limit = 720;

        enum class LoopRoute { home, advancing };

        const char* loop_name(LoopRoute route)
        {
            return route == LoopRoute::home ? "lower-home-loop-120-500" : "advancing-lower-loop-1150-2450";
        }

        const char* loop_tool_name(WeaponType type)
        {
            switch (type)
            {
            case WeaponType::sprayer: return "sprayer";
            case WeaponType::sniper: return "sniper";
            case WeaponType::roller: return "roller";
            case WeaponType::mister: return "mister";
            case WeaponType::bucket: return "bucket";
            default: throw std::runtime_error("Crossfeed home loops need a concrete primary");
            }
        }

        int flying_objects(const Run& run)
        {
            int count = 0;
            for (const CollisionObject* object : run.scene.collision_objects())
            {
                if (is_projectile(to_collision_type(object->tag())) && !object->for_deletion()) { ++count; }
            }
            return count;
        }

        float loop_steer(const Player& player, float goal, const PlayerTuning& tuning)
        {
            const float error = goal - player.center().x;
            const float velocity = player.velocity().x;
            const float stopping = velocity * velocity / (2.0f * tuning.x_deacceleration) +
                std::fabs(velocity) * dt + 4.0f;
            if (std::fabs(error) < 5.0f || (velocity != 0.0f &&
                (velocity > 0.0f) == (error > 0.0f) && std::fabs(error) <= stopping)) { return 0.0f; }
            return error > 0.0f ? 1.0f : -1.0f;
        }

        struct LoopResult
        {
            std::vector<std::array<float, 6>> trajectory;
            int turns = 0;
            int lower_landing_tick = -1;
            int home_exit_tick = -1;
            int plinth_visit_tick = -1;
            int paid = 0;
            float active_owned = 0.0f;
            float settled_owned = 0.0f;
        };

        LoopResult loop_trial(Services& services, WeaponType type, PlayerTeam team, LoopRoute route, bool fire)
        {
            const bool right = team == PlayerTeam::a;
            const PlayerTeam enemy = right ? PlayerTeam::b : PlayerTeam::a;
            const RectangleF bounds = services.map.info.out_of_bounds_rectangle();
            const auto x = [&](float canonical) { return right ? canonical : bounds.left() + bounds.right() - canonical; };
            const auto points = right ? services.map.info.team_a_spawns() : services.map.info.team_b_spawns();
            require(points.size() == 4, "Crossfeed home loops need four authored spawns per team");
            const Surface& home = services.map.named(right ? "spawn_a_floor" : "spawn_b_floor");
            const Surface& floor = services.map.named(right ? "floor_paint_a" : "floor_paint_b");
            const Surface& plinth = services.map.named("centre_plinth");
            Run run(services, points[0], team, type);
            require(run.ticks == 0 && run.paid_events == 0 && run.team_area(team) == 0.0f &&
                run.team_area(enemy) == 0.0f && run.player->rectangle() == RectangleF(points[0], services.tuning.player().size) &&
                run.support() == home.name && !run.player->primary_weapon().ready_to_shoot() &&
                approximately(run.player->weapon_ammo(), services.tuning.starting_ammo()),
                "Crossfeed home-loop comparison did not start cold at its complete authored spawn zero");
            std::srand(2401027);
            const float total = run.total_area();
            require(total > 0.0f, "Crossfeed home-loop comparison has no scored area");
            LoopResult result;
            result.trajectory.reserve(active_ticks);
            bool dropping = true;
            bool far_goal = false;
            float previous_checkpoint_owned = 0.0f;
            int previous_checkpoint_tick = 0;
            const auto owned = [&]
            {
                const float own_area = run.team_area(team);
                const float enemy_area = run.team_area(enemy);
                const float neutral = run.total_area() - own_area - enemy_area;
                require(approximately(run.total_area(), total) && own_area >= -tolerance &&
                    enemy_area == 0.0f && neutral >= -tolerance && approximately(total - neutral, own_area),
                    "Crossfeed home-loop score did not conserve neutral and owned area");
                return own_area;
            };

            for (int tick = 1; tick <= active_ticks; ++tick)
            {
                const float canonical_goal = route == LoopRoute::home ? (far_goal ? 500.0f : 120.0f) :
                    (far_goal ? 2450.0f : 1150.0f);
                const float goal = x(canonical_goal);
                const float direction = goal >= run.player->center().x ? 1.0f : -1.0f;
                PlayerInputData actions = aimed(direction > 0.0f ? PI / 3.0f : 2.0f * PI / 3.0f,
                    input(dropping ? 0.0f : loop_steer(*run.player, goal, services.tuning.player()),
                        false, false, dropping));
                actions.primary_shoot = fire;
                run.tick(actions);
                const RectangleF body = run.player->rectangle();
                if (route == LoopRoute::home)
                {
                    require(body.left() >= home.rectangle.left() - tolerance &&
                        body.right() <= home.rectangle.right() + tolerance,
                        "Crossfeed home-loop policy left its full-body home footprint");
                }
                result.trajectory.push_back({ actions.x_movement, actions.drop_through ? 1.0f : 0.0f,
                    run.player->center().x, run.player->center().y, run.player->velocity().x, run.player->velocity().y });
                const float current_owned = owned();
                if (dropping && supported(*run.player) && approximately(body.bottom(), floor.rectangle.top()))
                {
                    dropping = false;
                    result.lower_landing_tick = tick;
                }
                if (result.home_exit_tick < 0 &&
                    (right ? body.left() >= home.rectangle.right() : body.right() <= home.rectangle.left()))
                { result.home_exit_tick = tick; }
                if (result.plinth_visit_tick < 0 && supported(*run.player) &&
                    approximately(body.bottom(), plinth.rectangle.top()) &&
                    body.right() > plinth.rectangle.left() && body.left() < plinth.rectangle.right())
                { result.plinth_visit_tick = tick; }
                if (!dropping && std::fabs(run.player->center().x - goal) <= 8.0f &&
                    run.player->velocity().x == 0.0f && supported(*run.player))
                {
                    ++result.turns;
                    far_goal = !far_goal;
                }
                if (tick == 180 || tick == 360 || tick == active_ticks)
                {
                    std::cout << "MEASURE Crossfeed home_loop tool=" << loop_tool_name(type) <<
                        " team=" << (right ? 'a' : 'b') << " route=" << loop_name(route) << " fire=" << fire <<
                        " checkpoint_tick=" << tick << " elapsed_seconds=" << fixed(static_cast<float>(tick) * dt, 6) <<
                        " interval_ticks=" << tick - previous_checkpoint_tick <<
                        " owned_area=" << fixed(current_owned) <<
                        " additional_since_checkpoint=" << fixed(current_owned - previous_checkpoint_owned) <<
                        " enemy_area_lost=0 score_difference_gain=" << fixed(current_owned) <<
                        " reserve=" << fixed(run.player->weapon_ammo(), 6) << " paid_events=" << run.paid_events <<
                        " paid_cost=" << fixed(static_cast<float>(run.paid_events) * services.tuning.weapon(type).details.ammo_usage, 6) <<
                        " lower_landing_tick=" << result.lower_landing_tick << " full_body_home_exit_tick=" << result.home_exit_tick <<
                        " first_supported_plinth_tick=" << result.plinth_visit_tick << " stopped_endpoint_count=" << result.turns <<
                        " x=" << fixed(run.player->center().x) << " y=" << fixed(run.player->center().y) <<
                        " flying_objects=" << flying_objects(run) <<
                        " near_floor_object_owned=" << fixed(run.team_area(team, floor.name)) <<
                        " home_floor_object_owned=" << fixed(run.team_area(team, home.name)) <<
                        " plinth_owned=" << fixed(run.team_area(team, plinth.name)) <<
                        " scope=cold-spawn-fixed-horizon-no-live-opponent\n";
                    previous_checkpoint_owned = current_owned;
                    previous_checkpoint_tick = tick;
                }
            }
            require(result.lower_landing_tick > 0 && result.turns >= 3,
                "Crossfeed home-loop policy failed to drop and complete a stopped out-and-back loop");
            if (route == LoopRoute::advancing)
            {
                require(result.home_exit_tick > 0 && result.plinth_visit_tick > 0,
                    "Crossfeed advancing loop never left home and actually supported on the plinth");
            }
            result.paid = run.paid_events;
            result.active_owned = owned();
            int tail_ticks = 0;
            while (flying_objects(run) > 0 && tail_ticks < flight_tail_limit)
            {
                run.tick(input());
                ++tail_ticks;
                owned();
                require(run.paid_events == result.paid, "Crossfeed home-loop flight tail paid another shot");
            }
            require(flying_objects(run) == 0 && run.ticks <= active_ticks + flight_tail_limit && run.takeoffs == 0,
                "Crossfeed home-loop trial exceeded its flight/update budget or introduced a jump");
            result.settled_owned = owned();
            require(result.settled_owned + tolerance >= result.active_owned,
                "Crossfeed unopposed home loop lost previously owned paint");
            if (fire)
            {
                require(result.paid > 0 && result.settled_owned > 0.0f,
                    "Crossfeed home-loop positive control did not pay for actual paint");
            }
            else
            {
                require(result.paid == 0 && result.active_owned == 0.0f && result.settled_owned == 0.0f &&
                    approximately(run.player->weapon_ammo(), services.tuning.starting_ammo()),
                    "Crossfeed home-loop no-fire control paid ammunition or gained ownership");
            }
            std::cout << "MEASURE Crossfeed home_loop_settled tool=" << loop_tool_name(type) <<
                " team=" << (right ? 'a' : 'b') << " route=" << loop_name(route) << " fire=" << fire <<
                " active_ticks=" << active_ticks << " tail_ticks=" << tail_ticks <<
                " active_owned_area=" << fixed(result.active_owned) << " settled_owned_area=" << fixed(result.settled_owned) <<
                " post_release_gain=" << fixed(result.settled_owned - result.active_owned) << " paid_events=" << result.paid <<
                " tail_policy=release-all-input-brake-and-retire-flight scope=tail-excluded-from-fixed-time-comparison\n";
            return result;
        }
    }

    void home_loop_checks(Services& services)
    {
        std::cout << "NOTE Crossfeed home loops compare 600 active ticks from authored cold spawn zero: "
            "Down to the lower floor, then repeated stopped endpoints starting at canonical 120 then 500, "
            "or 1150 then 2450, mirrored by team. Held primary uses downward aim toward the next endpoint and can resume "
            "after recovery. Checkpoints at 180/360/600 include in-flight paint; a separate shot-free tail "
            "retires projectiles and may brake remaining movement. Near-floor object area includes home and "
            "approach; it is not geographically clipped home ownership. Endpoint count includes first arrival; "
            "checkpoint intervals are 180/180/240 ticks and paid cost is cumulative expenditure, not tank capacity. "
            "These two sampled policies establish "
            "neither optimal coverage, home-loop safety, opposition, human balance nor a route winner\n";
        int trials = 0;
        for (const WeaponType type : { WeaponType::sprayer, WeaponType::sniper, WeaponType::roller,
            WeaponType::mister, WeaponType::bucket })
        {
            for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
            {
                for (const LoopRoute route : { LoopRoute::home, LoopRoute::advancing })
                {
                    const LoopResult firing = loop_trial(services, type, team, route, true);
                    const LoopResult no_fire = loop_trial(services, type, team, route, false);
                    trials += 2;
                    require(firing.trajectory == no_fire.trajectory && firing.turns == no_fire.turns &&
                        firing.lower_landing_tick == no_fire.lower_landing_tick &&
                        firing.home_exit_tick == no_fire.home_exit_tick && firing.plinth_visit_tick == no_fire.plinth_visit_tick,
                        "Crossfeed home-loop paid fire changed matched no-fire movement or route checkpoints");
                }
            }
        }
        require(trials == 40, "Crossfeed home-loop comparison changed its bounded trial count");
        std::cout << "PASS Crossfeed fixed-time lower home/advancing loops: forty mirrored five-tool firing/no-fire "
            "trials, matched movement, paid paint, neutral score conservation and bounded flight completion\n";
    }
}
