#include "game/pch.h"
#include "game/debug/dev_tools.h"
#include "scripts/acceptance/moving_structure/loader_fixture.h"
#include <iostream>

#if COLOURWARS_DEV_TOOLS
#include "game/debug/match_fingerprint.h"
#include "game/debug/replay.h"
#include "game/debug/tune_document.h"
#include <process.h>

namespace moving_structure_acceptance
{
    namespace
    {
        PlayerInputData loaded_action(int step)
        {
            auto action = aimed_input({ 0.0f, 1.0f }, 1.5707963267948966f, step < 90);
            action.jump_pressed = step == 120;
            action.jump_held = step >= 120 && step < 132;
            action.drop_through = step == 180;
            return action;
        }

        void replay_loaded_stage(World& world, const std::filesystem::path& directory, bool one_way)
        {
            ReplayFile header;
            header.build = "public-loader-moving-fixture";
            header.dt_bits = bits(dt);
            header.seed = one_way ? 180u : 179u;
            auto settings = loaded_settings();
            settings.set_stage(one_way ? "public_moving_one_way" : "public_moving_solid");
            header.tape.start(0, settings);
            const auto path = directory / (header.tape.settings().stage() + ".json");
            const auto tape_path = directory / (header.tape.settings().stage() + "-replay.json");
            const auto csv_path = directory / (header.tape.settings().stage() + "-trace.csv");
            write_loaded_stage(path, one_way);
            auto original = std::make_unique<LoadedFixture>(world, path, header.tape.settings(), *header.seed);
            const auto initial = MatchFingerprint::of(*original->level, 0);
            require(initial.horizontal_motion && initial.horizontal_motion->structures.size() == 2 &&
                initial.horizontal_motion->structures[0].id == 1 && initial.horizontal_motion->structures[1].id == 2 &&
                initial.horizontal_motion->structures[0].spec[6] == bits(0.25f) &&
                initial.horizontal_motion->structures[1].spec[6] == bits(0.75f),
                "Fresh public replay build lost mover-only IDs or authored initial phases");

            constexpr int length = 360;
            ReplayRecorder recorder("public-loader-moving", header, nullptr, length);
            std::vector<MatchFingerprint> observations;
            observations.reserve(length);
            bool carried = false, paid = false, owned = false, jump_released = false, drop_released = false;
            bool reversed_at_right = false, reversed_at_left = false;
            auto before = initial;
            std::ofstream trace(csv_path, std::ios::binary | std::ios::trunc);
            require(trace.good(), "Cannot open public replay trace receipt");
            trace << "update,hash,carrier,phase_bits,direction,owned_tiles,owned_area_bits\n";
            for (int step = 0; step < length; ++step)
            {
                const auto action = loaded_action(step);
                original->tick(action, dt, &recorder);
                const auto actual = MatchFingerprint::of(*original->level, step + 1);
                observations.push_back(actual);
                const auto& motion = *actual.horizontal_motion;
                carried |= motion.carriers[0] == 1;
                paid |= actual.seats[0].ammo < initial.seats[0].ammo;
                jump_released |= action.jump_pressed && before.horizontal_motion->carriers[0] == 1 && motion.carriers[0] == 0;
                drop_released |= action.drop_through && before.horizontal_motion->carriers[0] == 1 && motion.carriers[0] == 0;
                reversed_at_right |= before.horizontal_motion->structures[0].direction == 1 && motion.structures[0].direction == -1;
                reversed_at_left |= before.horizontal_motion->structures[0].direction == -1 && motion.structures[0].direction == 1;
                size_t owned_tiles = 0;
                for (const auto& tile : motion.structures[0].tiles)
                {
                    if (tile.team == static_cast<int>(PlayerTeam::a) && tile.owner == input_slots::keyboard_and_mouse)
                    { ++owned_tiles; }
                }
                owned |= owned_tiles > 0;
                trace << step + 1 << ',' << hex64(actual.hash) << ',' << motion.carriers[0] << ',' <<
                    hex64(motion.structures[0].phase) << ',' << motion.structures[0].direction << ',' <<
                    owned_tiles << ',' << actual.team_a << '\n';
                if ((step + 1) % ReplayFile::checkpoint_interval == 0) { recorder.add_checkpoint(actual); }
                before = actual;
            }
            trace.close();
            require(trace.good(), "Cannot finish public replay trace receipt");
            require(carried && paid && owned && jump_released && reversed_at_right && reversed_at_left &&
                (!one_way || drop_released),
                "Public replay did not exercise carry, paid local-face credit, release and both reversals");
            recorder.file().updates = length;
            recorder.file().final_print = observations.back();
            recorder.file().write(tape_path);
            const unsigned int terminal_rng = static_cast<unsigned int>(std::rand());
            original.reset();

            const ReplayFile decoded = ReplayFile::read(tape_path);
            require(decoded.seed && decoded.dt_bits == bits(dt) && decoded.updates == length &&
                decoded.inputs_by_seat().size() == 1 && decoded.inputs_by_seat()[0].size() == length && decoded.checkpoints.size() == 6,
                "Public-loader tape lost its rebuilding setup, inputs or checkpoints");
            // Select the file and complete setup from the decoded tape, reread
            // that file into a new registry, and seed the real public builder.
            const auto rebuild_path = directory / (decoded.tape.settings().stage() + ".json");
            auto playback = std::make_unique<LoadedFixture>(world, rebuild_path, decoded.tape.settings(), *decoded.seed);
            require(MatchFingerprint::difference(initial, MatchFingerprint::of(*playback->level, 0)).empty(),
                "Fresh decoded-settings public build differs before playback begins");
            ReplayPlayer player("public-loader-moving", decoded);
            size_t checkpoint = 0;
            for (int step = 0; step < decoded.updates; ++step)
            {
                playback->tick(input(), float_from_bits(decoded.dt_bits), &player);
                const auto actual = MatchFingerprint::of(*playback->level, step + 1);
                const auto difference = MatchFingerprint::difference(observations[static_cast<size_t>(step)], actual);
                require(difference.empty(), "Public-loader replay diverged at " + std::to_string(step + 1) + ": " + difference);
                if ((step + 1) % ReplayFile::checkpoint_interval == 0)
                {
                    require(decoded.checkpoints[checkpoint].update == step + 1 &&
                        decoded.checkpoints[checkpoint].hash == actual.hash,
                        "Public-loader replay checkpoint differs");
                    ++checkpoint;
                }
            }
            require(MatchFingerprint::difference(decoded.final_print,
                MatchFingerprint::of(*playback->level, decoded.updates)).empty() &&
                static_cast<unsigned int>(std::rand()) == terminal_rng,
                "Public-loader replay final state or terminal RNG differs");
            std::cout << "PASS public LevelBuilder " << (one_way ? "one-way" : "solid") <<
                " replay: 360 exact updates, six checkpoints, seeded disk rebuild, carry/paid local paint/jump release/" <<
                (one_way ? "drop release/" : "") << "both reversals and terminal RNG\n";
        }
    }

    void check_public_replay(World& world)
    {
        const auto directory = std::filesystem::temp_directory_path() /
            (L"colourwars_public_moving_replay_" + std::to_wstring(_getpid()));
        require(!std::filesystem::exists(directory), "Refusing to overwrite an earlier public replay receipt directory");
        std::filesystem::create_directories(directory);
        std::cout << "Public moving replay receipts: " << labrador::path_to_utf8(directory) << '\n';
        replay_loaded_stage(world, directory, false);
        replay_loaded_stage(world, directory, true);
    }
}
#else
namespace moving_structure_acceptance
{
    void check_public_replay(World&)
    { std::cout << "LIMIT public moving replay requires Debug dev tools; public disk-loader checks run in both presets\n"; }
}
#endif
