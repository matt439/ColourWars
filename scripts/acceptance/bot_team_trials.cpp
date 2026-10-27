#include "game/pch.h"
#include "scripts/acceptance/bot_navigation/fixtures.h"
#include "scripts/acceptance/balance_regions/exchange.h"
#include "game/debug/dev_tools.h"
#if COLOURWARS_DEV_TOOLS
#include "game/debug/match_fingerprint.h"
#endif
#include <array>
#include <bit>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>

namespace
{
    using namespace labrador;
    using namespace mattmath;
    using namespace bot_navigation_acceptance;

    constexpr int trial_steps = 1800;
    constexpr int sustained_steps = 7200;
    constexpr int seats = 4;
    enum class Matrix { same_tool, mixed, sustained };
    // A same-tool trial gives every seat one tool; a mixed trial rotates the
    // three bots through all five. A sustained trial is one bot a side, with a
    // neutral human on each team. Seed is the whole-match srand value.
    struct Trial
    {
        const char* stage;
        std::array<WeaponType, seats> tools;
        PlayerTeam human_team;
        unsigned int seed;
        Matrix kind;
        int rotation;
        WeaponType tool(int seat) const { return this->tools[static_cast<std::size_t>(seat)]; }
        bool mixed() const { return this->kind != Matrix::same_tool; }
        bool bot(int seat) const { return this->kind == Matrix::sustained ? seat == 1 || seat == 2 : seat > 0; }
        int steps() const { return this->kind == Matrix::sustained ? sustained_steps : trial_steps; }
    };
    constexpr std::array<unsigned int, 3> mixed_seeds{ trial_seed, 52807u, 61403u };

    const char* name(WeaponType tool)
    {
        constexpr const char* names[] = { "sprayer", "sniper", "roller", "mister", "bucket" };
        return names[static_cast<int>(tool)];
    }
    char name(PlayerTeam team) { return team == PlayerTeam::a ? 'a' : 'b'; }
    const char* matrix(const Trial& trial)
    {
        constexpr const char* names[] = { "same_tool", "mixed", "sustained" };
        return names[static_cast<int>(trial.kind)];
    }

    Trial same_tool(const char* stage, WeaponType tool, PlayerTeam human_team)
    { return { stage, { tool, tool, tool, tool }, human_team, trial_seed, Matrix::same_tool, -1 }; }

    // Across five rotations each bot seat holds every tool once, and the lone
    // bot and both opponents always differ. The neutral human never fires.
    Trial mixed(const char* stage, int rotation, PlayerTeam human_team, unsigned int seed)
    {
        Trial trial{ stage, {}, human_team, seed, Matrix::mixed, rotation };
        trial.tools[0] = WeaponType::sprayer;
        constexpr int offset[seats] = { 0, 0, 2, 4 };
        for (int seat = 1; seat < seats; ++seat)
        { trial.tools[static_cast<std::size_t>(seat)] = static_cast<WeaponType>((rotation + offset[seat]) % 5); }
        return trial;
    }

    // One bot a side for four times the mixed horizon. Across the rotations
    // each bot seat holds every tool once and the two bots always differ.
    Trial sustained(const char* stage, int rotation, PlayerTeam human_team, unsigned int seed)
    {
        Trial trial{ stage, {}, human_team, seed, Matrix::sustained, rotation };
        trial.tools = { WeaponType::sprayer, static_cast<WeaponType>(rotation),
            static_cast<WeaponType>((rotation + 2) % 5), WeaponType::sprayer };
        return trial;
    }
    const char* tool_label(const Trial& trial) { return trial.mixed() ? "mixed" : name(trial.tool(1)); }

    MenuLevelSettings roster(const Trial& trial)
    {
        MenuLevelSettings settings;
        settings.set_player_count(seats);
        for (int seat = 1; seat < seats; ++seat) { if (trial.bot(seat)) { settings.set_player_bot(seat, BotDifficulty::normal); } }
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        // The second neutral human claims an ordinary pad slot and never acts.
        if (!trial.bot(3)) { require(settings.claim_input_slot(0), "Sustained trial could not seat its second human"); }
        settings.set_screen_layout(trial.bot(3) ? ScreenLayout::one_player : ScreenLayout::two_player);
        settings.set_stage(trial.stage); settings.set_duration(trial.kind == Matrix::sustained ? 180.0f : 60.0f);
        const PlayerTeam other = trial.human_team == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a;
        for (int seat = 0; seat < seats; ++seat)
        {
            settings.set_player_team(seat, seat < 2 ? trial.human_team : other);
            settings.set_player_weapon(seat, trial.tool(seat));
        }
        return settings;
    }

    void prepare(Level& level)
    {
        const std::vector<PlayerInputData> physical = physical_inputs();
        for (int tick = 0; tick < 900 && level.state() != LevelState::active; ++tick) { level.update(physical, dt); }
        require(level.state() == LevelState::active, "Team trial did not finish the ordinary countdown");
        // Natural spawns only. The neutral human is a roster teammate and an
        // ordinary target; no placement, route hint or live cheat is supplied.
    }

