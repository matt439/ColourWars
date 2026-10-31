#include "game/pch.h"
#include "scripts/acceptance/turbulence_slab_stage/fixtures.h"
#include "game/objects/level_builder.h"
#include "game/objects/stage.h"
#include <array>
#include <iomanip>
#include <iostream>
#include <set>

namespace turbulence_slab_stage_acceptance
{
    namespace
    {
        using namespace mattmath;
        using namespace labrador;
        using moving_structure_acceptance::World;
        constexpr const char* stage_name = "turbulence_slab_trial";
        constexpr int maximum_level_updates = 1000;

        std::string read(const fs::path& path)
        {
            std::ifstream file(path, std::ios::binary);
            require(file.good(), "Cannot read registered slab qualification input: " + path.string());
            std::string bytes{ std::istreambuf_iterator<char>(file), {} };
            require(!file.bad(), "Cannot finish registered slab qualification input: " + path.string());
            return bytes;
        }

        rapidjson::Document parse(const std::string& bytes)
        {
            rapidjson::Document document; document.Parse(bytes.c_str());
            require(!document.HasParseError() && document.IsObject(), "Malformed registered slab source JSON");
            return document;
        }

        struct Guard
        {
            std::vector<std::pair<fs::path, std::string>> bytes;
            void add(const fs::path& path) { bytes.emplace_back(path, read(path)); }
            void check() const
            {
                for (const auto& [path, original] : bytes)
                { require(read(path) == original, "Read-only slab qualification changed source/staged content: " + path.string()); }
            }
        };

        void check_stage_source(const fs::path& content, const fs::path& repo)
        {
            const auto stage = parse(read(content / "levels/turbulence_slab_trial.json"));
            const auto original = parse(read(content / "levels/turbulence_mover_trial.json"));
            require(stage["collision_objects"].Size() == 24 && original["collision_objects"].Size() == 22,
                "Registered slab stage lost its qualified24/22 object inventory");
            for (rapidjson::SizeType index = 0; index < 22; ++index)
            { require(stage["collision_objects"][index] == original["collision_objects"][index], "Registered slab stage changed an original object"); }
            for (auto field = original.MemberBegin(); field != original.MemberEnd(); ++field)
            {
                const std::string name = field->name.GetString();
                if (name == "display_name" || name == "collision_objects") { continue; }
                require(stage.HasMember(field->name) && stage[field->name] == field->value,
                    "Registered slab stage changed original spawns/non-display metadata");
            }
            require(std::string(stage["display_name"].GetString()) == "Turbulence Yard - Slab Trial" &&
                stage.HasMember("include_in_random") && !stage["include_in_random"].GetBool(),
                "Registered slab stage must retain its explicit-only display identity");
            for (rapidjson::SizeType index = 22; index < 24; ++index)
            {
                const auto& object = stage["collision_objects"][index];
                const bool a = index == 22;
                require(std::string(object["type"].GetString()) == "FixedSlabPaintable" &&
                    std::string(object["collision_type"].GetString()) == "STRUCTURE_FIXED_SLAB" &&
                    std::string(object["name"].GetString()) == (a ? "fixed_slab_a" : "fixed_slab_b") &&
                    object["slab"]["x"].GetFloat() == (a ? 1240.0f : 2600.0f) &&
                    object["slab"]["y"].GetFloat() == (a ? 880.0f : 800.0f) &&
                    std::string(object["slab"]["rise"].GetString()) == (a ? "right" : "left"),
                    "Registered slab stage changed the qualified fixed pair");
            }
            const auto runtime = read(repo / "cmake/runtime_assets.cmake");
            const std::string token = "levels/turbulence_slab_trial.json";
            const auto position = runtime.find(token);
            require(position != std::string::npos && runtime.find(token, position + token.size()) == std::string::npos,
                "Runtime asset list must package the registered slab stage exactly once");
        }

