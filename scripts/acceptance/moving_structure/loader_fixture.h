#pragma once

#include "scripts/acceptance/moving_structure/fixtures.h"
#include "game/objects/level_builder.h"

namespace moving_structure_acceptance
{
    // A complete public-loader stage. Every factory invocation rereads its
    // owned file into a fresh registry and constructs through LevelBuilder.
    void write_loaded_stage(const std::filesystem::path& path, bool jump_through = false,
        bool blocked = false);
    MenuLevelSettings loaded_settings();

    class LoadedFixture final
    {
    public:
        LoadedFixture(World& world, const std::filesystem::path& stage,
            const MenuLevelSettings& settings, unsigned int seed);
        void tick(const PlayerInputData& input = moving_structure_acceptance::input(),
            float seconds = dt, SeatInputFilter* next = nullptr);
        void start();

        GameTuning tuning;
        labrador::ResolutionManager resolution;
        labrador::ViewportManager viewports{ &resolution };
        labrador::Mouse mouse;
        labrador::Partitioner partitioner;
        labrador::Registry<LevelLoadedInfo> levels{ "moving loader fixture Level" };
        std::unique_ptr<Level> level;
    };
}