    balance_regions::Partition partition(const World& world, const char* stage)
    {
        const RectangleF arena = world.levels.get(stage)->camera_bounds_rectangle();
        if (std::string_view(stage) == "crossfeed") { return balance_regions::stage_partition(stage, arena); }
        const double left = arena.left(), width = arena.width;
        // Geographic thirds are measurements, not authored combat-role zones.
        return { "geographic_x_thirds", arena.top(), arena.bottom(),
            { { "left_third", left, left + width / 3.0 },
              { "middle_third", left + width / 3.0, left + 2.0 * width / 3.0 },
              { "right_third", left + 2.0 * width / 3.0, arena.right() } } };
    }

    struct ActorMetrics
    {
        Vector2F previous;
        float min_x = 0.0f, max_x = 0.0f, min_y = 0.0f, max_y = 0.0f;
        double path = 0.0, damage_taken = 0.0, final_paint = 0.0, off_home_paint = 0.0, middle_paint = 0.0;
        float health_before = 0.0f, min_ammo = 1.0f;
        int movement_steps = 0, jumps = 0, fire_requests = 0, shooting_flag_steps = 0, paid_fire_steps = 0;
        int low_steps = 0, empty_steps = 0, recoveries = 0, deaths = 0, first_damage = -1;
        int middle_steps = 0, enemy_half_steps = 0, off_home_steps = 0;
        int neutral_paints = 0, enemy_repaints = 0, ally_repaints = 0;
        double neutral_paint_area = 0.0, enemy_repaint_area = 0.0, ally_repaint_area = 0.0;
        bool was_low = false;
        PlayerState previous_state = PlayerState::alive;
        std::uint64_t eliminations = 0, enemy_lethal_taken = 0, uncredited_lethal = 0;
    };
    // One team's middle-band holding, sampled after every active step.
    struct MiddleHold
    {
        int first = -1, held_steps = 0, losses = 0, recaptures = 0, last_third_steps = 0, longest_gap = 0;
        int gap = 0;
        double peak = 0.0, last_third_min = -1.0;
        bool holding = false, lost = false;

        void sample(int tick, int steps, double area, double tolerance)
        {
            const bool now = area > tolerance;
            if (now && this->first < 0) { this->first = tick; }
            if (this->first >= 0)
            {
                this->held_steps += now ? 1 : 0;
                this->gap = now ? 0 : this->gap + 1;
                this->longest_gap = std::max(this->longest_gap, this->gap);
            }
            if (this->holding && !now) { ++this->losses; this->lost = true; }
            if (!this->holding && now && this->lost) { ++this->recaptures; }
            this->holding = now;
            this->peak = std::max(this->peak, area);
            if (tick >= steps - steps / 3)
            {
                this->last_third_steps += now ? 1 : 0;
                this->last_third_min = this->last_third_min < 0.0 ? area : std::min(this->last_third_min, area);
            }
        }
    };
    struct Metrics
    {
        std::array<ActorMetrics, seats> actors;
        std::array<double, 2> team_paint{}, team_off_home{}, team_middle{};
        std::array<double, 2> middle_enemy_capture{}, middle_contested{};
        std::array<MiddleHold, 2> middle_hold;
        // The enemy was still capturing this team's middle tiles in the final third.
        std::array<int, 2> middle_late_loss{};
        int both_middle_steps = 0;
        std::vector<std::uint64_t> final_state;
    };

    void observe(ActorMetrics& m, const Player& player, const PlayerInputData& input,
        const GameTuning& tuning, const balance_regions::Partition& regions, float ammo_before, int tick)
    {
        const Vector2F center = player.center();
        require(std::isfinite(center.x) && std::isfinite(center.y) && std::isfinite(player.health()) &&
            std::isfinite(player.weapon_ammo()) && player.weapon_ammo() >= 0.0f,
            "Team trial produced invalid actor/reserve state");
        m.min_x = std::min(m.min_x, center.x); m.max_x = std::max(m.max_x, center.x);
        m.min_y = std::min(m.min_y, center.y); m.max_y = std::max(m.max_y, center.y);
        if (player.state() == PlayerState::alive && m.previous_state == PlayerState::alive)
        { m.path += Vector2F::distance(m.previous, center); }
        if (player.health() < m.health_before)
        {
            m.damage_taken += m.health_before - player.health();
            if (m.first_damage < 0) { m.first_damage = tick; }
        }
        m.deaths += m.previous_state == PlayerState::alive && player.state() == PlayerState::dead ? 1 : 0;
        m.movement_steps += input.x_movement != 0.0f ? 1 : 0; m.jumps += input.jump_pressed ? 1 : 0;
        m.fire_requests += input.primary_shoot ? 1 : 0;
        m.shooting_flag_steps += player.primary_weapon().shooting_this_update() ? 1 : 0;
        m.paid_fire_steps += player.weapon_ammo() < ammo_before ? 1 : 0;
        const float ammo = player.weapon_ammo() / tuning.starting_ammo();
        m.min_ammo = std::min(m.min_ammo, ammo);
        const bool empty = player.primary_weapon().readiness() == WeaponReadiness::empty;
        const bool low = ammo <= tuning.bots().reserve_low;
        m.empty_steps += empty ? 1 : 0; m.low_steps += low ? 1 : 0;
        if (empty || low) { m.was_low = true; }
        if (m.was_low && ammo >= tuning.bots().reserve_resume) { ++m.recoveries; m.was_low = false; }
        if (player.state() == PlayerState::alive)
        {
            const balance_regions::Band& middle = regions.bands[regions.bands.size() / 2];
            m.middle_steps += center.x >= middle.left && center.x < middle.right ? 1 : 0;
            const double midpoint = (regions.bands.front().left + regions.bands.back().right) * 0.5;
            m.enemy_half_steps += (player.team() == PlayerTeam::a ? center.x > midpoint : center.x < midpoint) ? 1 : 0;
            const balance_regions::Band& home = player.team() == PlayerTeam::a ? regions.bands.front() : regions.bands.back();
            m.off_home_steps += center.x < home.left || center.x >= home.right ? 1 : 0;
        }
        m.previous = center; m.previous_state = player.state(); m.health_before = player.health();
    }

