#include "game/pch.h"
#include "scripts/acceptance/input_bindings/fixtures.h"
#include "game/objects/level_end_info.h"
#include <rapidjson/document.h>
#include <algorithm>
#include <iostream>

namespace input_bindings_acceptance
{
    const std::string independent_sections =
        "\"resolution\":{\"x\":1280,\"y\":720},\"fullscreen\":true,\"team_palette\":\"pink_green\","
        "\"aim_sensitivity\":4,\"music_volume\":23,\"sfx_volume\":67,\"view_shake\":false,\"aim_marker\":false,\"reduce_effects\":true,"
        "\"pad_sticks\":[{\"movement_deadzone\":0,\"aim_deadzone\":90},{\"movement_deadzone\":10,\"aim_deadzone\":35},"
        "{\"movement_deadzone\":25,\"aim_deadzone\":55},{\"movement_deadzone\":-1,\"aim_deadzone\":-1}],"
        "\"records\":[{\"stage\":\"retired\",\"versus_played\":7,\"team_a_wins\":4,\"team_b_wins\":2,\"draws\":1,"
        "\"best_win_percentage\":75,\"practice_played\":3,\"best_coverage_percentage\":37.5}],"
        "\"last_match\":{\"players\":[{\"team\":\"b\",\"weapon\":\"sniper\"}],\"stage\":\"movement_course\","
        "\"layout\":\"one_player\",\"best_of\":1,\"duration\":45.25}";

    std::string fixture(const std::string& bindings, const std::string& version)
    { return "{" + (version.empty() ? "" : "\"save_version\":" + version + ",") + independent_sections +
        (bindings.empty() ? "" : ",\"input_bindings\":" + bindings) + "}"; }

    Save load(const IsolatedSaveDirectory& directory, const std::string& bindings, const std::string& version)
    { directory.write(fixture(bindings, version)); Save result; SettingsSession result_session(result); result_session.load_save_file(); return result; }

