#include "game/pch.h"
#include "game/input_slots.h"
#include "game/match_setup.h"
#include "scripts/acceptance/support/probe.h"
#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

void check_unsupported_bot_consumers(const std::filesystem::path& content);

namespace
{
    using acceptance::require;

    void same_settings(const MatchSetup& before, const MatchSetup& after,
        bool compare_teams = true)
    {
        require(before.player_count() == after.player_count() &&
            before.human_count() == after.human_count() && before.bot_count() == after.bot_count() &&
            before.stage() == after.stage() && before.duration() == after.duration() &&
            before.best_of() == after.best_of() && before.screen_layout() == after.screen_layout(),
            "A roster operation changed unrelated match settings or seat counts");
        for (size_t seat = 0; seat < before.player_settings().size(); ++seat)
        {
            const SeatSetup& first = before.player_settings()[seat];
            const SeatSetup& second = after.player_settings()[seat];
            require(first.controller == second.controller && first.difficulty == second.difficulty &&
                first.num == second.num && first.weapon == second.weapon &&
                (!compare_teams || first.team == second.team),
                "A retained seat lost its controller, difficulty, device, weapon or team");
        }
    }

    template<class Exception, class Action>
    void rejects_unchanged(MatchSetup& settings, Action action, const char* message)
    {
        const MatchSetup before = settings;
        bool rejected = false;
        try { action(); }
        catch (const Exception&) { rejected = true; }
        require(rejected, message);
        same_settings(before, settings);
    }

    void check_defaults_and_errors()
    {
        MatchSetup settings;
        require(settings.player_count() == 0 && settings.human_count() == 0 &&
            settings.bot_count() == 0 && !settings.input_roster_complete(),
            "An empty roster acquired a participant or became complete");
        for (int count = 1; count <= 4; ++count)
        {
            settings.set_player_count(count);
            require(settings.human_count() == count && settings.bot_count() == 0 &&
                !settings.input_roster_complete(), "New seats must be unclaimed humans");
            for (const SeatSetup& player : settings.player_settings())
            {
                require(player.controller == PlayerController::human && player.num == -1 &&
                    player.difficulty == BotDifficulty::normal,
                    "A new human seat must not claim a device or carry a nondefault difficulty");
            }
        }
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        require(settings.claim_input_slot(2), "Could not claim an ordinary human seat");
        for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
        {
            rejects_unchanged<std::invalid_argument>(settings,
                [&] { settings.set_player_bot(0, difficulty); }, "The setup host must remain human");
        }
        for (const int index : { -1, 4, 100 })
        {
            rejects_unchanged<std::out_of_range>(settings,
                [&] { settings.set_player_bot(index, BotDifficulty::normal); },
                "An out-of-roster bot index must be rejected");
            rejects_unchanged<std::out_of_range>(settings,
                [&] { settings.set_player_human(index); },
                "An out-of-roster human index must be rejected");
        }
        for (const int difficulty : { -1, 3, 100 })
        {
            rejects_unchanged<std::invalid_argument>(settings,
                [&] { settings.set_player_bot(1, static_cast<BotDifficulty>(difficulty)); },
                "Unknown bot difficulties must be rejected before altering a claimed seat");
        }
        for (const int count : { 0, 5, -1 })
        {
            rejects_unchanged<std::invalid_argument>(settings,
                [&] { settings.set_player_count(count); }, "Invalid counts must preserve a mixed-capable roster");
        }
        for (const int slot : { -1, input_slots::count })
        {
            rejects_unchanged<std::invalid_argument>(settings,
                [&] { settings.begin_input_claims(slot); }, "An invalid host device must preserve the roster");
        }
    }

