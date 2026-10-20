#include "game/pch.h"
#include "scripts/acceptance/crossfeed/fixtures.h"

namespace crossfeed_acceptance
{
    namespace
    {
        constexpr int fire_ticks = 90;
        constexpr int trial_ticks = 150;

        const char* tool_name(WeaponType type)
        {
            switch (type)
            {
            case WeaponType::sprayer: return "sprayer";
            case WeaponType::sniper: return "sniper";
            case WeaponType::roller: return "roller";
            case WeaponType::mister: return "mister";
            case WeaponType::bucket: return "bucket";
            default: throw std::runtime_error("Crossfeed combat trial needs a concrete primary");
            }
        }

        void require_target_bounds(const Run& run, const Player& target)
        {
            const RectangleF body = target.rectangle();
            const RectangleF bounds = run.services.map.info.out_of_bounds_rectangle();
            require(body.left() >= bounds.left() - tolerance && body.right() <= bounds.right() + tolerance &&
                body.top() >= bounds.top() - tolerance && body.bottom() <= bounds.bottom() + tolerance,
                "Crossfeed combat target left full-body bounds at tick " + std::to_string(run.ticks));
            for (const auto& [surface, object] : run.structures)
            {
                if (surface->jump_through()) { continue; }
                const auto overlap = narrow_phase(*target.shape(), *object->shape());
                require(!overlap || overlap->penetration <= tolerance,
                    "Crossfeed combat target penetrated '" + surface->name + "' at tick " +
                    std::to_string(run.ticks));
            }
        }

        float tracked_angle(const Run& run, const Player& target, bool right)
        {
            // Track the current target centre from the weapon's rotation
            // origin every tick. This is scripted upper-bound tracking, not
            // human reaction, projectile lead or ballistic compensation.
            const Vector2F offset = run.services.tuning.weapon(run.player->primary_weapon().type()).details.offset;
            const Vector2F origin = run.player->center() + Vector2F(right ? offset.x : -offset.x, offset.y);
            const Vector2F to_target = target.center() - origin;
            return std::atan2(to_target.y, to_target.x);
        }

        std::string paid_sequence(const Run& run, int start)
        {
            std::ostringstream text;
            for (size_t index = 0; index < run.paid_ticks.size(); ++index)
            {
                if (index != 0) { text << ','; }
                text << run.paid_ticks[index] - start;
            }
            return text.str();
        }

