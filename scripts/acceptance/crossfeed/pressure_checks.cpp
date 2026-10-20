#include "game/pch.h"
#include "scripts/acceptance/crossfeed/fixtures.h"

namespace crossfeed_acceptance
{
    namespace
    {
        constexpr int pressure_fire_ticks = 90;
        constexpr int pressure_trial_ticks = 150;

        enum class Departure { idle, lower, upper };

        const char* pressure_tool_name(WeaponType type)
        {
            switch (type)
            {
            case WeaponType::sprayer: return "sprayer";
            case WeaponType::sniper: return "sniper";
            case WeaponType::roller: return "roller";
            case WeaponType::mister: return "mister";
            case WeaponType::bucket: return "bucket";
            default: throw std::runtime_error("Crossfeed pressure needs a concrete primary");
            }
        }

        const char* departure_name(Departure departure)
        {
            switch (departure)
            {
            case Departure::idle: return "idle-authored-spawn";
            case Departure::lower: return "drop-then-lower-walk";
            case Departure::upper: return "upper-tap-held-approach";
            default: throw std::runtime_error("Crossfeed pressure needs a concrete departure");
            }
        }

        std::string pressure_support(const Run& run, const Player& player)
        {
            const RectangleF& body = player.rectangle();
            for (const auto& [surface, object] : run.structures)
            {
                (void)object;
                if (!surface->ramp() && approximately(body.bottom(), surface->rectangle.top()) &&
                    body.right() > surface->rectangle.left() && body.left() < surface->rectangle.right())
                {
                    return surface->name;
                }
            }
            return "air-or-ramp";
        }

        void pressure_target_clear(const Run& run, const Player& target)
        {
            const RectangleF body = target.rectangle();
            const RectangleF bounds = run.services.map.info.out_of_bounds_rectangle();
            require(std::isfinite(body.x) && std::isfinite(body.y) &&
                std::isfinite(target.velocity().x) && std::isfinite(target.velocity().y),
                "Crossfeed pressure target produced non-finite movement");
            require(body.left() >= bounds.left() - tolerance && body.right() <= bounds.right() + tolerance &&
                body.top() >= bounds.top() - tolerance && body.bottom() <= bounds.bottom() + tolerance,
                "Crossfeed pressure target left full-body bounds at tick " + std::to_string(run.ticks));
            for (const auto& [surface, object] : run.structures)
            {
                if (surface->jump_through()) { continue; }
                const auto overlap = narrow_phase(*target.shape(), *object->shape());
                require(!overlap || overlap->penetration <= tolerance,
                    "Crossfeed pressure target penetrated '" + surface->name + "' at tick " +
                    std::to_string(run.ticks));
            }
        }

        struct DepartureInput
        {
            Run& run;
            Player& target;
            Departure departure;
            bool target_right;
            int phase = 0;
            int jump_age = 0;
            int lower_input_ticks = 0;
            int upper_input_ticks = 0;

            float canonical_x(float world) const
            {
                const RectangleF bounds = this->run.services.map.info.out_of_bounds_rectangle();
                return this->target_right ? world : bounds.left() + bounds.right() - world;
            }

            float steer(float canonical_goal) const
            {
                const float goal = this->canonical_x(canonical_goal);
                const float error = goal - this->target.center().x;
                const float velocity = this->target.velocity().x;
                const float stopping = velocity * velocity /
                    (2.0f * this->run.services.tuning.player().x_deacceleration) + std::fabs(velocity) * dt + 4.0f;
                if (std::fabs(error) < 5.0f || (velocity != 0.0f &&
                    (velocity > 0.0f) == (error > 0.0f) && std::fabs(error) <= stopping)) { return 0.0f; }
                return error > 0.0f ? 1.0f : -1.0f;
            }

