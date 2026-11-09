#include "game/pch.h"
#include "scripts/acceptance/paint_surfaces/fixtures.h"
#include "game/ai/match_bot.h"
#include "game/ai/match_bot_ballistics.h"
#include "game/contracts/match_variant.h"
#include "game/contracts/simulation.h"
#include "game/input/input_bindings.h"
#include "game/objects/projectile_ball.h"
#include "game/objects/projectile_grenade.h"
#include "game/objects/projectile_sprinkler.h"
#include "game/objects/projectile_wall.h"
#include "game/objects/sub_weapon.h"
#include "game/objects/weapon_builder.h"
#include "game/states/loadout_choice.h"
#include "scripts/acceptance/support/json_fixture.h"
#include "scripts/acceptance/support/probe.h"
#include <cmath>
#include <iostream>

// ROAD#11, slices 1 to 5: the paint grenade end to end. Its tuning and what
// the loader refuses; the shared tank's sub cost and Endless paint; the
// grenade's throw, cooldown and burst; every built player carrying one; the
// Sub action's default controls and their fallbacks; a real round in which a
// thrown grenade alone paints the stage; the shipping bots' throws, by
// policy at each difficulty and tool, and in real bot rounds; the
// sprinkler: its tuning, a seat that carries it, and in a real round where
// it sticks, how long it sprays, what it touches and what breaks it; and
// the paint wall: where a player sets it down, and in a real round that it
// stands on the floor, stops enemy shots and not its own team's, paints
// beneath itself, breaks and expires; and the loadout: Weapon Select's
// tool-then-sub steps, Random, the sub line's copy, and the HUD's readiness
// mark.

namespace
{
    using namespace labrador;
    using acceptance::require;
    constexpr float dt = simulation::seconds_per_step;

    bool close_to(float actual, float expected, float tolerance = 0.0001f)
    {
        return std::fabs(actual - expected) <= tolerance;
    }

    // A projectile's velocity is its own business; one step of flight shows
    // it, give or take that step's gravity and drag: 2500 * dt is under 42
    // at 60 steps a second, so `slack` covers a ball's first step.
    Vector2F flown(Projectile& projectile)
    {
        const Vector2F before = projectile.bounds().center();
        projectile.update(dt);
        return (projectile.bounds().center() - before) / dt;
    }
    constexpr float slack = 50.0f;

    MatchSetup setup(MatchVariant variant, bool bot = true)
    {
        MatchSetup settings;
        settings.set_variant(variant);
        settings.set_player_count(2);
        if (bot) { settings.set_player_bot(1, BotDifficulty::normal); }
        settings.begin_input_claims(0);
        if (!bot) { require(settings.claim_input_slot(1), "Cannot claim the second seat"); }
        settings.set_screen_layout(layout_for_humans(settings.human_count()));
        settings.set_stage("crossfeed");
        settings.set_duration(2.0f);
        settings.set_player_team(0, PlayerTeam::a);
        settings.set_player_team(1, PlayerTeam::b);
        settings.set_player_weapon(0, WeaponType::sprayer);
        settings.set_player_weapon(1, WeaponType::roller);
        return settings;
    }

    class World final
    {
    public:
        explicit World(const std::filesystem::path& content) : assets(content)
        {
            auto info = std::make_unique<LevelLoadedInfo>(
                path_to_utf8(content / "levels/crossfeed.json").c_str());
            info->validate();
            levels.add("crossfeed", std::move(info));
        }

        std::vector<std::unique_ptr<Player>> players(const MatchSetup& settings)
        {
            viewports.set_layout(settings.screen_layout());
            PlayerBuilder builder(&assets.tuning, &assets.presentation);
            return builder.build_players(settings, levels.get(settings.stage()),
                TeamColourTools::team_colours(TeamColourPair::blue_orange, assets.presentation.team_palette()),
                &assets.render, &assets.audio, &assets.preferences, &viewports);
        }

        std::unique_ptr<Level> level(const MatchSetup& settings)
        {
            viewports.set_layout(settings.screen_layout());
            LevelBuilder builder(&viewports, &assets.render, &assets.audio, &assets.preferences,
                &levels, &assets.tuning, &assets.presentation, &resolution, nullptr, &partitioner);
            return builder.build_level(settings, TeamColourPair::blue_orange, OpeningMode::off,
                MatchRandom(1108));
        }

        std::unique_ptr<Weapon> tank()
        {
            return WeaponBuilder::build_weapon(&assets.tuning, PlayerTeam::a, 0, 0, Colour::white,
                WeaponType::sprayer, Vector2F::zero, &assets.render, &assets.audio, &assets.preferences);
        }

        std::unique_ptr<SubWeapon> grenade()
        {
            return build_sub_weapon(&assets.tuning, SubType::grenade, PlayerTeam::a, 0,
                Colour::white, &assets.render, &assets.audio);
        }

        paint_surfaces_acceptance::World assets;
        Registry<LevelLoadedInfo> levels{ "sub-weapons" };
        ResolutionManager resolution;
        ViewportManager viewports{ &resolution };
        Partitioner partitioner;
    };

    template<class Action> bool refused(Action action)
    {
        try { action(); }
        catch (const std::runtime_error&) { return true; }
        return false;
    }

