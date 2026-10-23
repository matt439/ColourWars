#include "game/pch.h"
#include "scripts/acceptance/crossfeed/fixtures.h"
#include "game/objects/structure_paintable.h"
#include <cstdlib>

namespace crossfeed_acceptance
{
    namespace
    {
        const char* repaint_tool_name(WeaponType type)
        {
            switch (type)
            {
            case WeaponType::sprayer: return "sprayer";
            case WeaponType::sniper: return "sniper";
            case WeaponType::roller: return "roller";
            case WeaponType::mister: return "mister";
            case WeaponType::bucket: return "bucket";
            default: throw std::runtime_error("Crossfeed repaint needs a concrete primary");
            }
        }

        PlayerTeam opposing_team(PlayerTeam team)
        {
            return team == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a;
        }

        void seed_paint(Run& run, PlayerTeam initial)
        {
            if (initial == PlayerTeam::none) { return; }
            for (const auto& [surface, object] : run.structures)
            {
                (void)surface;
                if (auto* paintable = dynamic_cast<StructurePaintable*>(object))
                {
                    // Establish a controlled initial condition through public
                    // contacts, as in the native heavy-paint fixture. These
                    // oversized shots do not claim reachable weapon coverage.
                    RectangleF splash = object->shape()->bounding_box();
                    splash.inflate(32.0f);
                    splash.offset(run.services.tuning.projectile(ProjectileType::spray).details.col_rect_size / 2.0f);
                    const ProjectileSpray shot(splash, Vector2F::ZERO, initial, 1, Colour::white,
                        &run.services.render, resolve_projectile_strip(
                            run.services.tuning.projectile(ProjectileType::spray), run.services.render),
                        &run.services.tuning.projectile(ProjectileType::spray));
                    paintable->on_contact(shot, Vector2F::ZERO, 0.0f);
                }
            }
            require(approximately(run.team_area(initial), run.total_area()),
                "Crossfeed repaint seed did not cover every actual runtime scoring tile");
        }

        float steer(const Run& run, float goal)
        {
            const float error = goal - run.player->center().x;
            const float velocity = run.player->velocity().x;
            const float stopping = velocity * velocity /
                (2.0f * run.services.tuning.player().x_deacceleration) + std::fabs(velocity) * dt + 4.0f;
            if (std::fabs(error) < 5.0f || (velocity != 0.0f &&
                (velocity > 0.0f) == (error > 0.0f) && std::fabs(error) <= stopping)) { return 0.0f; }
            return error > 0.0f ? 1.0f : -1.0f;
        }

        bool arrived(const Run& run, float goal)
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

        struct RepaintResult
        {
            int outward_tick = -1;
            int return_tick = -1;
            int first_gain_tick = -1;
            int flight_finished_tick = -1;
            int refill_tick = -1;
            int outward_paid = 0;
            float outward_gain = 0.0f;
            float return_gain = 0.0f;
            float settled_gain = 0.0f;
            float enemy_loss = 0.0f;
            float neutral_loss = 0.0f;
            float release_ammo = 0.0f;
            // Every active input and resulting public centre/velocity, so
            // matched endpoint times cannot conceal a different route.
            std::vector<std::array<float, 5>> trajectory;
            std::vector<int> paid_ticks;
            int emitted = 0;
            int launch_retired = 0;
            int swept_retired = 0;
        };