    void require_independent(const Save& save)
    {
        const auto& play = save.saved_play_settings(); const StageRecord* record = save.records().find("retired");
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

    void require_schema(const std::string& bytes, const char* name,
        const std::array<PadProfile, input_slots::pad_count>& expected_pads,
        const ExpectedKeyboard& keyboard)
    {
        rapidjson::Document document; document.Parse(bytes.data(), bytes.size());
        require(!document.HasParseError() && document.IsObject() && document.HasMember("save_version") &&
            document["save_version"].IsInt() && document["save_version"].GetInt() == Save::version &&
            document.HasMember("input_bindings") && document["input_bindings"].IsObject(),
            "Save did not write the current version with the independent binding schema");
        const auto& section = document["input_bindings"];
        require(section.MemberCount() == 3 && section.HasMember("keyboard_jump") && section["keyboard_jump"].IsString() &&
            std::string_view(section["keyboard_jump"].GetString(), section["keyboard_jump"].GetStringLength()) == name,
            "Save binding section did not contain exactly its canonical physical Jump token, keyboard keys and pad profiles");
        require(section.HasMember("keyboard") && section["keyboard"].IsObject() && section["keyboard"].MemberCount() == 5,
            "Save did not write exactly the five other keyboard actions");
        for (const auto& [key, token] : { std::pair{ "move_left", keyboard.move_left }, std::pair{ "move_right", keyboard.move_right },
            std::pair{ "drop", keyboard.drop }, std::pair{ "shoot", keyboard.shoot }, std::pair{ "pause", keyboard.pause } })
        {
            const auto& value = section["keyboard"];
            require(value.HasMember(key) && value[key].IsString() &&
                std::string_view(value[key].GetString(), value[key].GetStringLength()) == token,
                "A saved keyboard action did not keep its canonical control token");
        }
        require(section.HasMember("pads") && section["pads"].IsArray() &&
            section["pads"].Size() == input_slots::pad_count,
            "Save did not write all four stable physical-slot pad profiles");
        const auto token = [](PadControl control)
        {
            const auto found = std::find_if(expected_controls.begin(), expected_controls.end(),
                [&](const ExpectedControl& expected) { return expected.control == control; });
            require(found != expected_controls.end(), "Schema oracle was given an unsupported pad control");
            return std::string_view(found->name);
        };
        for (size_t slot = 0; slot < expected_pads.size(); ++slot)
        {
            const auto& actual = section["pads"][static_cast<rapidjson::SizeType>(slot)];
            require(actual.IsObject() && actual.MemberCount() == 3, "A saved pad profile is not exactly jump/shoot/pause");
            for (size_t index = 0; index < pad_actions.size(); ++index)
            {
                const char* key = index == 0 ? "jump" : (index == 1 ? "shoot" : "pause");
                const PadActionChoice& choice = expected_pads[slot].actions[index];
                require(actual.HasMember(key) && actual[key].IsArray(), "A saved pad profile lost an action");
                const auto& controls = actual[key];
                require(controls.Size() == (choice.alternate == PadControl::none ? 1u : 2u),
                    "A saved pad action did not write its primary and only a real alternate");
                for (rapidjson::SizeType entry = 0; entry < controls.Size(); ++entry)
                {
                    require(controls[entry].IsString() && std::string_view(controls[entry].GetString(), controls[entry].GetStringLength()) ==
                        token(entry == 0 ? choice.primary : choice.alternate),
                        "Saved pad controls did not retain canonical tokens and physical slot order");
                }
            }
        }
    }

    void check_save_schema_and_recovery(const IsolatedSaveDirectory& directory)
    {
        require(Save::version >= 3, "Input binding schema needs save version 3 or later");
        for (const char* version : { "", "0", "1", "2" })
        {
            Save legacy = load(directory, "", version); SettingsSession legacy_session(legacy);
            require(legacy_session.input_bindings().keyboard_jump == Key::space, "Legacy save without bindings did not acquire default Space");
            require_independent(legacy); require(legacy.save_to_file(), "Legacy binding migration could not save");
            require_schema(directory.contents(), "space"); Save reread; SettingsSession reread_session(reread); reread_session.load_save_file(); require_independent(reread);
            require(reread_session.input_bindings().keyboard_jump == Key::space, "Migrated legacy binding did not round trip");
        }
        for (const char* version : { "0", "1", "2", "3", "999" })
        {
            Save known = load(directory, "{\"keyboard_jump\":\"j\"}", version); SettingsSession known_session(known);
            require(known_session.input_bindings().keyboard_jump == Key::j, "Versioned save discarded its known independent binding section");
            require_independent(known);
        }
        Save save = load(directory, "{\"keyboard_jump\":\"j\"}"); SettingsSession save_session(save);
        for (const auto& expected : expected_keys)
        {
            if (conflicting(expected.key)) { continue; }
            require(save_session.set_input_bindings_and_save(InputBindings{ expected.key }), "A valid physical Jump preference did not save");
            require(save_session.input_bindings().keyboard_jump == expected.key && save.save_data().input_bindings.keyboard_jump == expected.key,
                "Successful binding setter did not update both borrowed preference and SaveData copy");
            require_schema(directory.contents(), expected.name); Save reread; SettingsSession reread_session(reread); reread_session.load_save_file();
            require(reread_session.input_bindings().keyboard_jump == expected.key, "Canonical Jump preference did not round trip");
            require_independent(reread);
        }
        for (const char* broken : { "null", "false", "7", "\"j\"", "[]", "{}", "{\"jump\":\"j\"}",
            "{\"keyboard_jump\":null}", "{\"keyboard_jump\":false}", "{\"keyboard_jump\":7}", "{\"keyboard_jump\":[]}",
            "{\"keyboard_jump\":{}}", "{\"keyboard_jump\":\"unknown\"}", "{\"keyboard_jump\":\"J\"}",
            "{\"keyboard_jump\":\"Space\"}", "{\"keyboard_jump\":\"enter\"}", "{\"keyboard_jump\":\"grave\"}",
            "{\"keyboard_jump\":\"a\"}", "{\"keyboard_jump\":\"d\"}", "{\"keyboard_jump\":\"s\"}",
            "{\"keyboard_jump\":\"j\\u0000alias\"}" })
        {
            directory.write(fixture("{\"keyboard_jump\":\"j\"}")); save_session.load_save_file();
            directory.write(fixture(broken)); const std::string bytes = directory.contents(); save_session.load_save_file();
            require(save_session.input_bindings().keyboard_jump == Key::space && directory.contents() == bytes,
                "Malformed binding section retained old bindings or rewrote the player's bytes on load");
            require_independent(save);
        }
        directory.write("{\"save_version\":3,\"records\":false,\"resolution\":false,\"aim_sensitivity\":\"bad\","
            "\"last_match\":false,\"input_bindings\":{\"keyboard_jump\":\"j\"}}");
        save_session.load_save_file(); require(save_session.input_bindings().keyboard_jump == Key::j,
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
        Save save = load(directory, "{\"keyboard_jump\":\"j\"}"); SettingsSession save_session(save);
        const InputBindings* borrowed = &save_session.input_bindings();
        const std::string original = directory.contents();
        for (Key key : { Key::a, Key::d, Key::s, Key::none, Key::enter, Key::grave, static_cast<Key>(255) })
        {
            rejects([&] { save_session.set_input_bindings_and_save(InputBindings{ key }); }, "Invalid binding setter did not validate before assignment/write");
            require(&save_session.input_bindings() == borrowed && borrowed->keyboard_jump == Key::j && directory.contents() == original &&
                !std::filesystem::exists(directory.temporary_path()), "Rejected binding setter changed active memory, disk bytes or staging state");
            require_independent(save);
        }
        for (bool replacement_failure : { false, true })
        {
            save = load(directory, "{\"keyboard_jump\":\"j\"}"); save_session.load_save_file(); const std::string bytes = directory.contents();
            ReaderHarness h(content);
            PlayerInput saved_reader(&h.pads, &h.keyboard, &h.mouse, &h.tuning, &h.play, borrowed);
            if (replacement_failure)
            {
                const ReplacementLock lock(directory.save_path());
                require(!save_session.set_input_bindings_and_save(InputBindings{ Key::k }), "Locked binding save replacement unexpectedly succeeded");
            }
            else
            {
                require(std::filesystem::create_directory(directory.temporary_path()), "Cannot block own binding staging path");
                require(!save_session.set_input_bindings_and_save(InputBindings{ Key::k }), "Blocked binding staging write unexpectedly succeeded");
                require(std::filesystem::remove(directory.temporary_path()), "Cannot remove own binding staging blocker");
            }
            require(&save_session.input_bindings() == borrowed && borrowed->keyboard_jump == Key::k && directory.contents() == bytes &&
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
            require_independent(save); Save old_disk; SettingsSession old_disk_session(old_disk); old_disk_session.load_save_file();
            require(old_disk_session.input_bindings().keyboard_jump == Key::j, "Failed binding write damaged the reloadable previous preference");
            require_independent(old_disk);
            require(save_session.set_input_bindings_and_save(InputBindings{ Key::k }) && &save_session.input_bindings() == borrowed,
                "Targeted retry of the already active binding did not write after removing its real IO blocker");
            Save retried; SettingsSession retried_session(retried); retried_session.load_save_file(); require(retried_session.input_bindings().keyboard_jump == Key::k,
                "Retry did not persist the binding kept active after failure");
            require_schema(directory.contents(), "k"); require_independent(retried);
        }
        save = load(directory, "{\"keyboard_jump\":\"j\"}"); save_session.load_save_file();
        auto settings_copy = save_session.play_settings(); auto controls_copy = save_session.play_settings();
        require(save_session.set_input_bindings_and_save(InputBindings{ Key::w }), "Cannot prepare independent saved binding");
        settings_copy.music_volume = 81; settings_copy.reduce_effects = false;
        require(save_session.set_play_settings_and_save(settings_copy) && save_session.input_bindings().keyboard_jump == Key::w,
            "An older Settings PlaySettings snapshot overwrote independent input bindings");
        controls_copy.aim_marker = true; controls_copy.sticks[2] = { 35, 75 };
        require(save_session.set_play_settings_and_save(controls_copy) && save_session.input_bindings().keyboard_jump == Key::w &&
            save.set_display_settings_and_save(ScreenResolution::s_3840_2160, false, TeamColourPair::blue_orange),
            "An older Controls snapshot or display setter overwrote independent input bindings");
        LevelEndInfo result; result.total_area = 200.0f; result.team_a_area = 150.0f; result.team_b_area = 50.0f;
        require(save.record_result_and_save("new_stage", result).recorded, "Cannot exercise independent record persistence");
        const LastMatchSetup remembered = *save.last_match_setup();
        require(save.remember_match_setup_and_save(remembered), "Cannot exercise independent remembered-match persistence");
        Save reread; SettingsSession reread_session(reread); reread_session.load_save_file();
        require(&save_session.input_bindings() == borrowed && borrowed->keyboard_jump == Key::w &&
            reread_session.input_bindings().keyboard_jump == Key::w && reread.resolution() == ScreenResolution::s_3840_2160 &&
            !reread.fullscreen() && reread.team_palette() == TeamColourPair::blue_orange && reread_session.play_settings().aim_marker &&
            reread_session.play_settings().sticks[2].movement_deadzone == 35 && reread_session.play_settings().sticks[2].aim_deadzone == 75 &&
            reread.records().find("retired") && reread.records().find("new_stage") &&
            reread.records().find("new_stage")->team_a_wins == 1 && reread.last_match_setup() &&
            reread.last_match_setup()->stage == remembered.stage && reread.last_match_setup()->players[0].weapon == WeaponType::sniper,
            "Settings/Controls/display/records/remembered saves overwrote the independent binding or their expected data");
        require_schema(directory.contents(), "w");
    }

    void check_pad_save(const IsolatedSaveDirectory& directory, const char* content)
    {
        // The earlier face-button array migrates per slot; Shoot, Pause and LT keep their defaults.
        const auto selected = jump_faces({ PadControl::b, PadControl::x, PadControl::y, PadControl::a });
        Save save = load(directory, "{\"keyboard_jump\":\"j\",\"pad_jump\":[\"b\",\"x\",\"y\",\"a\"]}"); SettingsSession save_session(save);
        require(save_session.input_bindings().keyboard_jump == Key::j && save_session.input_bindings().pads == selected,
            "Legacy per-slot Jump preferences did not migrate in physical slot order");
        require_independent(save);
        require(save.save_to_file(), "Migrated pad profiles did not save");
        require_schema(directory.contents(), "j", selected);
        Save reread; SettingsSession reread_session(reread); reread_session.load_save_file(); require(reread_session.input_bindings() == save_session.input_bindings(),
            "Migrated pad profiles did not round trip");
        SaveData reset = save.save_data(); InputBindingsSection{}.reset(reset);
        require(reset.input_bindings == InputBindings{}, "Input section reset failed to restore default keyboard and every pad slot");

        for (const char* broken : { "null", "false", "7", "[]", "{}", "\"unknown\"", "\"B\"", "\"lt\"", "\"start\"", "\"b\\u0000alias\"" })
        {
            save = load(directory, "{\"keyboard_jump\":\"j\",\"pad_jump\":[\"b\"," + std::string(broken) + ",\"y\",\"x\"]}"); save_session.load_save_file();
            const auto recovered = jump_faces({ PadControl::b, PadControl::a, PadControl::y, PadControl::x });
            require(save_session.input_bindings().keyboard_jump == Key::j && save_session.input_bindings().pads == recovered,
                "Malformed legacy pad choice discarded valid keyboard or another slot's choice");
            require_independent(save);
        }
        for (const char* broken : { "null", "false", "7", "\"b\"", "{}" })
        {
            save = load(directory, "{\"keyboard_jump\":\"j\",\"pad_jump\":" + std::string(broken) + "}"); save_session.load_save_file();
            require(save_session.input_bindings().keyboard_jump == Key::j && save_session.input_bindings().pads == default_pads,
                "Malformed legacy pad array discarded valid keyboard or failed to use per-slot defaults");
        }
        for (const char* keyboard : { "", "\"keyboard_jump\":\"a\",", "\"keyboard_jump\":false," })
        {
            save = load(directory, "{" + std::string(keyboard) + "\"pad_jump\":[\"x\",\"y\",\"b\"]}"); save_session.load_save_file();
            const auto recovered = jump_faces({ PadControl::x, PadControl::y, PadControl::b, PadControl::a });
            require(save_session.input_bindings().keyboard_jump == Key::space && save_session.input_bindings().pads == recovered,
                "Malformed/missing keyboard or short legacy array discarded independently valid pad choices");
        }

        // Full profiles: any control, alternates, and per-slot recovery.
        const std::array<PadProfile, input_slots::pad_count> custom{
            profile(PadControl::right_trigger, PadControl::a, PadControl::right_shoulder, PadControl::left_trigger,
                PadControl::y, PadControl::none),
            PadProfile{},
            profile(PadControl::left_stick, PadControl::none, PadControl::right_trigger, PadControl::x,
                PadControl::start, PadControl::right_stick),
            profile(PadControl::b, PadControl::left_shoulder, PadControl::right_trigger, PadControl::none,
                PadControl::start, PadControl::y) };
        const std::string slots[]{
            "{\"jump\":[\"rt\",\"a\"],\"shoot\":[\"rb\",\"lt\"],\"pause\":[\"y\"]}",
            "{\"jump\":[\"a\",\"lt\"],\"shoot\":[\"rt\"],\"pause\":[\"start\"]}",
            "{\"jump\":[\"ls\"],\"shoot\":[\"rt\",\"x\"],\"pause\":[\"start\",\"rs\"]}",
            "{\"jump\":[\"b\",\"lb\"],\"shoot\":[\"rt\"],\"pause\":[\"start\",\"y\"]}" };
        const auto pads_json = [&](const std::string& second)
        { return "{\"keyboard_jump\":\"j\",\"pads\":[" + slots[0] + "," + second + "," + slots[2] + "," + slots[3] + "]}"; };
        save = load(directory, pads_json(slots[1])); save_session.load_save_file();
        require(save_session.input_bindings().keyboard_jump == Key::j && save_session.input_bindings().pads == custom,
            "Full per-slot pad profiles did not load");
        require(save.save_to_file(), "Full pad profiles did not save"); require_schema(directory.contents(), "j", custom);
        reread_session.load_save_file(); require(reread_session.input_bindings() == save_session.input_bindings(), "Full pad profiles did not round trip");
        for (const char* broken : { "null", "false", "7", "[]", "\"a\"", "{}",
            "{\"jump\":[\"a\"],\"shoot\":[\"rt\"]}", "{\"jump\":[],\"shoot\":[\"rt\"],\"pause\":[\"start\"]}",
            "{\"jump\":[\"none\"],\"shoot\":[\"rt\"],\"pause\":[\"start\"]}",
            "{\"jump\":[\"rt\"],\"shoot\":[\"rt\"],\"pause\":[\"start\"]}",
            "{\"jump\":[\"a\",\"a\"],\"shoot\":[\"rt\"],\"pause\":[\"start\"]}",
            "{\"jump\":[\"a\",\"b\",\"x\"],\"shoot\":[\"rt\"],\"pause\":[\"start\"]}",
            "{\"jump\":[\"back\"],\"shoot\":[\"rt\"],\"pause\":[\"start\"]}",
            "{\"jump\":[\"A\"],\"shoot\":[\"rt\"],\"pause\":[\"start\"]}",
            "{\"jump\":\"a\",\"shoot\":[\"rt\"],\"pause\":[\"start\"]}",
            "{\"jump\":[7],\"shoot\":[\"rt\"],\"pause\":[\"start\"]}" })
        {
            save = load(directory, pads_json(broken)); save_session.load_save_file();
            require(save_session.input_bindings().keyboard_jump == Key::j && save_session.input_bindings().pads == custom,
                "A malformed pad profile discarded the keyboard key or another slot's profile");
            require_independent(save);
        }
        for (const char* broken : { "null", "false", "7", "\"a\"", "{}" })
        {
            save = load(directory, "{\"keyboard_jump\":\"j\",\"pads\":" + std::string(broken) + ",\"pad_jump\":[\"b\"]}"); save_session.load_save_file();
            require(save_session.input_bindings().keyboard_jump == Key::j && save_session.input_bindings().pads == default_pads,
                "A malformed pads array discarded the keyboard key, or fell back to the older face array");
        }
        save = load(directory, "{\"keyboard_jump\":\"j\",\"pads\":[" + slots[0] + "],\"pad_jump\":[\"b\",\"x\"]}"); save_session.load_save_file();
        require(save_session.input_bindings().pads[0] == custom[0] && save_session.input_bindings().pads[1] == PadProfile{},
            "A short pads array did not default its missing slots, or mixed in the older face array");
        save = load(directory, "{\"keyboard_jump\":\"j\",\"pads\":[" + slots[0] + "," + slots[1] + "," + slots[2] + "," + slots[3] + ",false]}"); save_session.load_save_file();
        require(save_session.input_bindings().pads == custom, "Unknown extra slot discarded known physical-slot profiles");

        const InputBindings* borrowed = &save_session.input_bindings(); const std::string bytes = directory.contents();
        InputBindings invalid = *borrowed; invalid.pads[2][PadAction::jump].primary = PadControl::start;
        rejects([&] { save_session.set_input_bindings_and_save(invalid); }, "Invalid pad setter did not validate before mutation");
        require(&save_session.input_bindings() == borrowed && borrowed->pads == custom && directory.contents() == bytes &&
            !std::filesystem::exists(directory.temporary_path()), "Rejected pad setter mutated active choices, disk or staging state");
        for (bool replacement_failure : { false, true })
        {
            save = load(directory, "{\"keyboard_jump\":\"j\"}"); save_session.load_save_file(); const std::string previous = directory.contents();
            InputBindings candidate = save_session.input_bindings(); candidate.pads = custom;
            if (replacement_failure)
            {
                const ReplacementLock lock(directory.save_path());
                require(!save_session.set_input_bindings_and_save(candidate), "Locked pad preference replacement unexpectedly succeeded");
            }
            else
            {
                require(std::filesystem::create_directory(directory.temporary_path()), "Cannot block own pad save staging path");
                require(!save_session.set_input_bindings_and_save(candidate), "Blocked pad preference staging unexpectedly succeeded");
                require(std::filesystem::remove(directory.temporary_path()), "Cannot remove own pad save staging blocker");
            }
            require(save_session.input_bindings() == candidate && directory.contents() == previous &&
                !std::filesystem::exists(directory.temporary_path()), "Failed pad write rolled back active preferences or damaged disk bytes");
            ReaderHarness h(content);
            PlayerInput actual(&h.pads, &h.keyboard, &h.mouse, &h.tuning, &h.play, &save_session.input_bindings());
            auto& pad = binding_test_pads::states[2]; pad.connected = true; std::ignore = h.sample();
            pad = pressing(PadControl::left_stick); std::ignore = h.sample();
            require(actual.player_inputs()[2].jump_pressed, "Save-borrowing reader ignored a pad profile kept active after write failure");
            pad = pressing(PadControl::none); std::ignore = h.sample();
            Save previous_disk; SettingsSession previous_disk_session(previous_disk); previous_disk_session.load_save_file();
            require(previous_disk_session.input_bindings().pads == default_pads,
                "Failed pad persistence damaged reloadable previous choices");
            require(save_session.set_input_bindings_and_save(candidate), "Pad preference retry failed after removing real IO blocker");
            require_schema(directory.contents(), "j", custom); require_independent(save);
        }
        auto play = save_session.play_settings(); play.music_volume = 81;
        require(save_session.set_play_settings_and_save(play) && save_session.input_bindings().pads == custom &&
            save.set_display_settings_and_save(ScreenResolution::s_3840_2160, false, TeamColourPair::blue_orange),
            "Unrelated settings save overwrote per-slot pad profiles");
        Save final; SettingsSession final_session(final); final_session.load_save_file();
        require(final_session.input_bindings().keyboard_jump == Key::j && final_session.input_bindings().pads == custom,
            "Independent display/play save did not preserve keyboard and per-slot pad profiles");
    }

    void check_relaunch(const char* content, std::string_view mode, const std::filesystem::path& profile)
    {
        require(mode == "relaunch-write" || mode == "relaunch-read", "Unknown input-binding relaunch mode");
        const IsolatedSaveDirectory directory(profile);
        if (mode == "relaunch-write")
        {
            require(!std::filesystem::exists(directory.save_path()), "Relaunch writer requires a fresh private profile");
            directory.write(fixture()); Save save; SettingsSession save_session(save); save_session.load_save_file(); require_independent(save);
            require(save_session.input_bindings() == InputBindings{} && save_session.set_input_bindings_and_save(InputBindings{ Key::j }),
                "Relaunch writer did not persist J through the targeted Save setter");
            require_schema(directory.contents(), "j"); require_independent(save);
        }
        else
        {
            require(std::filesystem::is_regular_file(directory.save_path()), "Relaunch reader requires the writer's private save");
            const std::string before = directory.contents(); Save save; SettingsSession save_session(save); save_session.load_save_file(); require_independent(save);
            require(save_session.input_bindings() == InputBindings{ Key::j }, "Fresh-process Save lost the persisted J preference");
            ReaderHarness h(content);
            PlayerInput reader(&h.pads, &h.keyboard, &h.mouse, &h.tuning, &h.play, &save_session.input_bindings());
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
