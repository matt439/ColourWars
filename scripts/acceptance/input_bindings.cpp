#include "game/pch.h"
#include "game/input_bindings.h"
#include "game/input_slots.h"
#include "game/save.h"
#include "game/objects/level_end_info.h"
#include "game/objects/player_input.h"
#include "game/states/menu_input.h"
#include "game/states/resume_countdown.h"
#include "engine/input/gamepad_reader.h"
#include <objbase.h>
#include <rapidjson/document.h>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>

// The concrete reader link seam used by action_bindings.cpp: no XInput,
// window, graphics/audio device, physical keyboard or player's save.
namespace binding_test_pads { std::array<labrador::GamepadState, 4> states{}; }
namespace labrador
{
    struct GamepadReader::Impl { std::array<GamepadState, 4> states{}; bool suspended = false; };
    GamepadReader::GamepadReader() : impl_(std::make_unique<Impl>()) {}
    GamepadReader::~GamepadReader() = default;
    GamepadReader::GamepadReader(GamepadReader&&) noexcept = default;
    GamepadReader& GamepadReader::operator=(GamepadReader&&) noexcept = default;
    void GamepadReader::sample()
    { this->impl_->states = this->impl_->suspended ? std::array<GamepadState, 4>{} : binding_test_pads::states; }
    GamepadState GamepadReader::read(int slot) const
    { return slot >= 0 && slot < 4 ? this->impl_->states[static_cast<size_t>(slot)] : GamepadState{}; }
    void GamepadReader::suspend() { this->impl_->suspended = true; }
    void GamepadReader::resume() { this->impl_->suspended = false; }
}

using namespace labrador;

namespace
{
    void require(bool condition, const char* message)
    { if (!condition) { throw std::runtime_error(message); } }

