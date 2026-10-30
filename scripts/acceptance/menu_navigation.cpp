#include "game/pch.h"
#include "game/states/menu_input.h"
#include "game/match_setup.h"
#include "game/states/pause_menu.h"
#include "game/presentation.h"
#include "game/states/player_label.h"
#include "scripts/acceptance/support/probe.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
#include <stdexcept>

using namespace labrador;

namespace
{
    using acceptance::require;

    struct InputVariant
    {
        std::filesystem::path path = std::filesystem::temp_directory_path() /
            ("ColourWars-input-" + std::to_string(GetCurrentProcessId()) + ".json");
        std::string source;

        explicit InputVariant(const std::filesystem::path& content)
        {
            std::ifstream file(content / "tuning.json");
            source.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        }
        ~InputVariant()
        {
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
        }
        GameTuning load(const char* key, const char* value) const
        {
            std::string text = source;
            const size_t field = text.find(std::string("\"") + key + "\"");
            require(field != std::string::npos, "Input tuning field must exist");
            const size_t begin = text.find(':', field) + 1;
            const size_t end = text.find_first_of(",}", begin);
            text.replace(begin, end - begin, value);
            { std::ofstream file(path); file << text; }
            return GameTuning(path.string().c_str());
        }
    };

    uint16_t button(GamepadButton value)
    {
        return static_cast<uint16_t>(1u << static_cast<unsigned>(value));
    }

    void check_owner_routing()
    {
        MatchSetup roster;
        roster.set_player_count(2);
        roster.begin_input_claims(3);
        require(roster.claim_input_slot(0), "Owner-routing fixture must retain sparse pad slots 3 and 0");
        std::vector<ProcessedMenuInput> inputs(input_slots::count);
        const auto same = [](const ProcessedMenuInput& left, const ProcessedMenuInput& right)
        {
            return left.connected == right.connected && left.direction == right.direction && left.action == right.action;
        };
        const auto neutral = [&](const ProcessedMenuInput& result) { return same(result, ProcessedMenuInput()); };
        inputs[0] = { Direction::down, MenuInputAction::shuffle, true };
        inputs[1] = { Direction::left, MenuInputAction::proceed, true };
        inputs[3] = { Direction::right, MenuInputAction::back, true };
        inputs[input_slots::keyboard_and_mouse] = { Direction::up, MenuInputAction::pause, true };
        require(same(menu_input_for_owner(inputs, 3, roster), inputs[3]),
            "A connected owner must retain action and direction despite other participants and nonparticipants");
        inputs[3] = { Direction::none, MenuInputAction::none, true };
        require(same(menu_input_for_owner(inputs, 3, roster), inputs[3]),
            "A connected neutral owner must keep exclusive control");
        inputs[2] = { Direction::up, MenuInputAction::pause, true };
        require(roster.player_index_for_slot(2) == -1 && same(menu_input_for_owner(inputs, 2, roster), inputs[2]),
            "The shared owner helper must preserve a connected opening device even outside the roster");
        inputs[2].connected = false;
        inputs[3] = { Direction::right, MenuInputAction::back, true };
        require(same(menu_input_for_owner(inputs, 2, roster), inputs[0]),
            "A missing owner must fall back to an active participant in device-slot order, not roster order");
        inputs[0] = { Direction::none, MenuInputAction::none, true };
        require(same(menu_input_for_owner(inputs, 2, roster), inputs[3]),
            "A neutral earlier participant must allow a later participant's action and direction");
        inputs[3] = { Direction::right, MenuInputAction::none, true };
        require(same(menu_input_for_owner(inputs, 2, roster), inputs[3]),
            "Participant fallback must preserve navigation without an action");
        inputs[3] = { Direction::none, MenuInputAction::back, true };
        require(same(menu_input_for_owner(inputs, 2, roster), inputs[3]),
            "Participant fallback must preserve an action without navigation");
        inputs[3] = { Direction::none, MenuInputAction::none, true };
        inputs[0].connected = false;
        require(neutral(menu_input_for_owner(inputs, 2, roster)),
            "One connected neutral participant must block rescue by an active nonparticipant or keyboard");
        inputs[0] = { Direction::down, MenuInputAction::shuffle, false };
        inputs[3] = { Direction::right, MenuInputAction::back, false };
        require(same(menu_input_for_owner(inputs, 2, roster), inputs[1]),
            "When every participant is missing, the first active connected device must rescue the menu");
        inputs[1] = { Direction::none, MenuInputAction::none, true };
        require(same(menu_input_for_owner(inputs, 2, roster), inputs[input_slots::keyboard_and_mouse]),
            "Neutral nonparticipants must not block keyboard rescue when every participant is missing");
        inputs[input_slots::keyboard_and_mouse] = { Direction::none, MenuInputAction::none, true };
        require(neutral(menu_input_for_owner(inputs, 2, roster)),
            "All missing or neutral devices must leave owner routing neutral");
        inputs[1] = { Direction::left, MenuInputAction::proceed, true };
        for (const int invalid : { -1, input_slots::count, input_slots::count + 7 })
        {
            require(neutral(menu_input_for_owner(inputs, invalid, roster)),
                "An invalid opening slot must not route another device's activity");
        }
        const std::vector<ProcessedMenuInput> short_inputs(inputs.begin(), inputs.begin() + 2);
        require(neutral(menu_input_for_owner(short_inputs, 3, roster)) &&
            neutral(menu_input_for_owner({}, 0, roster)),
            "An owner outside the supplied input vector must stay neutral");
    }

