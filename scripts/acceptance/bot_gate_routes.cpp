#include "game/pch.h"
#include "scripts/acceptance/bot_navigation/fixtures.h"
#include "game/ai/match_bot.h"
#include "game/ai/match_bot_driver.h"
#include "game/objects/team_gate_structure_paintable.h"
#include <algorithm>
#include <fstream>
#include <iostream>

// ROAD#15: a permitted passage is navigation geometry, while the same gate
// remains projectile cover. These checks distinguish choosing a route from
// accidentally wandering through it.
namespace
{
    using namespace labrador;
    using namespace mattmath;
    using namespace bot_navigation_acceptance;

    PlayerTeam other_team(PlayerTeam team)
    { return team == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a; }

    bool contains(const std::vector<RectangleF>& obstacles, const RectangleF& bounds)
    { return std::find(obstacles.begin(), obstacles.end(), bounds) != obstacles.end(); }

    const MatchBotSupport* find_support(const std::vector<MatchBotSupport>& supports, const RectangleF& bounds)
    {
        for (const MatchBotSupport& support : supports)
        { if (support.bounds == bounds) { return &support; } }
        return nullptr;
    }

    void paint_gate(World& world, TeamGateStructurePaintable& gate, PlayerTeam team)
    {
        RectangleF splash = gate.bounds();
        splash.x -= 20.0f; splash.y -= 20.0f;
        splash.width += 40.0f; splash.height += 40.0f;
        world.assets.paint(gate, splash, team, team == PlayerTeam::a ? 0 : 1);
        require(gate.paint_leader() == team, "The gate fixture did not give the requested team its paint lead");
    }

    MenuLevelSettings human_settings()
    {
        MenuLevelSettings settings;
        settings.set_player_count(2); settings.set_screen_layout(ScreenLayout::two_player);
        settings.set_stage("turbulence_gate_trial"); settings.set_duration(120.0f);
        settings.set_player_team(0, PlayerTeam::a); settings.set_player_team(1, PlayerTeam::b);
        settings.set_player_weapon(0, WeaponType::sprayer); settings.set_player_weapon(1, WeaponType::sprayer);
        settings.begin_input_claims(0);
        require(settings.claim_input_slot(1), "Cannot claim the gate fixture's second human input");
        return settings;
    }

    void check_geometry(const Level& level, PlayerTeam permitted)
    {
        const RectangleF bounds = level.team_gates()[0]->bounds();
        const Vector2F west(bounds.left() - 100.0f, bounds.bottom() - 60.0f);
        const Vector2F east(bounds.right() + 100.0f, west.y);
        const std::vector<MatchBotSupport> all_supports = level.bot_supports();
        const MatchBotSupport* gate_support = find_support(all_supports, bounds);
        require(gate_support != nullptr, "The conservative support query lost the gate");
        require(contains(level.bot_obstacles(), bounds) && !level.bot_line_of_sight(west, east),
            "A gate disappeared from conservative geometry or projectile sight");
        for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
        {
            const bool open = team == permitted;
            const std::vector<MatchBotSupport> supports = level.bot_supports(team);
            require(contains(level.bot_obstacles(team), bounds) != open &&
                (find_support(supports, bounds) != nullptr) != open &&
                level.bot_traversal_line_of_sight(west, east, team) == open,
                "Team traversal geometry did not follow settled gate permission");
            for (const MatchBotSupport& support : supports)
            {
                const MatchBotSupport* before = find_support(all_supports, support.bounds);
                require(before != nullptr && before->id == support.id,
                    "Filtering a permitted gate renumbered another support");
            }
        }
    }

    class Boundary final : public SeatInputFilter
    {
    public:
        PlayerTeam expected = PlayerTeam::none;
        int calls = 0;
        void filter_seats(std::vector<PlayerInputData>& seats, const Level& level) override
        {
            ++calls;
            require(level.team_gates()[0]->open_for() == expected,
                "Gate settlement moved ahead of participant observations");
            check_geometry(level, expected);
            for (PlayerInputData& seat : seats) { seat = {}; seat.connected = true; }
        }
    };

