#include "game/pch.h"
#include "scripts/acceptance/turbulence_blockout/tool_fixtures.h"
#include <unordered_map>

namespace turbulence_blockout_acceptance
{
    namespace
    {
        enum class ExchangePolicy { standing_control, moving_attacker, jumping_attacker, counterfire };
        constexpr std::array<ExchangePolicy, 4> policies{ ExchangePolicy::standing_control,
            ExchangePolicy::moving_attacker, ExchangePolicy::jumping_attacker, ExchangePolicy::counterfire };
        constexpr int observation_limit = 180;
        constexpr int tail_limit = 40;
        constexpr int scene_limit = 360;
        constexpr int matrix_limit = 21600;

        const char* policy_name(ExchangePolicy policy)
        {
            switch (policy)
            {
            case ExchangePolicy::standing_control: return "standing_control";
            case ExchangePolicy::moving_attacker: return "moving_attacker";
            case ExchangePolicy::jumping_attacker: return "jumping_attacker";
            case ExchangePolicy::counterfire: return "counterfire";
            default: throw std::runtime_error("Unknown exchange policy");
            }
        }

        std::string support_of(const Run& run, const Player& player)
        {
            for (const auto& [surface, object] : run.structures)
            {
                (void)object;
                if (!surface->ramp() && std::fabs(player.rectangle().bottom() - surface->rectangle.top()) <= tolerance &&
                    player.rectangle().right() > surface->rectangle.left() && player.rectangle().left() < surface->rectangle.right())
                { return surface->name; }
            }
            return "air-or-ramp";
        }

        struct ActorResult
        {
            RectangleF initial;
            RectangleF final;
            float initial_health = 0.0f;
            float final_health = 0.0f;
            float initial_ammo = 0.0f;
            float pulse_ammo = 0.0f;
            float final_ammo = 0.0f;
            float angle = 0.0f;
            int paid_pulses = 0;
            int launches = 0;
            int first_launch = -1;
            int first_observed_victim_box_overlap = -1;
            int first_health_loss = -1;
            int takeoffs = 0;
            bool damaged_victim_during_launch = false;
            bool final_supported = false;
            std::string final_support;
        };

        struct ExchangeResult
        {
            std::array<ActorResult, 2> actors;
            int warmup = 0;
            int observation = 0;
            int tail = 0;
            int updates = 0;
            int lethal_update = -1;
            bool counterfire_suppressed_on_lethal = false;
        };