        RepaintResult repaint_route(Services& s, WeaponType type, PlayerTeam team,
            PlayerTeam initial, bool fire)
        {
            const bool right = team == PlayerTeam::a;
            const RectangleF bounds = s.map.info.out_of_bounds_rectangle();
            const auto x = [&](float value) { return right ? value : bounds.left() + bounds.right() - value; };
            Run run(s, standing_on(s, s.map.named(right ? "floor_paint_a" : "floor_paint_b"), x(1150.0f)), team, type);
            run.settle();
            seed_paint(run, initial);
            // Initial paint can trigger cosmetic work. Seed immediately before
            // identical active inputs, so each ownership condition shares RNG.
            std::srand(2401026);
            const float total = run.total_area();
            const float owned_before = run.team_area(team);
            const float enemy_before = run.team_area(opposing_team(team));
            const float neutral_before = total - owned_before - enemy_before;
            const int start = run.ticks;
            RepaintResult result;
            const auto observe = [&]
            {
                const float owned = run.team_area(team), enemy = run.team_area(opposing_team(team));
                const float gain = owned - owned_before;
                const float enemy_loss = enemy_before - enemy;
                const float neutral = total - owned - enemy;
                const float neutral_loss = neutral_before - neutral;
                require(owned >= -tolerance && enemy >= -tolerance && neutral >= -tolerance &&
                    gain >= -tolerance && enemy_loss >= -tolerance && neutral_loss >= -tolerance &&
                    approximately(gain, enemy_loss + neutral_loss) && approximately(run.total_area(), total),
                    "Crossfeed paid repaint did not conserve neutral/owned scoring area");
                if (gain > 0.0f && result.first_gain_tick < 0) { result.first_gain_tick = run.ticks - start; }
                result.settled_gain = gain;
                result.enemy_loss = enemy_loss;
                result.neutral_loss = neutral_loss;
            };
            int phase = 0;
            for (int tick = 1; tick <= 1800; ++tick)
            {
                const float goal = x(phase == 0 ? 2450.0f : 1150.0f);
                const float direction = (right != (phase == 1)) ? 1.0f : -1.0f;
                PlayerInputData actions = aimed(direction > 0.0f ? PI / 3.0f : 2.0f * PI / 3.0f,
                    input(steer(run, goal)));
                actions.primary_shoot = fire;
                run.tick(actions);
                result.trajectory.push_back({ actions.x_movement, run.player->center().x, run.player->center().y,
                    run.player->velocity().x, run.player->velocity().y });
                observe();
                if (!arrived(run, goal)) { continue; }
                if (phase == 0)
                {
                    result.outward_tick = tick;
                    result.outward_paid = run.paid_events;
                    result.outward_gain = result.settled_gain;
                    phase = 1;
                }
                else { result.return_tick = tick; result.return_gain = result.settled_gain; break; }
            }
            require(result.outward_tick > 0 && result.return_tick > result.outward_tick && run.takeoffs == 0,
                "Crossfeed repaint did not complete its actual jump-free crossing and stopped return");
            result.release_ammo = run.player->weapon_ammo();
            const int release_paid = run.paid_events;
            if (flying_objects(run) == 0) { result.flight_finished_tick = 0; }
            if (result.release_ammo >= s.tuning.starting_ammo()) { result.refill_tick = 0; }
            for (int tick = 1; tick <= 720; ++tick)
            {
                run.tick(input());
                observe();
                require(run.paid_events == release_paid, "Crossfeed released repaint route paid another shot");
                if (result.flight_finished_tick < 0 && flying_objects(run) == 0) { result.flight_finished_tick = tick; }
                if (result.refill_tick < 0 && run.player->weapon_ammo() >= s.tuning.starting_ammo())
                { result.refill_tick = tick; }
                if (result.flight_finished_tick >= 0 && result.refill_tick >= 0) { break; }
            }
            require(result.flight_finished_tick >= 0 && result.refill_tick >= 0 && run.takeoffs == 0 &&
                arrived(run, x(1150.0f)), "Crossfeed repaint did not settle its flight/refill at the return support");
            for (int paid : run.paid_ticks) { result.paid_ticks.push_back(paid - start); }
            result.emitted = run.emitted_objects;
            result.launch_retired = run.launch_retired_objects;
            result.swept_retired = run.swept_retired_objects;
            if (!fire)
            {
                require(result.paid_ticks.empty() && result.first_gain_tick < 0 && result.settled_gain == 0.0f,
                    "Crossfeed repaint no-fire control gained ownership or paid ammunition");
            }
            else if (initial == team)
            {
                require(!result.paid_ticks.empty() && result.first_gain_tick < 0 && result.settled_gain == 0.0f,
                    "Crossfeed friendly repaint falsely added unique team ownership");
            }
            else
            {
                require(!result.paid_ticks.empty() && result.first_gain_tick > 0 && result.settled_gain > 0.0f,
                    "Crossfeed paid route failed to gain actual neutral/enemy paint");
            }
            const char* condition = initial == PlayerTeam::none ? "neutral" : initial == team ? "friendly" : "opposing";
            std::cout << "MEASURE Crossfeed repaint_route tool=" << repaint_tool_name(type) <<
                " team=" << (right ? 'a' : 'b') << " initial=" << condition << " fire=" << fire <<
                " outward_tick=" << result.outward_tick << " returned_tick=" << result.return_tick <<
                " first_gain_tick=" << result.first_gain_tick << " outward_paid=" << result.outward_paid <<
                " paid_events=" << result.paid_ticks.size() <<
                " paid_cost=" << fixed(static_cast<float>(result.paid_ticks.size()) * s.tuning.weapon(type).details.ammo_usage, 6) <<
                " release_ammo=" << fixed(result.release_ammo, 6) << " outward_owned_gain=" << fixed(result.outward_gain) <<
                " return_additional_gain=" << fixed(result.return_gain - result.outward_gain) <<
                " post_release_gain=" << fixed(result.settled_gain - result.return_gain) <<
                " settled_owned_gain=" << fixed(result.settled_gain) << " enemy_area_lost=" << fixed(result.enemy_loss) <<
                " neutral_area_lost=" << fixed(result.neutral_loss) <<
                " flight_finished_after_release_tick=" << result.flight_finished_tick <<
                " full_refill_after_release_tick=" << result.refill_tick <<
                " scope=held-fire-fixed-route-controlled-initial-ownership-no-live-opponent\n";
            return result;
        }

