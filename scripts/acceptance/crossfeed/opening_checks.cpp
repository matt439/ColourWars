#include "game/pch.h"
#include "scripts/acceptance/crossfeed/fixtures.h"
#include <limits>

namespace crossfeed_acceptance
{
    namespace
    {
        constexpr int opening_ticks = 180;

        struct OpeningPolicy
        {
            const char* name;
            bool fire;
            bool delayed;
            bool aim_at_floor;
        };

        struct OpeningShot
        {
            int tick = -1;
            Vector2F owner;
            Vector2F velocity;
            Vector2F nozzle;
            float angle = 0.0f;
            float before_ammo = 0.0f;
            float after_ammo = 0.0f;
            int launch_survivors = 0;
            int launch_retired = 0;
        };

        struct OpeningSnapshot
        {
            int paid = 0;
            int first_paint = -1;
            float reserve = 0.0f;
            float area = 0.0f;
        };

        int flying_objects(const Run& run)
        {
            int count = 0;
            for (const CollisionObject* object : run.scene.collision_objects())
            {
                if (is_projectile(to_collision_type(object->tag())) && !object->for_deletion()) { ++count; }
            }
            return count;
        }

        std::pair<std::string, std::string> painted_surfaces(const Run& run, PlayerTeam team)
        {
            std::string names;
            std::string bounds;
            for (const auto& [surface, object] : run.structures)
            {
                (void)object;
                if (run.team_area(team, surface->name) <= 0.0f) { continue; }
                if (!names.empty()) { names += '|'; bounds += '|'; }
                names += surface->name;
                const RectangleF& r = surface->rectangle;
                bounds += surface->name + ':' + fixed(r.x) + ',' + fixed(r.y) + ',' +
                    fixed(r.width) + ',' + fixed(r.height);
            }
            return { names.empty() ? "none" : names, bounds.empty() ? "none" : bounds };
        }