    void check_tuning(const World& world, const std::filesystem::path& content)
    {
        const GameTuning& tuning = world.assets.tuning;
        require(sub_name(SubType::grenade) == std::string("grenade") &&
            sub_from_name("grenade") == SubType::grenade && !sub_from_name("grenade_std"),
            "The grenade is spelt once, as 'grenade'");
        const SubTuning& grenade = tuning.sub(SubType::grenade);
        require(grenade.cost > 0.0f && grenade.cost <= tuning.starting_ammo() &&
            grenade.cooldown > 0.0f && grenade.burst_count >= 1 && grenade.burst_count <= 32 &&
            grenade.body.max_velocity == tuning.projectile(ProjectileType::ball).max_velocity &&
            grenade.burst.details.player_damage > 0.0f,
            "The shipped grenade has a payable cost, a cooldown, a burst and the shared speed cap");

        acceptance::JsonVariants variants(content / "tuning.json", L"sub-weapons");
        const auto loads = [&](auto edit)
        {
            const auto path = variants.write(edit);
            return !refused([&] { const GameTuning edited(path_to_utf8(path).c_str()); });
        };
        require(loads([](rapidjson::Document&) {}), "An unedited copy of the tuning did not load");
        // A cost past a full tank loads, as a shot's does: the sub reads as
        // empty rather than refusing a fixture that shrinks the tank.
        require(loads([&](rapidjson::Document& document)
            { document["subs"]["grenade"]["cost"].SetFloat(tuning.starting_ammo() * 1.5f); }),
            "A sub costing more than a full tank was refused");
        require(!loads([](rapidjson::Document& document)
            { document["subs"]["grenade"]["cost"].SetFloat(0.0f); }), "A free sub loaded");
        for (const int count : { 0, 33 })
        {
            require(!loads([count](rapidjson::Document& document)
                { document["subs"]["grenade"]["burst_count"].SetInt(count); }),
                "A burst outside 1 to 32 drops loaded");
        }
        require(!loads([](rapidjson::Document& document)
            {
                document["subs"]["grenade"].AddMember("fuse", 1.0f, document.GetAllocator());
            }), "A misspelt sub key loaded");
        require(!loads([](rapidjson::Document& document) { document.RemoveMember("subs"); }),
            "Tuning without its subs loaded");
        const SubTuning& sprinkler = tuning.sub(SubType::sprinkler);
        require(sub_from_name("sprinkler") == SubType::sprinkler && sprinkler.cost > 0.0f &&
            sprinkler.interval >= dt && sprinkler.lifetime >= sprinkler.interval && sprinkler.hits >= 1 &&
            grenade.interval == 0.0f && grenade.hits == 0,
            "The shipped sprinkler has a spray schedule and a hit count, and the grenade none");
        require(!loads([](rapidjson::Document& document)
            { document["subs"]["grenade"].AddMember("hits", 3, document.GetAllocator()); }),
            "A grenade took the sprinkler's keys");
        for (const char* key : { "interval", "lifetime", "hits" })
        {
            require(!loads([key](rapidjson::Document& document)
                { document["subs"]["sprinkler"].RemoveMember(key); }), "A sprinkler without its schedule loaded");
        }
        require(!loads([](rapidjson::Document& document)
            { document["subs"]["sprinkler"]["interval"].SetFloat(0.001f); }), "A spray faster than a step loaded");
        require(!loads([](rapidjson::Document& document)
            { document["subs"]["sprinkler"]["lifetime"].SetFloat(0.1f); }), "A sprinkler that never sprays loaded");
        require(!loads([](rapidjson::Document& document)
            { document["subs"]["sprinkler"]["hits"].SetInt(0); }), "A sprinkler nothing can break loaded");
        const SubTuning& wall = tuning.sub(SubType::wall);
        require(sub_from_name("wall") == SubType::wall && wall.reach > 0.0f && wall.hits >= 1 &&
            wall.lifetime >= wall.interval && wall.body.details.col_rect_size.y > wall.body.details.col_rect_size.x &&
            sprinkler.reach == 0.0f, "The shipped wall is upright, with a reach, a schedule and a hit count");
        require(!loads([](rapidjson::Document& document)
            { document["subs"]["sprinkler"].AddMember("reach", 100.0f, document.GetAllocator()); }),
            "A sprinkler took the wall's reach");
        require(!loads([](rapidjson::Document& document) { document["subs"]["wall"].RemoveMember("reach"); }),
            "A wall without its reach loaded");
        require(!loads([](rapidjson::Document& document) { document["subs"]["wall"].RemoveMember("hits"); }),
            "A wall without its hits loaded");
        std::cout << "PASS tuning: shipped grenade, sprinkler and wall, cost bounds, burst bounds, the planted subs' schedules, the wall's reach and unknown keys\n";
    }

    void check_tank(World& world)
    {
        const float full = world.assets.tuning.starting_ammo();
        const float cost = world.assets.tuning.sub(SubType::grenade).cost;
        auto tank = world.tank();
        int paid = 0;
        while (tank->can_pay(cost) && paid < 100)
        {
            require(tank->spend_paint(cost), "A tank that could pay refused");
            ++paid;
        }
        require(paid == static_cast<int>(std::floor(full / cost + 0.0001f)) &&
            !tank->spend_paint(cost) && close_to(tank->ammo(), full - static_cast<float>(paid) * cost),
            "The tank paid for more or fewer subs than it holds");

        // The refill waits out the ordinary delay after a sub, as after a shot.
        auto refill = world.tank();
        require(refill->spend_paint(cost), "A full tank refused a sub");
        const float after = refill->ammo();
        PlayerInputData idle;
        idle.connected = true;
        const int delay_steps = static_cast<int>(world.assets.tuning.ammo_regen_delay() / dt) - 1;
        for (int step = 0; step < delay_steps; ++step)
        {
            refill->update_and_get_projectiles(idle, Vector2F::zero, Vector2F::zero, true, dt);
        }
        require(refill->ammo() == after, "The tank refilled inside the delay after a sub");
        for (int step = 0; step < 120; ++step)
        {
            refill->update_and_get_projectiles(idle, Vector2F::zero, Vector2F::zero, true, dt);
        }
        require(refill->ammo() > after, "The tank never refilled after a sub");

        auto endless = world.tank();
        endless->set_endless_paint(true);
        for (int use = 0; use < 10; ++use)
        {
            require(endless->can_pay(cost) && endless->spend_paint(cost), "Endless paint refused a sub");
        }
        require(endless->ammo() == full, "Endless paint spent the tank on subs");
        std::cout << "PASS tank: whole cost only, the refill delay restarts, Endless paint spends nothing\n";
    }