    template<class Action> void rejects(Action action, const char* message)
    {
        bool rejected = false;
        try { action(); }
        catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, message);
    }

    struct ExpectedKey { Key key; const char* name; const wchar_t* label; };
    // Explicit physical names and labels, independent of enum arithmetic and
    // the production conversion table. Supported does not imply conflict-free.
    constexpr std::array<ExpectedKey, 27> expected_keys{{
        { Key::a, "a", L"A" }, { Key::b, "b", L"B" }, { Key::c, "c", L"C" },
        { Key::d, "d", L"D" }, { Key::e, "e", L"E" }, { Key::f, "f", L"F" },
        { Key::g, "g", L"G" }, { Key::h, "h", L"H" }, { Key::i, "i", L"I" },
        { Key::j, "j", L"J" }, { Key::k, "k", L"K" }, { Key::l, "l", L"L" },
        { Key::m, "m", L"M" }, { Key::n, "n", L"N" }, { Key::o, "o", L"O" },
        { Key::p, "p", L"P" }, { Key::q, "q", L"Q" }, { Key::r, "r", L"R" },
        { Key::s, "s", L"S" }, { Key::t, "t", L"T" }, { Key::u, "u", L"U" },
        { Key::v, "v", L"V" }, { Key::w, "w", L"W" }, { Key::x, "x", L"X" },
        { Key::y, "y", L"Y" }, { Key::z, "z", L"Z" }, { Key::space, "space", L"Space" }
    }};

    bool conflicting(Key key) { return key == Key::a || key == Key::d || key == Key::s; }

    std::vector<Key> expected_game_keys(GameAction action, Key jump)
    {
        switch (action)
        {
        case GameAction::move_left: return { Key::a };
        case GameAction::move_right: return { Key::d };
        case GameAction::jump: return { jump };
        case GameAction::drop: return { Key::s, Key::down };
        case GameAction::pause: return { Key::escape };
        case GameAction::debug: return { Key::f1 };
        case GameAction::skip_intro: return { Key::space, Key::escape, Key::enter };
        default: return {};
        }
    }

    void require_mapping(const ActionBindings& actual, Key jump)
    {
        require(game_action_count == 8 && menu_action_count == 4 && menu_direction_count == 4,
            "Input binding test surface changed without its explicit physical-control oracle");
        for (size_t index = 0; index < game_action_count; ++index)
        {
            const GameAction action = static_cast<GameAction>(index);
            require(actual.game_desktop[index].keys == expected_game_keys(action, jump) &&
                actual.game_desktop[index].mouse_buttons == (action == GameAction::shoot ?
                    std::vector<MouseButton>{ MouseButton::left } : std::vector<MouseButton>{}),
                "Effective bindings changed another gameplay keyboard/mouse action");
            std::vector<GamepadButton> buttons;
            if (action == GameAction::jump) { buttons = { GamepadButton::a }; }
            if (action == GameAction::pause) { buttons = { GamepadButton::start }; }
            if (action == GameAction::debug) { buttons = { GamepadButton::back }; }
            if (action == GameAction::skip_intro) { buttons = { GamepadButton::a, GamepadButton::b, GamepadButton::start }; }
            require(actual.game_pad[index].buttons == buttons, "Keyboard Jump changed a fixed pad button alias");
            const auto& triggers = actual.game_pad[index].triggers;
            if (action == GameAction::jump || action == GameAction::shoot)
            {
                const bool jumping = action == GameAction::jump;
                require(triggers.size() == 1 && triggers[0].trigger == (jumping ? GamepadTrigger::left : GamepadTrigger::right) &&
                    triggers[0].threshold == (jumping ? ActionTriggerThreshold::jump : ActionTriggerThreshold::shoot),
                    "Keyboard Jump changed a fixed pad trigger or its tuning selector");
            }
            else { require(triggers.empty(), "Keyboard Jump added a pad trigger to another action"); }
        }
        require(actual.menu_keys[static_cast<size_t>(MenuInputAction::proceed)] == std::vector<Key>{ Key::enter, Key::space } &&
            actual.menu_keys[static_cast<size_t>(MenuInputAction::back)] == std::vector<Key>{ Key::escape, Key::backspace } &&
            actual.menu_keys[static_cast<size_t>(MenuInputAction::pause)].empty() &&
            actual.menu_keys[static_cast<size_t>(MenuInputAction::shuffle)] == std::vector<Key>{ Key::r },
            "Keyboard Jump changed fixed menu keyboard aliases");
        const std::array<GamepadButton, 4> menu_buttons{ GamepadButton::a, GamepadButton::b,
            GamepadButton::start, GamepadButton::x };
        for (size_t index = 0; index < menu_buttons.size(); ++index)
        { require(actual.menu_buttons[index] == std::vector<GamepadButton>{ menu_buttons[index] },
            "Keyboard Jump changed a fixed menu pad alias"); }
        const std::array<Direction, 4> directions{ Direction::up, Direction::down, Direction::left, Direction::right };
        const std::array<std::vector<Key>, 4> keys{ std::vector<Key>{ Key::up, Key::w }, { Key::down, Key::s },
            { Key::left, Key::a }, { Key::right, Key::d } };
        const std::array<GamepadButton, 4> dpad{ GamepadButton::dpad_up, GamepadButton::dpad_down,
            GamepadButton::dpad_left, GamepadButton::dpad_right };
        for (size_t index = 0; index < directions.size(); ++index)
        { require(actual.directions[index].direction == directions[index] && actual.directions[index].keys == keys[index] &&
            actual.directions[index].button == dpad[index], "Keyboard Jump changed a fixed menu navigation alias"); }
    }

    KeyboardState keys(std::initializer_list<Key> pressed)
    {
        KeyboardState result; result.focused = true;
        for (Key key : pressed)
        {
            const auto index = static_cast<unsigned int>(key);
            result.keys[index / 64] |= uint64_t(1) << (index % 64);
        }
        return result;
    }

    void check_domain_and_mapping(const InputTuning& tuning)
    {
        require(InputBindings{}.keyboard_jump == Key::space, "New input preferences do not default to Space");
        InputBindings{}.validate(); require_mapping(default_action_bindings(), Key::space);
        for (const auto& expected : expected_keys)
        {
            require(keyboard_jump_key_supported(expected.key) && input_key_name(expected.key) == expected.name &&
                input_key_label(expected.key) == expected.label && input_key_from_name(expected.name) == expected.key,
                "Physical Jump key name/label/domain differs from its explicit 27-entry contract");
            require(input_key_name(expected.key) == labrador::key_name(expected.key) &&
                labrador::key_from_name(expected.name) == expected.key,
                "A saved Jump token no longer matches the engine's key_name spelling");
            const auto conflict = keyboard_jump_conflict(expected.key);
            require(conflict.has_value() == conflicting(expected.key), "Jump conflict set is not exactly A/D/S within the supported domain");
            if (conflict)
            {
                require(*conflict == (expected.key == Key::a ? GameAction::move_left :
                    (expected.key == Key::d ? GameAction::move_right : GameAction::drop)), "Jump conflict names the wrong gameplay action");
                rejects([&] { InputBindings{ expected.key }.validate(); }, "Conflicting Jump preference validated");
                rejects([&] { effective_action_bindings(InputBindings{ expected.key }); }, "Conflicting Jump mapping was constructed");
                continue;
            }
            const InputBindings preferences{ expected.key }; preferences.validate();
            const ActionBindings effective = effective_action_bindings(preferences);
            require_mapping(effective, expected.key); require_mapping(default_action_bindings(), Key::space);
            const auto down = game_action(effective, GameAction::jump, keys({ expected.key }), keys({}), {}, {});
            const auto held = game_action(effective, GameAction::jump, keys({ expected.key }), keys({ expected.key }), {}, {});
            const auto up = game_action(effective, GameAction::jump, keys({}), keys({ expected.key }), {}, {});
            require(down.held && down.pressed && !down.released && !down.was_held && held.held && !held.pressed &&
                held.was_held && !up.held && !up.pressed && up.released && up.was_held,
                "Effective Jump did not use ordinary physical-key held/press/release samples");
            require(game_action(effective, GameAction::jump, keys({ Key::space }), keys({}), {}, {}).held ==
                (expected.key == Key::space), "Effective Jump retained Space as an extra gameplay alias");
            for (Key intro : { Key::space, Key::escape, Key::enter })
            { require(game_action(effective, GameAction::skip_intro, keys({}), keys({ intro }), {}, {}).released,
                "Rebinding Jump changed a fixed introductory skip release"); }
            if (expected.key != Key::space)
            { require(!game_action(effective, GameAction::skip_intro, keys({}), keys({ expected.key }), {}, {}).released,
                "A new Jump key became an introductory skip alias"); }
            GamepadState neutral; neutral.connected = true;
            GamepadState pad = neutral; pad.buttons = uint16_t(1u << static_cast<unsigned int>(GamepadButton::a));
            require(game_action(effective, GameAction::jump, pad, neutral, tuning).pressed,
                "Rebinding keyboard Jump changed pad A");
            pad = neutral; pad.left_trigger = 1.0f;
            const auto trigger = game_action(effective, GameAction::jump, pad, neutral, tuning);
            require(trigger.held == (tuning.trigger_jump_threshold < 1.0f) &&
                trigger.pressed == (tuning.trigger_jump_threshold < 1.0f),
                "Rebinding keyboard Jump changed its pad left trigger or strict threshold");
        }
        for (unsigned int value = 0; value <= static_cast<unsigned int>(Key::count); ++value)
        {
            const Key key = static_cast<Key>(value);
            const bool supported = std::any_of(expected_keys.begin(), expected_keys.end(),
                [key](const ExpectedKey& expected) { return expected.key == key; });
            require(keyboard_jump_key_supported(key) == supported, "An unlisted engine key entered the bounded Jump domain");
            if (!supported)
            {
                rejects([&] { input_key_name(key); }, "Out-of-domain key acquired a save name");
                rejects([&] { input_key_label(key); }, "Out-of-domain key acquired a help label");
                rejects([&] { InputBindings{ key }.validate(); }, "Out-of-domain Jump preference validated");
            }
        }
        rejects([] { InputBindings{ static_cast<Key>(255) }.validate(); }, "Invalid key enumerator validated");
        for (std::string_view name : { "", "J", "Space", "SPACE", "space ", " space", "enter", "escape", "grave", "f1", "1", "jump", "left" })
        { rejects([&] { input_key_from_name(name); }, "Noncanonical or unknown key name was accepted"); }
        rejects([] { input_key_from_name(std::string_view("j\0alias", 7)); }, "Embedded-null key name was accepted");
        require(!keyboard_jump_conflict(Key::w) && !keyboard_jump_conflict(Key::r) &&
            menu_action(effective_action_bindings(InputBindings{ Key::r }), MenuInputAction::shuffle, keys({}), keys({ Key::r })).released &&
            keyboard_direction_held(effective_action_bindings(InputBindings{ Key::w }), keys({ Key::w }))[0],
            "Menu-only W/R aliases incorrectly prevented Jump binding or stopped working");
    }

    bool same_input(const PlayerInputData& a, const PlayerInputData& b)
    {
        return a.connected == b.connected && a.x_movement == b.x_movement && a.jump_pressed == b.jump_pressed &&
            a.jump_held == b.jump_held && a.shoot_angle == b.shoot_angle && a.shoot_direction == b.shoot_direction &&
            a.shoot_direction_requested == b.shoot_direction_requested && a.primary_shoot == b.primary_shoot &&
            a.drop_through == b.drop_through && a.toggle_debug == b.toggle_debug &&
            a.toggle_pause_menu == b.toggle_pause_menu && a.pause_held == b.pause_held && a.skip_intro == b.skip_intro &&
            a.left_analog_stick == b.left_analog_stick && a.right_analog_stick == b.right_analog_stick;
    }

    struct ReaderHarness
    {
        GamepadReader reader; Gamepads pads{ &reader }; Keyboard keyboard; Mouse mouse;
        GameTuning tuning; PlaySettings play; InputBindings bindings{ Key::j };
        PlayerInput player; MenuInput menu; std::vector<ProcessedMenuInput> menus;
        explicit ReaderHarness(const char* content) : tuning((std::filesystem::path(content) / "tuning.json").string().c_str()),
            player(&pads, &keyboard, &mouse, &tuning, &play, &bindings), menu(&pads, &keyboard, &tuning)
        {
            binding_test_pads::states = {}; keyboard.set_focused(true); mouse.set_focused(true);
            std::ignore = this->sample(); std::ignore = this->sample();
        }
        std::vector<PlayerInputData> sample()
        {
            pads.poll(); keyboard.poll(); mouse.poll(); menu.update_directions(0.0f, 0.4f, 0.1f, false);
            auto result = player.player_inputs(); const auto repeated = player.player_inputs();
            require(result.size() == input_slots::count && repeated.size() == result.size(), "Actual input reader changed slot count");
            for (size_t slot = 0; slot < result.size(); ++slot)
            { require(same_input(result[slot], repeated[slot]), "Repeated reader access consumed or changed a sampled action edge"); }
            menus = menu.menu_inputs(); return result;
        }
        PlayerInputData desktop() { return this->sample()[input_slots::keyboard_and_mouse]; }
    };

    void check_actual_reader(const char* content)
    {
        ReaderHarness h(content);
        rejects([&] { PlayerInput invalid(&h.pads, &h.keyboard, &h.mouse, &h.tuning, &h.play, nullptr); },
            "PlayerInput accepted missing binding ownership");
        for (Key key : { Key::none, Key::a, Key::escape, static_cast<Key>(255) })
        {
            const InputBindings invalid{ key };
            rejects([&] { PlayerInput reader(&h.pads, &h.keyboard, &h.mouse, &h.tuning, &h.play, &invalid); },
                "PlayerInput constructed an invalid effective mapping");
        }
        h.keyboard.on_key_down(Key::space); auto input = h.desktop();
        require(!input.jump_pressed && !input.jump_held, "Actual custom Jump retained the old Space gameplay source");
        h.keyboard.on_key_up(Key::space); input = h.desktop();
        require(input.skip_intro && h.menus[input_slots::keyboard_and_mouse].action == MenuInputAction::proceed,
            "Space lost its independent menu/intro aliases after Jump changed");
        h.keyboard.on_key_down(Key::j); input = h.desktop();
        require(input.jump_held && input.jump_pressed && !input.skip_intro, "Actual custom Jump did not produce a fresh J press");
        h.keyboard.on_key_down(Key::j); input = h.desktop();
        require(input.jump_held && !input.jump_pressed, "Repeated key-down produced a second gameplay Jump");
        h.keyboard.on_key_up(Key::j); input = h.desktop();
        require(!input.jump_held && !input.jump_pressed && !input.skip_intro, "Releasing custom Jump skipped the intro or retained Jump");
        for (Key key : { Key::enter, Key::escape })
        {
            h.keyboard.on_key_down(key); std::ignore = h.desktop(); h.keyboard.on_key_up(key); input = h.desktop();
            require(input.skip_intro && input.toggle_pause_menu == (key == Key::escape),
                "Actual reader changed fixed Enter/Escape release behavior");
        }
        h.keyboard.on_key_down(Key::a); h.keyboard.on_key_down(Key::s); h.mouse.on_button_down(MouseButton::left);
        input = h.desktop(); require(input.x_movement == -1.0f && input.drop_through && input.primary_shoot,
            "Rebinding Jump changed movement, drop or mouse fire");
        h.keyboard.on_key_up(Key::a); h.keyboard.on_key_up(Key::s); h.mouse.on_button_up(MouseButton::left); std::ignore = h.desktop();
        h.keyboard.on_key_down(Key::j); std::ignore = h.desktop(); h.keyboard.set_focused(false); input = h.desktop();
        require(!input.jump_held && !input.jump_pressed && !input.skip_intro, "Focus loss leaked Jump or an intro release");
        h.keyboard.set_focused(true); h.keyboard.on_key_down(Key::j); input = h.desktop();
        require(input.jump_held && !input.jump_pressed, "Focus return invented a custom Jump press");
        input = h.desktop(); require(input.jump_held && !input.jump_pressed, "Focus-held custom Jump repeated its edge");
        h.keyboard.on_key_up(Key::j); std::ignore = h.desktop(); h.keyboard.on_key_down(Key::j); input = h.desktop();
        require(input.jump_pressed, "Release and fresh press did not rearm custom Jump after focus return");
        h.keyboard.on_key_up(Key::j); std::ignore = h.desktop();
        h.keyboard.on_key_down(Key::k); std::ignore = h.desktop(); std::ignore = h.desktop();
        h.bindings.keyboard_jump = Key::k; input = h.desktop();
        require(input.jump_held && !input.jump_pressed, "Changing to an already held key invented a Jump press");
        h.keyboard.on_key_up(Key::k); std::ignore = h.desktop(); h.keyboard.on_key_down(Key::k); input = h.desktop();
        require(input.jump_pressed, "Changed mapping did not accept a deliberate new key press");
        h.keyboard.on_key_up(Key::k); std::ignore = h.desktop();
        h.bindings.keyboard_jump = Key::r; std::ignore = h.desktop(); h.keyboard.on_key_down(Key::r); input = h.desktop();
        require(input.jump_pressed, "Menu shuffle key could not be used for gameplay Jump");
        h.keyboard.on_key_up(Key::r); std::ignore = h.desktop();
        require(h.menus[input_slots::keyboard_and_mouse].action == MenuInputAction::shuffle,
            "Gameplay R binding rewrote the actual menu shuffle alias");
        h.bindings.keyboard_jump = Key::w; std::ignore = h.desktop(); h.keyboard.on_key_down(Key::w); input = h.desktop();
        require(input.jump_pressed && h.menus[input_slots::keyboard_and_mouse].direction == Direction::up,
            "Gameplay W binding rewrote the actual menu direction alias");
        h.keyboard.on_key_up(Key::w); std::ignore = h.desktop();
        auto& pad = binding_test_pads::states[2]; pad.connected = true;
        pad.buttons = uint16_t(1u << static_cast<unsigned int>(GamepadButton::a));
        auto all = h.sample(); require(all[2].jump_held && !all[2].jump_pressed && !all[0].connected,
            "Custom keyboard mapping changed reconnect guarding or sparse pad identity");
        pad.buttons = 0; std::ignore = h.sample(); pad.buttons = uint16_t(1u << static_cast<unsigned int>(GamepadButton::a));
        all = h.sample(); require(all[2].jump_pressed, "Custom keyboard mapping changed actual pad A");
        pad.left_trigger = 1.0f;
        all = h.sample(); require(all[2].jump_held && all[2].jump_pressed == (h.tuning.input().trigger_jump_threshold < 1.0f),
            "Held pad A suppressed the independent left-trigger Jump edge or changed its strict threshold");
        pad = {}; std::ignore = h.sample();
        h.bindings.keyboard_jump = Key::none;
        rejects([&] { h.player.player_inputs(); }, "Borrowed invalid preference silently replaced the cached mapping");
        h.bindings.keyboard_jump = Key::j; std::ignore = h.desktop();
        require_mapping(default_action_bindings(), Key::space);
    }

    void check_actual_resume(const char* content)
    {
        ReaderHarness h(content); ResumeCountdown countdown; countdown.begin(0.5f);
        h.keyboard.on_key_down(Key::j); auto inputs = h.sample();
        require(inputs[input_slots::keyboard_and_mouse].jump_pressed, "Resume fixture did not sample a real J press");
        countdown.filter_inputs(inputs);
        require(inputs[input_slots::keyboard_and_mouse].jump_held && !inputs[input_slots::keyboard_and_mouse].jump_pressed,
            "Resume countdown failed to suppress mapped Jump while retaining held arc input");
        inputs = h.sample(); countdown.filter_inputs(inputs);
        require(countdown.advance(0.5f), "Actual mapped held-J final sample did not complete the countdown");
        for (int tick = 0; tick < 3; ++tick)
        {
            inputs = h.sample(); countdown.filter_inputs(inputs);
            require(inputs[input_slots::keyboard_and_mouse].jump_held && !inputs[input_slots::keyboard_and_mouse].jump_pressed,
                "Held custom Jump leaked across countdown completion");
        }
        h.keyboard.on_key_up(Key::j); inputs = h.sample(); countdown.filter_inputs(inputs);
        require(!inputs[input_slots::keyboard_and_mouse].jump_held && !inputs[input_slots::keyboard_and_mouse].jump_pressed,
            "Custom Jump release was not neutral through ResumeCountdown");
        h.keyboard.on_key_down(Key::j); inputs = h.sample(); countdown.filter_inputs(inputs);
        require(inputs[input_slots::keyboard_and_mouse].jump_pressed && inputs[input_slots::keyboard_and_mouse].jump_held,
            "Fresh custom Jump remained blocked after a release beyond countdown completion");
    }

    class IsolatedSaveDirectory
    {
    public:
        explicit IsolatedSaveDirectory(const std::filesystem::path& retained_root = {}) : retained_(!retained_root.empty())
        {
            wchar_t* previous = nullptr; size_t length = 0;
            require(_wdupenv_s(&previous, &length, L"LOCALAPPDATA") == 0, "Cannot read LOCALAPPDATA");
            if (previous) { previous_ = previous; std::free(previous); }
            if (retained_)
            {
                root_ = std::filesystem::absolute(retained_root);
                std::filesystem::create_directories(root_ / L"ColourWars");
            }
            else
            {
                GUID guid{}; wchar_t name[40]{};
                require(SUCCEEDED(CoCreateGuid(&guid)) && StringFromGUID2(guid, name, 40) > 0, "Cannot name input-binding acceptance profile");
                root_ = std::filesystem::temp_directory_path() / (std::wstring(L"ColourWars-input-bindings-") + name);
                require(std::filesystem::create_directory(root_) && std::filesystem::create_directory(root_ / L"ColourWars"),
                    "Cannot create input-binding acceptance profile");
            }
            require(_wputenv_s(L"LOCALAPPDATA", root_.c_str()) == 0, "Cannot isolate input-binding saves");
        }
        ~IsolatedSaveDirectory()
        {
            _wputenv_s(L"LOCALAPPDATA", previous_.c_str());
            if (retained_) { return; } // Explicit relaunch profiles belong to the caller across processes.
            std::error_code ignored;
            std::filesystem::remove(temporary_path(), ignored); std::filesystem::remove(save_path(), ignored);
            std::filesystem::remove(root_ / L"ColourWars", ignored); std::filesystem::remove(root_, ignored);
        }
        std::filesystem::path save_path() const { return root_ / L"ColourWars/save_data.json"; }
        std::filesystem::path temporary_path() const { return save_path().wstring() + L".tmp"; }
        void write(const std::string& json) const
        {
            std::ofstream stream(save_path(), std::ios::binary | std::ios::trunc); stream << json;
            require(stream.good(), "Cannot write private input-binding fixture");
        }
        std::string contents() const
        {
            std::ifstream stream(save_path(), std::ios::binary); require(stream.good(), "Cannot read private input-binding save");
            return { std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
        }
    private:
        std::filesystem::path root_; std::wstring previous_; bool retained_ = false;
    };

    const std::string independent_sections =
        "\"resolution\":{\"x\":1280,\"y\":720},\"fullscreen\":true,\"team_palette\":\"pink_green\","
        "\"aim_sensitivity\":4,\"music_volume\":23,\"sfx_volume\":67,\"view_shake\":false,\"aim_marker\":false,\"reduce_effects\":true,"
        "\"pad_sticks\":[{\"movement_deadzone\":0,\"aim_deadzone\":90},{\"movement_deadzone\":10,\"aim_deadzone\":35},"
        "{\"movement_deadzone\":25,\"aim_deadzone\":55},{\"movement_deadzone\":-1,\"aim_deadzone\":-1}],"
        "\"records\":[{\"stage\":\"retired\",\"versus_played\":7,\"team_a_wins\":4,\"team_b_wins\":2,\"draws\":1,"
        "\"best_win_percentage\":75,\"practice_played\":3,\"best_coverage_percentage\":37.5}],"
        "\"last_match\":{\"players\":[{\"team\":\"b\",\"weapon\":\"sniper\"}],\"stage\":\"movement_course\","
        "\"layout\":\"one_player\",\"best_of\":1,\"duration\":45.25}";

    std::string fixture(const std::string& bindings = "", const std::string& version = "3")
    { return "{" + (version.empty() ? "" : "\"save_version\":" + version + ",") + independent_sections +
        (bindings.empty() ? "" : ",\"input_bindings\":" + bindings) + "}"; }

    Save load(const IsolatedSaveDirectory& directory, const std::string& bindings = "", const std::string& version = "3")
    { directory.write(fixture(bindings, version)); Save result; result.load_save_file(); return result; }

    void require_independent(const Save& save)
    {
        const auto& play = save.play_settings(); const StageRecord* record = save.records().find("retired");
        require(save.resolution() == ScreenResolution::s_1280_720 && save.fullscreen() && save.team_palette() == TeamColourPair::pink_green &&
            play.aim_sensitivity == 4 && play.music_volume == 23 && play.sfx_volume == 67 && !play.view_shake && !play.aim_marker && play.reduce_effects,
            "Input-binding save/recovery changed independent display or play preferences");
        const std::array<StickSettings, 4> profiles{ StickSettings{0, 90}, StickSettings{10, 35}, StickSettings{25, 55}, StickSettings{-1, -1} };
        for (size_t slot = 0; slot < profiles.size(); ++slot)
        { require(play.sticks[slot].movement_deadzone == profiles[slot].movement_deadzone && play.sticks[slot].aim_deadzone == profiles[slot].aim_deadzone,
            "Input-binding save/recovery changed an independent per-pad profile"); }
        require(save.records().stages().size() == 1 && record && record->versus_played == 7 && record->team_a_wins == 4 &&
            record->team_b_wins == 2 && record->draws == 1 && record->best_win_percentage == 75.0f &&
            record->practice_played == 3 && record->best_coverage_percentage == 37.5f,
            "Input-binding save/recovery lost an independent removed-stage record");
        require(save.last_match_setup() && save.last_match_setup()->players.size() == 1 &&
            save.last_match_setup()->players[0].team == PlayerTeam::b && save.last_match_setup()->players[0].weapon == WeaponType::sniper &&
            save.last_match_setup()->stage == "movement_course" && save.last_match_setup()->layout == ScreenLayout::one_player &&
            save.last_match_setup()->best_of == 1 && save.last_match_setup()->duration == 45.25f,
            "Input-binding save/recovery changed independent remembered match choices");
    }

    void require_schema(const std::string& bytes, const char* name)
    {
        rapidjson::Document document; document.Parse(bytes.data(), bytes.size());
        require(!document.HasParseError() && document.IsObject() && document.HasMember("save_version") &&
            document["save_version"].IsInt() && document["save_version"].GetInt() == 3 &&
            document.HasMember("input_bindings") && document["input_bindings"].IsObject(),
            "Save did not write the explicit version-3 independent binding schema");
        const auto& section = document["input_bindings"];
        require(section.MemberCount() == 1 && section.HasMember("keyboard_jump") && section["keyboard_jump"].IsString() &&
            std::string_view(section["keyboard_jump"].GetString(), section["keyboard_jump"].GetStringLength()) == name,
            "Save binding section did not contain exactly its canonical physical Jump token");
    }

    void check_save_schema_and_recovery(const IsolatedSaveDirectory& directory)
    {
        require(Save::version == 3, "Input binding schema must be save version 3");
        for (const char* version : { "", "0", "1", "2" })
        {
            Save legacy = load(directory, "", version);
            require(legacy.input_bindings().keyboard_jump == Key::space, "Legacy save without bindings did not acquire default Space");
            require_independent(legacy); require(legacy.save_to_file(), "Legacy binding migration could not save");
            require_schema(directory.contents(), "space"); Save reread; reread.load_save_file(); require_independent(reread);
            require(reread.input_bindings().keyboard_jump == Key::space, "Migrated legacy binding did not round trip");
        }
        for (const char* version : { "0", "1", "2", "3", "999" })
        {
            Save known = load(directory, "{\"keyboard_jump\":\"j\"}", version);
            require(known.input_bindings().keyboard_jump == Key::j, "Versioned save discarded its known independent binding section");
            require_independent(known);
        }
        Save save = load(directory, "{\"keyboard_jump\":\"j\"}");
        for (const auto& expected : expected_keys)
        {
            if (conflicting(expected.key)) { continue; }
            require(save.set_input_bindings_and_save(InputBindings{ expected.key }), "A valid physical Jump preference did not save");
            require(save.input_bindings().keyboard_jump == expected.key && save.save_data().input_bindings.keyboard_jump == expected.key,
                "Successful binding setter did not update both borrowed preference and SaveData copy");
            require_schema(directory.contents(), expected.name); Save reread; reread.load_save_file();
            require(reread.input_bindings().keyboard_jump == expected.key, "Canonical Jump preference did not round trip");
            require_independent(reread);
        }
        for (const char* broken : { "null", "false", "7", "\"j\"", "[]", "{}", "{\"jump\":\"j\"}",
            "{\"keyboard_jump\":null}", "{\"keyboard_jump\":false}", "{\"keyboard_jump\":7}", "{\"keyboard_jump\":[]}",
            "{\"keyboard_jump\":{}}", "{\"keyboard_jump\":\"unknown\"}", "{\"keyboard_jump\":\"J\"}",
            "{\"keyboard_jump\":\"Space\"}", "{\"keyboard_jump\":\"enter\"}", "{\"keyboard_jump\":\"grave\"}",
            "{\"keyboard_jump\":\"a\"}", "{\"keyboard_jump\":\"d\"}", "{\"keyboard_jump\":\"s\"}",
            "{\"keyboard_jump\":\"j\\u0000alias\"}" })
        {
            directory.write(fixture("{\"keyboard_jump\":\"j\"}")); save.load_save_file();
            directory.write(fixture(broken)); const std::string bytes = directory.contents(); save.load_save_file();
            require(save.input_bindings().keyboard_jump == Key::space && directory.contents() == bytes,
                "Malformed binding section retained old bindings or rewrote the player's bytes on load");
            require_independent(save);
        }
        directory.write("{\"save_version\":3,\"records\":false,\"resolution\":false,\"aim_sensitivity\":\"bad\","
            "\"last_match\":false,\"input_bindings\":{\"keyboard_jump\":\"j\"}}");
        save.load_save_file(); require(save.input_bindings().keyboard_jump == Key::j,
            "An unrelated malformed save section reset valid input bindings");
    }

    class ReplacementLock
    {
    public:
        explicit ReplacementLock(const std::filesystem::path& path)
        {
            handle_ = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            require(handle_ != INVALID_HANDLE_VALUE, "Cannot lock private binding save replacement");
        }
        ~ReplacementLock() { CloseHandle(handle_); }
    private:
        HANDLE handle_ = INVALID_HANDLE_VALUE;
    };

    void check_save_validation_failures_and_independence(const IsolatedSaveDirectory& directory, const char* content)
    {
        Save save = load(directory, "{\"keyboard_jump\":\"j\"}");
        const InputBindings* borrowed = &save.input_bindings();
        const std::string original = directory.contents();
        for (Key key : { Key::a, Key::d, Key::s, Key::none, Key::enter, Key::grave, static_cast<Key>(255) })
        {
            rejects([&] { save.set_input_bindings_and_save(InputBindings{ key }); }, "Invalid binding setter did not validate before assignment/write");
            require(&save.input_bindings() == borrowed && borrowed->keyboard_jump == Key::j && directory.contents() == original &&
                !std::filesystem::exists(directory.temporary_path()), "Rejected binding setter changed active memory, disk bytes or staging state");
            require_independent(save);
        }
        for (bool replacement_failure : { false, true })
        {
            save = load(directory, "{\"keyboard_jump\":\"j\"}"); const std::string bytes = directory.contents();
            ReaderHarness h(content);
            PlayerInput saved_reader(&h.pads, &h.keyboard, &h.mouse, &h.tuning, &h.play, borrowed);
            if (replacement_failure)
            {
                const ReplacementLock lock(directory.save_path());
                require(!save.set_input_bindings_and_save(InputBindings{ Key::k }), "Locked binding save replacement unexpectedly succeeded");
            }
            else
            {
                require(std::filesystem::create_directory(directory.temporary_path()), "Cannot block own binding staging path");
                require(!save.set_input_bindings_and_save(InputBindings{ Key::k }), "Blocked binding staging write unexpectedly succeeded");
                require(std::filesystem::remove(directory.temporary_path()), "Cannot remove own binding staging blocker");
            }
            require(&save.input_bindings() == borrowed && borrowed->keyboard_jump == Key::k && directory.contents() == bytes &&
                !std::filesystem::exists(directory.temporary_path()), "Failed binding persistence rolled back active choice, damaged old bytes or left a temporary file");
            // This reader cached J before the failed write. It borrows the
            // Save member that GameLevel borrows, rather than a copied test preference.
            h.keyboard.on_key_down(Key::j); std::ignore = h.sample();
            auto actual = saved_reader.player_inputs()[input_slots::keyboard_and_mouse];
            require(!actual.jump_held && !actual.jump_pressed, "Failed persistence left the actual saved reader on old J");
            h.keyboard.on_key_up(Key::j); std::ignore = h.sample();
            h.keyboard.on_key_down(Key::k); std::ignore = h.sample();
            actual = saved_reader.player_inputs()[input_slots::keyboard_and_mouse];
            require(actual.jump_held && actual.jump_pressed &&
                same_input(actual, saved_reader.player_inputs()[input_slots::keyboard_and_mouse]),
                "Actual Save-borrowing reader did not follow active K after persistence failed");
            h.keyboard.on_key_up(Key::k); std::ignore = h.sample();
            require_independent(save); Save old_disk; old_disk.load_save_file();
            require(old_disk.input_bindings().keyboard_jump == Key::j, "Failed binding write damaged the reloadable previous preference");
            require_independent(old_disk);
            require(save.set_input_bindings_and_save(InputBindings{ Key::k }) && &save.input_bindings() == borrowed,
                "Targeted retry of the already active binding did not write after removing its real IO blocker");
            Save retried; retried.load_save_file(); require(retried.input_bindings().keyboard_jump == Key::k,
                "Retry did not persist the binding kept active after failure");
            require_schema(directory.contents(), "k"); require_independent(retried);
        }
        save = load(directory, "{\"keyboard_jump\":\"j\"}");
        auto settings_copy = save.play_settings(); auto controls_copy = save.play_settings();
        require(save.set_input_bindings_and_save(InputBindings{ Key::w }), "Cannot prepare independent saved binding");
        settings_copy.music_volume = 81; settings_copy.reduce_effects = false;
        require(save.set_play_settings_and_save(settings_copy) && save.input_bindings().keyboard_jump == Key::w,
            "An older Settings PlaySettings snapshot overwrote independent input bindings");
        controls_copy.aim_marker = true; controls_copy.sticks[2] = { 35, 75 };
        require(save.set_play_settings_and_save(controls_copy) && save.input_bindings().keyboard_jump == Key::w &&
            save.set_display_settings_and_save(ScreenResolution::s_3840_2160, false, TeamColourPair::blue_orange),
            "An older Controls snapshot or display setter overwrote independent input bindings");
        LevelEndInfo result; result.total_area = 200.0f; result.team_a_area = 150.0f; result.team_b_area = 50.0f;
        require(save.record_result_and_save("new_stage", result).recorded, "Cannot exercise independent record persistence");
        const LastMatchSetup remembered = *save.last_match_setup();
        require(save.remember_match_setup_and_save(remembered), "Cannot exercise independent remembered-match persistence");
        Save reread; reread.load_save_file();
        require(&save.input_bindings() == borrowed && borrowed->keyboard_jump == Key::w &&
            reread.input_bindings().keyboard_jump == Key::w && reread.resolution() == ScreenResolution::s_3840_2160 &&
            !reread.fullscreen() && reread.team_palette() == TeamColourPair::blue_orange && reread.play_settings().aim_marker &&
            reread.play_settings().sticks[2].movement_deadzone == 35 && reread.play_settings().sticks[2].aim_deadzone == 75 &&
            reread.records().find("retired") && reread.records().find("new_stage") &&
            reread.records().find("new_stage")->team_a_wins == 1 && reread.last_match_setup() &&
            reread.last_match_setup()->stage == remembered.stage && reread.last_match_setup()->players[0].weapon == WeaponType::sniper,
            "Settings/Controls/display/records/remembered saves overwrote the independent binding or their expected data");
        require_schema(directory.contents(), "w");
    }

    void check_relaunch(const char* content, std::string_view mode, const std::filesystem::path& profile)
    {
        require(mode == "relaunch-write" || mode == "relaunch-read", "Unknown input-binding relaunch mode");
        const IsolatedSaveDirectory directory(profile);
        if (mode == "relaunch-write")
        {
            require(!std::filesystem::exists(directory.save_path()), "Relaunch writer requires a fresh private profile");
            directory.write(fixture()); Save save; save.load_save_file(); require_independent(save);
            require(save.input_bindings() == InputBindings{} && save.set_input_bindings_and_save(InputBindings{ Key::j }),
                "Relaunch writer did not persist J through the targeted Save setter");
            require_schema(directory.contents(), "j"); require_independent(save);
        }
        else
        {
            require(std::filesystem::is_regular_file(directory.save_path()), "Relaunch reader requires the writer's private save");
            const std::string before = directory.contents(); Save save; save.load_save_file(); require_independent(save);
            require(save.input_bindings() == InputBindings{ Key::j }, "Fresh-process Save lost the persisted J preference");
            ReaderHarness h(content);
            PlayerInput reader(&h.pads, &h.keyboard, &h.mouse, &h.tuning, &h.play, &save.input_bindings());
            h.keyboard.on_key_down(Key::j); std::ignore = h.sample();
            auto input = reader.player_inputs()[input_slots::keyboard_and_mouse];
            require(input.jump_pressed && input.jump_held, "Fresh-process actual reader did not use persisted J");
            h.keyboard.on_key_up(Key::j); std::ignore = h.sample();
            require(!reader.player_inputs()[input_slots::keyboard_and_mouse].skip_intro, "Persisted J became an intro skip alias");
            h.keyboard.on_key_down(Key::space); std::ignore = h.sample();
            input = reader.player_inputs()[input_slots::keyboard_and_mouse];
            require(!input.jump_pressed && !input.jump_held, "Fresh-process custom reader retained default Space Jump");
            h.keyboard.on_key_up(Key::space); std::ignore = h.sample();
            require(reader.player_inputs()[input_slots::keyboard_and_mouse].skip_intro &&
                h.menus[input_slots::keyboard_and_mouse].action == MenuInputAction::proceed && directory.contents() == before,
                "Fresh-process Space aliases or unchanged relaunch save bytes were lost");
            require_independent(save);
        }
        std::cout << "PASS input bindings " << mode << ": explicit retained private profile, actual Save/reader; no game window or physical input\n";
    }
}

