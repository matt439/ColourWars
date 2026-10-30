#include "game/pch.h"
#include "scripts/acceptance/moving_structure/fixtures.h"
#include "game/objects/fixed_slab_paintable.h"
#include "game/objects/level_builder.h"
#include "editor/blockout_aids.h"
#include "editor/level_document.h"
#include <array>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <rapidjson/prettywriter.h>

namespace fixed_slab_acceptance
{
    namespace
    {
        namespace fs = std::filesystem;
        using namespace mattmath;
        using namespace labrador;
        using namespace colourwars_editor;
        using moving_structure_acceptance::World;
        using moving_structure_acceptance::require;
        using moving_structure_acceptance::dt;

        std::string bytes(const fs::path& path)
        {
            std::ifstream stream(path, std::ios::binary);
            require(stream.good(), "Cannot read public slab input: " + path.string());
            std::string result{ std::istreambuf_iterator<char>(stream), {} };
            require(!stream.bad(), "Cannot finish public slab input: " + path.string());
            return result;
        }

        rapidjson::Document parsed(const std::string& text)
        {
            rapidjson::Document document;
            document.Parse(text.c_str());
            require(!document.HasParseError() && document.IsObject(), "Malformed public slab fixture JSON");
            return document;
        }

        void write(const fs::path& path, const std::string& text)
        {
            require(!fs::exists(path), "Refusing earlier public slab receipt: " + path.string());
            std::ofstream stream(path, std::ios::binary);
            require(stream.good(), "Cannot create public slab receipt: " + path.string());
            stream << text;
            stream.close();
            require(stream.good(), "Cannot finish public slab receipt: " + path.string());
        }

        std::string encoded(const rapidjson::Document& document)
        {
            rapidjson::StringBuffer buffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
            require(document.Accept(writer), "Cannot encode public slab fixture JSON");
            return buffer.GetString();
        }

        struct SourceGuard
        {
            std::vector<std::pair<fs::path, std::string>> originals;
            fs::path content;
            std::set<fs::path> levels;

            static std::set<fs::path> inventory(const fs::path& root)
            {
                const auto manifest = parsed(bytes(root / "manifest.json"));
                std::set<fs::path> expected, actual;
                for (const auto& asset : manifest["assets"].GetArray())
                {
                    if (std::string(asset["kind"].GetString()) != "level") { continue; }
                    for (const auto& name : asset["names"].GetArray())
                    {
                        const auto path = (fs::path(asset["directory"].GetString()) /
                            (std::string(name.GetString()) + ".json")).lexically_normal();
                        require(expected.insert(path).second, "Public slab guard found a duplicate manifest level");
                    }
                }
                for (const auto& file : fs::directory_iterator(root / "levels"))
                {
                    if (file.is_regular_file() && file.path().extension() == ".json")
                    { actual.insert(file.path().lexically_relative(root).lexically_normal()); }
                }
                require(actual == expected, "Public slab guard needs exact manifest/filesystem level inventory");
                for (const char* name : { "king_of_the_hill", "turbulence", "close_quarters", "movement_course",
                    "crossfeed", "press_tower", "turbulence_blockout", "turbulence_mover_trial" })
                {
                    require(expected.contains(fs::path("levels") / (std::string(name) + ".json")),
                        "Public slab guard lost an original arena");
                }
                return expected;
            }

            explicit SourceGuard(const fs::path& root) : content(root.lexically_normal()), levels(inventory(content))
            {
                for (const char* file : { "manifest.json", "tuning.json", "presentation.json" })
                { originals.emplace_back(content / file, bytes(content / file)); }
                for (const auto& file : levels) { originals.emplace_back(content / file, bytes(content / file)); }
                require(originals.size() == levels.size() + 3, "Public slab guard lost startup or registered-level byte snapshots");
            }
            void check() const
            {
                for (const auto& [path, original] : originals)
                { require(bytes(path) == original, "Public slab check changed source content: " + path.string()); }
                require(inventory(content) == levels, "Public slab check changed the source level inventory");
            }
        };

        using FaceAreas = std::array<double, 4>;
        using Budget = std::vector<FaceAreas>;