    void check_settlement(World& world)
    {
        std::unique_ptr<Level> level = world.build(human_settings());
        level->start_without_countdown();
        require(level->team_gates().size() == 1, "The gate trial no longer contains exactly one gate");
        auto& gate = const_cast<TeamGateStructurePaintable&>(*level->team_gates()[0]);
        const RectangleF bounds = gate.bounds();
        const std::vector<PlayerInputData> idle = physical_inputs();
        check_geometry(*level, PlayerTeam::none);
        Boundary boundary;
        paint_gate(world, gate, PlayerTeam::a);
        check_geometry(*level, PlayerTeam::none);
        level->update(idle, dt, &boundary);
        check_geometry(*level, PlayerTeam::a);
        boundary.expected = PlayerTeam::a;
        level->update(idle, dt, &boundary);

        // The paint lead changes while an A player occupies the passage. Both
        // the collision and bot observations retain A's actual permission.
        const float height = level->player(0).rectangle().height;
        require(level->place_player(0, Vector2F(bounds.left() + 14.0f, bounds.bottom() - height)),
            "Cannot place the occupied-gate witness");
        paint_gate(world, gate, PlayerTeam::b);
        for (int step = 0; step < 3; ++step) { level->update(idle, dt, &boundary); }
        require(gate.open_for() == PlayerTeam::a, "The gate closed on its permitted occupant");
        check_geometry(*level, PlayerTeam::a);
        require(level->place_player(0, Vector2F(bounds.left() - 200.0f, bounds.bottom() - height)),
            "Cannot clear the occupied-gate witness");
        level->update(idle, dt, &boundary);
        check_geometry(*level, PlayerTeam::b);
        boundary.expected = PlayerTeam::b;
        level->update(idle, dt, &boundary);
        require(boundary.calls == 7, "The settlement fixture did not observe every intended boundary");
        std::cout << "PASS gate traversal observations: closed, each team, stable support IDs, takeover, occupied hold and pre-settlement input boundary\n";
    }

    MatchBotObservation policy_view(const GameTuning& tuning)
    {
        MatchBotObservation view;
        view.alive = view.grounded = true; view.ammo = 1.0f;
        view.size = tuning.player().size; view.engage_distance = 800.0f;
        const RectangleF floor(-2000.0f, view.size.y * 0.5f, 4000.0f, 100.0f);
        view.obstacles.push_back(floor); view.supports.push_back({ floor, false, false, 1 });
        return view;
    }

    PlayerInputData first_decision(MatchBot& bot, const MatchBotObservation& view, const GameTuning& tuning,
        BotDifficulty difficulty)
    {
        PlayerInputData input;
        for (int step = 0; step < tuning.bot(difficulty).reaction_steps; ++step) { input = bot.think(view); }
        return input;
    }

