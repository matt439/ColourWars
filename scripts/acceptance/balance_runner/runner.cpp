#include "game/pch.h"
#include "scripts/acceptance/balance_runner/runner.h"
#if COLOURWARS_DEV_TOOLS
#include "game/ai/match_bot_driver.h"
#include "game/debug/bot_driver.h"
#include "game/debug/match_fingerprint.h"
#include "game/input_slots.h"
#include "game/paint_owners.h"
#include "game/objects/level_builder.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/weapon_feedback.h"
#include "engine/assets/json.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/audio/sound_bank.h"
#include "engine/render/sprite_font_file.h"
#include "engine/core/file_path.h"
#include <array>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <set>
#include <stdexcept>
#include <utility>

namespace balance_runner
{
    namespace
    {
        using namespace labrador;

        // CPU content and services only. There is no Application, window,
        // render device, audio device, input reader or player profile.
        class CpuWorld final
        {
        public:
            explicit CpuWorld(const std::filesystem::path& content) :
                tuning_(path_to_utf8(content / "tuning.json").c_str()),
                presentation_(path_to_utf8(content / "presentation.json").c_str())
            {
                const JsonDocument manifest = read_json_file(path_to_utf8(content / "manifest.json").c_str());
                const JsonValue assets = manifest.root().array("assets");
                for (std::size_t index = 0; index < assets.size(); ++index)
                {
                    const JsonValue asset = assets.at(index);
                    const std::string kind = asset.identifier("kind");
                    const std::filesystem::path directory = content / path_from_utf8(asset.identifier("directory"));
                    const JsonValue names = asset.array("names");
                    for (std::size_t entry = 0; entry < names.size(); ++entry)
                    {
                        const std::string name = names.at(entry).as_identifier();
                        if (kind == "sprite_sheet")
                        {
                            this->render_.add_sprite_sheet(name, read_sprite_sheet(
                                path_to_utf8(directory / path_from_utf8(name + ".json")).c_str(), TextureHandle{}));
                        }
                        else if (kind == "font")
                        {
                            const SpriteFontFile font = read_sprite_font_file(
                                path_to_utf8(directory / path_from_utf8(name + ".spritefont")));
                            auto loaded = std::make_unique<Font>(TextureHandle{}, font.glyphs, font.line_spacing);
                            if (loaded->contains(U'?')) { loaded->set_stand_in(U'?'); }
                            this->render_.add_font(name, std::move(loaded));
                        }
                        else if (kind == "sound_bank")
                        {
                            // The accepted bank is not read: this tool has no
                            // audio backend/device and never evaluates the mix.
                            this->audio_.add_sound_bank(name, SoundBank::silent());
                        }
                        else if (kind == "level")
                        {
                            auto level = std::make_unique<LevelLoadedInfo>(
                                path_to_utf8(directory / path_from_utf8(name + ".json")).c_str());
                            level->validate();
                            this->levels_.add(name, std::move(level));
                            this->stages_.insert(name);
                        }
                        else { throw std::runtime_error("Unsupported manifest kind in balance runner: " + kind); }
                    }
                }
                this->tuning_.validate_resources(this->render_, this->audio_);
                this->presentation_.validate_resources(this->render_, this->audio_);
                this->builder_ = std::make_unique<LevelBuilder>(&this->viewports_, &this->render_, &this->audio_,
                    &this->preferences_, &this->levels_, &this->tuning_, &this->presentation_,
                    &this->resolution_, nullptr, &this->partitioner_);
            }

