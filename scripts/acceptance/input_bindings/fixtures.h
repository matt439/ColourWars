#pragma once

#include "game/input_bindings.h"
#include "game/input_slots.h"
#include "game/save.h"
#include "game/settings_session.h"
#include "game/objects/player_input.h"
#include "game/states/menu_input.h"
#include "engine/input/gamepad_reader.h"
#include "scripts/acceptance/support/probe.h"
#include <objbase.h>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace binding_test_pads { extern std::array<labrador::GamepadState, 4> states; }

namespace input_bindings_acceptance
{
    using namespace labrador;

    using acceptance::require;

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
    inline constexpr std::array<ExpectedKey, 27> expected_keys{{
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

    struct ExpectedControl { PadControl control; const char* name; const wchar_t* label; };
    // Explicit tokens and labels in page order, with None first: independent of
    // the production table and of PadControl ordinals.
    inline constexpr std::array<ExpectedControl, 12> expected_controls{{
        { PadControl::none, "none", L"None" }, { PadControl::a, "a", L"A" }, { PadControl::b, "b", L"B" },
        { PadControl::x, "x", L"X" }, { PadControl::y, "y", L"Y" },
        { PadControl::left_shoulder, "lb", L"LB" }, { PadControl::right_shoulder, "rb", L"RB" },
        { PadControl::left_trigger, "lt", L"LT" }, { PadControl::right_trigger, "rt", L"RT" },
        { PadControl::left_stick, "ls", L"LS" }, { PadControl::right_stick, "rs", L"RS" },
        { PadControl::start, "start", L"Start" }
    }};
    inline constexpr std::array<PadAction, 3> pad_actions{ PadAction::jump, PadAction::shoot, PadAction::pause };

    inline const std::array<PadProfile, input_slots::pad_count> default_pads{};

    uint16_t button_bit(GamepadButton button);
    GamepadState pressing(PadControl control, GamepadState state = {});
    PadProfile profile(PadControl jump, PadControl jump_also, PadControl shoot, PadControl shoot_also,
        PadControl pause, PadControl pause_also);
    PadProfile jump_on(PadControl primary);
    std::array<PadProfile, input_slots::pad_count> jump_faces(
        const std::array<PadControl, input_slots::pad_count>& primaries);
    bool conflicting(Key key);
    void require_mapping(const ActionBindings& actual, Key jump, const PadProfile& pad = PadProfile{});
    KeyboardState keys(std::initializer_list<Key> pressed);
    bool same_input(const SlotInput& a, const SlotInput& b);

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
        std::vector<SlotInput> sample()
        {
            pads.poll(); keyboard.poll(); mouse.poll(); menu.update_directions(0.0f, 0.4f, 0.1f, false);
            auto result = player.player_inputs(); const auto repeated = player.player_inputs();
            require(result.size() == input_slots::count && repeated.size() == result.size(), "Actual input reader changed slot count");
            for (size_t slot = 0; slot < result.size(); ++slot)
            { require(same_input(result[slot], repeated[slot]), "Repeated reader access consumed or changed a sampled action edge"); }
            menus = menu.menu_inputs(); return result;
        }
        SlotInput desktop() { return this->sample()[input_slots::keyboard_and_mouse]; }
    };

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

    struct ExpectedKeyboard
    {
        const char* move_left = "a"; const char* move_right = "d"; const char* drop = "s";
        const char* shoot = "mouse_left"; const char* pause = "none";
    };

    std::string fixture(const std::string& bindings = "", const std::string& version = "3");
    Save load(const IsolatedSaveDirectory& directory, const std::string& bindings = "", const std::string& version = "3");
    void require_independent(const Save& save);
    void require_schema(const std::string& bytes, const char* name,
        const std::array<PadProfile, input_slots::pad_count>& expected_pads = default_pads,
        const ExpectedKeyboard& keyboard = {});

    void check_domain_and_mapping(const InputTuning& tuning);
    void check_pad_domain_and_mapping(const InputTuning& tuning);
    void check_actual_reader(const char* content);
    void check_actual_pad_reader(const char* content);
    void check_actual_resume(const char* content);
    void check_save_schema_and_recovery(const IsolatedSaveDirectory& directory);
    void check_save_validation_failures_and_independence(const IsolatedSaveDirectory& directory, const char* content);
    void check_pad_save(const IsolatedSaveDirectory& directory, const char* content);
    void check_keyboard_domain_and_mapping(const InputTuning& tuning);
    void check_keyboard_reader(const char* content);
    void check_keyboard_save(const IsolatedSaveDirectory& directory);
    void check_relaunch(const char* content, std::string_view mode, const std::filesystem::path& profile);
}