    void write_exchanges(std::ostream& csv, int trial_id, const Trial& trial,
        const balance_regions::Partition& regions, const balance_regions::ExchangeLedger& ledger)
    {
        for (std::size_t region = 0; region < ledger.regions(); ++region)
        {
            const std::string region_name = region == regions.bands.size() ? "outside_partition" : regions.bands[region].name;
            const bool mutual = ledger.team(region, 0).enemy_capture > 0.0 && ledger.team(region, 1).enemy_capture > 0.0;
            const auto prefix = [&](const char* scope, char team, int seat)
            {
                csv << trial_id << ',' << matrix(trial) << ',' << trial.stage << ',' << trial.rotation << ',' <<
                    name(trial.human_team) << ',' << trial.seed << ',' << regions.name << ',' << region_name << ',' <<
                    scope << ',' << team << ',';
                if (seat > 0) { csv << seat << ',' << name(trial.tool(seat - 1)); } else { csv << ','; }
                csv << ',';
            };
            for (std::size_t team = 0; team < 2; ++team)
            {
                const balance_regions::TeamRegionExchange& x = ledger.team(region, team);
                prefix("team", team == 0 ? 'a' : 'b', 0);
                csv << x.initial_held << ',' << x.neutral_claim << ',' << x.enemy_capture << ',' << x.enemy_capture_tiles << ',' <<
                    x.lost_to_enemy << ',' << x.lost_tiles << ',' << x.to_neutral << ',' << x.ally_owner_change << ',' <<
                    x.final_held << ',' << x.contested_held << ',' << x.first_enemy_capture_step << ',' << x.last_lost_step <<
                    ',' << mutual << '\n';
            }
            for (int seat = 1; seat < seats; ++seat)
            {
                if (!trial.bot(seat)) { continue; }
                const balance_regions::SeatRegionExchange& x = ledger.seat(region, static_cast<std::size_t>(seat));
                const PlayerTeam team = seat < 2 ? trial.human_team :
                    (trial.human_team == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a);
                prefix("seat", name(team), seat + 1);
                csv << ',' << x.neutral_claim << ',' << x.enemy_capture << ',' << x.enemy_capture_tiles << ",,,," <<
                    x.ally_owner_change << ",,,,," << mutual << '\n';
            }
        }
    }