        OpeningSnapshot sniper_opening(Services& s, const OpeningPolicy& policy, PlayerTeam team, size_t spawn)
        {
            const bool right = team == PlayerTeam::a;
            const float direction = right ? 1.0f : -1.0f;
            const auto points = right ? s.map.info.team_a_spawns() : s.map.info.team_b_spawns();
            require(points.size() == 4 && spawn < points.size(),
                "Crossfeed Sniper openings need all four authored spawns per team");
            const Surface& floor = s.map.named(right ? "floor_paint_a" : "floor_paint_b");
            const Surface& home = s.map.named(right ? "spawn_a_floor" : "spawn_b_floor");
            // Home accepts paint too. Its inward edge defines a geographic
            // departure gate independently of the lower floor's scored extent.
            const float gate_x = right ? home.rectangle.right() : home.rectangle.left();
            const Vector2F target(floor.rectangle.center().x, floor.rectangle.top());
            const float forward_down = right ? PI / 3.0f : 2.0f * PI / 3.0f;
            Run run(s, points[spawn], team, WeaponType::sniper);
            const float starting_ammo = run.player->weapon_ammo();
            require(run.ticks == 0 && run.paid_events == 0 && run.team_area(team) == 0.0f &&
                !run.player->primary_weapon().ready_to_shoot() &&
                approximately(starting_ammo, s.tuning.starting_ammo()) &&
                run.support() == (right ? "spawn_a_floor" : "spawn_b_floor") &&
                run.player->rectangle() == RectangleF(points[spawn], s.tuning.player().size),
                "Crossfeed Sniper opening did not begin cold at its complete authored spawn body");
            require((run.player->center().x - gate_x) * direction < 0.0f,
                "Crossfeed Sniper opening must start inside its home-exit gate");

            // This is a fixed authored target, not a trace, a visible-surface
            // search or a reaction to the previous shot. Sniper's barrel lies
            // along the ray from its signed attachment pivot; aim after actual
            // movement just as the existing current-centre combat policy does.
            if (policy.aim_at_floor)
            {
                const Vector2F offset = s.tuning.weapon(WeaponType::sniper).details.offset;
                run.weapon_aim_after_movement = [&run, target, offset, right]()
                {
                    const Vector2F centre = run.player->center();
                    const bool target_right = target.x > centre.x || (target.x == centre.x && right);
                    const Vector2F pivot(centre.x + (target_right ? offset.x : -offset.x), centre.y + offset.y);
                    return std::atan2(target.y - pivot.y, target.x - pivot.x);
                };
            }

            std::vector<OpeningShot> shots;
            int observed_paid = 0;
            float tick_before_ammo = starting_ammo;
            run.on_weapon_launch = [&](std::span<const std::unique_ptr<Projectile>> emitted)
            {
                if (run.paid_events == observed_paid) { return; }
                require(run.paid_events == observed_paid + 1 && emitted.size() <= 1,
                    "Crossfeed Sniper opening lost its actual paid-launch association");
                observed_paid = run.paid_events;
                OpeningShot shot;
                shot.tick = run.ticks;
                shot.owner = run.player->center();
                shot.velocity = run.player->velocity();
                shot.nozzle = run.player->primary_weapon().nozzle_pos();
                shot.angle = run.player->primary_weapon().rotation();
                shot.before_ammo = tick_before_ammo;
                shot.after_ammo = run.player->weapon_ammo();
                shot.launch_survivors = static_cast<int>(emitted.size());
                shot.launch_retired = 1 - shot.launch_survivors;
                shots.push_back(shot);
            };

            int landed_lower_tick = -1;
            int first_home_exit_tick = -1;
            int first_fire_request_tick = -1;
            int first_paint_tick = -1;
            Vector2F first_paint_owner;
            std::string first_paint_surfaces = "none";
            std::string first_paint_bounds = "none";
            bool armed = !policy.delayed;
            for (int tick = 1; tick <= opening_ticks; ++tick)
            {
                // Establish lower support by real contacts before walking.
                // The first live input starts at construction: no idle ticks
                // or weapon updates have been inserted to prime the cooldown.
                if (landed_lower_tick < 0 && supported(*run.player) &&
                    approximately(run.player->rectangle().bottom(), floor.rectangle.top()))
                { landed_lower_tick = tick - 1; }
                if (first_home_exit_tick < 0 && (run.player->center().x - gate_x) * direction >= 0.0f)
                { first_home_exit_tick = tick; }
                if (policy.delayed && first_home_exit_tick >= 0) { armed = true; }
                PlayerInputData actions = aimed(forward_down,
                    input(landed_lower_tick >= 0 ? direction : 0.0f, false, false, landed_lower_tick < 0));
                actions.primary_shoot = policy.fire && armed;
                if (actions.primary_shoot && first_fire_request_tick < 0) { first_fire_request_tick = tick; }
                tick_before_ammo = run.player->weapon_ammo();
                run.tick(actions);
                if (first_paint_tick < 0 && run.team_area(team) > 0.0f)
                {
                    require(run.paid_events > 0, "Crossfeed Sniper opening painted without a paid shot");
                    first_paint_tick = tick;
                    first_paint_owner = run.player->center();
                    const auto [names, bounds] = painted_surfaces(run, team);
                    first_paint_surfaces = names;
                    first_paint_bounds = bounds;
                }
            }
            // Neither callback retains a projectile. Clear borrowed local
            // references before their storage is destroyed at the trial end.
            run.on_weapon_launch = {};
            run.weapon_aim_after_movement = {};
            require(run.ticks == opening_ticks && landed_lower_tick >= 0 && run.takeoffs == 0 &&
                (run.player->center().x - (points[spawn].x + s.tuning.player().size.x / 2.0f)) * direction >
                    s.tuning.player().size.x,
                "Crossfeed Sniper opening did not drop and make jump-free walking progress");
            require(shots.size() == static_cast<size_t>(run.paid_events) &&
                (shots.empty() ? run.first_paid_tick < 0 : shots.front().tick == run.first_paid_tick),
                "Crossfeed Sniper opening paid-shot observations disagree with the actual Run");
            require(first_home_exit_tick > 1 && first_home_exit_tick <= opening_ticks,
                "Crossfeed Sniper opening did not cross its geographic home exit after live movement");
            if (policy.fire)
            {
                require(first_fire_request_tick == (policy.delayed ? first_home_exit_tick : 1),
                    "Crossfeed immediate/delayed firing request did not follow its distinct home-exit policy");
                require(run.first_paid_tick >= first_fire_request_tick,
                    "Crossfeed Sniper opening paid before its first firing request");
            }
            if (!policy.fire)
            {
                require(first_fire_request_tick == -1 && run.paid_events == 0 &&
                    run.team_area(team) == 0.0f && flying_objects(run) == 0 &&
                    run.player->weapon_ammo() == starting_ammo,
                    "Crossfeed Sniper no-fire control paid, painted or lost ammunition");
            }

            std::cout << "MEASURE Crossfeed sniper_opening policy=" << policy.name <<
                " team=" << (right ? 'a' : 'b') << " spawn=" << spawn <<
                " active_input_ticks=" << opening_ticks << " start_x=" << fixed(points[spawn].x) <<
                " start_y=" << fixed(points[spawn].y) << " landed_lower_tick=" << landed_lower_tick <<
                " home_exit_gate_x=" << fixed(gate_x) << " home_exit_input_tick=" << first_home_exit_tick <<
                " fixed_target_surface=" << (policy.aim_at_floor ? floor.name : "none") <<
                " fixed_target_x=" << (policy.aim_at_floor ? fixed(target.x) : "none") <<
                " fixed_target_y=" << (policy.aim_at_floor ? fixed(target.y) : "none") <<
                " first_fire_request_tick=" << first_fire_request_tick <<
                " first_paid_tick=" << run.first_paid_tick <<
                " first_paid_nozzle_x=" << fixed(run.first_paid_nozzle.x) <<
                " first_paid_nozzle_y=" << fixed(run.first_paid_nozzle.y) <<
                " first_any_paint_tick=" << first_paint_tick <<
                " first_any_paint_seconds=" << (first_paint_tick < 0 ? "unobserved" :
                    fixed(static_cast<float>(first_paint_tick) * dt, 4)) <<
                " first_paint_owner_centre_x=" << fixed(first_paint_owner.x) <<
                " first_paint_owner_centre_y=" << fixed(first_paint_owner.y) <<
                " first_paint_surfaces=" << first_paint_surfaces <<
                " first_paint_surface_bounds=" << first_paint_bounds <<
                " paid_events=" << run.paid_events << " emitted_survivors=" << run.emitted_objects <<
                " launch_retired_objects=" << run.launch_retired_objects <<
                " swept_retired_objects=" << run.swept_retired_objects <<
                " starting_ammo=" << fixed(starting_ammo, 6) <<
                " ammo_at_180=" << fixed(run.player->weapon_ammo(), 6) <<
                " ammo_fraction_at_180=" << fixed(run.player->weapon_ammo() / s.tuning.starting_ammo(), 6) <<
                " minimum_ammo=" << fixed(run.minimum_ammo, 6) <<
                " unique_owned_area_at_180=" << fixed(run.team_area(team)) <<
                " flying_objects_at_180=" << flying_objects(run) <<
                " progress_centre_x=" << fixed(run.player->center().x) <<
                " progress_centre_y=" << fixed(run.player->center().y) <<
                " progress_vx=" << fixed(run.player->velocity().x) <<
                " progress_vy=" << fixed(run.player->velocity().y) <<
                " progress_support=" << run.support() << " takeoffs=" << run.takeoffs <<
                " paint_observation=" << (first_paint_tick < 0 ? "unobserved-within-window" : "observed-within-window") <<
                " scope=180-live-input-tick-snapshot-no-released-settling-or-later-flight-credit\n";
            for (size_t index = 0; index < shots.size(); ++index)
            {
                const OpeningShot& shot = shots[index];
                std::cout << "MEASURE Crossfeed sniper_opening_paid policy=" << policy.name <<
                    " team=" << (right ? 'a' : 'b') << " spawn=" << spawn << " paid_index=" << index <<
                    " tick=" << shot.tick << " owner_centre_x=" << fixed(shot.owner.x) <<
                    " owner_centre_y=" << fixed(shot.owner.y) << " owner_vx=" << fixed(shot.velocity.x) <<
                    " owner_vy=" << fixed(shot.velocity.y) << " aim=" << fixed(shot.angle, 6) <<
                    " nozzle_x=" << fixed(shot.nozzle.x) << " nozzle_y=" << fixed(shot.nozzle.y) <<
                    " before_ammo=" << fixed(shot.before_ammo, 6) << " after_ammo=" << fixed(shot.after_ammo, 6) <<
                    " launch_survivors=" << shot.launch_survivors << " launch_retired=" << shot.launch_retired <<
                    " scope=actual-paid-launch-before-scene-contact-resolution\n";
            }
            return { run.paid_events, first_paint_tick, run.player->weapon_ammo(), run.team_area(team) };
        }
    }

