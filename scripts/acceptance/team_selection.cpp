#include "game/pch.h"
#include "game/input_slots.h"
#include "game/states/menu_input.h"
#include "game/match_setup.h"
#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <set>
#include <stdexcept>

namespace
{
    void require(bool condition, const std::string& message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    MatchSetup roster(int count, unsigned int variant = 0)
    {
        MatchSetup settings;
        settings.set_player_count(count);
        settings.set_stage(variant % 2 == 0 ? "close_quarters" : "movement_course");
        settings.set_duration(variant % 2 == 0 ? std::optional<float>(123.5f) : std::nullopt);
        settings.set_best_of(std::array{ 1, 3, 5 }[variant % 3]);
        settings.set_screen_layout(static_cast<labrador::ScreenLayout>(count - 1));
        // Rotate a sparse device order: keyboard-first, pad-first and unused
        // device gaps must not turn seat identity into a shuffled property.
        constexpr std::array slots{ input_slots::keyboard_and_mouse, 2, 0, 3, 1 };
        settings.begin_input_claims(slots[variant % slots.size()]);
        constexpr std::array weapons{ WeaponType::random_primary, WeaponType::roller,
            WeaponType::bucket, WeaponType::sniper };
        for (int seat = 0; seat < count; ++seat)
        {
            if (seat > 0)
            {
                require(settings.claim_input_slot(slots[(variant + static_cast<unsigned int>(seat)) % slots.size()]),
                    "Cannot claim the team-selection fixture's device");
            }
            settings.set_player_weapon(seat, weapons[static_cast<size_t>(seat)]);
            settings.set_player_team(seat, static_cast<PlayerTeam>(variant % 3));
        }
        require(settings.input_roster_complete(), "The team-selection fixture must begin fully joined");
        return settings;
    }

    void unchanged(const MatchSetup& before, const MatchSetup& after, bool teams)
    {
        require(before.player_count() == after.player_count() && before.best_of() == after.best_of() &&
            before.duration() == after.duration() && before.stage() == after.stage() &&
            before.screen_layout() == after.screen_layout(), "Shuffle changed match setup outside team assignments");
        const auto original = before.player_settings();
        const auto shuffled = after.player_settings();
        require(original.size() == shuffled.size(), "Shuffle changed the roster size");
        for (size_t seat = 0; seat < original.size(); ++seat)
        {
            require(original[seat].num == shuffled[seat].num && original[seat].weapon == shuffled[seat].weapon &&
                (!teams || original[seat].team == shuffled[seat].team),
                "Shuffle changed a retained seat, its device, weapon or protected team choice");
        }
        for (int slot = -1; slot <= input_slots::count; ++slot)
        {
            require(before.player_index_for_slot(slot) == after.player_index_for_slot(slot),
                "Shuffle changed the device-to-player lookup or admitted an invalid slot");
        }
    }

    void check_refused_rosters()
    {
        MatchSetup empty;
        const MatchSetup empty_before = empty;
        require(!empty.shuffle_teams(), "An empty roster must refuse team shuffle");
        unchanged(empty_before, empty, true);
        for (const bool joined : { false, true })
        {
            MatchSetup solo;
            solo.set_player_count(1);
            solo.set_player_team(0, PlayerTeam::b);
            solo.set_player_weapon(0, WeaponType::bucket);
            if (joined) { solo.begin_input_claims(input_slots::keyboard_and_mouse); }
            const MatchSetup before = solo;
            require(!solo.shuffle_teams(), "Solo practice must retain its chosen team");
            unchanged(before, solo, true);
        }
        for (int count = 2; count <= 4; ++count)
        {
            MatchSetup waiting;
            waiting.set_player_count(count);
            waiting.set_player_team(0, PlayerTeam::b);
            const MatchSetup before_join = waiting;
            require(!waiting.shuffle_teams(), "An unclaimed multiplayer roster must refuse shuffle");
            unchanged(before_join, waiting, true);
            for (int gap = 1; gap < count; ++gap)
            {
                MatchSetup missing = roster(count, static_cast<unsigned int>(gap));
                require(missing.release_input_slot(missing.player_settings()[static_cast<size_t>(gap)].num),
                    "Cannot release the team-selection fixture seat");
                const MatchSetup before = missing;
                require(!missing.input_roster_complete() && !missing.shuffle_teams(),
                    "A gap in joined players must refuse shuffle");
                unchanged(before, missing, true);
            }
        }
    }

    void check_balanced_rosters()
    {
        bool larger_a = false;
        bool larger_b = false;
        for (int count = 2; count <= 4; ++count)
        {
            std::set<unsigned int> partitions;
            for (unsigned int seed = 0; seed < 128; ++seed)
            {
                const MatchSetup retained = roster(count, seed);
                const MatchSetup before = retained;
                MatchSetup settings = retained;
                std::srand(seed);
                require(settings.shuffle_teams(), "A fully joined multiplayer roster must support shuffle");
                unchanged(before, settings, false);
                int team_a = 0;
                int team_b = 0;
                unsigned int partition = 0;
                for (int seat = 0; seat < count; ++seat)
                {
                    const PlayerTeam team = settings.player_team(seat);
                    require(team == PlayerTeam::a || team == PlayerTeam::b,
                        "Every shuffled player must receive one valid team");
                    team_a += team == PlayerTeam::a ? 1 : 0;
                    team_b += team == PlayerTeam::b ? 1 : 0;
                    if (team == PlayerTeam::a) { partition |= 1u << seat; }
                }
                require(team_a > 0 && team_b > 0 && team_a + team_b == count && std::abs(team_a - team_b) <= 1,
                    "Shuffle must form balanced 1v1, 2v1 or 2v2 teams");
                partitions.insert(partition);
                if (count == 3) { larger_a |= team_a == 2; larger_b |= team_b == 2; }

                // Team selection uses a copied model; the retained setup and
                // deterministic game RNG remain usable by the normal page.
                MatchSetup repeated = before;
                std::srand(seed);
                require(repeated.shuffle_teams(), "A copied joined roster must support shuffle");
                unchanged(settings, repeated, true);
                unchanged(before, retained, true);
                settings.set_player_team(0, PlayerTeam::none);
                require(settings.player_team(0) == PlayerTeam::none, "Shuffle must leave manual unassignment available");
                settings.set_player_team(0, PlayerTeam::a);
                require(settings.player_team(0) == PlayerTeam::a, "Shuffle must leave manual Team A selection available");
                settings.set_player_team(0, PlayerTeam::b);
                require(settings.player_team(0) == PlayerTeam::b, "Shuffle must leave manual Team B selection available");
                unchanged(before, settings, false);
            }
            require(partitions.size() > 1, "Seeded shuffles must offer more than one balanced assignment");
        }
        require(larger_a && larger_b, "Three-player shuffles must allow either team to receive the extra player");
    }

    void check_keyboard_shuffle(const std::filesystem::path& content)
    {
        using namespace labrador;
        const GameTuning tuning((content / "tuning.json").string().c_str());
        // The same disconnected, unpolled pad fixture as menu_navigation:
        // keyboard checks neither sample controllers nor replace their backend.
        GamepadReader reader;
        Gamepads pads(&reader);
        Keyboard keyboard;
        keyboard.set_focused(true);
        keyboard.poll();
        keyboard.poll();
        MenuInput input(&pads, &keyboard, &tuning);
        const auto read = [&] { return input.menu_inputs()[input_slots::keyboard_and_mouse].action; };
        const auto poll = [&] { keyboard.poll(); return read(); };
        const auto press = [&](Key key) { keyboard.on_key_down(key); return poll(); };
        const auto release = [&](Key key) { keyboard.on_key_up(key); return poll(); };
        require(read() == MenuInputAction::none && press(Key::r) == MenuInputAction::none &&
            poll() == MenuInputAction::none, "Shuffle must wait for an owned R release, without held repeats");
        require(release(Key::r) == MenuInputAction::shuffle && read() == MenuInputAction::shuffle &&
            poll() == MenuInputAction::none, "One R release must produce a stable shuffle action for its frame");

        keyboard.on_key_down(Key::r);
        keyboard.poll(); // This context never saw the press.
        require(release(Key::r) == MenuInputAction::none, "An unobserved R press must not shuffle on release");
        require(press(Key::r) == MenuInputAction::none, "Fresh R must arm after an unowned release");
        input.reset_actions();
        require(release(Key::r) == MenuInputAction::none, "A context reset must discard a held shuffle press");
        press(Key::r);
        require(release(Key::r) == MenuInputAction::shuffle, "Shuffle must rearm after context-reset neutral input");
        poll();

        press(Key::r);
        keyboard.set_focused(false);
        require(poll() == MenuInputAction::none, "Focus loss must not convert a held shuffle into a release action");
        keyboard.set_focused(true);
        keyboard.on_key_down(Key::r);
        require(poll() == MenuInputAction::none && poll() == MenuInputAction::none &&
            release(Key::r) == MenuInputAction::none, "Focus return must suppress the old held shuffle press");
        press(Key::r);
        require(release(Key::r) == MenuInputAction::shuffle, "A fresh shuffle must work after focus and neutral input");
        poll();

        for (const Key other : { Key::enter, Key::space, Key::escape })
        {
            keyboard.on_key_down(Key::r);
            keyboard.on_key_down(other);
            require(poll() == MenuInputAction::none, "Conflicting held menu actions must stay neutral");
            keyboard.on_key_up(Key::r);
            keyboard.on_key_up(other);
            require(poll() == MenuInputAction::none && read() == MenuInputAction::none &&
                poll() == MenuInputAction::none, "Simultaneous shuffle/confirm/back releases must not select a winner");
        }
        press(Key::enter);
        require(release(Key::enter) == MenuInputAction::proceed, "Conflicting R must not break later fresh confirmation");
        poll();

        MatchSetup joined;
        joined.set_player_count(2);
        joined.begin_input_claims(3);
        require(joined.claim_input_slot(input_slots::keyboard_and_mouse), "Cannot claim the second keyboard seat");
        press(Key::r);
        require(release(Key::r) == MenuInputAction::shuffle, "The joined keyboard must retain its shuffle mapping");
        const auto joined_input = input.player_inputs(joined);
        require(joined_input.size() == 2 && joined_input[0].action == MenuInputAction::none &&
            joined_input[1].action == MenuInputAction::shuffle,
            "Shuffle belongs to the keyboard's retained seat rather than the first player");
        const MatchSetup unjoined = roster(2, 1); // Pad 3 and Pad 1, no keyboard.
        const auto unjoined_input = input.player_inputs(unjoined);
        require(std::all_of(unjoined_input.begin(), unjoined_input.end(), [](const ProcessedMenuInput& value)
            { return value.action == MenuInputAction::none; }), "An unjoined keyboard must not shuffle a pad-only roster");
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: team_selection.exe <game/content directory>");
        check_refused_rosters();
        check_balanced_rosters();
        check_keyboard_shuffle(argv[1]);
        std::cout << "PASS team selection: balanced seeded shuffles, both odd sides, joined-roster guards, "
            "retained devices/weapons/setup, manual choices and armed keyboard shuffle.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL team selection: " << error.what() << '\n';
        return 1;
    }
}
