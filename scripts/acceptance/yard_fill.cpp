#include "game/pch.h"
#include "game/ai/match_bot_driver.h"
#include "game/objects/paint_fill.h"
#include "scripts/acceptance/bot_navigation/fixtures.h"

namespace
{
    using namespace bot_navigation_acceptance;
    constexpr PlayerTeam a = PlayerTeam::a, b = PlayerTeam::b, neutral = PlayerTeam::none;
    constexpr int human_a = 0, human_b = 1, ally_a = 2, bot_b = paint_owners::index(-1, 2);
    constexpr char stage[] = "turbulence_fill_trial";

    // Use the trial baffle and the two shoulders that clip its side strips.
    // A tiny ordinary projectile can address one tile without also hitting a corner.
    struct Baffle
    {
        paint_surfaces_acceptance::World& assets;
        std::unique_ptr<StructurePaintable> surface;
        PaintFill fill;

        explicit Baffle(World& world) :
            assets(world.assets),
            surface(assets.surface(world.named("centre_baffle"), { true, true, true, true },
                std::array{ world.named("crossover_a"), world.named("crossover_b") })),
            fill(*surface, { PaintSurfaceFace::left, PaintSurfaceFace::right }, *surface, PaintSurfaceFace::top,
                assets.tuning.paint().fill.open_share, assets.tuning.paint().fill.cooldown)
        {
            require(fill.trigger_total() == 1920 && fill.trigger_tiles().size() == 40 &&
                fill.target_tiles().size() == 30, "Trial region must have 40 trigger and 30 destination tiles");
            require(fill.leader() == neutral && fill.last_team() == neutral && fill.cooldown_remaining() == 0,
                "A new round retained fill state");
        }

        void contact(PaintSurfaceFace face, int first, int count, PlayerTeam team, int owner)
        {
            const auto tiles = face == PaintSurfaceFace::top ? fill.target_tiles() : fill.trigger_tiles();
            int index = 0, painted = 0;
            for (const auto& tile : tiles)
            {
                if (tile.face != face) { continue; }
                if (index >= first && index < first + count)
                {
                    const auto r = paint_surfaces_acceptance::static_rectangle(tile);
                    const auto p = r.center();
                    assets.paint(*surface, RectangleF(p.x - 0.25f, p.y - 0.25f, 0.5f, 0.5f), team, owner);
                    ++painted;
                }
                ++index;
            }
            require(painted == count, "Missing requested ordinary contact tiles");
        }
        void advance(int steps)
        {
            for (int i = 0; i < steps; ++i) { fill.settle(dt); }
        }
    };

    void check_results(const PaintTotal& local, int tenths_a, int tenths_b)
    {
        // ROAD#43's worked match keeps all paint outside the baffle fixed.
        LevelEndInfo end;
        end.team_a_area = 23520 + local.team_a;
        end.team_b_area = 24000 + local.team_b;
        require(percentage_tenths(end.team_a_percentage()) == tenths_a &&
            percentage_tenths(end.team_b_percentage()) == tenths_b, "Worked match results shares changed");
    }

    void worked_match(World& world, bool pending_during_cooldown)
    {
        Baffle f(world);
        const float total = f.surface->paint_total().total;
        f.contact(PaintSurfaceFace::left, 5, 9, a, human_a);
        f.advance(1);
        require(f.fill.last_team() == neutral && f.fill.trigger_area(a) == 432,
            "A lead below 25 percent filled the target");
        f.contact(PaintSurfaceFace::left, 14, 1, a, human_a);
        require(f.surface->paint_total().team_a == 480 && f.fill.last_team() == neutral,
            "The threshold contact must apply ordinary paint before settlement");
        check_results(f.surface->paint_total(), 500, 500);
        f.advance(1);
        const auto first = f.surface->paint_total();
        require(f.fill.last_team() == a && f.fill.changed_tiles().size() == 30 &&
            first.team_a == 1920 && first.credit.laid[human_a] == 1920 && first.credit.recaptured[human_a] == 0 &&
            std::abs(f.fill.cooldown_remaining() - 3.0f) < 0.02f,
            "A's 480 trigger paint did not fill the neutral 1440-square-unit top");
        check_results(first, 514, 485);
        if (!pending_during_cooldown) { f.advance(185); }
        f.contact(PaintSurfaceFace::right, 5, 10, b, human_b);
        f.advance(1);
        require(f.fill.leader() == neutral && f.fill.last_team() == a && f.fill.pending_owner() == -1,
            "A strict tie must clear pending attribution and remember the previous activation");
        f.contact(PaintSurfaceFace::right, 15, 1, b, bot_b);
        require(f.fill.leader() == b && f.fill.pending_owner() == bot_b &&
            f.fill.trigger_area(b) == 528 && f.surface->paint_total().team_b == 528,
            "B's 11th trigger tile did not establish the bot's pending credit");
        check_results(f.surface->paint_total(), 509, 490);
        if (pending_during_cooldown)
        {
            f.contact(PaintSurfaceFace::right, 16, 1, b, human_b);
            const auto before = f.fill.cooldown_remaining();
            for (int i = 0; i < 30; ++i) { f.fill.settle(0.0f); }
            require(f.fill.cooldown_remaining() == before && f.fill.pending_owner() == bot_b,
                "Paused time or an assisting teammate changed a pending activation");
            f.advance(170);
            require(f.fill.last_team() == a && f.fill.cooldown_remaining() > 0,
                "The target flipped before the three live seconds expired");
            f.advance(15);
        }
        else { f.advance(1); }
        const auto second = f.surface->paint_total();
        if (!pending_during_cooldown) { check_results(second, 480, 519); }
        const float human_credit = pending_during_cooldown ? 528.0f : 480.0f;
        require(f.fill.last_team() == b && second.total == total && second.team_a == 480 &&
            second.team_b == human_credit + 1488 && second.player_areas[human_b] == human_credit &&
            second.player_areas[bot_b] == 1488 && second.credit.laid[bot_b] == 1488 &&
            second.credit.recaptured[bot_b] == 1440 && second.credit.laid[human_b] == human_credit,
            "B's bot did not receive the 1440-area recapture while its teammate kept ordinary paint");
        for (const auto& tile : f.fill.target_tiles())
        {
            require(tile.team == b && tile.paint_owner == bot_b && tile.scored_area == 48,
                "A recaptured target tile lost its bot credit or changed size");
        }
        f.advance(200);
        require(f.fill.cooldown_remaining() == 0 && f.surface->paint_total().credit.laid == second.credit.laid,
            "A maintained lead fired a second fill without an opposing activation");
    }

