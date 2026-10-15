#include "game/pch.h"
#include "game/states/menu_input.h"
#include "game/presentation.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
#include <stdexcept>

using namespace labrador;

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

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
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Expected staged content directory");
        const Presentation look((std::filesystem::path(argv[1]) / "presentation.json").string().c_str());
        const GameTuning tuning((std::filesystem::path(argv[1]) / "tuning.json").string().c_str());
        const float delay = look.menu().repeat_delay;
        const float interval = look.menu().repeat_interval;
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
        std::cout << "PASS menu navigation: press, repeat, hysteresis, page arming, focus and stable reads\n";
        return EXIT_SUCCESS;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