    void check_throw(World& world)
    {
        const SubTuning& tuning = world.assets.tuning.sub(SubType::grenade);
        auto tank = world.tank();
        auto sub = world.grenade();
        const Vector2F origin(100.0f, 200.0f);
        const Vector2F up_right = Vector2F(1.0f, -1.0f).normalized();
        const Vector2F running(300.0f, -500.0f);

        require(sub->type() == SubType::grenade && sub->ready(*tank), "A new grenade was not ready");
        require(sub->update(true, *tank, origin, up_right, running, 0.0f).empty() &&
            tank->ammo() == world.assets.tuning.starting_ammo(), "A zero step threw");
        require(sub->update(false, *tank, origin, up_right, running, dt).empty(),
            "The grenade left without a press");

        auto thrown = sub->update(true, *tank, origin, up_right, running, dt);
        require(thrown.size() == 1, "A press threw other than one grenade");
        auto* grenade = dynamic_cast<ProjectileGrenade*>(thrown.front().get());
        require(grenade != nullptr && grenade->projectile_type() == ProjectileType::ball &&
            grenade->team() == PlayerTeam::a && grenade->player_num() == 0,
            "The throw was not this player's paint grenade");
        require(close_to(grenade->bounds().center().x, origin.x) && close_to(grenade->bounds().center().y, origin.y),
            "The grenade did not leave the thrower's centre");
        const Vector2F expected = up_right * tuning.throw_speed + Vector2F(running.x, 0.0f);
        const Vector2F measured = flown(*grenade);
        require(close_to(measured.x, expected.x, slack) && close_to(measured.y, expected.y, slack),
            "The grenade did not leave along the aim with the thrower's run");
        require(close_to(tank->ammo(), world.assets.tuning.starting_ammo() - tuning.cost),
            "The throw did not take its cost from the tank");
        require(close_to(sub->cooldown_remaining(), tuning.cooldown) && !sub->ready(*tank),
            "The throw did not start the cooldown");

        // Inside the cooldown a press throws nothing and costs nothing.
        const float before = tank->ammo();
        require(sub->update(true, *tank, origin, up_right, running, dt).empty() && tank->ammo() == before,
            "A press inside the cooldown threw");
        sub->reset();
        require(sub->cooldown_remaining() == 0.0f, "Reset kept the cooldown");

        // A press the tank cannot pay for throws nothing and leaves it alone.
        while (tank->can_pay(tuning.cost)) { tank->spend_paint(tuning.cost); }
        const float short_of = tank->ammo();
        require(sub->update(true, *tank, origin, up_right, running, dt).empty() &&
            tank->ammo() == short_of && sub->cooldown_remaining() == 0.0f,
            "A tank short of the cost threw or was charged");

        // The grenade bursts once, and only once it has retired.
        require(grenade->take_burst().empty() && !grenade->burst(), "The grenade burst in flight");
        grenade->set_for_deletion(true);
        const Vector2F centre = grenade->bounds().center();
        // Spoke 0 is straight up; the rest follow evenly round the circle.
        auto drops = grenade->take_burst();
        require(static_cast<int>(drops.size()) == tuning.burst_count && grenade->burst() &&
            grenade->take_burst().empty(), "The grenade did not burst into its drops exactly once");
        for (size_t index = 0; index < drops.size(); ++index)
        {
            const Projectile& drop = *drops[index];
            const float angle = -pi / 2.0f + 2.0f * pi * static_cast<float>(index) /
                static_cast<float>(drops.size());
            const Vector2F velocity = Vector2F::unit_vec_from_angle(angle) * tuning.burst_speed;
            require(dynamic_cast<const ProjectileBall*>(&drop) != nullptr &&
                drop.team() == PlayerTeam::a && drop.player_num() == 0 &&
                drop.player_damage() == tuning.burst.details.player_damage &&
                close_to(drop.bounds().center().x, centre.x) && close_to(drop.bounds().center().y, centre.y),
                "A drop did not leave the grenade's centre with the burst's damage");
            const Vector2F measured_drop = flown(*drops[index]);
            require(close_to(measured_drop.x, velocity.x, slack) && close_to(measured_drop.y, velocity.y, slack),
                "A drop did not fly out along its own spoke at the burst's speed");
        }
        require(grenade->player_damage() == tuning.body.details.player_damage,
            "The grenade's body does not carry its own damage");
        std::cout << "PASS throw: aim and run, cost, cooldown, short tank, one even burst\n";
    }

    PlayerInputData throwing(const Vector2F& aim)
    {
        PlayerInputData input;
        input.connected = true;
        input.sub_pressed = true;
        input.shoot_direction_requested = true;
        input.shoot_direction = aim;
        input.shoot_angle = aim.angle();
        return input;
    }

    void check_players(World& world)
    {
        const float full = world.assets.tuning.starting_ammo();
        const SubTuning& tuning = world.assets.tuning.sub(SubType::grenade);
        auto players = world.players(setup(MatchVariant::standard));
        for (const auto& player : players)
        {
            require(player->sub_weapon() != nullptr && player->sub_weapon()->type() == SubType::grenade,
                "A human or bot was built without the grenade");
        }
        Player& thrower = *players[0];
        Player& enemy = *players[1];

        thrower.set_player_input(throwing(Vector2F::direction_up));
        auto first = thrower.update_weapon_and_get_projectiles(dt, {});
        require(first.size() == 1 && dynamic_cast<ProjectileGrenade*>(first.front().get()) != nullptr &&
            flown(*first.front()).y < 0.0f && close_to(thrower.weapon_ammo(), full - tuning.cost),
            "A player's Sub press did not throw one grenade along the aim for its cost");
        require(thrower.update_weapon_and_get_projectiles(dt, {}).empty(),
            "A held press threw again inside the cooldown");

        // The roller, whose aim is a fixed downward tilt, throws forward and up.
        enemy.set_player_input(throwing(Vector2F::direction_down));
        auto lobbed = enemy.update_weapon_and_get_projectiles(dt, {});
        require(lobbed.size() == 1 && flown(*lobbed.front()).y < 0.0f,
            "The roller threw its grenade at the floor");

        // A drop's hit takes the burst's damage, not the bucket's.
        auto grenade = std::move(first.front());
        grenade->set_for_deletion(true);
        auto drops = grenade->take_burst();
        const float health = enemy.health();
        enemy.on_projectile_contact(*drops.front(), Vector2F::zero, 0.0f);
        require(close_to(health - enemy.health(), tuning.burst.details.player_damage),
            "A drop's hit did not take the burst's damage");

        // An eliminated player throws nothing; a respawn clears the cooldown.
        thrower.kill();
        thrower.set_player_input(throwing(Vector2F::direction_up));
        require(thrower.update_weapon_and_get_projectiles(dt, {}).empty(), "An eliminated player threw");
        thrower.force_respawn();
        require(thrower.update_weapon_and_get_projectiles(dt, {}).size() == 1,
            "A respawned player kept the cooldown or lost the tank");

        // Endless paint: the tank stays full and the cooldown paces the throws.
        auto endless = world.players(setup(MatchVariant::endless_paint));
        endless[0]->set_player_input(throwing(Vector2F::direction_up));
        const float seconds = 10.0f;
        int throws = 0;
        for (int step = 0; step < static_cast<int>(seconds / dt); ++step)
        {
            for (const auto& released : endless[0]->update_weapon_and_get_projectiles(dt, {}))
            {
                throws += dynamic_cast<const ProjectileGrenade*>(released.get()) != nullptr ? 1 : 0;
            }
            require(endless[0]->weapon_ammo() == full, "Endless paint spent the tank on a grenade");
        }
        const int paced = static_cast<int>(std::floor(seconds / tuning.cooldown)) + 1;
        require(throws >= paced - 1 && throws <= paced, "Endless paint did not pace grenades by the cooldown");
        std::cout << "PASS players: every seat carries one, aim, roller lob, burst damage, elimination, Endless paint\n";
    }

