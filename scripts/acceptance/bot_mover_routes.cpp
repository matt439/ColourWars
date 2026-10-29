#include "game/pch.h"
#include "scripts/acceptance/bot_navigation/fixtures.h"
#include "game/ai/match_bot_support.h"
#include <iostream>
#include <sstream>

// Horizontal paintable one-way movers on the explicit Turbulence mover stage.
// A bot either starts below its side's mover, where the mover is the nearest
// raised support, or starts riding it. Each trial records whether the bot
// lands on the mover, paints its top and later leaves it for other ground.
namespace
{
    using namespace labrador;
    using namespace mattmath;
    using namespace bot_navigation_acceptance;

    constexpr int trial_steps = 2400;

    enum class Route { reach, ride };

    const char* route_name(Route route) { return route == Route::reach ? "reach" : "ride"; }

    struct Trial
    {
        Route route;
        PlayerTeam team;
        WeaponType tool;
        BotDifficulty difficulty;
    };

    MatchSetup roster(const Trial& trial)
    {
        MatchSetup settings;
        settings.set_player_count(2);
        settings.set_player_bot(1, trial.difficulty);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_screen_layout(ScreenLayout::one_player);
        settings.set_stage("turbulence_mover_trial"); settings.set_duration(120.0f);
        settings.set_player_team(0, trial.team == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a);
        settings.set_player_team(1, trial.team);
        settings.set_player_weapon(0, trial.tool); settings.set_player_weapon(1, trial.tool);
        return settings;
    }

    // The mover on the bot's side of the stage, at its current position.
    RectangleF mover(const Level& level, PlayerTeam team)
    {
        for (const MatchBotSupport& support : level.bot_supports())
        {
            const RectangleF& bounds = support.bounds;
            if (support.one_way && support.paintable && bounds.width == 320.0f && bounds.height == 20.0f &&
                (bounds.center().x < 2000.0f) == (team == PlayerTeam::a)) { return bounds; }
        }
        throw std::runtime_error("Mover-route trial lost its side's mover");
    }

    // Injection ends at initial placement, as in the other route cases.
    void prepare(Level& level, const World& world, const Trial& trial)
    {
        const std::vector<PlayerInputData> physical = physical_inputs();
        for (int tick = 0; tick < 900 && level.state() != LevelState::active; ++tick)
        { level.update(physical, dt); }
        require(level.state() == LevelState::active, "Mover-route trial did not finish its ordinary countdown");
        const Vector2F size = world.assets.tuning.player().size;
        const bool left = trial.team == PlayerTeam::a;
        RectangleF start = world.named(left ? "upper_approach_a" : "upper_approach_b");
        float x = left ? 1000.0f : 3000.0f;
        if (trial.route == Route::ride) { start = mover(level, trial.team); x = start.center().x; }
        require(level.place_player(1, Vector2F(x - size.x * 0.5f, start.top() - size.y)),
            "Mover-route trial could not place its initial endpoint");
    }

    float owned_mover_paint(const Level& level, const RectangleF& bounds, float thickness)
    {
        const int owner = paint_owners::index(-1, 1);
        float area = 0.0f;
        for (const PaintSurfaceSample& surface : level.paint_surfaces())
        {
            if (surface.face != PaintSurfaceFace::top || surface.paint_owner != owner) { continue; }
            bool belongs = true;
            for (const Vector2F& corner : surface.corners)
            {
                belongs = belongs && corner.x >= bounds.left() - endpoint_tolerance &&
                    corner.x <= bounds.right() + endpoint_tolerance &&
                    std::fabs(corner.y - bounds.top()) <= thickness + endpoint_tolerance;
            }
            if (belongs) { area += surface.scored_area; }
        }
        return area;
    }

    struct TrialResult
    {
        bool landed;
        bool painted;
        bool left;
    };

