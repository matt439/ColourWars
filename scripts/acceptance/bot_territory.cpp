#include "game/pch.h"
#include "game/ai/match_bot_territory.h"
#include "scripts/acceptance/bot_navigation/fixtures.h"
#include <iostream>

namespace
{
    using namespace labrador;
    using namespace mattmath;
    using namespace bot_navigation_acceptance;

    void same_navigation(const PlayerInputData& a, const PlayerInputData& b)
    {
        require(a.x_movement == b.x_movement && a.left_analog_stick == b.left_analog_stick &&
            a.jump_pressed == b.jump_pressed && a.jump_held == b.jump_held &&
            a.drop_through == b.drop_through && a.primary_shoot == b.primary_shoot,
            "An aim-only current-support supplement changed ordinary navigation or reserve decisions");
    }

    void check_geometry(const GameTuning& tuning)
    {
        MatchBotObservation view;
        view.alive = view.grounded = true;
        view.size = tuning.player().size;
        const float feet = view.size.y * 0.5f;
        view.supports.push_back({ RectangleF(-500.0f, feet, 1000.0f, 50.0f), false, true });
        const float thickness = tuning.paint().thickness;
        PaintSurfaceSample tile;
        tile.corners = { Vector2F(100.0f, feet), Vector2F(125.0f, feet),
            Vector2F(125.0f, feet + thickness), Vector2F(100.0f, feet + thickness) };
        const std::optional<Vector2F> point = match_bot_territory::support_top_point(view, tile, thickness);
        require(point && point->x == 112.5f && point->y < feet,
            "A current horizontal scoring top did not supply an outward paint aim point");
        view.supports[0].one_way = true;
        require(match_bot_territory::support_top_point(view, tile, thickness).has_value(),
            "An independently scoring one-way support lost its local paint role");
        view.supports[0].paintable = false;
        require(!match_bot_territory::support_top_point(view, tile, thickness),
            "An ordinary non-scoring support supplied local scoring paint");
        view.supports[0].paintable = true;
        view.grounded = false;
        require(!match_bot_territory::support_top_point(view, tile, thickness),
            "An airborne player received a current-support paint target");
        view.grounded = true;
        view.center.y -= 10.0f;
        require(!match_bot_territory::support_top_point(view, tile, thickness),
            "A nearby support below the feet was treated as the current support");
        view.center.y = 0.0f;
        tile.face = PaintSurfaceFace::bottom;
        require(!match_bot_territory::support_top_point(view, tile, thickness),
            "A bottom face was supplied as current-support top paint");
        tile.face = PaintSurfaceFace::top;
        tile.corners[1].y += 2.0f;
        require(!match_bot_territory::support_top_point(view, tile, thickness),
            "A rotated local top face was treated as a horizontal standing face");
    }

    void check_aim_only(const GameTuning& tuning)
    {
        for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
        {
            for (const std::uint64_t seed : { 7ull, 19ull, 32ull })
            {
                MatchBot plain(&tuning, difficulty, seed), supplement(&tuning, difficulty, seed);
                MatchBotObservation view;
                view.alive = view.grounded = true;
                view.size = tuning.player().size; view.ammo = 1.0f; view.engage_distance = 800.0f;
                view.obstacles.emplace_back(-2000.0f, view.size.y * 0.5f, 4000.0f, 100.0f);
                view.supports.push_back({ view.obstacles.back(), false, true });
                bool aimed_locally = false;
                for (int step = 0; step < 600; ++step)
                {
                    const PlayerInputData a = plain.think(view);
                    MatchBotObservation local = view;
                    // Removing it after one replan verifies the normal replan
                    // clears a stale local aim rather than keeping it forever.
                    if (step < 180) { local.support_paint = Vector2F(100.0f, 30.0f); }
                    const PlayerInputData b = supplement.think(local);
                    same_navigation(a, b);
                    if (b.shoot_direction_requested && step < 180)
                    {
                        const float expected = Vector2F(100.0f, 30.0f).angle();
                        aimed_locally = aimed_locally || std::fabs(b.shoot_angle - expected) <=
                            tuning.bot(difficulty).aim_error_radians + 0.001f;
                    }
                    if (step >= 400)
                    {
                        require(a.shoot_angle == b.shoot_angle,
                            "A removed support-paint supplement survived normal replanning/reaction");
                    }
                }
                require(aimed_locally && plain.random() == supplement.random(),
                    "Current-support aim was ignored or consumed the navigation random stream");
            }

            MatchBot plain(&tuning, difficulty, 23), supplement(&tuning, difficulty, 23);
            MatchBotObservation view;
            view.alive = view.grounded = true; view.ammo = 1.0f;
            view.size = tuning.player().size; view.engage_distance = 800.0f;
            view.enemies.push_back({ Vector2F(100.0f, 0.0f), Vector2F::ZERO });
            for (int step = 0; step < 60; ++step)
            {
                const PlayerInputData a = plain.think(view);
                MatchBotObservation local = view; local.support_paint = Vector2F(-100.0f, 30.0f);
                const PlayerInputData b = supplement.think(local);
                same_navigation(a, b);
                require(a.shoot_angle == b.shoot_angle,
                    "Local scoring paint displaced a visible opponent's combat aim");
            }
        }
    }