    // ROAD#4: a pad back in another slot takes its seat with Start while the
    // match waits. The seat keeps its slot as identity and reads the new pad.
    void check_seat_reclaim(const MenuText& text)
    {
        MatchSetup roster;
        roster.set_player_count(3);
        roster.begin_input_claims(0);
        require(roster.claim_input_slot(1) && roster.claim_input_slot(input_slots::keyboard_and_mouse),
            "Reclaim fixture must seat pads 1 and 2 and the keyboard");
        require(roster.device_slot(0) == 0 && roster.device_slot(1) == 1 &&
            roster.device_slot(input_slots::keyboard_and_mouse) == input_slots::keyboard_and_mouse &&
            roster.player_index_for_device(1) == 1 && roster.player_index_for_device(2) == -1,
            "Every seat must read its own slot until a pad reclaims it");

        std::vector<ProcessedMenuInput> inputs(input_slots::count);
        const ProcessedMenuInput start{ Direction::none, MenuInputAction::pause, true };
        const unsigned int first_missing = 1u << 0;
        require(reclaim_missing_seat(roster, inputs, first_missing) == -1,
            "No press must reclaim nothing");
        inputs[1] = start;
        inputs[input_slots::keyboard_and_mouse] = start;
        inputs[2] = { Direction::none, MenuInputAction::pause, false };
        inputs[3] = { Direction::up, MenuInputAction::proceed, true };
        require(reclaim_missing_seat(roster, inputs, first_missing) == -1 && roster.device_slot(0) == 0,
            "A seated pad, the keyboard, a disconnected pad and a non-Start press must not reclaim");
        inputs[2].connected = true;
        require(reclaim_missing_seat(roster, inputs, 0) == -1 && roster.device_slot(0) == 0,
            "Start must reclaim nothing when no seat is missing");
        require(reclaim_missing_seat(roster, inputs, first_missing) == 0,
            "Start on a spare pad must take the missing seat");
        require(roster.device_slot(0) == 2 && roster.player_index_for_slot(0) == 0 &&
            roster.player_index_for_device(2) == 0 && roster.player_index_for_device(0) == -1,
            "The reclaimed seat must keep its identity slot and read the new pad only");
        require(roster.pad_devices() == std::array<int, input_slots::pad_count>{ 2, 1, 2, 3 },
            "Indexed readers must see the seat's new pad in its own slot");
        require(player_label(roster, 0, text) == text.roster_label(0, 2),
            "The roster must name the pad the seat now uses");

        const auto same = [](const ProcessedMenuInput& left, const ProcessedMenuInput& right)
        {
            return left.connected == right.connected && left.direction == right.direction && left.action == right.action;
        };
        inputs = std::vector<ProcessedMenuInput>(input_slots::count);
        inputs[0] = { Direction::down, MenuInputAction::proceed, true };
        inputs[2] = { Direction::left, MenuInputAction::back, true };
        require(same(menu_input_for_owner(inputs, 0, roster), inputs[2]),
            "A reclaimed owner's menu must follow its new pad, not the slot it left");
        inputs[2].connected = false;
        inputs[1] = { Direction::none, MenuInputAction::none, true };
        require(same(menu_input_for_owner(inputs, 0, roster), ProcessedMenuInput()),
            "The slot a seat left must not count as a participant for the fallback");

        inputs = std::vector<ProcessedMenuInput>(input_slots::count);
        inputs[0] = start;
        inputs[3] = start;
        require(reclaim_missing_seat(roster, inputs, (1u << 1) | (1u << 0)) == 0 &&
            roster.device_slot(0) == 0 && roster.device_slot(1) == 1,
            "Two missing seats must fill in roster order, the first back onto its own pad");
        require(reclaim_missing_seat(roster, inputs, 1u << 1) == 1 && roster.device_slot(1) == 3,
            "A second missing seat must take the next spare pad");
        require(roster.player_index_for_device(1) == -1 && roster.player_index_for_device(3) == 1,
            "The second seat must leave its old slot");

        require(!roster.move_to_device(0, 3) && roster.device_slot(0) == 0,
            "A pad another seat reads must not be taken");
        require(!roster.move_to_device(0, input_slots::keyboard_and_mouse) &&
            !roster.move_to_device(input_slots::keyboard_and_mouse, 2) && !roster.move_to_device(2, 2),
            "Only a seated pad slot may move, and only onto a pad");
        require(roster.move_to_device(1, 1) && roster.device_slot(1) == 1,
            "A seat may move back onto its own slot");
        require(roster.move_to_device(1, 2) && roster.release_input_slot(1) &&
            roster.device_slot(1) == 1 && roster.player_index_for_device(2) == -1,
            "A setup claim operation must put every seat back on its own slot");

        MatchSetup with_bot;
        with_bot.set_player_count(2);
        with_bot.begin_input_claims(0);
        with_bot.set_player_bot(1, BotDifficulty::normal);
        inputs = std::vector<ProcessedMenuInput>(input_slots::count);
        inputs[2] = start;
        require(reclaim_missing_seat(with_bot, inputs, 1u << 1) == -1,
            "A bot seat must never be reclaimed");
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("menu navigation", [&]
    {
        require(argc == 2, "Expected staged content directory");
        const Presentation look((std::filesystem::path(argv[1]) / "presentation.json").string().c_str());
        const GameTuning tuning((std::filesystem::path(argv[1]) / "tuning.json").string().c_str());
        const float delay = look.menu().repeat_delay;
        const float interval = look.menu().repeat_interval;
        check_owner_routing();
        check_seat_reclaim(look.menu_text());
        require(delay > interval && interval > 0, "Authored repeat must have a longer first delay");
        MenuDirectionRepeat navigation;
        GamepadState pad;
        pad.connected = true;
        const auto step = [&](float dt = 0.0f)
        {
            return navigation.update_pad(pad, dt, delay, interval, tuning.input());
        };
        pad.buttons = button(GamepadButton::dpad_down);
        require(step(10) == Direction::none, "Opening under a held dpad must stay neutral");
        pad.buttons = 0;
        require(step() == Direction::none, "Release must not navigate");
        pad.buttons = button(GamepadButton::dpad_down);
        require(step() == Direction::down, "Dpad must fire on press");
        require(step(delay * 0.75f) == Direction::none, "Repeat must wait for first delay");
        require(step(delay * 0.3f) == Direction::down, "Held dpad must repeat");
        require(step(interval * 0.5f) == Direction::none, "Repeat interval must be respected");
        require(step(interval * 0.6f) == Direction::down, "Later repeats use short interval");
        require(step(10) == Direction::down && step() == Direction::none,
            "A hitch must emit at most one move with no catch-up burst");
        navigation = MenuDirectionRepeat();
        require(step(10) == Direction::none, "New page must discard held repeat");
        pad.buttons = 0;
        step();
        pad.left_stick = { 0.0f, -0.9f };
        require(step() == Direction::up, "Stick must fire on engagement");
        pad.left_stick.y = -0.7f;
        require(step(delay + interval) == Direction::up, "Stick hysteresis must retain direction below engagement");
        pad.left_stick.y = -0.5f;
        require(step() == Direction::none, "Stick release must be neutral");
        pad.left_stick.y = -0.7f;
        require(step() == Direction::none, "Subthreshold stick must not engage after release");
        pad.left_stick = { 0.9f, -0.9f };
        require(step() == Direction::none, "Ambiguous diagonal must not choose a row");
        pad.left_stick.y = 0;
        require(step(10) == Direction::none, "Ambiguous input must rearm through neutral");
        pad.left_stick = {};
        step();
        pad.left_stick.x = 0.9f;
        require(step() == Direction::right, "Neutral must rearm stick navigation");

        // Calibration reserves both sticks for the response preview. A full
        // stick push cannot change the selected pad, row or deadzone, while
        // the D-pad keeps ordinary press, repeat and page-rearming behavior.
        MenuDirectionRepeat dpad_only;
        GamepadState calibration;
        calibration.connected = true;
        calibration.left_stick = { 1.0f, 1.0f };
        calibration.right_stick = { -1.0f, -1.0f };
        const auto calibration_step = [&](float dt = 0.0f)
        {
            return dpad_only.update_pad(calibration, dt, delay, interval, tuning.input(), false);
        };
        require(calibration_step() == Direction::none && calibration_step(10.0f) == Direction::none,
            "Calibration stick movement must not navigate or repeat without D-pad input");
        calibration.buttons = button(GamepadButton::dpad_down);
        require(calibration_step() == Direction::down,
            "Calibration D-pad must navigate while both sticks are held");
        require(calibration_step(delay + interval) == Direction::down,
            "Calibration D-pad must retain held navigation repeat");
        calibration.buttons = 0;
        require(calibration_step() == Direction::none,
            "Calibration D-pad release must stay neutral with sticks held");
        calibration.left_stick = { -1.0f, 0.0f };
        calibration.buttons = button(GamepadButton::dpad_right);
        require(calibration_step() == Direction::right,
            "An opposing calibration stick must not cancel D-pad navigation");
        dpad_only = MenuDirectionRepeat();
        require(calibration_step(10.0f) == Direction::none,
            "Calibration page reset must discard a held D-pad");
        calibration.buttons = 0;
        require(calibration_step() == Direction::none,
            "Calibration D-pad must rearm through neutral even with a full stick held");
        calibration.buttons = button(GamepadButton::dpad_right);
        require(calibration_step() == Direction::right,
            "Calibration D-pad must work after the page reset and neutral input");

        // Exercise authored thresholds that would fail with the old constants.
        InputVariant variant(argv[1]);
        const GameTuning lower_engage = variant.load("menu_engage_threshold", "0.65");
        const GameTuning lower_release = variant.load("menu_release_threshold", "0.3");
        navigation = MenuDirectionRepeat();
        pad.left_stick = {};
        navigation.update_pad(pad, 0, delay, interval, lower_engage.input());
        pad.left_stick.y = -0.7f;
        require(navigation.update_pad(pad, 0, delay, interval, lower_engage.input()) == Direction::up,
            "Authored engagement threshold must control navigation");
        pad.left_stick.y = -0.4f;
        require(navigation.update_pad(pad, delay + interval, delay, interval, lower_release.input()) == Direction::up,
            "Authored release threshold must control hysteresis");
        pad.left_stick.y = -0.2f;
        require(navigation.update_pad(pad, 0, delay, interval, lower_release.input()) == Direction::none,
            "Authored release threshold must allow neutral");
        for (const char* field : { "trigger_shoot_threshold", "trigger_jump_threshold",
            "movement_deadzone", "menu_engage_threshold", "menu_release_threshold" })
        {
            for (const char* value : { "-0.01", "1.01", "1e40" })
            {
                bool rejected = false;
                try { variant.load(field, value); }
                catch (const std::exception&) { rejected = true; }
                require(rejected, "Out-of-range input threshold must be rejected");
            }
        }
        for (const auto& invalid : { std::pair{ "movement_deadzone", "1" },
            std::pair{ "menu_release_threshold", "0" },
            std::pair{ "menu_release_threshold", "0.8" },
            std::pair{ "menu_engage_threshold", "0.5" } })
        {
            bool rejected = false;
            try { variant.load(invalid.first, invalid.second); }
            catch (const std::exception&) { rejected = true; }
            require(rejected, "Unusable input ranges must be rejected");
        }

        GamepadReader reader;
        Gamepads pads(&reader);
        Keyboard keyboard;
        keyboard.set_focused(true);
        keyboard.poll();
        keyboard.poll();
        MenuInput input(&pads, &keyboard, &tuning);
        const auto keys = [&](float dt = 0.0f)
        {
            keyboard.poll();
            input.update_directions(dt, delay, interval);
            const ProcessedMenuInput first = input.menu_inputs()[input_slots::keyboard_and_mouse];
            require(first.direction == input.menu_inputs()[input_slots::keyboard_and_mouse].direction,
                "Repeated reads must not advance repeat time");
            return first;
        };
        keys();
        keyboard.on_key_down(Key::s);
        require(keys().direction == Direction::down, "WASD must navigate on press");
        require(keys(delay + interval).direction == Direction::down, "Held keyboard must repeat");
        input.reset_directions();
        require(keys(10).direction == Direction::none, "Page change must block held keys");
        keyboard.on_key_up(Key::s);
        require(keys().direction == Direction::none, "Releasing across a page change must not move");
        keyboard.on_key_down(Key::down);
        require(keys().direction == Direction::down, "Arrow keys must rearm after neutral");
        keyboard.set_focused(false);
        require(keys().direction == Direction::none, "Focus loss must clear navigation");
        keyboard.set_focused(true);
        keyboard.on_key_down(Key::down);
        require(keys(10).direction == Direction::none, "Focus return must suppress held keys");
        require(keys(10).direction == Direction::none, "Focus-held keys must not repeat later");
        keyboard.on_key_up(Key::down);
        keys();
        keyboard.on_key_down(Key::up);
        require(keys().direction == Direction::up, "Focus return must rearm through neutral");
        keyboard.on_key_down(Key::enter);
        require(keys().action == MenuInputAction::none, "Confirm remains release-triggered");
        keyboard.on_key_up(Key::enter);
        require(keys().action == MenuInputAction::proceed, "Navigation must preserve confirm ownership");
        const Direction before_reset = input.menu_inputs()[input_slots::keyboard_and_mouse].direction;
        input.reset_pad_actions(0);
        require(input.menu_inputs()[input_slots::keyboard_and_mouse].direction == before_reset,
            "Resetting one pad must preserve other device results");
        std::cout << "PASS menu navigation: press, repeat, hysteresis, page arming, focus, stable reads, owner routing and seat reclaim\n";
        return EXIT_SUCCESS;
    });
}