        StageList catalog(World& world, const fs::path& content, const fs::path& repo,
            Registry<LevelLoadedInfo>& infos, Guard& guard)
        {
            const auto manifest = read(content / "manifest.json");
            require(manifest == read(repo / "game/content/manifest.json"), "Staged and source registered manifests differ");
            guard.add(content / "manifest.json"); guard.add(repo / "game/content/manifest.json");
            guard.add(repo / "cmake/runtime_assets.cmake");
            for (const char* file : { "tuning.json", "presentation.json" })
            {
                require(read(content / file) == read(repo / "game/content" / file), "Registered stage test needs current source tuning/presentation");
                guard.add(content / file); guard.add(repo / "game/content" / file);
            }
            const auto document = parse(manifest);
            StageList result;
            std::set<std::string> unique;
            LevelObjectBuilder builder(&world.render, &world.audio, &world.preferences, &world.tuning.paint());
            for (const auto& group : document["assets"].GetArray())
            {
                if (std::string(group["kind"].GetString()) != "level") { continue; }
                for (const auto& value : group["names"].GetArray())
                {
                    const std::string name = value.GetString();
                    require(unique.insert(name).second, "Registered stage catalog contains duplicate level names");
                    const auto relative = fs::path("levels") / (name + ".json");
                    const auto staged_path = content / relative;
                    const auto source_path = repo / "game/content" / relative;
                    require(read(staged_path) == read(source_path), "A registered stage source/staged file differs");
                    guard.add(staged_path); guard.add(source_path);
                    auto info = std::make_unique<LevelLoadedInfo>(path_to_utf8(staged_path).c_str());
                    info->validate(); info->validate_content(builder, world.render, world.audio);
                    result.push_back({ name, info->display_name(), info->icon_frame(), info->include_in_random() });
                    infos.add(name, std::move(info));
                }
            }
            require(result.size() == 11, "Registered slab catalog must expose eleven explicit stages");
            const auto random = random_stage_indices(result);
            require(random.size() == 4, "Registered slab comparison must preserve the four accepted Random arenas");
            bool found = false;
            for (std::size_t index = 0; index < result.size(); ++index)
            {
                if (result[index].name != stage_name) { continue; }
                require(!found && !result[index].include_in_random &&
                    result[index].display_name == "Turbulence Yard - Slab Trial" &&
                    std::find(random.begin(), random.end(), index) == random.end(),
                    "Registered slab stage leaked into Random or lost explicit selection");
                found = true;
            }
            require(found, "Registered slab stage is absent from the actual manifest catalog");
            for (std::size_t draw = 0; draw < 16; ++draw)
            { require(result[random_stage_index(result, draw)].name != stage_name, "Random mapping returned the explicit-only slab trial"); }
            return result;
        }

