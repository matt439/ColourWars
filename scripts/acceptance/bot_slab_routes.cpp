#include "game/pch.h"
#include "scripts/acceptance/bot_navigation/fixtures.h"
#include "game/objects/fixed_slab_geometry.h"
#include "scripts/acceptance/support/probe.h"
#include <rapidjson/document.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

// ROAD#38's measurement. The explicit slab trial is the mover trial plus two
// fixed scored slabs, each bridging its side's upper approach (top 880) to its
// crossover (top 800). Bots model a slab only as a conservative barrier, so
// this case counts what the shipping bots actually do there - steps grounded
// on a slab, owned slab-top paint and arrivals on the crossover - per tool and
// difficulty, against the same counts on the mover trial, whose gap is empty.
// Counts are measurements; the gates are only bounded, valid driver output.
namespace
{
    using namespace labrador;
    using namespace mattmath;
    using namespace bot_navigation_acceptance;

    constexpr int trial_steps = 2400;

    enum class Start { approach, spawn };
    const char* start_name(Start start) { return start == Start::approach ? "approach" : "spawn"; }

    struct Trial
    {
        const char* stage;
        Start start;
        PlayerTeam team;
        WeaponType tool;
        BotDifficulty difficulty;
    };

    struct Slabs
    {
        // Indexed by side: 0 is team a's (left), 1 is team b's.
        std::vector<FixedSlabGeometry> sides;
    };

    Slabs read_slabs(const std::filesystem::path& content)
    {
        std::ifstream file(content / "levels" / "turbulence_slab_trial.json", std::ios::binary);
        require(static_cast<bool>(file), "Slab-route case could not read the slab trial");
        std::ostringstream text; text << file.rdbuf();
        rapidjson::Document document; document.Parse(text.str().c_str());
        require(!document.HasParseError() && document.HasMember("collision_objects"), "Slab trial did not parse");
        std::optional<FixedSlabSpec> a, b;
        for (const auto& object : document["collision_objects"].GetArray())
        {
            if (std::string(object["type"].GetString()) != "FixedSlabPaintable") { continue; }
            const auto& slab = object["slab"];
            FixedSlabSpec spec;
            spec.top_left = Vector2F(slab["x"].GetFloat(), slab["y"].GetFloat());
            spec.rise = std::string(slab["rise"].GetString()) == "left" ? FixedSlabRise::left : FixedSlabRise::right;
            const std::string name = object["name"].GetString();
            if (name == "fixed_slab_a") { a = spec; }
            else if (name == "fixed_slab_b") { b = spec; }
            else { throw std::runtime_error("Slab trial has an unexpected slab '" + name + "'"); }
        }
        require(a.has_value() && b.has_value(), "Slab trial lost one of its two fixed slabs");
        Slabs slabs;
        slabs.sides.emplace_back(*a); slabs.sides.emplace_back(*b);
        return slabs;
    }

    MatchSetup roster(const Trial& trial)
    {
        MatchSetup settings;
        settings.set_player_count(2);
        settings.set_player_bot(1, trial.difficulty);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_screen_layout(ScreenLayout::one_player);
        settings.set_stage(trial.stage); settings.set_duration(120.0f);
        settings.set_player_team(0, trial.team == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a);
        settings.set_player_team(1, trial.team);
        settings.set_player_weapon(0, trial.tool); settings.set_player_weapon(1, trial.tool);
        return settings;
    }

    // Injection ends at initial placement, as in the other route cases; the
    // spawn start keeps the ordinary spawn.
    void prepare(Level& level, const World& world, const Trial& trial)
    {
        const std::vector<PlayerInputData> physical = physical_inputs();
        for (int tick = 0; tick < 900 && level.state() != LevelState::active; ++tick)
        { level.update(physical, dt); }
        require(level.state() == LevelState::active, "Slab-route trial did not finish its ordinary countdown");
        if (trial.start != Start::approach) { return; }
        const Vector2F size = world.assets.tuning.player().size;
        const bool left = trial.team == PlayerTeam::a;
        const RectangleF start = world.named(left ? "upper_approach_a" : "upper_approach_b");
        const float x = left ? 1000.0f : 3000.0f;
        require(level.place_player(1, Vector2F(x - size.x * 0.5f, start.top() - size.y)),
            "Slab-route trial could not place its initial endpoint");
    }

    // Grounded on the slope itself: a ramp state with the feet on the slab's
    // top line at the body's uphill corner, the point Player supports it at.
    // The trials' real ramps sit at the lower bed, 320 units below either slab.
    bool on_slab(const Player& player, const FixedSlabGeometry& slab)
    {
        if (player.move_state() != PlayerMoveState::on_ramp_left &&
            player.move_state() != PlayerMoveState::on_ramp_right) { return false; }
        const RectangleF& body = player.rectangle();
        const Vector2F start = slab.top_start(), end = slab.top_end();
        if (body.right() < start.x || body.left() > end.x) { return false; }
        const float x = std::clamp(end.y < start.y ? body.right() : body.left(), start.x, end.x);
        const float top = start.y + (end.y - start.y) * (x - start.x) / (end.x - start.x);
        return std::fabs(body.bottom() - top) <= 2.0f;
    }

    // Owned top-face paint lying on the slab's top, in its own frame.
    float owned_slab_paint(const Level& level, const FixedSlabGeometry& slab)
    {
        const int owner = paint_owners::index(-1, 1);
        float area = 0.0f;
        for (const PaintSurfaceSample& surface : level.paint_surfaces())
        {
            if (surface.face != PaintSurfaceFace::top || surface.paint_owner != owner) { continue; }
            bool belongs = true;
            for (const Vector2F& corner : surface.corners)
            {
                const Vector2F local = slab.to_local(corner);
                belongs = belongs && local.x >= -endpoint_tolerance && local.x <= slab.length() + endpoint_tolerance &&
                    local.y >= -endpoint_tolerance && local.y <= FixedSlabGeometry::normal_thickness + endpoint_tolerance;
            }
            if (belongs) { area += surface.scored_area; }
        }
        return area;
    }