    Metrics run(World& world, const Trial& trial, const balance_regions::Partition& regions,
        bool replay_check, int trial_id, std::ostream& regional_csv, std::ostream& exchange_csv)
    {
        const MenuLevelSettings settings = roster(trial);
        std::unique_ptr<Level> level = world.build(settings, trial.seed); prepare(*level);
        Metrics result;
        for (int seat = 0; seat < seats; ++seat)
        {
            ActorMetrics& m = result.actors[static_cast<std::size_t>(seat)];
            const Player& player = level->player(seat);
            m.previous = player.center(); m.min_x = m.max_x = m.previous.x; m.min_y = m.max_y = m.previous.y;
            m.health_before = player.health(); m.previous_state = player.state();
        }
        std::vector<PaintSurfaceSample> previous_paint = level->paint_surfaces();
        std::array<int, seats> owners{};
        for (int seat = 0; seat < seats; ++seat)
        { owners[static_cast<std::size_t>(seat)] = paint_owners::index(level->player(seat).player_num(), seat); }
        balance_regions::ExchangeLedger ledger(previous_paint, regions, owners);
        Tape tape("Team playback exhausted its finite final-input tape");
        const std::vector<PlayerInputData> physical = physical_inputs();
        const int steps = trial.steps();
        const std::size_t middle = regions.bands.size() / 2;
        for (int tick = 0; tick < steps; ++tick)
        {
            std::array<float, seats> ammo_before{};
            for (int seat = 0; seat < seats; ++seat)
            { ammo_before[static_cast<std::size_t>(seat)] = level->player(seat).weapon_ammo(); }
            level->update(physical, dt, &tape);
            require(level->state() == LevelState::active && tape.steps.size() == static_cast<std::size_t>(tick + 1),
                "Team trial escaped its finite active-step budget");
            const std::vector<PlayerInputData>& inputs = tape.steps.back();
            require(inputs.size() == seats && level->player_count() == seats && level->view_count() == (trial.bot(3) ? 1 : 2),
                "Team trial lost participants or changed human pane allocation");
            for (int seat = 0; seat < seats; ++seat)
            {
                const PlayerInputData& input = inputs[static_cast<std::size_t>(seat)];
                require(input.connected && !input.toggle_pause_menu && !input.pause_held && !input.toggle_debug,
                    "Team trial emitted a menu/pause/device action");
                require(level->player(seat).primary_weapon().type() == trial.tool(seat) &&
                    level->player(seat).team() == settings.player_team(seat), "Team trial changed a participant's tool/team");
                if (trial.bot(seat)) { require(level->player(seat).player_num() == -1, "Team bot claimed a physical slot"); }
                else
                {
                    require(input.x_movement == 0.0f && !input.jump_pressed && !input.jump_held && !input.primary_shoot &&
                        !input.drop_through, "Team driver replaced a neutral human's input");
                }
                observe(result.actors[static_cast<std::size_t>(seat)], level->player(seat), input,
                    world.assets.tuning, regions, ammo_before[static_cast<std::size_t>(seat)], tick);
            }
            const std::vector<PaintSurfaceSample> paint = level->paint_surfaces();
            require(paint.size() == previous_paint.size(), "Static team arena changed its scoring tile count");
            for (std::size_t tile = 0; tile < paint.size(); ++tile)
            {
                const PaintSurfaceSample& now = paint[tile]; const PaintSurfaceSample& before = previous_paint[tile];
                // Area bits too: the exchange ledger allocates each tile once.
                require(now.source_index == before.source_index && now.face == before.face && now.corners == before.corners &&
                    std::bit_cast<std::uint32_t>(now.scored_area) == std::bit_cast<std::uint32_t>(before.scored_area),
                    "Static team scoring tile order changed during measurement");
                if (now.paint_owner == before.paint_owner || now.team == PlayerTeam::none) { continue; }
                for (int seat = 1; seat < seats; ++seat)
                {
                    if (!trial.bot(seat) || now.paint_owner != paint_owners::index(-1, seat)) { continue; }
                    ActorMetrics& m = result.actors[static_cast<std::size_t>(seat)];
                    if (before.team == PlayerTeam::none) { ++m.neutral_paints; m.neutral_paint_area += now.scored_area; }
                    else if (before.team != now.team) { ++m.enemy_repaints; m.enemy_repaint_area += now.scored_area; }
                    else { ++m.ally_repaints; m.ally_repaint_area += now.scored_area; }
                }
            }
            ledger.step(tick, paint);
            previous_paint = paint;
            for (std::size_t team = 0; team < 2; ++team)
            { result.middle_hold[team].sample(tick, steps, ledger.held(middle, team), ledger.tolerance()); }
            result.both_middle_steps += result.middle_hold[0].holding && result.middle_hold[1].holding ? 1 : 0;
        }
        std::vector<balance_regions::RosterSeat> metric_roster;
        for (int seat = 0; seat < seats; ++seat)
        {
            const Player& player = level->player(seat);
            metric_roster.push_back({ player.team(), paint_owners::index(player.player_num(), seat), name(trial.tool(seat)) });
            const PlayerLethalProjectiles& lethal = player.lethal_projectiles();
            result.actors[static_cast<std::size_t>(seat)].uncredited_lethal = lethal.uncredited;
            for (int killer = 0; killer < seats; ++killer)
            {
                const int owner = paint_owners::index(level->player(killer).player_num(), killer);
                const std::uint64_t count = lethal.enemy_by_owner.at(static_cast<std::size_t>(owner));
                require(player.team() != level->player(killer).team() || count == 0, "Team trial attributed a teammate lethal hit");
                result.actors[static_cast<std::size_t>(seat)].enemy_lethal_taken += count;
                result.actors[static_cast<std::size_t>(killer)].eliminations += count;
            }
        }
        const balance_regions::Measurement measured(previous_paint, metric_roster, regions);
        const LevelEndInfo score = level->level_end_info();
        PaintTotal expected_score;
        expected_score.total = score.total_area; expected_score.team_a = score.team_a_area;
        expected_score.team_b = score.team_b_area; expected_score.player_areas = score.player_areas;
        measured.verify_runtime(expected_score);
        for (const balance_regions::Cell& cell : measured.cells())
        {
            const bool outside = cell.region == regions.bands.size();
            const std::string region_name = outside ? "outside_partition" : regions.bands[cell.region].name;
            require(cell.team == PlayerTeam::none || (cell.seat > 0 && trial.bot(cell.seat - 1)),
                "Team regional paint lost its valid participant owner");
            regional_csv << trial_id << ',' << trial.stage << ',' << tool_label(trial) << ',' << name(trial.human_team) << ',' <<
                regions.name << ',' << region_name << ',';
            if (outside) { regional_csv << ",,,"; }
            else
            {
                regional_csv << regions.bands[cell.region].left << ',' << regions.bands[cell.region].right << ',' <<
                    regions.top << ',' << regions.bottom;
            }
            regional_csv << ',' << static_cast<int>(cell.face) << ',' <<
                (cell.team == PlayerTeam::none ? '-' : name(cell.team)) << ',' << cell.seat << ',' << cell.paint_owner << ',' <<
                cell.held_sample_area << ',' << cell.region_face_sample_paintable_area << ',' <<
                cell.region_face_sample_painted_area << ',' << matrix(trial) << ',' << trial.seed << '\n';
            if (cell.seat == 0) { continue; }
            ActorMetrics& m = result.actors.at(static_cast<std::size_t>(cell.seat - 1));
            const std::size_t team = cell.team == PlayerTeam::a ? 0 : 1;
            const std::size_t home = team == 0 ? 0 : regions.bands.size() - 1;
            m.final_paint += cell.held_sample_area; result.team_paint[team] += cell.held_sample_area;
            if (!outside && cell.region != home)
            { m.off_home_paint += cell.held_sample_area; result.team_off_home[team] += cell.held_sample_area; }
            if (cell.region == regions.bands.size() / 2)
            { m.middle_paint += cell.held_sample_area; result.team_middle[team] += cell.held_sample_area; }
        }
        ledger.finish(previous_paint);
        std::vector<std::array<double, 2>> held(ledger.regions());
        for (const balance_regions::Cell& cell : measured.cells())
        {
            if (cell.team != PlayerTeam::none)
            { held[cell.region][cell.team == PlayerTeam::a ? 0 : 1] += cell.held_sample_area; }
        }
        const double tolerance = ledger.tolerance() + measured.allocation_tolerance();
        for (std::size_t region = 0; region < ledger.regions(); ++region)
        {
            for (std::size_t team = 0; team < 2; ++team)
            {
                require(std::abs(ledger.conservation_residual(region, team)) <= tolerance,
                    "Regional exchange does not conserve team-held area");
                require(std::abs(ledger.team(region, team).final_held - held[region][team]) <= tolerance,
                    "Regional exchange disagrees with the reviewed final regional measurement");
            }
        }
        // The regional seat transitions must sum to the arena-wide counters:
        // the same transitions, attributed to Measurement's exact regions.
        for (int seat = 1; seat < seats; ++seat)
        {
            if (!trial.bot(seat)) { continue; }
            double enemy = 0.0, neutral = 0.0, ally = 0.0;
            for (std::size_t region = 0; region < ledger.regions(); ++region)
            {
                const balance_regions::SeatRegionExchange& x = ledger.seat(region, static_cast<std::size_t>(seat));
                enemy += x.enemy_capture; neutral += x.neutral_claim; ally += x.ally_owner_change;
            }
            const ActorMetrics& m = result.actors[static_cast<std::size_t>(seat)];
            require(std::abs(enemy - m.enemy_repaint_area) <= tolerance && std::abs(neutral - m.neutral_paint_area) <= tolerance &&
                std::abs(ally - m.ally_repaint_area) <= tolerance, "Regional exchange does not reconcile with arena-wide transitions");
        }
        for (std::size_t team = 0; team < 2; ++team)
        {
            result.middle_enemy_capture[team] = ledger.team(middle, team).enemy_capture;
            result.middle_contested[team] = ledger.team(middle, team).contested_held;
            result.middle_late_loss[team] = ledger.team(middle, team).last_lost_step >= steps - steps / 3 ? 1 : 0;
        }
        write_exchanges(exchange_csv, trial_id, trial, regions, ledger);
        result.final_state = snapshot(*level);
        if (replay_check)
        {
            const std::vector<std::uint64_t> expected = snapshot(*level);
#if COLOURWARS_DEV_TOOLS
            const MatchFingerprint expected_full = MatchFingerprint::of(*level, steps);
#endif
            level.reset(); level = world.build(settings, trial.seed); prepare(*level);
            tape.playback = true;
            for (int tick = 0; tick < steps; ++tick) { level->update(physical, dt, &tape); }
            require(tape.next == static_cast<std::size_t>(steps) && snapshot(*level) == expected,
                "Team final-input replay changed players, projectile count or exact paint state");
#if COLOURWARS_DEV_TOOLS
            require(MatchFingerprint::difference(expected_full, MatchFingerprint::of(*level, steps)).empty(),
                "Team final-input replay changed complete Debug fingerprint");
#endif
        }
        return result;
    }