        std::size_t face_index(bool along_x, bool leading)
        { return along_x ? (leading ? 0u : 1u) : (leading ? 2u : 3u); }

        Budget editor_budget(const LevelDocument& document, const rapidjson::Document& source,
            const BlockoutTuning& tuning)
        {
            std::map<std::size_t, std::size_t> ordinals;
            std::size_t ordinal = 0;
            const auto& objects = source["collision_objects"];
            for (rapidjson::SizeType index = 0; index < objects.Size(); ++index)
            {
                const std::string type = objects[index]["type"].GetString();
                if (type == "StructurePaintable" || type == "FixedSlabPaintable")
                { ordinals.emplace(index, ordinal++); }
            }
            Budget result(ordinal);
            for (const auto& face : scoring_faces(document, tuning))
            {
                require(face.entry.kind == EntryKind::collision && ordinals.contains(face.entry.index),
                    "Editor slab budget lost a runtime paint-source ordinal");
                result[ordinals.at(face.entry.index)][face_index(face.along_x, face.leading)] +=
                    face.strip.width * face.strip.height;
            }
            return result;
        }

        double compare_budget(const Budget& expected, const std::vector<PaintSurfaceSample>& samples)
        {
            Budget actual(expected.size());
            double total = 0.0;
            for (const auto& sample : samples)
            {
                require(sample.source_index < actual.size(), "Public collector added an unexpected slab paint source");
                require(sample.team == PlayerTeam::none && sample.paint_owner == -1,
                    "Neutral public slab load began with owned or credited paint");
                require(std::isfinite(sample.scored_area) && sample.scored_area > 0.0f,
                    "Public slab tile has invalid authoritative local area");
                for (const auto& point : sample.corners)
                { require(std::isfinite(point.x) && std::isfinite(point.y), "Public slab tile has nonfinite corners"); }
                actual[sample.source_index][static_cast<std::size_t>(sample.face)] += sample.scored_area;
                total += sample.scored_area;
            }
            for (std::size_t source = 0; source < expected.size(); ++source)
            {
                for (std::size_t face = 0; face < expected[source].size(); ++face)
                {
                    require(std::abs(actual[source][face] - expected[source][face]) <= 0.05,
                        "Editor/public slab source-face area differs at source " + std::to_string(source) +
                        " face " + std::to_string(face));
                }
            }
            return total;
        }