        void require_same_route_and_payments(const RepaintResult& first, const RepaintResult& second)
        {
            require(first.outward_tick == second.outward_tick && first.return_tick == second.return_tick &&
                first.trajectory == second.trajectory &&
                first.paid_ticks == second.paid_ticks && first.release_ammo == second.release_ammo &&
                first.emitted == second.emitted && first.launch_retired == second.launch_retired &&
                first.swept_retired == second.swept_retired && first.flight_finished_tick == second.flight_finished_tick &&
                first.refill_tick == second.refill_tick,
                "Crossfeed initial paint ownership changed the matched route, payment or projectile completion");
        }
    }

    void repaint_checks(Services& s)
    {
        std::cout << "NOTE Crossfeed repaint compares a fixed held-fire 1150/2450 crossing and return on clean, "
            "fully friendly and fully opposing initial scoring surfaces. Controlled seed contacts establish an "
            "initial condition, not accessible coverage. Every subsequent ownership change comes from real paid "
            "Player shots. Held fire can recover and resume; costs are cumulative paid expenditure, not starting "
            "tank capacity. Endpoint snapshots include flight still in progress; final paint waits for all shots "
            "to retire. No live opponent, safe-loop dominance, optimal policy or human balance is established\n";
        for (const WeaponType type : { WeaponType::sprayer, WeaponType::sniper, WeaponType::roller,
            WeaponType::mister, WeaponType::bucket })
        {
            for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
            {
                const RepaintResult clean = repaint_route(s, type, team, PlayerTeam::none, true);
                const RepaintResult friendly = repaint_route(s, type, team, team, true);
                const RepaintResult opposing = repaint_route(s, type, team, opposing_team(team), true);
                const RepaintResult no_fire = repaint_route(s, type, team, opposing_team(team), false);
                require_same_route_and_payments(clean, friendly);
                require_same_route_and_payments(clean, opposing);
                require(approximately(clean.settled_gain, opposing.settled_gain) &&
                    approximately(clean.outward_gain, opposing.outward_gain) &&
                    approximately(clean.return_gain, opposing.return_gain) &&
                    approximately(clean.neutral_loss, clean.settled_gain) && clean.enemy_loss == 0.0f &&
                    approximately(opposing.enemy_loss, opposing.settled_gain) && opposing.neutral_loss == 0.0f &&
                    clean.first_gain_tick == opposing.first_gain_tick && clean.outward_tick == no_fire.outward_tick &&
                    clean.return_tick == no_fire.return_tick && clean.trajectory == no_fire.trajectory,
                    "Crossfeed matched neutral capture and opposing repaint failed their ownership/control comparison");
            }
        }
        std::cout << "PASS Crossfeed five-tool mirrored paid repaint, friendly and no-fire controls\n";
    }
}
