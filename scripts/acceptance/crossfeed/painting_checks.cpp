#include "game/pch.h"
#include "scripts/acceptance/crossfeed/fixtures.h"

namespace crossfeed_acceptance
{
    namespace
    {
        const char* painting_tool_name(WeaponType type)
        {
            switch (type)
            {
            case WeaponType::sprayer: return "sprayer";
            case WeaponType::sniper: return "sniper";
            case WeaponType::roller: return "roller";
            case WeaponType::mister: return "mister";
            case WeaponType::bucket: return "bucket";
            default: throw std::runtime_error("Crossfeed painting needs a concrete primary");
            }
        }

        float painting_steer(const Run& run, float goal)
        {
            const float error = goal - run.player->center().x;
            const float velocity = run.player->velocity().x;
            const float stopping = velocity * velocity /
                (2.0f * run.services.tuning.player().x_deacceleration) + std::fabs(velocity) * dt + 4.0f;
            if (std::fabs(error) < 5.0f || (velocity != 0.0f &&
                (velocity > 0.0f) == (error > 0.0f) && std::fabs(error) <= stopping)) { return 0.0f; }
            return error > 0.0f ? 1.0f : -1.0f;
        }

        bool painting_arrived(const Run& run, float goal)
        {
            return std::fabs(run.player->center().x - goal) <= 8.0f &&
                run.player->velocity().x == 0.0f && supported(*run.player);
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

        struct PaintingObservation
        {
            Run& run;
            WeaponType type;
            PlayerTeam team;
            int start_tick;
            float starting_ammo;
            int first_paint_tick = -1;
            bool first_paint_during_active = false;
            Vector2F first_paint_owner;
            std::string first_paint_surfaces = "none";
            std::string first_paint_surface_bounds = "none";
            int first_empty_state_tick = -1;
            Vector2F first_empty_state_owner;
            int first_post_paid_empty_tick = -1;
            int first_missed_tick = -1;
            Vector2F first_missed_owner;
            float first_missed_before_ammo = -1.0f;
            float first_missed_after_ammo = -1.0f;
            float observed_cooldown = 0.0f;
            float recovered_during_active = 0.0f;
            int release_tick = -1;
            float release_ammo = -1.0f;
            std::array<float, 3> released_ammo{ -1.0f, -1.0f, -1.0f };
            int release_observed_ticks = 0;
            int full_refill_tick = -1;
            int all_flight_finished_tick = -1;

            PaintingObservation(Run& with, WeaponType tool, PlayerTeam colour) :
                run(with), type(tool), team(colour), start_tick(with.ticks), starting_ammo(with.player->weapon_ammo())
            {
                require(with.paid_events == 0 && with.team_area(colour) == 0.0f,
                    "Crossfeed painting assay did not start with clean actual scored surfaces");
                // Run has updated this zero-initialized weapon once per tick,
                // with no paid reset before observation starts. Keep the same
                // float addition order rather than multiply ticks by delta.
                for (int tick = 0; tick < with.ticks; ++tick) { this->observed_cooldown += dt; }
            }

            void tick(const PlayerInputData& actions, bool active)
            {
                const float before_ammo = this->run.player->weapon_ammo();
                const Weapon& weapon_before = this->run.player->primary_weapon();
                const WeaponReadiness readiness_before = weapon_before.readiness();
                const float interval = this->run.services.tuning.weapon(this->type).details.shoot_interval;
                const bool cooldown_eligible = this->observed_cooldown > interval;
                if (readiness_before != WeaponReadiness::empty)
                {
                    require(weapon_before.ready_to_shoot() == cooldown_eligible,
                        "Crossfeed painting observed cooldown disagrees with public affordable readiness");
                }
                const int paid_before = this->run.paid_events;
                this->run.tick(actions);
                const float after_ammo = this->run.player->weapon_ammo();
                const bool paid_this_tick = this->run.paid_events > paid_before;
                if (active && after_ammo > before_ammo)
                { this->recovered_during_active += after_ammo - before_ammo; }
                const float cost = this->run.services.tuning.weapon(this->type).details.ammo_usage;
                if (active && actions.primary_shoot && after_ammo < cost &&
                    this->run.player->primary_weapon().readiness() == WeaponReadiness::empty)
                {
                    if (this->first_empty_state_tick < 0)
                    {
                        this->first_empty_state_tick = this->run.ticks - this->start_tick;
                        this->first_empty_state_owner = this->run.player->center();
                    }
                    if (paid_this_tick && this->first_post_paid_empty_tick < 0)
                    { this->first_post_paid_empty_tick = this->run.ticks - this->start_tick; }
                }
                if (active && actions.primary_shoot && this->first_missed_tick < 0 && cooldown_eligible &&
                    readiness_before == WeaponReadiness::empty && !paid_this_tick)
                {
                    this->first_missed_tick = this->run.ticks - this->start_tick;
                    this->first_missed_owner = this->run.player->center();
                    this->first_missed_before_ammo = before_ammo;
                    this->first_missed_after_ammo = after_ammo;
                }
                if (paid_this_tick) { this->observed_cooldown = 0.0f; }
                this->observed_cooldown += dt;
                if (this->first_paint_tick >= 0 || this->run.team_area(this->team) <= 0.0f) { return; }
                require(this->run.paid_events > 0 && this->run.first_paid_tick >= this->start_tick,
                    "Crossfeed opening paint appeared without a paid event in this assay");
                this->first_paint_tick = this->run.ticks - this->start_tick;
                this->first_paint_during_active = active;
                this->first_paint_owner = this->run.player->center();
                this->first_paint_surfaces.clear();
                this->first_paint_surface_bounds.clear();
                // Public paint_total identifies the actual scored surface(s).
                // Bounds locate that authored surface, not an invented exact
                // impact point. The player's simultaneous centre is separate.
                for (const auto& [surface, object] : this->run.structures)
                {
                    (void)object;
                    if (this->run.team_area(this->team, surface->name) <= 0.0f) { continue; }
                    if (!this->first_paint_surfaces.empty())
                    {
                        this->first_paint_surfaces += '|';
                        this->first_paint_surface_bounds += '|';
                    }
                    this->first_paint_surfaces += surface->name;
                    const RectangleF& r = surface->rectangle;
                    this->first_paint_surface_bounds += surface->name + ':' + fixed(r.x) + ',' + fixed(r.y) + ',' +
                        fixed(r.width) + ',' + fixed(r.height);
                }
            }

            void released_recovery()
            {
                this->release_tick = this->run.ticks - this->start_tick;
                this->release_ammo = this->run.player->weapon_ammo();
                if (this->release_ammo >= this->run.services.tuning.starting_ammo()) { this->full_refill_tick = 0; }
                if (flying_objects(this->run) == 0) { this->all_flight_finished_tick = 0; }
                for (int tick = 1; tick <= 720; ++tick)
                {
                    this->tick(input(), false);
                    if (tick == 30) { this->released_ammo[0] = this->run.player->weapon_ammo(); }
                    if (tick == 60) { this->released_ammo[1] = this->run.player->weapon_ammo(); }
                    if (tick == 120) { this->released_ammo[2] = this->run.player->weapon_ammo(); }
                    if (this->full_refill_tick < 0 && this->run.player->weapon_ammo() >=
                        this->run.services.tuning.starting_ammo()) { this->full_refill_tick = tick; }
                    if (this->all_flight_finished_tick < 0 && flying_objects(this->run) == 0)
                    { this->all_flight_finished_tick = tick; }
                    this->release_observed_ticks = tick;
                    if (tick >= 120 && this->full_refill_tick >= 0 && this->all_flight_finished_tick >= 0 &&
                        this->run.player->velocity().x == 0.0f && supported(*this->run.player)) { break; }
                }
            }

            void report(const char* assay, int spawn) const
            {
                const float cost = this->run.services.tuning.weapon(this->type).details.ammo_usage;
                std::cout << "MEASURE Crossfeed " << assay << " tool=" << painting_tool_name(this->type) <<
                    " team=" << (this->team == PlayerTeam::a ? 'a' : 'b') << " spawn=" << spawn <<
                    " first_paid_tick=" << (this->run.first_paid_tick < 0 ? -1 : this->run.first_paid_tick - this->start_tick) <<
                    " first_paid_owner_vx=" << fixed(this->run.first_paid_velocity.x) <<
                    " first_paid_nozzle_x=" << fixed(this->run.first_paid_nozzle.x) <<
                    " first_paid_nozzle_y=" << fixed(this->run.first_paid_nozzle.y) <<
                    " first_contest_paint_tick=" << this->first_paint_tick <<
                    " first_contest_paint_seconds=" << (this->first_paint_tick < 0 ? "unobserved" :
                        fixed(static_cast<float>(this->first_paint_tick) * dt, 4)) <<
                    " first_paint_during_active=" << this->first_paint_during_active <<
                    " first_paint_owner_centre_x=" << fixed(this->first_paint_owner.x) <<
                    " first_paint_owner_centre_y=" << fixed(this->first_paint_owner.y) <<
                    " first_paint_surfaces=" << this->first_paint_surfaces <<
                    " first_paint_surface_bounds=" << this->first_paint_surface_bounds <<
                    " first_empty_readiness_tick=" << this->first_empty_state_tick <<
                    " first_empty_owner_centre_x=" << fixed(this->first_empty_state_owner.x) <<
                    " first_empty_owner_centre_y=" << fixed(this->first_empty_state_owner.y) <<
                    " first_post_paid_empty_tick=" << this->first_post_paid_empty_tick <<
                    " first_missed_cooldown_eligible_tick=" << this->first_missed_tick <<
                    " first_missed_owner_centre_x=" << fixed(this->first_missed_owner.x) <<
                    " first_missed_owner_centre_y=" << fixed(this->first_missed_owner.y) <<
                    " first_missed_before_ammo=" << fixed(this->first_missed_before_ammo, 6) <<
                    " first_missed_after_regen_ammo=" << fixed(this->first_missed_after_ammo, 6) <<
                    " paid_events=" << this->run.paid_events << " emitted_survivors=" << this->run.emitted_objects <<
                    " launch_retired_objects=" << this->run.launch_retired_objects <<
                    " swept_retired_objects=" << this->run.swept_retired_objects <<
                    " authored_paid_cost=" << fixed(static_cast<float>(this->run.paid_events) * cost, 6) <<
                    " regained_during_active=" << fixed(this->recovered_during_active, 6) <<
                    " release_tick=" << this->release_tick << " release_ammo=" << fixed(this->release_ammo, 6) <<
                    " active_net_ammo_drop=" << fixed(this->starting_ammo - this->release_ammo, 6) <<
                    " minimum_ammo=" << fixed(this->run.minimum_ammo, 6) <<
                    " release_plus_30_ticks_ammo=" << fixed(this->released_ammo[0], 6) <<
                    " release_plus_60_ticks_ammo=" << fixed(this->released_ammo[1], 6) <<
                    " release_plus_120_ticks_ammo=" << fixed(this->released_ammo[2], 6) <<
                    " full_refill_after_release_tick=" << this->full_refill_tick <<
                    " all_flight_finished_after_release_tick=" << this->all_flight_finished_tick <<
                    " release_observed_ticks=" << this->release_observed_ticks <<
                    " flying_objects_at_end=" << flying_objects(this->run) <<
                    " unique_owned_score_area=" << fixed(this->run.team_area(this->team)) <<
                    " launch_paint=" << fixed(this->run.launch_paint) << " swept_paint=" << fixed(this->run.swept_paint);
                for (const Surface& surface : this->run.services.map.surfaces)
                {
                    if (std::any_of(surface.faces.begin(), surface.faces.end(), [](bool face) { return face; }))
                    { std::cout << " owned_" << surface.name << '=' << fixed(this->run.team_area(this->team, surface.name)); }
                }
                std::cout << " scope=one-scripted-input-policy-actual-paid-map-paint-not-earliest-possible-or-whole-tank-efficiency\n";
            }
        };

        void shared_painting_route(Services& s, WeaponType type, PlayerTeam team)
        {
            const bool right = team == PlayerTeam::a;
            const RectangleF bounds = s.map.info.out_of_bounds_rectangle();
            const auto x = [&](float value) { return right ? value : bounds.left() + bounds.right() - value; };
            const Surface& start_floor = s.map.named(right ? "floor_paint_a" : "floor_paint_b");
            Run run(s, standing_on(s, start_floor, x(1150.0f)), team, type);
            run.settle();
            PaintingObservation observation(run, type, team);
            int phase = 0;
            int outward_ticks = -1;
            int return_ticks = -1;
            for (int tick = 1; tick <= 1800; ++tick)
            {
                const float goal = x(phase == 0 ? 2450.0f : 1150.0f);
                const float aim_direction = (right != (phase == 1)) ? 1.0f : -1.0f;
                PlayerInputData actions = aimed(aim_direction > 0.0f ? PI / 3.0f : 2.0f * PI / 3.0f,
                    input(painting_steer(run, goal)));
                actions.primary_shoot = observation.first_missed_tick < 0;
                observation.tick(actions, true);
                if (!painting_arrived(run, goal)) { continue; }
                if (phase == 0) { outward_ticks = tick; phase = 1; }
                else { return_ticks = tick; break; }
            }
            require(outward_ticks > 0 && return_ticks > outward_ticks && run.takeoffs == 0,
                "Crossfeed shared painting route did not walk, stop and return without a jump");
            const Vector2F returned = run.player->center();
            const int active_paid = run.paid_events;
            observation.released_recovery();
            require(run.paid_events == active_paid, "Crossfeed released route paid for another shot");
            std::cout << "MEASURE Crossfeed paint_route_progress tool=" << painting_tool_name(type) <<
                " team=" << (right ? 'a' : 'b') << " start_centre_x=" << fixed(x(1150.0f)) <<
                " far_centre_x=" << fixed(x(2450.0f)) << " outward_ticks=" << outward_ticks <<
                " returned_tick=" << return_ticks << " returned_centre_x=" << fixed(returned.x) <<
                " returned_centre_y=" << fixed(returned.y) << " takeoffs=" << run.takeoffs <<
                " fire_end_reason=" << (observation.first_missed_tick >= 0 ? "first-missed-eligible-shot-then-complete-route" :
                    "route-completion-without-missed-eligible-shot") << '\n';
            observation.report("paint_route", -1);
        }

        void painting_opening(Services& s, WeaponType type, PlayerTeam team, size_t spawn)
        {
            const bool right = team == PlayerTeam::a;
            const float direction = right ? 1.0f : -1.0f;
            const auto points = right ? s.map.info.team_a_spawns() : s.map.info.team_b_spawns();
            require(points.size() == 4 && spawn < points.size(), "Crossfeed painting opening needs all four authored spawns");
            Run run(s, points[spawn], team, type);
            // Start the first live input at construction. Level does not tick
            // gameplay or weapon cooldown during its countdown. An idle
            // settling second here would artificially prime a new sniper.
            // Actual first contacts establish home support before Down drops.
            require(run.support() == (right ? "spawn_a_floor" : "spawn_b_floor") &&
                run.player->rectangle() == RectangleF(points[spawn], s.tuning.player().size),
                "Crossfeed painting opening did not begin at its complete authored spawn body over home");
            PaintingObservation observation(run, type, team);
            const float lower_y = s.map.named("floor_paint_a").rectangle.top();
            int landed_lower_tick = -1;
            const float angle = right ? PI / 3.0f : 2.0f * PI / 3.0f;
            for (int tick = 1; tick <= 180; ++tick)
            {
                // Change to walking as soon as real contact reaches the lower
                // floor. There is no fixed-duration idle/drop interval.
                if (landed_lower_tick < 0 && supported(*run.player) &&
                    approximately(run.player->rectangle().bottom(), lower_y))
                { landed_lower_tick = tick - 1; }
                PlayerInputData actions = aimed(angle,
                    input(landed_lower_tick >= 0 ? direction : 0.0f, false, false, landed_lower_tick < 0));
                actions.primary_shoot = observation.first_missed_tick < 0;
                observation.tick(actions, true);
            }
            const Vector2F opening_position = run.player->center();
            const Vector2F opening_velocity = run.player->velocity();
            const std::string opening_support = run.support();
            const float active_area = run.team_area(team);
            require(landed_lower_tick >= 0 && run.takeoffs == 0 &&
                (opening_position.x - (points[spawn].x + s.tuning.player().size.x / 2.0f)) * direction >
                    s.tuning.player().size.x,
                "Crossfeed painting opening did not drop and make actual jump-free walking progress");
            const int active_paid = run.paid_events;
            observation.released_recovery();
            require(run.paid_events == active_paid, "Crossfeed released opening paid for another shot");
            std::cout << "MEASURE Crossfeed paint_opening_progress tool=" << painting_tool_name(type) <<
                " team=" << (right ? 'a' : 'b') << " spawn=" << spawn <<
                " active_input_ticks=180 landed_lower_tick=" << landed_lower_tick <<
                " progress_centre_x=" << fixed(opening_position.x) << " progress_centre_y=" << fixed(opening_position.y) <<
                " progress_vx=" << fixed(opening_velocity.x) << " progress_vy=" << fixed(opening_velocity.y) <<
                " progress_support=" << opening_support << " active_unique_owned_score_area=" << fixed(active_area) <<
                " takeoffs=" << run.takeoffs << " fire_end_reason=" <<
                (observation.first_missed_tick >= 0 ? "first-missed-eligible-shot-before-active-window-end" :
                    "180-input-tick-window-end") << '\n';
            observation.report("paint_opening", static_cast<int>(spawn));
        }
    }