        MatchSetup setup(int count)
        {
            MatchSetup settings; settings.set_stage(stage_name); settings.set_player_count(count);
            constexpr std::array<ScreenLayout, 4> layouts{ ScreenLayout::one_player, ScreenLayout::two_player,
                ScreenLayout::three_player, ScreenLayout::four_player };
            settings.set_screen_layout(layouts[static_cast<std::size_t>(count - 1)]);
            constexpr std::array<int, 4> slots{ 3, 1, 2, 0 };
            settings.begin_input_claims(count == 1 ? input_slots::keyboard_and_mouse : slots[0]);
            for (int view = 1; view < count; ++view)
            { require(settings.claim_input_slot(slots[static_cast<std::size_t>(view)]), "Registered slab roster lost sparse slot claims"); }
            for (int view = 0; view < count; ++view)
            {
                settings.set_player_team(view, view % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
                settings.set_player_weapon(view, view % 2 == 0 ? WeaponType::roller : WeaponType::sniper);
            }
            settings.set_duration(2.0f);
            return settings;
        }

        int short_match(World& world, const Registry<LevelLoadedInfo>& infos, int count, std::ofstream& csv)
        {
            ResolutionManager resolution; ViewportManager viewports{ &resolution }; Mouse mouse; Partitioner partitioner;
            const auto settings = setup(count); viewports.set_layout(settings.screen_layout());
            LevelBuilder builder(&viewports, &world.render, &world.audio, &world.preferences, &infos,
                &world.tuning, &world.presentation, &resolution, nullptr, &partitioner);
            const auto level = builder.build_level(settings, TeamColourPair::blue_orange, OpeningMode::off,
                MatchRandom(260026u));
            require(level->player_count() == count && level->state() == LevelState::start_countdown,
                "Public registered slab build lost its cold roster/count-in");
            std::set<std::size_t> moving_sources;
            std::size_t ordinal = 0;
            const auto objects = infos.get(stage_name)->collision_objects_json();
            for (std::size_t index = 0; index < objects.size(); ++index)
            {
                const auto object = objects.at(index);
                const auto type = object.string("type");
                if (type != "StructurePaintable" && type != "FixedSlabPaintable") { continue; }
                if (object.has("motion")) { moving_sources.insert(ordinal); }
                ++ordinal;
            }
            const auto static_paint = [&]
            {
                auto samples = level->paint_surfaces();
                std::erase_if(samples, [&](const PaintSurfaceSample& sample) { return moving_sources.contains(sample.source_index); });
                return moving_structure_acceptance::sample_bits(samples);
            };
            const auto initial = static_paint();
            const auto initial_movers = level->horizontal_structure_states();
            require(moving_sources.size() == 2 && initial_movers.size() == 2,
                "Registered slab short match lost the existing moving paint roles");
            const float total = level->level_end_info().total_area;
            const double independent_total = 165120.0 + 2.0 *
                (12.0 * (160.0 * std::sqrt(5.0) + 12.0) - 144.0 * std::sqrt(5.0));
            require(std::isfinite(total) && std::abs(total - independent_total) <= 0.15,
                "Registered slab public score differs from its qualified local-face/reciprocal-clipping budget");
            class Neutral final : public SeatInputFilter
            {
                void filter_seats(std::vector<PlayerInputData>& seats, const Level&) override
                { for (auto& input : seats) { input = {}; input.connected = true; } }
            } neutral;
            int updates = 0, active_at = -1, time_up_at = -1, overview_at = -1;
            while (level->state() != LevelState::finished && updates < maximum_level_updates)
            {
                level->update({}, moving_structure_acceptance::dt, &neutral); ++updates;
                const auto state = level->state();
                if (state == LevelState::active && active_at < 0) { active_at = updates; }
                if (state == LevelState::zoom_out && time_up_at < 0) { time_up_at = updates; }
                if (state == LevelState::overview && overview_at < 0) { overview_at = updates; }
                require(level->player_count() == count && viewports.layout() == settings.screen_layout(),
                    "Registered short-match transition changed roster/shared pane layout");
                require(level->projectile_count() == 0, "Neutral registered short match unexpectedly fired a tool");
                const auto end = level->level_end_info();
                require(end.total_area == total && end.team_a_area == 0.0f && end.team_b_area == 0.0f &&
                    end.team_a_played && end.team_b_played == (count > 1) && end.one_team_only() == (count == 1) &&
                    end.coverage_ratio() == 0.0f, "Registered short ending lost neutral coverage/team participation preconditions");
                for (int view = 0; view < count; ++view)
                {
                    const auto& player = level->player(view); const auto& camera = player.camera();
                    require(player.state() == PlayerState::alive && player.health() == 1.0f &&
                        std::isfinite(player.rectangle().x) && std::isfinite(player.rectangle().y) &&
                        std::isfinite(camera.translation.x) && std::isfinite(camera.translation.y) &&
                        std::isfinite(camera.scale) && camera.scale > 0.0f,
                        "Registered short match lost finite live-player/camera preconditions");
                }
            }
            require(level->state() == LevelState::finished && active_at == 181 && time_up_at > active_at &&
                overview_at > time_up_at && updates > overview_at, "Registered slab short match did not traverse count-in/time-up/overview/finish within its bound");
            require(initial == static_paint(), "Neutral registered ending changed static/slab paint geometry or ownership");
            const auto final_movers = level->horizontal_structure_states();
            require(final_movers.size() == initial_movers.size(), "Registered short ending lost an original mover");
            for (std::size_t index = 0; index < final_movers.size(); ++index)
            {
                require(initial_movers[index].id == final_movers[index].id &&
                    moving_structure_acceptance::sample_bits(initial_movers[index].local_paint) ==
                    moving_structure_acceptance::sample_bits(final_movers[index].local_paint),
                    "Neutral registered ending changed authoritative mover-local paint/ownership");
            }
            csv << count << ',' << active_at << ',' << time_up_at << ',' << overview_at << ',' << updates << ',' << total <<
                ",0,0,1," << (count > 1) << ',' << (count == 1) << '\n';
            csv.flush(); require(csv.good(), "Cannot flush registered slab ending receipt");
            std::cout << "MEASURE registered slab " << count << "P active=" << active_at << " time_up=" << time_up_at <<
                " overview=" << overview_at << " finished=" << updates << " total=" << total << '\n';
            return updates;
        }
    }

    void qualify(const fs::path& content, const fs::path& repository, const fs::path& evidence)
    {
        World world(content); Registry<LevelLoadedInfo> infos{ "Registered slab acceptance stages" }; Guard guard;
        const auto stages = catalog(world, content, repository, infos, guard);
        check_stage_source(content, repository);
        std::ofstream csv(evidence / "registered-slab-short-endings.csv");
        require(csv.good(), "Cannot create registered slab ending CSV");
        csv << std::setprecision(12) << "players,count_in_updates,time_up_update,overview_update,finished_update,total_area,"
            "team_a_area,team_b_area,team_a_played,team_b_played,one_team_only\n";
        int updates = 0;
        for (int count = 1; count <= 4; ++count) { updates += short_match(world, infos, count, csv); }
        guard.check();
        std::ofstream summary(evidence / "registered-slab-summary.json");
        summary << "{\"outcome\":\"passed\",\"scope\":\"registered-public-LevelBuilder-short-endings-CPU\","
            "\"explicit_stages\":" << stages.size() << ",\"random_stages\":4,\"fresh_levels\":4,\"player_counts\":[1,2,3,4],"
            "\"actual_level_updates\":" << updates << ",\"maximum_level_updates\":4000,\"source_objects\":24,\"preserved_original_objects\":22,"
            "\"original_movers\":2,\"random_eligible\":false,\"source_staged_identity\":true,\"source_writes\":false,"
            "\"native_ending_pixels_measured\":false,\"human_balance_acceptance\":false}\n";
        summary.close(); require(summary.good(), "Cannot finish registered slab qualification summary");
    }
}