        ExchangeResult exchange_trial(Services& services, WeaponType tool, bool right, bool upper,
            ExchangePolicy policy, int& matrix_updates)
        {
            std::srand(260026u);
            const PlayerTeam attacker_team = right ? PlayerTeam::a : PlayerTeam::b;
            const PlayerTeam defender_team = right ? PlayerTeam::b : PlayerTeam::a;
            const std::string source = std::string(upper ? "crossover" : "lower_bed") + (right ? "_a" : "_b");
            Run run(services, standing(services, services.named(source), reflected(right, upper ? 1670.0f : 1420.0f)),
                attacker_team, 0.0f, tool);
            Player& target = run.add_target(standing(services, services.named(upper ? source : "ground"),
                reflected(right, upper ? 1840.0f : 1780.0f)), defender_team);
            // Every exchange/control scene enables the same neutral guest
            // weapon seam before warmup. Other modes keep it disabled.
            run.target_weapon_updates = true;
            run.death_expected = true;
            const std::array<Player*, 2> actors{ run.player, &target };
            const std::array<RectangleF, 2> constructed{ run.player->rectangle(), target.rectangle() };
            const std::array<float, 2> constructed_health{ run.player->health(), target.health() };
            const auto bounded_tick = [&](const PlayerInputData& actions)
            {
                require(matrix_updates < matrix_limit && run.ticks < scene_limit,
                    "Exchange matrix or scene exceeded its exact update budget");
                run.tick(actions); ++matrix_updates;
            };
            for (int tick = 0; tick < 45; ++tick) { bounded_tick(input()); }
            int ready_wait = 0;
            while ((run.player->primary_weapon().readiness() != WeaponReadiness::ready ||
                target.primary_weapon().readiness() != WeaponReadiness::ready) && ready_wait++ < 135)
            { bounded_tick(input()); }
            ExchangeResult result;
            for (std::size_t index = 0; index < actors.size(); ++index)
            {
                const Player& actor = *actors[index];
                require(actor.primary_weapon().readiness() == WeaponReadiness::ready &&
                    actor.health() == constructed_health[index] && actor.rectangle() == constructed[index] && supported(actor),
                    "Unfired exchange/control actor moved, lost health or never became ready");
                ActorResult& record = result.actors[index];
                record.initial = actor.rectangle(); record.initial_health = actor.health();
                record.initial_ammo = actor.weapon_ammo();
            }
            require(run.paid_events == 0 && run.target_paid_events == 0 && !flying(run),
                "Neutral exchange readiness warmup invented a paid pulse or flight");
            if (tool == WeaponType::sniper) { result.actors[0].angle = (target.center() - run.player->center()).angle(); }
            else
            {
                const float angle = tool == WeaponType::bucket ? -PI / 6.0f : 0.0f;
                result.actors[0].angle = right ? angle : PI - angle;
            }
            result.actors[1].angle = (run.player->center() - target.center()).angle();
            result.warmup = run.ticks;
            int local_update = 0;
            std::unordered_map<const CollisionObject*, std::size_t> observed;
            const auto inspect_box = [&](const CollisionObject& projectile, std::size_t owner)
            {
                // Sampled actual launch/post-contact poses can omit retired or
                // swept-through contacts. Actual health loss remains separate;
                // this observer makes no continuous arrival/warning claim.
                ActorResult& record = result.actors[owner];
                if (record.first_observed_victim_box_overlap < 0 &&
                    narrow_phase(*projectile.shape(), *actors[1 - owner]->shape()))
                { record.first_observed_victim_box_overlap = local_update; }
            };
            const auto observe_launch = [&](std::span<const std::unique_ptr<Projectile>> emitted, std::size_t owner)
            {
                const int paid = owner == 0 ? run.paid_events : run.target_paid_events;
                const bool permitted = owner == 0 || policy == ExchangePolicy::counterfire;
                ActorResult& record = result.actors[owner];
                if (local_update == 1 && paid == 1 && actors[1 - owner]->health() < result.actors[1 - owner].initial_health)
                { record.damaged_victim_during_launch = true; }
                for (const auto& projectile : emitted)
                {
                    require(permitted && local_update == 1 && paid == 1,
                        "Exchange observed an unpaid, unrequested or later projectile birth");
                    require(actors[owner]->team() == (owner == 0 ? attacker_team : defender_team) &&
                        projectile->player_num() == actors[owner]->player_num() &&
                        observed.emplace(projectile.get(), owner).second,
                        "Exchange projectile lost its source player, paint owner or unique birth provenance");
                    if (record.first_launch < 0) { record.first_launch = local_update; }
                    ++record.launches; inspect_box(*projectile, owner);
                }
            };
            run.on_weapon_launch = [&](std::span<const std::unique_ptr<Projectile>> emitted) { observe_launch(emitted, 0); };
            run.on_target_weapon_launch = [&](std::span<const std::unique_ptr<Projectile>> emitted) { observe_launch(emitted, 1); };
            run.after_scene_contacts = [&]
            {
                for (const CollisionObject* object : run.scene.collision_objects())
                {
                    if (!is_projectile(to_collision_type(object->tag()))) { continue; }
                    const auto entry = observed.find(object);
                    require(entry != observed.end(), "Exchange live flight had no observed paid birth");
                    const Projectile& projectile = dynamic_cast<const Projectile&>(*object);
                    require(projectile.player_num() == actors[entry->second]->player_num(), "Exchange live paint owner changed");
                    inspect_box(projectile, entry->second);
                }
                for (std::size_t index = 0; index < actors.size(); ++index)
                {
                    if (actors[index]->health() < result.actors[index].initial_health && result.actors[index].first_health_loss < 0)
                    { result.actors[index].first_health_loss = local_update; }
                }
            };
            const auto advance = [&](bool core)
            {
                require(result.lethal_update < 0 && run.player->health() > 0.0f && target.health() > 0.0f,
                    "Exchange advanced after a terminal health event");
                ++local_update;
                const bool walking = core && (policy == ExchangePolicy::moving_attacker || policy == ExchangePolicy::jumping_attacker);
                const bool jumping = core && policy == ExchangePolicy::jumping_attacker;
                const bool counter = policy == ExchangePolicy::counterfire;
                PlayerInputData actions = firing(result.actors[0].angle, walking ? (right ? 1.0f : -1.0f) : 0.0f, local_update == 1);
                actions.jump_pressed = jumping && local_update == 1;
                actions.jump_held = jumping && local_update <= 30;
                run.target_input = firing(result.actors[1].angle, core && counter ? (right ? 1.0f : -1.0f) : 0.0f,
                    counter && local_update == 1);
                bounded_tick(actions);
                if (local_update == 1)
                {
                    for (std::size_t index = 0; index < actors.size(); ++index)
                    { result.actors[index].pulse_ammo = actors[index]->weapon_ammo(); }
                }
                result.counterfire_suppressed_on_lethal = counter && local_update == 1 &&
                    target.health() <= 0.0f && run.target_paid_events == 0;
                require(run.paid_events == 1 &&
                    run.target_paid_events == (counter && !result.counterfire_suppressed_on_lethal ? 1 : 0),
                    "Exchange repeated or omitted a participant's single paid pulse");
                require(services.info.out_of_bounds_rectangle().contains(target.rectangle()), "Exchange defender body left authored bounds");
                for (const auto& [surface, object] : run.structures)
                {
                    if (surface->one_way()) { continue; }
                    const auto hit = narrow_phase(*target.shape(), *object->shape());
                    require(!hit || hit->penetration <= tolerance, "Exchange defender body overlaps " + surface->name);
                }
                for (std::size_t index = 0; index < actors.size(); ++index)
                {
                    for (const auto& effect : actors[index]->movement_feedback().effects())
                    { if (effect.age == 0.0f && effect.kind == MovementFeedbackKind::takeoff) { ++result.actors[index].takeoffs; } }
                    if (actors[index]->health() <= 0.0f || actors[index]->state() != PlayerState::alive)
                    { result.lethal_update = local_update; }
                }
            };
            while (result.observation < observation_limit && result.lethal_update < 0) { advance(true); ++result.observation; }
            while (flying(run) && result.tail < tail_limit && result.lethal_update < 0) { advance(false); ++result.tail; }
            require(result.lethal_update >= 0 || !flying(run), "Exchange pulse outlived its bounded flight tail");
            result.updates = run.ticks;
            require(result.updates == result.warmup + result.observation + result.tail &&
                run.last_paid_tick == result.warmup + 1 && (run.target_paid_events == 0 ||
                    run.target_last_paid_tick == result.warmup + 1),
                "Exchange warmup/observation/tail or actual paid-pulse clocks disagree");
            for (std::size_t index = 0; index < actors.size(); ++index)
            {
                const Player& actor = *actors[index];
                ActorResult& record = result.actors[index];
                record.final = actor.rectangle(); record.final_health = actor.health(); record.final_ammo = actor.weapon_ammo();
                record.final_supported = supported(actor); record.final_support = support_of(run, actor);
                record.paid_pulses = index == 0 ? run.paid_events : run.target_paid_events;
                require(record.first_launch == -1 || record.first_launch == 1, "Exchange flight birth was outside its trigger update");
                require(record.first_health_loss == -1 || (record.first_health_loss >= 1 && record.first_health_loss <= local_update),
                    "Exchange health loss preceded its trigger or followed recorded completion");
                const bool fired = record.paid_pulses == 1;
                require(fired ? record.pulse_ammo < record.initial_ammo : record.pulse_ammo == record.initial_ammo,
                    "Exchange actual ammunition did not match that participant's trigger policy");
                require(record.final_ammo >= record.pulse_ammo, "Released exchange weapon did not preserve/recover its reserve");
            }
            if (result.lethal_update < 0)
            {
                require(result.actors[0].takeoffs == (policy == ExchangePolicy::jumping_attacker ? 1 : 0) &&
                    result.actors[1].takeoffs == 0, "Exchange policy invented/missed a takeoff");
                if (policy == ExchangePolicy::standing_control || policy == ExchangePolicy::counterfire)
                { require(result.actors[0].final == result.actors[0].initial, "Stationary exchange attacker moved without input"); }
                else
                {
                    require((result.actors[0].final.x - result.actors[0].initial.x) * (right ? 1.0f : -1.0f) >
                        services.tuning.player().size.x / 2.0f, "Exchange attack policy produced no meaningful inward displacement");
                }
                if (policy != ExchangePolicy::counterfire)
                {
                    require(result.actors[1].final == result.actors[1].initial &&
                        result.actors[0].final_health == result.actors[0].initial_health,
                        "Idle defender moved or damaged the attacker without a trigger");
                }
                else
                {
                    require((result.actors[1].final.x - result.actors[1].initial.x) * (right ? 1.0f : -1.0f) >
                        services.tuning.player().size.x / 2.0f, "Counterfire defender produced no meaningful ordinary retreat");
                }
            }
            return result;
        }

