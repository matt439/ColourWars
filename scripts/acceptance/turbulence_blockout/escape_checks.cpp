#include "game/pch.h"
#include "scripts/acceptance/turbulence_blockout/tool_fixtures.h"
#include <unordered_set>

namespace turbulence_blockout_acceptance
{
    namespace
    {
        enum class EscapePolicy { idle, retreat, jump_retreat };
        constexpr std::array<EscapePolicy, 3> policies{ EscapePolicy::idle, EscapePolicy::retreat, EscapePolicy::jump_retreat };
        const char* policy_name(EscapePolicy policy)
        {
            return policy == EscapePolicy::idle ? "idle" : policy == EscapePolicy::retreat ? "retreat" : "jump_retreat";
        }

        std::string support_of(const Run& run, const Player& player)
        {
            const RectangleF& body = player.rectangle();
            for (const auto& [surface, object] : run.structures)
            {
                (void)object;
                if (!surface->ramp() && std::fabs(body.bottom() - surface->rectangle.top()) <= tolerance &&
                    body.right() > surface->rectangle.left() && body.left() < surface->rectangle.right())
                { return surface->name; }
            }
            return "air-or-ramp";
        }

        struct EscapeResult
        {
            RectangleF source;
            RectangleF initial_target;
            RectangleF core_target;
            RectangleF final_target;
            float initial_health = 0.0f;
            float core_health = 0.0f;
            float final_health = 0.0f;
            float initial_ammo = 0.0f;
            float pulse_ammo = 0.0f;
            float final_ammo = 0.0f;
            float angle = 0.0f;
            int warmup = 0;
            int tail = 0;
            int first_launch = -1;
            int first_observed_box_overlap = -1;
            int first_damage = -1;
            bool launch_damage = false;
            int launches = 0;
            int target_takeoffs = 0;
            bool core_supported = false;
            bool final_supported = false;
            std::string core_support;
            std::string final_support;
            int updates = 0;
        };