    void check_policy(const GameTuning& tuning)
    {
        for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
        {
            for (const float direction : { -1.0f, 1.0f })
            {
                const MatchBotObservation plain = policy_view(tuning);
                std::uint64_t seed = 1;
                for (; seed < 100; ++seed)
                {
                    MatchBot baseline(&tuning, difficulty, seed);
                    if (first_decision(baseline, plain, tuning, difficulty).x_movement == -direction) { break; }
                }
                require(seed < 100, "Cannot find an initially opposite-heading control seed");
                MatchBotObservation open = plain;
                open.route_territory.push_back(Vector2F(direction * 500.0f, plain.size.y * 0.5f - 1.0f));
                MatchBot routed(&tuning, difficulty, seed);
                const PlayerInputData picked = first_decision(routed, open, tuning, difficulty);
                require(picked.x_movement == direction && !picked.jump_pressed,
                    "A navigation-only floor goal did not deliberately reverse the control's heading");
                // Match a route to an equally placed visible goal: movement is
                // the same, but the occluded tile must not become a paint aim.
                MatchBotObservation visible = plain; visible.territory = open.route_territory;
                MatchBot aiming(&tuning, difficulty, seed);
                const PlayerInputData shot = first_decision(aiming, visible, tuning, difficulty);
                require(shot.x_movement == direction && picked.shoot_angle != shot.shoot_angle,
                    "A navigation-only paint goal leaked into projectile aiming");
                MatchBotObservation closed = open;
                const RectangleF gate(direction > 0.0f ? 200.0f : -280.0f, -600.0f, 80.0f, 660.0f);
                closed.obstacles.push_back(gate); closed.supports.push_back({ gate, false, true, 2 });
                MatchBot refused(&tuning, difficulty, seed);
                const PlayerInputData denied = first_decision(refused, closed, tuning, difficulty);
                require(denied.x_movement == -direction,
                    "A blocked floor goal displaced the opposite-heading route rejection control");

                // A permission change must discard the stale route immediately,
                // rather than wait for the old replan/reaction countdown.
                MatchBotObservation reversed = plain;
                reversed.route_territory.push_back(Vector2F(-direction * 500.0f, plain.size.y * 0.5f - 1.0f));
                routed.replan_traversal();
                require(routed.think(reversed).x_movement == -direction,
                    "Traversal invalidation retained a stale movement decision");
                MatchBot random_a(&tuning, difficulty, seed), random_b(&tuning, difficulty, seed);
                random_a.replan_traversal();
                require(random_a.random() == random_b.random(), "Traversal invalidation consumed policy random draws");
            }
        }
        std::cout << "PASS gate route policy: six mirrored difficulty/control comparisons, closed-route refusal, aim separation and immediate RNG-preserving replanning\n";
    }

    void check_shots(World& world)
    {
        const RectangleF bounds(400.0f, 300.0f, 80.0f, 500.0f);
        TeamGateStructurePaintable gate("sprite_sheet_1", "square_white_4", bounds,
            &world.assets.render, &world.assets.audio, &world.assets.preferences, &world.assets.tuning.paint(),
            TeamColour{}, { true, true, true, false });
        const std::vector<Player*> nobody;
        const ProjectileTuning& tuning = world.assets.tuning.projectile(ProjectileType::spray);
        CollisionObject* target = &gate;
        for (const PlayerTeam permitted : { PlayerTeam::a, PlayerTeam::b })
        {
            paint_gate(world, gate, permitted); gate.settle(nobody);
            for (const PlayerTeam shooter : { PlayerTeam::a, PlayerTeam::b })
            {
                const float before = shooter == PlayerTeam::a ? gate.paint_total().team_a : gate.paint_total().team_b;
                ProjectileSpray shot(RectangleF(550.0f, 540.0f, 20.0f, 20.0f), Vector2F::ZERO,
                    shooter, shooter == PlayerTeam::a ? 0 : 1, Colour::white, &world.assets.render,
                    resolve_projectile_strip(tuning, world.assets.render), &tuning);
                shot.resolve_launch_obstruction(Vector2F(300.0f, 550.0f), std::span<CollisionObject* const>(&target, 1));
                require(shot.for_deletion() && shot.bounds().left() < bounds.right(),
                    "An open gate let an actual projectile launch cross its cover");
                const float after = shooter == PlayerTeam::a ? gate.paint_total().team_a : gate.paint_total().team_b;
                require(after >= before, "A projectile hit removed its own team's gate paint");
                require(gate.open_for() == permitted, "Projectile paint settled a gate outside Level's step boundary");
            }
        }
        std::cout << "PASS open-gate projectile cover: actual launch sweeps stop both teams' shots for each permission\n";
    }

