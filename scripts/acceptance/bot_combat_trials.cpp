#include "game/pch.h"
#include "scripts/acceptance/bot_navigation/fixtures.h"
#include "game/debug/dev_tools.h"
#include "scripts/acceptance/support/probe.h"
#if COLOURWARS_DEV_TOOLS
#include "game/debug/match_fingerprint.h"
#endif
#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace
{
    using namespace labrador;
    using namespace mattmath;
    using namespace bot_navigation_acceptance;

    constexpr int trial_steps = 1200;
    enum class Mode { moving_human, opposed_bots };
    struct Trial
    {
        Mode mode;
        PlayerTeam team;
        WeaponType tool;
        BotDifficulty difficulty;
    };

    const char* name(Mode mode) { return mode == Mode::moving_human ? "moving_human" : "opposed_bots"; }
    const char* name(WeaponType tool)
    {
        constexpr const char* names[] = { "sprayer", "sniper", "roller", "mister", "bucket" };
        return names[static_cast<int>(tool)];
    }
    const char* name(BotDifficulty difficulty)
    {
        constexpr const char* names[] = { "easy", "normal", "hard" };
        return names[static_cast<int>(difficulty)];
    }

    MatchSetup roster(const Trial& trial)
    {
        MatchSetup settings;
        settings.set_player_count(trial.mode == Mode::moving_human ? 2 : 3);
        settings.set_player_bot(1, trial.difficulty);
        if (trial.mode == Mode::opposed_bots) { settings.set_player_bot(2, trial.difficulty); }
        // A physical pad slot bypasses mouse aim; this still drives ordinary
        // physical input and needs no native device or injected seat decisions.
        settings.begin_input_claims(0);
        settings.set_screen_layout(ScreenLayout::one_player);
        settings.set_stage("crossfeed"); settings.set_duration(60.0f);
        const PlayerTeam other = trial.team == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a;
        settings.set_player_team(0, other); settings.set_player_team(1, trial.team);
        if (trial.mode == Mode::opposed_bots) { settings.set_player_team(2, other); }
        for (int seat = 0; seat < settings.player_count(); ++seat) { settings.set_player_weapon(seat, trial.tool); }
        return settings;
    }

    void prepare(Level& level, const World& world, const Trial& trial)
    {
        const std::vector<PlayerInputData> physical = physical_inputs();
        for (int tick = 0; tick < 900 && level.state() != LevelState::active; ++tick) { level.update(physical, dt); }
        require(level.state() == LevelState::active, "Combat trial did not finish ordinary countdown");
        const Vector2F size = world.assets.tuning.player().size;
        const bool left = trial.team == PlayerTeam::a;
        const float bot_x = left ? 300.0f : 3600.0f - 300.0f - size.x;
        const float opponent_x = left ? 1000.0f : 3600.0f - 1000.0f - size.x;
        const int opponent = trial.mode == Mode::moving_human ? 0 : 2;
        require(level.place_player(1, { bot_x, 1280.0f - size.y }) &&
            level.place_player(opponent, { opponent_x, 1280.0f - size.y }), "Combat trial initial placement failed");
        require(level.bot_line_of_sight(level.player(1).center(), level.player(opponent).center()),
            "Combat trial initial opponents are not visible");
    }

    struct HumanMotion
    {
        float direction = -1.0f;
        bool jump_was_held = false;
        std::vector<PlayerInputData> inputs(const Level& level, const Trial& trial, int tick)
        {
            std::vector<PlayerInputData> physical = physical_inputs();
            if (trial.mode != Mode::moving_human) { return physical; }
            const Player& human = level.player(0);
            // Mirror the same bounded shuttle on the right floor. After a
            // normal death/respawn, ordinary movement returns toward this band.
            const bool left = trial.team == PlayerTeam::a;
            const float local_x = left ? human.center().x : 3600.0f - human.center().x;
            if (local_x <= 850.0f) { direction = 1.0f; }
            if (local_x >= 1200.0f) { direction = -1.0f; }
            PlayerInputData& input = physical[0];
            input.x_movement = direction * (left ? 1.0f : -1.0f);
            input.left_analog_stick = { input.x_movement, 0.0f };
            // Vertical motion exercises leading rather than only pursuit of
            // a same-height target. Normal ceilings may shorten these jumps.
            const bool held = tick % 180 < 12;
            input.jump_held = held;
            input.jump_pressed = held && !jump_was_held;
            jump_was_held = held;
            return physical;
        }
    };

    struct ActorMetrics
    {
        Vector2F previous;
        float min_x = 0.0f, max_x = 0.0f, min_y = 0.0f, max_y = 0.0f;
        float path = 0.0f, damage_taken = 0.0f, health_before = 0.0f, min_ammo = 1.0f;
        float moving_damage = 0.0f, airborne_damage = 0.0f;
        float final_paint = 0.0f;
        int first_damage = -1;
        int firing_steps = 0, fire_requests = 0, movement_steps = 0, jumps = 0;
        int low_steps = 0, empty_steps = 0, recoveries = 0, deaths = 0;
        int neutral_paints = 0, enemy_repaints = 0;
        bool was_low = false;
        bool was_moving = false, was_airborne = false;
        PlayerState previous_state = PlayerState::alive;
        std::uint64_t kills_by_opponent = 0, kills_by_other = 0, uncredited_deaths = 0;
    };

    struct Metrics { std::vector<ActorMetrics> actors; };

    void observe_actor(ActorMetrics& m, const Player& player, const PlayerInputData& input, const GameTuning& tuning, int tick)
    {
        const Vector2F center = player.center();
        require(std::isfinite(center.x) && std::isfinite(center.y) && std::isfinite(player.health()) &&
            std::isfinite(player.weapon_ammo()) && player.weapon_ammo() >= 0.0f,
            "Combat trial produced invalid actor/reserve state");
        m.min_x = std::min(m.min_x, center.x); m.max_x = std::max(m.max_x, center.x);
        m.min_y = std::min(m.min_y, center.y); m.max_y = std::max(m.max_y, center.y);
        if (player.state() == PlayerState::alive && m.previous_state == PlayerState::alive)
        { m.path += Vector2F::distance(m.previous, center); }
        if (player.health() < m.health_before)
        {
            const float damage = m.health_before - player.health();
            m.damage_taken += damage;
            if (m.was_moving) { m.moving_damage += damage; }
            if (m.was_airborne) { m.airborne_damage += damage; }
            if (m.first_damage < 0) { m.first_damage = tick; }
        }
        m.deaths += m.previous_state == PlayerState::alive && player.state() == PlayerState::dead ? 1 : 0;
        m.movement_steps += input.x_movement != 0.0f ? 1 : 0; m.jumps += input.jump_pressed ? 1 : 0;
        m.fire_requests += input.primary_shoot ? 1 : 0;
        m.firing_steps += player.primary_weapon().shooting_this_update() ? 1 : 0;
        const float ammo = player.weapon_ammo() / tuning.starting_ammo();
        m.min_ammo = std::min(m.min_ammo, ammo);
        const bool empty = player.primary_weapon().readiness() == WeaponReadiness::empty;
        const bool low = ammo <= tuning.bots().reserve_low;
        m.empty_steps += empty ? 1 : 0; m.low_steps += low ? 1 : 0;
        if (empty || low) { m.was_low = true; }
        if (m.was_low && ammo >= tuning.bots().reserve_resume) { ++m.recoveries; m.was_low = false; }
        m.previous = center; m.previous_state = player.state(); m.health_before = player.health();
        m.was_moving = player.state() == PlayerState::alive && std::fabs(player.velocity().x) > 1.0f;
        m.was_airborne = player.state() == PlayerState::alive && !grounded(player);
    }

    Metrics run(World& world, const Trial& trial, bool replay_check)
    {
        const MatchSetup settings = roster(trial);
        std::unique_ptr<Level> level = world.build(settings); prepare(*level, world, trial);
        Metrics result; result.actors.resize(static_cast<std::size_t>(level->player_count()));
        for (int seat = 0; seat < level->player_count(); ++seat)
        {
            ActorMetrics& m = result.actors[static_cast<std::size_t>(seat)];
            const Player& player = level->player(seat);
            m.previous = player.center(); m.min_x = m.max_x = m.previous.x; m.min_y = m.max_y = m.previous.y;
            m.health_before = player.health(); m.previous_state = player.state();
            m.was_moving = std::fabs(player.velocity().x) > 1.0f; m.was_airborne = !grounded(player);
        }
        std::vector<PaintSurfaceSample> previous_paint = level->paint_surfaces();
        Tape tape("Combat playback exhausted its finite final-input tape");
        HumanMotion motion;
        for (int tick = 0; tick < trial_steps; ++tick)
        {
            const std::vector<PlayerInputData> physical = motion.inputs(*level, trial, tick);
            level->update(physical, dt, &tape);
            require(level->state() == LevelState::active && tape.steps.size() == static_cast<std::size_t>(tick + 1),
                "Combat trial escaped its finite active-step budget");
            const std::vector<PlayerInputData>& inputs = tape.steps.back();
            require(inputs.size() == result.actors.size(), "Combat trial lost a roster participant");
            for (int seat = 0; seat < level->player_count(); ++seat)
            {
                const PlayerInputData& input = inputs[static_cast<std::size_t>(seat)];
                require(input.connected && !input.toggle_debug,
                    "Combat trial emitted a menu/pause/device action");
                require(level->player(seat).primary_weapon().type() == trial.tool,
                    "Combat trial changed a participant's tool");
                if (seat > 0) { require(level->player(seat).player_num() == -1, "Combat bot claimed a physical slot"); }
                observe_actor(result.actors[static_cast<std::size_t>(seat)], level->player(seat), input, world.assets.tuning, tick);
            }
            require(inputs[0].x_movement == physical[0].x_movement && inputs[0].jump_pressed == physical[0].jump_pressed &&
                !inputs[0].primary_shoot, "Combat driver replaced the human's ordinary physical input");
            const std::vector<PaintSurfaceSample> paint = level->paint_surfaces();
            require(paint.size() == previous_paint.size(), "Static combat arena changed its scoring tile count");
            for (std::size_t tile = 0; tile < paint.size(); ++tile)
            {
                const PaintSurfaceSample& now = paint[tile]; const PaintSurfaceSample& before = previous_paint[tile];
                require(now.source_index == before.source_index && now.face == before.face && now.corners == before.corners,
                    "Static combat scoring tile order changed during measurement");
                if (now.team == before.team || now.team == PlayerTeam::none) { continue; }
                for (int seat = 1; seat < level->player_count(); ++seat)
                {
                    if (now.paint_owner != paint_owners::index(-1, seat)) { continue; }
                    ActorMetrics& m = result.actors[static_cast<std::size_t>(seat)];
                    if (before.team == PlayerTeam::none) { ++m.neutral_paints; } else { ++m.enemy_repaints; }
                }
            }
            previous_paint = paint;
        }
        const int opponent = trial.mode == Mode::moving_human ? 0 : 2;
        for (int seat = 0; seat < level->player_count(); ++seat)
        {
            ActorMetrics& m = result.actors[static_cast<std::size_t>(seat)];
            const PlayerLethalProjectiles& lethal = level->player(seat).lethal_projectiles();
            const int enemy = seat == 1 ? opponent : 1;
            const int opponent_owner = paint_owners::index(level->player(enemy).player_num(), enemy);
            m.kills_by_opponent = lethal.enemy_by_owner.at(static_cast<std::size_t>(opponent_owner));
            m.uncredited_deaths = lethal.uncredited;
            for (std::size_t owner = 0; owner < lethal.enemy_by_owner.size(); ++owner)
            { if (owner != static_cast<std::size_t>(opponent_owner)) { m.kills_by_other += lethal.enemy_by_owner[owner]; } }
            const int owner = paint_owners::index(level->player(seat).player_num(), seat);
            for (const PaintSurfaceSample& surface : previous_paint)
            { if (surface.paint_owner == owner) { m.final_paint += surface.scored_area; } }
        }
        if (trial.mode == Mode::moving_human)
        {
            require(result.actors[0].max_x - result.actors[0].min_x >= 100.0f &&
                result.actors[0].max_y - result.actors[0].min_y >= 10.0f && result.actors[0].jumps > 0,
                "Moving-opponent trial did not produce actual horizontal and vertical human motion");
        }
        if (replay_check)
        {
            const std::vector<std::uint64_t> expected = snapshot(*level);
#if COLOURWARS_DEV_TOOLS
            const MatchFingerprint expected_full = MatchFingerprint::of(*level, trial_steps);
#endif
            level.reset(); level = world.build(settings); prepare(*level, world, trial);
            tape.playback = true;
            const std::vector<PlayerInputData> physical = physical_inputs();
            for (int tick = 0; tick < trial_steps; ++tick) { level->update(physical, dt, &tape); }
            require(tape.next == trial_steps && snapshot(*level) == expected,
                "Combat final-input replay changed players, projectile count or exact paint state");
#if COLOURWARS_DEV_TOOLS
            require(MatchFingerprint::difference(expected_full, MatchFingerprint::of(*level, trial_steps)).empty(),
                "Combat final-input replay changed complete Debug fingerprint");
#endif
        }
        return result;
    }

    void write(std::ostream& csv, const Trial& trial, const Metrics& result)
    {
        for (std::size_t seat = 0; seat < result.actors.size(); ++seat)
        {
            const ActorMetrics& m = result.actors[seat];
            csv << name(trial.mode) << ",crossfeed," << name(trial.tool) << ',' << (trial.team == PlayerTeam::a ? 'a' : 'b') << ',' <<
                name(trial.difficulty) << ',' << trial_seed << ',' << trial_steps << ',' << seat << ',' <<
                m.max_x - m.min_x << ',' << m.max_y - m.min_y << ',' << m.path << ',' << m.movement_steps << ',' <<
                m.jumps << ',' << m.fire_requests << ',' << m.firing_steps << ',' << m.min_ammo << ',' << m.empty_steps << ',' <<
                m.low_steps << ',' << m.recoveries << ',' << m.damage_taken << ',' << m.first_damage << ',' <<
                m.moving_damage << ',' << m.airborne_damage << ',' << m.deaths << ',' <<
                m.kills_by_opponent << ',' << m.kills_by_other << ',' << m.uncredited_deaths << ',' <<
                m.final_paint << ',' << m.neutral_paints << ',' << m.enemy_repaints << '\n';
        }
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("bot combat trials", [&]
    {
        require(argc == 2, "usage: bot_combat_trials <content>");
        World world(std::filesystem::absolute(argv[1]), "bot-combat-trials", "Combat trial needs support '", "crossfeed");
        const std::filesystem::path report = std::filesystem::absolute(argv[0]).parent_path() / "bot_combat_trials.csv";
        std::ofstream csv(report); require(csv.good(), "Could not open combat-trial CSV");
        csv << std::fixed << std::setprecision(3);
        csv << "mode,stage,tool,bot_team,difficulty,seed,steps,seat,x_extent,y_extent,path,movement_steps,jumps,";
        csv << "fire_requests,firing_steps,min_ammo,empty_steps,low_steps,reserve_recoveries,damage_taken,first_damage_step,";
        csv << "damage_while_moving,damage_while_airborne,deaths,";
        csv << "lethal_by_opponent,lethal_by_other,lethal_uncredited,final_owned_paint,neutral_paint_events,enemy_repaint_events\n";
        int trials = 0, replays = 0;
        for (const Mode mode : { Mode::moving_human, Mode::opposed_bots })
        {
            int damaged = 0, painted = 0, contested = 0, moving_damage = 0;
            int qualified_bots = 0;
            for (int tool = 0; tool < 5; ++tool)
            {
                for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
                {
                    for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
                    {
                        const Trial trial{ mode, team, static_cast<WeaponType>(tool), difficulty };
                        const bool replay = team == PlayerTeam::a && static_cast<int>(difficulty) == tool % 3 && tool < 3;
                        const Metrics metrics = run(world, trial, replay); write(csv, trial, metrics); csv.flush();
                        ++trials; replays += replay ? 1 : 0;
                        const int opponent = mode == Mode::moving_human ? 0 : 2;
                        damaged += metrics.actors[static_cast<std::size_t>(opponent)].damage_taken > 0.0f ? 1 : 0;
                        moving_damage += metrics.actors[static_cast<std::size_t>(opponent)].moving_damage > 0.0f ? 1 : 0;
                        painted += metrics.actors[1].final_paint > 0.0f ? 1 : 0;
                        contested += metrics.actors[1].enemy_repaints > 0 ? 1 : 0;
                        for (std::size_t seat = 1; seat < metrics.actors.size(); ++seat)
                        {
                            const ActorMetrics& bot = metrics.actors[seat];
                            qualified_bots += bot.firing_steps > 0 && bot.path > 100.0f && bot.final_paint > 0.0f ? 1 : 0;
                        }
                        // Opposed play can produce a clean win without retaliation.
                        // Require a real exchange outcome, not damage in both directions.
                        require(metrics.actors[static_cast<std::size_t>(opponent)].damage_taken > 0.0f ||
                            (mode == Mode::opposed_bots && metrics.actors[1].damage_taken > 0.0f),
                            "Combat trial had no actual opponent health loss within its finite budget");
                    }
                }
            }
            std::cout << "MEASURE " << name(mode) << ": opponent health loss=" << damaged <<
                "/30; moving health loss=" << moving_damage << "/30; P2 retained paint=" << painted <<
                "/30; P2 enemy repaint=" << contested << "/30\n";
            require(qualified_bots == (mode == Mode::moving_human ? 30 : 60),
                "Combat bot did not move, pay for fire and retain paint within the finite budget");
            if (mode == Mode::moving_human)
            { require(moving_damage == 30, "Moving opponent did not lose health while already moving in every trial"); }
        }
        require(trials == 60 && replays == 6 && csv.good(), "Combat trial finite matrix/report contract failed");
        std::cout << "PASS 60 bounded moving/opposed combat trials and six 1200-step final-input replays; report=" <<
            path_to_utf8(report) << "; ordinary fire/motion/damage/owned-paint gates; only lethal contacts have owner attribution; repaint counts remain measurements\n";
        return 0;
    });
}