    void write(std::ostream& csv, int trial_id, const Trial& trial, const Metrics& result)
    {
        for (int seat = 0; seat < seats; ++seat)
        {
            const ActorMetrics& m = result.actors[static_cast<std::size_t>(seat)];
            const PlayerTeam team = seat < 2 ? trial.human_team :
                (trial.human_team == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a);
            const std::size_t team_index = team == PlayerTeam::a ? 0 : 1;
            csv << trial_id << ',' << trial.stage << ',' << tool_label(trial) << ',' << name(trial.human_team) << ',' <<
                trial.seed << ',' << trial.steps() << ',' << seat + 1 << ',' << (trial.bot(seat) ? "normal" : "human") << ',' <<
                name(team) << ',' << m.max_x - m.min_x << ',' << m.max_y - m.min_y << ',' << m.path << ',' <<
                m.movement_steps << ',' << m.jumps << ',' << m.fire_requests << ',' << m.shooting_flag_steps << ',' <<
                m.paid_fire_steps << ',' <<
                m.min_ammo << ',' << m.empty_steps << ',' << m.low_steps << ',' << m.recoveries << ',' <<
                m.damage_taken << ',' << m.first_damage << ',' << m.deaths << ',' << m.eliminations << ',' <<
                m.enemy_lethal_taken << ',' << m.uncredited_lethal << ',' << m.final_paint << ',' <<
                m.off_home_paint << ',' << m.middle_paint << ',' << m.middle_steps << ',' << m.enemy_half_steps << ',' <<
                m.neutral_paints << ',' << m.enemy_repaints << ',' << m.ally_repaints << ',' <<
                m.neutral_paint_area << ',' << m.enemy_repaint_area << ',' << m.ally_repaint_area << ',' <<
                result.team_paint[team_index] << ',' << result.team_off_home[team_index] << ',' <<
                result.team_middle[team_index] << ',' << matrix(trial) << ',' << trial.rotation << ',' <<
                name(trial.tool(seat)) << ',' << m.off_home_steps << '\n';
        }
    }