            bool has_stage(const std::string& stage) const { return this->stages_.contains(stage); }
            mattmath::RectangleF arena(const std::string& stage) const
            { return this->levels_.get(stage)->out_of_bounds_rectangle(); }
            const GameTuning& tuning() const { return this->tuning_; }
            std::unique_ptr<Level> build(const Options& options, const std::string& stage, std::uint32_t seed)
            {
                MatchSetup settings;
                settings.set_player_count(options.players);
                settings.set_stage(stage);
                settings.set_duration(options.duration);
                switch (options.players)
                {
                case 1: settings.set_screen_layout(ScreenLayout::one_player); break;
                case 2: settings.set_screen_layout(ScreenLayout::two_player); break;
                case 3: settings.set_screen_layout(ScreenLayout::three_player); break;
                case 4: settings.set_screen_layout(ScreenLayout::four_player); break;
                default: throw std::invalid_argument("Players must be in 1..4");
                }
                for (int view = 0; view < options.players; ++view)
                {
                    settings.set_player_team(view, view % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
                    settings.set_player_weapon(view, options.tools.at(static_cast<std::size_t>(view)));
                }
                // Keep each seat unclaimed (-1): BotDriver addresses views,
                // while PlayerBuilder allocates distinct device-less paint owners.
                // These must remain sequential: gameplay also uses global rand().
                std::srand(seed);
                this->viewports_.set_layout(settings.screen_layout());
                return this->builder_->build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
            }

        private:
            GameTuning tuning_;
            Presentation presentation_;
            RenderResources render_;
            AudioResources audio_;
            PlaySettings preferences_;
            Registry<LevelLoadedInfo> levels_{ "Level" };
            std::set<std::string> stages_;
            ResolutionManager resolution_;
            ViewportManager viewports_{ &this->resolution_ };
            Mouse mouse_;
            Partitioner partitioner_;
            std::unique_ptr<LevelBuilder> builder_;
        };

        // This is the existing active-logic input seam, not a per-frame or
        // countdown clock. The bot's just-produced fire intent is sampled with
        // the actual full-shot affordability state before the world advances.
        class MetricsSeats final : public SeatInputFilter
        {
        public:
            MetricsSeats(const GameTuning& tuning, std::uint32_t seed, const Options& options)
            {
                if (options.bot == "match")
                {
                    // Every seat is the shipping driver's bot, P1 included,
                    // which a roster cannot express: P1 stays its human host.
                    std::vector<std::optional<BotDifficulty>> seats(options.difficulties.begin(),
                        options.difficulties.end());
                    auto driver = std::make_unique<MatchBotDriver>(&tuning, seats, seed);
                    this->match_ = driver.get();
                    this->bots_ = std::move(driver);
                }
                else
                {
                    auto driver = std::make_unique<BotDriver>(&tuning, seed);
                    for (int view = 0; view < options.players; ++view) { driver->assign(view, options.bot); }
                    this->bots_ = std::move(driver);
                }
            }
            void filter_seats(std::vector<PlayerInputData>& seats, const Level& level) override
            {
                this->bots_->filter_seats(seats, level);
                ++this->active_ticks_;
                for (int view = 0; view < level.player_count(); ++view)
                {
                    const Player& player = level.player(view);
                    SeatResult& result = this->seats_.at(static_cast<std::size_t>(view));
                    if (player.state() != PlayerState::alive || player.health() <= 0.0f) { continue; }
                    ++result.alive_ticks;
                    if (player.primary_weapon().readiness() == WeaponReadiness::empty)
                    {
                        ++result.ammo_starved_ticks;
                        if (seats.at(static_cast<std::size_t>(view)).primary_shoot) { ++result.fire_starved_ticks; }
                    }
                }
            }
            void observe_after(const Level& level, const std::array<PlayerState, 4>& before)
            {
                for (int view = 0; view < level.player_count(); ++view)
                {
                    if (before.at(static_cast<std::size_t>(view)) == PlayerState::alive &&
                        level.player(view).state() == PlayerState::dead)
                    { ++this->seats_.at(static_cast<std::size_t>(view)).observed_death_edges; }
                }
            }
            std::uint64_t active_ticks() const { return this->active_ticks_; }
            const std::array<SeatResult, 4>& seats() const { return this->seats_; }
            // The shipping driver, or null under a Debug behaviour.
            const MatchBotDriver* match() const { return this->match_; }

        private:
            std::unique_ptr<SeatInputFilter> bots_;
            const MatchBotDriver* match_ = nullptr;
            std::uint64_t active_ticks_ = 0;
            std::array<SeatResult, 4> seats_{};
        };