    void check_controls()
    {
        const auto pad_sub = [](const ActionBindings& bindings)
        { return bindings.game_pad[static_cast<size_t>(GameAction::sub)].buttons; };
        const auto mouse_sub = [](const ActionBindings& bindings)
        { return bindings.game_desktop[static_cast<size_t>(GameAction::sub)].mouse_buttons; };

        const ActionBindings& defaults = default_action_bindings();
        require(pad_sub(defaults) == std::vector<GamepadButton>{ GamepadButton::right_shoulder } &&
            mouse_sub(defaults) == std::vector<MouseButton>{ MouseButton::right } &&
            defaults.game_desktop[static_cast<size_t>(GameAction::sub)].keys.empty(),
            "The Sub action's defaults are not RB and the right mouse button");

        // Sub is a choice like the others (slice 5): one slot's profile moves
        // only that slot, a key works as well as a button, and no control
        // serves Sub and another action at once.
        InputBindings lb_sub;
        lb_sub.pads[1][PadAction::sub] = { PadControl::left_shoulder, PadControl::none };
        require(pad_sub(effective_action_bindings(lb_sub, 1)) == std::vector<GamepadButton>{ GamepadButton::left_shoulder } &&
            pad_sub(effective_action_bindings(lb_sub, 0)) == std::vector<GamepadButton>{ GamepadButton::right_shoulder },
            "A profile that chose LB for Sub did not move only its own Sub");
        InputBindings shared;
        shared.pads[2][PadAction::jump] = { PadControl::right_shoulder, PadControl::none };
        bool refused = false;
        try { shared.validate(); } catch (const std::invalid_argument&) { refused = true; }
        InputBindings alternate;
        alternate.pads[0][PadAction::sub] = { PadControl::right_shoulder, PadControl::left_shoulder };
        bool alternate_refused = false;
        try { alternate.validate(); } catch (const std::invalid_argument&) { alternate_refused = true; }
        require(refused && alternate_refused, "Sub shared RB with Jump, or took a second pad control");

        InputBindings key_sub;
        key_sub.keyboard_sub = { Key::q, MouseButton::none };
        const ActionBindings keyed = effective_action_bindings(key_sub);
        require(mouse_sub(keyed).empty() &&
            keyed.game_desktop[static_cast<size_t>(GameAction::sub)].keys == std::vector<Key>{ Key::q },
            "The keyboard Sub key did not replace the right mouse button");
        InputBindings right_shoot;
        right_shoot.keyboard_shoot = { Key::none, MouseButton::right };
        require(keyboard_control_conflict(right_shoot, KeyboardAction::shoot, right_shoot.keyboard_shoot) ==
            KeyboardAction::sub, "Shoot on the right mouse button did not conflict with Sub");
        std::cout << "PASS controls: RB and right mouse by default, rebindable per pad and on the keyboard, never shared\n";
    }

    void check_round(World& world)
    {
        const MatchSetup settings = setup(MatchVariant::standard, false);
        auto level = world.level(settings);
        level->start_without_countdown();
        std::vector<PlayerInputData> devices(input_slots::count);
        devices[0].connected = true;
        devices[1].connected = true;
        const int burst = world.assets.tuning.sub(SubType::grenade).burst_count;
        int most = 0;
        for (int step = 0; step < 90; ++step)
        {
            // One press, aimed down at the floor; never the tool.
            devices[0] = step == 0 ? throwing(Vector2F::direction_down) : PlayerInputData{};
            devices[0].connected = true;
            level->update(devices, dt);
            most = std::max(most, level->projectile_count());
        }
        require(most >= burst, "The thrown grenade never burst into its drops in the round");
        const LevelEndInfo result = level->level_end_info();
        require(result.team_a_area > 0.0f && result.team_b_area == 0.0f,
            "A grenade alone did not paint the stage for its team");
        std::cout << "PASS round: one thrown grenade bursts and paints without the tool\n";
    }

    // The bot policy's fixture: flat open floor, one enemy straight ahead.
    MatchBotObservation bot_view(const GameTuning& tuning, WeaponType tool, Vector2F enemy)
    {
        MatchBotObservation view;
        view.alive = view.grounded = true;
        view.size = tuning.player().size;
        view.ammo = 1.0f;
        view.engage_distance = 800.0f;
        view.weapon = tool;
        view.sub = SubType::grenade;
        view.sub_ready = true;
        view.obstacles.emplace_back(-3000.0f, view.size.y * 0.5f, 6000.0f, 100.0f);
        view.supports.push_back({ view.obstacles.back(), false });
        view.enemies.push_back({ enemy, {}, 800.0f });
        return view;
    }

    // The steps a bot presses Sub over a run, ready throughout until it throws.
    struct BotRun
    {
        std::vector<int> presses;
        PlayerInputData press;
        PlayerInputData after;
    };

    BotRun run_bot(MatchBot& bot, MatchBotObservation view, int steps)
    {
        BotRun run;
        bool thrown = false;
        for (int step = 0; step < steps; ++step)
        {
            const PlayerInputData input = bot.think(view);
            require(input.connected && std::isfinite(input.shoot_angle), "A bot emitted invalid input");
            if (thrown && run.presses.size() == 1 && step == run.presses.front() + 1) { run.after = input; }
            if (input.sub_pressed)
            {
                run.presses.push_back(step);
                run.press = input;
                thrown = true;
                // As the player would: the throw starts the cooldown.
                view.sub_ready = false;
            }
        }
        return run;
    }

