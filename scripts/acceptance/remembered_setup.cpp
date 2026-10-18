#include "game/pch.h"
#include "game/last_match_setup.h"
#include "game/save.h"
#include "game/input_slots.h"
#include "game/objects/level_end_info.h"
#include "game/objects/stage.h"
#include "game/states/menu_level_settings.h"
#include "engine/assets/json.h"

#include <objbase.h>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    template<class Action> void rejects(Action action, const char* message)
    {
        bool rejected = false;
        try { action(); }
        catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, message);
    }

    // A fresh GUID directory is the only profile this process can access.
    // Cleanup removes only the files and directories created by this fixture.
    class IsolatedSaveDirectory
    {
    public:
        IsolatedSaveDirectory()
        {
            wchar_t* previous = nullptr;
            size_t length = 0;
            require(_wdupenv_s(&previous, &length, L"LOCALAPPDATA") == 0,
                "Cannot read LOCALAPPDATA");
            if (previous != nullptr) { previous_ = previous; std::free(previous); }
            GUID guid{};
            wchar_t name[40]{};
            require(SUCCEEDED(CoCreateGuid(&guid)) && StringFromGUID2(guid, name, 40) > 0,
                "Cannot name remembered-setup acceptance profile");
            root_ = std::filesystem::temp_directory_path() / (std::wstring(L"ColourWars-setup-") + name);
            require(std::filesystem::create_directory(root_) &&
                std::filesystem::create_directory(root_ / L"ColourWars"),
                "Cannot create remembered-setup acceptance profile");
            require(_wputenv_s(L"LOCALAPPDATA", root_.c_str()) == 0, "Cannot isolate save profile");
        }

        ~IsolatedSaveDirectory()
        {
            _wputenv_s(L"LOCALAPPDATA", previous_.c_str());
            std::error_code ignored;
            std::filesystem::remove(temporary_path(), ignored);
            std::filesystem::remove(save_path(), ignored);
            std::filesystem::remove(root_ / L"ColourWars", ignored);
            std::filesystem::remove(root_, ignored);
        }

        std::filesystem::path save_path() const { return root_ / L"ColourWars" / L"save_data.json"; }
        std::filesystem::path temporary_path() const { return save_path().wstring() + L".tmp"; }
        void write(const std::string& json) const
        {
            std::ofstream stream(save_path(), std::ios::binary | std::ios::trunc);
            stream << json;
            require(stream.good(), "Cannot write isolated remembered-setup fixture");
        }
        std::string contents() const
        {
            std::ifstream stream(save_path(), std::ios::binary);
            require(stream.good(), "Cannot read isolated remembered-setup save");
            return { std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
        }
    private:
        std::filesystem::path root_;
        std::wstring previous_;
    };

    const std::string other_sections =
        "\"resolution\":{\"x\":1280,\"y\":720},\"fullscreen\":true,"
        "\"team_palette\":\"pink_green\",\"aim_sensitivity\":4,\"music_volume\":23,\"sfx_volume\":67,"
        "\"view_shake\":false,\"aim_marker\":false,\"reduce_effects\":true,"
        "\"pad_sticks\":[{\"movement_deadzone\":0,\"aim_deadzone\":90},"
        "{\"movement_deadzone\":10,\"aim_deadzone\":35},{\"movement_deadzone\":25,\"aim_deadzone\":55},"
        "{\"movement_deadzone\":-1,\"aim_deadzone\":-1}],"
        "\"records\":[{\"stage\":\"retired\",\"versus_played\":7,\"team_a_wins\":4,"
        "\"team_b_wins\":2,\"draws\":1,\"best_win_percentage\":75,\"practice_played\":3,"
        "\"best_coverage_percentage\":37.5}]";

    const StageList stages{ { "arena", "Arena", "", true },
        { "movement_course", "Movement Course", "", false } };
    const std::array<labrador::ScreenLayout, 4> layouts{ labrador::ScreenLayout::one_player,
        labrador::ScreenLayout::two_player, labrador::ScreenLayout::three_player,
        labrador::ScreenLayout::four_player };
    const std::array<const char*, 4> layout_keys{ "one_player", "two_player", "three_player", "four_player" };
    const std::array<WeaponType, 5> weapons{ WeaponType::sprayer, WeaponType::sniper,
        WeaponType::roller, WeaponType::mister, WeaponType::bucket };
    const std::array<const char*, 5> weapon_keys{ "sprayer", "sniper", "roller", "mister", "bucket" };

    Save load(const IsolatedSaveDirectory& directory, const std::string& section,
        const std::string& version = "2")
    {
        directory.write("{" + (version.empty() ? "" : "\"save_version\":" + version + ",") +
            other_sections + (section.empty() ? "" : ",\"last_match\":" + section) + "}");
        Save save;
        save.load_save_file();
        return save;
    }

    void require_other_sections(const Save& save)
    {
        const auto& play = save.play_settings();
        require(save.resolution() == labrador::ScreenResolution::s_1280_720 && save.fullscreen() &&
            save.team_palette() == TeamColourPair::pink_green && play.aim_sensitivity == 4 &&
            play.music_volume == 23 && play.sfx_volume == 67 && !play.view_shake && !play.aim_marker &&
            play.reduce_effects, "Remembered setup changed independent display/play preferences");
        const std::array<StickSettings, 4> sticks{ StickSettings{0, 90}, StickSettings{10, 35},
            StickSettings{25, 55}, StickSettings{-1, -1} };
        for (size_t slot = 0; slot < sticks.size(); ++slot)
        {
            require(play.sticks[slot].movement_deadzone == sticks[slot].movement_deadzone &&
                play.sticks[slot].aim_deadzone == sticks[slot].aim_deadzone,
                "Remembered setup changed an independent pad profile");
        }
        const StageRecord* record = save.records().find("retired");
        require(save.records().stages().size() == 1 && record != nullptr && record->versus_played == 7 &&
            record->team_a_wins == 4 && record->team_b_wins == 2 && record->draws == 1 &&
            record->best_win_percentage == 75.0f && record->practice_played == 3 &&
            record->best_coverage_percentage == 37.5f, "Remembered setup lost a removed stage's records");
    }

    MenuLevelSettings settings(int count, size_t variant)
    {
        MenuLevelSettings result;
        result.set_player_count(count);
        result.set_screen_layout(layouts[static_cast<size_t>(count - 1)]);
        result.set_stage(variant % 2 == 0 ? "arena" : "movement_course");
        result.set_duration(variant % 2 == 0 ? std::nullopt : std::optional<float>{123.5f});
        result.set_best_of(count == 1 ? 1 : std::array<int, 3>{1, 3, 5}[variant % 3]);
        result.begin_input_claims(input_slots::keyboard_and_mouse);
        const std::array<int, 3> slots{2, 0, 3};
        for (int seat = 0; seat < count; ++seat)
        {
            result.set_player_team(seat, (count == 1 ? variant % 2 != 0 : seat % 2 != 0) ?
                PlayerTeam::b : PlayerTeam::a);
            result.set_player_weapon(seat, weapons[(variant + static_cast<size_t>(seat)) % weapons.size()]);
            if (seat != 0) { require(result.claim_input_slot(slots[static_cast<size_t>(seat - 1)]),
                "Cannot prepare explicit sparse fixture devices"); }
        }
        require(result.input_roster_complete(), "Fixture did not claim its complete roster");
        return result;
    }

    bool same(const LastMatchSetup& actual, const LastMatchSetup& expected)
    {
        if (actual.players.size() != expected.players.size() || actual.stage != expected.stage ||
            actual.layout != expected.layout || actual.duration != expected.duration ||
            actual.best_of != expected.best_of) { return false; }
        for (size_t seat = 0; seat < actual.players.size(); ++seat)
        {
            if (actual.players[seat].team != expected.players[seat].team ||
                actual.players[seat].weapon != expected.players[seat].weapon) { return false; }
        }
        return true;
    }

    void require_choices(const LastMatchSetup& actual, const MenuLevelSettings& expected)
    {
        require(actual.players.size() == static_cast<size_t>(expected.player_count()) &&
            actual.stage == expected.stage() && actual.layout == expected.screen_layout() &&
            actual.duration == expected.duration() && actual.best_of == expected.best_of(),
            "Remembered setup changed concrete match choices");
        for (int seat = 0; seat < expected.player_count(); ++seat)
        {
            require(actual.players[static_cast<size_t>(seat)].team == expected.player_team(seat) &&
                actual.players[static_cast<size_t>(seat)].weapon == expected.player_weapon(seat),
                "Remembered setup reordered seats or changed a concrete team/tool");
        }
    }

    void require_fresh(const MenuLevelSettings& actual)
    {
        require(!actual.input_roster_complete(), "Restored setup retained match readiness");
        for (const auto& player : actual.player_settings())
        {
            require(player.num == -1, "Restored setup silently claimed a saved device");
        }
        for (int slot = 0; slot < input_slots::count; ++slot)
        {
            require(actual.player_index_for_slot(slot) == -1, "Restored setup retained a device assignment");
        }
    }

    void check_round_trips(const IsolatedSaveDirectory& directory)
    {
        require(Save::version == 2, "Remembered setup did not introduce save format version 2");
        for (int count = 1; count <= 4; ++count)
        {
            for (size_t variant = 0; variant < weapons.size(); ++variant)
            {
                Save save = load(directory, "");
                const auto original = settings(count, variant);
                const LastMatchSetup captured = LastMatchSetup::capture(original);
                require_choices(captured, original);
                require(same(captured.normalised(), captured), "Valid setup normalisation changed choices");
                captured.validate();
                require(save.remember_match_setup_and_save(captured), "Cannot persist remembered setup");
                require(save.last_match_setup() && same(*save.last_match_setup(), captured),
                    "Successful save did not adopt remembered choices in memory");
                require_other_sections(save);

                // Check the public on-disk schema independently of its reader.
                const auto document = labrador::read_json_file(directory.save_path().string().c_str());
                const auto root = document.root();
                const auto section = root.object("last_match");
                const auto players = section.array("players");
                require(root.integer("save_version") == 2 && players.size() == static_cast<size_t>(count) &&
                    section.string("stage") == original.stage() && section.string("layout") ==
                    layout_keys[static_cast<size_t>(count - 1)] && section.integer("best_of") == original.best_of() &&
                    section.has("duration") == original.duration().has_value(), "Saved setup schema or enum keys drifted");
                if (original.duration()) { require(section.number("duration") == 123.5f,
                    "Saved duration lost its fractional seconds"); }
                for (int seat = 0; seat < count; ++seat)
                {
                    const auto player = players.at(static_cast<size_t>(seat));
                    require(player.string("team") == (original.player_team(seat) == PlayerTeam::a ? "a" : "b") &&
                        player.string("weapon") == weapon_keys[(variant + static_cast<size_t>(seat)) % weapons.size()],
                        "Saved seat used a numeric or incorrect concrete team/tool key");
                    for (const char* key : {"num", "device", "slot", "ready", "confirmed"})
                    {
                        require(!player.has(key), "Save persisted a transient device claim or confirmation");
                    }
                }
                for (const char* key : {"player_count", "claims", "ready", "round", "score", "session"})
                {
                    require(!section.has(key), "Remembered choices included live session state or a second count");
                }

                Save reread;
                reread.load_save_file();
                require(reread.last_match_setup() && same(*reread.last_match_setup(), captured),
                    "Save reload changed remembered choices");
                require_other_sections(reread);
                std::srand(3491); const int expected_random = std::rand(); std::srand(3491);
                auto restored = reread.last_match_setup()->restore(stages);
                require(std::rand() == expected_random, "Restoring concrete choices consumed gameplay RNG");
                require_choices(LastMatchSetup::capture(restored), original);
                require_fresh(restored);
                restored.begin_input_claims(1);
                require(restored.player_index_for_slot(1) == 0 &&
                    restored.input_roster_complete() == (count == 1), "Fresh setup did not require explicit new joins");
                for (int seat = 1; seat < count; ++seat)
                {
                    require(restored.player_settings()[static_cast<size_t>(seat)].num == -1,
                        "Beginning new setup restored another player's old device");
                }
                require(original.input_roster_complete() &&
                    original.player_index_for_slot(input_slots::keyboard_and_mouse) == 0,
                    "Capture or restore mutated the active input roster");
            }
        }
    }

    const std::string valid_section =
        "{\"players\":[{\"team\":\"b\",\"weapon\":\"bucket\"},{\"team\":\"a\",\"weapon\":\"mister\"}],"
        "\"stage\":\"movement_course\",\"layout\":\"two_player\",\"best_of\":5,\"duration\":45.25}";

    void check_versions_and_sections(const IsolatedSaveDirectory& directory)
    {
        for (const std::string version : {std::string(), std::string("1"), std::string("2"), std::string("7")})
        {
            Save legacy = load(directory, "", version);
            require(!legacy.last_match_setup(), "Save without remembered section gained a remembered match");
            require_other_sections(legacy);
            require(legacy.save_to_file(), "Cannot rewrite legacy save");
            require(directory.contents().rfind("{\"save_version\":2,", 0) == 0 &&
                directory.contents().find("\"last_match\"") == std::string::npos,
                "Rewriting absent remembered setup invented choices or wrote an old version");
            Save known = load(directory, valid_section, version);
            require(known.last_match_setup() && known.last_match_setup()->players.size() == 2 &&
                known.last_match_setup()->players[0].team == PlayerTeam::b &&
                known.last_match_setup()->players[0].weapon == WeaponType::bucket &&
                known.last_match_setup()->best_of == 5 && known.last_match_setup()->duration == 45.25f,
                "Versioned save discarded a known remembered section");
            require_other_sections(known);
        }
        for (const char* broken : {"null", "false", "7", "\"match\"", "[]", "{}",
            "{\"players\":null}", "{\"players\":{}}", "{\"players\":[]}",
            "{\"players\":[{},{},{},{},{}]}"})
        {
            Save save = load(directory, broken);
            require(!save.last_match_setup(), "Malformed whole remembered section produced a roster");
            require_other_sections(save);
        }
        // A bad independent section must not discard valid remembered choices.
        directory.write("{\"save_version\":2,\"records\":true,\"resolution\":false,"
            "\"aim_sensitivity\":\"bad\",\"last_match\":" + valid_section + "}");
        Save independent;
        independent.load_save_file();
        require(independent.last_match_setup() && independent.last_match_setup()->stage == "movement_course" &&
            independent.last_match_setup()->best_of == 5 && independent.records().stages().empty(),
            "Broken preferences or records discarded remembered setup");
    }

    void check_defensive_restore(const IsolatedSaveDirectory& directory)
    {
        for (const char* team : {"\"b\"", "\"a\"", "\"missing\"", "null", "4"})
        {
            Save save = load(directory, std::string("{\"players\":[{\"team\":") + team +
                ",\"weapon\":\"sniper\"}],\"stage\":\"retired\",\"layout\":\"four_player\",\"best_of\":5}");
            require(save.last_match_setup().has_value(), "A bad individual field discarded a valid-size roster");
            const auto restored = save.last_match_setup()->restore(stages);
            require(restored.player_team(0) == (std::string(team) == "\"b\"" ? PlayerTeam::b : PlayerTeam::a) &&
                restored.player_weapon(0) == WeaponType::sniper && restored.best_of() == 1 &&
                restored.screen_layout() == labrador::ScreenLayout::one_player && restored.stage() == "arena" &&
                !restored.duration(), "Solo recovery failed to preserve B or normalise stale choices");
            require_fresh(restored);
        }
        for (const char* entries : {"null,\"seat\",7,{}",
            "{\"team\":\"a\",\"weapon\":\"random_primary\"},{\"team\":\"a\",\"weapon\":\"grenade_std\"},"
            "{\"team\":\"a\",\"weapon\":\"none\"},{\"team\":\"a\",\"weapon\":999}"})
        {
            Save save = load(directory, std::string("{\"players\":[") + entries +
                "],\"stage\":\"movement_course\",\"layout\":\"bogus\",\"best_of\":2,\"duration\":-1}");
            require(save.last_match_setup().has_value(), "Invalid seat fields discarded a recoverable roster");
            const auto restored = save.last_match_setup()->restore(stages);
            require(restored.player_count() == 4 && restored.screen_layout() == labrador::ScreenLayout::four_player &&
                restored.best_of() == 1 && !restored.duration() && restored.stage() == "movement_course",
                "Multiplayer recovery changed count/stage or retained invalid optional fields");
            for (int seat = 0; seat < 4; ++seat)
            {
                require(restored.player_team(seat) == (seat % 2 == 0 ? PlayerTeam::a : PlayerTeam::b) &&
                    restored.player_weapon(seat) == WeaponType::sprayer,
                    "Invalid or one-sided multiplayer choices did not recover usable teams/tools");
            }
            require_fresh(restored);
        }
        for (const char* duration : {"0", "-0.5", "1e100", "null", "\"45\"", "true", "[]"})
        {
            Save save = load(directory, std::string("{\"players\":[{}],\"duration\":") + duration + "}");
            require(save.last_match_setup() && !save.last_match_setup()->restore(stages).duration(),
                "Invalid duration survived recovery or discarded the roster");
        }
        for (const char* best_of : {"0", "-1", "2", "7", "1.5", "null", "\"3\""})
        {
            Save save = load(directory, std::string("{\"players\":[{\"team\":\"a\"},{\"team\":\"b\"}],\"best_of\":") +
                best_of + "}");
            require(save.last_match_setup() && save.last_match_setup()->restore(stages).best_of() == 1,
                "Invalid multiplayer series length survived recovery");
        }
        for (const char* stage : {"null", "7", "\"\"", "\"removed\""})
        {
            Save save = load(directory, std::string("{\"players\":[{}],\"stage\":") + stage + "}");
            require(save.last_match_setup() && save.last_match_setup()->restore(stages).stage() == "arena",
                "Invalid stage did not select the manifest's first entry");
        }
        auto raw = LastMatchSetup::capture(settings(3, 3));
        raw.players[0].team = PlayerTeam::none;
        raw.players[1].weapon = WeaponType::random_primary;
        raw.layout = static_cast<labrador::ScreenLayout>(-1);
        raw.duration = std::numeric_limits<float>::quiet_NaN();
        raw.best_of = 4;
        raw.stage = "removed";
        std::srand(7723); const int expected_random = std::rand(); std::srand(7723);
        const auto restored = raw.restore(stages);
        require(std::rand() == expected_random, "Repairing a stale setup consumed gameplay RNG");
        require(restored.player_team(0) == PlayerTeam::a && restored.player_team(1) == PlayerTeam::b &&
            restored.player_team(2) == PlayerTeam::a && restored.player_weapon(1) == WeaponType::sprayer &&
            restored.screen_layout() == labrador::ScreenLayout::three_player && restored.best_of() == 1 &&
            !restored.duration() && restored.stage() == "arena", "Restore did not defensively normalise direct aggregate data");
        require_fresh(restored);
        rejects([&] { raw.restore({}); }, "Empty stage catalogue was accepted");
        for (size_t count : {size_t{0}, size_t{5}})
        {
            auto invalid_count = raw; invalid_count.players.resize(count);
            rejects([&] { invalid_count.normalised(); }, "Invalid aggregate count was silently normalised");
            rejects([&] { invalid_count.restore(stages); }, "Invalid aggregate count was restored");
        }
        auto asymmetric = LastMatchSetup::capture(settings(4, 2));
        asymmetric.players[1].team = PlayerTeam::a;
        asymmetric.players[3].team = PlayerTeam::b;
        require(same(LastMatchSetup::capture(asymmetric.restore(stages)), asymmetric),
            "Restore rebalanced an intentional valid three-versus-one roster");
        Save transient = load(directory,
            "{\"players\":[{\"team\":\"b\",\"weapon\":\"roller\",\"num\":4,\"ready\":true}],"
            "\"stage\":\"arena\",\"layout\":\"one_player\",\"claims\":[4],\"score\":99,\"round\":3}");
        require(transient.last_match_setup().has_value(), "Unknown transient fields discarded concrete choices");
        require_fresh(transient.last_match_setup()->restore(stages));
    }

    void check_validation(const IsolatedSaveDirectory& directory)
    {
        Save save = load(directory, valid_section);
        const LastMatchSetup before = *save.last_match_setup();
        const std::string bytes = directory.contents();
        const auto invalid = [&](LastMatchSetup candidate)
        {
            rejects([&] { candidate.validate(); }, "Invalid remembered aggregate passed validation");
            rejects([&] { save.remember_match_setup_and_save(candidate); }, "Invalid remembered aggregate was saved");
            require(save.last_match_setup() && same(*save.last_match_setup(), before) &&
                directory.contents() == bytes && !std::filesystem::exists(directory.temporary_path()),
                "Rejected remembered setup changed memory, bytes or staging files");
        };
        auto candidate = before; candidate.players.clear(); invalid(candidate);
        candidate = before; candidate.players.resize(5); invalid(candidate);
        candidate = before; candidate.stage.clear(); invalid(candidate);
        candidate = before; candidate.stage = std::string("arena\0alias", 11); invalid(candidate);
        candidate = before; candidate.players[0].team = PlayerTeam::none; invalid(candidate);
        candidate = before; candidate.players[0].team = static_cast<PlayerTeam>(999); invalid(candidate);
        candidate = before; candidate.players[1].team = PlayerTeam::b; invalid(candidate);
        for (WeaponType weapon : {WeaponType::random_primary, WeaponType::grenade_std,
            WeaponType::none, static_cast<WeaponType>(999)})
        {
            candidate = before; candidate.players[0].weapon = weapon; invalid(candidate);
        }
        candidate = before; candidate.layout = labrador::ScreenLayout::four_player; invalid(candidate);
        candidate = before; candidate.layout = static_cast<labrador::ScreenLayout>(-1); invalid(candidate);
        for (int best_of : {0, 2, 7}) { candidate = before; candidate.best_of = best_of; invalid(candidate); }
        candidate = LastMatchSetup::capture(settings(1, 1)); candidate.best_of = 3; invalid(candidate);
        for (float duration : {0.0f, -1.0f, std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::quiet_NaN()})
        {
            candidate = before; candidate.duration = duration; invalid(candidate);
        }
        rejects([] { LastMatchSetup::capture(MenuLevelSettings{}); }, "An unresolved empty menu was captured");
        auto menu = settings(2, 1); menu.set_stage("");
        rejects([&] { LastMatchSetup::capture(menu); }, "An unresolved stage was captured");
        menu = settings(2, 1); menu.set_player_team(1, PlayerTeam::none);
        rejects([&] { LastMatchSetup::capture(menu); }, "An unresolved team was captured");
        menu = settings(2, 1); menu.set_player_weapon(0, WeaponType::random_primary);
        rejects([&] { LastMatchSetup::capture(menu); }, "An unresolved random tool was captured");
        menu = settings(2, 1); menu.set_screen_layout(labrador::ScreenLayout::one_player);
        rejects([&] { LastMatchSetup::capture(menu); }, "A mismatched layout was captured");
        require(directory.contents() == bytes, "Capture validation wrote to the save");
    }

    class ReplacementLock
    {
    public:
        explicit ReplacementLock(const std::filesystem::path& path)
        {
            handle_ = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            require(handle_ != INVALID_HANDLE_VALUE, "Cannot lock private save replacement");
        }
        ~ReplacementLock() { CloseHandle(handle_); }
    private:
        HANDLE handle_ = INVALID_HANDLE_VALUE;
    };

    void check_failures_and_independence(const IsolatedSaveDirectory& directory)
    {
        for (bool replacement_failure : {false, true})
        {
            Save save = load(directory, valid_section);
            const auto old = *save.last_match_setup();
            const std::string bytes = directory.contents();
            const auto active = settings(4, 4);
            const auto proposed = LastMatchSetup::capture(active);
            if (replacement_failure)
            {
                const ReplacementLock lock(directory.save_path());
                require(!save.remember_match_setup_and_save(proposed), "Locked save replacement unexpectedly succeeded");
            }
            else
            {
                require(std::filesystem::create_directory(directory.temporary_path()), "Cannot block own staging path");
                require(!save.remember_match_setup_and_save(proposed), "Blocked staging write unexpectedly succeeded");
                require(std::filesystem::remove(directory.temporary_path()), "Cannot remove own staging blocker");
            }
            require(save.last_match_setup() && same(*save.last_match_setup(), old) && directory.contents() == bytes &&
                !std::filesystem::exists(directory.temporary_path()),
                "Failed remembered save changed old memory/bytes or left a temporary file");
            require_other_sections(save);
            require(active.input_roster_complete() && active.player_index_for_slot(2) == 1 &&
                active.stage() == "arena", "Failed persistence mutated the active match setup");
            Save old_disk;
            old_disk.load_save_file();
            require(old_disk.last_match_setup() && same(*old_disk.last_match_setup(), old),
                "Failed remembered write damaged the reloadable previous setup");
            require(save.remember_match_setup_and_save(proposed), "Remembered save did not recover after releasing blocker");
            require(save.last_match_setup() && same(*save.last_match_setup(), proposed), "Successful retry kept old memory");
            Save retry;
            retry.load_save_file();
            require(retry.last_match_setup() && same(*retry.last_match_setup(), proposed), "Successful retry did not persist");
            require_other_sections(retry);
        }
        Save save = load(directory, valid_section);
        const auto remembered = *save.last_match_setup();
        auto play = save.play_settings(); play.music_volume = 81; play.sticks[2] = {35, 75};
        require(save.set_play_settings_and_save(play) &&
            save.set_display_settings_and_save(labrador::ScreenResolution::s_3840_2160, false, TeamColourPair::blue_orange),
            "Cannot persist independent preference changes");
        LevelEndInfo result; result.total_area = 200.0f; result.team_a_area = 150.0f; result.team_b_area = 50.0f;
        require(save.record_result_and_save("new_stage", result).recorded, "Cannot add independent stage record");
        Save reread;
        reread.load_save_file();
        require(reread.last_match_setup() && same(*reread.last_match_setup(), remembered) &&
            reread.resolution() == labrador::ScreenResolution::s_3840_2160 && !reread.fullscreen() &&
            reread.team_palette() == TeamColourPair::blue_orange && reread.play_settings().music_volume == 81 &&
            reread.play_settings().sticks[2].movement_deadzone == 35 &&
            reread.play_settings().sticks[2].aim_deadzone == 75 && reread.records().find("retired") != nullptr &&
            reread.records().find("new_stage") != nullptr && reread.records().find("new_stage")->team_a_wins == 1,
            "Independent section saves overwrote remembered setup or each other");
    }
}

int main(int argc, char** /*argv*/)
{
    try
    {
        require(argc == 2, "Usage: remembered_setup.exe <staged content directory>");
        const IsolatedSaveDirectory directory;
        check_round_trips(directory);
        check_versions_and_sections(directory);
        check_defensive_restore(directory);
        check_validation(directory);
        check_failures_and_independence(directory);
        std::cout << "PASS remembered setup: all 1-4 seat layouts/five tools, concrete schema, solo B, fresh claims, "
            "deterministic stage fallback/no RNG, legacy/newer/independent sections, malformed recovery, "
            "strict capture/save validation, actual staged-write/replacement failures, unchanged bytes/memory and retry; "
            "GUID-isolated profile, no window/device\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL remembered setup: " << error.what() << '\n';
        return 1;
    }
}