    void opposing_contacts(World& world)
    {
        Baffle f(world);
        f.contact(PaintSurfaceFace::left, 5, 10, a, human_a);
        f.contact(PaintSurfaceFace::right, 5, 10, b, human_b);
        f.advance(1);
        require(f.fill.last_team() == neutral && f.surface->paint_total().team_a == 480 &&
            f.surface->paint_total().team_b == 480, "A transient leader filled before same-step paint settled");
        f.contact(PaintSurfaceFace::right, 15, 1, b, bot_b);
        f.contact(PaintSurfaceFace::left, 15, 1, a, human_a);
        require(f.fill.pending_owner() == -1, "A tie retained the bot's earlier lead");
        f.contact(PaintSurfaceFace::right, 16, 1, b, human_b);
        f.advance(1);
        require(f.surface->paint_total().credit.laid[human_b] == 1968 &&
            f.surface->paint_total().credit.laid[bot_b] == 48, "Surviving lead used a superseded paint owner");
    }

    void expired_tie(World& world)
    {
        Baffle f(world);
        f.contact(PaintSurfaceFace::left, 5, 10, a, human_a);
        f.advance(1);
        f.contact(PaintSurfaceFace::right, 5, 11, b, bot_b);
        f.advance(20);
        require(f.fill.pending_owner() == bot_b && f.fill.last_team() == a,
            "B's first challenger did not establish its lead during cooldown");
        f.contact(PaintSurfaceFace::left, 15, 1, a, human_a);
        f.advance(185);
        require(f.fill.last_team() == a && f.fill.cooldown_remaining() == 0 &&
            f.fill.pending_owner() == -1 && f.surface->paint_total().credit.recaptured[bot_b] == 0,
            "A lead that disappeared during cooldown filled on expiry");
        f.contact(PaintSurfaceFace::right, 16, 1, b, human_b);
        f.advance(1);
        require(f.fill.last_team() == b && f.surface->paint_total().credit.recaptured[human_b] == 1440,
            "The new lead after expiry did not credit the player who re-established it");
    }

    void target_ownership(World& world)
    {
        Baffle f(world);
        f.contact(PaintSurfaceFace::top, 5, 5, a, ally_a);
        f.contact(PaintSurfaceFace::top, 10, 5, b, bot_b);
        f.contact(PaintSurfaceFace::left, 5, 10, a, human_a);
        f.advance(1);
        const auto mixed = f.surface->paint_total();
        require(mixed.player_areas[ally_a] == 240 && mixed.credit.laid[human_a] == 480 + 1200 &&
            mixed.credit.recaptured[human_a] == 240 && f.fill.changed_tiles().size() == 25,
            "Mixed destination ownership failed to preserve friendly tiles or separate recaptured area");
        Baffle friendly(world);
        friendly.surface->fill_paint_face(PaintSurfaceFace::top, a, ally_a);
        friendly.contact(PaintSurfaceFace::left, 5, 10, a, human_a);
        friendly.advance(1);
        require(friendly.fill.last_team() == a && friendly.fill.cooldown_remaining() > 0 &&
            friendly.fill.changed_tiles().empty() && friendly.surface->paint_total().credit.laid[human_a] == 480 &&
            friendly.surface->paint_total().player_areas[ally_a] == 1440,
            "An already friendly target must spend the activation without changing paint ownership");
    }