            PlayerInputData next()
            {
                if (this->target.state() != PlayerState::alive || this->target.health() <= 0.0f ||
                    this->departure == Departure::idle) { return input(); }
                const float direction = this->target_right ? 1.0f : -1.0f;
                if (this->departure == Departure::lower)
                {
                    ++this->lower_input_ticks;
                    const float floor_y = this->run.services.map.named("floor_paint_a").rectangle.top();
                    if (this->phase == 0 && supported(this->target) &&
                        approximately(this->target.rectangle().bottom(), floor_y)) { this->phase = 1; }
                    return input(this->phase == 0 ? 0.0f : direction, false, false, this->phase == 0);
                }

                ++this->upper_input_ticks;
                const auto side = [&](const char* a, const char* b) { return this->target_right ? a : b; };
                const std::string support = pressure_support(this->run, this->target);
                const auto stopped_at = [&](float goal)
                {
                    return std::fabs(this->canonical_x(this->target.center().x) - goal) <= 8.0f &&
                        this->target.velocity().x == 0.0f && supported(this->target);
                };
                if (this->phase == 0 && support == side("spawn_a_floor", "spawn_b_floor") && stopped_at(550.0f))
                { this->phase = 1; this->jump_age = 0; }
                if (this->phase == 1 && support == side("spawn_a_step", "spawn_b_step") && stopped_at(710.0f))
                { this->phase = 2; this->jump_age = 0; }
                if (this->phase == 2 && support == side("spawn_a_baffle", "spawn_b_baffle") && stopped_at(920.0f))
                { this->phase = 3; }
                const float goal = this->phase == 0 ? 550.0f : this->phase == 1 ? 710.0f :
                    this->phase == 2 ? 920.0f : 1200.0f;
                const bool jumping = this->phase == 1 || this->phase == 2;
                const bool press = jumping && this->jump_age == 0;
                const bool hold = jumping && (this->phase == 2 || this->jump_age == 0);
                if (jumping) { ++this->jump_age; }
                return input(this->steer(goal), press, hold);
            }
        };

        void pressure_trial(Services& s, WeaponType type, PlayerTeam team, const char* station,
            const char* surface_name, float attacker_x, size_t spawn, Departure departure)
        {
            const bool right = team == PlayerTeam::a;
            const Surface& surface = s.map.named(surface_name);
            Run run(s, standing_on(s, surface, attacker_x), team, type);
            // Establish fixed-tool facing by actual input, then let that small
            // orientation step brake before creating the authored spawn body.
            run.tick(input(right ? 1.0f : -1.0f));
            run.settle();
            require(supported(*run.player) && run.support() == surface.name,
                std::string("Crossfeed pressure attacker missed station '") + station + "'");
            const auto points = right ? s.map.info.team_b_spawns() : s.map.info.team_a_spawns();
            require(points.size() == 4 && spawn < points.size(), "Crossfeed pressure needs an authored opposing spawn");
            Player* target = run.add_opponent(points[spawn]);
            run.settle();
            pressure_target_clear(run, *target);
            require(supported(*target) && target->rectangle() == RectangleF(points[spawn], s.tuning.player().size),
                "Crossfeed pressure target did not settle at its complete authored spawn body");
            DepartureInput driver{ run, *target, departure, !right };
            run.set_opponent_input(target, [&driver](int) { return driver.next(); });
            run.weapon_aim_after_movement = [&run, target, right]
            {
                const Vector2F offset = run.services.tuning.weapon(run.player->primary_weapon().type()).details.offset;
                const Vector2F origin = run.player->center() + Vector2F(right ? offset.x : -offset.x, offset.y);
                const Vector2F to_target = target->center() - origin;
                return std::atan2(to_target.y, to_target.x);
            };
            const Vector2F start = target->center();
            int first_damage = -1, lethal = -1, dead_state = -1, release = -1, takeoffs = 0;
            float damage = 0.0f, max_speed = 0.0f, minimum_feet = target->rectangle().bottom();
            float release_ammo = -1.0f, release_plus_60 = -1.0f;
            for (int tick = 1; tick <= pressure_trial_ticks; ++tick)
            {
                const bool fire = tick <= pressure_fire_ticks && lethal < 0;
                if (!fire && release < 0) { release = tick - 1; release_ammo = run.player->weapon_ammo(); }
                PlayerInputData actions = input(); actions.primary_shoot = fire;
                const float before = target->health();
                run.tick(actions);
                pressure_target_clear(run, *target);
                const float received = std::max(0.0f, before - target->health());
                damage += received;
                if (received > 0.0f && first_damage < 0) { first_damage = tick; }
                if (target->health() <= 0.0f && lethal < 0) { lethal = tick; }
                if (target->state() == PlayerState::dead && dead_state < 0) { dead_state = tick; }
                max_speed = std::max(max_speed, std::fabs(target->velocity().x));
                minimum_feet = std::min(minimum_feet, target->rectangle().bottom());
                for (const auto& effect : target->movement_feedback().effects())
                { if (effect.age == 0.0f && effect.kind == MovementFeedbackKind::takeoff) { ++takeoffs; } }
                if (release >= 0 && tick - release == 60) { release_plus_60 = run.player->weapon_ammo(); }
            }
            require(run.paid_events > 0, "Crossfeed pressure trial never paid for its primary");
            require(run.team_area(target->team()) == 0.0f, "Crossfeed pressure target's idle weapon unexpectedly painted");
            if (departure == Departure::idle)
            {
                require(approximately(target->center().x, start.x), "Crossfeed idle spawn pressure control moved horizontally");
            }
            std::cout << "MEASURE Crossfeed pressure station=" << station << " tool=" << pressure_tool_name(type) <<
                " attacker_team=" << (right ? 'a' : 'b') << " target_spawn=" << spawn <<
                " target_input=" << departure_name(departure) <<
                " aim=" << (type == WeaponType::roller ? "fixed-forward-down" : "scripted-current-centre-tracking") <<
                " lead=none ballistic_compensation=none total_ticks=" << pressure_trial_ticks <<
                " attacker_x=" << fixed(run.player->center().x) << " attacker_y=" << fixed(run.player->center().y) <<
                " target_start_x=" << fixed(start.x) << " target_start_y=" << fixed(start.y) <<
                " target_end_x=" << fixed(target->center().x) << " target_end_y=" << fixed(target->center().y) <<
                " target_max_abs_vx=" << fixed(max_speed) << " target_min_feet_y=" << fixed(minimum_feet) <<
                " target_end_support=" << pressure_support(run, *target) << " departure_phase=" << driver.phase <<
                " drop_walk_input_ticks=" << driver.lower_input_ticks << " upper_input_ticks=" << driver.upper_input_ticks <<
                " target_takeoffs=" << takeoffs << " first_damage_tick=" << first_damage <<
                " lethal_health_tick=" << lethal << " dead_state_tick=" << dead_state << " damage=" << fixed(damage, 4) <<
                " final_health=" << fixed(target->health(), 4) << " paid_events=" << run.paid_events <<
                " emitted_survivors=" << run.emitted_objects << " launch_retired_objects=" << run.launch_retired_objects <<
                " swept_retired_objects=" << run.swept_retired_objects << " release_tick=" << release <<
                " release_ammo=" << fixed(release_ammo, 4) << " release_plus_60_ticks_ammo=" << fixed(release_plus_60, 4) <<
                " painted_area=" << fixed(run.team_area(team)) <<
                " target_full_body_clear_each_step=1 scope=sampled-spawn-exit-threat-no-zero-damage-gate\n";
        }
    }

