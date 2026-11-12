// HYG#100: a rematch or series builds a new Level every round, and MSVC's
// AddressSanitizer has no leak detection, so a per-round leak would grow over a
// long couch session unseen. This Debug-only case builds, plays and destroys a
// Level on every stage the manifest lists, once to warm first-use caches, then
// several rounds more between two _CrtMemCheckpoint calls, and requires the
// debug heap to hold no more normal or client blocks afterwards than before.
#include "game/pch.h"
#include "game/contracts/simulation.h"
#include "game/input/input_slots.h"
#include "game/match/match_rules.h"
#include "game/objects/level_builder.h"
#include "game/config/presentation/presentation.h"
#include "scripts/acceptance/support/content_world.h"
#include "scripts/acceptance/support/probe.h"
#include "scripts/acceptance/support/stage_catalog.h"

#include <crtdbg.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#ifndef _DEBUG
#error "level_heap reads the debug CRT heap; cases.psd1 declares it debug-only."
#endif

using namespace labrador;

namespace
{
    using acceptance::require;

    // Each stage is played for this many fixed steps per build: long enough
    // for every seat to fire, paint, take hits and for the bots to plan.
    constexpr int steps_per_round = 180;
    // Measured rounds over every stage after the warm-up round.
    constexpr int measured_rounds = 2;

    // Only metadata is loaded. These objects never create a device or a save.
    struct World
    {
        RenderResources render;
        AudioResources audio;
        PlaySettings preferences;
        GameTuning tuning;
        Presentation presentation;
        Registry<LevelLoadedInfo> levels{ "Level" };
        ResolutionManager resolution;
        ViewportManager viewports{ &resolution };
        Partitioner partitioner;
        std::vector<std::string> stages;
        std::unique_ptr<LevelBuilder> builder;

        explicit World(const std::filesystem::path& content) :
            tuning((content / "tuning.json").string().c_str()),
            presentation((content / "presentation.json").string().c_str())
        {
            acceptance::load_cpu_assets(content, this->render, this->audio);
            for (const acceptance::ManifestEntry& level : acceptance::manifest_level_paths(content))
            {
                auto info = std::make_unique<LevelLoadedInfo>(path_to_utf8(level.path).c_str());
                info->validate();
                this->levels.add(level.name, std::move(info));
                this->stages.push_back(level.name);
            }
            // No thread pool: every allocation happens on this thread, between
            // the two checkpoints, where the debug heap counts it.
            this->builder = std::make_unique<LevelBuilder>(&this->viewports, &this->render,
                &this->audio, &this->preferences, &this->levels, &this->tuning,
                &this->presentation, &this->resolution, nullptr, &this->partitioner);
        }

        // One human on the keyboard and three bots, two to a team, with the
        // tools and difficulties rotating by round so each build differs.
        MatchSetup setup(const std::string& stage, int round) const
        {
            MatchSetup settings;
            settings.set_player_count(4);
            for (int seat = 1; seat < 4; ++seat)
            {
                settings.set_player_bot(seat, static_cast<BotDifficulty>((seat + round) % 3));
            }
            settings.begin_input_claims(input_slots::keyboard_and_mouse);
            settings.set_screen_layout(layout_for_humans(settings.human_count()));
            settings.set_stage(stage);
            settings.set_duration(300.0f);
            for (int seat = 0; seat < 4; ++seat)
            {
                settings.set_player_team(seat, seat % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
                settings.set_player_weapon(seat, static_cast<WeaponType>((seat + round) % 5));
            }
            return settings;
        }

        // Builds, plays and destroys one Level on `stage`.
        void play_round(const std::string& stage, int round,
            const std::vector<PlayerInputData>& inputs)
        {
            const MatchSetup settings = this->setup(stage, round);
            this->viewports.set_layout(settings.screen_layout());
            auto level = this->builder->build_level(settings, TeamColourPair::blue_orange,
                OpeningMode::off, MatchRandom(static_cast<unsigned int>(4100 + round)));
            level->start_without_countdown();
            for (int step = 0; step < steps_per_round; ++step)
            {
                level->update(inputs, simulation::seconds_per_step);
            }
        }

        void play_every_stage(int round, const std::vector<PlayerInputData>& inputs)
        {
            for (const std::string& stage : this->stages) { this->play_round(stage, round, inputs); }
        }
    };

