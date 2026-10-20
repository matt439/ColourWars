#include "game/pch.h"
#include "scripts/acceptance/crossfeed/fixtures.h"

namespace crossfeed_acceptance
{
    namespace
    {
        const char* tool_name(WeaponType type)
        {
            switch (type)
            {
            case WeaponType::sprayer: return "sprayer";
            case WeaponType::sniper: return "sniper";
            case WeaponType::roller: return "roller";
            case WeaponType::mister: return "mister";
            case WeaponType::bucket: return "bucket";
            default: throw std::runtime_error("Crossfeed station needs a concrete primary");
            }
        }

        void launch_checks(Services& s)
        {
            // Injected velocity isolates launch inheritance from route/control
            // feasibility. Observe public collision-rectangle displacement
            // over the first fixed step, including gravity/drag, not ranges.
            for (const WeaponType type : { WeaponType::sprayer, WeaponType::sniper, WeaponType::roller,
                WeaponType::mister, WeaponType::bucket })
            {
                for (const bool right : { true, false })
                {
                    for (const float speed : { 0.0f, 400.0f, 800.0f })
                    {
                        for (const bool retreat : { false, true })
                        {
                            const float angle = (right != retreat) ? 0.0f : PI;
                            const Vector2F injected(right ? speed : -speed, 0.0f);
                            const Vector2F origin(right ? 1250.0f : 2350.0f, 1220.0f);
                            auto weapon = WeaponBuilder::build_weapon(&s.tuning, right ? PlayerTeam::a : PlayerTeam::b,
                                0, 0, Colour::white, type, origin, &s.render, &s.audio, &s.preferences,
                                &s.presentation.tool_motion(), &s.presentation.weapon_feedback());
                            for (int tick = 0; tick < 120 && !weapon->ready_to_shoot(); ++tick)
                            { weapon->update_and_get_projectiles(aimed(angle), origin, injected, right, dt); }
                            PlayerInputData actions = aimed(angle);
                            actions.primary_shoot = true;
                            const float before = weapon->ammo();
                            auto objects = weapon->update_and_get_projectiles(actions, origin, injected, right, dt);
                            require(weapon->ammo() < before && !objects.empty(),
                                "Crossfeed direct launch station did not pay and emit projectiles");
                            float min_x = 0.0f, max_x = 0.0f, min_y = 0.0f, max_y = 0.0f;
                            bool first = true;
                            int opposite_aim = 0;
                            for (const auto& object : objects)
                            {
                                const Vector2F before_step = object->shape()->bounding_box().center();
                                object->update(dt);
                                const Vector2F v = (object->shape()->bounding_box().center() - before_step) / dt;
                                if (first) { min_x = max_x = v.x; min_y = max_y = v.y; first = false; }
                                min_x = std::min(min_x, v.x); max_x = std::max(max_x, v.x);
                                min_y = std::min(min_y, v.y); max_y = std::max(max_y, v.y);
                                opposite_aim += v.x * std::cos(angle) + v.y * std::sin(angle) < -tolerance ? 1 : 0;
                            }
                            std::cout << "MEASURE Crossfeed injected_launch tool=" << tool_name(type) <<
                                " team=" << (right ? 'a' : 'b') << " player_vx=" << fixed(injected.x) <<
                                " aim_family=" << (retreat ? "retreat" : "forward") << " aim=" << fixed(angle, 3) <<
                                " paid_events=1 emitted_objects=" << objects.size() <<
                                " first_step_vx_min=" << fixed(min_x) << " first_step_vx_max=" << fixed(max_x) <<
                                " first_step_vy_min=" << fixed(min_y) << " first_step_vy_max=" << fixed(max_y) <<
                                " opposite_requested_aim=" << opposite_aim <<
                                " scope=injected-player-velocity-first-fixed-step-displacement-includes-gravity-drag-no-collision-or-range\n";
                        }
                    }
                }
            }
        }
    }

