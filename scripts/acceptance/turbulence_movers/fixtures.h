#pragma once

#include "scripts/acceptance/moving_structure/fixtures.h"
#include "game/objects/level_builder.h"
#include "editor/level_document.h"
#include "engine/math/shape.h"
#include <string>

namespace turbulence_movers_acceptance
{
    namespace fs = std::filesystem;
    using World = moving_structure_acceptance::World;
    using moving_structure_acceptance::require;
    using moving_structure_acceptance::dt;

    struct Context
    {
        World& world;
        fs::path content;
        fs::path stage;
        fs::path evidence;
    };

    void author_trial(const fs::path& static_stage, const fs::path& tuning, const fs::path& output);
    std::string read_text(const fs::path& path);
    MatchSetup settings(int count = 1, PlayerTeam first_team = PlayerTeam::a);
    mattmath::RectangleF named_rectangle(const fs::path& path, const std::string& name);
    fs::path station_variant(Context& context, const std::string& label, float phase,
        bool side_b, bool on_mover = false);
    PlayerInputData input(float movement = 0.0f, bool pressed = false, bool held = false,
        bool drop = false, bool fire = false);
    bool supported(const Player& player);
    float mirrored(bool side_b, float x);

    class Trial final
    {
    public:
        Trial(Context& context, const fs::path& stage, const MatchSetup& setup,
            unsigned int seed = 439u);
        void tick(const PlayerInputData& action = input(), SeatInputFilter* next = nullptr,
            float seconds = dt);
        void tick_inputs(const std::vector<PlayerInputData>& actions, SeatInputFilter* next = nullptr,
            float seconds = dt);
        int count_in();
        void start();
        void check() const;
        HorizontalStructureState mover(bool side_b) const;
        const Player& player() const { return level->player(0); }

        Context& context;
        labrador::Registry<LevelLoadedInfo> infos{ "Turbulence mover trial Level" };
        labrador::ResolutionManager resolution;
        labrador::ViewportManager viewports{ &resolution };
        labrador::Mouse mouse;
        labrador::Partitioner partitioner;
        std::unique_ptr<Level> level;
        int updates = 0;
        bool allow_damage = false;
    private:
        mattmath::RectangleF bounds_;
        std::vector<std::pair<std::string, std::unique_ptr<mattmath::Shape>>> solids_;
    };

    float steering(const Trial& trial, float target);
    void geometry(Context& context);
    void routes(Context& context, bool full_matrix);
    void replay(Context& context);
    void pressure(Context& context);
    void local_roles(Context& context);
}
