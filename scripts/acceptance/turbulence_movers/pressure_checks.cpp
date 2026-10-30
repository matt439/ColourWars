#include "game/pch.h"
#include "scripts/acceptance/turbulence_movers/fixtures.h"
#include <array>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace turbulence_movers_acceptance
{
    namespace
    {
        using namespace mattmath;
        using namespace colourwars_editor;
        constexpr std::array<WeaponType, 5> tools{ WeaponType::sprayer, WeaponType::sniper,
            WeaponType::roller, WeaponType::mister, WeaponType::bucket };
        constexpr std::array<float, 4> phases{ 0.0f, 0.25f, 0.5f, 0.75f };
        constexpr int observation_updates = 180;
        constexpr int maximum_readiness_updates = 135;
        constexpr int maximum_trial_updates = 181 + maximum_readiness_updates + observation_updates;
        constexpr int maximum_matrix_updates = 350 * maximum_trial_updates;
        constexpr int maximum_total_updates = 352 * maximum_trial_updates;
        constexpr unsigned int seed = 260026u;

        const char* tool_name(WeaponType tool)
        {
            switch (tool)
            {
            case WeaponType::sprayer: return "sprayer";
            case WeaponType::sniper: return "sniper";
            case WeaponType::roller: return "roller";
            case WeaponType::mister: return "mister";
            case WeaponType::bucket: return "bucket";
            default: throw std::runtime_error("Pressure role requires a concrete tool");
            }
        }

        struct Target
        {
            std::string name;
            RectangleF body;
        };

        std::array<Target, 7> target_bodies(Context& context, bool holder_b)
        {
            const LevelLoadedInfo source(labrador::path_to_utf8(context.content / "levels/turbulence_blockout.json").c_str());
            const auto spawns = holder_b ? source.team_a_spawns() : source.team_b_spawns();
            require(spawns.size() == 4, "Forward pressure requires all four original opposing home bodies");
            const auto size = context.world.tuning.player().size;
            std::array<Target, 7> result;
            for (std::size_t index = 0; index < spawns.size(); ++index)
            { result[index] = { "home_body_" + std::to_string(index + 1), RectangleF(spawns[index], size) }; }
            // One real body at each mouth's centre, rather than the old 100-wide
            // virtual volume. The receiver is on the opposing forward approach.
            result[4] = { "lower_departure", { mirrored(!holder_b, 550.0f) - size.x / 2.0f, 1280.0f - size.y, size.x, size.y } };
            result[5] = { "upper_departure", { mirrored(!holder_b, 700.0f) - size.x / 2.0f, 1040.0f - size.y, size.x, size.y } };
            const RectangleF receiver = named_rectangle(context.stage, holder_b ? "upper_approach_a" : "upper_approach_b");
            result[6] = { "forward_receiver", { receiver.center().x - size.x / 2.0f, receiver.top() - size.y, size.x, size.y } };
            return result;
        }

        struct Variant
        {
            fs::path path;
            RectangleF holder;
            Target target;
            bool side_b = false;
            bool moving = false;
            float initial_phase = -1.0f;
            std::string holder_name;
            std::string scope = "cross-arena-forward-pressure";
        };

        Variant author_variant(Context& context, const Target& target, bool side_b, bool moving, std::size_t phase_index,
            const RectangleF* holder_override = nullptr)
        {
            Variant result;
            result.target = target; result.side_b = side_b; result.moving = moving;
            require(holder_override == nullptr || !moving, "A near positive control uses original static ground");
            result.holder_name = moving ? "own_mover" : holder_override == nullptr ? "original_static_upper_approach" : "near_ground_sniper_control";
            if (holder_override != nullptr) { result.scope = "public-health-pipeline-near-positive-control"; }
            LevelDocument document(context.content / "tuning.json");
            document.load(moving ? context.stage : context.content / "levels/turbulence_blockout.json");
            const auto size = context.world.tuning.player().size;
            result.holder = { mirrored(side_b, 1060.0f) - size.x / 2.0f, 880.0f - size.y, size.x, size.y };
            if (holder_override != nullptr) { result.holder = *holder_override; }
            if (moving)
            {
                result.initial_phase = phases[phase_index];
                bool selected = false;
                for (const Entry& entry : document.entries())
                {
                    if (entry.kind != EntryKind::collision || (entry.label != "mover_a" && entry.label != "mover_b")) { continue; }
                    const bool own = entry.label == (side_b ? "mover_b" : "mover_a");
                    float phase = result.initial_phase + (own ? 0.0f : 0.5f);
                    if (phase >= 1.0f) { phase -= 1.0f; }
                    document.set_property(entry, "motion/phase", std::to_string(phase));
                    if (own)
                    {
                        const auto guide = document.motion_guide(entry);
                        require(guide.has_value(), "Pressure variant lost its initial editor mover guide");
                        result.holder = { static_cast<float>(guide->initial_pose.x + guide->initial_pose.width / 2.0) - size.x / 2.0f,
                            static_cast<float>(guide->initial_pose.y) - size.y, size.x, size.y };
                        selected = true;
                    }
                }
                require(selected, "Pressure variant did not find its source mover");
            }
            document.set_rectangle({ side_b ? EntryKind::spawn_b : EntryKind::spawn_a, 0, "" },
                { result.holder.x, result.holder.y, size.x, size.y });
            document.set_rectangle({ side_b ? EntryKind::spawn_a : EntryKind::spawn_b, 0, "" },
                { target.body.x, target.body.y, size.x, size.y });
            require(document.validate().empty(), "Editor rejected legal own-half forward-pressure spawns");
            result.path = context.evidence / "pressure-stages" / (std::string(side_b ? "b-" : "a-") +
                (moving ? "mover-phase" + std::to_string(phase_index) : holder_override == nullptr ? "static-approach" : "positive-ground") +
                "-" + target.name + ".json");
            require(!fs::exists(result.path), "Refusing an existing forward-pressure variant");
            document.save(result.path);
            LevelDocument reopened(context.content / "tuning.json"); reopened.load(result.path);
            require(reopened.validate().empty() && reopened.serialize() == document.serialize(),
                "Forward-pressure variant changed during actual editor save/reopen");
            return result;
        }

        void on_source(const Trial& trial, const Variant& variant)
        {
            const Player& source = trial.player();
            require(supported(source) && source.velocity().x == 0.0f, "Neutral forward-pressure source lost support or personal rest");
            if (variant.moving)
            {
                const auto mover = trial.mover(variant.side_b);
                require(source.carrier_id() == mover.id && std::fabs(source.rectangle().bottom() - mover.current_bounds.top()) < 0.06f &&
                    source.rectangle().left() >= mover.current_bounds.left() - 0.06f &&
                    source.rectangle().right() <= mover.current_bounds.right() + 0.06f,
                    "Forward-pressure source lost its whole-body carried top");
            }
            else
            { require(source.rectangle() == variant.holder && source.carrier_id() == 0, "Static pressure control moved from its initial body"); }
        }

        bool stationary_target(const Player& player, const RectangleF& expected)
        {
            const RectangleF& actual = player.rectangle();
            // Solid support can reconstruct Y across a float exponent boundary:
            // require the exact physical foot plane and unchanged body/column,
            // rather than one particular Y representation or an epsilon.
            return supported(player) && actual.x == expected.x && actual.width == expected.width &&
                actual.height == expected.height && actual.bottom() == expected.bottom();
        }

        struct Measurement
        {
            RectangleF source_before;
            RectangleF source_after_trigger;
            RectangleF source_final;
            double phase_before = -1.0;
            double phase_after_trigger = -1.0;
            double phase_final = -1.0;
            int direction_before = 0;
            int count_in = 0;
            int readiness = 0;
            int updates = 0;
            int first_damage = -1;
            int paid_pulses = 0;
            int projectiles_after_trigger = 0;
            int peak_projectiles = 0;
            int final_projectiles = 0;
            int terminal_next_rand = 0;
            float source_health = 0.0f;
            float target_health = 0.0f;
            float ammo_initial = 0.0f;
            float ammo_after_trigger = 0.0f;
            float ammo_final = 0.0f;
            float angle = 0.0f;
            float held_area = 0.0f;
            bool target_supported = false;
            int source_slot = -1;
            int target_slot = -1;
        };

        Measurement measure(Context& context, const Variant& variant, WeaponType tool)
        {
            MatchSetup setup = settings(2, variant.side_b ? PlayerTeam::b : PlayerTeam::a);
            setup.set_player_team(1, variant.side_b ? PlayerTeam::a : PlayerTeam::b);
            setup.set_player_weapon(0, tool); setup.set_player_weapon(1, WeaponType::sprayer);
            Trial trial(context, variant.path, setup, seed);
            trial.allow_damage = true;
            require(trial.level->player_count() == 2 && trial.player().rectangle() == variant.holder &&
                trial.level->player(1).rectangle() == variant.target.body && trial.player().team() != trial.level->player(1).team(),
                "Actual two-player LevelBuilder did not use the authored forward-pressure bodies/teams");
            Measurement result;
            result.count_in = trial.count_in();
            require(result.count_in == 181, "Forward-pressure scene changed the full production count-in");
            for (int tick = 0; tick < 45; ++tick) { trial.tick_inputs({ input(), input() }); ++result.readiness; }
            while (trial.player().primary_weapon().readiness() != WeaponReadiness::ready && result.readiness < maximum_readiness_updates)
            { trial.tick_inputs({ input(), input() }); ++result.readiness; }
            require(trial.player().primary_weapon().readiness() == WeaponReadiness::ready &&
                trial.player().health() == context.world.tuning.player().starting_health &&
                trial.level->player(1).health() == context.world.tuning.player().starting_health && trial.level->projectile_count() == 0,
                "Forward-pressure source never became ready or neutral warmup invented damage/flight");
            on_source(trial, variant);
            const auto& target = trial.level->player(1);
            require(stationary_target(target, variant.target.body), "Forward target did not retain its authored supported body");
            result.source_before = trial.player().rectangle(); result.ammo_initial = trial.player().weapon_ammo();
            result.source_slot = trial.player().player_num(); result.target_slot = target.player_num();
            result.angle = (target.center() - trial.player().center()).angle();
            const Vector2F direction = Vector2F::unit_vec_from_angle(result.angle);
            if (variant.moving)
            {
                const auto state = trial.mover(variant.side_b);
                result.phase_before = state.motion.phase; result.direction_before = state.motion.direction;
            }
            const float initial_target_health = target.health(), initial_source_health = trial.player().health();
            const float target_ammo = target.weapon_ammo();
            for (int update = 1; update <= observation_updates; ++update)
            {
                const float before = trial.player().weapon_ammo();
                auto action = moving_structure_acceptance::aimed_input(direction, result.angle, update == 1);
                trial.tick_inputs({ action, input() });
                if (trial.player().weapon_ammo() < before)
                { ++result.paid_pulses; require(update == 1, "Released forward-pressure tool paid for a later pulse"); }
                if (update == 1)
                {
                    result.ammo_after_trigger = trial.player().weapon_ammo();
                    result.source_after_trigger = trial.player().rectangle();
                    result.projectiles_after_trigger = trial.level->projectile_count();
                    if (variant.moving) { result.phase_after_trigger = trial.mover(variant.side_b).motion.phase; }
                    require(result.paid_pulses == 1 && result.ammo_after_trigger < result.ammo_initial,
                        "One forward-pressure trigger update did not pay for exactly one pulse");
                }
                result.peak_projectiles = (std::max)(result.peak_projectiles, trial.level->projectile_count());
                if (target.health() < initial_target_health && result.first_damage < 0) { result.first_damage = update; }
                if (!stationary_target(target, variant.target.body) || target.weapon_ammo() != target_ammo ||
                    trial.player().health() != initial_source_health)
                {
                    std::ostringstream detail;
                    detail << std::setprecision(std::numeric_limits<float>::max_digits10) <<
                        "Neutral forward target moved/fired or damaged the holder: variant=" << variant.path.filename().string() <<
                        " tool=" << tool_name(tool) << " observation_update=" << update << " level_updates=" << trial.updates;
                    const auto body_detail = [&](const char* label, const RectangleF& body)
                    {
                        detail << ' ' << label << "=(" << body.x << ',' << body.y << ',' << body.width << ',' << body.height <<
                            ") bits=(" << moving_structure_acceptance::bits(body.x) << ',' << moving_structure_acceptance::bits(body.y) << ',' <<
                            moving_structure_acceptance::bits(body.width) << ',' << moving_structure_acceptance::bits(body.height) <<
                            ") bottom=" << body.bottom() << " bottom_bits=" << moving_structure_acceptance::bits(body.bottom());
                    };
                    body_detail("target_actual", target.rectangle()); body_detail("target_expected", variant.target.body);
                    detail << " target_ammo_actual=" << target.weapon_ammo() << " bits=" << moving_structure_acceptance::bits(target.weapon_ammo()) <<
                        " target_ammo_expected=" << target_ammo << " bits=" << moving_structure_acceptance::bits(target_ammo) <<
                        " source_health_actual=" << trial.player().health() << " bits=" << moving_structure_acceptance::bits(trial.player().health()) <<
                        " source_health_expected=" << initial_source_health << " bits=" << moving_structure_acceptance::bits(initial_source_health) <<
                        " target_supported=" << supported(target) << " target_health=" << target.health() <<
                        " target_input_movement=" << target.input().x_movement << " target_input_fire=" << target.input().primary_shoot;
                    throw std::runtime_error(detail.str());
                }
                on_source(trial, variant);
            }
            require(result.paid_pulses == 1 && trial.updates == result.count_in + result.readiness + observation_updates &&
                trial.updates <= maximum_trial_updates, "Forward-pressure pulse/update inventory differs");
            result.source_final = trial.player().rectangle(); result.source_health = trial.player().health();
            result.target_health = target.health(); result.target_supported = supported(target);
            result.ammo_final = trial.player().weapon_ammo(); result.final_projectiles = trial.level->projectile_count();
            const int owner = paint_owners::index(result.source_slot, 0);
            result.held_area = trial.level->level_end_info().player_areas[static_cast<std::size_t>(owner)];
            if (variant.moving) { result.phase_final = trial.mover(variant.side_b).motion.phase; }
            result.updates = trial.updates;
            // A terminal witness, not a claim to expose the RNG's entire state.
            result.terminal_next_rand = std::rand();
            return result;
        }

        void header(std::ofstream& csv)
        {
            csv << std::setprecision(std::numeric_limits<double>::max_digits10);
            csv << "holder_team,target_team,tool,holder,target_role,requested_initial_phase,target_x,target_y,target_w,target_h,"
                "source_initial_x,source_initial_y,source_before_trigger_x,source_before_trigger_y,phase_before_trigger,direction_before_trigger,"
                "source_after_trigger_x,source_after_trigger_y,phase_after_trigger,source_final_x,source_final_y,phase_final,"
                "count_in_updates,readiness_updates,observation_updates,total_updates,paid_pulses,first_target_health_loss_update,"
                "source_health_final,target_health_final,target_supported_final,source_ammo_initial,source_ammo_after_trigger,source_ammo_final,"
                "fixed_initial_aim_angle,public_projectiles_after_trigger,public_peak_projectiles,public_final_projectiles,source_held_area,"
                "source_raw_slot,target_raw_slot,terminal_next_rand,static_control_first_damage,static_control_target_health,scope\n";
        }

        void row(std::ofstream& csv, const Variant& variant, WeaponType tool, const Measurement& value, const Measurement& control)
        {
            csv << (variant.side_b ? 'b' : 'a') << ',' << (variant.side_b ? 'a' : 'b') << ',' << tool_name(tool) << ',' <<
                variant.holder_name << ',' << variant.target.name << ',' << variant.initial_phase << ',' <<
                variant.target.body.x << ',' << variant.target.body.y << ',' << variant.target.body.width << ',' << variant.target.body.height << ',' <<
                variant.holder.x << ',' << variant.holder.y << ',' << value.source_before.x << ',' << value.source_before.y << ',' <<
                value.phase_before << ',' << value.direction_before << ',' << value.source_after_trigger.x << ',' << value.source_after_trigger.y << ',' <<
                value.phase_after_trigger << ',' << value.source_final.x << ',' << value.source_final.y << ',' << value.phase_final << ',' <<
                value.count_in << ',' << value.readiness << ',' << observation_updates << ',' << value.updates << ',' << value.paid_pulses << ',' <<
                value.first_damage << ',' << value.source_health << ',' << value.target_health << ',' << value.target_supported << ',' <<
                value.ammo_initial << ',' << value.ammo_after_trigger << ',' << value.ammo_final << ',' << value.angle << ',' <<
                value.projectiles_after_trigger << ',' << value.peak_projectiles << ',' << value.final_projectiles << ',' << value.held_area << ',' <<
                value.source_slot << ',' << value.target_slot << ',' << value.terminal_next_rand << ',' << control.first_damage << ',' <<
                control.target_health << ',' << variant.scope << '\n';
        }
    }

    void pressure(Context& context)
    {
        const auto static_path = context.content / "levels/turbulence_blockout.json";
        const std::string original_static = read_text(static_path), original_trial = read_text(context.stage);
        fs::create_directories(context.evidence / "pressure-stages");
        std::array<std::array<Variant, 7>, 2> static_variants;
        std::array<std::array<std::array<Variant, 7>, 4>, 2> moving_variants;
        for (std::size_t side = 0; side < 2; ++side)
        {
            const auto targets = target_bodies(context, side != 0);
            for (std::size_t target = 0; target < targets.size(); ++target)
            {
                static_variants[side][target] = author_variant(context, targets[target], side != 0, false, 0);
                for (std::size_t phase = 0; phase < phases.size(); ++phase)
                { moving_variants[side][phase][target] = author_variant(context, targets[target], side != 0, true, phase); }
            }
        }
        std::ofstream controls(context.evidence / "pressure-static-controls.csv"), samples(context.evidence / "pressure-movers.csv"),
            positives(context.evidence / "positive-controls.csv");
        require(controls.good() && samples.good() && positives.good(), "Cannot open actual forward-pressure/positive receipts");
        header(controls); header(samples); header(positives);
        int trials = 0, updates = 0, paid_pulses = 0, static_hits = 0, mover_hits = 0, remaining_flights = 0, positive_hits = 0;
        const auto size = context.world.tuning.player().size;
        for (const bool side_b : { false, true })
        {
            const RectangleF holder(mirrored(side_b, 1850.0f) - size.x / 2.0f, 1280.0f - size.y, size.x, size.y);
            const Target target{ "near_ground_opponent", { mirrored(side_b, 2150.0f) - size.x / 2.0f,
                1280.0f - size.y, size.x, size.y } };
            const auto variant = author_variant(context, target, side_b, false, 0, &holder);
            const auto value = measure(context, variant, WeaponType::sniper);
            require(value.first_damage >= 1 && value.target_health > 0.0f &&
                value.target_health < context.world.tuning.player().starting_health && value.paid_pulses == 1,
                "Known-near public Sniper control did not produce a real paid nonfatal health loss");
            row(positives, variant, WeaponType::sniper, value, value);
            positives.flush(); require(positives.good(), "Cannot flush known-near public health control");
            ++positive_hits; updates += value.updates; paid_pulses += value.paid_pulses;
            remaining_flights += value.final_projectiles;
            std::cout << "MEASURE known-near public Sniper control side=" << (side_b ? 'b' : 'a') <<
                " first_damage=" << value.first_damage << " health=" << value.target_health << " updates=" << value.updates << '\n';
        }
        const int positive_updates = updates;
        std::array<int, 4> phase_hits{};
        for (const WeaponType tool : tools)
        {
            for (std::size_t side = 0; side < 2; ++side)
            {
                for (std::size_t target = 0; target < 7; ++target)
                {
                    const auto& control_variant = static_variants[side][target];
                    const auto control = measure(context, control_variant, tool);
                    row(controls, control_variant, tool, control, control);
                    ++trials; updates += control.updates; paid_pulses += control.paid_pulses;
                    static_hits += control.first_damage >= 0 ? 1 : 0; remaining_flights += control.final_projectiles;
                    for (std::size_t phase = 0; phase < phases.size(); ++phase)
                    {
                        const auto& variant = moving_variants[side][phase][target];
                        const auto value = measure(context, variant, tool);
                        require(value.ammo_initial == control.ammo_initial && variant.target.body == control_variant.target.body,
                            "Forward mover comparison changed its initial reserve or real target body");
                        row(samples, variant, tool, value, control);
                        ++trials; updates += value.updates; paid_pulses += value.paid_pulses;
                        mover_hits += value.first_damage >= 0 ? 1 : 0; phase_hits[phase] += value.first_damage >= 0 ? 1 : 0;
                        remaining_flights += value.final_projectiles;
                        require(updates - positive_updates <= maximum_matrix_updates && updates <= maximum_total_updates,
                            "Forward-pressure matrix/positive controls exceeded their bounded update inventory");
                    }
                    controls.flush(); samples.flush();
                    require(controls.good() && samples.good(), "Cannot flush actual forward-pressure role receipts");
                    std::cout << "MEASURE forward pressure tool=" << tool_name(tool) << " side=" << (side == 0 ? 'a' : 'b') <<
                        " target=" << control_variant.target.name << " static_damage=" << control.first_damage << " completed=" << trials << "/350\n";
                }
            }
        }
        require(trials == 350 && positive_hits == 2 && paid_pulses == 352 && updates <= maximum_total_updates &&
            read_text(static_path) == original_static && read_text(context.stage) == original_trial,
            "Forward-pressure inventory changed or the original static/proposed source was modified");
        controls.close(); samples.close(); positives.close();
        require(controls.good() && samples.good() && positives.good(), "Cannot finish actual forward-pressure/positive receipts");
        std::ofstream summary(context.evidence / "pressure-summary.json");
        summary << "{\"outcome\":\"passed\",\"scope\":\"cross-arena-forward-pressure\",\"seed\":" << seed << ",\"mover_trials\":280,"
            "\"original_static_controls\":70,\"known_near_positive_controls\":2,\"known_near_target_hits\":" << positive_hits <<
            ",\"authored_variants\":72,\"actual_two_player_levels\":352,\"paid_pulses\":" << paid_pulses <<
            ",\"observation_updates_per_trial\":180,\"maximum_trial_updates\":" << maximum_trial_updates <<
            ",\"maximum_matrix_updates\":" << maximum_matrix_updates << ",\"maximum_total_updates\":" << maximum_total_updates <<
            ",\"positive_control_updates\":" << positive_updates << ",\"matrix_updates\":" << updates - positive_updates << ",\"total_updates\":" << updates <<
            ",\"static_target_hits\":" << static_hits << ",\"mover_target_hits\":" << mover_hits << ",\"mover_phase_hits\":[" <<
            phase_hits[0] << ',' << phase_hits[1] << ',' << phase_hits[2] << ',' << phase_hits[3] <<
            "],\"public_projectiles_remaining_at_end_sum\":" << remaining_flights <<
            ",\"real_target_health_measured\":true,\"projectile_count_snapshots_only\":true,\"invader_camping_measured\":false,"
            "\"tracking_or_continuous_fire_measured\":false,\"human_acceptance\":false}\n";
        summary.close(); require(summary.good(), "Cannot finish actual forward-pressure summary");
        std::cout << "PASS 280 own-mover phase/tool/real-target pulses plus70 original-static upper-approach controls and2 known-near positive controls via actual2P LevelBuilder; "
            "cross-arena forward pressure, actual health/paint and public projectile counts, no invader-camping/trajectory/balance acceptance\n";
    }
}