        void write_header(std::ostream& csv)
        {
            csv << "attacker_team,defender_team,tool,source,receiver,policy,warmup_updates,observation_updates,tail_updates,total_updates,lethal_update,"
                "counterfire_suppressed_on_lethal,";
            for (const char* prefix : { "attacker", "defender" })
            {
                for (const char* field : { "initial_x", "initial_y", "final_x", "final_y", "health_initial", "health_final", "ammo_initial",
                    "ammo_after_pulse", "ammo_final", "fixed_aim_angle", "paid_pulses", "surviving_launches", "first_launch_update",
                    "first_observed_victim_box_overlap_update", "first_health_loss_update", "damaged_victim_during_paid_launch",
                    "takeoffs", "final_supported", "final_support" })
                { csv << prefix << '_' << field << ','; }
            }
            csv << "standing_defender_first_health_loss_update,standing_defender_final_health,standing_attacker_final_health,scope\n";
        }

        void write_row(std::ostream& csv, WeaponType tool, bool right, bool upper, ExchangePolicy policy,
            const ExchangeResult& result, const ExchangeResult& baseline)
        {
            const std::string source = std::string(upper ? "crossover" : "lower_bed") + (right ? "_a" : "_b");
            csv << (right ? 'a' : 'b') << ',' << (right ? 'b' : 'a') << ',' << tool_name(tool) << ',' << source << ',' <<
                (upper ? source : "ground") << ',' << policy_name(policy) << ',' << result.warmup << ',' << result.observation << ',' <<
                result.tail << ',' << result.updates << ',' << result.lethal_update << ',' << result.counterfire_suppressed_on_lethal << ',';
            for (const ActorResult& actor : result.actors)
            {
                csv << actor.initial.x << ',' << actor.initial.y << ',' << actor.final.x << ',' << actor.final.y << ',' <<
                    actor.initial_health << ',' << actor.final_health << ',' << actor.initial_ammo << ',' << actor.pulse_ammo << ',' <<
                    actor.final_ammo << ',' << actor.angle << ',' << actor.paid_pulses << ',' << actor.launches << ',' << actor.first_launch << ',' <<
                    actor.first_observed_victim_box_overlap << ',' << actor.first_health_loss << ',' << actor.damaged_victim_during_launch << ',' <<
                    actor.takeoffs << ',' << actor.final_supported << ',' << actor.final_support << ',';
            }
            csv << baseline.actors[1].first_health_loss << ',' << baseline.actors[1].final_health << ',' << baseline.actors[0].final_health <<
                ",single-paid-pulses-fixed-initial-aim-no-tracking\n";
        }
    }