    // An isolated public-loader recipe removes unrelated arena goals: an
    // unscored floor, one tall gate and one scoring patch beyond the passage.
    // Its mirrored target is the only unowned paint after the gate is opened.
    class StageFiles final
    {
    public:
        StageFiles()
        {
            root = std::filesystem::temp_directory_path() / ("ColourWars-bot-gate-" +
                std::to_string(GetCurrentProcessId()) + "-" + std::to_string(GetTickCount64()));
            require(std::filesystem::create_directory(root), "Cannot isolate gate route stage fixtures");
            for (const bool west : { false, true })
            {
                const std::filesystem::path path = root / (west ? "west.json" : "east.json");
                std::ofstream out(path, std::ios::binary);
                out << R"json({"format":1,"display_name":"Gate route fixture","icon_frame":"square_white_4",
"include_in_random":false,"sound":{"sound_bank_name":"paint_exe"},
"spawn_locations":{"team_a":[{"x":1700,"y":1160},{"x":1500,"y":1160},{"x":1300,"y":1160},{"x":1100,"y":1160}],
"team_b":[{"x":2250,"y":1160},{"x":2450,"y":1160},{"x":2650,"y":1160},{"x":2850,"y":1160}]},
"level_bounds":{"out_of_bounds":{"x":-100,"y":0,"width":4200,"height":1800},
"camera_bounds":{"x":0,"y":0,"width":4000,"height":1600},
"zoom_out_start_bounds":{"x":0,"y":0,"width":4000,"height":1600},
"zoom_out_finish_bounds":{"x":0,"y":0,"width":4000,"height":1600}},
"collision_objects":[
{"type":"Structure","sheet_name":"sprite_sheet_1","frame_name":"square_white_4","colour":"WHITE",
"collision_type":"STRUCTURE","rectangle":{"x":0,"y":1280,"width":)json" << (west ? 1500 : 2250) << R"json(,"height":400}},
{"type":"StructurePaintable","sheet_name":"sprite_sheet_1","frame_name":"square_white_4","colour":"WHITE",
"collision_type":"STRUCTURE_PAINTABLE","rectangle":{"x":1960,"y":580,"width":80,"height":700},
"paintable_faces":{"left":true,"top":true,"right":true,"bottom":false},"gate":"paint"},
{"type":"StructurePaintable","sheet_name":"sprite_sheet_1","frame_name":"square_white_4","colour":"WHITE",
"collision_type":"STRUCTURE_PAINTABLE","rectangle":{"x":)json" << (west ? 1500 : 2250) << R"json(,"y":1280,"width":200,"height":400},
"paintable_faces":{"left":false,"top":true,"right":false,"bottom":false}},
{"type":"Structure","sheet_name":"sprite_sheet_1","frame_name":"square_white_4","colour":"WHITE",
"collision_type":"STRUCTURE","rectangle":{"x":)json" << (west ? 1700 : 2450) << R"json(,"y":1280,"width":)json" <<
                    (west ? 2300 : 1550) << R"json(,"height":400}}],"non_collision_objects":[]})json";
                out.close(); require(out.good(), "Cannot write isolated gate route stage recipe");
            }
        }
        ~StageFiles()
        {
            std::error_code ignored;
            std::filesystem::remove(root / "west.json", ignored);
            std::filesystem::remove(root / "east.json", ignored);
            std::filesystem::remove(root, ignored);
        }
        std::filesystem::path root;
    };

    MenuLevelSettings route_settings(PlayerTeam team)
    {
        MenuLevelSettings settings;
        settings.set_player_count(2); settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_player_bot(1, BotDifficulty::hard); settings.set_screen_layout(ScreenLayout::one_player);
        settings.set_player_team(0, other_team(team)); settings.set_player_team(1, team);
        settings.set_player_weapon(1, WeaponType::sprayer);
        settings.set_stage(team == PlayerTeam::a ? "gate_east" : "gate_west"); settings.set_duration(120.0f);
        return settings;
    }

    std::unique_ptr<Level> route_level(World& world, PlayerTeam team)
    {
        std::unique_ptr<Level> level = world.build(route_settings(team), 10191);
        level->start_without_countdown();
        require(level->place_player(0, Vector2F(50.0f, 1160.0f)), "Cannot isolate the route fixture's idle human");
        require(level->place_player(1, Vector2F(team == PlayerTeam::a ? 1800.0f : 2148.0f, 1160.0f)),
            "Cannot place the route fixture's bot");
        auto& gate = const_cast<TeamGateStructurePaintable&>(*level->team_gates()[0]);
        paint_gate(world, gate, team);
        const std::vector<Player*> nobody; gate.settle(nobody);
        return level;
    }

    bool same_decision(const PlayerInputData& a, const PlayerInputData& b)
    {
        return a.x_movement == b.x_movement && a.jump_pressed == b.jump_pressed && a.jump_held == b.jump_held &&
            a.shoot_angle == b.shoot_angle && a.shoot_direction == b.shoot_direction &&
            a.shoot_direction_requested == b.shoot_direction_requested && a.primary_shoot == b.primary_shoot &&
            a.drop_through == b.drop_through && a.left_analog_stick == b.left_analog_stick &&
            a.right_analog_stick == b.right_analog_stick;
    }

    // Exercise the actual driver at Level's input/settlement boundary, while
    // holding both twin worlds still. Its public decision is retained before
    // neutral input keeps placement and paint from contaminating this oracle.
    class DriverProbe final : public SeatInputFilter
    {
    public:
        DriverProbe(const GameTuning& tuning, PlayerTeam team) : driver(&tuning, route_settings(team), 7751), expected(team) {}
        bool replaces_bot_inputs() const override { return true; }
        void filter_seats(std::vector<PlayerInputData>& seats, const Level& level) override
        {
            require(level.team_gates()[0]->open_for() == expected,
                "The integrated driver observed the wrong side of gate settlement");
            driver.filter_seats(seats, level);
            last = seats[1];
            for (PlayerInputData& seat : seats) { seat = {}; seat.connected = true; }
        }
        MatchBotDriver driver;
        PlayerTeam expected;
        PlayerInputData last;
    };

    void check_driver_changes(World& world, World& control_world)
    {
        // The chosen window is inside Hard's actual outstanding reaction and
        // territory intervals. A missing invalidation would return the exact
        // held decision on the first post-settlement observation.
        const int reaction = world.assets.tuning.bot(BotDifficulty::hard).reaction_steps;
        require(reaction >= 4 && world.assets.tuning.bot(BotDifficulty::hard).replan_steps > reaction + 3,
            "Update the integrated gate-invalidation timing oracle for changed Hard cadence");
        const std::vector<PlayerInputData> idle = physical_inputs();
        for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
        {
            for (const bool occupied : { false, true })
            {
                std::unique_ptr<Level> changed = route_level(world, team), control = route_level(control_world, team);
                DriverProbe changed_driver(world.assets.tuning, team), control_driver(world.assets.tuning, team);
                const auto tick_pair = [&]()
                {
                    changed->update(idle, dt, &changed_driver);
                    control->update(idle, dt, &control_driver);
                };
                for (int step = 0; step < reaction; ++step)
                {
                    tick_pair();
                    require(same_decision(changed_driver.last, control_driver.last),
                        "Identical driver/Level controls diverged before a permission experiment");
                }
                require(changed_driver.last.x_movement == (team == PlayerTeam::a ? 1.0f : -1.0f),
                    "The integrated permission fixture did not establish a held floor-route decision");
                if (occupied)
                {
                    const Vector2F inside(1974.0f, 1160.0f);
                    require(changed->place_player(1, inside) && control->place_player(1, inside),
                        "Cannot place the integrated driver's permitted occupant");
                }
                auto& gate = const_cast<TeamGateStructurePaintable&>(*changed->team_gates()[0]);
                paint_gate(world, gate, other_team(team));
                tick_pair();
                require(same_decision(changed_driver.last, control_driver.last),
                    "The driver invalidated for paint leadership before actual settled permission changed");
                if (occupied)
                {
                    require(gate.open_for() == team, "The integrated gate failed to retain its occupied permission");
                    const Vector2F outside(team == PlayerTeam::a ? 1800.0f : 2148.0f, 1160.0f);
                    require(changed->place_player(1, outside) && control->place_player(1, outside),
                        "Cannot clear the integrated driver's occupied gate");
                    tick_pair();
                    require(same_decision(changed_driver.last, control_driver.last),
                        "Clearing an occupant invalidated navigation before the gate settled");
                }
                require(gate.open_for() == other_team(team), "The integrated takeover never settled after clearance");
                changed_driver.expected = other_team(team);
                tick_pair();
                require(!same_decision(changed_driver.last, control_driver.last),
                    "The actual driver retained its held decision on the first observation after settled takeover");
            }
        }
        std::cout << "PASS actual driver invalidation: four mirrored direct/occupied takeovers, unchanged pre-settlement/held decisions and immediate post-settlement replanning\n";
    }

    void check_driver(World& world, World& control_world, const StageFiles& files)
    {
        for (const bool west : { false, true })
        {
            for (World* destination : { &world, &control_world })
            {
                auto info = std::make_unique<LevelLoadedInfo>(path_to_utf8(files.root / (west ? "west.json" : "east.json")).c_str());
                info->validate(); destination->levels.add(west ? "gate_west" : "gate_east", std::move(info));
            }
        }
        check_driver_changes(world, control_world);
        for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
        {
            std::unique_ptr<Level> level = route_level(world, team);
            const std::vector<PlayerInputData> idle = physical_inputs();
            Tape tape;
            std::vector<std::vector<std::uint64_t>> states;
            bool crossed = false, painted = false;
            int first_direction = 0;
            constexpr int steps = 420;
            for (int step = 0; step < steps; ++step)
            {
                level->update(idle, dt, &tape);
                states.push_back(snapshot(*level));
                if (first_direction == 0 && tape.steps.back()[1].x_movement != 0.0f)
                { first_direction = tape.steps.back()[1].x_movement > 0.0f ? 1 : -1; }
                const RectangleF& bot = level->player(1).rectangle();
                crossed = crossed || (team == PlayerTeam::a ? bot.left() > 2040.0f : bot.right() < 1960.0f);
                for (const PaintSurfaceSample& tile : level->paint_surfaces())
                {
                    if (tile.face == PaintSurfaceFace::top && tile.corners[0].y >= 1250.0f && tile.team == team)
                    { painted = true; }
                }
            }
            require(first_direction == (team == PlayerTeam::a ? 1 : -1) && crossed && painted,
                "The actual shipping driver did not deliberately cross its gate and paint the useful floor patch");
            level.reset();
            level = route_level(world, team); tape.playback = true;
            for (int step = 0; step < steps; ++step)
            {
                level->update(idle, dt, &tape);
                require(snapshot(*level) == states[static_cast<std::size_t>(step)],
                    "A mirrored gate route diverged under final-input replay");
            }
            require(tape.next == tape.steps.size(), "Gate route replay did not consume its complete finite tape");
        }
        std::cout << "PASS actual gate routes: both permitted teams take the only scoring-floor goal, paint it and replay 420 final-input steps exactly\n";
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "usage: bot_gate_routes <content>");
        World world(std::filesystem::absolute(argv[1]), "bot-gate-routes", "Gate routes need support '", "turbulence_gate_trial");
        check_policy(world.assets.tuning);
        check_settlement(world);
        check_shots(world);
        const StageFiles files;
        World control_world(std::filesystem::absolute(argv[1]), "bot-gate-control", "Gate routes need support '", "turbulence_gate_trial");
        check_driver(world, control_world, files);
        return 0;
    }
    catch (const std::exception& error)
    { std::cerr << "FAIL bot gate routes: " << error.what() << '\n'; return 1; }
}
