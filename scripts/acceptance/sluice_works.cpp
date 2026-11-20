#include "game/pch.h"
#include "game/objects/team_gate_structure_paintable.h"
#include "scripts/acceptance/bot_navigation/fixtures.h"
#include "scripts/acceptance/support/route_run.h"

namespace
{
    using namespace bot_navigation_acceptance;
    namespace route = acceptance::route_run;
    constexpr char stage[] = "sluice_works";

    void check_bridge(route::Services& services, PlayerTeam team)
    {
        const bool west = team == PlayerTeam::a;
        const auto x = [west](float from_west) { return west ? from_west : 3600.0f - from_west; };
        const std::string side = west ? "_a" : "_b";
        const auto spawns = west ? services.map.info.team_a_spawns() : services.map.info.team_b_spawns();
        route::Run run(services, spawns.at(0), team);
        run.settle();
        require(run.support() == "loading_dock" + side, "Spawn did not settle on its loading dock");
        route::drive_to(run, x(614));
        route::jump_to(run, services.map.named("gauge_step" + side), x(800), 0, route::default_jump_hold, true);
        route::drive_to(run, x(1014));
        route::jump_to(run, services.map.named("inspection_landing" + side), x(1200), 0, route::default_jump_hold, true);
        route::drive_to(run, x(1374));
        route::jump_to(run, services.map.named("inspection_bridge"), x(1520), 0, route::default_jump_hold, true);
        route::drive_to(run, x(2090));
        require(run.support() == "inspection_bridge" && run.takeoffs == 3,
            "The closed-gate bypass did not cross the gate from its home dock");
        route::drive_to(run, x(3250));
        require(run.support() == (west ? "loading_dock_b" : "loading_dock_a") && run.takeoffs == 3,
            "The bridge bypass did not descend onto the opposite loading dock");
    }

    MatchSetup humans()
    {
        MatchSetup setup;
        setup.set_player_count(2);
        setup.set_screen_layout(ScreenLayout::two_player);
        setup.set_stage(stage);
        setup.set_duration(120.0f);
        setup.begin_input_claims(0);
        require(setup.claim_input_slot(1), "Could not seat both Sluice Works players");
        setup.set_player_team(0, PlayerTeam::a);
        setup.set_player_team(1, PlayerTeam::b);
        setup.set_player_weapon(0, WeaponType::sprayer);
        setup.set_player_weapon(1, WeaponType::sprayer);
        return setup;
    }

    void check_channel(World& world, int seat)
    {
        auto level = world.build(humans());
        level->start_without_countdown();
        require(level->team_gates().size() == 1, "Channel needs one paint gate");
        const auto& gate = *level->team_gates().front();
        const auto bounds = gate.bounds();
        const Player& player = level->player(seat);
        const bool west = player.team() == PlayerTeam::a;
        const float direction = west ? 1.0f : -1.0f;
        const float face = west ? bounds.left() : bounds.right();
        auto inputs = physical_inputs();
        auto& action = inputs[static_cast<size_t>(seat)];
        action.x_movement = direction;
        for (int tick = 0; tick < 480; ++tick) { level->update(inputs, dt); }
        const float edge = west ? player.rectangle().right() : player.rectangle().left();
        require(std::abs(edge - face) < 1.0f && on_support(player, world.named("channel_floor")) &&
            gate.open_for() == PlayerTeam::none, "The lower lane must walk from spawn to the shut gate");

        // Back up to spray a sweep of the exposed face under the bridge.
        const float firing_x = face - direction * 200.0f;
        for (int tick = 0; tick < 130; ++tick)
        {
            action.x_movement = route::steer(player, firing_x, world.assets.tuning.player());
            level->update(inputs, dt);
        }
        for (int tick = 0; tick < 600 && gate.open_for() != player.team(); ++tick)
        {
            const float height = 28.0f + static_cast<float>(tick % 100) * 2.0f;
            const float aim = std::atan2(bounds.bottom() - height - player.center().y, face - player.center().x);
            action = route::aimed(aim);
            action.primary_shoot = true;
            level->update(inputs, dt);
        }
        const auto paint = gate.paint_total();
        require(gate.open_for() == player.team() && (west ? paint.team_a : paint.team_b) > 0,
            "Shots from the lower lane failed to score and claim the gate");
        const Vector2F a(bounds.left() - 50, bounds.bottom() - 55);
        const Vector2F b(bounds.right() + 50, a.y);
        require(level->bot_traversal_line_of_sight(a, b, player.team()) &&
            !level->bot_traversal_line_of_sight(a, b, opposing(player.team())) && !level->bot_line_of_sight(a, b),
            "The captured channel must grant team traversal while retaining projectile cover");
        action = route::input(direction);
        for (int tick = 0; tick < 150; ++tick) { level->update(inputs, dt); }
        require((west ? player.rectangle().left() > bounds.right() : player.rectangle().right() < bounds.left()) &&
            on_support(player, world.named("channel_floor")), "The gate owner failed to cross the lower channel");
    }

    void observe_bots(World& world)
    {
        const auto setup = two_a_side_roster(stage, 60, PlayerTeam::a,
            { WeaponType::sprayer, WeaponType::roller, WeaponType::bucket, WeaponType::sprayer },
            { false, true, true, true }, BotDifficulty::normal);
        auto level = world.build(setup);
        level->start_without_countdown();
        const auto inputs = physical_inputs();
        const auto& gate = *level->team_gates().front();
        const auto bridge = world.named("inspection_bridge");
        int open_ticks = 0, bridge_ticks = 0, gate_ticks = 0;
        for (int tick = 0; tick < 1200; ++tick)
        {
            level->update(inputs, dt);
            if (gate.open_for() != PlayerTeam::none) { ++open_ticks; }
            for (int seat = 1; seat < 4; ++seat)
            {
                const auto& bot = level->player(seat);
                if (bot.state() != PlayerState::alive) { continue; }
                if (on_support(bot, bridge)) { ++bridge_ticks; }
                const auto body = bot.rectangle();
                const auto wall = gate.bounds();
                if (body.right() > wall.left() + 0.5f && body.left() < wall.right() - 0.5f &&
                    body.bottom() > wall.top() + 0.5f && body.top() < wall.bottom() - 0.5f)
                {
                    require(bot.team() == gate.open_for(), "A bot occupied a gate closed to its team");
                    ++gate_ticks;
                }
            }
        }
        require(open_ticks > 0 && bridge_ticks > 0 && gate_ticks > 0,
            "The seeded bots did not use both the upper bridge and the paint-opened channel");
        std::cout << "MEASURE Sluice Works seed=" << trial_seed << " ticks=1200 open_ticks=" << open_ticks
            << " bot_bridge_ticks=" << bridge_ticks << " bot_gate_ticks=" << gate_ticks << '\n';
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("Sluice Works", [&]
    {
        require(argc == 2, "usage: sluice_works <content>");
        route::Services services(argv[1], stage, "Sluice Works");
        services.paint_accounting = false;
        World world(argv[1], "sluice-works", "Sluice Works needs support '", stage);
        for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b }) { check_bridge(services, team); }
        for (int seat = 0; seat < 2; ++seat) { check_channel(world, seat); }
        observe_bots(world);
        return 0;
    });
}
