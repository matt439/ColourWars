#pragma once

#include "game/debug/dev_tools.h"
#if COLOURWARS_DEV_TOOLS
#include "game/objects/player_team.h"
#include "game/objects/weapon_type.h"
#include "game/objects/sub_type.h"
#include "game/objects/round_shot_shape.h"
#include "game/objects/shot_retirements.h"
#include "game/match/player_control.h"
#include "scripts/acceptance/balance_regions/metrics.h"
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct PlayerLethalProjectiles;

namespace balance_runner
{
    // A developer measurement, never an AI opponent in the shipped game.
    struct Options
    {
        std::filesystem::path content;
        std::vector<std::string> stages{ "crossfeed" };
        std::vector<std::uint32_t> seeds{ 1 };
        int players = 2;
        float duration = 60.0f;
        std::vector<WeaponType> tools;
        // A Debug behaviour, or "match": the shipping MatchBotDriver at one
        // difficulty per seat (all Normal unless --difficulty names them).
        std::string bot = "paint";
        std::vector<BotDifficulty> difficulties;
        std::vector<SubType> subs; // Empty preserves the shipped default grenade.
        std::filesystem::path tuning_patch;
        // ROAD#44: the shape every round shot in the sweep is built with,
        // set before each match is built and put back to rectangle after.
        RoundShotShape round_shots = RoundShotShape::rectangle;
        std::filesystem::path csv;
        std::filesystem::path paint_csv;
    };

    struct SeatResult
    {
        PlayerTeam team = PlayerTeam::none;
        WeaponType tool = WeaponType::none;
        int paint_owner = -1;
        float paint_area = 0.0f;
        std::uint64_t alive_ticks = 0;
        std::uint64_t ammo_starved_ticks = 0;
        std::uint64_t fire_starved_ticks = 0;
        std::uint64_t engaged_ticks = 0;
        std::uint64_t refilling_ticks = 0;
        std::uint64_t ready_fire_intents = 0;
        std::uint64_t observed_sub_cooldown_resets = 0;
        SubType sub = SubType::grenade;
        // Sampled alive -> dead state edges, NOT attacker eliminations.
        // A terminal lethal hit can precede its next-step state transition.
        std::uint64_t observed_death_edges = 0;
        float final_health = 0.0f;
        float final_ammo = 0.0f;
        std::uint64_t credited_enemy_eliminations = 0;
        std::uint64_t credited_enemy_projectile_deaths = 0;
        std::uint64_t uncredited_projectile_lethals = 0;
        // REV#37, 1v1 only: ticks on which this seat's projectile hurt its one
        // opponent (a victim-keyed hit cue fired), and where that victim's body
        // was in this seat's own pane camera at the time. Never CSV fields:
        // main prints them as hit-view lines for scripts/balance/hit_view.py.
        std::uint64_t dealt_hit_ticks = 0;
        std::uint64_t dealt_hits_outside_pane = 0; // No part of the body in view.
        std::uint64_t dealt_hits_partial_pane = 0; // In view, but under half of it.
        std::uint64_t dealt_hits_shooter_down = 0; // Shooter dead or respawning.
        // Read back from the shipping driver after the match; empty for a
        // Debug behaviour. Never a CSV field: the policy label carries it.
        std::optional<BotDifficulty> difficulty;
    };

    struct Result
    {
        std::string stage;
        std::uint32_t seed = 0;
        int players = 0;
        std::string bot;
        float duration = 0.0f;
        std::uint64_t active_ticks = 0;
        float team_a_area = 0.0f;
        float team_b_area = 0.0f;
        float paintable_area = 0.0f;
        std::uint64_t final_fingerprint = 0;
        bool activity_observed = false; // Actual motion/ammo change, for nontrivial self-checks.
        // Seat 0's pane: its whole-pixel height and the body's height in it.
        // Zero when the roster is not 1v1 or the seat has no pane.
        float pane_height_pixels = 0.0f;
        float pane_body_pixels = 0.0f;
        std::array<SeatResult, 4> seats{};
        std::uint64_t credited_enemy_eliminations = 0;
        std::uint64_t uncredited_projectile_lethals = 0;
        // ROAD#44: the round-shot shape and how the match's round shots left
        // play. Never CSV fields: main prints them as shot-contacts lines.
        RoundShotShape round_shots = RoundShotShape::rectangle;
        ShotRetirements flight_retirements;
        ShotRetirements launch_retirements;
        // Owned final snapshot metrics; never drive gameplay or the main CSV fields.
        std::optional<balance_regions::Measurement> paint_report;
    };

    Options parse_options(const std::filesystem::path& content,
        const std::vector<std::wstring>& arguments);
    void validate_options(const Options& options);
    float fixed_dt();
    std::string patched_tuning_text(const std::filesystem::path& content, const std::filesystem::path& patch);
    std::string tool_name(WeaponType tool);
    std::string difficulty_name(BotDifficulty difficulty);
    // The main CSV's bot field: the behaviour, or match:<seat 1>:<seat 2>...
    std::string policy_label(const Options& options);
    std::string csv_header();
    std::string csv_row(const Result& result);
    std::string csv_text(const std::vector<Result>& results);
    std::string paint_csv_text(const std::vector<Result>& results);
    // REV#37: one line per seat of a 1v1 result, empty otherwise.
    std::string hit_view_lines(const Result& result);
    std::string round_shots_name(RoundShotShape shape);
    // ROAD#44: four lines per result, flight and launch for the balls and the
    // grenade bodies, each with its cover, player, breakable, expired and
    // out-of-bounds counts.
    std::string shot_contacts_lines(const Result& result);
    // Reserves two exclusive sibling stage files before simulation. Both are
    // complete/closed before publication. Companion moves first; main moves
    // last as a completion-marker convention, not a two-file atomic transaction.
    // Failures preserve stages/orphan companion and never replace destinations.
    class CsvOutputs final
    {
    public:
        CsvOutputs(const std::filesystem::path& main, const std::filesystem::path& paint);
        ~CsvOutputs();
        CsvOutputs(const CsvOutputs&) = delete;
        CsvOutputs& operator=(const CsvOutputs&) = delete;
        void stage(const std::vector<Result>& results);
        void publish();
        const std::filesystem::path& main_stage_path() const;
        const std::filesystem::path& paint_stage_path() const;
    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
    // Validate fixed owner provenance against the actual opposing roster.
    void attribute_eliminations(Result& result, const std::vector<PlayerLethalProjectiles>& victims);
    std::vector<Result> run_sweep(const Options& options);
    void self_tests(const std::filesystem::path& content);
}
#endif
