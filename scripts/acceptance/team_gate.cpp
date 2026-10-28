#include "game/pch.h"
#include "scripts/acceptance/bot_navigation/fixtures.h"
#include "game/objects/team_gate_structure_paintable.h"
#include "game/team_gate_contract.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>

// ROAD#15's second paint-activated stage object: the team gate, a solid
// paintable wall that the team holding more of its paint (and at least the
// tuned share of its area) walks through, and that never closes on a player
// still inside it. The rule checks drive the shared contract and a
// standalone gate; the walk checks play the explicit Turbulence Yard gate
// trial with two scripted humans; the stage checks play whole bot matches and
// hold every step of the gate to the rule, from the paint held before it.
namespace
{
    using namespace labrador;
    using namespace mattmath;
    using namespace bot_navigation_acceptance;
    namespace gate_rule = team_gate_contract;

    bool decode_refused(const gate_rule::Record& record)
    {
        try { gate_rule::decode(record); }
        catch (const std::invalid_argument&) { return true; }
        return false;
    }

    void check_contract()
    {
        using gate_rule::Leader;
        require(gate_rule::leader(0.0f, 0.0f, 100.0f, 0.25f) == Leader::none, "An unpainted gate opened");
        require(gate_rule::leader(30.0f, 30.0f, 100.0f, 0.25f) == Leader::none, "A tied gate opened");
        require(gate_rule::leader(24.0f, 0.0f, 100.0f, 0.25f) == Leader::none, "A gate opened below its share");
        require(gate_rule::leader(25.0f, 0.0f, 100.0f, 0.25f) == Leader::team_a, "A gate at its share stayed shut");
        require(gate_rule::leader(30.0f, 40.0f, 100.0f, 0.25f) == Leader::team_b, "A gate ignored team B's lead");
        require(gate_rule::leader(40.0f, 30.0f, 100.0f, 0.5f) == Leader::none,
            "A lead under the share opened the gate");
        require(gate_rule::leader(1.0f, 0.0f, 100.0f, 0.0f) == Leader::team_a &&
            gate_rule::leader(0.0f, 0.0f, 100.0f, 0.0f) == Leader::none, "A zero share misread a lead or no paint");
        for (const float bad : { -0.1f, 1.5f, std::numeric_limits<float>::quiet_NaN() })
        {
            bool refused = false;
            try { gate_rule::validate_open_share(bad); }
            catch (const std::invalid_argument&) { refused = true; }
            require(refused, "A gate accepted an open share outside 0..1");
        }

        const gate_rule::Record good{ std::string("paint"), true, true, false, true };
        require(!decode_refused(good) && gate_rule::decode(good), "A solid static painted gate was refused");
        require(!gate_rule::decode({ std::nullopt, true, true, false, true }), "An absent gate decoded as a gate");
        gate_rule::Record bad = good; bad.gate = "wind";
        require(decode_refused(bad), "An unknown gate rule was accepted");
        bad = good; bad.structure_paintable = false;
        require(decode_refused(bad), "A gate off a StructurePaintable was accepted");
        bad = good; bad.solid = false;
        require(decode_refused(bad), "A one-way gate was accepted");
        bad = good; bad.moving = true;
        require(decode_refused(bad), "A moving gate was accepted");
        bad = good; bad.any_paintable_face = false;
        require(decode_refused(bad), "A gate no paint could open was accepted");
        bool spawn_refused = false;
        try { gate_rule::require_clear_of_spawn({ 0.0f, 0.0f, 80.0f, 240.0f }, { 79.0f, 100.0f, 52.0f, 120.0f }); }
        catch (const std::invalid_argument&) { spawn_refused = true; }
        gate_rule::require_clear_of_spawn({ 0.0f, 0.0f, 80.0f, 240.0f }, { 80.0f, 100.0f, 52.0f, 120.0f });
        require(spawn_refused, "A spawn body inside a gate was accepted");
        std::cout << "PASS team gate rule: no paint, ties and leads under the share stay shut; the lead at its "
            "share opens; malformed, one-way, moving, unpainted and spawn-covering gates are refused\n";
    }