        void moving_mister_trial(Services& s, PlayerTeam team, bool pursuing)
        {
            const bool right = team == PlayerTeam::a;
            const float direction = right ? 1.0f : -1.0f;
            const RectangleF bounds = s.map.info.out_of_bounds_rectangle();
            const Surface& floor = s.map.named(right ? "boundary_floor_a" : "boundary_floor_b");
            const float start_x = right ? 100.0f : bounds.left() + bounds.right() - 100.0f;
            Run run(s, standing_on(s, floor, start_x), team, WeaponType::mister);
            run.settle();
            int acceleration_ticks = 0;
            const float cap = s.tuning.player().max_velocity.x;
            while (std::fabs(run.player->velocity().x) < cap - tolerance && acceleration_ticks < 120)
            {
                run.tick(input(direction));
                ++acceleration_ticks;
            }
            require(approximately(run.player->velocity().x, direction * cap) &&
                approximately(run.player->rectangle().bottom(), floor.rectangle.top()),
                "Crossfeed moving Mister did not reach its configured speed on the initial flat floor");
            Player* target = run.add_opponent(standing_on(s, floor, run.player->center().x - direction * 180.0f));
            const int start = run.ticks;
            const Vector2F owner_start = run.player->center(), target_start = target->center();
            const float target_input = pursuing ? direction : 0.0f;
            run.set_opponent_input(target, [target_input](int) { return input(target_input); });
            run.weapon_aim_after_movement = [&run, target, right] { return tracked_angle(run, *target, !right); };
            int first_damage = -1, lethal = -1, dead_state = -1, owner_plinth = -1, release = -1;
            float damage = 0.0f, owner_min_speed = cap, owner_max_speed = cap, target_max_speed = 0.0f;
            float owner_min_feet = floor.rectangle.top(), target_min_feet = floor.rectangle.top();
            float release_ammo = -1.0f, recovery_ammo = -1.0f;
            Vector2F first_step_min, first_step_max;
            std::vector<std::pair<const CollisionObject*, Vector2F>> first_launch;
            int first_launch_retired = 0, first_step_objects = 0, first_update_retired = 0;
            bool first_step_observed = false;
            run.after_scene_update = [&]
            {
                if (first_launch.empty() || first_step_observed) { return; }
                // Scene keeps every object alive until end_tick, including
                // one that expires during update. Observe before the swept
                // contacts can move or retire it, and never reuse these
                // borrowed pointers after this single observation.
                for (const auto& [object, origin] : first_launch)
                {
                    const auto objects = run.scene.collision_objects();
                    require(std::find(objects.begin(), objects.end(), object) != objects.end(),
                        "Crossfeed first Mister launch retired before its first Scene update");
                    const Vector2F displacement = (object->shape()->bounding_box().center() - origin) / dt;
                    if (first_step_objects++ == 0) { first_step_min = first_step_max = displacement; }
                    first_step_min.x = std::min(first_step_min.x, displacement.x);
                    first_step_min.y = std::min(first_step_min.y, displacement.y);
                    first_step_max.x = std::max(first_step_max.x, displacement.x);
                    first_step_max.y = std::max(first_step_max.y, displacement.y);
                    first_update_retired += object->for_deletion() ? 1 : 0;
                }
                first_launch.clear();
                first_step_observed = true;
            };
            for (int tick = 1; tick <= trial_ticks; ++tick)
            {
                const bool firing = tick <= fire_ticks && lethal < 0;
                if (!firing && release < 0) { release = tick - 1; release_ammo = run.player->weapon_ammo(); }
                PlayerInputData actions = input(direction);
                actions.primary_shoot = firing;
                const float before = target->health();
                run.tick(actions);
                require_target_bounds(run, *target);
                const float received = std::max(0.0f, before - target->health());
                damage += received;
                if (received > 0.0f && first_damage < 0) { first_damage = tick; }
                if (target->health() <= 0.0f && lethal < 0) { lethal = tick; }
                if (target->state() == PlayerState::dead && dead_state < 0) { dead_state = tick; }
                owner_min_speed = std::min(owner_min_speed, run.player->velocity().x * direction);
                owner_max_speed = std::max(owner_max_speed, run.player->velocity().x * direction);
                target_max_speed = std::max(target_max_speed, target->velocity().x * direction);
                owner_min_feet = std::min(owner_min_feet, run.player->rectangle().bottom());
                target_min_feet = std::min(target_min_feet, target->rectangle().bottom());
                if (owner_plinth < 0 && run.support() == "centre_plinth") { owner_plinth = tick; }
                if (release >= 0 && tick - release == 60) { recovery_ammo = run.player->weapon_ammo(); }
                if (run.first_paid_tick == run.ticks)
                {
                    first_launch_retired = run.launch_retired_objects;
                    for (CollisionObject* object : run.scene.collision_objects())
                    {
                        if (!is_projectile(to_collision_type(object->tag()))) { continue; }
                        first_launch.emplace_back(object, object->shape()->bounding_box().center());
                    }
                }
            }
            require(run.paid_events > 0 && first_step_observed && first_step_objects > 0,
                "Crossfeed moving Mister never paid for its unobstructed first launch");
            require(first_step_objects + first_launch_retired == 5,
                "Crossfeed first Mister launch lost a pellet between launch and first-step observation");
            require((run.player->center().x - owner_start.x) * direction > tolerance,
                "Crossfeed moving Mister owner did not advance under continued full input");
            require(!pursuing || ((target->center().x - target_start.x) * direction > tolerance && target_max_speed > 0.0f),
                "Crossfeed Mister pursuer did not accelerate under its explicit input");
            require(pursuing || approximately(target->center().x, target_start.x),
                "Crossfeed stationary Mister chase control moved horizontally");
            std::cout << "MEASURE Crossfeed moving_mister team=" << (right ? 'a' : 'b') <<
                " target=" << (pursuing ? "accelerating-pursuer" : "stationary-behind") <<
                " aim=scripted-upper-bound-backward-current-centre-tracking lead=none ballistic_compensation=none" <<
                " total_ticks=" << trial_ticks <<
                " acceleration_ticks=" << acceleration_ticks << " configured_cap=" << fixed(cap) <<
                " owner_input_x=" << fixed(direction) << " target_input_x=" << fixed(target_input) <<
                " owner_start_x=" << fixed(owner_start.x) << " owner_end_x=" << fixed(run.player->center().x) <<
                " owner_end_y=" << fixed(run.player->center().y) << " owner_min_forward_vx=" << fixed(owner_min_speed) <<
                " owner_max_forward_vx=" << fixed(owner_max_speed) << " target_start_x=" << fixed(target_start.x) <<
                " target_end_x=" << fixed(target->center().x) << " target_end_y=" << fixed(target->center().y) <<
                " target_displacement_x=" << fixed(target->center().x - target_start.x) <<
                " target_max_forward_vx=" << fixed(target_max_speed) << " target_end_vx=" << fixed(target->velocity().x) <<
                " owner_min_feet_y=" << fixed(owner_min_feet) << " target_min_feet_y=" << fixed(target_min_feet) <<
                " owner_first_plinth_tick=" << owner_plinth << " first_damage_tick=" << first_damage <<
                " lethal_health_tick=" << lethal << " dead_state_tick=" << dead_state << " damage=" << fixed(damage, 4) <<
                " final_health=" << fixed(target->health(), 4) << " first_paid_tick=" << run.first_paid_tick - start <<
                " first_paid_owner_vx=" << fixed(run.first_paid_velocity.x) << " first_paid_owner_vy=" << fixed(run.first_paid_velocity.y) <<
                " first_paid_nozzle_x=" << fixed(run.first_paid_nozzle.x) << " first_paid_nozzle_y=" << fixed(run.first_paid_nozzle.y) <<
                " first_step_vx_min=" << fixed(first_step_min.x) << " first_step_vx_max=" << fixed(first_step_max.x) <<
                " first_step_vy_min=" << fixed(first_step_min.y) << " first_step_vy_max=" << fixed(first_step_max.y) <<
                " first_step_objects=" << first_step_objects << " first_launch_retired_objects=" << first_launch_retired <<
                " first_update_retired_objects=" << first_update_retired <<
                " first_step_scope=collider-centre-displacement-after-update-before-sweep-includes-gravity-drag" <<
                " paid_events=" << run.paid_events << " paid_ticks=" << paid_sequence(run, start) <<
                " emitted_survivors=" << run.emitted_objects << " launch_retired_objects=" << run.launch_retired_objects <<
                " swept_retired_objects=" << run.swept_retired_objects << " release_tick=" << release <<
                " release_ammo=" << fixed(release_ammo, 4) << " release_plus_60_ticks_ammo=" << fixed(recovery_ammo, 4) <<
                " total_painted_area=" << fixed(run.team_area(team)) <<
                " near_floor_area=" << fixed(run.team_area(team, right ? "floor_paint_a" : "floor_paint_b")) <<
                " far_floor_area=" << fixed(run.team_area(team, right ? "floor_paint_b" : "floor_paint_a")) <<
                " plinth_area=" << fixed(run.team_area(team, "centre_plinth")) <<
                " mixer_area=" << fixed(run.team_area(team, "centre_mixer")) << " scope=relative-motion-observation-no-positive-damage-gate\n";
        }