        EscapeResult escape_trial(Services& services, WeaponType tool, bool right, bool upper, EscapePolicy policy)
        {
            // Each policy starts from a fresh scene and the same seed. Only
            // defender input changes; the attacker never tracks a moved target.
            std::srand(260026u);
            const PlayerTeam attacker = right ? PlayerTeam::a : PlayerTeam::b;
            const PlayerTeam defender = right ? PlayerTeam::b : PlayerTeam::a;
            const std::string source = std::string(upper ? "crossover" : "lower_bed") + (right ? "_a" : "_b");
            const std::string receiver = upper ? source : "ground";
            Run run(services, standing(services, services.named(source), reflected(right, upper ? 1670.0f : 1420.0f)),
                attacker, 0.0f, tool);
            Player& target = run.add_target(standing(services, services.named(receiver), reflected(right, upper ? 1840.0f : 1780.0f)),
                defender);
            const RectangleF constructed_target = target.rectangle();
            const float constructed_health = target.health();
            run.settle();
            int ready_wait = 0;
            while (run.player->primary_weapon().readiness() != WeaponReadiness::ready && ready_wait++ < 240) { run.tick(); }
            require(run.player->primary_weapon().readiness() == WeaponReadiness::ready &&
                target.health() == constructed_health && target.rectangle() == constructed_target && supported(target),
                "Unfired escape baseline changed target health/position or never became ready");
            EscapeResult result;
            result.source = run.player->rectangle(); result.initial_target = target.rectangle();
            result.initial_health = target.health(); result.initial_ammo = run.player->weapon_ammo();
            result.warmup = run.ticks;
            if (tool == WeaponType::sniper) { result.angle = (target.center() - run.player->center()).angle(); }
            else
            {
                const float angle = tool == WeaponType::bucket ? -PI / 6.0f : 0.0f;
                result.angle = right ? angle : PI - angle;
            }
            const int epoch = run.ticks;
            int local_update = 0;
            std::unordered_set<const CollisionObject*> observed;
            const auto inspect_box = [&](const CollisionObject& projectile)
            {
                // These are actual launch/post-contact poses, not a continuous
                // sweep: retired or swept-through contacts may have no sampled
                // overlap. The separate real health event is authoritative.
                if (result.first_observed_box_overlap < 0 && narrow_phase(*projectile.shape(), *target.shape()))
                { result.first_observed_box_overlap = local_update; }
            };
            run.on_weapon_launch = [&](std::span<const std::unique_ptr<Projectile>> emitted)
            {
                if (local_update == 1 && target.health() < result.initial_health)
                { result.launch_damage = true; }
                for (const auto& projectile : emitted)
                {
                    require(local_update == 1 && run.paid_events == 1,
                        "Single-pulse escape observer received a later or unpaid launch");
                    require(run.player->team() == attacker && projectile->player_num() == run.player->player_num() &&
                        observed.insert(projectile.get()).second, "Escape flight lost owner/team/unique birth provenance");
                    if (result.first_launch < 0) { result.first_launch = local_update; }
                    ++result.launches; inspect_box(*projectile);
                }
            };
            run.after_scene_contacts = [&]
            {
                for (const CollisionObject* projectile : run.scene.collision_objects())
                {
                    if (!is_projectile(to_collision_type(projectile->tag()))) { continue; }
                    require(observed.contains(projectile), "Escape flight did not originate from the observed paid pulse");
                    inspect_box(*projectile);
                }
                if (target.health() < result.initial_health && result.first_damage < 0)
                { result.first_damage = local_update; }
            };
            const auto advance = [&](bool core)
            {
                ++local_update;
                const bool moving = core && policy != EscapePolicy::idle;
                const bool jumping = core && policy == EscapePolicy::jump_retreat;
                run.target_input = input(moving ? (right ? 1.0f : -1.0f) : 0.0f,
                    jumping && local_update == 1, jumping && local_update <= 30);
                run.tick(firing(result.angle, 0.0f, local_update == 1));
                if (local_update == 1) { result.pulse_ammo = run.player->weapon_ammo(); }
                require(run.paid_events == 1 && run.player->rectangle() == result.source &&
                    target.health() > 0.0f && target.state() == PlayerState::alive,
                    "Escape trial repeated its paid shot, moved the attacker or continued through elimination");
                require(services.info.out_of_bounds_rectangle().contains(target.rectangle()),
                    "Moving defender body left the authored bounds");
                for (const auto& effect : target.movement_feedback().effects())
                { if (effect.age == 0.0f && effect.kind == MovementFeedbackKind::takeoff) { ++result.target_takeoffs; } }
            };
            for (int update = 0; update < 180; ++update) { advance(true); }
            result.core_target = target.rectangle(); result.core_health = target.health();
            result.core_supported = supported(target); result.core_support = support_of(run, target);
            while (flying(run) && result.tail < 180) { advance(false); ++result.tail; }
            require(!flying(run), "Single-pulse escape flight exceeded its bounded observation/tail");
            result.final_target = target.rectangle(); result.final_health = target.health();
            result.final_supported = supported(target); result.final_support = support_of(run, target);
            result.final_ammo = run.player->weapon_ammo(); result.updates = run.ticks;
            require(run.ticks == epoch + 180 + result.tail && result.pulse_ammo < result.initial_ammo &&
                result.final_ammo >= result.pulse_ammo, "Escape clocks or real paid-shot/recovery ordering differ");
            require(result.first_launch == -1 || result.first_launch == 1, "Escape birth happened outside the single trigger update");
            require(result.first_damage == -1 || (result.first_damage >= 1 && result.first_damage <= local_update),
                "Escape damage preceded its pulse or followed its recorded end");
            require(result.target_takeoffs == (policy == EscapePolicy::jump_retreat ? 1 : 0),
                "Defender policy invented/missed a takeoff");
            if (policy == EscapePolicy::idle)
            { require(result.final_target == result.initial_target, "Idle defender baseline moved without input"); }
            else
            {
                const float away = (result.core_target.x - result.initial_target.x) * (right ? 1.0f : -1.0f);
                require(away > services.tuning.player().size.x / 2.0f, "Retreat policy produced no meaningful ordinary displacement");
            }
            return result;
        }
    }

