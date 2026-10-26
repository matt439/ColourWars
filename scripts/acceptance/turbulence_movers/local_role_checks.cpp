#include "game/pch.h"
#include "scripts/acceptance/turbulence_movers/fixtures.h"
#include <array>
#include <iomanip>
#include <iostream>

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
        constexpr int maximum_readiness = 135;
        constexpr int maximum_trial_updates = 181 + maximum_readiness + observation_updates;
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
            default: throw std::runtime_error("Local mover role requires one concrete tool");
            }
        }

        rapidjson::Document parsed(const fs::path& path)
        {
            const auto text = read_text(path);
            rapidjson::Document result; result.Parse(text.c_str());
            require(!result.HasParseError() && result.IsObject(), "Cannot read local-role editor source");
            return result;
        }

        fs::path phase_variant(Context& context, bool side_b, size_t phase_index)
        {
            LevelDocument document(context.content / "tuning.json"); document.load(context.stage);
            int found = 0;
            for (const Entry& entry : document.entries())
            {
                if (entry.kind != EntryKind::collision || (entry.label != "mover_a" && entry.label != "mover_b")) { continue; }
                const bool own = entry.label == (side_b ? "mover_b" : "mover_a");
                float phase = phases[phase_index] + (own ? 0.0f : 0.5f);
                if (phase >= 1.0f) { phase -= 1.0f; }
                document.set_property(entry, "motion/phase", std::to_string(phase)); ++found;
            }
            require(found == 2 && document.validate().empty(), "Local-role phase edit lost its valid authored mover pair");
            const auto output = context.evidence / "local-role-stages" /
                (std::string(side_b ? "b-phase" : "a-phase") + std::to_string(phase_index) + ".json");
            require(!fs::exists(output), "Refusing to overwrite an earlier local-role stage");
            document.save(output);
            LevelDocument reopened(context.content / "tuning.json"); reopened.load(output);
            require(reopened.serialize() == document.serialize(), "Local-role phase variant changed during editor save/reopen");
            const auto original = parsed(context.stage), saved = parsed(output);
            const auto baseline = parsed(context.content / "levels/turbulence_blockout.json");
            require(saved["spawn_locations"] == original["spawn_locations"] &&
                saved["spawn_locations"] == baseline["spawn_locations"],
                "Local-role phase variant altered the original valid team spawns");
            return output;
        }

        size_t paint_source(const fs::path& path, const std::string& name)
        {
            const auto document = parsed(path);
            size_t ordinal = 0;
            for (const auto& object : document["collision_objects"].GetArray())
            {
                if (std::string(object["type"].GetString()) != "StructurePaintable") { continue; }
                if (std::string(object["name"].GetString()) == name) { return ordinal; }
                ++ordinal;
            }
            throw std::runtime_error("Missing local-role receiver paint source: " + name);
        }

        std::array<double, 4> receiver_area(const Trial& trial, size_t source, PlayerTeam team, int owner)
        {
            std::array<double, 4> result{};
            bool seen = false;
            // source_index is the public collector's paintable-role ordinal.
            // The public builder preserves authored collision order, also used
            // by geometry_checks. No world-box overlap guesses the receiver.
            for (const auto& sample : trial.level->paint_surfaces())
            {
                if (sample.source_index != source) { continue; }
                seen = true;
                if (sample.team == team && sample.paint_owner == owner)
                { result[static_cast<size_t>(sample.face)] += sample.scored_area; }
            }
            require(seen, "Public local-role paint collector lost the fixed crossover source");
            return result;
        }

        bool same_supported_target(const Player& target, const RectangleF& placed)
        {
            const auto& body = target.rectangle();
            return supported(target) && target.carrier_id() == 0 && body.x == placed.x &&
                body.width == placed.width && body.height == placed.height && body.bottom() == placed.bottom();
        }

        void source_supported(const Trial& trial, bool moving, bool side_b, const RectangleF& placed)
        {
            const auto& source = trial.player();
            require(supported(source) && source.velocity().x == 0.0f,
                "Neutral local-role source lost physical support or personal rest");
            const auto& body = source.rectangle();
            if (moving)
            {
                const auto state = trial.mover(side_b);
                require(source.carrier_id() == state.id && body.bottom() == state.current_bounds.top() &&
                    body.left() >= state.current_bounds.left() && body.right() <= state.current_bounds.right(),
                    "Local-role source lost actual whole-body carrier support");
            }
            else
            { require(source.carrier_id() == 0 && body.x == placed.x && body.bottom() == placed.bottom(),
                "Static local-role control moved from its supported column/foot plane"); }
        }

        struct Measurement
        {
            RectangleF source_placed, target_placed, source_before, source_after, source_final;
            int readiness = 0, updates = 0, first_damage = -1, first_receiver_paint = -1;
            int paid = 0, after_projectiles = 0, peak_projectiles = 0, final_projectiles = 0;
            int direction = 0, reversals = 0;
            double phase_before = -1.0, phase_after = -1.0, phase_final = -1.0;
            float target_health_initial = 0, target_health_final = 0;
            float ammo_initial = 0, ammo_after = 0, ammo_final = 0, angle = 0, owner_area = 0;
            std::array<double, 4> receiver{};
        };

        Measurement measure(Context& context, const fs::path& stage, bool side_b, bool moving, WeaponType tool)
        {
            auto setup = settings(2, side_b ? PlayerTeam::b : PlayerTeam::a);
            setup.set_player_weapon(0, tool); setup.set_player_weapon(1, WeaponType::sprayer);
            Trial trial(context, stage, setup, seed); trial.allow_damage = true;
            require(trial.count_in() == 181, "Local role changed the source full count-in");
            Measurement result;
            const auto size = context.world.tuning.player().size;
            const auto source_support = moving ? trial.mover(side_b).current_bounds :
                named_rectangle(stage, side_b ? "upper_approach_b" : "upper_approach_a");
            result.source_placed = { source_support.center().x - size.x / 2.0f, source_support.top() - size.y, size.x, size.y };
            result.target_placed = { mirrored(side_b, 1520.0f) - size.x / 2.0f, 800.0f - size.y, size.x, size.y };
            // Two public controlled placements, once, before readiness. Original
            // JSON spawns already passed normal editor/loader admission. These
            // actors have not earned travel to this controlled combat station.
            require(trial.level->place_player(0, result.source_placed.position()) &&
                trial.level->place_player(1, result.target_placed.position()), "Public local-role placement rejected an actual roster seat");
            trial.check();
            require(trial.player().rectangle() == result.source_placed && trial.level->player(1).rectangle() == result.target_placed &&
                trial.player().team() != trial.level->player(1).team(), "Controlled local-role bodies or opposed teams differ");
            const auto verify = [&]
            {
                source_supported(trial, moving, side_b, result.source_placed);
                require(same_supported_target(trial.level->player(1), result.target_placed),
                    "Neutral local receiver left its exact supported column/foot plane");
            };
            for (int step = 0; step < 45; ++step)
            { trial.tick_inputs({ input(), input() }); ++result.readiness; verify(); }
            while (trial.player().primary_weapon().readiness() != WeaponReadiness::ready && result.readiness < maximum_readiness)
            { trial.tick_inputs({ input(), input() }); ++result.readiness; verify(); }
            const auto& target = trial.level->player(1);
            require(trial.player().primary_weapon().readiness() == WeaponReadiness::ready &&
                trial.level->projectile_count() == 0 && trial.player().health() == context.world.tuning.player().starting_health &&
                target.health() == context.world.tuning.player().starting_health,
                "Neutral local-role readiness invented damage/flight or exceeded its bound");
            result.source_before = trial.player().rectangle();
            result.target_health_initial = target.health(); result.ammo_initial = trial.player().weapon_ammo();
            const float source_health = trial.player().health(), target_ammo = target.weapon_ammo();
            const int owner = paint_owners::index(trial.player().player_num(), 0);
            const auto receiver = paint_source(stage, side_b ? "crossover_b" : "crossover_a");
            const auto team = trial.player().team();
            require(receiver_area(trial, receiver, team, owner) == std::array<double, 4>{},
                "Local receiver was already painted before its paid pulse");
            result.angle = (target.center() - trial.player().center()).angle();
            const auto direction = Vector2F::unit_vec_from_angle(result.angle);
            float local_x = 0.0f;
            int previous_direction = 0;
            if (moving)
            {
                const auto state = trial.mover(side_b); result.phase_before = state.motion.phase;
                result.direction = state.motion.direction; previous_direction = state.motion.direction;
                local_x = trial.player().rectangle().x - state.current_bounds.x;
            }
            for (int update = 1; update <= observation_updates; ++update)
            {
                const float ammo_before = trial.player().weapon_ammo();
                trial.tick_inputs({ moving_structure_acceptance::aimed_input(direction, result.angle, update == 1), input() });
                verify();
                if (trial.player().weapon_ammo() < ammo_before)
                { ++result.paid; require(update == 1, "Released local-role input paid for another pulse"); }
                if (update == 1)
                {
                    result.ammo_after = trial.player().weapon_ammo(); result.source_after = trial.player().rectangle();
                    result.after_projectiles = trial.level->projectile_count();
                    require(result.paid == 1 && result.ammo_after < result.ammo_initial, "Local-role trigger did not pay for exactly one pulse");
                    if (moving) { result.phase_after = trial.mover(side_b).motion.phase; }
                }
                result.peak_projectiles = (std::max)(result.peak_projectiles, trial.level->projectile_count());
                if (target.health() < result.target_health_initial && result.first_damage < 0) { result.first_damage = update; }
                result.receiver = receiver_area(trial, receiver, team, owner);
                if (result.first_receiver_paint < 0 && std::any_of(result.receiver.begin(), result.receiver.end(),
                    [](double area) { return area > 0.0; })) { result.first_receiver_paint = update; }
                require(target.weapon_ammo() == target_ammo && trial.player().health() == source_health &&
                    !target.input().primary_shoot && target.input().x_movement == 0.0f,
                    "Neutral local receiver fired/moved or damaged the holder");
                if (moving)
                {
                    const auto state = trial.mover(side_b);
                    if (state.motion.direction != previous_direction) { ++result.reversals; }
                    previous_direction = state.motion.direction;
                    require(std::fabs(trial.player().rectangle().x - state.current_bounds.x - local_x) < 0.06f,
                        "Local-role carry changed stationary local footing");
                }
            }
            require(result.paid == 1 && trial.updates == 181 + result.readiness + observation_updates &&
                trial.updates <= maximum_trial_updates, "Local-role finite update/pulse inventory differs");
            result.source_final = trial.player().rectangle(); result.target_health_final = target.health();
            result.ammo_final = trial.player().weapon_ammo(); result.final_projectiles = trial.level->projectile_count();
            result.owner_area = trial.level->level_end_info().player_areas[static_cast<size_t>(owner)];
            result.updates = trial.updates;
            if (moving) { result.phase_final = trial.mover(side_b).motion.phase; }
            return result;
        }

        void row(std::ofstream& csv, bool side_b, bool moving, float phase, WeaponType tool,
            const Measurement& result, const Measurement& control)
        {
            csv << (side_b ? 'b' : 'a') << ',' << tool_name(tool) << ',' << (moving ? "own_mover" : "static_upper_approach") << ',' << phase <<
                ",2," << result.source_placed.x << ',' << result.source_placed.y << ',' << result.target_placed.x << ',' <<
                result.target_placed.y << ',' << result.source_before.x << ',' << result.source_before.y << ',' <<
                result.source_after.x << ',' << result.source_after.y << ',' << result.source_final.x << ',' << result.source_final.y << ',' <<
                result.phase_before << ',' << result.direction << ',' << result.phase_after << ',' << result.phase_final << ',' << result.reversals <<
                ",181," << result.readiness << ",180," << result.updates << ',' << result.paid << ',' << result.first_damage << ',' <<
                result.target_health_initial << ',' << result.target_health_final << ',' << result.ammo_initial << ',' << result.ammo_after << ',' <<
                result.ammo_final << ',' << result.angle << ',' << result.first_receiver_paint;
            for (const auto area : result.receiver) { csv << ',' << area; }
            csv << ',' << result.owner_area << ',' << result.after_projectiles << ',' << result.peak_projectiles << ',' << result.final_projectiles << ',' <<
                control.first_damage << ',' << control.target_health_final << ",controlled-local-single-pulse-no-travel-or-trajectory-qualification\n";
            csv.flush(); require(csv.good(), "Cannot write local-role pulse receipt");
        }

        struct PaintMeasurement
        {
            RectangleF placed, before, after, final;
            int readiness = 0, updates = 0, paid = 0, first_any = -1, first_top = -1;
            int direction = 0, reversals = 0, after_projectiles = 0, peak_projectiles = 0, final_projectiles = 0;
            double phase_before = -1.0, phase_after = -1.0, phase_final = -1.0;
            float ammo_initial = 0, ammo_after = 0, ammo_final = 0, angle = 0, owner_area = 0;
            std::array<double, 4> receiver{};
        };

        PaintMeasurement measure_paint(Context& context, const fs::path& stage, bool side_b, bool moving, WeaponType tool)
        {
            auto setup = settings(1, side_b ? PlayerTeam::b : PlayerTeam::a);
            setup.set_player_weapon(0, tool);
            Trial trial(context, stage, setup, seed);
            require(trial.count_in() == 181 && trial.level->player_count() == 1,
                "Unoccupied paint control changed the full count-in or actual solo roster");
            PaintMeasurement result;
            const auto size = context.world.tuning.player().size;
            const auto support = moving ? trial.mover(side_b).current_bounds :
                named_rectangle(stage, side_b ? "upper_approach_b" : "upper_approach_a");
            result.placed = { support.center().x - size.x / 2.0f, support.top() - size.y, size.x, size.y };
            // One controlled source placement before readiness; the fixed
            // receiver is unoccupied in this real one-player Level.
            require(trial.level->place_player(0, result.placed.position()) && trial.player().rectangle() == result.placed,
                "Public unoccupied paint placement did not retain the requested source body");
            trial.check();
            for (int step = 0; step < 45; ++step)
            {
                trial.tick(); ++result.readiness;
                source_supported(trial, moving, side_b, result.placed);
            }
            while (trial.player().primary_weapon().readiness() != WeaponReadiness::ready && result.readiness < maximum_readiness)
            {
                trial.tick(); ++result.readiness;
                source_supported(trial, moving, side_b, result.placed);
            }
            require(trial.player().primary_weapon().readiness() == WeaponReadiness::ready &&
                trial.level->projectile_count() == 0 && trial.player().health() == context.world.tuning.player().starting_health,
                "Unoccupied paint readiness invented damage/flight or exceeded its bound");
            const int owner = paint_owners::index(trial.player().player_num(), 0);
            require(owner == input_slots::keyboard_and_mouse, "Unoccupied solo paint source lost raw keyboard-slot credit");
            const auto receiver = paint_source(stage, side_b ? "crossover_b" : "crossover_a");
            const auto team = trial.player().team();
            require(receiver_area(trial, receiver, team, owner) == std::array<double, 4>{},
                "Unoccupied receiver was painted before its one pulse");
            result.before = trial.player().rectangle(); result.ammo_initial = trial.player().weapon_ammo();
            const Vector2F aim_point{ mirrored(side_b, 1520.0f), 800.0f };
            result.angle = (aim_point - trial.player().center()).angle();
            const auto direction = Vector2F::unit_vec_from_angle(result.angle);
            float local_x = 0.0f;
            int previous_direction = 0;
            if (moving)
            {
                const auto state = trial.mover(side_b); result.phase_before = state.motion.phase;
                result.direction = state.motion.direction; previous_direction = state.motion.direction;
                local_x = trial.player().rectangle().x - state.current_bounds.x;
            }
            for (int update = 1; update <= observation_updates; ++update)
            {
                const float ammo_before = trial.player().weapon_ammo();
                trial.tick(moving_structure_acceptance::aimed_input(direction, result.angle, update == 1));
                source_supported(trial, moving, side_b, result.placed);
                if (trial.player().weapon_ammo() < ammo_before)
                { ++result.paid; require(update == 1, "Released unoccupied paint input paid for another pulse"); }
                if (update == 1)
                {
                    result.ammo_after = trial.player().weapon_ammo(); result.after = trial.player().rectangle();
                    result.after_projectiles = trial.level->projectile_count();
                    require(result.paid == 1 && result.ammo_after < result.ammo_initial,
                        "Unoccupied fixed-point trigger did not pay for exactly one pulse");
                    if (moving) { result.phase_after = trial.mover(side_b).motion.phase; }
                }
                result.receiver = receiver_area(trial, receiver, team, owner);
                if (result.first_any < 0 && std::any_of(result.receiver.begin(), result.receiver.end(),
                    [](double area) { return area > 0.0; })) { result.first_any = update; }
                if (result.first_top < 0 && result.receiver[static_cast<size_t>(PaintSurfaceFace::top)] > 0.0)
                { result.first_top = update; }
                result.peak_projectiles = (std::max)(result.peak_projectiles, trial.level->projectile_count());
                require(trial.player().health() == context.world.tuning.player().starting_health,
                    "Unoccupied source lost health during its own fixed-point pulse");
                if (moving)
                {
                    const auto state = trial.mover(side_b);
                    if (state.motion.direction != previous_direction) { ++result.reversals; }
                    previous_direction = state.motion.direction;
                    require(std::fabs(trial.player().rectangle().x - state.current_bounds.x - local_x) < 0.06f,
                        "Unoccupied fixed-point role changed stationary local footing");
                }
            }
            require(result.paid == 1 && trial.updates == 181 + result.readiness + observation_updates &&
                trial.updates <= maximum_trial_updates, "Unoccupied paint pulse/update inventory differs");
            result.final = trial.player().rectangle(); result.ammo_final = trial.player().weapon_ammo();
            result.final_projectiles = trial.level->projectile_count(); result.updates = trial.updates;
            result.owner_area = trial.level->level_end_info().player_areas[static_cast<size_t>(owner)];
            if (moving) { result.phase_final = trial.mover(side_b).motion.phase; }
            return result;
        }

        void paint_row(std::ofstream& csv, bool side_b, bool moving, float phase, WeaponType tool,
            const PaintMeasurement& result, const PaintMeasurement& control)
        {
            csv << (side_b ? 'b' : 'a') << ',' << tool_name(tool) << ',' << (moving ? "own_mover" : "static_upper_approach") << ',' << phase <<
                ",1," << result.placed.x << ',' << result.placed.y << ',' << result.before.x << ',' << result.before.y << ',' <<
                result.after.x << ',' << result.after.y << ',' << result.final.x << ',' << result.final.y << ',' <<
                result.phase_before << ',' << result.direction << ',' << result.phase_after << ',' << result.phase_final << ',' << result.reversals <<
                ",181," << result.readiness << ",180," << result.updates << ',' << result.paid << ',' << mirrored(side_b, 1520.0f) <<
                ",800," << result.angle << ',' << result.ammo_initial << ',' << result.ammo_after << ',' << result.ammo_final << ',' <<
                result.first_any << ',' << result.first_top;
            for (const auto area : result.receiver) { csv << ',' << area; }
            csv << ',' << result.owner_area << ',' << result.after_projectiles << ',' << result.peak_projectiles << ',' << result.final_projectiles << ',' <<
                control.first_any << ',' << control.first_top;
            for (const auto area : control.receiver) { csv << ',' << area; }
            csv << ",4,unoccupied-fixed-crossover-top-point-single-pulse-no-earned-travel-or-trajectory-qualification\n";
            csv.flush(); require(csv.good(), "Cannot write unoccupied receiver paint receipt");
        }
    }

    void local_roles(Context& context)
    {
        const auto source_bytes = read_text(context.stage);
        const auto static_stage = context.content / "levels/turbulence_blockout.json";
        const auto static_bytes = read_text(static_stage);
        const auto directory = context.evidence / "local-role-stages";
        require(!fs::exists(directory), "Refusing earlier owned local-role receipts"); fs::create_directory(directory);
        std::ofstream csv(context.evidence / "local-roles.csv"), summary(context.evidence / "local-roles-summary.csv");
        require(csv.good() && summary.good(), "Cannot open local-role evidence");
        csv << std::setprecision(std::numeric_limits<double>::max_digits10);
        csv << "holder_team,tool,source,authored_own_phase,controlled_pre_readiness_placements,source_placed_x,source_placed_y,"
            "target_placed_x,target_placed_y,source_before_trigger_x,source_before_trigger_y,source_after_trigger_x,source_after_trigger_y,"
            "source_final_x,source_final_y,actual_phase_before_trigger,outgoing_direction,actual_phase_after_trigger,actual_phase_final,"
            "carried_reversals,count_in_updates,readiness_updates,observation_updates,total_updates,paid_pulses,first_target_health_loss_update,"
            "target_health_initial,target_health_final,source_ammo_initial,source_ammo_after_trigger,source_ammo_final,fixed_initial_torso_angle,"
            "first_crossover_paint_update,crossover_top_owner_area,crossover_bottom_owner_area,crossover_left_owner_area,crossover_right_owner_area,"
            "all_source_owner_area,public_projectiles_after_trigger,public_peak_projectiles,public_final_projectiles,"
            "matched_static_first_damage,matched_static_final_health,scope\n";
        summary << "holder_team,tool,static_hits,mover_hits,mover_samples,controlled_placements,scope\n";
        int trials = 0, updates = 0, static_hits = 0, mover_hits = 0;
        std::array<std::array<fs::path, 4>, 2> variants_by_side;
        for (const bool side_b : { false, true })
        {
            auto& variants = variants_by_side[side_b ? 1u : 0u];
            for (size_t phase_index = 0; phase_index < phases.size(); ++phase_index)
            { variants[phase_index] = phase_variant(context, side_b, phase_index); }
            for (const auto tool : tools)
            {
                const auto control = measure(context, static_stage, side_b, false, tool);
                row(csv, side_b, false, -1.0f, tool, control, control);
                ++trials; updates += control.updates; static_hits += control.first_damage >= 0 ? 1 : 0;
                int hits = 0;
                for (size_t phase_index = 0; phase_index < phases.size(); ++phase_index)
                {
                    const auto result = measure(context, variants[phase_index], side_b, true, tool);
                    require(result.target_placed == control.target_placed && result.target_health_initial == control.target_health_initial &&
                        result.ammo_initial == control.ammo_initial, "Local phase/control comparison changed target body, initial health or reserve");
                    row(csv, side_b, true, phases[phase_index], tool, result, control);
                    ++trials; updates += result.updates; hits += result.first_damage >= 0 ? 1 : 0;
                }
                mover_hits += hits;
                summary << (side_b ? 'b' : 'a') << ',' << tool_name(tool) << ',' << (control.first_damage >= 0 ? 1 : 0) << ',' << hits <<
                    ",4,10,controlled-placement-local-angle-comparison-no-earned-travel-or-balance-acceptance\n";
            }
        }
        require(trials == 50 && updates <= 50 * maximum_trial_updates,
            "Occupied local-role matrix changed its finite budget or inventory");
        csv.close(); summary.close(); require(csv.good() && summary.good(), "Cannot finish local-role receipts");

        std::ofstream paint_csv(context.evidence / "local-paint.csv"), paint_summary(context.evidence / "local-paint-summary.csv");
        require(paint_csv.good() && paint_summary.good(), "Cannot open unoccupied local-paint evidence");
        paint_csv << std::setprecision(std::numeric_limits<double>::max_digits10);
        paint_summary << std::setprecision(std::numeric_limits<double>::max_digits10);
        paint_csv << "holder_team,tool,source,authored_own_phase,controlled_pre_readiness_placements,source_placed_x,source_placed_y,"
            "source_before_trigger_x,source_before_trigger_y,source_after_trigger_x,source_after_trigger_y,source_final_x,source_final_y,"
            "actual_phase_before_trigger,outgoing_direction,actual_phase_after_trigger,actual_phase_final,carried_reversals,"
            "count_in_updates,readiness_updates,observation_updates,total_updates,paid_pulses,fixed_aim_point_x,fixed_aim_point_y,fixed_initial_point_angle,"
            "source_ammo_initial,source_ammo_after_trigger,source_ammo_final,first_receiver_paint_update,first_receiver_top_paint_update,"
            "crossover_top_owner_area,crossover_bottom_owner_area,crossover_left_owner_area,crossover_right_owner_area,all_source_owner_area,"
            "public_projectiles_after_trigger,public_peak_projectiles,public_final_projectiles,matched_static_first_receiver_paint,matched_static_first_top_paint,"
            "matched_static_top_owner_area,matched_static_bottom_owner_area,matched_static_left_owner_area,matched_static_right_owner_area,source_raw_slot,scope\n";
        paint_summary << "holder_team,tool,static_top_area,static_bottom_area,static_left_area,static_right_area,mover_top_samples,mover_any_face_samples,"
            "mover_top_area_sum_across_four_samples,mover_samples,controlled_placements,scope\n";
        int paint_trials = 0, paint_updates = 0, static_top = 0, mover_top = 0;
        for (const bool side_b : { false, true })
        {
            for (const auto tool : tools)
            {
                const auto control = measure_paint(context, static_stage, side_b, false, tool);
                paint_row(paint_csv, side_b, false, -1.0f, tool, control, control);
                ++paint_trials; paint_updates += control.updates; static_top += control.first_top >= 0 ? 1 : 0;
                int top_samples = 0, any_samples = 0;
                double top_sum = 0.0;
                const auto& variants = variants_by_side[side_b ? 1u : 0u];
                for (size_t phase_index = 0; phase_index < phases.size(); ++phase_index)
                {
                    const auto result = measure_paint(context, variants[phase_index], side_b, true, tool);
                    require(result.ammo_initial == control.ammo_initial, "Unoccupied phase/control comparison changed its initial tool reserve");
                    paint_row(paint_csv, side_b, true, phases[phase_index], tool, result, control);
                    ++paint_trials; paint_updates += result.updates;
                    top_samples += result.first_top >= 0 ? 1 : 0; any_samples += result.first_any >= 0 ? 1 : 0;
                    top_sum += result.receiver[static_cast<size_t>(PaintSurfaceFace::top)];
                }
                mover_top += top_samples;
                paint_summary << (side_b ? 'b' : 'a') << ',' << tool_name(tool);
                for (const auto area : control.receiver) { paint_summary << ',' << area; }
                paint_summary << ',' << top_samples << ',' << any_samples << ',' << top_sum <<
                    ",4,5,unoccupied-fixed-top-point-samples-not-one-match-score-or-balance-acceptance\n";
            }
        }
        require(paint_trials == 50 && paint_updates <= 50 * maximum_trial_updates &&
            trials + paint_trials == 100 && updates + paint_updates <= 100 * maximum_trial_updates &&
            read_text(context.stage) == source_bytes && read_text(static_stage) == static_bytes,
            "Combined local role/paint matrix changed its finite budget, inventory or original content bytes");
        paint_csv.close(); paint_summary.close();
        require(paint_csv.good() && paint_summary.good(), "Cannot finish unoccupied local-paint receipts");
        std::cout << "PASS controlled local mover role: 40 mover +10 static pulses, 100 placements before readiness, " << updates <<
            " actual Level updates; health hits static=" << static_hits << "/10 mover=" << mover_hits <<
            "/40. Hits/misses and occupied-crossover paint are measurements; no earned travel, full trajectory, human or balance qualification\n";
        std::cout << "PASS unoccupied local receiver paint: 40 mover +10 static pulses, 50 placements before readiness, " << paint_updates <<
            " actual Level updates; crossover-top samples static=" << static_top << "/10 mover=" << mover_top <<
            "/40. Exact per-face raw-slot credit and first paint are measured at the fixed top point; " <<
            "combined100-scene cap=" << 100 * maximum_trial_updates << " updates, no earned travel or balance qualification\n";
    }
}