        MatchSetup setup(int count)
        {
            MatchSetup result;
            result.set_stage("turbulence_slab_trial"); result.set_player_count(count);
            result.set_screen_layout(count == 1 ? ScreenLayout::one_player : ScreenLayout::four_player);
            constexpr std::array<int, 4> slots{ 3, 1, 2, 0 };
            result.begin_input_claims(count == 1 ? input_slots::keyboard_and_mouse : slots[0]);
            for (int view = 1; view < count; ++view)
            { require(result.claim_input_slot(slots[static_cast<std::size_t>(view)]), "Public slab roster claim failed"); }
            for (int view = 0; view < count; ++view)
            {
                result.set_player_team(view, view % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
                result.set_player_weapon(view, WeaponType::sprayer);
            }
            return result;
        }

        double public_load(World& world, const fs::path& stage, const Budget& expected, int count)
        {
            Registry<LevelLoadedInfo> infos{ "Fixed slab public acceptance" };
            auto info = std::make_unique<LevelLoadedInfo>(path_to_utf8(stage).c_str());
            info->validate();
            LevelObjectBuilder objects(&world.render, &world.audio, &world.preferences, &world.tuning.paint());
            info->validate_content(objects, world.render, world.audio);
            infos.add("turbulence_slab_trial", std::move(info));
            ResolutionManager resolution;
            ViewportManager viewports{ &resolution };
            Mouse mouse;
            Partitioner partitioner;
            auto settings = setup(count); viewports.set_layout(settings.screen_layout());
            LevelBuilder builder(&viewports, &world.render, &world.audio, &world.preferences, &infos,
                &world.tuning, &world.presentation, &resolution, nullptr, &partitioner);
            std::srand(260026u);
            const auto level = builder.build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
            require(level->player_count() == count, "Public slab load lost a roster seat");
            const double total = compare_budget(expected, level->paint_surfaces());
            const auto initial = level->horizontal_structure_states();
            require(initial.size() == 2, "Public slab comparison changed the two original movers");
            class Neutral final : public SeatInputFilter
            {
                void filter_seats(std::vector<PlayerInputData>& seats, const Level&) override
                {
                    for (auto& seat : seats) { seat = {}; seat.connected = true; }
                }
            } neutral;
            int count_in = 0;
            while (!level->match_started() && count_in < 240)
            { level->update({}, dt, &neutral); ++count_in; }
            require(count_in == 181 && level->match_started(), "Public slab load changed source-tuned count-in");
            for (int tick = 0; tick < 12; ++tick) { level->update({}, dt, &neutral); }
            require(compare_budget(expected, level->paint_surfaces()) == total,
                "Neutral public slab stepping changed the actual scoring budget");
            for (int view = 0; view < count; ++view)
            {
                const Player& player = level->player(view);
                require(player.state() == PlayerState::alive && player.health() == 1.0f,
                    "Public slab count-in lost or damaged a player");
            }
            require(level->horizontal_structure_states().size() == initial.size(),
                "Public slab stepping lost an original mover");
            return total;
        }

        template<class Edit>
        void refuse_variant(World& world, const fs::path& content, const fs::path& evidence,
            const std::string& candidate, const char* label, Edit edit)
        {
            auto variant = parsed(candidate); edit(variant);
            const auto path = evidence / (std::string("refused-") + label + ".json");
            write(path, encoded(variant));
            bool editor_refused = false, runtime_refused = false;
            try { LevelDocument document(content / "tuning.json"); document.load(path); }
            catch (const std::exception&) { editor_refused = true; }
            try
            {
                LevelLoadedInfo info(path_to_utf8(path).c_str()); info.validate();
                LevelObjectBuilder builder(&world.render, &world.audio, &world.preferences, &world.tuning.paint());
                info.validate_content(builder, world.render, world.audio);
            }
            catch (const std::exception&) { runtime_refused = true; }
            require(editor_refused && runtime_refused,
                std::string("Editor/runtime fixed slab refusal differs for ") + label);
        }
    }

