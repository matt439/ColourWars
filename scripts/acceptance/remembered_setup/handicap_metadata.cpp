#include "game/pch.h"
#include "engine/assets/json.h"
#include "game/input_slots.h"
#include "game/last_match_setup.h"
#include "game/save.h"
#include "game/match_setup.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    const StageList handicap_stages{ { "arena", "Arena", "", true } };

    void write_section(const std::filesystem::path& path, const std::string& section)
    {
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        stream << "{\"save_version\":2,\"music_volume\":41,\"last_match\":" << section << '}';
        require(stream.good(), "Cannot write isolated handicap remembered-setup fixture");
    }

    std::string read_bytes(const std::filesystem::path& path)
    {
        std::ifstream stream(path, std::ios::binary);
        require(stream.good(), "Cannot read isolated handicap remembered-setup fixture");
        return { std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
    }

    MatchSetup handicapped_roster()
    {
        MatchSetup roster;
        roster.set_player_count(3);
        roster.set_stage("arena");
        roster.begin_input_claims(input_slots::keyboard_and_mouse);
        require(roster.claim_input_slot(0), "Cannot claim the handicap fixture's second human");
        roster.set_player_bot(2, BotDifficulty::easy);
        roster.set_screen_layout(labrador::ScreenLayout::two_player);
        for (int seat = 0; seat < 3; ++seat)
        {
            roster.set_player_team(seat, seat == 1 ? PlayerTeam::b : PlayerTeam::a);
            roster.set_player_weapon(seat, WeaponType::sniper);
        }
        // Host none, the human +2 and the bot +1: every level, both kinds.
        roster.set_player_handicap(1, 2);
        roster.set_player_handicap(2, 1);
        return roster;
    }
}

void check_handicap_remembered_setup(const std::filesystem::path& save_path)
{
    const MatchSetup original = handicapped_roster();
    const LastMatchSetup captured = LastMatchSetup::capture(original);
    require(captured.players[0].handicap == 0 && captured.players[1].handicap == 2 &&
        captured.players[2].handicap == 1, "Capture lost a seat's handicap");
    Save save;
    require(save.remember_match_setup_and_save(captured), "Cannot persist a handicapped setup");
    const auto document = labrador::read_json_file(save_path.string().c_str());
    const auto players = document.root().object("last_match").array("players");
    require(!players.at(0).has("handicap") && players.at(1).integer("handicap") == 2 &&
        players.at(2).integer("handicap") == 1,
        "A seat without a handicap wrote one, or a handicapped seat lost it");
    Save reloaded;
    reloaded.load_save_file();
    require(reloaded.last_match_setup().has_value(), "Handicapped setup did not reload");
    const MatchSetup restored = reloaded.last_match_setup()->restore(handicap_stages);
    for (int seat = 0; seat < 3; ++seat)
    {
        require(restored.player_handicap(seat) == original.player_handicap(seat),
            "Save/restore changed a seat's handicap");
    }
    require(restored.player_settings()[2].controller == PlayerController::bot &&
        restored.player_settings()[2].difficulty == BotDifficulty::easy,
        "A bot's handicap disturbed its controller or difficulty");

    // Absent and malformed handicaps recover to none, keeping the rest.
    for (const char* bad : { "-1", "3", "1.5", "\"two\"", "null", "true" })
    {
        write_section(save_path, std::string("{\"players\":[{\"team\":\"a\",\"weapon\":\"roller\",\"handicap\":") + bad +
            "},{\"team\":\"b\",\"weapon\":\"bucket\",\"controller\":\"bot\",\"difficulty\":\"hard\",\"handicap\":2}]," +
            "\"stage\":\"arena\",\"layout\":\"one_player\",\"best_of\":3}");
        Save recovered;
        recovered.load_save_file();
        require(recovered.last_match_setup() && recovered.play_settings().music_volume == 41,
            "A malformed handicap discarded the setup or independent preferences");
        const LastMatchSetup& setup = *recovered.last_match_setup();
        require(setup.players[0].handicap == 0 && setup.players[1].handicap == 2 &&
            setup.players[1].controller == PlayerController::bot && setup.best_of == 3,
            "A malformed handicap was kept or disturbed its neighbours");
    }
    write_section(save_path, "{\"players\":[{\"team\":\"a\",\"weapon\":\"roller\"},{\"team\":\"b\",\"weapon\":\"bucket\"}],"
        "\"stage\":\"arena\",\"layout\":\"two_player\",\"best_of\":1}");
    Save legacy;
    legacy.load_save_file();
    require(legacy.last_match_setup() && legacy.last_match_setup()->players[0].handicap == 0 &&
        legacy.last_match_setup()->players[1].handicap == 0, "A save from before handicaps gained one");

    // Strict validation refuses an unresolved handicap without touching the save.
    const std::string before = read_bytes(save_path);
    LastMatchSetup invalid = captured;
    invalid.players[1].handicap = handicaps::count;
    bool rejected = false;
    try { invalid.validate(); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "An out-of-range handicap validated");
    rejected = false;
    try { legacy.remember_match_setup_and_save(invalid); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected && read_bytes(save_path) == before, "An out-of-range handicap reached the save");
    require(invalid.normalised().players[1].handicap == 0, "Recovery kept an out-of-range handicap");
}