        void trial(Services& s, WeaponType type, PlayerTeam team, bool retreat,
            float target_x, bool require_damage, const char* station)
        {
            const bool right = team == PlayerTeam::a;
            const RectangleF bounds = s.map.info.out_of_bounds_rectangle();
            const auto x = [&](float value) { return right ? value : bounds.left() + bounds.right() - value; };
            const Surface& near_floor = s.map.named(right ? "floor_paint_a" : "floor_paint_b");
            Run run(s, standing_on(s, near_floor, x(1050.0f)), team, type);
            // Establish mirrored facing through real input, including the
            // roller's fixed aim. Let the small orientation step brake before
            // constructing the target at the same measured centre separation.
            run.tick(input(right ? 1.0f : -1.0f));
            run.settle();
            const float oriented_target_x = x(target_x) + run.player->center().x - x(1050.0f);
            Player* target = run.add_opponent(standing_on(s, near_floor, oriented_target_x));
            run.settle();
            require(supported(*target), "Crossfeed combat target did not settle on its lower floor");
            require_target_bounds(run, *target);
            const int start = run.ticks;
            const Vector2F initial_position = target->center();
            const float initial_health = target->health();
            const float initial_ammo = run.player->weapon_ammo();
            const float movement = retreat ? (right ? 1.0f : -1.0f) : 0.0f;
            run.set_opponent_input(target, [movement](int) { return input(movement); });
            run.weapon_aim_after_movement = [&run, target, right] { return tracked_angle(run, *target, right); };

            int first_damage = -1, lethal = -1, dead_state = -1, escape = -1;
            int release = -1;
            float release_ammo = initial_ammo;
            float release_plus_30 = -1.0f, release_plus_60 = -1.0f;
            float damage = 0.0f, max_target_speed = 0.0f;
            const Surface& plinth = s.map.named("centre_plinth");
            for (int tick = 1; tick <= trial_ticks; ++tick)
            {
                const bool firing = tick <= fire_ticks && lethal < 0;
                if (!firing && release < 0)
                {
                    release = tick - 1;
                    release_ammo = run.player->weapon_ammo();
                }
                PlayerInputData actions = input();
                actions.primary_shoot = firing;
                const float before = target->health();
                run.tick(actions);
                require_target_bounds(run, *target);
                const float received = std::max(0.0f, before - target->health());
                damage += received;
                if (received > 0.0f && first_damage < 0) { first_damage = tick; }
                if (target->health() <= 0.0f && lethal < 0) { lethal = tick; }
                if (target->state() == PlayerState::dead && dead_state < 0) { dead_state = tick; }
                max_target_speed = std::max(max_target_speed, std::fabs(target->velocity().x));
                const RectangleF body = target->rectangle();
                // A geometric marker only: full body beyond the far plinth
                // face. This does not assert immunity from arcs or every aim.
                if (retreat && escape < 0 && target->health() > 0.0f &&
                    (right ? body.left() >= plinth.rectangle.right() : body.right() <= plinth.rectangle.left()))
                {
                    escape = tick;
                }
                if (release >= 0 && tick - release == 30) { release_plus_30 = run.player->weapon_ammo(); }
                if (release >= 0 && tick - release == 60) { release_plus_60 = run.player->weapon_ammo(); }
            }

            require(run.paid_events > 0 && !run.paid_ticks.empty() && run.minimum_ammo < initial_ammo,
                std::string("Crossfeed ") + tool_name(type) + " combat station never paid for fire");
            require(!require_damage || damage > 0.0f,
                std::string("Crossfeed ") + tool_name(type) + " stationary contact control never damaged its real Player target");
            if (retreat)
            {
                require((target->center().x - initial_position.x) * movement > tolerance && max_target_speed > 0.0f,
                    "Crossfeed retreat target did not move under its explicit input stream");
            }
            else
            {
                require(approximately(target->center().x, initial_position.x),
                    "Crossfeed stationary combat control moved horizontally");
            }
            require(run.team_area(target->team()) == 0.0f, "Crossfeed idle-weapon target unexpectedly painted");
            const float near_area = run.team_area(team, near_floor.name);
            const float far_area = run.team_area(team, right ? "floor_paint_b" : "floor_paint_a");
            const float near_bench = run.team_area(team, right ? "side_paint_a" : "side_paint_b");
            const float far_bench = run.team_area(team, right ? "side_paint_b" : "side_paint_a");
            std::cout << "MEASURE Crossfeed combat tool=" << tool_name(type) << " team=" << (right ? 'a' : 'b') <<
                " station=" << station << " target=" << (retreat ? "active-retreat" : "stationary") <<
                " aim=" << (type == WeaponType::roller ? "fixed-forward-down" : "scripted-upper-bound-current-centre-tracking") <<
                " lead=none ballistic_compensation=none dt=" << fixed(dt, 7) << " total_ticks=" << trial_ticks <<
                " attacker_x=" << fixed(run.player->center().x) <<
                " target_input_x=" << fixed(movement) << " target_start_x=" << fixed(initial_position.x) <<
                " target_end_x=" << fixed(target->center().x) << " target_end_y=" << fixed(target->center().y) <<
                " target_max_abs_vx=" << fixed(max_target_speed) << " first_damage_tick=" << first_damage <<
                " lethal_health_tick=" << lethal << " dead_state_tick=" << dead_state <<
                " full_body_beyond_far_plinth_tick=" << escape << " damage=" << fixed(damage, 4) <<
                " initial_health=" << fixed(initial_health, 4) << " final_health=" << fixed(target->health(), 4) <<
                " paid_events=" << run.paid_events << " paid_ticks=" << paid_sequence(run, start) <<
                " emitted_survivors=" << run.emitted_objects << " launch_retired_objects=" << run.launch_retired_objects <<
                " paid_without_surviving_flight=" << run.paid_events_without_flight <<
                " swept_retired_objects=" << run.swept_retired_objects <<
                " minimum_ammo=" << fixed(run.minimum_ammo, 4) << " release_tick=" << release <<
                " release_ammo=" << fixed(release_ammo, 4) << " release_plus_30_ticks_ammo=" << fixed(release_plus_30, 4) <<
                " release_plus_60_ticks_ammo=" << fixed(release_plus_60, 4) <<
                " final_ammo=" << fixed(run.player->weapon_ammo(), 4) <<
                " near_floor_area=" << fixed(near_area) << " far_floor_area=" << fixed(far_area) <<
                " near_bench_area=" << fixed(near_bench) << " far_bench_area=" << fixed(far_bench) <<
                " plinth_area=" << fixed(run.team_area(team, "centre_plinth")) <<
                " mixer_area=" << fixed(run.team_area(team, "centre_mixer")) <<
                " launch_paint=" << fixed(run.launch_paint) << " swept_paint=" << fixed(run.swept_paint) << '\n';
        }
    }

