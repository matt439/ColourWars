#pragma once

#include "game/debug/dev_tools.h"
#if COLOURWARS_DEV_TOOLS
#include "game/objects/player_team.h"
#include "game/objects/weapon_type.h"
#include <array>
#include <cstdint>
#include <filesystem>
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
        std::string bot = "paint";
        std::filesystem::path csv;
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
        // Sampled alive -> dead state edges, NOT attacker eliminations.
        // A terminal lethal hit can precede its next-step state transition.
        std::uint64_t observed_death_edges = 0;
        float final_health = 0.0f;
        float final_ammo = 0.0f;
        std::uint64_t credited_enemy_eliminations = 0;
        std::uint64_t credited_enemy_projectile_deaths = 0;
        std::uint64_t uncredited_projectile_lethals = 0;
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
        std::array<SeatResult, 4> seats{};
        std::uint64_t credited_enemy_eliminations = 0;
        std::uint64_t uncredited_projectile_lethals = 0;
    };

    Options parse_options(const std::filesystem::path& content,
        const std::vector<std::wstring>& arguments);
    void validate_options(const Options& options);
    float fixed_dt();
    std::string tool_name(WeaponType tool);
    std::string csv_header();
    std::string csv_row(const Result& result);
    std::string csv_text(const std::vector<Result>& results);
    void write_csv(const std::filesystem::path& path, const std::vector<Result>& results);
    // Validate fixed owner provenance against the actual opposing roster.
    void attribute_eliminations(Result& result, const std::vector<PlayerLethalProjectiles>& victims);
    std::vector<Result> run_sweep(const Options& options);
    void self_tests(const std::filesystem::path& content);
}
#endif