    void opening_checks(Services& s)
    {
        std::cout << "NOTE Crossfeed Sniper cold-opening comparison: five policies across all eight authored spawns. "
            "Every fresh Player begins without cooldown priming, drops until actual lower-floor contact and walks "
            "toward the centre for the same 180 live input ticks. The four firing policies combine immediate or "
            "home-exit-delayed held input with forward/down aim or aim at the fixed own scored-floor top "
            "midpoint from the actual attachment pivot. Gates and targets use authored geometry and current motion, "
            "never visibility searches, prior shot success or preselected outcomes. Once enabled, firing remains "
            "requested through the window; home-exit delay is verified to withhold live input while the centre "
            "remains inside home. Weapon cooldown may still make actual paid ticks coincide. "
            "The no-fire policy is a negative control. Public paid-launch observations "
            "include launch retirements and ordinary scene sweeps. First paint includes home and identifies actual scored surfaces "
            "and their bounds, with the owner's simultaneous centre separately, not an exact impact point. "
            "The snapshot credits only paint observed by tick180, with surviving projectiles counted but no later "
            "flight or released settling credited. Failure to paint is reported, not rejected. These policies are "
            "not earliest possible openings, human aim, whole-tank efficiency or evidence that Sniper needs a nerf\n";
        for (const OpeningPolicy policy : {
            OpeningPolicy{ "no-fire-control", false, false, false },
            OpeningPolicy{ "immediate-forward-down", true, false, false },
            OpeningPolicy{ "immediate-aimed-floor", true, false, true },
            OpeningPolicy{ "delayed-forward-down", true, true, false },
            OpeningPolicy{ "delayed-aimed-floor", true, true, true } })
        {
            int painted = 0, no_payment = 0, first_minimum = opening_ticks + 1, first_maximum = -1;
            int paid_minimum = std::numeric_limits<int>::max(), paid_maximum = 0;
            float reserve_minimum = std::numeric_limits<float>::max(), reserve_maximum = 0.0f;
            float area_minimum = std::numeric_limits<float>::max(), area_maximum = 0.0f;
            for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
            {
                for (size_t spawn = 0; spawn < 4; ++spawn)
                {
                    const OpeningSnapshot result = sniper_opening(s, policy, team, spawn);
                    painted += result.first_paint >= 0 ? 1 : 0;
                    no_payment += result.paid == 0 ? 1 : 0;
                    if (result.first_paint >= 0)
                    {
                        first_minimum = std::min(first_minimum, result.first_paint);
                        first_maximum = std::max(first_maximum, result.first_paint);
                    }
                    paid_minimum = std::min(paid_minimum, result.paid);
                    paid_maximum = std::max(paid_maximum, result.paid);
                    reserve_minimum = std::min(reserve_minimum, result.reserve);
                    reserve_maximum = std::max(reserve_maximum, result.reserve);
                    area_minimum = std::min(area_minimum, result.area);
                    area_maximum = std::max(area_maximum, result.area);
                }
            }
            std::cout << "MEASURE Crossfeed sniper_opening_summary policy=" << policy.name <<
                " trials=8 active_input_ticks=" << opening_ticks << " painted_within_window=" << painted <<
                " unobserved_within_window=" << 8 - painted << " trials_without_payment=" << no_payment <<
                " first_paint_min_tick=" << (painted > 0 ? first_minimum : -1) <<
                " first_paint_max_tick=" << first_maximum <<
                " paid_events_min=" << paid_minimum << " paid_events_max=" << paid_maximum <<
                " ammo_at_180_min=" << fixed(reserve_minimum, 6) <<
                " ammo_at_180_max=" << fixed(reserve_maximum, 6) <<
                " unique_owned_area_at_180_min=" << fixed(area_minimum) <<
                " unique_owned_area_at_180_max=" << fixed(area_maximum) <<
                " scope=matched-cold-opening-policy-observations-not-optimality-or-balance\n";
        }
        std::cout << "PASS Crossfeed bounded Sniper aimed/delayed cold-opening observations and no-fire controls\n";
    }
}