    void combat_checks(Services& s)
    {
        require(static_cast<float>(trial_ticks) * dt < s.tuning.player().respawn_delay,
            "Crossfeed bounded combat observation must finish before a lethal target can respawn");
        std::cout << "NOTE Crossfeed combat: 90 fire-input ticks then 60 shot-free ticks; lethal health ends fire early. "
            "Each trial lasts 150 ticks, below the target's three-second respawn delay. "
            "Launch retirements include enemy hits and wet/dry cover; they are not all blocked shots. "
            "No human aim, projectile-leading skill, complete escape safety or balance acceptance is claimed\n";
        for (const WeaponType type : { WeaponType::sprayer, WeaponType::sniper, WeaponType::roller,
            WeaponType::mister, WeaponType::bucket })
        {
            for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
            {
                // Common 130-unit centre separation measures all five at the
                // same geometry. Roller is expected to need a closer control.
                trial(s, type, team, false, 1180.0f, type != WeaponType::roller, "lower-130");
                trial(s, type, team, true, 1180.0f, false, "lower-130");
                if (type == WeaponType::roller)
                {
                    trial(s, type, team, false, 1120.0f, true, "roller-contact-70");
                }
            }
        }
        for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
        {
            moving_mister_trial(s, team, false);
            moving_mister_trial(s, team, true);
        }
        std::cout << "PASS Crossfeed bounded paid combat observations include mirrored real stationary targets, "
            "all-five explicit retreat inputs, and close roller damage controls; retreat damage is a measurement, not a pass gate\n";
    }
}