    void painting_checks(Services& s)
    {
        std::cout << "NOTE Crossfeed map painting: ten shared lower crossing/return assays and forty authored-spawn "
            "openings. Shared routes prepare their floor support with sixty shot-free ticks; openings begin at "
            "fresh construction with no idle settling or cooldown priming, matching Level's frozen countdown. "
            "All use actual Player movement and the same forward/down aim policy. Fire stops at the first "
            "cooldown-eligible opportunity that actually misses for lack of paint, or the active route/window end. "
            "An empty readiness state just after a paid shot is recorded separately; recovery can afford the next "
            "scheduled shot. A float cooldown clock follows all actual Run updates and paid resets, checked against "
            "public affordable readiness, without reading private state. Remaining movement continues, then fire "
            "releases while surviving shots settle and ammunition recovers. All score is contested geometry in this "
            "map; first paint locates its actual scored surface bounds and the player's simultaneous centre, not an "
            "inferred exact impact point. This is one scripted policy, not optimal opening time, whole-tank efficiency "
            "when ammunition remains, adversarial painting, human aim or competitive balance\n";
        for (const WeaponType type : { WeaponType::sprayer, WeaponType::sniper, WeaponType::roller,
            WeaponType::mister, WeaponType::bucket })
        {
            for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
            {
                shared_painting_route(s, type, team);
                for (size_t spawn = 0; spawn < 4; ++spawn) { painting_opening(s, type, team, spawn); }
            }
        }
        std::cout << "PASS Crossfeed actual shared painting routes and all authored-spawn opening observations\n";
    }
}
