#include "game/pch.h"
#include "scripts/acceptance/turbulence_movers/fixtures.h"
#include <iostream>
#include <iomanip>
#include <set>

namespace turbulence_movers_acceptance
{
    namespace
    {
        rapidjson::Document document(const fs::path& path)
        {
            rapidjson::Document result;
            const auto text = read_text(path); result.Parse(text.c_str());
            require(!result.HasParseError() && result.IsObject(), "Cannot decode mover-trial source comparison");
            return result;
        }
    }

    void geometry(Context& context)
    {
        const auto baseline_path = context.content / "levels/turbulence_blockout.json";
        auto baseline = document(baseline_path);
        auto candidate = document(context.stage);
        require(baseline["collision_objects"].Size() == 20 && candidate["collision_objects"].Size() == 22 &&
            candidate["include_in_random"].IsBool() && !candidate["include_in_random"].GetBool(),
            "Mover trial must be an explicit-only two-object extension of the static skeleton");
        for (rapidjson::SizeType index = 0; index < 20; ++index)
        { require(candidate["collision_objects"][index] == baseline["collision_objects"][index], "Mover trial altered a static collision object"); }
        // POL#40: the trial keeps the blockout's stations, in order, and adds
        // only the two ends of each mover.
        const auto& shared = baseline["stations"];
        const auto& stations = candidate["stations"];
        require(shared.Size() == 10 && stations.Size() == 14, "Mover trial must add four mover-end stations to the ten shared");
        for (rapidjson::SizeType index = 0; index < shared.Size(); ++index)
        { require(stations[index] == shared[index], "Mover trial altered a shared station"); }
        for (rapidjson::SizeType index = shared.Size(); index < stations.Size(); ++index)
        {
            require(std::string(stations[index]["name"].GetString()).rfind("mover_", 0) == 0,
                "Mover trial adds a station that is not a mover end");
        }
        baseline.RemoveMember("display_name"); baseline.RemoveMember("include_in_random"); baseline.RemoveMember("collision_objects");
        baseline.RemoveMember("stations");
        auto candidate_metadata = document(context.stage);
        candidate_metadata.RemoveMember("display_name"); candidate_metadata.RemoveMember("include_in_random");
        candidate_metadata.RemoveMember("collision_objects"); candidate_metadata.RemoveMember("stations");
        require(candidate_metadata == baseline, "Mover trial altered static spawns, bounds, resources or unrelated metadata");

        Trial static_level(context, baseline_path, settings());
        require(static_level.level->level_end_info().total_area == 148800.0f && !static_level.level->has_horizontal_motion(),
            "Preserved static Turbulence score or motion baseline changed");
        std::ofstream budget(context.evidence / "mover-budget.csv");
        require(budget.good(), "Cannot open mover budget receipt");
        budget << "players,total,top,bottom,left,right,mover_local,upper_role,upper_share,count_in\n";
        for (int count : { 1, 4 })
        {
            Trial trial(context, context.stage, settings(count));
            const auto initial = trial.level->horizontal_structure_states();
            require(initial.size() == 2 && initial[0].id == 1 && initial[1].id == 2 &&
                initial[0].current_bounds == mattmath::RectangleF(600, 720, 320, 20) &&
                initial[1].current_bounds == mattmath::RectangleF(3080, 720, 320, 20),
                "Authored paired mover phases/IDs/initial world rectangles changed");
            const auto first_bits = moving_structure_acceptance::motion_bits(initial[0].motion);
            trial.tick(input(), nullptr, 0.0f);
            require(moving_structure_acceptance::motion_bits(trial.mover(false).motion) == first_bits,
                "Trial zero-step changed the authored initial mover phase/history");
            double mover_area = 0.0;
            for (const auto& state : initial)
            {
                require(state.motion.spec.travel_x == 320.0f && state.motion.spec.period_seconds == 4.0f,
                    "Trial mover no longer follows its bounded travel/period proposal");
                for (const auto& tile : state.local_paint) { mover_area += tile.scored_area; }
            }
            std::set<size_t> upper_sources;
            size_t source = 0;
            for (const auto& object : candidate["collision_objects"].GetArray())
            {
                if (std::string(object["type"].GetString()) != "StructurePaintable") { continue; }
                const std::string name = object["name"].GetString();
                if (name.starts_with("crossover_") || name == "centre_baffle" || name.starts_with("mover_"))
                { upper_sources.insert(source); }
                ++source;
            }
            std::array<double, 4> faces{};
            double total = 0.0, upper = 0.0;
            for (const auto& sample : trial.level->paint_surfaces())
            {
                total += sample.scored_area;
                faces[static_cast<size_t>(sample.face)] += sample.scored_area;
                if (upper_sources.contains(sample.source_index)) { upper += sample.scored_area; }
            }
            require(total == 165120.0 && trial.level->level_end_info().total_area == 165120.0f &&
                mover_area == 16320.0 && upper == 48960.0 && faces[0] == 111360.0 && faces[1] == 37440.0 &&
                faces[2] + faces[3] == 16320.0, "Trial actual local/face/upper score budget differs from the explicit all-face proposal");
            const int count_in = trial.count_in();
            require(count_in == 181 && trial.level->player_count() == count,
                "Public trial changed source count-in or actual requested roster count");
            for (int step = 0; step < 12; ++step) { trial.tick(); }
            const auto states = trial.level->horizontal_structure_states();
            require(states[0].motion.phase != initial[0].motion.phase && trial.level->level_end_info().total_area == 165120.0f,
                "Actual Level trial did not advance motion or changed locally attached score");
            constexpr std::array<int, 4> sparse{ 3, 1, 2, 0 };
            for (int view = 0; view < count; ++view)
            { require(trial.level->player(view).player_num() == (count == 1 ? input_slots::keyboard_and_mouse : sparse[static_cast<size_t>(view)]),
                "Public trial changed sparse physical-slot association"); }
            budget << count << ',' << total << ',' << faces[0] << ',' << faces[1] << ',' << faces[2] << ',' << faces[3] << ',' <<
                mover_area << ',' << upper << ',' << std::setprecision(12) << upper / total << ',' << count_in << '\n';
        }
        budget.close(); require(budget.good(), "Cannot finish mover budget receipt");
        std::cout << "PASS isolated mover proposal: preserved static skeleton, source-tuned public 1/4 loads, 16320 added local score, "
            "165120 total and upper-role share 48960/165120\n";
    }
}