    void pressure_checks(Services& s)
    {
        require(static_cast<float>(pressure_trial_ticks) * dt < s.tuning.player().respawn_delay,
            "Crossfeed pressure window must finish before its target can respawn");
        const RectangleF bounds = s.map.info.out_of_bounds_rectangle();
        struct Station { const char* name; const char* a; const char* b; float x; };
        constexpr std::array<Station, 5> stations{
            Station{ "mixer-top", "centre_mixer", "centre_mixer", 1800.0f },
            Station{ "enemy-upper-ruler", "upper_step_b", "upper_step_a", 2070.0f },
            Station{ "enemy-side-bench", "side_paint_b", "side_paint_a", 2400.0f },
            Station{ "enemy-home-baffle", "spawn_b_baffle", "spawn_a_baffle", 2680.0f },
            Station{ "enemy-lower-approach", "floor_paint_b", "floor_paint_a", 2450.0f } };
        size_t idle_trials = 0, departure_trials = 0;
        std::cout << "NOTE Crossfeed pressure: five selected stationary attacker positions at the centre and opponent-side "
            "ruler/bench/home-baffle/lower approach, all five primaries, "
            "all four opposing authored spawn bodies and mirrored teams. Two actual-input departures sample "
            "the nearest-exit spawn only. Straight current-centre tracking has no projectile lead or ballistic "
            "compensation; zero damage is not a safety guarantee and damage is not a balance failure. "
            "Lower phases:0=drop,1=walk; upper phases:0=home approach,1=step tap,2=baffle held,3=bench approach\n";
        for (const Station& station : stations)
        {
            for (const WeaponType type : { WeaponType::sprayer, WeaponType::sniper, WeaponType::roller,
                WeaponType::mister, WeaponType::bucket })
            {
                for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
                {
                    const bool right = team == PlayerTeam::a;
                    const float x = right ? station.x : bounds.left() + bounds.right() - station.x;
                    for (size_t spawn = 0; spawn < 4; ++spawn)
                    {
                        pressure_trial(s, type, team, station.name, right ? station.a : station.b, x, spawn, Departure::idle);
                        ++idle_trials;
                    }
                    for (const Departure departure : { Departure::lower, Departure::upper })
                    {
                        pressure_trial(s, type, team, station.name, right ? station.a : station.b, x, 3, departure);
                        ++departure_trials;
                    }
                }
            }
        }
        require(idle_trials == 200 && departure_trials == 100, "Crossfeed pressure matrix changed its bounded sample count");
        std::cout << "PASS Crossfeed sampled pressure observations: 200 authored-spawn controls and 100 representative "
            "actual-input departures; target bounds and solid clearance checked each step. Unobserved aim, attacker "
            "movement, all-spawn departure combinations and attended pressure/balance remain open\n";
    }
}