    void check_bot_policy(const GameTuning& tuning)
    {
        const SubTuning& grenade = tuning.sub(SubType::grenade);
        const Vector2F near_enemy(320.0f, 0.0f);
        int throwers = 0;
        for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
        {
            const int window = tuning.bot(difficulty).reaction_steps * 3 + 2;
            const float error = tuning.bot(difficulty).aim_error_radians;
            for (const WeaponType tool : { WeaponType::sprayer, WeaponType::sniper, WeaponType::roller,
                WeaponType::mister, WeaponType::bucket })
            {
                const MatchBotObservation view = bot_view(tuning, tool, near_enemy);
                std::optional<float> fixed;
                if (tool == WeaponType::roller) { fixed = -tuning.weapon(WeaponType::roller).aim_tilt; }
                const std::optional<float> expected = match_bot_ballistics::throw_angle(grenade, {},
                    near_enemy, {}, fixed, tuning.bots().sub_throw_miss);
                MatchBot bot(&tuning, difficulty, 31);
                const BotRun run = run_bot(bot, view, window);
                require(run.presses.size() == (expected ? 1u : 0u),
                    "A bot's throws at a reachable enemy did not match the ballistics");
                if (!expected) { continue; }
                ++throwers;
                const Vector2F aim = Vector2F::unit_vec_from_angle(*expected);
                require(Vector2F::dot(run.press.shoot_direction, aim) >= std::cos(error) - 0.001f &&
                    run.press.shoot_direction_requested && !run.press.primary_shoot,
                    "A bot's throw strayed past its difficulty's aim error or fired the tool with it");
                require(!run.after.sub_pressed, "A bot held Sub past one step");
                // Not ready, no sub, or an enemy out of reach: never a press.
                MatchBotObservation idle = view; idle.sub_ready = false;
                MatchBot waiting(&tuning, difficulty, 31);
                require(run_bot(waiting, idle, window).presses.empty(), "A bot pressed Sub while not ready");
                MatchBotObservation bare = view; bare.sub.reset();
                MatchBot unarmed(&tuning, difficulty, 31);
                require(run_bot(unarmed, bare, window).presses.empty(), "A bot without a sub pressed Sub");
                const MatchBotObservation remote = bot_view(tuning, tool, { 1500.0f, 0.0f });
                MatchBot distant(&tuning, difficulty, 31);
                require(run_bot(distant, remote, window).presses.empty(), "A bot threw at an enemy out of reach");
                // A throw that would leave the reserve at its low mark waits,
                // unless Endless paint means it spends nothing.
                MatchBotObservation short_tank = view;
                short_tank.ammo = tuning.bots().reserve_low + grenade.cost / tuning.starting_ammo() - 0.01f;
                MatchBot careful(&tuning, difficulty, 31);
                require(run_bot(careful, short_tank, window).presses.empty(),
                    "A bot threw its reserve below the low mark");
                short_tank.refill_enabled = false;
                MatchBot endless(&tuning, difficulty, 31);
                require(run_bot(endless, short_tank, window).presses.size() == 1,
                    "Endless paint did not let a bot throw on a short tank");
            }
        }
        require(throwers >= 12, "Too few tools could throw at a nearby enemy");
        std::cout << "PASS bot policy: " << throwers << " difficulty/tool pairs throw once, on aim, "
            "never unready, unarmed, out of reach or below the reserve; Endless paint throws\n";
    }

    // The sprinkler and the wall each have their own rule (slice 6): the
    // sprinkler is planted on unowned scoring floor ahead while no enemy is
    // engaged, the wall set down between the bot and a visible enemy whose
    // tool reaches it, within the wall's height.
    void check_bot_planted_subs(const GameTuning& tuning)
    {
        const SubTuning& sprinkler = tuning.sub(SubType::sprinkler);
        const SubTuning& wall = tuning.sub(SubType::wall);
        int planters = 0;
        for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
        {
            const int window = tuning.bot(difficulty).reaction_steps * 3 + 2;
            const float error = tuning.bot(difficulty).aim_error_radians;
            for (const WeaponType tool : { WeaponType::sprayer, WeaponType::sniper, WeaponType::roller,
                WeaponType::mister, WeaponType::bucket })
            {
                // Sprinkler: open paintable floor, nobody in sight.
                MatchBotObservation open = bot_view(tuning, tool, {});
                open.enemies.clear();
                open.sub = SubType::sprinkler;
                open.supports.back().paintable = true;
                MatchBot planter(&tuning, difficulty, 31);
                const BotRun planted = run_bot(planter, open, window);
                require(planted.presses.size() <= 1, "A bot planted its sprinkler twice on one cooldown");
                if (!planted.presses.empty())
                {
                    ++planters;
                    // A throw at floor level a few strides out on either side.
                    bool on_aim = false;
                    for (const float side : { 1.0f, -1.0f })
                    {
                        std::optional<float> fixed;
                        if (tool == WeaponType::roller)
                        {
                            const float tilt = tuning.weapon(WeaponType::roller).aim_tilt;
                            fixed = side > 0.0f ? -tilt : pi + tilt;
                        }
                        const std::optional<float> expected = match_bot_ballistics::throw_angle(sprinkler, {},
                            { side * open.size.x * 3.0f, open.size.y * 0.5f }, {}, fixed, tuning.bots().sub_throw_miss);
                        on_aim = on_aim || (expected && Vector2F::dot(planted.press.shoot_direction,
                            Vector2F::unit_vec_from_angle(*expected)) >= std::cos(error) - 0.001f);
                    }
                    require(on_aim && !planted.press.primary_shoot,
                        "A bot's sprinkler missed the floor ahead or fired the tool with it");
                }
                // Never on its own paint, on floor that scores nothing, or in a fight.
                MatchBotObservation owned = open; owned.footing = MatchBotFooting::own;
                MatchBot proud(&tuning, difficulty, 31);
                require(run_bot(proud, owned, window).presses.empty(), "A bot planted a sprinkler on its own paint");
                MatchBotObservation bare = open; bare.supports.back().paintable = false;
                MatchBot wasteful(&tuning, difficulty, 31);
                require(run_bot(wasteful, bare, window).presses.empty(), "A bot planted a sprinkler on unscored floor");
                MatchBotObservation fight = bot_view(tuning, tool, { 320.0f, 0.0f });
                fight.sub = SubType::sprinkler;
                fight.supports.back().paintable = true;
                MatchBot fighter(&tuning, difficulty, 31);
                require(run_bot(fighter, fight, window).presses.empty(), "A bot planted a sprinkler mid-fight");

                // Wall: an enemy on the bot's floor, clear of where it lands.
                const float clear = wall.reach + wall.body.details.col_rect_size.x * 0.5f + open.size.x * 0.5f;
                for (const float side : { 1.0f, -1.0f })
                {
                    MatchBotObservation duel = bot_view(tuning, tool, { side * (clear + 120.0f), 0.0f });
                    duel.sub = SubType::wall;
                    MatchBot guard(&tuning, difficulty, 31);
                    const BotRun walled = run_bot(guard, duel, window);
                    // The roller sets it down on the side it faces, which is right here.
                    const size_t expected = tool == WeaponType::roller && side < 0.0f ? 0u : 1u;
                    require(walled.presses.size() == expected, "A bot did not set its wall down against a duel");
                    if (expected == 1u)
                    {
                        require(walled.press.shoot_direction.x * side > 0.0f && !walled.press.primary_shoot,
                            "A bot set its wall down on the wrong side or fired the tool with it");
                    }
                }
                const auto never = [&](MatchBotObservation view, const char* message)
                {
                    view.sub = SubType::wall;
                    MatchBot bot(&tuning, difficulty, 31);
                    require(run_bot(bot, view, window).presses.empty(), message);
                };
                never(bot_view(tuning, tool, { clear - 10.0f, 0.0f }), "A bot walled an enemy inside the wall's reach");
                never(bot_view(tuning, tool, { clear + 120.0f, -wall.body.details.col_rect_size.y }),
                    "A bot walled an enemy above the wall's top");
                never(bot_view(tuning, tool, { clear + 120.0f, open.size.y }), "A bot walled an enemy below its floor");
                MatchBotObservation harmless = bot_view(tuning, tool, { clear + 120.0f, 0.0f });
                harmless.enemies.back().engage_distance = clear;
                never(harmless, "A bot walled an enemy whose tool cannot reach it");
                MatchBotObservation alone = open; alone.supports.back().paintable = false;
                never(alone, "A bot set a wall down with nobody to stop");
            }
        }
        require(planters >= 12, "Too few tools planted a sprinkler on open floor");
        std::cout << "PASS bot planted subs: " << planters << " difficulty/tool pairs plant a sprinkler ahead, never "
            "on own paint, unscored floor or mid-fight; walls go down between duels, never too close, outside the "
            "wall's height, against a harmless or absent enemy\n";
    }