    float owned_top_paint(const Level& level, const RectangleF& support, float thickness)
    {
        float area = 0.0f;
        for (const PaintSurfaceSample& surface : level.paint_surfaces())
        {
            if (surface.face != PaintSurfaceFace::top || surface.paint_owner != paint_owners::index(-1, 1))
            { continue; }
            bool belongs = true;
            for (const Vector2F& corner : surface.corners)
            {
                belongs = belongs && corner.x >= support.left() - endpoint_tolerance &&
                    corner.x <= support.right() + endpoint_tolerance &&
                    std::fabs(corner.y - support.top()) <= thickness + endpoint_tolerance;
            }
            if (belongs) { area += surface.scored_area; }
        }
        return area;
    }

    void check_driver(World& world)
    {
        constexpr int steps = 180;
        int painted = 0;
        for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
        {
            for (int tool = 0; tool < 5; ++tool)
            {
                for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
                {
                    MenuLevelSettings settings;
                    settings.set_player_count(2); settings.set_player_bot(1, difficulty);
                    settings.begin_input_claims(input_slots::keyboard_and_mouse);
                    settings.set_screen_layout(ScreenLayout::one_player);
                    settings.set_stage("turbulence"); settings.set_duration(20.0f);
                    settings.set_player_team(0, team == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a);
                    settings.set_player_team(1, team);
                    settings.set_player_weapon(1, static_cast<WeaponType>(tool));
                    std::unique_ptr<Level> level = world.build(settings);
                    const std::vector<PlayerInputData> physical = physical_inputs();
                    for (int step = 0; step < 900 && level->state() != LevelState::active; ++step)
                    { level->update(physical, dt); }
                    require(level->state() == LevelState::active, "Local territory trial did not finish countdown");
                    const RectangleF& support = world.named("upper_top_paint");
                    const Vector2F size = world.assets.tuning.player().size;
                    require(level->place_player(1, Vector2F(support.center().x - size.x * 0.5f,
                        support.top() - size.y)), "Could not place local territory trial once at its starting support");
                    Tape tape;
                    float maximum_owned = 0.0f;
                    bool paid_fire = false;
                    for (int step = 0; step < steps; ++step)
                    {
                        level->update(physical, dt, &tape);
                        require(level->state() == LevelState::active && level->player(1).state() == PlayerState::alive,
                            "Local territory trial ended or killed its bot outside the finite contract");
                        require(tape.steps.size() == static_cast<std::size_t>(step + 1) &&
                            tape.steps.back().size() == 2 && !tape.steps.back()[0].primary_shoot &&
                            tape.steps.back()[0].x_movement == 0.0f,
                            "Local territory trial lost inputs or changed its stationary human");
                        paid_fire = paid_fire || level->player(1).weapon_ammo() < world.assets.tuning.starting_ammo();
                        maximum_owned = std::max(maximum_owned, owned_top_paint(*level, support,
                            world.assets.tuning.paint().thickness));
                    }
                    std::cout << "MEASURE local support paint team=" << (team == PlayerTeam::a ? 'a' : 'b') <<
                        " tool=" << tool << " difficulty=" << static_cast<int>(difficulty) <<
                        " maximum_top_paint=" << maximum_owned << " paid_fire=" << paid_fire << '\n';
                    require(paid_fire && maximum_owned > 0.0f,
                        "A bot did not pay for ordinary fire and paint its current scoring support within 180 steps");
                    ++painted;
                }
            }
        }
        require(painted == 30, "Local territory trials did not cover their finite contract");
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "usage: bot_territory <content>");
        World world(std::filesystem::absolute(argv[1]), "bot-territory", "Bot territory needs support '", "turbulence");
        check_geometry(world.assets.tuning); check_aim_only(world.assets.tuning); check_driver(world);
        std::cout << "PASS current-support paint geometry, aim-only/RNG/expiry/enemy priority and 30 finite "
            "actual-driver paid top-paint trials; opposed territory usefulness remains unqualified\n";
        return 0;
    }
    catch (const std::exception& error)
    { std::cerr << "FAIL bot territory: " << error.what() << '\n'; return 1; }
}