    struct TrialResult
    {
        int slab_steps = 0;
        float slab_paint = 0.0f;
        int arrival_step = -1;
        bool via_slab = false;
    };

    TrialResult run(World& world, const Slabs& slabs, const Trial& trial)
    {
        std::unique_ptr<Level> level = world.build(roster(trial));
        prepare(*level, world, trial);
        const std::vector<PlayerInputData> physical = physical_inputs();
        const bool left = trial.team == PlayerTeam::a;
        const FixedSlabGeometry& slab = slabs.sides[left ? 0 : 1];
        const FixedSlabGeometry& other = slabs.sides[left ? 1 : 0];
        const RectangleF crossover = world.named(left ? "crossover_a" : "crossover_b");
        TrialResult result;
        int jumps = 0, deaths = 0, other_slab_steps = 0;
        bool was_alive = true;
        Tape tape("Slab-route tape exhausted");
        for (int tick = 0; tick < trial_steps; ++tick)
        {
            level->update(physical, dt, &tape);
            require(level->state() == LevelState::active, "Slab-route trial escaped its finite active-step budget");
            const PlayerInputData& input = tape.steps.back()[1];
            require(input.connected && !input.toggle_pause_menu && !input.pause_held && !input.toggle_debug,
                "Slab-route bot emitted a device/menu/pause action");
            const Player& bot = level->player(1);
            require(bot.player_num() == -1 && bot.primary_weapon().type() == trial.tool &&
                std::isfinite(bot.center().x) && std::isfinite(bot.center().y),
                "Slab-route bot changed identity/tool or produced invalid state");
            const bool alive = bot.state() == PlayerState::alive;
            deaths += was_alive && !alive ? 1 : 0; was_alive = alive;
            if (alive && on_slab(bot, slab)) { ++result.slab_steps; }
            if (alive && on_slab(bot, other)) { ++other_slab_steps; }
            if (result.arrival_step < 0 && alive && on_support(bot, crossover))
            { result.arrival_step = tick; result.via_slab = result.slab_steps > 0; }
            jumps += input.jump_pressed ? 1 : 0;
        }
        result.slab_paint = owned_slab_paint(*level, slab) + owned_slab_paint(*level, other);
        const Player& bot = level->player(1);
        std::cout << "MEASURE bot slab stage=" << trial.stage << " start=" << start_name(trial.start) <<
            " team=" << (left ? 'a' : 'b') << " tool=" << static_cast<int>(trial.tool) <<
            " difficulty=" << static_cast<int>(trial.difficulty) << " slab_steps=" << result.slab_steps <<
            " other_slab_steps=" << other_slab_steps << " final_slab_top_paint=" << result.slab_paint <<
            " crossover_arrival_step=" << result.arrival_step << " via_slab=" << result.via_slab <<
            " jumps=" << jumps << " deaths=" << deaths << " final_x=" << bot.center().x <<
            " final_feet=" << bot.rectangle().bottom() << '\n';
        result.slab_steps += other_slab_steps;
        return result;
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("bot slab routes", [&]
    {
        require(argc == 2, "usage: bot_slab_routes <content>");
        const std::filesystem::path content = std::filesystem::absolute(argv[1]);
        const Slabs slabs = read_slabs(content);
        require(slabs.sides[0].top_end().x <= 2000.0f && slabs.sides[1].top_start().x >= 2000.0f,
            "Slab trial's slabs are no longer one per side");
        int trials = 0;
        for (const char* stage : { "turbulence_slab_trial", "turbulence_mover_trial" })
        {
            World world(content, "bot-slab-routes", "Bot slab route needs authored support '", stage);
            const bool slab_stage = std::string_view(stage) == "turbulence_slab_trial";
            for (const Start start : { Start::approach, Start::spawn })
            {
                int grounded = 0, steps = 0, painted = 0, arrived = 0, via_slab = 0;
                float paint = 0.0f;
                for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
                {
                    for (int tool = 0; tool < 5; ++tool)
                    {
                        for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
                        {
                            const TrialResult result = run(world, slabs,
                                { stage, start, team, static_cast<WeaponType>(tool), difficulty });
                            grounded += result.slab_steps > 0 ? 1 : 0; steps += result.slab_steps;
                            painted += result.slab_paint > 0.0f ? 1 : 0; paint += result.slab_paint;
                            arrived += result.arrival_step >= 0 ? 1 : 0; via_slab += result.via_slab ? 1 : 0;
                            ++trials;
                        }
                    }
                }
                std::cout << "MEASURE bot slab routes stage=" << stage << " start=" << start_name(start) <<
                    " slab_grounded=" << grounded << "/30 slab_steps=" << steps << " slab_top_painted=" << painted <<
                    "/30 slab_top_paint=" << paint << " crossover_arrivals=" << arrived << "/30 arrivals_via_slab=" <<
                    via_slab << "/30\n";
                // The mover trial has no slab, so its gap windows must read zero.
                require(slab_stage || (steps == 0 && paint == 0.0f && via_slab == 0),
                    "Mover trial reported slab contact where it has no slab");
            }
        }
        require(trials == 120, "Slab-route matrix changed size");
        std::cout << "PASS 120 bounded actual-driver slab/mover trials (two stages, approach and spawn starts, both "
            "sides, five tools, three difficulties, 2400 steps); slab and crossover counts are measurements\n";
        return 0;
    });
}