    TrialResult run(World& world, const Trial& trial)
    {
        std::unique_ptr<Level> level = world.build(roster(trial));
        prepare(*level, world, trial);
        const std::vector<PlayerInputData> physical = physical_inputs();
        const float thickness = world.assets.tuning.paint().thickness;
        int landed_step = -1, left_step = -1, mover_steps = 0, longest = 0, run_length = 0, jumps = 0;
        int deaths = 0;
        bool was_alive = true;
        float peak_paint = 0.0f;
        Tape tape("Mover-route tape exhausted");
        std::ostringstream trace;
        for (int tick = 0; tick < trial_steps; ++tick)
        {
            level->update(physical, dt, &tape);
            require(level->state() == LevelState::active, "Mover-route trial escaped its finite active-step budget");
            const PlayerInputData& input = tape.steps.back()[1];
            require(input.connected && !input.toggle_pause_menu && !input.pause_held && !input.toggle_debug,
                "Mover-route bot emitted a device/menu/pause action");
            const Player& bot = level->player(1);
            require(bot.player_num() == -1 && bot.primary_weapon().type() == trial.tool &&
                std::isfinite(bot.center().x) && std::isfinite(bot.center().y),
                "Mover-route bot changed identity/tool or produced invalid state");
            const RectangleF bounds = mover(*level, trial.team);
            const bool alive = bot.state() == PlayerState::alive;
            deaths += was_alive && !alive ? 1 : 0; was_alive = alive;
            const bool riding = alive && on_support(bot, bounds);
            if (riding)
            {
                ++mover_steps; longest = std::max(longest, ++run_length);
                if (landed_step < 0) { landed_step = tick; }
            }
            else
            {
                run_length = 0;
                if (landed_step >= 0 && left_step < 0 && bot.state() == PlayerState::alive && grounded(bot))
                { left_step = tick; }
            }
            jumps += input.jump_pressed ? 1 : 0;
            if (tick % 30 == 29) { peak_paint = std::max(peak_paint, owned_mover_paint(*level, bounds, thickness)); }
            if (tick % 30 == 0 || input.jump_pressed)
            {
                trace << "TRACE step=" << tick << " x=" << bot.center().x << " feet=" << bot.rectangle().bottom()
                    << " ground=" << grounded(bot) << " mover_left=" << bounds.left() << " riding=" << riding
                    << " move=" << input.x_movement << " jump=" << input.jump_pressed << " drop=" << input.drop_through
                    << " fire=" << input.primary_shoot << '\n';
            }
        }
        const RectangleF bounds = mover(*level, trial.team);
        const float final_paint = owned_mover_paint(*level, bounds, thickness);
        peak_paint = std::max(peak_paint, final_paint);
        const Player& bot = level->player(1);
        std::cout << "MEASURE bot mover route=" << route_name(trial.route) <<
            " team=" << (trial.team == PlayerTeam::a ? 'a' : 'b') << " tool=" << static_cast<int>(trial.tool) <<
            " difficulty=" << static_cast<int>(trial.difficulty) << " landed_step=" << landed_step <<
            " mover_steps=" << mover_steps << " longest_ride=" << longest << " left_step=" << left_step <<
            " peak_mover_paint=" << peak_paint << " final_mover_paint=" << final_paint << " jumps=" << jumps <<
            " deaths=" << deaths << " final_x=" << bot.center().x <<
            " final_feet=" << bot.rectangle().bottom() << '\n';
        const bool landed = landed_step >= 0, painted = peak_paint > 0.0f, left = left_step >= 0;
        if (landed && !left) { std::cout << trace.str(); }
        return { landed, painted, left };
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "usage: bot_mover_routes <content>");
        World world(std::filesystem::absolute(argv[1]), "bot-mover-routes",
            "Bot mover route needs authored support '", "turbulence_mover_trial");
        int trials = 0, landed = 0, painted = 0, left = 0;
        for (const Route route : { Route::reach, Route::ride })
        {
            int route_landed = 0, route_painted = 0, route_left = 0;
            for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
            {
                for (int tool = 0; tool < 5; ++tool)
                {
                    for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
                    {
                        const TrialResult result = run(world, { route, team, static_cast<WeaponType>(tool), difficulty });
                        route_landed += result.landed ? 1 : 0;
                        route_painted += result.painted ? 1 : 0;
                        route_left += result.left ? 1 : 0;
                        ++trials;
                    }
                }
            }
            std::cout << "MEASURE bot mover routes route=" << route_name(route) << " landed=" << route_landed <<
                "/30 painted_mover_top=" << route_painted << "/30 left_mover=" << route_left << "/30\n";
            // A bot placed on its mover stands on it; one that lands always
            // leaves it again for other ground within the budget.
            require(route == Route::reach || route_landed == 30, "A riding bot was not grounded on its mover");
            require(route_left == route_landed, "A bot that landed on a moving support never left it");
            landed += route_landed; painted += route_painted; left += route_left;
        }
        require(trials == 60, "Mover-route matrix changed size");
        std::cout << "PASS 60 bounded actual-driver mover trials (reach and ride, both sides, five tools, three "
            "difficulties, 2400 steps); every landing left the mover again (" << left << '/' << landed <<
            "); reaching and painting the mover are measurements (painted=" << painted << "/60)\n";
        return 0;
    }
    catch (const std::exception& error)
    { std::cerr << "FAIL bot mover routes: " << error.what() << '\n'; return 1; }
}