    void check_object(World& world)
    {
        paint_surfaces_acceptance::World& assets = world.assets;
        const RectangleF bounds(400.0f, 300.0f, 80.0f, 240.0f);
        TeamGateStructurePaintable gate("sprite_sheet_1", "square_white_4", bounds, &assets.render, &assets.audio,
            &assets.preferences, &assets.tuning.paint(), TeamColour{}, { true, true, true, false });
        const CollisionMask all = collision_mask(CollisionObjectType::structure_paintable);
        require(gate.open_for() == PlayerTeam::none && gate.mask() == all && gate.tag() ==
            to_collision_tag(CollisionObjectType::structure_paintable), "A new gate is not a closed solid paintable");
        const std::vector<Player*> nobody;
        // A splash on part of one face is under the 0.25 share.
        assets.paint(gate, RectangleF(390.0f, 300.0f, 20.0f, 40.0f), PlayerTeam::a, 0);
        gate.settle(nobody);
        require(gate.paint_leader() == PlayerTeam::none && gate.open_for() == PlayerTeam::none,
            "A gate opened for a splash under its share");
        assets.paint(gate, RectangleF(390.0f, 290.0f, 40.0f, 260.0f), PlayerTeam::a, 0);
        require(gate.paint_leader() == PlayerTeam::a && gate.open_for() == PlayerTeam::none,
            "Paint alone changed the gate before it was settled");
        gate.settle(nobody);
        require(gate.open_for() == PlayerTeam::a && gate.mask() == (all & ~collision_layers::PLAYER_TEAM_A),
            "A gate held by team A did not let team A through only");
        require((gate.mask() & collision_layers::ALL_PROJECTILES) == collision_layers::ALL_PROJECTILES &&
            (gate.mask() & collision_layers::PLAYER_TEAM_B) != 0u, "An open gate stopped taking shots or team B");
        assets.paint(gate, RectangleF(380.0f, 280.0f, 120.0f, 280.0f), PlayerTeam::b, 2);
        gate.settle(nobody);
        require(gate.open_for() == PlayerTeam::b && gate.mask() == (all & ~collision_layers::PLAYER_TEAM_B),
            "A gate retaken by team B did not open for team B only");
        std::cout << "PASS team gate object: opens only once settled, for the leader only, and keeps taking shots\n";
    }

    bool contains(const RectangleF& outer, const RectangleF& inner)
    {
        return inner.left() >= outer.left() && inner.top() >= outer.top() &&
            inner.right() <= outer.right() && inner.bottom() <= outer.bottom();
    }

