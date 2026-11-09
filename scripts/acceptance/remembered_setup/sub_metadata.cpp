#include "game/pch.h"
#include "engine/assets/json.h"
#include "game/input/input_slots.h"
#include "game/save/last_match_setup.h"
#include "game/save/save.h"
#include "game/config/settings_session.h"
#include "game/match/match_setup.h"
#include "scripts/acceptance/support/probe.h"
#include "scripts/acceptance/support/scratch.h"

#include <filesystem>
#include <stdexcept>
#include <string>

// ROAD#11: each seat's sub weapon in "last_match" (save version 8).
namespace
{
    using acceptance::require;

    const StageList sub_stages{ { "arena", "Arena", "", true } };

    void write_section(const std::filesystem::path& path, const std::string& section)
    {
        acceptance::write_text(path, "{\"save_version\":7,\"music_volume\":41,\"last_match\":" + section + '}');
    }

    MatchSetup sub_roster()
    {
        MatchSetup roster;
        roster.set_player_count(3);
        roster.set_stage("arena");
        roster.begin_input_claims(input_slots::keyboard_and_mouse);
        require(roster.claim_input_slot(0), "Cannot claim the sub fixture's second human");
        roster.set_player_bot(2, BotDifficulty::hard);
        roster.set_screen_layout(labrador::ScreenLayout::two_player);
        for (int seat = 0; seat < 3; ++seat)
        {
            roster.set_player_team(seat, seat == 1 ? PlayerTeam::b : PlayerTeam::a);
        }
        // Host the grenade, the human the wall and the bot the sprinkler.
        roster.set_player_sub(1, SubType::wall);
        roster.set_player_sub(2, SubType::sprinkler);
        return roster;
    }
}

void check_sub_remembered_setup(const std::filesystem::path& save_path)
{
    const MatchSetup original = sub_roster();
    const LastMatchSetup captured = LastMatchSetup::capture(original);
    require(captured.players[0].sub == SubType::grenade && captured.players[1].sub == SubType::wall &&
        captured.players[2].sub == SubType::sprinkler, "Capture lost a seat's sub");
    Save save; SettingsSession save_session(save);
    require(save.remember_match_setup_and_save(captured), "Cannot persist a setup with subs");
    const auto document = labrador::read_json_file(save_path.string().c_str());
    require(document.root().integer("save_version") >= 8, "A save with subs is older than save version 8");
    const auto players = document.root().object("last_match").array("players");
    require(!players.at(0).has("sub") && players.at(1).string("sub") == "wall" &&
        players.at(2).string("sub") == "sprinkler",
        "A grenade seat wrote its sub, or another seat lost its");
    Save reloaded; SettingsSession reloaded_session(reloaded);
    reloaded_session.load_save_file();
    require(reloaded.last_match_setup().has_value(), "A setup with subs did not reload");
    const MatchSetup restored = reloaded.last_match_setup()->restore(sub_stages);
    for (int seat = 0; seat < 3; ++seat)
    {
        require(restored.player_sub(seat) == original.player_sub(seat), "Save/restore changed a seat's sub");
    }
    require(restored.player_settings()[2].controller == PlayerController::bot &&
        restored.player_settings()[2].difficulty == BotDifficulty::hard,
        "A bot's sub disturbed its controller or difficulty");

    // Absent (every save before version 8) and unknown subs read as the grenade.
    for (const char* bad : { "\"mine\"", "\"Grenade\"", "\"random\"", "2", "null", "true" })
    {
        write_section(save_path, std::string("{\"players\":[{\"team\":\"a\",\"weapon\":\"roller\",\"sub\":") + bad +
            "},{\"team\":\"b\",\"weapon\":\"bucket\",\"controller\":\"bot\",\"difficulty\":\"hard\",\"sub\":\"wall\"}]," +
            "\"stage\":\"arena\",\"layout\":\"one_player\",\"best_of\":3}");
        Save recovered; SettingsSession recovered_session(recovered);
        recovered_session.load_save_file();
        require(recovered.last_match_setup() && recovered_session.play_settings().music_volume == 41,
            "A malformed sub discarded the setup or independent preferences");
        const LastMatchSetup& setup = *recovered.last_match_setup();
        require(setup.players[0].sub == SubType::grenade && setup.players[1].sub == SubType::wall &&
            setup.players[1].controller == PlayerController::bot && setup.best_of == 3,
            "A malformed sub was kept or disturbed its neighbours");
    }
    write_section(save_path, "{\"players\":[{\"team\":\"a\",\"weapon\":\"roller\"},{\"team\":\"b\",\"weapon\":\"bucket\"}],"
        "\"stage\":\"arena\",\"layout\":\"two_player\",\"best_of\":1}");
    Save legacy; SettingsSession legacy_session(legacy);
    legacy_session.load_save_file();
    require(legacy.last_match_setup() && legacy.last_match_setup()->players[0].sub == SubType::grenade &&
        legacy.last_match_setup()->players[1].sub == SubType::grenade, "A save from before subs gained another");

    // Strict validation refuses an unresolved sub without touching the save.
    const std::string before = acceptance::read_text(save_path);
    LastMatchSetup invalid = captured;
    invalid.players[1].sub = SubType::count;
    bool rejected = false;
    try { invalid.validate(); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "An unresolved sub validated");
    rejected = false;
    try { legacy.remember_match_setup_and_save(invalid); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected && acceptance::read_text(save_path) == before, "An unresolved sub reached the save");
    require(invalid.normalised().players[1].sub == SubType::grenade, "Recovery kept an unresolved sub");
}
