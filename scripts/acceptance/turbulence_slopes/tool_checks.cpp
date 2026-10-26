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
        using namespace colourwars_editor;
        constexpr int observation_updates = 180, maximum_readiness = 135;
        constexpr int maximum_trial_updates = 181 + maximum_readiness + observation_updates;
        constexpr unsigned int seed = 260026u;
        enum class ToolCase { shoe_blocks, removed_shoe_passes, shoe_top };

        const char* case_name(ToolCase value)
        {
            return value == ToolCase::shoe_blocks ? "shoe_blocks" :
                value == ToolCase::removed_shoe_passes ? "removed_shoe_passes" : "shoe_top";
        }

        RectangleF box(const fs::path& stage, const fs::path& tuning, const std::string& name)
        {
            LevelDocument document(tuning); document.load(stage);
            for (const Entry& entry : document.entries())
            {
                if (entry.kind != EntryKind::collision || entry.label != name) { continue; }
                const auto value = document.rectangle(entry);
                return { static_cast<float>(value.x), static_cast<float>(value.y),
                    static_cast<float>(value.width), static_cast<float>(value.height) };
            }
            throw std::runtime_error("Missing controlled shoulder role geometry: " + name);
        }

        std::optional<std::size_t> paint_source(const fs::path& stage, const std::string& name)
        {
            const auto text = read_text(stage); rapidjson::Document source; source.Parse(text.c_str());
            require(!source.HasParseError(), "Cannot read shoulder paint source ordinal");
            std::size_t ordinal = 0;
            for (const auto& object : source["collision_objects"].GetArray())
            {
                if (std::string(object["type"].GetString()) != "StructurePaintable") { continue; }
                if (std::string(object["name"].GetString()) == name) { return ordinal; }
                ++ordinal;
            }
            return std::nullopt;
        }

        std::array<double, 4> credited(const Trial& trial, std::optional<std::size_t> source, int owner)
        {
            std::array<double, 4> result{};
            if (!source) { return result; }
            bool seen = false;
            for (const auto& tile : trial.level->paint_surfaces())
            {
                if (tile.source_index != *source) { continue; }
                seen = true;
                if (tile.team == trial.player().team() && tile.paint_owner == owner)
                { result[static_cast<std::size_t>(tile.face)] += tile.scored_area; }
            }
            require(seen, "Public paint collector lost the controlled flat shoe source");
            return result;
        }

        fs::path without_shoe(Context& context, bool side_b)
        {
            LevelDocument document(context.content / "tuning.json"); document.load(context.stage);
            bool removed = false;
            for (const Entry& entry : document.entries())
            {
                if (entry.kind == EntryKind::collision && entry.label == (side_b ? "shoulder_shoe_b" : "shoulder_shoe_a"))
                { document.erase(entry); removed = true; break; }
            }
            require(removed && document.validate().empty(), "Actual editor did not delete exactly the own flat shoe");
            const auto output = context.evidence / (side_b ? "tool-pass-b.json" : "tool-pass-a.json");
            require(!fs::exists(output), "Refusing an earlier shoe-removal control"); document.save(output);
            LevelDocument reopened(context.content / "tuning.json"); reopened.load(output);
            require(reopened.serialize() == document.serialize() && reopened.validate().empty(),
                "Own-shoe deletion changed during actual editor save/reopen");
            const auto before_text = read_text(context.stage), after_text = read_text(output);
            rapidjson::Document before, after; before.Parse(before_text.c_str()); after.Parse(after_text.c_str());
            require(before["collision_objects"].Size() == 26 && after["collision_objects"].Size() == 25,
                "Shoe-removal control did not remove exactly one collision object");
            rapidjson::SizeType saved_index = 0;
            for (const auto& object : before["collision_objects"].GetArray())
            {
                if (std::string(object["name"].GetString()) == (side_b ? "shoulder_shoe_b" : "shoulder_shoe_a")) { continue; }
                require(object == after["collision_objects"][saved_index++], "Shoe-removal control changed a retained collision object");
            }
            for (auto member = before.MemberBegin(); member != before.MemberEnd(); ++member)
            {
                if (std::string(member->name.GetString()) == "collision_objects") { continue; }
                require(after.HasMember(member->name) && after[member->name] == member->value,
                    "Shoe-removal control changed spawns, bounds or metadata");
            }
            return output;
        }

        // This is an explicit geometric preflight for a20x20 public-tuning
        // collision body, not a fabricated observed projectile trajectory.
        bool corridor_hits(const Vector2F& from, const Vector2F& to, const RectangleF& obstacle, const Vector2F& half_size)
        {
            double entry = 0.0, exit = 1.0;
            const auto axis = [&](double origin, double finish, double low, double high)
            {
                if (origin == finish) { return origin >= low && origin <= high; }
                double first = (low - origin) / (finish - origin), last = (high - origin) / (finish - origin);
                if (first > last) { std::swap(first, last); }
                entry = (std::max)(entry, first); exit = (std::min)(exit, last);
                return entry <= exit;
            };
            return axis(from.x, to.x, obstacle.left() - half_size.x, obstacle.right() + half_size.x) &&
                axis(from.y, to.y, obstacle.top() - half_size.y, obstacle.bottom() + half_size.y) && entry <= exit;
        }

        RectangleF top_station(Context& context, bool side_b, std::ofstream& preflight)
        {
            const auto crossover = named_rectangle(context.stage, side_b ? "crossover_b" : "crossover_a");
            const auto upper = named_rectangle(context.stage, side_b ? "upper_approach_b" : "upper_approach_a");
            const auto shoe = named_rectangle(context.stage, side_b ? "shoulder_shoe_b" : "shoulder_shoe_a");
            const auto size = context.world.tuning.player().size;
            const auto half_shot = context.world.tuning.projectile(ProjectileType::jet).details.col_rect_size / 2.0f;
            const Vector2F aim(shoe.center().x, shoe.top());
            // Retain the rejected crossover proposals as labelled analysis,
            // then select the roomy existing upper-approach station before the
            // one actual placement. No shot is fired at a rejected proposal.
            for (const float a_centre : { 1500.0f, 1430.0f })
            {
                const float centre = mirrored(side_b, a_centre);
                const RectangleF candidate(centre - size.x / 2.0f, crossover.top() - size.y, size.x, size.y);
                const bool contained = candidate.left() >= crossover.left() && candidate.right() <= crossover.right();
                const bool blocked = corridor_hits(candidate.center(), aim, crossover, half_shot);
                preflight << (side_b ? 'b' : 'a') << ",rejected_crossover_" << a_centre << ',' << centre << ',' << candidate.bottom() << ',' <<
                    half_shot.x * 2.0f << ',' << half_shot.y * 2.0f << ',' << contained << ',' << blocked <<
                    ",analytic-body-corridor-preflight-no-shot-fired\n";
                require(contained && blocked, "Rejected top-shot crossover proposal no longer matches its recorded geometry");
            }
            const float centre = mirrored(side_b, 1160.0f);
            const RectangleF selected(centre - size.x / 2.0f, upper.top() - size.y, size.x, size.y);
            const bool contained = selected.left() >= upper.left() && selected.right() <= upper.right();
            const bool blocked = corridor_hits(selected.center(), aim, upper, half_shot);
            preflight << (side_b ? 'b' : 'a') << ",selected_upper_approach," << centre << ',' << selected.bottom() << ',' <<
                half_shot.x * 2.0f << ',' << half_shot.y * 2.0f << ',' << contained << ',' << blocked <<
                ",analytic-body-corridor-preflight-before-sole-placement\n";
            require(contained && !blocked, "Whole-body upper-approach station did not clear its actual Sniper-body corridor to the shoe top");
            return selected;
        }

        bool same_footing(const Player& player, const RectangleF& placed)
        {
            const auto& actual = player.rectangle();
            return supported(player) && player.carrier_id() == 0 && player.velocity().x == 0.0f && actual.x == placed.x &&
                actual.width == placed.width && actual.height == placed.height && actual.bottom() == placed.bottom();
        }

        struct Result
        {
            RectangleF source, target;
            bool has_target = false;
            int readiness = 0, updates = 0, first_damage = -1, first_paint = -1, paid = 0;
            int after_projectiles = 0, peak_projectiles = 0, final_projectiles = 0, raw_slot = -1, owner = -1, terminal_next_rand = 0;
            float angle = 0, source_health = 0, target_initial_health = -1, target_final_health = -1;
            float ammo_initial = 0, ammo_after = 0, ammo_final = 0;
            std::array<double, 4> shoe{};
        };

        Result measure(Context& context, const fs::path& stage, bool side_b, ToolCase which, const RectangleF& top_source)
        {
            const bool top = which == ToolCase::shoe_top;
            const auto tuning = context.content / "tuning.json";
            const auto size = context.world.tuning.player().size;
            const auto ramp = box(stage, tuning, side_b ? "shoulder_ramp_b" : "shoulder_ramp_a");
            const auto lower_bed = named_rectangle(stage, side_b ? "lower_bed_b" : "lower_bed_a");
            Result result; result.has_target = !top;
            result.source = top ? top_source : RectangleF(ramp.center().x - size.x / 2.0f, lower_bed.top() - size.y, size.x, size.y);
            const float support_x = ramp.center().x + (side_b ? -size.x / 2.0f : size.x / 2.0f);
            const float along = (support_x - ramp.left()) / ramp.width;
            const float feet = side_b ? ramp.top() + along * ramp.height : ramp.bottom() - along * ramp.height;
            if (!top) { result.target = { ramp.center().x - size.x / 2.0f, feet - size.y, size.x, size.y }; }
            auto setup = settings(top ? 1 : 2, side_b ? PlayerTeam::b : PlayerTeam::a);
            setup.set_player_weapon(0, WeaponType::sniper);
            if (!top) { setup.set_player_weapon(1, WeaponType::sprayer); }
            Trial trial(context, stage, setup, seed); trial.allow_damage = true;
            require(trial.count_in() == 181, "Controlled shoe tool scene changed the full production count-in");
            require(trial.level->place_player(0, result.source.position()) &&
                (top || trial.level->place_player(1, result.target.position())), "Public pre-observation shoe placement rejected a real roster seat");
            trial.check();
            const auto verify = [&]
            {
                require(same_footing(trial.player(), result.source), "Controlled shoe source left its supported exact column/foot plane");
                if (!top)
                {
                    const auto& target = trial.level->player(1);
                    require(same_footing(target, result.target) && target.move_state() ==
                        (side_b ? PlayerMoveState::on_ramp_left : PlayerMoveState::on_ramp_right),
                        "Controlled shoe target left its actual hypotenuse support");
                }
            };
            for (int update = 0; update < 45; ++update)
            { trial.tick_inputs({ input() }); ++result.readiness; verify(); }
            while (trial.player().primary_weapon().readiness() != WeaponReadiness::ready && result.readiness < maximum_readiness)
            { trial.tick_inputs({ input() }); ++result.readiness; verify(); }
            require(trial.player().primary_weapon().readiness() == WeaponReadiness::ready && trial.level->projectile_count() == 0,
                "Controlled Sniper did not reach ordinary ready state without flight");
            const float source_health = trial.player().health();
            require(source_health == context.world.tuning.player().starting_health, "Controlled source lost health before its pulse");
            float target_ammo = -1.0f;
            if (!top)
            {
                result.target_initial_health = trial.level->player(1).health(); target_ammo = trial.level->player(1).weapon_ammo();
                require(result.target_initial_health == context.world.tuning.player().starting_health,
                    "Controlled target lost health during neutral readiness");
            }
            result.raw_slot = trial.player().player_num(); result.owner = paint_owners::index(result.raw_slot, 0);
            require(result.raw_slot == (top ? input_slots::keyboard_and_mouse : 3) && result.owner == result.raw_slot,
                "Controlled shoe credit lost its actual raw input slot");
            const auto shoe_source = paint_source(stage, side_b ? "shoulder_shoe_b" : "shoulder_shoe_a");
            require(shoe_source.has_value() == (which != ToolCase::removed_shoe_passes), "Controlled shoe presence differs from its experiment");
            require(credited(trial, shoe_source, result.owner) == std::array<double, 4>{}, "Shoe was painted before its single paid pulse");
            const Vector2F aim = top ? Vector2F(ramp.center().x, ramp.bottom()) : trial.level->player(1).center();
            result.angle = (aim - trial.player().center()).angle();
            const auto direction = Vector2F::unit_vec_from_angle(result.angle);
            result.ammo_initial = trial.player().weapon_ammo();
            const std::size_t wanted_face = top ? 0u : 1u;
            for (int update = 1; update <= observation_updates; ++update)
            {
                const float before = trial.player().weapon_ammo();
                trial.tick_inputs({ moving_structure_acceptance::aimed_input(direction, result.angle, update == 1) });
                verify();
                if (trial.player().weapon_ammo() < before)
                { ++result.paid; require(update == 1, "Released shoe-control Sniper paid for another pulse"); }
                if (update == 1)
                {
                    result.ammo_after = trial.player().weapon_ammo(); result.after_projectiles = trial.level->projectile_count();
                    require(result.paid == 1 && result.ammo_after < result.ammo_initial, "Shoe-control trigger did not pay exactly once");
                }
                result.peak_projectiles = (std::max)(result.peak_projectiles, trial.level->projectile_count());
                result.shoe = credited(trial, shoe_source, result.owner);
                if (result.shoe[wanted_face] > 0.0 && result.first_paint < 0) { result.first_paint = update; }
                require(trial.player().health() == source_health, "Controlled shoe source was damaged without opposing fire");
                if (!top)
                {
                    const auto& target = trial.level->player(1);
                    require(target.weapon_ammo() == target_ammo, "Neutral controlled ramp target fired a weapon");
                    if (target.health() < result.target_initial_health && result.first_damage < 0) { result.first_damage = update; }
                }
            }
            result.updates = trial.updates; result.source_health = trial.player().health();
            result.ammo_final = trial.player().weapon_ammo(); result.final_projectiles = trial.level->projectile_count();
            if (!top) { result.target_final_health = trial.level->player(1).health(); }
            require(result.updates == 181 + result.readiness + observation_updates && result.updates <= maximum_trial_updates &&
                result.paid == 1 && result.ammo_final >= result.ammo_after, "Controlled shoe clocks/pulse/recovery inventory differs");
            if (which == ToolCase::shoe_blocks)
            { require(result.shoe[1] > 0.0 && result.first_damage < 0 && result.target_final_health == result.target_initial_health,
                "Actual flat shoe did not paint its owned underside and block this target line"); }
            else if (which == ToolCase::removed_shoe_passes)
            { require(result.first_damage >= 1 && result.target_final_health > 0.0f && result.target_final_health < result.target_initial_health,
                "Matched removed-shoe control did not prove actual nonfatal health loss through the existing paint-pass ramp"); }
            else { require(result.shoe[0] > 0.0, "Solo controlled shot did not credit the reachable flat shoe top through its paint-pass ramp"); }
            result.terminal_next_rand = std::rand();
            return result;
        }

        void row(std::ofstream& csv, bool side_b, ToolCase which, const Result& value)
        {
            csv << (side_b ? 'b' : 'a') << ',' << case_name(which) << ',' << value.source.x << ',' << value.source.y << ',' <<
                value.source.bottom() << ',' << value.has_target << ',' << value.target.x << ',' << value.target.y << ',' << value.target.bottom() << ',' <<
                value.readiness << ",180," << value.updates << ',' << value.paid << ',' << value.first_damage << ',' << value.first_paint << ',' <<
                value.target_initial_health << ',' << value.target_final_health << ',' << value.source_health << ',' << value.ammo_initial << ',' <<
                value.ammo_after << ',' << value.ammo_final << ',' << value.angle << ',' << value.after_projectiles << ',' <<
                value.peak_projectiles << ',' << value.final_projectiles << ',' << value.raw_slot << ',' << value.owner;
            for (double area : value.shoe) { csv << ',' << area; }
            csv << ',' << value.terminal_next_rand << ",controlled-pre-observation-public-placement-no-earned-travel\n";
        }
    }

    void tool_checks(Context& context)
    {
        SourceGuard originals(context.content, context.content / "tuning.json", context.stage);
        std::ofstream csv(context.evidence / "shoulder-tools.csv"), preflight(context.evidence / "shoulder-top-station-preflight.csv");
        require(csv.good() && preflight.good(), "Cannot open controlled shoulder tool receipts");
        csv << std::setprecision(std::numeric_limits<double>::max_digits10) <<
            "side,experiment,source_x,source_y,source_feet,target_present,target_x,target_y,target_feet,readiness_updates,observation_updates,"
            "total_updates,paid_pulses,first_target_health_loss,first_wanted_face_paint,target_health_initial,target_health_final,source_health_final,"
            "ammo_initial,ammo_after_trigger,ammo_final,fixed_aim_angle,public_projectiles_after_trigger,public_peak_projectiles,public_final_projectiles,"
            "source_raw_slot,paint_owner,shoe_top_area,shoe_bottom_area,shoe_left_area,shoe_right_area,terminal_next_rand,scope\n";
        preflight << std::setprecision(std::numeric_limits<double>::max_digits10) <<
            "side,proposal,source_centre,source_feet,shot_width,shot_height,whole_body_fits,expanded_own_support_intersected,scope\n";
        int updates = 0, scenes = 0, paid = 0;
        for (const bool side_b : { false, true })
        {
            const auto removed = without_shoe(context, side_b);
            const RectangleF unused;
            const auto blocked = measure(context, context.stage, side_b, ToolCase::shoe_blocks, unused);
            const auto passed = measure(context, removed, side_b, ToolCase::removed_shoe_passes, unused);
            require(blocked.source == passed.source && blocked.target == passed.target && blocked.angle == passed.angle &&
                blocked.readiness == passed.readiness && blocked.ammo_initial == passed.ammo_initial && blocked.ammo_after == passed.ammo_after &&
                blocked.target_initial_health == passed.target_initial_health, "Shoe-present/deleted pair changed its controlled actors/aim/readiness/paid cost");
            row(csv, side_b, ToolCase::shoe_blocks, blocked); row(csv, side_b, ToolCase::removed_shoe_passes, passed);
            const auto top_source = top_station(context, side_b, preflight);
            const auto top = measure(context, context.stage, side_b, ToolCase::shoe_top, top_source);
            row(csv, side_b, ToolCase::shoe_top, top);
            updates += blocked.updates + passed.updates + top.updates; scenes += 3; paid += blocked.paid + passed.paid + top.paid;
            std::cout << "MEASURE controlled shoe tools side=" << (side_b ? 'b' : 'a') << " blocked_health=" << blocked.target_final_health <<
                " underside_area=" << blocked.shoe[1] << " pass_first_damage=" << passed.first_damage << " pass_health=" << passed.target_final_health <<
                " solo_top_area=" << top.shoe[0] << " top_source_centre=" << top_source.center().x << '\n';
        }
        require(scenes == 6 && paid == 6 && updates <= maximum_trial_updates * 6, "Controlled shoe experiment inventory exceeded its finite budget");
        originals.check(); csv.close(); preflight.close(); require(csv.good() && preflight.good(), "Cannot finish controlled shoe tool receipts");
        std::ofstream summary(context.evidence / "shoulder-tools-summary.json");
        summary << "{\"outcome\":\"passed\",\"scope\":\"controlled-shoe-blocking-ramp-pass-flat-top-paint\",\"seed\":" << seed <<
            ",\"actual_levelbuilder_scenes\":6,\"paired_opposed_scenes\":4,\"solo_top_scenes\":2,\"paid_sniper_pulses\":6,"
            "\"blocked_target_controls\":2,\"ramp_pass_health_positive_controls\":2,\"credited_top_controls\":2,\"total_updates\":" << updates <<
            ",\"maximum_trial_updates\":" << maximum_trial_updates << ",\"observation_updates_per_scene\":180,"
            "\"post_countin_pre_readiness_placements\":10,\"projectile_count_snapshots_only\":true,\"sloped_face_score_measured\":false,"
            "\"earned_travel_measured\":false,\"human_acceptance\":false}\n";
        summary.close(); require(summary.good(), "Cannot finish controlled shoulder tool summary");
        std::cout << "PASS6 actual public controlled Sniper scenes: mirrored shoe-block/ramp-pass health pairs and raw-slot4 flat-top paint; "
            "one paid pulse each, no sloped-face/OBB, earned-travel or human acceptance\n";
    }
}