    void escape_trials(Services& services, const std::filesystem::path& output)
    {
        std::ofstream csv(output / "turbulence-escape.csv");
        require(csv.good(), "Cannot open defender escape receipt");
        csv << std::setprecision(std::numeric_limits<float>::max_digits10);
        csv << "attacker_team,target_team,tool,source,receiver,policy,source_x,source_y,target_initial_x,target_initial_y,"
            "warmup_updates,core_updates,tail_updates,total_updates,paid_pulses,surviving_launches,first_launch_update,"
            "first_observed_box_overlap_update,first_damage_update,damage_during_paid_launch,target_health_initial,target_health_core,"
            "target_health_final,target_core_x,target_core_y,target_core_supported,target_core_support,target_final_x,"
            "target_final_y,target_final_supported,target_final_support,target_takeoffs,attacker_ammo_initial,"
            "attacker_ammo_after_pulse,attacker_ammo_final,aim_angle,scope\n";
        int trials = 0, updates = 0, launch_contacts = 0;
        std::array<int, 3> hits{}, misses{};
        for (const WeaponType tool : concrete_tools)
        {
            for (const bool right : { true, false })
            {
                for (const bool upper : { false, true })
                {
                    EscapeResult baseline;
                    for (std::size_t policy_index = 0; policy_index < policies.size(); ++policy_index)
                    {
                        const EscapePolicy policy = policies[policy_index];
                        const EscapeResult result = escape_trial(services, tool, right, upper, policy);
                        if (policy == EscapePolicy::idle) { baseline = result; }
                        else
                        {
                            require(result.source == baseline.source && result.initial_target == baseline.initial_target &&
                                result.initial_health == baseline.initial_health && result.initial_ammo == baseline.initial_ammo &&
                                result.pulse_ammo == baseline.pulse_ammo && result.angle == baseline.angle && result.warmup == baseline.warmup,
                                "Escape policies changed initial actors, health, readiness, fixed aim or paid cost");
                        }
                        const bool hit = result.first_damage >= 0;
                        if (hit) { ++hits[policy_index]; } else { ++misses[policy_index]; }
                        launch_contacts += result.launch_damage ? 1 : 0;
                        const std::string source = std::string(upper ? "crossover" : "lower_bed") + (right ? "_a" : "_b");
                        csv << (right ? 'a' : 'b') << ',' << (right ? 'b' : 'a') << ',' << tool_name(tool) << ',' << source << ',' <<
                            (upper ? source : "ground") << ',' << policy_name(policy) << ',' << result.source.x << ',' << result.source.y << ',' <<
                            result.initial_target.x << ',' << result.initial_target.y << ',' << result.warmup << ",180," << result.tail << ',' <<
                            result.updates << ",1," << result.launches << ',' << result.first_launch << ',' << result.first_observed_box_overlap << ',' <<
                            result.first_damage << ',' << result.launch_damage << ',' << result.initial_health << ',' << result.core_health << ',' <<
                            result.final_health << ',' << result.core_target.x << ',' << result.core_target.y << ',' << result.core_supported << ',' <<
                            result.core_support << ',' << result.final_target.x << ',' << result.final_target.y << ',' << result.final_supported << ',' <<
                            result.final_support << ',' << result.target_takeoffs << ',' << result.initial_ammo << ',' << result.pulse_ammo << ',' <<
                            result.final_ammo << ',' << result.angle << ",real-defender-single-pulse-no-return-fire\n";
                        updates += result.updates; ++trials;
                        std::cout << "MEASURE Turbulence escape tool=" << tool_name(tool) << " source=" << source << " policy=" <<
                            policy_name(policy) << " launch=" << result.first_launch << " observed_box_overlap=" << result.first_observed_box_overlap <<
                            " damage=" << result.first_damage << " launch_contact=" << result.launch_damage << " target_dx=" <<
                            result.core_target.x - result.initial_target.x << " target_health=" << result.final_health << '\n';
                    }
                }
            }
        }
        require(trials == 60 && updates <= 40000 && hits[0] > 0,
            "Escape matrix lost its finite inventory or all meaningful idle first-hit controls");
        csv.close(); require(csv.good(), "Cannot flush defender escape receipt");
        std::ofstream summary(output / "turbulence-escape-summary.json");
        summary << "{\"outcome\":\"passed\",\"scope\":\"turbulence-defender-escape\",\"trials\":60,\"paid_pulses\":60,"
            "\"core_updates_per_trial\":180,\"maximum_tail_updates_per_trial\":180,\"total_updates\":" << updates <<
            ",\"idle_hits\":" << hits[0] << ",\"idle_misses\":" << misses[0] << ",\"retreat_hits\":" << hits[1] <<
            ",\"retreat_misses\":" << misses[1] << ",\"jump_retreat_hits\":" << hits[2] << ",\"jump_retreat_misses\":" << misses[2] <<
            ",\"paid_launch_contact_damage_cases\":" << launch_contacts << ",\"real_target_damage_measured\":true,"
            "\"return_fire_measured\":false,\"human_acceptance\":false}\n";
        summary.close(); require(summary.good(), "Cannot flush defender escape summary");
        std::cout << "PASS 60 real-defender idle/retreat/jump-retreat single-paid-pulse trials with fixed initial actors/aim, "
            "actual damage and sampled launch/post-contact box overlaps; hit/miss outcomes retained, no continuous-arrival, "
            "return-fire or balance acceptance\n";
    }
}