    void exchange_trials(Services& services, const std::filesystem::path& output)
    {
        std::ofstream csv(output / "turbulence-exchange.csv"), controls(output / "turbulence-exchange-controls.csv");
        require(csv.good() && controls.good(), "Cannot open exchange/control receipts");
        csv << std::setprecision(std::numeric_limits<float>::max_digits10);
        controls << std::setprecision(std::numeric_limits<float>::max_digits10);
        write_header(csv); write_header(controls);
        int updates = 0, scenes = 0, lethal_stops = 0, attacker_paid = 0, defender_paid = 0, suppressed_counterfire = 0;
        std::array<int, 4> defender_hits{}, attacker_hits{};
        for (const WeaponType tool : concrete_tools)
        {
            for (const bool right : { true, false })
            {
                for (const bool upper : { false, true })
                {
                    ExchangeResult baseline;
                    for (std::size_t policy_index = 0; policy_index < policies.size(); ++policy_index)
                    {
                        const ExchangePolicy policy = policies[policy_index];
                        const ExchangeResult result = exchange_trial(services, tool, right, upper, policy, updates);
                        if (policy == ExchangePolicy::standing_control) { baseline = result; }
                        else
                        {
                            require(result.warmup == baseline.warmup, "Exchange and standing control readiness clocks differ");
                            for (std::size_t index = 0; index < result.actors.size(); ++index)
                            {
                                require(result.actors[index].initial == baseline.actors[index].initial &&
                                    result.actors[index].initial_health == baseline.actors[index].initial_health &&
                                    result.actors[index].initial_ammo == baseline.actors[index].initial_ammo &&
                                    result.actors[index].angle == baseline.actors[index].angle,
                                    "Exchange and standing control initial actors, health, ammo or fixed aim differ");
                            }
                            require(result.actors[0].pulse_ammo == baseline.actors[0].pulse_ammo,
                                "Exchange motion/counterfire changed the attacker's single paid cost");
                        }
                        write_row(policy == ExchangePolicy::standing_control ? controls : csv,
                            tool, right, upper, policy, result, baseline);
                        defender_hits[policy_index] += result.actors[1].first_health_loss >= 0 ? 1 : 0;
                        attacker_hits[policy_index] += result.actors[0].first_health_loss >= 0 ? 1 : 0;
                        lethal_stops += result.lethal_update >= 0 ? 1 : 0;
                        suppressed_counterfire += result.counterfire_suppressed_on_lethal ? 1 : 0;
                        attacker_paid += result.actors[0].paid_pulses; defender_paid += result.actors[1].paid_pulses; ++scenes;
                        std::cout << "MEASURE Turbulence exchange tool=" << tool_name(tool) << " source=" <<
                            (upper ? "crossover" : "lower_bed") << (right ? "_a" : "_b") << " policy=" << policy_name(policy) <<
                            " defender_damage=" << result.actors[1].first_health_loss << " attacker_damage=" << result.actors[0].first_health_loss <<
                            " attacker_dx=" << result.actors[0].final.x - result.actors[0].initial.x << " defender_dx=" <<
                            result.actors[1].final.x - result.actors[1].initial.x << " lethal_update=" << result.lethal_update << '\n';
                    }
                }
            }
        }
        require(scenes == 80 && attacker_paid == 80 && defender_paid + suppressed_counterfire == 20 &&
            defender_hits[0] > 0 && updates <= matrix_limit,
            "Exchange lost its bounded inventory, paid-pulse provenance or positive standing controls");
        csv.close(); controls.close(); require(csv.good() && controls.good(), "Cannot flush exchange/control receipts");
        std::ofstream summary(output / "turbulence-exchange-summary.json");
        summary << "{\"outcome\":\"passed\",\"scope\":\"turbulence-moving-fire-single-pulse-counterfire\",\"exchange_trials\":60,"
            "\"standing_controls\":20,\"total_scenes\":80,\"maximum_observation_updates_per_scene\":180,\"maximum_tail_updates_per_scene\":40,"
            "\"maximum_updates_per_scene\":360,\"maximum_matrix_updates\":21600,\"total_updates\":" << updates <<
            ",\"paid_attacker_pulses\":" << attacker_paid << ",\"paid_defender_pulses\":" << defender_paid <<
            ",\"standing_defender_hits\":" << defender_hits[0] << ",\"moving_attacker_defender_hits\":" << defender_hits[1] <<
            ",\"jumping_attacker_defender_hits\":" << defender_hits[2] << ",\"counterfire_defender_hits\":" << defender_hits[3] <<
            ",\"counterfire_attacker_hits\":" << attacker_hits[3] << ",\"lethal_stops\":" << lethal_stops <<
            ",\"counterfire_suppressed_on_lethal\":" << suppressed_counterfire <<
            ",\"actual_health_loss_measured\":true,\"continuous_fire_measured\":false,\"human_acceptance\":false}\n";
        summary.close(); require(summary.good(), "Cannot flush exchange summary");
        std::cout << "PASS 60 moving/jumping-attacker or single-Sprayer-counterfire trials plus 20 matched standing controls; "
            "actual paid pulses/health and sampled post-contact boxes, fixed initial aim and one seed, no tracking or sustained-fire/balance acceptance\n";
    }
}