    // The keyboard player holds fire to the right; every other slot is idle.
    std::vector<PlayerInputData> held_fire()
    {
        std::vector<PlayerInputData> inputs(input_slots::count);
        PlayerInputData& keyboard = inputs.at(static_cast<size_t>(input_slots::keyboard_and_mouse));
        keyboard.connected = true;
        keyboard.primary_shoot = true;
        keyboard.shoot_direction_requested = true;
        keyboard.shoot_direction = Vector2F(1, 0);
        return inputs;
    }

    struct HeapGrowth
    {
        long long blocks = 0;
        long long bytes = 0;
        // Every byte allocated between the checkpoints, freed or not: proof
        // that the window measured real work.
        long long allocated = 0;
    };

    // Normal blocks are what operator new and malloc hand out; client blocks
    // are the same under a caller's tag. CRT-internal and ignored blocks are
    // not the game's.
    HeapGrowth growth(const _CrtMemState& before, const _CrtMemState& after)
    {
        _CrtMemState difference{};
        static_cast<void>(_CrtMemDifference(&difference, &before, &after));
        HeapGrowth result;
        for (const int use : { _NORMAL_BLOCK, _CLIENT_BLOCK })
        {
            result.blocks += static_cast<long long>(difference.lCounts[use]);
            result.bytes += static_cast<long long>(difference.lSizes[use]);
        }
        result.allocated = static_cast<long long>(difference.lTotalCount);
        return result;
    }

    // The check can fail: one block allocated between two checkpoints is
    // reported as one block of growth, and freeing it as none.
    void check_detector()
    {
        _CrtMemState before{};
        _CrtMemCheckpoint(&before);
        auto* planted = new int(17);
        _CrtMemState held{};
        _CrtMemCheckpoint(&held);
        const HeapGrowth leaked = growth(before, held);
        delete planted;
        _CrtMemState freed{};
        _CrtMemCheckpoint(&freed);
        const HeapGrowth after_free = growth(before, freed);
        require(leaked.blocks == 1 && leaked.bytes == static_cast<long long>(sizeof(int)),
            "The debug heap did not report one planted block as one block of growth");
        require(after_free.blocks == 0 && after_free.bytes == 0,
            "The debug heap still reported a planted block after it was freed");
        std::cout << "PASS debug heap detector: one planted block is reported, and none once freed\n";
    }

    void report_growth(const _CrtMemState& before, const HeapGrowth& grown)
    {
        _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
        std::cerr << "Heap growth over the measured rounds: " << grown.blocks << " blocks, "
            << grown.bytes << " bytes\n";
        // A handful per build is a leak to read; thousands would drown the log.
        if (grown.blocks > 0 && grown.blocks <= 200) { _CrtMemDumpAllObjectsSince(&before); }
    }

    void check_repeated_builds(const std::filesystem::path& content)
    {
        World world(content);
        const std::vector<PlayerInputData> inputs = held_fire();
        // Warm-up: first-use caches (glyph lookups, statics, the stream
        // buffers) fill here, outside the measured window.
        world.play_every_stage(0, inputs);
        std::cout.flush();
        std::cerr.flush();

        _CrtMemState before{};
        _CrtMemCheckpoint(&before);
        for (int round = 1; round <= measured_rounds; ++round) { world.play_every_stage(round, inputs); }
        _CrtMemState after{};
        _CrtMemCheckpoint(&after);

        const HeapGrowth grown = growth(before, after);
        require(grown.allocated > 0, "The measured rounds allocated nothing, so the heap check measured nothing");
        if (grown.blocks > 0 || grown.bytes > 0) { report_growth(before, grown); }
        require(grown.blocks <= 0 && grown.bytes <= 0,
            "Repeated Level builds left the debug heap holding more than before (HYG#100)");
        const int builds = measured_rounds * static_cast<int>(world.stages.size());
        std::cout << "PASS " << builds << " measured Level builds over " << world.stages.size()
            << " stages, " << steps_per_round << " steps each: no normal-block growth ("
            << grown.blocks << " blocks, " << grown.bytes << " bytes; "
            << grown.allocated << " bytes allocated and freed in between)\n";
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("level heap", [&]
    {
        require(argc == 2, "Usage: level_heap.exe <content directory>");
        const std::filesystem::path content(argv[1]);
        check_detector();
        check_repeated_builds(content);
        return 0;
    });
}