    void write_middle(std::ostream& csv, int trial_id, const Trial& trial, const Metrics& result)
    {
        for (std::size_t team = 0; team < 2; ++team)
        {
            const MiddleHold& x = result.middle_hold[team];
            const PlayerTeam side = team == 0 ? PlayerTeam::a : PlayerTeam::b;
            int bots = 0;
            for (int seat = 0; seat < seats; ++seat)
            {
                const PlayerTeam seat_team = seat < 2 ? trial.human_team :
                    (trial.human_team == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a);
                bots += trial.bot(seat) && seat_team == side ? 1 : 0;
            }
            csv << trial_id << ',' << matrix(trial) << ',' << trial.stage << ',' << trial.rotation << ',' <<
                name(trial.human_team) << ',' << trial.seed << ',' << trial.steps() << ',' << name(side) << ',' << bots << ',' <<
                x.first << ',' << x.held_steps << ',' << x.losses << ',' << x.recaptures << ',' << x.longest_gap << ',' <<
                x.last_third_steps << ',' << x.last_third_min << ',' << x.peak << ',' << result.team_middle[team] << ',' <<
                result.both_middle_steps << ',' << result.middle_late_loss[team] << '\n';
        }
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "usage: bot_team_trials <content>");
        const std::filesystem::path output = std::filesystem::absolute(argv[0]).parent_path();
        std::ofstream csv(output / "bot_team_trials.csv"), regional_csv(output / "bot_team_trials_regions.csv"),
            exchange_csv(output / "bot_team_trials_exchanges.csv"), middle_csv(output / "bot_team_trials_middle.csv");
        require(csv.good() && regional_csv.good() && exchange_csv.good() && middle_csv.good(), "Could not open team-trial CSVs");
        csv.imbue(std::locale::classic()); regional_csv.imbue(std::locale::classic()); exchange_csv.imbue(std::locale::classic());
        middle_csv.imbue(std::locale::classic());
        csv << std::fixed << std::setprecision(3); regional_csv << std::setprecision(17); exchange_csv << std::setprecision(17);
        middle_csv << std::setprecision(17);
        csv << "trial,stage,tool,human_team,seed,steps,seat,controller,team,x_extent,y_extent,path,movement_steps,jumps,"
            "fire_requests,shooting_flag_steps,paid_fire_steps,min_ammo,empty_steps,low_steps,reserve_recoveries,damage_taken,first_damage_step,"
            "deaths,attributed_eliminations,enemy_lethal_taken,uncredited_lethal,final_owned_paint,off_home_owned_paint,"
            "middle_owned_paint,middle_steps,enemy_half_steps,neutral_paint_events,enemy_repaint_events,ally_repaint_events,"
            "neutral_transition_area,enemy_transition_area,ally_transition_area,team_final_paint,team_off_home_paint,team_middle_paint,"
            "matrix,rotation,seat_tool,off_home_steps\n";
        regional_csv << "trial,stage,tool,human_team,partition,region,region_left,region_right,region_top,region_bottom,face,team,seat,owner,held_sample_area,"
            "region_face_paintable_area,region_face_painted_area,matrix,seed\n";
        exchange_csv << "trial,matrix,stage,rotation,human_team,seed,partition,region,scope,team,seat,seat_tool,initial_held,"
            "neutral_claim_area,enemy_capture_area,enemy_capture_tiles,lost_to_enemy_area,lost_tiles,to_neutral_area,"
            "ally_owner_change_area,final_held,contested_held_area,first_enemy_capture_step,last_lost_step,mutual_exchange\n";
        middle_csv << "trial,matrix,stage,rotation,human_team,seed,steps,team,bots,first_held_step,held_steps,losses,recaptures,"
            "longest_gap_steps,last_third_held_steps,last_third_min_area,peak_area,final_area,both_held_steps,final_third_enemy_capture\n";
        int trials = 0, replays = 0, mixed_trials = 0, distinct_configs = 0;
        int mixed_active_total = 0, mixed_off_home_total = 0, sustained_trials = 0, sustained_active_total = 0;
        int left_home_total = 0, sustained_retaining = 0;
        std::vector<std::string> off_home_misses, lone_off_home_misses;
        for (const char* stage : { "crossfeed", "close_quarters", "king_of_the_hill", "turbulence" })
        {
            World world(std::filesystem::absolute(argv[1]), "bot-team-trials", "Team trial needs support '", stage);
            const balance_regions::Partition regions = partition(world, stage);
            int active_bots = 0, damaged_matches = 0, repaint_matches = 0, off_home_teams = 0, middle_teams = 0;
            int mixed_active = 0, mixed_damaged = 0, mixed_repaint = 0, mixed_off_home = 0, mixed_middle = 0;
            int mutual_middle = 0, contested_middle_teams = 0, seed_distinct = 0;
            std::array<double, 2> middle_capture{};
            int left_home = 0;
            const auto tally = [&](const Trial& trial, const Metrics& metrics, int& active, int& damaged_count,
                int& repaint_count, int& off_home, int& middle_count)
            {
                bool damaged = false, repainted = false;
                for (int seat = 0; seat < seats; ++seat)
                {
                    const ActorMetrics& m = metrics.actors[static_cast<std::size_t>(seat)];
                    damaged = damaged || m.damage_taken > 0.0; repainted = repainted || m.enemy_repaints > 0;
                    if (!trial.bot(seat)) { continue; }
                    active += m.path > 100.0 && m.paid_fire_steps > 0 && m.final_paint > 0.0 ? 1 : 0;
                    // Navigation, separate from what an outnumbered side retains.
                    left_home += m.off_home_steps > 0 ? 1 : 0;
                }
                damaged_count += damaged ? 1 : 0; repaint_count += repainted ? 1 : 0;
                for (std::size_t team = 0; team < 2; ++team)
                {
                    off_home += metrics.team_off_home[team] > 0.0 ? 1 : 0;
                    middle_count += metrics.team_middle[team] > 0.0 ? 1 : 0;
                }
            };
            for (int tool = 0; tool < 5; ++tool)
            {
                for (const PlayerTeam human_team : { PlayerTeam::a, PlayerTeam::b })
                {
                    const Trial trial = same_tool(stage, static_cast<WeaponType>(tool), human_team);
                    const bool replay = tool == 0 && human_team == PlayerTeam::a;
                    const Metrics metrics = run(world, trial, regions, replay, ++trials, regional_csv, exchange_csv);
                    write(csv, trials, trial, metrics); write_middle(middle_csv, trials, trial, metrics);
                    csv.flush(); regional_csv.flush(); exchange_csv.flush(); middle_csv.flush();
                    replays += replay ? 1 : 0;
                    tally(trial, metrics, active_bots, damaged_matches, repaint_matches, off_home_teams, middle_teams);
                }
            }
            std::cout << "MEASURE " << stage << ": moving/paid-fire/retained-paint bots=" << active_bots <<
                "/30; damage matches=" << damaged_matches << "/10; enemy-repaint matches=" << repaint_matches <<
                "/10; off-home teams=" << off_home_teams << "/20; middle-paint teams=" << middle_teams << "/20\n";
            require(active_bots == 30 && off_home_teams == 20,
                "Team bots did not move, buy fire and retain paint, or a team retained no paint outside its home band");
            for (int rotation = 0; rotation < 5; ++rotation)
            {
                for (const PlayerTeam human_team : { PlayerTeam::a, PlayerTeam::b })
                {
                    std::vector<std::vector<std::uint64_t>> finals;
                    for (const unsigned int seed : mixed_seeds)
                    {
                        const Trial trial = mixed(stage, rotation, human_team, seed);
                        const bool replay = rotation == 0 && human_team == PlayerTeam::b && seed == mixed_seeds[1];
                        const Metrics metrics = run(world, trial, regions, replay, ++trials, regional_csv, exchange_csv);
                        write(csv, trials, trial, metrics); write_middle(middle_csv, trials, trial, metrics);
                        csv.flush(); regional_csv.flush(); exchange_csv.flush(); middle_csv.flush();
                        replays += replay ? 1 : 0; ++mixed_trials;
                        tally(trial, metrics, mixed_active, mixed_damaged, mixed_repaint, mixed_off_home, mixed_middle);
                        const bool mutual = metrics.middle_enemy_capture[0] > 0.0 && metrics.middle_enemy_capture[1] > 0.0;
                        mutual_middle += mutual ? 1 : 0;
                        for (std::size_t team = 0; team < 2; ++team)
                        {
                            contested_middle_teams += metrics.middle_contested[team] > 0.0 ? 1 : 0;
                            middle_capture[team] += metrics.middle_enemy_capture[team];
                        }
                        finals.push_back(metrics.final_state);
                        for (std::size_t team = 0; team < 2; ++team)
                        {
                            if (metrics.team_off_home[team] > 0.0) { continue; }
                            const bool lone = (team == 0) == (human_team == PlayerTeam::a);
                            std::ostringstream key;
                            key << stage << " seed=" << seed << " human=" << name(human_team) << " rotation=" << rotation <<
                                " team=" << (team == 0 ? 'a' : 'b');
                            std::cout << "MEASURE mixed off-home miss " << key.str() << (lone ? " (one bot)" : " (two bots)") << '\n';
                            (lone ? lone_off_home_misses : off_home_misses).push_back(key.str());
                        }
                    }
                    // Seeds must reach play: every pair of final outcomes differs.
                    const bool distinct = finals[0] != finals[1] && finals[0] != finals[2] && finals[1] != finals[2];
                    seed_distinct += distinct ? 1 : 0;
                }
            }
            distinct_configs += seed_distinct;
            mixed_active_total += mixed_active; mixed_off_home_total += mixed_off_home;
            std::cout << "MEASURE " << stage << " mixed: moving/paid-fire/retained-paint bots=" << mixed_active <<
                "/90; damage matches=" << mixed_damaged << "/30; enemy-repaint matches=" << mixed_repaint <<
                "/30; off-home teams=" << mixed_off_home << "/60; middle-paint teams=" << mixed_middle <<
                "/60; seed-distinct configs=" << seed_distinct << "/10; middle mutual-capture matches=" << mutual_middle <<
                "/30; middle contested-held teams=" << contested_middle_teams << "/60; middle enemy-capture area a=" <<
                middle_capture[0] << " b=" << middle_capture[1] << '\n';
            int sustained_active = 0, sustained_damaged = 0, sustained_repaint = 0, sustained_off_home = 0, sustained_middle = 0;
            int held_teams = 0, recapturing_teams = 0, losing_teams = 0, last_third_teams = 0, late_lost_teams = 0;
            long long held_steps = 0, held_possible = 0, both_steps = 0;
            double lowest_share = 1.0;
            for (int rotation = 0; rotation < 5; ++rotation)
            {
                for (const PlayerTeam human_team : { PlayerTeam::a, PlayerTeam::b })
                {
                    const Trial trial = sustained(stage, rotation, human_team, mixed_seeds[static_cast<std::size_t>(rotation) % mixed_seeds.size()]);
                    const bool replay = rotation == 0 && human_team == PlayerTeam::a && std::string_view(stage) == "crossfeed";
                    const Metrics metrics = run(world, trial, regions, replay, ++trials, regional_csv, exchange_csv);
                    write(csv, trials, trial, metrics); write_middle(middle_csv, trials, trial, metrics);
                    csv.flush(); regional_csv.flush(); exchange_csv.flush(); middle_csv.flush();
                    replays += replay ? 1 : 0; ++sustained_trials;
                    tally(trial, metrics, sustained_active, sustained_damaged, sustained_repaint, sustained_off_home, sustained_middle);
                    both_steps += metrics.both_middle_steps;
                    for (const MiddleHold& hold : metrics.middle_hold)
                    {
                        if (hold.first < 0) { continue; }
                        ++held_teams; held_steps += hold.held_steps; held_possible += trial.steps() - hold.first;
                        losing_teams += hold.losses > 0 ? 1 : 0;
                        recapturing_teams += hold.recaptures > 0 ? 1 : 0;
                        // Held at every final-third step; its share of peak is a measurement.
                        const bool retained = hold.last_third_steps == trial.steps() / 3;
                        last_third_teams += retained ? 1 : 0;
                        lowest_share = std::min(lowest_share, hold.peak > 0.0 ? hold.last_third_min / hold.peak : 0.0);
                    }
                    late_lost_teams += metrics.middle_late_loss[0] + metrics.middle_late_loss[1];
                }
            }
            sustained_active_total += sustained_active;
            std::cout << "MEASURE " << stage << " sustained 1v1: moving/paid-fire/retained-paint bots=" << sustained_active <<
                "/20; damage matches=" << sustained_damaged << "/10; enemy-repaint matches=" << sustained_repaint <<
                "/10; off-home teams=" << sustained_off_home << "/20; middle-paint teams=" << sustained_middle <<
                "/20; middle-holding teams=" << held_teams << "/20; held after first=" <<
                (held_possible > 0 ? static_cast<double>(held_steps) / static_cast<double>(held_possible) : 0.0) <<
                "; teams losing the middle=" << losing_teams << "; recapturing=" << recapturing_teams <<
                "; teams losing middle tiles to the enemy in the final third=" << late_lost_teams <<
                "/20; retaining through the final third=" << last_third_teams << "/20; lowest final-third share of peak=" <<
                lowest_share << "; both-held steps=" << both_steps << '\n';
            std::cout << "MEASURE " << stage << " bots leaving home band=" << left_home << "/140\n";
            left_home_total += left_home; sustained_retaining += last_third_teams;
        }
        require(distinct_configs == 40, "An independent team-trial seed did not change the match outcome");
        require(mixed_active_total == 360, "A mixed-tool team bot did not move, buy fire and retain paint");
        require(sustained_active_total == 80, "A sustained team bot did not move, buy fire and retain paint");
        require(sustained_retaining == 80, "A sustained 1v1 team lost the whole middle band during the final third");
        // Navigation is the bot's own body leaving its home band. What the
        // one-bot side still holds at the end is an outnumbered outcome.
        require(left_home_total == 560, "A team bot never left its home band");
        require(off_home_misses.empty(), "A two-bot mixed-tool team retained no paint outside its home band");
        for (const std::string& miss : lone_off_home_misses)
        {
            // The fixed home-band loop: its lone Sprayer now reaches scoring ground.
            require(miss != "close_quarters seed=61403 human=a rotation=0 team=a",
                "The Close Quarters seed-61403 home-band loop returned");
        }
        std::cout << "MEASURE mixed one-bot teams retaining off-home paint=" << 120 - lone_off_home_misses.size() << "/120\n";
        require(trials == 200 && mixed_trials == 120 && sustained_trials == 40 && replays == 9 && csv.good() &&
            regional_csv.good() && exchange_csv.good() && middle_csv.good(), "Team-trial finite matrix/report contract failed");
        std::cout << "MEASURE mixed totals: moving/paid-fire/retained-paint bots=" << mixed_active_total <<
            "/360; off-home teams=" << mixed_off_home_total << "/240\n";
        std::cout << "PASS 200 bounded natural-spawn team measurements (40 same-tool and 120 mixed-tool 2v2 over three "
            "independent seeds, 40 sustained 7200-step 1v1) and eight 1800-step plus one 7200-step final-input replays; reports=" <<
            path_to_utf8(output / "bot_team_trials.csv") << ", " << path_to_utf8(output / "bot_team_trials_regions.csv") <<
            ", " << path_to_utf8(output / "bot_team_trials_exchanges.csv") << " and " <<
            path_to_utf8(output / "bot_team_trials_middle.csv") <<
            "; all bots move/buy fire/retain paint and leave their home band, two-bot teams retain off-home paint and "
            "sustained 1v1 teams retain the middle through the final third; neutral humans never act, bots use Normal; "
            "one-bot retention, damage and repaint outcomes remain measurements\n";
        return 0;
    }
    catch (const std::exception& error) { std::cerr << "FAIL bot team trials: " << error.what() << '\n'; return 1; }
}