    void check_device_conversion()
    {
        MatchSetup settings;
        settings.set_player_count(4);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        require(settings.claim_input_slot(3) && settings.claim_input_slot(1),
            "Could not prepare sparse human device claims");
        settings.set_player_team(1, PlayerTeam::b);
        settings.set_player_weapon(1, WeaponType::bucket);
        settings.set_player_bot(1, BotDifficulty::hard);
        require(settings.player_settings()[1].num == -1 && settings.player_index_for_slot(3) == -1 &&
            settings.bot_count() == 1 && settings.human_count() == 3 &&
            settings.player_team(1) == PlayerTeam::b && settings.player_weapon(1) == WeaponType::bucket,
            "Converting a human to a bot must release only its device claim");
        require(settings.claim_input_slot(3) && settings.player_settings()[3].num == 3 &&
            settings.player_settings()[1].num == -1 && settings.input_roster_complete(),
            "The released device must join the next human gap, never the bot");
        require(!settings.claim_input_slot(3) && !settings.claim_input_slot(-1) &&
            !settings.claim_input_slot(input_slots::count) && !settings.release_input_slot(-1) &&
            !settings.release_input_slot(input_slots::keyboard_and_mouse),
            "Bots must not relax unique-device, valid-slot or host-retention rules");

        const MatchSetup before_noop = settings;
        settings.set_player_human(2);
        same_settings(before_noop, settings);
        settings.set_player_bot(1, BotDifficulty::easy);
        require(settings.player_settings()[1].difficulty == BotDifficulty::easy &&
            settings.player_settings()[1].num == -1 && settings.input_roster_complete(),
            "Changing bot difficulty must preserve its device-less occupied seat");
        settings.set_player_human(1);
        require(settings.player_settings()[1].controller == PlayerController::human &&
            settings.player_settings()[1].num == -1 && !settings.input_roster_complete() &&
            settings.human_count() == 4 && settings.bot_count() == 0,
            "Returning a bot to human control must require an explicit new device claim");
        require(settings.claim_input_slot(0) && settings.player_index_for_slot(0) == 1 &&
            settings.input_roster_complete(), "A returned human must reclaim its stable seat");
        require(settings.release_input_slot(1) && !settings.input_roster_complete() &&
            settings.player_settings()[2].num == -1 && settings.player_index_for_slot(0) == 1 &&
            settings.player_index_for_slot(3) == 3,
            "Human leave must preserve all other stable seat identities");
    }

