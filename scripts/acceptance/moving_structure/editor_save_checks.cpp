#include "game/pch.h"
#include "game/debug/dev_tools.h"
#include "scripts/acceptance/moving_structure/loader_fixture.h"
#include "editor/level_document.h"
#include "editor/blockout_aids.h"
#include <iostream>
#include <process.h>

#if COLOURWARS_DEV_TOOLS
#include "game/debug/replay.h"
#endif

namespace moving_structure_acceptance
{
    void check_editor_save(World& world)
    {
        using colourwars_editor::Entry;
        using colourwars_editor::EntryKind;
        using colourwars_editor::LevelDocument;
        const auto directory = std::filesystem::temp_directory_path() /
            (L"colourwars_editor_moving_runtime_" + std::to_wstring(_getpid()));
        require(!std::filesystem::exists(directory), "Refusing to overwrite earlier editor/runtime receipts");
        std::filesystem::create_directories(directory);
        const auto source = directory / L"source_stage.json";
        const auto tuning_path = directory / L"source-tuning.json";
        const auto authored_path = directory / L"editor_authored_motion.json";
        std::cout << "Editor moving runtime receipts: " << labrador::path_to_utf8(directory) << '\n';
        write_loaded_stage(source);
        {
            std::ofstream output(tuning_path, std::ios::binary | std::ios::trunc);
            output << world.source;
            output.close(); require(output.good(), "Cannot retain source-derived editor player-size context");
        }
        LevelDocument editor(tuning_path);
        editor.load(source);
        const Entry first{ EntryKind::collision, 1, "" };
        editor.set_rectangle(first, { 980, 780, 360, 24 });
        editor.set_property(first, "motion/travel_x", "144");
        editor.set_property(first, "motion/phase", "0.375");
        const auto guide = *editor.motion_guide(first);
        const auto player_size = world.tuning.player().size;
        editor.set_rectangle(Entry{ EntryKind::spawn_a, 0, "" },
            { guide.initial_pose.x + guide.initial_pose.width / 2.0 - player_size.x / 2.0,
              guide.initial_pose.y - player_size.y, player_size.x, player_size.y });
        const Entry added = editor.add_object("MovingJumpThrough", { 2000, 900, 320, 20 });
        const Entry copied = editor.duplicate(added);
        require(editor.motion_guide(copied)->standing_envelope.y >=
            editor.motion_guide(added)->travel_envelope.y + editor.motion_guide(added)->travel_envelope.height,
            "Actual editor duplicate did not clear its source corridor");
        editor.save(authored_path);
        LevelDocument reopened(tuning_path);
        reopened.load(authored_path);
        require(reopened.serialize() == editor.serialize() && reopened.validate().empty(),
            "Actual editor save/reopen changed motion or invalidated its source-derived envelope");

        auto settings = loaded_settings();
        settings.set_stage("editor_authored_motion");
        constexpr unsigned int seed = 313u;
        auto runtime = std::make_unique<LoadedFixture>(world, authored_path, settings, seed);
        const auto states = runtime->level->horizontal_structure_states();
        require(states.size() == 4 && states[0].id == 1 && states[3].id == 4 &&
            states[0].motion.spec.anchor.x == 980.0f && states[0].motion.spec.travel_x == 144.0f &&
            states[0].motion.spec.initial_phase == 0.375f && states[0].current_bounds.x == guide.initial_pose.x &&
            states[0].current_bounds.y == guide.initial_pose.y,
            "Production LevelBuilder did not load the editor's anchor/travel/phase/duplicate");
        const colourwars_editor::BlockoutTuning blockout{
            player_size.x, player_size.y, world.tuning.paint().thickness, false };
        const double editor_area = colourwars_editor::scoring_area(colourwars_editor::scoring_faces(reopened, blockout));
        require(editor_area == runtime->level->level_end_info().total_area,
            "Editor counted a sweep, buried static paint or lost locally attached runtime paint");

#if COLOURWARS_DEV_TOOLS
        ReplayFile header;
        header.build = "editor-authored-moving-public";
        header.settings = settings; header.seed = seed; header.dt_bits = bits(dt);
        constexpr int updates = 180;
        ReplayRecorder recorder("editor-authored-moving", header, nullptr, updates);
        const auto initial = MatchFingerprint::of(*runtime->level, 0);
        std::vector<MatchFingerprint> prints;
        bool carried = false, paid_local = false;
        for (int step = 0; step < updates; ++step)
        {
            auto action = aimed_input({ 0.0f, 1.0f }, 1.5707963267948966f, step < 60);
            action.jump_pressed = step == 90; action.jump_held = step >= 90 && step < 102;
            runtime->tick(action, dt, &recorder);
            const auto print = MatchFingerprint::of(*runtime->level, step + 1);
            prints.push_back(print);
            carried |= print.horizontal_motion->carriers[0] == 1;
            for (const auto& tile : print.horizontal_motion->structures[0].tiles)
            {
                paid_local |= print.seats[0].ammo < initial.seats[0].ammo &&
                    tile.team == static_cast<int>(PlayerTeam::a) && tile.owner == input_slots::keyboard_and_mouse;
            }
            if ((step + 1) % ReplayFile::checkpoint_interval == 0) { recorder.add_checkpoint(print); }
        }
        require(carried && paid_local, "Editor-saved runtime never exercised actual carry and paid local-face credit");
        recorder.file().updates = updates; recorder.file().final_print = prints.back();
        const auto tape_path = directory / L"editor_authored_motion-replay.json";
        recorder.file().write(tape_path);
        const unsigned int terminal_rng = static_cast<unsigned int>(std::rand());
        runtime.reset();
        const auto decoded = ReplayFile::read(tape_path);
        require(decoded.seed && decoded.inputs.size() == 1 && decoded.inputs[0].size() == updates,
            "Editor-saved public replay lost its input or reconstruction header");
        LoadedFixture replay(world, directory / (decoded.settings.stage() + ".json"), decoded.settings, *decoded.seed);
        require(MatchFingerprint::difference(initial, MatchFingerprint::of(*replay.level, 0)).empty(),
            "Fresh public rebuild of editor-saved stage differs at initial admission");
        ReplayPlayer player("editor-authored-moving", decoded);
        size_t checkpoint = 0;
        for (int step = 0; step < decoded.updates; ++step)
        {
            replay.tick(input(), float_from_bits(decoded.dt_bits), &player);
            const auto actual = MatchFingerprint::of(*replay.level, step + 1);
            require(MatchFingerprint::difference(prints[static_cast<size_t>(step)], actual).empty(),
                "Public replay of editor-saved stage diverged at " + std::to_string(step + 1));
            if ((step + 1) % ReplayFile::checkpoint_interval == 0)
            {
                require(decoded.checkpoints[checkpoint].hash == actual.hash, "Editor-saved public checkpoint differs");
                ++checkpoint;
            }
        }
        require(MatchFingerprint::difference(decoded.final_print, MatchFingerprint::of(*replay.level, updates)).empty() &&
            static_cast<unsigned int>(std::rand()) == terminal_rng,
            "Editor-saved public replay final state or terminal RNG differs");
        std::cout << "PASS actual editor save -> public LevelBuilder -> seeded replay: 180 exact updates, local paint/credit, carry and terminal RNG\n";
#else
        std::cout << "PASS actual editor save -> public LevelBuilder anchor/phase/duplicate/material/scoring; replay is Debug-only\n";
#endif
        // Actual editor/runtime parity across a float binade. A moving
        // rectangle's represented world width can differ from its anchor
        // width; its owned local tile areas must remain exactly unchanged.
        rapidjson::Document fractional;
        {
            std::ifstream input(source, std::ios::binary);
            require(input.good(), "Cannot read source for fractional editor/runtime receipt");
            const std::string text{ std::istreambuf_iterator<char>(input), {} };
            fractional.Parse(text.c_str());
            require(!fractional.HasParseError(), "Cannot parse fractional editor/runtime receipt");
        }
        fractional["level_bounds"]["out_of_bounds"]["width"].SetInt(140000);
        fractional["spawn_locations"]["team_a"][0]["x"].SetInt(65650);
        fractional["spawn_locations"]["team_a"][0]["y"].SetInt(680);
        for (rapidjson::SizeType index = 0; index < 4; ++index)
        { fractional["spawn_locations"]["team_b"][index]["x"].SetDouble(130000 + index * 100); }
        auto& fractional_object = fractional["collision_objects"][1];
        fractional_object["rectangle"]["x"].SetDouble(65535.99);
        fractional_object["motion"]["phase"].SetDouble(0);
        const auto fractional_source = directory / L"fractional-source.json";
        {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            require(fractional.Accept(writer), "Cannot encode fractional editor/runtime receipt");
            std::ofstream output(fractional_source, std::ios::binary | std::ios::trunc);
            output.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
            output.close(); require(output.good(), "Cannot retain fractional editor/runtime source");
        }
        double anchor_area = 0.0;
        double editor_anchor_area = 0.0;
        for (const auto& [phase, stem] : { std::pair{ "0", "fractional-anchor" }, { "0.25", "fractional-quarter" } })
        {
            LevelDocument measured(tuning_path);
            measured.load(fractional_source);
            measured.set_property(first, "motion/phase", phase);
            const auto measured_path = directory / (std::string(stem) + ".json");
            measured.save(measured_path);
            auto measured_settings = loaded_settings(); measured_settings.set_stage(stem);
            LoadedFixture measured_runtime(world, measured_path, measured_settings, 314u);
            const double editor_scored = colourwars_editor::scoring_area(colourwars_editor::scoring_faces(measured, blockout));
            const double runtime_scored = measured_runtime.level->level_end_info().total_area;
            // The editor measures continuous strips in double; gameplay sums
            // tile floats per producer, then sums producer totals in float.
            // Compare gameplay with its exact float accumulation. Continuous
            // strips also allow the two float roundings (pitch division and
            // tile-area multiplication) used to publish each tile's area.
            double tile_area = 0.0;
            float source_area = 0.0f, folded_total = 0.0f;
            size_t source_index = 0;
            bool started = false;
            for (const auto& sample : measured_runtime.level->paint_surfaces())
            {
                if (started && sample.source_index != source_index) { folded_total += source_area; source_area = 0.0f; }
                started = true; source_index = sample.source_index;
                source_area += sample.scored_area;
                tile_area += static_cast<double>(sample.scored_area);
            }
            if (started) { folded_total += source_area; }
            const double tile_rounding_bound = 2.0 * std::numeric_limits<float>::epsilon() *
                std::max(std::abs(editor_scored), std::abs(tile_area));
            require(std::abs(editor_scored - tile_area) <= tile_rounding_bound &&
                runtime_scored == static_cast<double>(folded_total),
                "Fractional phase changed editor strips or runtime authoritative tile accumulation");
            if (anchor_area == 0.0) { anchor_area = runtime_scored; editor_anchor_area = editor_scored; }
            else
            {
                require(runtime_scored == anchor_area && editor_scored == editor_anchor_area,
                    "Fractional phase changed runtime-owned or editor local scored area");
            }
        }
        std::cout << "PASS editor-save/public-loader fractional float-binade phase invariance with bounded tile rounding\n";
    }
}