    // A roller and a sprayer bot on crossfeed for a minute, under either
    // refill rule, carrying each sub in turn.
    void check_bot_round(World& world)
    {
        for (const SubType carried : { SubType::grenade, SubType::sprinkler, SubType::wall })
        {
        int used = 0;
        for (const MatchVariant variant : { MatchVariant::standard, MatchVariant::endless_paint })
        {
            // The host stays human and idle; a sprayer bot joins its team.
            MatchSetup settings = setup(variant);
            settings.set_player_count(3);
            settings.set_player_bot(2, BotDifficulty::hard);
            settings.set_player_team(2, PlayerTeam::a);
            settings.set_player_weapon(2, WeaponType::sprayer);
            settings.set_player_sub(1, carried);
            settings.set_player_sub(2, carried);
            settings.set_screen_layout(layout_for_humans(settings.human_count()));
            settings.set_duration(120.0f);
            auto level = world.level(settings);
            level->start_without_countdown();
            const std::vector<PlayerInputData> devices(input_slots::count);
            std::array<int, 2> throws{};
            std::array<float, 2> cooldown{};
            for (int step = 0; step < 60 * simulation::simulation_hz; ++step)
            {
                level->update(devices, dt);
                for (size_t index = 0; index < throws.size(); ++index)
                {
                    const SubWeapon* sub = level->player(static_cast<int>(index) + 1).sub_weapon();
                    require(sub != nullptr && sub->type() == carried, "A bot seat did not carry its sub");
                    const float now = sub->cooldown_remaining();
                    if (now > cooldown[index]) { ++throws[index]; }
                    cooldown[index] = now;
                }
            }
            // Where the two bots meet is the stage's business; that they use
            // each sub when they should, at each tool, is the policy checks'.
            // Here: a real round's bots use it, never faster than the cooldown.
            const int most = static_cast<int>(60.0f / world.assets.tuning.sub(carried).cooldown) + 1;
            used += throws[0] + throws[1];
            require(throws[0] <= most && throws[1] <= most, "A bot used its sub faster than the cooldown");
            std::cout << "PASS bot round " << sub_name(carried) << " " << match_variant_key(variant) <<
                ": roller used " << throws[0] << ", sprayer used " << throws[1] << " in a minute\n";
        }
        require(used > 0, std::string("No bot used its ") + sub_name(carried) + " in either round");
        }
    }
}

    // The sprinkler in a real round: dropped at the idle host's feet, it
    // sticks to the floor, touches nothing, sprays on its schedule for its
    // lifetime and retires; enemy shots break it, its own team's do not.
    void check_sprinkler(World& world)
    {
        const SubTuning& tuning = world.assets.tuning.sub(SubType::sprinkler);
        MatchSetup settings = setup(MatchVariant::standard, false);
        settings.set_player_sub(0, SubType::sprinkler);
        settings.set_duration(120.0f);
        require(settings.player_sub(0) == SubType::sprinkler && settings.player_sub(1) == SubType::grenade &&
            [&] { try { settings.set_player_sub(1, SubType::count); } catch (const std::invalid_argument&) { return true; }
                return false; }(),
            "A seat's sub is not its own choice, or took a sub that is not one");
        auto players = world.players(settings);
        require(players[0]->sub_weapon()->type() == SubType::sprinkler &&
            players[1]->sub_weapon()->type() == SubType::grenade, "A seat did not carry the sub it was given");

        auto level = world.level(settings);
        level->start_without_countdown();
        const std::vector<PlayerInputData> idle(input_slots::count);
        const Vector2F feet = level->player(0).center();
        const auto strip = [&](const ProjectileTuning& record)
        { return resolve_projectile_strip(record, world.assets.render); };
        const auto drop_sprinkler = [&](Vector2F at, PlayerTeam team)
        {
            return dynamic_cast<ProjectileSprinkler*>(level->admit_projectile(std::make_unique<ProjectileSprinkler>(
                RectangleF(at, tuning.body.details.col_rect_size), Vector2F(0.0f, 300.0f),
                team, 1, Colour::white, &world.assets.render, strip(tuning.body), strip(tuning.burst), &tuning)));
        };
        const auto step = [&] { level->update(idle, dt); };

        ProjectileSprinkler* sprinkler = drop_sprinkler(feet, PlayerTeam::a);
        int steps = 0;
        while (!sprinkler->planted() && steps < 120) { step(); ++steps; }
        require(sprinkler->planted() && sprinkler->mask() == 0 && sprinkler->player_damage() == 0.0f &&
            sprinkler->takes_hits(), "A dropped sprinkler did not stick to the floor, or still touches players");
        const Vector2F planted_at = sprinkler->bounds().center();
        const float before = level->level_end_info().team_a_area;
        // Live until just before its lifetime ends, staying put and spraying
        // once at once and then once each interval.
        const int life = static_cast<int>(std::floor(tuning.lifetime / dt)) - 2;
        for (int index = 0; index < life; ++index) { step(); }
        const int expected = static_cast<int>(std::floor(static_cast<float>(life + 1) * dt / tuning.interval)) + 1;
        require(sprinkler->bounds().center() == planted_at && std::abs(sprinkler->sprays() - expected) <= 1,
            "A planted sprinkler moved or did not spray on its schedule");
        require(level->level_end_info().team_a_area > before, "A spraying sprinkler painted nothing more");
        // Past its lifetime and its last drops' flight, nothing is left.
        for (int index = 0; index < static_cast<int>((tuning.interval + tuning.burst.details.delete_timer) / dt) + 10; ++index)
        { step(); }
        require(level->projectile_count() == 0, "A sprinkler outlived its lifetime");

        // Shots reaching it: its own team's leave it be, each enemy one is
        // spent on it, and the last of its hits breaks it.
        // Beside the host, clear of both players, on the same floor.
        sprinkler = drop_sprinkler(feet + Vector2F(160.0f, 0.0f), PlayerTeam::a);
        for (steps = 0; !sprinkler->planted() && steps < 120; ++steps) { step(); }
        require(sprinkler->planted() && std::fabs(sprinkler->bounds().center().y - planted_at.y) < 1.0f,
            "A sprinkler dropped beside the host did not stick to its floor");
        // A small ball (the spray drop's record), so it can sit on the
        // sprinkler's top without reaching the floor under it.
        const ProjectileTuning& shot = tuning.burst;
        const auto shoot_at = [&](PlayerTeam team)
        {
            // Overlapping its top, clear of the floor under it.
            const RectangleF target = sprinkler->bounds();
            level->admit_projectile(std::make_unique<ProjectileBall>(
                RectangleF(Vector2F(target.center().x, target.top() - 4.0f), shot.details.col_rect_size),
                Vector2F::zero, team, 1, Colour::white, &world.assets.render, strip(shot), &shot));
            step();
        };
        shoot_at(PlayerTeam::a);
        require(sprinkler->hits_left() == tuning.hits, "Its own team's shot broke a sprinkler");
        for (int hit = 1; hit < tuning.hits; ++hit)
        {
            shoot_at(PlayerTeam::b);
            require(sprinkler->hits_left() == tuning.hits - hit && sprinkler->takes_hits(),
                "An enemy shot that reached a sprinkler was not counted against it");
        }
        const int sprays = sprinkler->sprays();
        shoot_at(PlayerTeam::b);
        // The last hit retires it this tick; the scene drops it at the next.
        for (int index = 0; index < static_cast<int>(tuning.burst.details.delete_timer / dt) + 10; ++index) { step(); }
        require(level->projectile_count() == 0 && sprays < static_cast<int>(tuning.lifetime / tuning.interval),
            "Its last hit did not break the sprinkler");
        std::cout << "PASS sprinkler: a seat carries it; it sticks, sprays " << expected <<
            " times over its life and retires; " << tuning.hits << " enemy shots break it, its own team's do not\n";
    }

    // The paint wall: a player sets it down beside them, foot level with
    // theirs; in a real round it stands on the floor, spends each enemy shot
    // that reaches it and none of its own team's, drips paint beneath
    // itself, and goes on its last hit or at the end of its lifetime.
    void check_wall(World& world)
    {
        const GameTuning& game = world.assets.tuning;
        const SubTuning& tuning = game.sub(SubType::wall);
        MatchSetup settings = setup(MatchVariant::standard, false);
        settings.set_player_sub(0, SubType::wall);
        settings.set_duration(120.0f);
        auto players = world.players(settings);
        Player& builder = *players[0];
        require(builder.sub_weapon()->type() == SubType::wall, "A seat did not carry the wall it was given");
        const float full = game.starting_ammo();
        for (const float side : { 1.0f, -1.0f })
        {
            builder.force_respawn();
            builder.set_player_input(throwing(Vector2F(side, -0.2f).normalized()));
            auto placed = builder.update_weapon_and_get_projectiles(dt, {});
            require(placed.size() == 1 && dynamic_cast<ProjectileWall*>(placed.front().get()) != nullptr,
                "A Sub press with the wall did not set one down");
            const RectangleF wall = placed.front()->bounds();
            const float feet = builder.center().y + game.player().size.y / 2.0f;
            require(close_to(wall.center().x, builder.center().x + side * tuning.reach, 0.5f) &&
                close_to(wall.bottom(), feet, 2.0f) && flown(*placed.front()).x == 0.0f,
                "A wall was not set down beside the player, foot level with theirs, dropping straight");
            require(close_to(builder.weapon_ammo(), full - tuning.cost), "A wall did not take its cost");
        }

        auto level = world.level(settings);
        level->start_without_countdown();
        const std::vector<PlayerInputData> idle(input_slots::count);
        const auto step = [&] { level->update(idle, dt); };
        const auto strip = [&](const ProjectileTuning& record)
        { return resolve_projectile_strip(record, world.assets.render); };
        const Vector2F host = level->player(0).center();
        const float floor = host.y + game.player().size.y / 2.0f;
        const auto stand_wall = [&]
        {
            // Beside team A's idle host, clear of both players.
            const Vector2F size = tuning.body.details.col_rect_size;
            auto* wall = dynamic_cast<ProjectileWall*>(level->admit_projectile(std::make_unique<ProjectileWall>(
                RectangleF(Vector2F(host.x + 160.0f, floor - size.y / 2.0f - 1.0f), size),
                Vector2F(0.0f, tuning.throw_speed), PlayerTeam::a, 0, Colour::white, &world.assets.render,
                strip(tuning.body), strip(tuning.burst), &tuning)));
            for (int count = 0; !wall->planted() && count < 60; ++count) { step(); }
            require(wall->planted() && close_to(wall->bounds().bottom(), floor, 2.0f) && wall->mask() == 0,
                "A wall did not stand on the floor it was set down above");
            return wall;
        };
        const float before = level->level_end_info().team_a_area;
        ProjectileWall* wall = stand_wall();
        // A shot of the burst record (small, so it clears the floor) at the
        // wall's middle from one side, moving across it.
        const ProjectileTuning& shot = tuning.burst;
        const auto shoot = [&](PlayerTeam team)
        {
            const RectangleF target = wall->bounds();
            level->admit_projectile(std::make_unique<ProjectileBall>(
                RectangleF(Vector2F(target.left() - 40.0f, target.center().y), shot.details.col_rect_size),
                Vector2F(1500.0f, 0.0f), team, 1, Colour::white, &world.assets.render, strip(shot), &shot));
            for (int count = 0; count < 4; ++count) { step(); }
        };
        shoot(PlayerTeam::a);
        require(wall->hits_left() == tuning.hits, "Its own team's shot was stopped by a wall");
        shoot(PlayerTeam::b);
        require(wall->hits_left() == tuning.hits - 1, "An enemy shot passed a standing wall");
        for (int count = 0; count < static_cast<int>(tuning.interval / dt) * 2; ++count) { step(); }
        require(wall->sprays() >= 2 && level->level_end_info().team_a_area > before,
            "A standing wall did not paint beneath itself");
        // It goes at the end of its lifetime, with its last drips.
        for (int count = 0; count < static_cast<int>((tuning.lifetime + shot.details.delete_timer) / dt) + 10; ++count)
        { step(); }
        require(level->projectile_count() == 0, "A wall outlived its lifetime");

        // Its last hit breaks it well before then.
        wall = stand_wall();
        for (int hit = 0; hit < tuning.hits; ++hit)
        {
            require(wall->takes_hits(), "A wall broke before its last hit");
            shoot(PlayerTeam::b);
        }
        for (int count = 0; count < static_cast<int>(shot.details.delete_timer / dt) + 10; ++count) { step(); }
        require(level->projectile_count() == 0, "Its last hit did not break the wall");
        std::cout << "PASS wall: set down " << tuning.reach << " to either side at the player's feet; it stands, "
            "stops enemy shots and not its own team's, paints beneath, goes after its lifetime and on its "
            << tuning.hits << "th hit\n";
    }