    void distinct_objects(World& world)
    {
        auto trigger = world.assets.surface({ 0, 0, 40, 80 }, { true, false, false, false });
        auto target = world.assets.surface({ 120, 0, 80, 40 }, { false, true, false, false });
        PaintFill fill(*trigger, { PaintSurfaceFace::left }, *target, PaintSurfaceFace::top,
            world.assets.tuning.paint().fill.open_share, world.assets.tuning.paint().fill.cooldown);
        world.assets.paint(*trigger, { 2, 25, 2, 25 }, a, human_a);
        fill.settle(dt);
        require(fill.last_team() == a && target->paint_total().team_a == 960 &&
            trigger->paint_total().team_a < trigger->paint_total().total,
            "A separate target did not fill independently of the trigger");
        world.assets.paint(*target, { 140, 2, 20, 2 }, b, human_b);
        for (int i = 0; i < 185; ++i) { fill.settle(dt); }
        require(target->paint_total().team_b > 0 && fill.last_team() == a && fill.trigger_area(b) == 0,
            "Ordinary post-fill target repaint disturbed the trigger or was overwritten by a maintained lead");
    }

    std::optional<BotFillObservation> bot_view(World& world, const Level& level)
    {
        const auto& bot_world = level.bot_world();
        const auto body = bot_world.body(1);
        const auto walls = bot_world.planted_subs();
        const float distance = world.assets.tuning.bots().sight_distance;
        return bot_world.visible_fill(body.team, [&](Vector2F point)
        {
            return Vector2F::distance_squared(body.center, point) <= distance * distance &&
                MatchBotDriver::clear_sight(bot_world, walls, body.team, body.center, point);
        });
    }

    void check_match(World& world)
    {
        auto settings = one_bot_roster(stage, 120, b, WeaponType::sprayer, BotDifficulty::normal);
        auto level = world.build(settings);
        auto inputs = physical_inputs();
        level->start_without_countdown();
        for (int i = 0; i < 240 && level->state() != LevelState::active; ++i) { level->update(inputs, dt); }
        require(level->state() == LevelState::active && level->paint_fill() &&
            level->level_end_info().total_area == 148800, "Registered Yard Fill did not build its fixed region");
        require(!bot_view(world, *level), "The fill state leaked to the bot at its distant spawn");
        require(level->place_player(0, { 1670, 680 }), "Cannot position the sprayer on the crossover");
        const auto& fill = *level->paint_fill();
        auto& input = inputs.front();
        for (int tick = 0; tick < 240 && fill.last_team() == neutral; ++tick)
        {
            const auto centre = level->player(0).center();
            const float y = 738.0f + static_cast<float>(tick % 50);
            const float angle = std::atan2(y - centre.y, 1940.0f - centre.x);
            input.primary_shoot = true;
            input.shoot_direction_requested = true;
            input.shoot_angle = angle;
            input.shoot_direction = Vector2F::unit_vec_from_angle(angle);
            level->update(inputs, dt);
        }
        const auto end = level->level_end_info();
        require(fill.last_team() == a && fill.trigger_area(a) >= 480 && end.total_area == 148800 &&
            end.team_a_area >= 1920, "Ordinary human sprayer input did not settle a scored fill in the live Level");
        const int owner = paint_owners::index(input_slots::keyboard_and_mouse, 0);
        for (const auto& tile : fill.target_tiles())
        {
            require(tile.team == a && tile.paint_owner == owner, "Live top did not credit the human trigger owner");
        }
        require(level->place_player(1, { 2240, 580 }), "Cannot position the bot observer");
        const auto view = bot_view(world, *level);
        require(view && view->last_team == a && view->leader == a && !view->ready &&
            view->own_trigger_share == fill.trigger_area(b) / fill.trigger_total() &&
            !view->target_tiles.empty() && !view->trigger_tiles.empty(),
            "Bot in sight of the baffle lost visible ownership, qualification or readiness");
        for (const auto& tile : view->trigger_tiles)
        {
            require(tile.point.x > 2060, "The bot saw the baffle's hidden opposite trigger face");
        }
        for (const auto& tile : view->target_tiles)
        {
            require(tile.team == a, "Bot observed stale ownership of freshly filled visible tiles");
        }
        level.reset();
        level = world.build(settings);
        const auto fresh = level->level_end_info();
        require(level->paint_fill()->last_team() == neutral && level->paint_fill()->leader() == neutral &&
            level->paint_fill()->cooldown_remaining() == 0 && fresh.team_a_area == 0 && fresh.team_b_area == 0 &&
            fresh.total_area == end.total_area, "A new match retained fill state, award paint or a changed denominator");
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("Yard Fill", [&]
    {
        require(argc == 2, "usage: yard_fill <content>");
        World world(argv[1], "yard-fill", "Yard Fill needs support '", stage);
        worked_match(world, false);
        worked_match(world, true);
        opposing_contacts(world);
        expired_tie(world);
        target_ownership(world);
        distinct_objects(world);
        check_match(world);
        return 0;
    });
}