    void check_mixed_rosters()
    {
        constexpr std::array difficulties{ BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard };
        constexpr std::array slots{ input_slots::keyboard_and_mouse, 3, 1, 0 };
        constexpr std::array weapons{ WeaponType::sprayer, WeaponType::roller, WeaponType::bucket, WeaponType::sniper };
        for (int count = 1; count <= 4; ++count)
        {
            for (unsigned int bots = 0; bots < (1u << (count - 1)); ++bots)
            {
                for (size_t variant = 0; variant < difficulties.size(); ++variant)
                {
                    MatchSetup settings;
                    settings.set_player_count(count);
                    settings.set_stage("crossfeed");
                    settings.set_duration(123.5f);
                    settings.set_best_of(count == 1 ? 1 : 3);
                    settings.set_screen_layout(static_cast<labrador::ScreenLayout>(count - 1));
                    int expected_bots = 0;
                    for (int seat = 0; seat < count; ++seat)
                    {
                        settings.set_player_team(seat, seat % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
                        settings.set_player_weapon(seat, weapons[static_cast<size_t>(seat)]);
                        if (seat > 0 && (bots & (1u << (seat - 1))) != 0)
                        {
                            settings.set_player_bot(seat, difficulties[(variant + static_cast<size_t>(seat)) % difficulties.size()]);
                            ++expected_bots;
                        }
                    }
                    require(!settings.input_roster_complete(), "Bots cannot substitute for an unclaimed host");
                    settings.begin_input_claims(slots[0]);
                    for (int seat = 1; seat < count; ++seat)
                    {
                        if (settings.player_settings()[static_cast<size_t>(seat)].controller == PlayerController::human)
                        {
                            require(settings.claim_input_slot(slots[static_cast<size_t>(seat)]),
                                "Mixed roster could not claim a remaining human gap");
                            require(settings.player_index_for_slot(slots[static_cast<size_t>(seat)]) == seat,
                                "Device joining renumbered a human around a bot");
                        }
                    }
                    require(settings.input_roster_complete() && settings.bot_count() == expected_bots &&
                        settings.human_count() == count - expected_bots,
                        "Mixed roster completeness must count bot seats and claimed human seats exactly once");
                    for (const SeatSetup& player : settings.player_settings())
                    {
                        require(player.controller == PlayerController::human || player.num == -1,
                            "A bot consumed a physical device slot");
                    }
                    const MatchSetup retained = settings;
                    MatchSetup copied = settings;
                    same_settings(retained, copied);
                    if (count > 1)
                    {
                        std::srand(static_cast<unsigned int>(variant) + bots * 7u);
                        require(copied.shuffle_teams(), "A complete mixed roster must support balanced random teams");
                        same_settings(retained, copied, false);
                        int team_a = 0;
                        for (const SeatSetup& player : copied.player_settings())
                        {
                            require(player.team == PlayerTeam::a || player.team == PlayerTeam::b,
                                "Shuffle left an unresolved mixed-roster team");
                            team_a += player.team == PlayerTeam::a ? 1 : 0;
                        }
                        require(team_a > 0 && team_a < count && std::abs(team_a - (count - team_a)) <= 1,
                            "Bots and humans must participate in the same balanced team shuffle");
                    }
                    same_settings(retained, settings);

                    settings.begin_input_claims(2);
                    require(settings.player_index_for_slot(2) == 0 && settings.bot_count() == expected_bots,
                        "Starting fresh input claims must retain bot roles and bind only the host");
                    for (int seat = 1; seat < count; ++seat)
                    {
                        const SeatSetup& current = settings.player_settings()[static_cast<size_t>(seat)];
                        const SeatSetup& previous = retained.player_settings()[static_cast<size_t>(seat)];
                        require(current.num == -1 && current.controller == previous.controller &&
                            current.difficulty == previous.difficulty && current.team == previous.team &&
                            current.weapon == previous.weapon,
                            "Starting fresh claims must clear devices without resetting retained bot or gameplay choices");
                    }
                    require(settings.input_roster_complete() == (settings.human_count() == 1),
                        "Fresh claims may complete only a host-and-bots roster");
                }
            }
        }
    }

    void check_resize()
    {
        MatchSetup settings;
        settings.set_player_count(4);
        settings.begin_input_claims(2);
        settings.set_player_bot(1, BotDifficulty::hard);
        settings.set_player_team(1, PlayerTeam::b);
        settings.set_player_weapon(1, WeaponType::mister);
        settings.set_player_bot(3, BotDifficulty::easy);
        require(settings.claim_input_slot(0), "Could not claim the resized roster's human gap");
        const MatchSetup before = settings;
        settings.set_player_count(2);
        require(settings.player_settings()[0].num == before.player_settings()[0].num &&
            settings.player_settings()[1].controller == PlayerController::bot &&
            settings.player_settings()[1].difficulty == BotDifficulty::hard &&
            settings.player_team(1) == PlayerTeam::b && settings.player_weapon(1) == WeaponType::mister &&
            settings.input_roster_complete() && settings.human_count() == 1 && settings.bot_count() == 1 &&
            settings.player_index_for_slot(0) == -1,
            "Shrinking must preserve retained bot choices and drop removed device claims");
        settings.set_player_count(4);
        require(settings.human_count() == 3 && settings.bot_count() == 1 && !settings.input_roster_complete(),
            "Regrowing must add fresh unclaimed humans, without resurrecting removed bots");
        for (const size_t seat : { 2u, 3u })
        {
            require(settings.player_settings()[seat].controller == PlayerController::human &&
                settings.player_settings()[seat].num == -1,
                "A regrown seat inherited a removed player's controller or device");
        }
        settings.set_player_count(1);
        require(settings.human_count() == 1 && settings.bot_count() == 0 && settings.input_roster_complete(),
            "Reducing to the host must preserve bot-free solo practice");
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("bot roster", [&]
    {
        require(argc == 2, "Usage: bot_roster.exe <game/content directory>");
        check_defaults_and_errors();
        check_device_conversion();
        check_mixed_rosters();
        check_resize();
        check_unsupported_bot_consumers(argv[1]);
        std::cout << "PASS bot roster: per-seat difficulty, mixed human/bot claims, host retention, "
            "conversion, reset, copies, balanced teams and resizing.\n";
        return 0;
    });
}