    void authoring_checks(const std::filesystem::path& content, const std::filesystem::path& evidence)
    {
        require(fs::is_directory(evidence) && fs::is_empty(evidence), "Public slab receipts require a fresh empty directory");
        SourceGuard originals(content);
        const auto source_path = content / "levels/turbulence_mover_trial.json";
        const std::string source_bytes = bytes(source_path);
        auto original = parsed(source_bytes);
        require(original["collision_objects"].Size() == 22, "Public slab source must preserve the qualified 22-object mover comparison");
        LevelDocument document(content / "tuning.json"); document.load(source_path);
        const auto a = document.add_object("FixedSlabRight", { 1240.0, 880.0, 1.0, 1.0 });
        const auto b = document.add_object("FixedSlabLeft", { 2600.0, 800.0, 1.0, 1.0 });
        document.set_property(a, "name", "fixed_slab_a");
        document.set_property(b, "name", "fixed_slab_b");
        const auto stage = evidence / "turbulence_slab_trial.json";
        document.save(stage);
        auto authored = parsed(bytes(stage));
        require(authored["collision_objects"].Size() == 24, "Public slab authoring changed the bounded object inventory");
        for (rapidjson::SizeType index = 0; index < 22; ++index)
        { require(authored["collision_objects"][index] == original["collision_objects"][index], "Public slab authoring changed an existing object"); }
        for (auto field = original.MemberBegin(); field != original.MemberEnd(); ++field)
        {
            if (std::string(field->name.GetString()) != "collision_objects")
            { require(authored.HasMember(field->name) && authored[field->name] == field->value, "Public slab authoring changed source stage metadata or spawns"); }
        }
        authored["collision_objects"][22].AddMember("future_object", "preserved", authored.GetAllocator());
        authored["collision_objects"][22]["slab"].AddMember("future_slab", "preserved", authored.GetAllocator());
        const auto metadata_stage = evidence / "turbulence_slab_metadata.json";
        write(metadata_stage, encoded(authored));
        LevelDocument reopened(content / "tuning.json"); reopened.load(metadata_stage);
        const auto roundtrip = evidence / "turbulence_slab_roundtrip.json"; reopened.save(roundtrip);
        const auto retained = parsed(bytes(roundtrip));
        require(retained["collision_objects"][22]["future_object"] == authored["collision_objects"][22]["future_object"] &&
            retained["collision_objects"][22]["slab"]["future_slab"] == authored["collision_objects"][22]["slab"]["future_slab"],
            "Public slab round trip lost unknown metadata");
        World world(content);
        // This case compares geometry, not the separately attended/measured
        // movement-baseline aids. Read the actual source tuning already loaded.
        const BlockoutTuning tuning{ world.tuning.player().size.x, world.tuning.player().size.y,
            world.tuning.paint().thickness, true };
        const Budget expected = editor_budget(reopened, retained, tuning);
        const double total1 = public_load(world, roundtrip, expected, 1);
        const double total4 = public_load(world, roundtrip, expected, 4);
        require(total1 == total4, "Fresh public 1/4-player slab loads have different paint budgets");
        const double diagonal = 80.0 * std::sqrt(5.0);
        const double independent_total = 165120.0 + 2.0 * (12.0 * (2.0 * diagonal + 12.0) - 12.0 * 12.0 * std::sqrt(5.0));
        require(std::abs(total1 - independent_total) <= 0.15,
            "Public slab total differs from independent local-edge and reciprocal crossover clipping census");
        const auto candidate = bytes(roundtrip);
        refuse_variant(world, content, evidence, candidate, "rise", [](auto& v) { v["collision_objects"][22]["slab"]["rise"].SetString("up"); });
        refuse_variant(world, content, evidence, candidate, "bounds", [](auto& v) { v["collision_objects"][22]["slab"]["x"].SetInt(65500); });
        refuse_variant(world, content, evidence, candidate, "material", [](auto& v) { v["collision_objects"][22]["material"].SetString("pencil"); });
        refuse_variant(world, content, evidence, candidate, "motion", [](auto& v) { v["collision_objects"][22].AddMember("motion", rapidjson::Value(rapidjson::kObjectType), v.GetAllocator()); });
        refuse_variant(world, content, evidence, candidate, "rotation", [](auto& v) { v["collision_objects"][22].AddMember("rotation", 0.3, v.GetAllocator()); });
        refuse_variant(world, content, evidence, candidate, "thickness", [](auto& v) { v["collision_objects"][22]["slab"].AddMember("thickness", 12, v.GetAllocator()); });
        refuse_variant(world, content, evidence, candidate, "overlap", [](auto& v)
        {
            v["collision_objects"][23]["slab"]["x"].SetInt(1240);
            v["collision_objects"][23]["slab"]["y"].SetInt(880);
            v["collision_objects"][23]["slab"]["rise"].SetString("right");
        });
        originals.check();
        std::ostringstream summary;
        summary << std::setprecision(12) << "{\"outcome\":\"passed\",\"scope\":\"editor-save-public-LevelBuilder\","
            "\"objects\":24,\"unchanged_original_objects\":22,\"fresh_levels\":2,\"player_counts\":[1,4],"
            "\"count_in_updates_per_level\":181,\"neutral_updates_per_level\":12,\"total_level_updates\":386,"
            "\"editor_runtime_refusal_pairs\":7,\"total_scored_area\":" << total1 << ",\"analytic_total\":" << independent_total << ","
            "\"per_source_face_tolerance\":0.05,\"analytic_total_tolerance\":0.15,\"registered\":false,"
            "\"driven_routes_measured\":false,\"native_rendering_measured\":false,\"human_acceptance\":false}\n";
        write(evidence / "slab-public-summary.json", summary.str());
        std::cout << "PASS public slab authoring: real editor save/reopen preserves22 original objects/spawns/metadata, fresh1/4 LevelBuilder loads+386 updates, "
            "actual source-face scoring and7 paired refusal controls; total=" << total1 << " receipts=" << evidence.string() << '\n';
    }
}