    void check_permission_geometry(World& world)
    {
        auto& assets = world.assets;
        const PaintTuning& paint = assets.tuning.paint();
        const auto build = [&](const RectangleF& body, const GatePermissionPresentation* cue)
        {
            return std::make_unique<TeamGateStructurePaintable>("sprite_sheet_1", "square_white_4", body,
                &assets.render, &assets.audio, &assets.preferences, &paint, TeamColour{},
                PaintableFaces{ true, true, true, true }, std::span<const RectangleF>{},
                Colour::white, nullptr, std::span<const PaintPolygon>{}, cue);
        };
        const GatePermissionPresentation& authored = assets.presentation.gate_permission();
        const RectangleF large(400.0f, 300.0f, 400.0f, 400.0f);
        require(build(large, nullptr)->permission_bounds() == RectangleF::ZERO,
            "A gate without a cue published a permission plaque");
        require(build(large, &authored)->permission_bounds() ==
            RectangleF(large.center().x - authored.size.x * 0.5f,
                large.top() + std::max(authored.inset, paint.thickness), authored.size.x, authored.size.y),
            "A roomy gate did not retain the authored plaque size and upper interior placement");

        GatePermissionPresentation paint_inset = authored;
        paint_inset.inset = 0.0f;
        GatePermissionPresentation wide_inset = authored;
        wide_inset.inset = paint.thickness + 16.0f;
        for (const GatePermissionPresentation* cue :
            std::array<const GatePermissionPresentation*, 3>{ &authored, &paint_inset, &wide_inset })
        {
            const float inset = std::max(cue->inset, paint.thickness);
            for (const RectangleF& body : {
                RectangleF(31.0f, 47.0f, inset * 2.0f, 240.0f),
                RectangleF(31.0f, 47.0f, 240.0f, inset * 2.0f),
                RectangleF(31.0f, 47.0f, inset, 240.0f),
                RectangleF(31.0f, 47.0f, 240.0f, inset) })
            {
                require(build(body, cue)->permission_bounds() == RectangleF::ZERO,
                    "A gate without a paint-free interior published a permission plaque");
            }
            for (const RectangleF& body : {
                large,
                RectangleF(31.0f, 47.0f, inset * 2.0f + 0.125f, 240.0f),
                RectangleF(31.0f, 47.0f, 240.0f, inset * 2.0f + 0.125f),
                RectangleF(31.0f, 47.0f, inset * 2.0f + 0.25f, inset * 2.0f + 0.25f) })
            {
                const auto gate = build(body, cue);
                const RectangleF plaque = gate->permission_bounds();
                const RectangleF interior(body.x + inset, body.y + inset,
                    body.width - inset * 2.0f, body.height - inset * 2.0f);
                require(std::isfinite(plaque.x) && std::isfinite(plaque.y) &&
                    std::isfinite(plaque.width) && std::isfinite(plaque.height) &&
                    plaque.width > 0.0f && plaque.height > 0.0f,
                    "A narrow or shallow gate produced an empty or invalid permission plaque");
                require(contains(interior, plaque), "A permission plaque covered a paintable face");
                require(plaque.width <= cue->size.x && plaque.height <= cue->size.y,
                    "A small gate grew its permission plaque beyond the authored size");
                for (const float units : { 0.5f, 1.0f, 2.0f, 4.0f, 16.0f })
                {
                    require(contains(gate->cull_bounds(units), plaque),
                        "A permission plaque escaped its declared cull at a pane or miniature scale");
                }
            }
        }

        const RectangleF body(400.0f, 300.0f, 80.0f, 240.0f);
        const auto plain = build(body, nullptr);
        const auto cued = build(body, &authored);
        const std::vector<Player*> nobody;
        const auto unchanged = [&]()
        {
            const auto plain_surfaces = paint_surfaces_acceptance::snapshot(*plain);
            const auto cued_surfaces = paint_surfaces_acceptance::snapshot(*cued);
            require(paint_surfaces_acceptance::sample_state(plain_surfaces) ==
                paint_surfaces_acceptance::sample_state(cued_surfaces),
                "Adding a permission cue changed exact paint geometry, ownership or credit");
            paint_surfaces_acceptance::same_total(plain->paint_total(), cued->paint_total());
            require(plain->mask() == cued->mask() && plain->open_for() == cued->open_for() &&
                plain->held_open() == cued->held_open(), "Adding a permission cue changed collision permission");
            for (const float units : { 0.5f, 1.0f, 2.0f, 4.0f, 16.0f })
            {
                require(plain->cull_bounds(units) == cued->cull_bounds(units),
                    "Adding an interior permission cue changed paint culling bounds");
            }
        };
        unchanged();
        for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b, PlayerTeam::a })
        {
            for (const RectangleF& splash : { RectangleF(390.0f, 290.0f, 40.0f, 120.0f),
                RectangleF(380.0f, 280.0f, 120.0f, 280.0f) })
            {
                const int owner = team == PlayerTeam::a ? 0 : 1;
                assets.paint(*plain, splash, team, owner);
                assets.paint(*cued, splash, team, owner);
                plain->settle(nobody); cued->settle(nobody);
                plain->update(dt); cued->update(dt);
                unchanged();
            }
        }
        std::cout << "PASS gate cue geometry: absent and interior-less cues are empty; narrow/shallow plaques "
            "stay clear of paint and inside culling; cues preserve exact paint, credit, permission and culling\n";
    }

    std::unique_ptr<Level> two_humans(World& world)
    {
        MenuLevelSettings settings;
        settings.set_player_count(2);
        settings.set_screen_layout(ScreenLayout::two_player);
        settings.set_stage("turbulence_gate_trial");
        settings.set_duration(120.0f);
        settings.set_player_team(0, PlayerTeam::a); settings.set_player_team(1, PlayerTeam::b);
        settings.set_player_weapon(0, WeaponType::sprayer); settings.set_player_weapon(1, WeaponType::sprayer);
        settings.begin_input_claims(0);
        require(settings.claim_input_slot(1), "Cannot claim the second real input slot");
        return world.build(settings, 5150);
    }

    // ROAD#26: the cue reports the reason recorded at settlement, not a
    // comparison of permission with paint that may have arrived afterwards.
    void check_settled_hold_reason(World& world)
    {
        auto& assets = world.assets;
        for (const PlayerTeam permitted : { PlayerTeam::a, PlayerTeam::b })
        {
            std::unique_ptr<Level> level = two_humans(world);
            const std::vector<PlayerInputData> idle = physical_inputs();
            for (int tick = 0; tick < 900 && level->state() != LevelState::active; ++tick)
            { level->update(idle, dt); }
            require(level->state() == LevelState::active, "Hold-reason fixture did not reach play");
            const RectangleF bounds(1960.0f, 1040.0f, 80.0f, 240.0f);
            TeamGateStructurePaintable gate("sprite_sheet_1", "square_white_4", bounds,
                &assets.render, &assets.audio, &assets.preferences, &assets.tuning.paint(),
                TeamColour{}, { true, false, true, false });
            const std::vector<Player*> nobody;
            const int seat = permitted == PlayerTeam::a ? 0 : 1;
            Player& player = const_cast<Player&>(level->player(seat));
            const PlayerTeam hostile = permitted == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a;
            const std::vector<Player*> occupant{ nullptr, &player };
            const RectangleF whole(1940.0f, 1020.0f, 120.0f, 280.0f);
            const Vector2F inside(bounds.left() + 14.0f,
                bounds.bottom() - level->player(seat).rectangle().height);
            const Vector2F clear(bounds.right() + 100.0f, inside.y);
            require(gate.open_for() == PlayerTeam::none && !gate.held_open(),
                "A new gate reported an occupied hold");
            gate.settle(nobody);
            require(!gate.held_open(), "An unpainted gate reported an occupied hold");

            // Equal independent side faces give an exact nonzero paint tie.
            assets.paint(gate, RectangleF(1940.0f, 1020.0f, 40.0f, 280.0f), PlayerTeam::a, 0);
            assets.paint(gate, RectangleF(2020.0f, 1020.0f, 40.0f, 280.0f), PlayerTeam::b, 1);
            const PaintTotal tie = gate.paint_total();
            require(tie.team_a > 0.0f && tie.team_a == tie.team_b,
                "Hold-reason fixture did not create an exact painted tie");
            gate.settle(nobody);
            require(gate.open_for() == PlayerTeam::none && !gate.held_open(),
                "A painted tie opened or reported an occupied hold");

            assets.paint(gate, whole, permitted, seat);
            gate.settle(nobody);
            require(gate.open_for() == permitted && !gate.held_open(),
                "A paint-driven opening reported an occupied hold");
            level->place_player(seat, inside);
            gate.settle(occupant);
            require(!gate.held_open(), "An occupant holding the paint lead invented a hold reason");

            assets.paint(gate, whole, hostile, 1 - seat);
            require(gate.paint_leader() == hostile && gate.open_for() == permitted && !gate.held_open(),
                "Paint after settlement invented an occupied hold before settlement");
            gate.settle(occupant);
            require(gate.open_for() == permitted && gate.held_open(),
                "Occupied takeover did not publish its actual hold reason");

            // The published reason remains that settlement's observation even
            // when a later shot restores the permitted team's paint lead.
            assets.paint(gate, whole, permitted, seat);
            require(gate.paint_leader() == permitted && gate.held_open(),
                "A later shot rewrote the settled occupied-hold reason");
            gate.settle(occupant);
            require(gate.open_for() == permitted && !gate.held_open(),
                "Regaining the paint lead left an obsolete occupied-hold reason");

            assets.paint(gate, whole, hostile, 1 - seat);
            gate.settle(occupant);
            require(gate.held_open(), "Repeated occupied takeover lost its hold reason");
            level->place_player(seat, clear);
            gate.settle(occupant);
            require(gate.open_for() == hostile && !gate.held_open(),
                "Leaving the gate did not clear the hold reason and change permission");

            // A hostile occupant cannot preserve the current permission.
            level->place_player(seat, inside);
            assets.paint(gate, whole, permitted, seat);
            gate.settle(occupant);
            require(gate.open_for() == permitted && !gate.held_open(),
                "An occupant of the other team falsely preserved permission");

            // A tie closes after the permitted occupant leaves, and its hold
            // reason clears at the same settlement as the permission.
            assets.paint(gate, RectangleF(1940.0f, 1020.0f, 40.0f, 280.0f), PlayerTeam::a, 0);
            assets.paint(gate, RectangleF(2020.0f, 1020.0f, 40.0f, 280.0f), PlayerTeam::b, 1);
            gate.settle(occupant);
            require(gate.paint_leader() == PlayerTeam::none && gate.open_for() == permitted && gate.held_open(),
                "A painted tie did not retain occupied permission with a hold reason");
            level->place_player(seat, clear);
            gate.settle(occupant);
            require(gate.open_for() == PlayerTeam::none && !gate.held_open(),
                "Final closure retained an occupied hold");

            assets.paint(gate, whole, permitted, seat);
            gate.settle(nobody);
            level->place_player(seat, inside);
            assets.paint(gate, whole, hostile, 1 - seat);
            gate.settle(occupant);
            require(gate.held_open(), "Death control did not establish an occupied hold");
            player.kill();
            player.update(dt);
            require(player.state() == PlayerState::dead, "Death control left its occupant alive");
            player.place_at(inside);
            gate.settle(occupant);
            require(gate.open_for() == hostile && !gate.held_open(),
                "A dead permitted occupant retained an occupied hold");
        }
        std::cout << "PASS gate hold reason: either team, exact painted ties, actual occupied takeover, "
            "late paint, lead restoration, hostile/dead occupants, departure and final closure\n";
    }

    bool overlaps(const RectangleF& a, const RectangleF& b, float tolerance = 0.0f)
    {
        return a.left() < b.right() - tolerance && b.left() < a.right() - tolerance &&
            a.top() < b.bottom() - tolerance && b.top() < a.bottom() - tolerance;
    }

    // Walk one seat for a number of steps; the other stands still.
    void walk(Level& level, int seat, float direction, int steps)
    {
        std::vector<PlayerInputData> inputs = physical_inputs();
        inputs[static_cast<std::size_t>(seat)].x_movement = direction;
        for (int step = 0; step < steps; ++step) { level.update(inputs, dt); }
    }

    void check_walks(World& world)
    {
        std::unique_ptr<Level> level = two_humans(world);
        require(level->team_gates().size() == 1, "The gate trial needs exactly one gate");
        auto& gate = const_cast<TeamGateStructurePaintable&>(*level->team_gates()[0]);
        const RectangleF bounds = gate.bounds();
        require(bounds == RectangleF(1960.0f, 1040.0f, 80.0f, 240.0f), "The gate trial's gate moved");
        const std::vector<PlayerInputData> idle = physical_inputs();
        for (int tick = 0; tick < 900 && level->state() != LevelState::active; ++tick) { level->update(idle, dt); }
        require(level->state() == LevelState::active, "The gate trial did not finish its countdown");
        require(gate.open_for() == PlayerTeam::none, "An unpainted gate opened during the countdown");

        const float floor_top = 1280.0f;
        const float body_height = level->player(0).rectangle().height;
        const float body_width = level->player(0).rectangle().width;
        const Vector2F west(bounds.left() - body_width - 8.0f, floor_top - body_height);
        const Vector2F east(bounds.right() + 8.0f, floor_top - body_height);
        constexpr int crossing = 150;

        // Closed to both teams.
        level->place_player(0, west); level->place_player(1, east);
        walk(*level, 0, 1.0f, crossing); walk(*level, 1, -1.0f, crossing);
        require(level->player(0).rectangle().right() <= bounds.left() + endpoint_tolerance &&
            level->player(1).rectangle().left() >= bounds.right() - endpoint_tolerance,
            "A closed gate let a player through");

        // Team A's paint opens it for team A only.
        world.assets.paint(gate, RectangleF(1940.0f, 1020.0f, 60.0f, 280.0f), PlayerTeam::a, 0);
        level->place_player(0, west); level->place_player(1, east);
        level->update(idle, dt);
        require(gate.open_for() == PlayerTeam::a, "Team A's paint did not open the gate for team A");
        walk(*level, 0, 1.0f, crossing);
        require(level->player(0).rectangle().left() >= bounds.right(), "Team A could not walk through its gate");
        walk(*level, 1, -1.0f, crossing);
        require(level->player(1).rectangle().left() >= bounds.right() - endpoint_tolerance,
            "Team B walked through team A's gate");

        // Retaken while a team A player stands inside: it stays open for
        // team A until they are clear, so it never closes on them.
        level->place_player(0, Vector2F(bounds.left() + 14.0f, floor_top - body_height));
        level->place_player(1, east);
        world.assets.paint(gate, RectangleF(1940.0f, 1020.0f, 120.0f, 280.0f), PlayerTeam::b, 1);
        require(gate.paint_leader() == PlayerTeam::b, "Team B's paint did not take the gate");
        for (int step = 0; step < 30; ++step) { level->update(idle, dt); }
        require(gate.open_for() == PlayerTeam::a && overlaps(level->player(0).rectangle(), bounds),
            "The gate closed on a team A player inside it");
        walk(*level, 1, -1.0f, crossing);
        require(level->player(1).rectangle().left() >= bounds.right() - endpoint_tolerance,
            "Team B walked through while team A held the gate open");
        walk(*level, 0, -1.0f, crossing);
        require(level->player(0).rectangle().right() <= bounds.left() && gate.open_for() == PlayerTeam::b,
            "The gate did not change hands once team A was clear");

        // Now team B walks through and team A is shut out.
        level->place_player(0, west); level->place_player(1, east);
        walk(*level, 1, -1.0f, crossing);
        require(level->player(1).rectangle().right() <= bounds.left(), "Team B could not walk through its gate");
        walk(*level, 0, 1.0f, crossing);
        require(level->player(0).rectangle().right() <= bounds.left() + endpoint_tolerance,
            "Team A walked through team B's gate");
        std::cout << "PASS team gate walks: shut to both unpainted, open to its leader only, and held open "
            "for a player inside until they are clear\n";
    }

    struct Roster
    {
        unsigned int seed;
        std::array<WeaponType, 4> tools;
        BotDifficulty difficulty;
    };

    // Seat 0 is an idle human on team A; seats 1-3 are bots, two of them on B.
    MenuLevelSettings roster(const Roster& variant)
    {
        MenuLevelSettings settings;
        settings.set_player_count(4);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_screen_layout(ScreenLayout::one_player);
        settings.set_stage("turbulence_gate_trial");
        settings.set_duration(120.0f);
        for (int seat = 0; seat < 4; ++seat)
        {
            if (seat > 0) { settings.set_player_bot(seat, variant.difficulty); }
            settings.set_player_team(seat, seat < 2 ? PlayerTeam::a : PlayerTeam::b);
            settings.set_player_weapon(seat, variant.tools[static_cast<std::size_t>(seat)]);
        }
        return settings;
    }

    struct Totals
    {
        int open_steps = 0;
        int bot_passes = 0;
    };

    void check_stage(World& world, const Roster& variant, Totals& totals)
    {
        std::unique_ptr<Level> level = world.build(roster(variant), variant.seed);
        require(level->team_gates().size() == 1, "The gate trial lost its gate");
        const TeamGateStructurePaintable& gate = *level->team_gates()[0];
        const RectangleF bounds = gate.bounds();
        const std::vector<PlayerInputData> physical = physical_inputs();
        for (int tick = 0; tick < 900 && level->state() != LevelState::active; ++tick) { level->update(physical, dt); }
        require(level->state() == LevelState::active && gate.open_for() == PlayerTeam::none,
            "The gate trial did not reach play with its gate shut");

        int open_steps = 0, changes = 0, held_for_rider = 0, passes = 0;
        std::array<int, 2> opened{};
        // A bot is stuck if, alive throughout, it stays inside one body box for
        // this long; bots hold still to finish painting for far less.
        constexpr int stuck_window = 900;
        std::array<Vector2F, 4> window_low{}, window_high{};
        std::array<int, 4> window_steps{}, longest_still{};
        std::array<bool, 4> inside{};
        constexpr int trial_steps = 7200;
        for (int tick = 0; tick < trial_steps && level->state() == LevelState::active; ++tick)
        {
            // What the gate settles from: the paint held, and where players
            // stand, before any of this step's movement or paint.
            const PlayerTeam leader = gate.paint_leader();
            const PlayerTeam before = gate.open_for();
            bool rider = false;
            for (int seat = 0; seat < level->player_count(); ++seat)
            {
                const Player& player = level->player(seat);
                rider = rider || (before != PlayerTeam::none && player.team() == before &&
                    player.state() == PlayerState::alive && overlaps(player.rectangle(), bounds));
            }
            level->update(physical, dt);
            const PlayerTeam after = gate.open_for();
            const PlayerTeam expected = leader == before || rider ? before : leader;
            require(after == expected, "The gate did not follow the paint held before the step");
            require(gate.held_open() == (leader != before && rider),
                "The gate hold cue disagreed with the actual pre-step occupied-hold reason");
            held_for_rider += leader != before && rider ? 1 : 0;
            changes += after != before ? 1 : 0;
            open_steps += after != PlayerTeam::none ? 1 : 0;
            if (after != PlayerTeam::none) { ++opened[after == PlayerTeam::a ? 0 : 1]; }
            for (int seat = 0; seat < level->player_count(); ++seat)
            {
                const Player& player = level->player(seat);
                const RectangleF& body = player.rectangle();
                require(std::isfinite(body.x) && std::isfinite(body.y), "A gate trial player reached an invalid position");
                const bool alive = player.state() == PlayerState::alive;
                // Nobody the gate is shut to is ever left inside it.
                require(!alive || player.team() == after || !overlaps(body, bounds, 0.5f),
                    "A player was left inside a gate shut to their team");
                const std::size_t index = static_cast<std::size_t>(seat);
                const bool now_inside = alive && overlaps(body, bounds);
                passes += seat > 0 && now_inside && !inside[index] ? 1 : 0;
                inside[index] = now_inside;
                if (seat == 0) { continue; }
                const Vector2F centre = player.center();
                if (!alive || window_steps[index] == 0)
                {
                    window_low[index] = window_high[index] = centre;
                    window_steps[index] = alive ? 1 : 0;
                    continue;
                }
                window_low[index] = Vector2F(std::min(window_low[index].x, centre.x), std::min(window_low[index].y, centre.y));
                window_high[index] = Vector2F(std::max(window_high[index].x, centre.x), std::max(window_high[index].y, centre.y));
                if (window_high[index].x - window_low[index].x > body.width ||
                    window_high[index].y - window_low[index].y > body.height)
                {
                    window_low[index] = window_high[index] = centre;
                    window_steps[index] = 1;
                    continue;
                }
                ++window_steps[index];
                longest_still[index] = std::max(longest_still[index], window_steps[index]);
                require(window_steps[index] < stuck_window, "A bot stuck in one place on the gate trial");
            }
        }
        std::cout << "MEASURE team gate trial seed=" << variant.seed << " steps=" << trial_steps <<
            " open_steps=" << open_steps << " open_a=" << opened[0] << " open_b=" << opened[1] <<
            " changes=" << changes << " held_for_player_inside=" << held_for_rider <<
            " bot_entries=" << passes << " longest_still=" << std::max({ longest_still[1], longest_still[2],
                longest_still[3] }) << '\n';
        totals.open_steps += open_steps;
        totals.bot_passes += passes;
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "usage: team_gate <content>");
        check_contract();
        World world(std::filesystem::absolute(argv[1]), "team-gate",
            "Team gate trial needs authored support '", "turbulence_gate_trial");
        check_object(world);
        check_permission_geometry(world);
        check_walks(world);
        check_settled_hold_reason(world);
        const Roster rosters[] = {
            { 41731, { WeaponType::sprayer, WeaponType::roller, WeaponType::bucket, WeaponType::mister },
                BotDifficulty::hard },
            { 9127, { WeaponType::sprayer, WeaponType::sprayer, WeaponType::roller, WeaponType::sniper },
                BotDifficulty::normal },
            { 30011, { WeaponType::sprayer, WeaponType::mister, WeaponType::sprayer, WeaponType::bucket },
                BotDifficulty::easy } };
        Totals totals;
        for (const Roster& variant : rosters) { check_stage(world, variant, totals); }
        require(totals.open_steps > 0, "No bot painted the gate open in any trial");
        std::cout << "PASS team gate trial: the gate loads shut; every step of three whole 3-bot matches follows "
            "the paint held before it, never shuts on a player inside, and no bot sticks; passing is a measurement\n";
        return 0;
    }
    catch (const std::exception& error)
    { std::cerr << "FAIL team gate: " << error.what() << '\n'; return 1; }
}