    void tool_checks(Services& s)
    {
        launch_checks(s);
        const RectangleF bounds = s.map.info.out_of_bounds_rectangle();
        const auto reflected = [&](float value, bool right)
            { return right ? value : bounds.left() + bounds.right() - value; };
        for (const WeaponType type : { WeaponType::sprayer, WeaponType::sniper, WeaponType::roller,
            WeaponType::mister, WeaponType::bucket })
        {
            for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
            {
                const bool right = team == PlayerTeam::a;
                const Surface& floor = s.map.named(right ? "floor_paint_a" : "floor_paint_b");
                Run run(s, standing_on(s, floor, reflected(1050.0f, right)), team, type);
                run.settle();
                const float initial = run.player->weapon_ammo();
                const float angle = right ? PI / 3.0f : 2.0f * PI / 3.0f;
                std::array<float, 4> recovery{};
                for (int tick = 0; tick < 240; ++tick)
                {
                    PlayerInputData actions = aimed(angle, input(tick < 12 ? (right ? 1.0f : -1.0f) : 0.0f));
                    actions.primary_shoot = tick < 90;
                    run.tick(actions);
                    if (tick == 89) { recovery[0] = run.player->weapon_ammo(); }
                    if (tick == 119) { recovery[1] = run.player->weapon_ammo(); }
                    if (tick == 149) { recovery[2] = run.player->weapon_ammo(); }
                    if (tick == 209) { recovery[3] = run.player->weapon_ammo(); }
                }
                const float area = run.team_area(team, floor.name);
                require(run.paid_events > 0 && run.minimum_ammo < initial && area > 0.0f,
                    std::string("Crossfeed ") + tool_name(type) + " did not pay for paint on its connected floor");
                require(run.team_area(right ? PlayerTeam::b : PlayerTeam::a) == 0.0f,
                    "Crossfeed tool station assigned paint to the wrong team");
                std::cout << "MEASURE Crossfeed contact tool=" << tool_name(type) << " team=" << (right ? 'a' : 'b') <<
                    " floor_area=" << fixed(area) << " minimum_ammo=" << fixed(run.minimum_ammo, 4) <<
                    " paid_events=" << run.paid_events << " emitted_objects=" << run.emitted_objects <<
                    " paid_events_without_surviving_flight=" << run.paid_events_without_flight <<
                    " launch_paint=" << fixed(run.launch_paint) << " swept_paint=" << fixed(run.swept_paint) <<
                    " release_ammo=" << fixed(recovery[0], 4) << " release_plus_0.5s=" << fixed(recovery[1], 4) <<
                    " release_plus_1s=" << fixed(recovery[2], 4) << " release_plus_2s=" << fixed(recovery[3], 4) << '\n';
            }
        }

        // Four aimed tools have a separate actual-flight station. Their shots
        // cross the traversable ramp and stop on the authored plinth's solid
        // vertical face. That face is deliberately unscored: require its real
        // blocked-contact receipt, not just paid ammunition or paint elsewhere.
        const Surface& plinth = s.map.named("centre_plinth");
        for (const WeaponType type : { WeaponType::sprayer, WeaponType::sniper, WeaponType::mister, WeaponType::bucket })
        {
            for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
            {
                const bool right = team == PlayerTeam::a;
                const Surface& floor = s.map.named(right ? "floor_paint_a" : "floor_paint_b");
                const Surface& far_floor = s.map.named(right ? "floor_paint_b" : "floor_paint_a");
                for (const bool moving : { false, true })
                {
                    Run run(s, standing_on(s, floor, reflected(moving ? 1200.0f : 1250.0f, right)), team, type);
                    Player* protected_player = run.add_opponent(standing_on(s, far_floor, reflected(2350.0f, right)));
                    run.settle();
                    const float forward = type == WeaponType::bucket ? -0.4f :
                        type == WeaponType::sprayer ? -0.25f : 0.0f;
                    const float angle = right ? forward : PI - forward;
                    for (int tick = 0; moving && tick < 12; ++tick)
                    { run.tick(aimed(angle, input(right ? 1.0f : -1.0f))); }
                    run.tick(aimed(angle, input(moving ? (right ? 1.0f : -1.0f) : 0.0f)));
                    const Vector2F half = s.tuning.projectile(s.tuning.weapon(type).details.proj_type).details.col_rect_size / 2.0f;
                    const float initial = run.player->weapon_ammo();
                    run.interception_surface = plinth.name;
                    for (int tick = 0; tick < 240; ++tick)
                    {
                        PlayerInputData actions = aimed(angle);
                        actions.primary_shoot = tick < 90;
                        run.tick(actions);
                    }
                    const Vector2F muzzle = run.first_paid_nozzle;
                    const float gap = right ? plinth.rectangle.left() - muzzle.x - half.x :
                        muzzle.x - half.x - plinth.rectangle.right();
                    require(run.paid_events > 0 && run.minimum_ammo < initial && run.emitted_objects > 0 && run.cover_contacts > 0 &&
                        gap > tolerance && run.first_cover_tick > run.first_paid_tick, std::string("Crossfeed ") + tool_name(type) +
                        " did not reach the plinth by paid flight across the ramp");
                    require(approximately(protected_player->health(), s.tuning.player().starting_health),
                        "Crossfeed lower firing station damaged the opponent behind its solid plinth");
                    std::cout << "MEASURE Crossfeed flight tool=" << tool_name(type) << " team=" << (right ? 'a' : 'b') <<
                        " movement=" << (moving ? "moving-launch" : "stationary") << " first_paid_vx=" << fixed(run.first_paid_velocity.x) <<
                        " first_paid_nozzle_x=" << fixed(muzzle.x) << " first_paid_nozzle_y=" << fixed(muzzle.y) <<
                        " first_paid_gap=" << fixed(gap) << " aim=" << fixed(angle, 3) <<
                        " first_paid_to_cover_seconds=" << fixed(static_cast<float>(run.first_cover_tick - run.first_paid_tick) * dt, 4) <<
                        " contact_x=" << fixed(run.first_cover_point.x) << " contact_y=" << fixed(run.first_cover_point.y) <<
                        " intercepted=" << run.cover_contacts << " protected_health=" << fixed(protected_player->health()) << '\n';
                }
            }
        }
        std::cout << "PASS Crossfeed five tools paint paid floor contacts from both teams; four aimed tools reach solid "
            "lower cover by stationary/moving launch across paint-pass ramps without damaging the opponent behind it. "
            "Station results do not establish moving-target counterplay, whole-tank coverage or balance\n";
    }
}