        Result run_match(CpuWorld& world, const Options& options, const std::string& stage, std::uint32_t seed)
        {
            std::unique_ptr<Level> level = world.build(options, stage, seed);
            MetricsSeats samples(world.tuning(), seed, options);
            const float dt = fixed_dt();
            const double bound = std::ceil((static_cast<double>(options.duration) +
                world.tuning().match().countdown) / dt) + 600.0;
            if (!std::isfinite(bound) || bound > 50000.0)
            { throw std::runtime_error("Configured countdown/duration exceeds the runner's 50000-step per-match budget"); }
            const std::uint64_t max_steps = static_cast<std::uint64_t>(bound);
            const std::vector<PlayerInputData> neutral(static_cast<std::size_t>(input_slots::count));
            std::uint64_t updates = 0;
            bool time_up = false, activity_observed = false;
            for (; updates < max_steps; ++updates)
            {
                std::array<PlayerState, 4> before{};
                std::array<mattmath::Vector2F, 4> before_centers{};
                std::array<float, 4> before_ammo{};
                for (int view = 0; view < level->player_count(); ++view)
                {
                    const auto index = static_cast<std::size_t>(view);
                    before.at(index) = level->player(view).state();
                    before_centers.at(index) = level->player(view).center();
                    before_ammo.at(index) = level->player(view).weapon_ammo();
                }
                const std::uint64_t active_before = samples.active_ticks();
                level->update(neutral, dt, &samples);
                if (samples.active_ticks() != active_before)
                {
                    samples.observe_after(*level, before);
                    for (int view = 0; view < level->player_count(); ++view)
                    {
                        const auto index = static_cast<std::size_t>(view);
                        activity_observed = activity_observed || level->player(view).center() != before_centers.at(index) ||
                            level->player(view).weapon_ammo() != before_ammo.at(index);
                    }
                }
                if (level->state() == LevelState::zoom_out)
                {
                    time_up = true;
                    ++updates;
                    break;
                }
            }
            if (!time_up || samples.active_ticks() == 0)
            { throw std::runtime_error("Match did not reach its real time-up transition within the bounded step budget"); }
            const LevelEndInfo end = level->level_end_info();
            Result result;
            result.stage = stage; result.seed = seed; result.players = options.players;
            result.bot = policy_label(options); result.duration = options.duration;
            result.active_ticks = samples.active_ticks();
            result.team_a_area = end.team_a_area; result.team_b_area = end.team_b_area;
            result.paintable_area = end.total_area;
            result.final_fingerprint = MatchFingerprint::of(*level, static_cast<int>(updates)).hash;
            result.seats = samples.seats();
            result.activity_observed = activity_observed;
            for (int view = 0; view < level->player_count(); ++view)
            {
                const Player& player = level->player(view);
                SeatResult& seat = result.seats.at(static_cast<std::size_t>(view));
                seat.team = player.team(); seat.tool = player.primary_weapon().type();
                seat.paint_owner = paint_owners::index(player.player_num(), view);
                if (seat.paint_owner < 0 || static_cast<std::size_t>(seat.paint_owner) >= end.player_areas.size())
                { throw std::runtime_error("Actual player has no valid paint owner"); }
                seat.paint_area = end.player_areas.at(static_cast<std::size_t>(seat.paint_owner));
                seat.final_health = player.health(); seat.final_ammo = player.weapon_ammo();
                if (samples.match() != nullptr)
                {
                    const MatchBot* bot = samples.match()->bot(view);
                    if (bot == nullptr) { throw std::runtime_error("A match-policy seat has no shipping bot"); }
                    seat.difficulty = bot->difficulty();
                }
            }
            std::vector<PlayerLethalProjectiles> victims;
            victims.reserve(static_cast<std::size_t>(level->player_count()));
            for (int view = 0; view < level->player_count(); ++view)
            { victims.push_back(level->player(view).lethal_projectiles()); }
            attribute_eliminations(result, victims);
            std::vector<balance_regions::RosterSeat> roster;
            for (int view = 0; view < result.players; ++view)
            {
                const SeatResult& seat = result.seats.at(static_cast<std::size_t>(view));
                roster.push_back({ seat.team, seat.paint_owner, tool_name(seat.tool) });
            }
            PaintTotal expected;
            expected.total = end.total_area; expected.team_a = end.team_a_area; expected.team_b = end.team_b_area;
            expected.player_areas = end.player_areas;
            result.paint_report.emplace(level->paint_surfaces(), std::move(roster),
                balance_regions::stage_partition(stage, world.arena(stage)));
            result.paint_report->verify_runtime(expected);
            if (MatchFingerprint::of(*level, static_cast<int>(updates)).hash != result.final_fingerprint)
            { throw std::runtime_error("Read-only final paint snapshot changed the existing match fingerprint"); }
            return result;
        }
    }