namespace
{
    // Weapon Select's row (LoadoutChoice), the sub line's copy, and what a
    // pane's HUD shows of the sub (Level::sub_hud_cue).
    void check_loadout(World& world)
    {
        using Step = LoadoutChoice::Step;
        LoadoutChoice row(WeaponType::roller, SubType::sprinkler);
        require(row.step() == Step::tool && row.tool() == WeaponType::roller &&
            row.sub_choice() == static_cast<size_t>(SubType::sprinkler), "A row starts on its tool, keeping the sub it came with");
        require(!row.back() && row.step() == Step::tool, "Back on the tool has nothing to undo");
        require(row.cycle(Direction::right) && row.tool() == WeaponType::mister &&
            row.sub_choice() == static_cast<size_t>(SubType::sprinkler), "Left/Right on the tool changes only the tool");
        require(!row.cycle(Direction::up) && row.tool() == WeaponType::mister, "Up/Down is P1's row move, not a choice");
        require(row.proceed() && row.step() == Step::sub, "A moves from the tool to the sub");
        require(row.cycle(Direction::right) && row.sub_choice() == static_cast<size_t>(SubType::wall) &&
            row.tool() == WeaponType::mister, "Left/Right on the sub changes only the sub");
        require(row.cycle(Direction::right) && row.sub_choice() == LoadoutChoice::random_sub &&
            row.cycle(Direction::right) && row.sub_choice() == static_cast<size_t>(SubType::grenade) &&
            row.cycle(Direction::left) && row.sub_choice() == LoadoutChoice::random_sub,
            "The subs wrap through Random");
        require(row.proceed() && row.step() == Step::ready && !row.proceed() && !row.cycle(Direction::left) &&
            row.sub_choice() == LoadoutChoice::random_sub, "A readies the row, which then holds its choices");
        require(row.back() && row.step() == Step::sub && row.back() && row.step() == Step::tool,
            "Back steps from ready to the sub and from the sub to the tool");
        for (unsigned int draw = 0; draw < 6; ++draw)
        {
            require(row.resolved_sub(draw) == static_cast<SubType>(draw % sub_count), "Random draws each sub");
        }
        require(LoadoutChoice(WeaponType::random_primary, SubType::wall).resolved_sub(5) == SubType::wall,
            "A concrete sub ignores the draw");
        bool refused = false;
        try { static_cast<void>(LoadoutChoice(WeaponType::sprayer, SubType::count)); }
        catch (const std::invalid_argument&) { refused = true; }
        require(refused, "A row refuses an unresolved sub");

        // Every choice has a name and a description, and its line in both
        // steps names it after the Boost label.
        const MenuText& text = world.assets.presentation.menu_text();
        const SubMenuText& copy = text.subs;
        for (size_t choice = 0; choice < SubMenuText::choice_count; ++choice)
        {
            const std::wstring boost = text.handicap_label(2);
            const std::wstring line = copy.label(choice, false, boost);
            const std::wstring choosing = copy.label(choice, true, boost);
            require(!copy.names[choice].empty() && !copy.descriptions[choice].empty() &&
                line.find(boost) == 0 && choosing.find(boost) == 0 &&
                line.find(copy.names[choice]) != std::wstring::npos &&
                choosing.find(copy.names[choice]) != std::wstring::npos && line != choosing,
                "Each sub's line and choosing line follow the Boost label, name it and differ");
        }

        // The HUD: ready with a full tank, not once spent, the tick at its cost.
        const GameTuning& tuning = world.assets.tuning;
        MatchSetup settings = setup(MatchVariant::standard, false);
        settings.set_duration(120.0f);
        settings.set_player_sub(1, SubType::wall);
        auto level = world.level(settings);
        level->start_without_countdown();
        const Player& player = level->player(0);
        SubHudCue cue = level->sub_hud_cue(player);
        require(cue.shown && cue.ready &&
            close_to(cue.cost, tuning.sub(SubType::grenade).cost / tuning.starting_ammo()),
            "A full tank shows the sub ready, with the tick at its cost");
        std::vector<PlayerInputData> inputs(input_slots::count);
        inputs[0] = throwing(Vector2F::direction_up);
        level->update(inputs, dt);
        cue = level->sub_hud_cue(player);
        require(cue.shown && !cue.ready, "A spent sub shows not ready");
        require(level->player(1).sub_weapon()->type() == SubType::wall, "The other seat carries its own sub");

        MatchSetup endless = setup(MatchVariant::endless_paint, false);
        endless.set_duration(120.0f);
        auto endless_level = world.level(endless);
        endless_level->start_without_countdown();
        const SubHudCue free = endless_level->sub_hud_cue(endless_level->player(0));
        require(free.shown && free.ready && free.cost == 0.0f, "Endless paint shows readiness without a cost tick");
        std::cout << "PASS loadout: tool then sub then ready, back steps, Random draws each sub, every sub's copy; "
            "HUD readiness and cost tick, none under Endless paint\n";
    }
}

int wmain(int argc, wchar_t* argv[])
{
    return acceptance::run_probe("sub weapons", [&]
    {
        require(argc == 2, "sub_weapons <content>");
        const std::filesystem::path content = std::filesystem::absolute(argv[1]);
        World world(content);
        check_tuning(world, content);
        check_tank(world);
        check_throw(world);
        check_players(world);
        check_controls();
        check_round(world);
        check_bot_policy(world.assets.tuning);
        check_bot_planted_subs(world.assets.tuning);
        check_bot_round(world);
        check_sprinkler(world);
        check_wall(world);
        check_loadout(world);
        return 0;
    });
}
