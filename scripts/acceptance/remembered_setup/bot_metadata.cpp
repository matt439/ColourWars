#include "game/pch.h"
#include "engine/assets/json.h"
#include "game/input_slots.h"
#include "game/last_match_setup.h"
#include "game/save.h"
#include "game/settings_session.h"
#include "game/match_setup.h"
#include "scripts/acceptance/support/probe.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace
{
    using acceptance::require;

    template<class Action> void rejects(Action action)
    {
        bool rejected = false;
        try { action(); }
        catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "Invalid remembered bot metadata was accepted");
    }

    const std::array<labrador::ScreenLayout, 4> layouts{
        labrador::ScreenLayout::one_player, labrador::ScreenLayout::two_player,
        labrador::ScreenLayout::three_player, labrador::ScreenLayout::four_player };
    const std::array<const char*, 4> layout_keys{
        "one_player", "two_player", "three_player", "four_player" };
    const std::array<BotDifficulty, 3> difficulties{
        BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard };
    const std::array<const char*, 3> difficulty_keys{ "easy", "normal", "hard" };
    const StageList stages{ { "arena", "Arena", "", true } };

    void write(const std::filesystem::path& path, const std::string& section)
    {
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        stream << "{\"save_version\":2,\"music_volume\":37,\"last_match\":" << section << '}';
        require(stream.good(), "Cannot write isolated bot remembered-setup fixture");
    }

    std::string bytes(const std::filesystem::path& path)
    {
        std::ifstream stream(path, std::ios::binary);
        require(stream.good(), "Cannot read isolated bot remembered-setup fixture");
        return { std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
    }

    void check_round_trips(const std::filesystem::path& path)
    {
        for (int count = 1; count <= 4; ++count)
        {
            // P1 is always human; every other seat may independently be a bot.
            for (int mask = 0; mask < (1 << (count - 1)); ++mask)
            {
                for (size_t variant = 0; variant < difficulties.size(); ++variant)
                {
                    MatchSetup original;
                    original.set_player_count(count);
                    original.set_stage("arena");
                    original.set_best_of(count == 1 ? 1 : 5);
                    original.set_duration(45.25f);
                    original.begin_input_claims(input_slots::keyboard_and_mouse);
                    for (int seat = 0; seat < count; ++seat)
                    {
                        original.set_player_team(seat, seat % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
                        original.set_player_weapon(seat, seat % 2 == 0 ? WeaponType::roller : WeaponType::bucket);
                        if (seat != 0)
                        {
                            if ((mask & (1 << (seat - 1))) != 0)
                            {
                                original.set_player_bot(seat, difficulties[(variant + static_cast<size_t>(seat)) % 3]);
                            }
                            else { require(original.claim_input_slot(3 - seat), "Cannot claim mixed fixture human"); }
                        }
                    }
                    const size_t humans = static_cast<size_t>(original.human_count());
                    original.set_screen_layout(layouts[humans - 1]);
                    require(original.input_roster_complete(), "Mixed fixture did not complete its human claims");
                    const LastMatchSetup captured = LastMatchSetup::capture(original);
                    captured.validate();
                    Save save; SettingsSession save_session(save);
                    require(save.remember_match_setup_and_save(captured), "Cannot persist mixed remembered setup");
                    const auto document = labrador::read_json_file(path.string().c_str());
                    const auto section = document.root().object("last_match");
                    require(section.string("layout") == layout_keys[humans - 1] &&
                        section.array("players").size() == static_cast<size_t>(count) &&
                        section.integer("best_of") == (count == 1 ? 1 : 5),
                        "Saved mixed roster confused participants, human views or versus series");
                    Save reloaded; SettingsSession reloaded_session(reloaded);
                    reloaded_session.load_save_file();
                    require(reloaded.last_match_setup().has_value(), "Mixed saved roster did not reload");
                    const auto restored = reloaded.last_match_setup()->restore(stages);
                    require(restored.player_count() == count && restored.human_count() == static_cast<int>(humans) &&
                        restored.screen_layout() == layouts[humans - 1] && !restored.input_roster_complete() &&
                        restored.duration() == original.duration() && restored.best_of() == original.best_of(),
                        "Mixed restore changed seat counts, human views, timing or readiness");
                    for (int seat = 0; seat < count; ++seat)
                    {
                        const auto& expected = original.player_settings()[static_cast<size_t>(seat)];
                        const auto& actual = restored.player_settings()[static_cast<size_t>(seat)];
                        const auto entry = section.array("players").at(static_cast<size_t>(seat));
                        const bool bot = expected.controller == PlayerController::bot;
                        require(actual.num == -1 && actual.controller == expected.controller &&
                            actual.difficulty == expected.difficulty && actual.team == expected.team &&
                            actual.weapon == expected.weapon && entry.string("controller") == (bot ? "bot" : "human") &&
                            entry.string("difficulty") == (bot ? difficulty_keys[(variant + static_cast<size_t>(seat)) % 3] : "normal"),
                            "Mixed save/restore changed bot identity, difficulty, team/tool, or retained a device");
                        for (const char* transient : { "num", "device", "slot", "ready", "confirmed" })
                        {
                            require(!entry.has(transient), "Bot metadata save persisted a transient field");
                        }
                    }
                    auto joined = restored;
                    joined.begin_input_claims(2);
                    require(joined.bot_count() == original.bot_count() &&
                        joined.input_roster_complete() == (humans == 1),
                        "Fresh claims removed bots or required bot device claims");
                    const auto recaptured = LastMatchSetup::capture(restored);
                    require(recaptured.layout == captured.layout && recaptured.players.size() == captured.players.size(),
                        "Bot setup could not be captured after restoring unclaimed humans");
                }
            }
        }
    }

    void check_recovery_and_validation(const std::filesystem::path& path)
    {
        const auto recover = [&](const std::string& host, const std::string& second)
        {
            write(path, "{\"players\":[{\"team\":\"a\",\"weapon\":\"roller\"," + host +
                "},{\"team\":\"b\",\"weapon\":\"bucket\"," + second +
                "}],\"stage\":\"arena\",\"layout\":\"four_player\",\"best_of\":3}");
            Save save; SettingsSession save_session(save); save_session.load_save_file();
            require(save.last_match_setup() && save_session.play_settings().music_volume == 37,
                "Malformed bot metadata discarded setup or independent preferences");
            return *save.last_match_setup();
        };
        for (const char* bad : { "null", "3", "true", "[]", "{}", "\"unknown\"" })
        {
            auto setup = recover("\"controller\":\"bot\",\"difficulty\":\"hard\"",
                std::string("\"controller\":") + bad + ",\"difficulty\":\"easy\"");
            require(setup.players[0].controller == PlayerController::human &&
                setup.players[0].difficulty == BotDifficulty::normal &&
                setup.players[1].controller == PlayerController::human &&
                setup.players[1].difficulty == BotDifficulty::normal && setup.layout == layouts[1],
                "Unknown controller did not recover to human or host was not repaired before view count");
            setup = recover("\"controller\":\"human\"",
                std::string("\"controller\":\"bot\",\"difficulty\":") + bad);
            require(setup.players[1].controller == PlayerController::bot &&
                setup.players[1].difficulty == BotDifficulty::normal && setup.layout == layouts[0] && setup.best_of == 3,
                "Malformed difficulty discarded the bot, human-only layout or versus series");
        }
        auto legacy = recover("\"unused\":true", "\"unused\":true");
        require(legacy.players[0].controller == PlayerController::human &&
            legacy.players[1].controller == PlayerController::human && legacy.layout == layouts[1],
            "Legacy entries were inferred to be bots");
        auto missing_difficulty = recover("\"unused\":true", "\"controller\":\"bot\"");
        require(missing_difficulty.players[1].controller == PlayerController::bot &&
            missing_difficulty.players[1].difficulty == BotDifficulty::normal && missing_difficulty.layout == layouts[0],
            "Missing bot difficulty did not default independently to Normal");
        Save save; SettingsSession save_session(save); save_session.load_save_file();
        const std::string before = bytes(path);
        const auto invalid = [&](LastMatchSetup candidate)
        {
            rejects([&] { candidate.validate(); });
            rejects([&] { save.remember_match_setup_and_save(candidate); });
            require(bytes(path) == before && save.last_match_setup()->players[1].controller == PlayerController::bot,
                "Invalid bot metadata changed previous save bytes or remembered controller");
        };
        auto candidate = missing_difficulty; candidate.players[0].controller = PlayerController::bot; invalid(candidate);
        candidate = missing_difficulty; candidate.players[1].controller = static_cast<PlayerController>(99); invalid(candidate);
        candidate = missing_difficulty; candidate.players[1].difficulty = static_cast<BotDifficulty>(99); invalid(candidate);
        candidate = missing_difficulty; candidate.players[0].difficulty = BotDifficulty::hard; invalid(candidate);
        candidate = missing_difficulty; candidate.layout = layouts[1]; invalid(candidate);
        candidate = missing_difficulty; candidate.players[1].controller = static_cast<PlayerController>(99);
        candidate.players[1].difficulty = static_cast<BotDifficulty>(99);
        const auto repaired = candidate.normalised();
        require(repaired.players[1].controller == PlayerController::human &&
            repaired.players[1].difficulty == BotDifficulty::normal && repaired.layout == layouts[1],
            "Aggregate recovery counted invalid controllers as bots or retained invalid difficulty");
    }
}

void check_bot_remembered_setup(const std::filesystem::path& save_path)
{
    check_round_trips(save_path);
    check_recovery_and_validation(save_path);
}