    void attribute_eliminations(Result& result, const std::vector<PlayerLethalProjectiles>& victims)
    {
        if (result.players < 1 || result.players > 4 || victims.size() != static_cast<std::size_t>(result.players))
        { throw std::invalid_argument("Lethal snapshots must match the actual roster"); }
        std::array<int, paint_owners::count> owner_views;
        owner_views.fill(-1);
        for (int view = 0; view < result.players; ++view)
        {
            const SeatResult& seat = result.seats.at(static_cast<std::size_t>(view));
            if (seat.paint_owner < 0 || seat.paint_owner >= paint_owners::count ||
                (seat.team != PlayerTeam::a && seat.team != PlayerTeam::b))
            { throw std::invalid_argument("Lethal attribution requires valid roster owners and teams"); }
            int& known = owner_views.at(static_cast<std::size_t>(seat.paint_owner));
            if (known >= 0) { throw std::invalid_argument("Lethal attribution requires unique roster owners"); }
            known = view;
        }
        std::array<std::uint64_t, 4> credits{}, deaths{}, uncredited{};
        std::uint64_t total_credits = 0, total_uncredited = 0;
        for (int victim = 0; victim < result.players; ++victim)
        {
            const auto index = static_cast<std::size_t>(victim);
            uncredited[index] = victims[index].uncredited;
            for (std::size_t owner = 0; owner < victims[index].enemy_by_owner.size(); ++owner)
            {
                const std::uint64_t count = victims[index].enemy_by_owner[owner];
                const int attacker = owner_views[owner];
                // A raw source owner is not sufficient credit: it must identify
                // another actual participant on the opposing team.
                if (attacker >= 0 && attacker != victim &&
                    result.seats[static_cast<std::size_t>(attacker)].team != result.seats[index].team)
                {
                    credits[static_cast<std::size_t>(attacker)] += count;
                    deaths[index] += count;
                    total_credits += count;
                }
                else { uncredited[index] += count; }
            }
            total_uncredited += uncredited[index];
        }
        // Publish only after the complete roster has validated; repeated
        // snapshots recompute the same totals instead of accumulating twice.
        result.credited_enemy_eliminations = total_credits;
        result.uncredited_projectile_lethals = total_uncredited;
        for (std::size_t view = 0; view < result.seats.size(); ++view)
        {
            result.seats[view].credited_enemy_eliminations = credits[view];
            result.seats[view].credited_enemy_projectile_deaths = deaths[view];
            result.seats[view].uncredited_projectile_lethals = uncredited[view];
        }
    }

    std::vector<Result> run_sweep(const Options& options)
    {
        validate_options(options);
        CpuWorld world(options.content);
        // Validate the complete request before the first match, including
        // manifest membership (never build an arbitrary level path).
        for (const std::string& stage : options.stages)
        {
            if (!world.has_stage(stage)) { throw std::invalid_argument("Stage is not in the content manifest: " + stage); }
        }
        std::vector<Result> results;
        results.reserve(options.stages.size() * options.seeds.size());
        for (const std::string& stage : options.stages)
        {
            for (std::uint32_t seed : options.seeds)
            {
                try { results.push_back(run_match(world, options, stage, seed)); }
                catch (const std::exception& error)
                { throw std::runtime_error("Stage " + stage + ", seed " + std::to_string(seed) + ": " + error.what()); }
            }
        }
        return results;
    }
}
#endif
