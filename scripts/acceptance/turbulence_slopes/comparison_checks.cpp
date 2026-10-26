#include "game/pch.h"
#include "scripts/acceptance/turbulence_slopes/fixtures.h"
#include <array>
#include <iomanip>
#include <iostream>

namespace turbulence_slopes_acceptance
{
    namespace
    {
        using namespace mattmath;
        constexpr std::array<WeaponType, 5> tools{ WeaponType::sprayer, WeaponType::sniper,
            WeaponType::roller, WeaponType::mister, WeaponType::bucket };
        constexpr std::array<const char*, 3> group_names{ "crossover", "lower_bed", "shoe" };
        constexpr std::array<const char*, 4> face_names{ "top", "bottom", "left", "right" };
        constexpr int observation_updates = 180, maximum_readiness = 135;
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
            default: throw std::runtime_error("Geometry comparison requires one concrete tool");
            }
        }

        struct Geometry
        {
            const char* name;
            fs::path stage;
            float total_area;
            bool shoe;
        };

        struct PaintGroups
        {
            std::array<std::optional<std::size_t>, 3> sources;
        };

        PaintGroups groups(const Geometry& geometry, bool side_b)
        {
            const std::string suffix = side_b ? "_b" : "_a";
            const std::array<std::string, 3> names{ "crossover" + suffix, "lower_bed" + suffix, "shoulder_shoe" + suffix };
            const auto text = read_text(geometry.stage);
            rapidjson::Document source; source.Parse(text.c_str());
            require(!source.HasParseError() && source.IsObject(), "Cannot read comparison geometry paint source map");
            PaintGroups result;
            std::size_t ordinal = 0;
            for (const auto& object : source["collision_objects"].GetArray())
            {
                if (std::string(object["type"].GetString()) != "StructurePaintable") { continue; }
                for (std::size_t group = 0; group < names.size(); ++group)
                {
                    if (std::string(object["name"].GetString()) == names[group])
                    { require(!result.sources[group], "Duplicate comparison receiver name"); result.sources[group] = ordinal; }
                }
                ++ordinal;
            }
            require(result.sources[0] && result.sources[1] && result.sources[2].has_value() == geometry.shoe,
                "Comparison geometry lost required receivers or misreported optional shoe presence");
            return result;
        }

        using Areas = std::array<std::array<double, 4>, 3>;
        Areas held_groups(const Trial& trial, const PaintGroups& map, int owner)
        {
            Areas result{};
            std::array<bool, 3> seen{};
            for (const auto& sample : trial.level->paint_surfaces())
            {
                for (std::size_t group = 0; group < map.sources.size(); ++group)
                {
                    if (!map.sources[group] || sample.source_index != *map.sources[group]) { continue; }
                    seen[group] = true;
                    if (sample.team == trial.player().team() && sample.paint_owner == owner)
                    { result[group][static_cast<std::size_t>(sample.face)] += sample.scored_area; }
                }
            }
            for (std::size_t group = 0; group < map.sources.size(); ++group)
            { require(!map.sources[group] || seen[group], "Public collector lost a comparison paint-source ordinal"); }
            return result;
        }

        bool same_footing(const Player& player, const RectangleF& placed)
        {
            const auto& body = player.rectangle();
            return player.state() == PlayerState::alive && supported(player) && player.carrier_id() == 0 &&
                player.velocity().x == 0.0f && body.x == placed.x && body.width == placed.width &&
                body.height == placed.height && body.bottom() == placed.bottom();
        }

        struct Result
        {
            RectangleF source, target;
            float source_initial_health = 0.0f, source_final_health = 0.0f;
            float target_initial_health = -1.0f, target_final_health = -1.0f, target_ammo = -1.0f;
            float ammo_initial = 0.0f, ammo_after = 0.0f, ammo_final = 0.0f, angle = 0.0f, total_owned = 0.0f;
            int readiness = 0, updates = 0, paid = 0, first_damage = -1, first_paint = -1;
            int after_projectiles = 0, peak_projectiles = 0, final_projectiles = 0, raw_slot = -1, owner = -1, terminal_next_rand = 0;
            std::array<int, 3> first_group_paint{ -1, -1, -1 };
            Areas areas{};
        };

        Result measure(Context& context, const Geometry& geometry, WeaponType tool, bool side_b, bool opposed)
        {
            const auto source_support = named_rectangle(geometry.stage, side_b ? "upper_approach_b" : "upper_approach_a");
            const auto receiver = named_rectangle(geometry.stage, side_b ? "crossover_b" : "crossover_a");
            const auto size = context.world.tuning.player().size;
            Result result;
            result.source = { mirrored(side_b, 1160.0f) - size.x / 2.0f, source_support.top() - size.y, size.x, size.y };
            if (opposed) { result.target = { mirrored(side_b, 1500.0f) - size.x / 2.0f, receiver.top() - size.y, size.x, size.y }; }
            require(result.source.left() >= source_support.left() && result.source.right() <= source_support.right() &&
                (!opposed || (result.target.left() >= receiver.left() && result.target.right() <= receiver.right())),
                "Comparison station does not fit an original full-body flat landing");
            auto setup = settings(opposed ? 2 : 1, side_b ? PlayerTeam::b : PlayerTeam::a);
            setup.set_player_weapon(0, tool);
            if (opposed) { setup.set_player_weapon(1, WeaponType::sprayer); }
            Trial trial(context, geometry.stage, setup, seed); trial.allow_damage = true;
            require(trial.count_in() == 181, "Geometry comparison changed the full production count-in");
            // These are explicitly controlled public placements, once per real
            // actor after normal admission/count-in, before readiness. Route
            // cost or access to the station is not inferred from this setup.
            require(trial.level->place_player(0, result.source.position()) &&
                (!opposed || trial.level->place_player(1, result.target.position())), "Public comparison placement rejected a roster seat");
            trial.check();
            const auto verify = [&]
            {
                require(same_footing(trial.player(), result.source), "Comparison source left its supported original column/foot plane");
                if (opposed)
                { require(same_footing(trial.level->player(1), result.target), "Comparison target left the original fixed crossover support"); }
            };
            for (int update = 0; update < 45; ++update) { trial.tick_inputs({ input() }); ++result.readiness; verify(); }
            while (trial.player().primary_weapon().readiness() != WeaponReadiness::ready && result.readiness < maximum_readiness)
            { trial.tick_inputs({ input() }); ++result.readiness; verify(); }
            result.source_initial_health = trial.player().health(); result.ammo_initial = trial.player().weapon_ammo();
            require(trial.player().primary_weapon().readiness() == WeaponReadiness::ready && trial.level->projectile_count() == 0 &&
                result.source_initial_health == context.world.tuning.player().starting_health && result.ammo_initial == 1.0f,
                "Comparison neutral readiness changed health/full reserve or exceeded its finite ready bound");
            if (opposed)
            {
                result.target_initial_health = trial.level->player(1).health(); result.target_ammo = trial.level->player(1).weapon_ammo();
                require(result.target_initial_health == context.world.tuning.player().starting_health && result.target_ammo == 1.0f &&
                    trial.player().team() != trial.level->player(1).team(), "Comparison neutral target health/reserve or opposed team differs");
            }
            result.raw_slot = trial.player().player_num(); result.owner = paint_owners::index(result.raw_slot, 0);
            require(result.raw_slot == (opposed ? 3 : input_slots::keyboard_and_mouse) && result.owner == result.raw_slot,
                "Comparison lost the actual raw-slot paint owner");
            const auto map = groups(geometry, side_b);
            require(held_groups(trial, map, result.owner) == Areas{} &&
                trial.level->level_end_info().player_areas[static_cast<std::size_t>(result.owner)] == 0.0f &&
                trial.level->level_end_info().total_area == geometry.total_area, "Comparison did not begin with neutral paint and its actual geometry budget");
            const Vector2F aim = opposed ? trial.level->player(1).center() : Vector2F(mirrored(side_b, 1320.0f), 880.0f);
            result.angle = (aim - trial.player().center()).angle();
            const auto direction = Vector2F::unit_vec_from_angle(result.angle);
            for (int update = 1; update <= observation_updates; ++update)
            {
                const float before = trial.player().weapon_ammo();
                trial.tick_inputs({ moving_structure_acceptance::aimed_input(direction, result.angle, update == 1) });
                verify();
                if (trial.player().weapon_ammo() < before)
                { ++result.paid; require(update == 1, "Released comparison input paid for a later pulse"); }
                if (update == 1)
                {
                    result.ammo_after = trial.player().weapon_ammo(); result.after_projectiles = trial.level->projectile_count();
                    require(result.paid == 1 && result.ammo_after < result.ammo_initial, "Comparison did not pay for its single ready trigger update");
                }
                result.peak_projectiles = (std::max)(result.peak_projectiles, trial.level->projectile_count());
                result.areas = held_groups(trial, map, result.owner);
                const auto score = trial.level->level_end_info(); result.total_owned = score.player_areas[static_cast<std::size_t>(result.owner)];
                if (result.total_owned > 0.0f && result.first_paint < 0) { result.first_paint = update; }
                for (std::size_t group = 0; group < result.areas.size(); ++group)
                {
                    double area = 0.0; for (double face : result.areas[group]) { area += face; }
                    if (area > 0.0 && result.first_group_paint[group] < 0) { result.first_group_paint[group] = update; }
                }
                require(trial.player().health() == result.source_initial_health && score.total_area == geometry.total_area,
                    "Comparison changed holder health or the geometry's local score budget");
                if (opposed)
                {
                    const auto& target = trial.level->player(1);
                    require(target.weapon_ammo() == result.target_ammo, "Neutral comparison target fired or changed reserve");
                    if (target.health() < result.target_initial_health && result.first_damage < 0) { result.first_damage = update; }
                }
            }
            result.updates = trial.updates; result.source_final_health = trial.player().health();
            result.ammo_final = trial.player().weapon_ammo(); result.final_projectiles = trial.level->projectile_count();
            if (opposed) { result.target_final_health = trial.level->player(1).health(); }
            require(result.paid == 1 && result.updates == 181 + result.readiness + observation_updates &&
                result.updates <= maximum_trial_updates && result.ammo_final >= result.ammo_after,
                "Comparison clocks, single paid pulse or released recovery inventory differs");
            result.terminal_next_rand = std::rand();
            return result;
        }

        void row(std::ofstream& csv, const Geometry& geometry, WeaponType tool, bool side_b, bool opposed, const Result& result)
        {
            csv << geometry.name << ',' << tool_name(tool) << ',' << (side_b ? 'b' : 'a') << ',' << (opposed ? "opposed_crossover" : "unoccupied_paint") << ',' <<
                result.source.x << ',' << result.source.y << ',' << result.source.bottom() << ',' << opposed << ',' <<
                result.target.x << ',' << result.target.y << ',' << result.target.bottom() << ',' << result.angle << ',' <<
                result.readiness << ",180," << result.updates << ',' << result.paid << ',' << result.first_damage << ',' <<
                result.target_initial_health << ',' << result.target_final_health << ',' << result.target_ammo << ',' <<
                result.source_initial_health << ',' << result.source_final_health << ',' << result.ammo_initial << ',' << result.ammo_after << ',' <<
                result.ammo_final << ',' << result.after_projectiles << ',' << result.peak_projectiles << ',' << result.final_projectiles << ',' <<
                result.first_paint << ',' << result.total_owned << ',' << geometry.total_area << ',' << result.raw_slot << ',' << result.owner;
            for (std::size_t group = 0; group < group_names.size(); ++group)
            {
                csv << ',' << (group < 2 || geometry.shoe) << ',' << result.first_group_paint[group];
                for (double area : result.areas[group]) { csv << ',' << area; }
            }
            csv << ',' << result.terminal_next_rand << ",three-geometry-controlled-single-pulse-no-earned-travel\n";
        }
    }

    void comparison_checks(Context& context)
    {
        SourceGuard originals(context.content, context.content / "tuning.json", context.stage);
        const std::array<Geometry, 3> geometries{ {
            { "static", context.content / "levels/turbulence_blockout.json", 148800.0f, false },
            { "mover", context.content / "levels/turbulence_mover_trial.json", 165120.0f, false },
            { "shoulder", context.stage, 172800.0f, true }
        } };
        std::ofstream csv(context.evidence / "shoulder-geometry-comparison.csv");
        require(csv.good(), "Cannot open controlled three-geometry comparison receipt");
        csv << std::setprecision(std::numeric_limits<double>::max_digits10) <<
            "geometry,tool,side,family,source_x,source_y,source_feet,target_present,target_x,target_y,target_feet,fixed_initial_aim,"
            "readiness_updates,observation_updates,total_updates,paid_pulses,first_target_health_loss,target_health_initial,target_health_final,"
            "target_unfired_ammo,source_health_initial,source_health_final,source_ammo_initial,source_ammo_after_trigger,source_ammo_final,"
            "public_projectiles_after_trigger,public_peak_projectiles,public_final_projectiles,first_owned_paint,total_owned_area,total_geometry_area,source_raw_slot,paint_owner";
        for (const char* group : group_names)
        {
            csv << ',' << group << "_present," << group << "_first_paint";
            for (const char* face : face_names) { csv << ',' << group << '_' << face << "_area"; }
        }
        csv << ",terminal_next_rand,scope\n";
        int scenes = 0, updates = 0, paid = 0, placements = 0;
        std::array<int, 3> opposed_hits{}, opposed_paint{}, unoccupied_paint{};
        for (const bool opposed : { true, false })
        {
            for (const WeaponType tool : tools)
            {
                for (const bool side_b : { false, true })
                {
                    Result baseline;
                    for (std::size_t geometry = 0; geometry < geometries.size(); ++geometry)
                    {
                        const auto result = measure(context, geometries[geometry], tool, side_b, opposed);
                        if (geometry == 0) { baseline = result; }
                        else
                        {
                            require(result.source == baseline.source && result.target == baseline.target && result.angle == baseline.angle &&
                                result.readiness == baseline.readiness && result.source_initial_health == baseline.source_initial_health &&
                                result.target_initial_health == baseline.target_initial_health && result.target_ammo == baseline.target_ammo &&
                                result.ammo_initial == baseline.ammo_initial && result.ammo_after == baseline.ammo_after,
                                "Three-geometry pair changed initial actors, ready clocks, fixed aim, health, reserves or paid cost");
                        }
                        row(csv, geometries[geometry], tool, side_b, opposed, result);
                        ++scenes; updates += result.updates; paid += result.paid; placements += opposed ? 2 : 1;
                        if (opposed)
                        { opposed_hits[geometry] += result.first_damage >= 0 ? 1 : 0; opposed_paint[geometry] += result.first_paint >= 0 ? 1 : 0; }
                        else { unoccupied_paint[geometry] += result.first_paint >= 0 ? 1 : 0; }
                        std::cout << "MEASURE shoulder comparison geometry=" << geometries[geometry].name << " tool=" << tool_name(tool) <<
                            " side=" << (side_b ? 'b' : 'a') << " family=" << (opposed ? "opposed_crossover" : "unoccupied_paint") <<
                            " first_damage=" << result.first_damage << " first_owned_paint=" << result.first_paint <<
                            " total_owned=" << result.total_owned << " total_geometry=" << geometries[geometry].total_area << '\n';
                    }
                    csv.flush(); require(csv.good(), "Cannot flush controlled geometry comparison pair");
                }
            }
        }
        require(scenes == 60 && paid == 60 && placements == 90 && updates <= maximum_trial_updates * 60,
            "Controlled geometry comparison lost its fixed60-scene finite inventory");
        originals.check(); csv.close(); require(csv.good(), "Cannot finish controlled geometry comparison receipt");
        std::ofstream summary(context.evidence / "shoulder-geometry-comparison-summary.json");
        summary << "{\"outcome\":\"passed\",\"scope\":\"three-geometry-controlled-single-pulse-stations\",\"seed\":" << seed <<
            ",\"actual_levelbuilder_scenes\":60,\"opposed_scenes\":30,\"fresh_unoccupied_paint_scenes\":30,\"paid_pulses\":60,"
            "\"post_countin_pre_readiness_placements\":90,\"total_updates\":" << updates << ",\"maximum_trial_updates\":" << maximum_trial_updates <<
            ",\"maximum_matrix_updates\":" << maximum_trial_updates * 60 << ",\"observation_updates_per_scene\":180,"
            "\"geometry_order\":[\"static\",\"mover\",\"shoulder\"],\"geometry_total_areas\":[148800,165120,172800],\"opposed_target_hits\":[" <<
            opposed_hits[0] << ',' << opposed_hits[1] << ',' << opposed_hits[2] << "],\"opposed_positive_paint_scenes\":[" <<
            opposed_paint[0] << ',' << opposed_paint[1] << ',' << opposed_paint[2] << "],\"unoccupied_positive_paint_scenes\":[" <<
            unoccupied_paint[0] << ',' << unoccupied_paint[1] << ',' << unoccupied_paint[2] <<
            "],\"projectile_count_snapshots_only\":true,\"earned_travel_measured\":false,\"trajectory_or_fire_rate_measured\":false,"
            "\"global_balance_or_human_acceptance\":false}\n";
        summary.close(); require(summary.good(), "Cannot finish controlled geometry comparison summary");
        std::cout << "PASS60 actual controlled five-tool comparisons across static/mover/shoulder geometry: matched supported original stations, "
            "one paid pulse, real health/paint by source+face/raw owner, actual differing score budgets; no tracking, earned-travel or balance acceptance\n";
    }
}