int main(int argc, char** argv)
{
    try
    {
        if (argc == 4) { check_relaunch(argv[1], argv[2], std::filesystem::path(argv[3])); return EXIT_SUCCESS; }
        require(argc == 2, "Usage: input_bindings.exe <content> [relaunch-write|relaunch-read <private profile root>]");
        const GameTuning tuning((std::filesystem::path(argv[1]) / "tuning.json").string().c_str());
        check_domain_and_mapping(tuning.input()); check_actual_reader(argv[1]); check_actual_resume(argv[1]);
        const IsolatedSaveDirectory directory; check_save_schema_and_recovery(directory);
        check_save_validation_failures_and_independence(directory, argv[1]);
        std::cout << "PASS input bindings: independent 27-key names/labels/domain, A/D/S conflicts and W/R aliases, "
            "isolated effective Jump with fixed menu/intro/pad controls; real readers, repeated snapshots/focus/remapping, "
            "held-J ResumeCountdown release gating, version-0/1/2 migration/version-3/future known sections, malformed-section isolation, "
            "validation before mutation, real staged-write/replacement failures with active-before-write preference and retry, "
            "independent Settings/Controls/display/records/remembered saves; GUID-isolated profile, no window/device\n";
        return EXIT_SUCCESS;
    }
    catch (const std::exception& error)
    { std::cerr << "FAIL input bindings: " << error.what() << '\n'; return EXIT_FAILURE; }
}
