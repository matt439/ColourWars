#include "game/pch.h"
#include "game/series_stage_rule.h"
#include "game/session.h"
#include "game/last_match_setup.h"
#include "game/save.h"
#include "game/presentation.h"
#include "game/objects/level_end_info.h"
#include "game/states/main_menu.h"
#include "game/states/menu_content_validation.h"
#include "game/match_setup.h"
#include "engine/assets/json.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/audio/audio_resources.h"
#include "engine/core/file_path.h"
#include "engine/render/sprite_font_file.h"

#include <objbase.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

// ROAD#20: the series stage rule. Which stage each rule plays next after a
// win, a draw and a bot-only loss; stages left out of Random; a stage chosen
// by hand between rounds winning over every rule; the rule's place in the
// remembered setup, including saves from before it existed; and its menu text.

using namespace labrador;

namespace
{
    void require(bool condition, const std::string& message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    // A fresh GUID directory is the only profile this process touches.
    class IsolatedSaveDirectory
    {
    public:
        IsolatedSaveDirectory()
        {
            wchar_t* previous = nullptr;
            size_t length = 0;
            require(_wdupenv_s(&previous, &length, L"LOCALAPPDATA") == 0, "Cannot read LOCALAPPDATA");
            if (previous != nullptr) { previous_ = previous; std::free(previous); }
            GUID guid{};
            wchar_t name[40]{};
            require(SUCCEEDED(CoCreateGuid(&guid)) && StringFromGUID2(guid, name, 40) > 0,
                "Cannot name the series stage rule profile");
            root_ = std::filesystem::temp_directory_path() / (std::wstring(L"ColourWars-stage-rule-") + name);
            require(std::filesystem::create_directory(root_) &&
                std::filesystem::create_directory(root_ / L"ColourWars"),
                "Cannot create the series stage rule profile");
            require(_wputenv_s(L"LOCALAPPDATA", root_.c_str()) == 0, "Cannot isolate the save profile");
        }
        ~IsolatedSaveDirectory()
        {
            _wputenv_s(L"LOCALAPPDATA", previous_.c_str());
            std::error_code ignored;
            std::filesystem::remove_all(root_, ignored);
        }
        IsolatedSaveDirectory(const IsolatedSaveDirectory&) = delete;
        IsolatedSaveDirectory& operator=(const IsolatedSaveDirectory&) = delete;
        std::filesystem::path root() const { return root_; }
        std::filesystem::path save_path() const { return root_ / L"ColourWars" / L"save_data.json"; }
        void write(const std::string& json) const
        {
            std::ofstream stream(save_path(), std::ios::binary | std::ios::trunc);
            stream << json;
            require(stream.good(), "Cannot write the isolated save");
        }
        std::string contents() const
        {
            std::ifstream stream(save_path(), std::ios::binary);
            require(stream.good(), "Cannot read the isolated save");
            return { std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
        }
    private:
        std::filesystem::path root_;
        std::wstring previous_;
    };

    // Manifest order: two pool stages around an explicit-only trial, then a third.
    const StageList stages{ { "arena", "Arena", "", true }, { "trial", "Trial", "", false },
        { "hill", "Hill", "", true }, { "ferry", "Ferry", "", true } };

    LevelEndInfo round_won_by(PlayerTeam team)
    {
        LevelEndInfo result;
        result.total_area = 100.0f;
        result.team_a_area = team == PlayerTeam::a ? 60.0f : team == PlayerTeam::b ? 30.0f : 45.0f;
        result.team_b_area = team == PlayerTeam::b ? 60.0f : team == PlayerTeam::a ? 30.0f : 45.0f;
        return result;
    }

    // P1 (human) and P3 on A; P2 on B, human unless `bot_b`.
    MatchSetup series(SeriesStageRule rule, const std::string& stage, bool bot_b = false, int best_of = 3)
    {
        MatchSetup settings;
        settings.set_player_count(3);
        settings.set_player_team(0, PlayerTeam::a);
        settings.set_player_team(1, PlayerTeam::b);
        settings.set_player_team(2, PlayerTeam::a);
        settings.set_player_bot(2, BotDifficulty::easy);
        if (bot_b) { settings.set_player_bot(1, BotDifficulty::hard); }
        settings.set_best_of(best_of);
        settings.set_stage(stage);
        settings.set_series_stage_rule(rule);
        return settings;
    }

    void check_keys()
    {
        for (const SeriesStageRule rule : { SeriesStageRule::same, SeriesStageRule::rotate, SeriesStageRule::losers_pick })
        {
            SeriesStageRule read = SeriesStageRule::same;
            require(read_series_stage_rule(series_stage_rule_key(rule), read) && read == rule,
                std::string("Series stage rule key does not round-trip: ") + series_stage_rule_key(rule));
        }
        require(std::string(series_stage_rule_key(SeriesStageRule::losers_pick)) == "losers_pick" &&
            std::string(series_stage_rule_key(SeriesStageRule::rotate)) == "rotate" &&
            std::string(series_stage_rule_key(SeriesStageRule::same)) == "same", "Series stage rule keys changed");
        SeriesStageRule untouched = SeriesStageRule::rotate;
        require(!read_series_stage_rule("Rotate", untouched) && !read_series_stage_rule("", untouched) &&
            untouched == SeriesStageRule::rotate, "An unknown rule key was accepted or changed the rule");
        bool refused = false;
        try { std::ignore = series_stage_rule_key(static_cast<SeriesStageRule>(9)); }
        catch (const std::invalid_argument&) { refused = true; }
        require(refused, "An invalid rule value has a key");
        MatchSetup settings;
        require(settings.series_stage_rule() == SeriesStageRule::same, "A new setup is not Same stage");
        refused = false;
        try { settings.set_series_stage_rule(static_cast<SeriesStageRule>(9)); }
        catch (const std::invalid_argument&) { refused = true; }
        require(refused && settings.series_stage_rule() == SeriesStageRule::same, "An invalid rule was stored");
        settings.set_series_stage_rule(SeriesStageRule::losers_pick);
        const MatchSetup copy = settings;
        require(copy.series_stage_rule() == SeriesStageRule::losers_pick, "A setup copy lost its rule");
    }

    void check_rotation()
    {
        require(rotated_stage(stages, "arena") == "hill", "Rotate did not skip the explicit-only stage");
        require(rotated_stage(stages, "hill") == "ferry", "Rotate did not take the next pool stage");
        require(rotated_stage(stages, "ferry") == "arena", "Rotate did not wrap to the first pool stage");
        require(rotated_stage(stages, "trial") == "hill", "Rotate from an explicit-only stage did not join the pool after it");
        require(rotated_stage(stages, "retired") == "arena", "Rotate from an unknown stage did not start at the top");
        const StageList one{ { "arena", "Arena", "", true }, { "trial", "Trial", "", false } };
        require(rotated_stage(one, "arena") == "arena" && rotated_stage(one, "trial") == "arena",
            "A one-stage pool did not keep or join its only stage");
        const StageList none{ { "trial", "Trial", "", false } };
        require(rotated_stage(none, "trial") == "trial", "An empty pool did not keep the stage");
        require(rotated_stage(StageList{}, "arena") == "arena", "An empty catalogue did not keep the stage");
        // Three rounds of a rotating series from the trial: it is played once.
        std::string stage = "trial";
        std::string order;
        for (int round = 0; round < 4; ++round) { stage = rotated_stage(stages, stage); order += stage + ' '; }
        require(order == "hill ferry arena hill ", "A rotation from the trial visited it again or broke order");
    }

    void check_rules()
    {
        Session won(3);
        won.finish_round(round_won_by(PlayerTeam::a));
        Session drawn(3);
        drawn.finish_round(round_won_by(PlayerTeam::none));
        Session lost_by_a(5);
        lost_by_a.finish_round(round_won_by(PlayerTeam::b));
        require(won.round_winner() == PlayerTeam::a && drawn.round_winner() == PlayerTeam::none &&
            lost_by_a.round_winner() == PlayerTeam::b, "Session does not report the round's winner");
        require(Session(3).round_winner() == PlayerTeam::none, "An unplayed round has a winner");

        // Same: the original behaviour, whatever happened.
        for (const Session* session : { &won, &drawn, &lost_by_a })
        {
            const MatchSetup same = series(SeriesStageRule::same, "arena", false, 5);
            require(series_next_stage(same, stages, *session, false) == "arena" &&
                series_stage_picker(same, *session) == PlayerTeam::none, "Same stage moved or opened a pick");
        }

        // Rotate: the next pool stage after a decisive round, never a pick.
        const MatchSetup rotate = series(SeriesStageRule::rotate, "arena");
        require(series_next_stage(rotate, stages, won, false) == "hill" &&
            series_stage_picker(rotate, won) == PlayerTeam::none, "Rotate did not move to the next pool stage");
        require(series_next_stage(series(SeriesStageRule::rotate, "trial"), stages, won, false) == "hill",
            "Rotate from a series started on the trial did not join the pool");

        // Loser's pick: B lost to A and has a human, so B picks on Stage Select
        // and the rule leaves the stage to that page; A losing hands it to A.
        const MatchSetup losers = series(SeriesStageRule::losers_pick, "arena");
        require(series_stage_picker(losers, won) == PlayerTeam::b, "The losing team B was not given the pick");
        require(series_next_stage(losers, stages, won, false) == "arena",
            "Loser's pick moved the stage before the loser chose");
        const MatchSetup losers_five = series(SeriesStageRule::losers_pick, "arena", false, 5);
        require(series_stage_picker(losers_five, lost_by_a) == PlayerTeam::a, "The losing team A was not given the pick");
        // A bot-only loser cannot pick: the stage rotates instead.
        const MatchSetup bot_loser = series(SeriesStageRule::losers_pick, "arena", true);
        require(series_stage_picker(bot_loser, won) == PlayerTeam::none &&
            series_next_stage(bot_loser, stages, won, false) == "hill",
            "A bot-only loser was given the pick or did not rotate");

        // Draws replay their stage under every rule, and nobody picks.
        for (const SeriesStageRule rule : { SeriesStageRule::rotate, SeriesStageRule::losers_pick })
        {
            const MatchSetup settings = series(rule, "hill");
            require(series_next_stage(settings, stages, drawn, false) == "hill" &&
                series_stage_picker(settings, drawn) == PlayerTeam::none, "A draw moved the stage or opened a pick");
        }

        // A single match has no next round for the rule to act on.
        Session single(1);
        single.finish_round(round_won_by(PlayerTeam::a));
        for (const SeriesStageRule rule : { SeriesStageRule::rotate, SeriesStageRule::losers_pick })
        {
            const MatchSetup settings = series(rule, "arena", false, 1);
            require(series_next_stage(settings, stages, single, false) == "arena" &&
                series_stage_picker(settings, single) == PlayerTeam::none, "A single match's Rematch moved the stage");
        }

        // A completed series' Play Series Again follows the rule as well.
        Session finished(3);
        finished.finish_round(round_won_by(PlayerTeam::a));
        finished.next_round();
        finished.finish_round(round_won_by(PlayerTeam::a));
        require(finished.complete() && series_next_stage(series(SeriesStageRule::rotate, "hill"), stages, finished, false) == "ferry" &&
            series_stage_picker(series(SeriesStageRule::losers_pick, "hill"), finished) == PlayerTeam::b,
            "Play Series Again did not follow the rule");

        // Change Level by hand: the chosen stage is already in the settings
        // and wins over every rule, even one that would rotate or pick.
        for (const SeriesStageRule rule : { SeriesStageRule::same, SeriesStageRule::rotate, SeriesStageRule::losers_pick })
        {
            for (const bool bot_b : { false, true })
            {
                const MatchSetup chosen = series(rule, "trial", bot_b);
                require(series_next_stage(chosen, stages, won, true) == "trial",
                    "A stage chosen by hand between rounds was overridden by the rule");
            }
        }
    }

    std::string last_match(const std::string& extra)
    {
        return "\"last_match\":{\"players\":[{\"team\":\"a\",\"weapon\":\"sprayer\",\"controller\":\"human\","
            "\"difficulty\":\"normal\"},{\"team\":\"b\",\"weapon\":\"roller\",\"controller\":\"bot\","
            "\"difficulty\":\"hard\"}],\"stage\":\"hill\",\"layout\":\"one_player\",\"best_of\":3" + extra + "}";
    }

    std::optional<LastMatchSetup> load(const IsolatedSaveDirectory& directory, const std::string& json)
    {
        directory.write(json);
        Save save;
        save.load_save_file();
        return save.last_match_setup();
    }

    void check_remembered(const IsolatedSaveDirectory& directory)
    {
        require(Save::version == 5, "The series stage rule needs save version 5");
        // Every rule survives capture, the file and restore.
        for (const SeriesStageRule rule : { SeriesStageRule::same, SeriesStageRule::rotate, SeriesStageRule::losers_pick })
        {
            MatchSetup settings = series(rule, "hill");
            settings.set_screen_layout(ScreenLayout::one_player);
            settings.set_player_human(2);
            settings.set_player_bot(1, BotDifficulty::hard);
            settings.set_player_bot(2, BotDifficulty::easy);
            const LastMatchSetup captured = LastMatchSetup::capture(settings);
            require(captured.stage_rule == rule, "Capture lost the series stage rule");
            std::filesystem::remove(directory.save_path());
            {
                Save save;
                save.load_save_file();
                require(save.remember_match_setup_and_save(captured), "Cannot save the remembered setup");
            }
            const std::string bytes = directory.contents();
            const bool written = bytes.find("\"stage_rule\"") != std::string::npos;
            require(written == (rule != SeriesStageRule::same) &&
                (rule == SeriesStageRule::same || bytes.find(std::string("\"stage_rule\":\"") +
                    series_stage_rule_key(rule) + "\"") != std::string::npos),
                "The rule was not written only when it differs from Same");
            require(bytes.rfind("{\"save_version\":5,", 0) == 0, "The save did not write version 5");
            Save reloaded;
            reloaded.load_save_file();
            require(reloaded.last_match_setup() && reloaded.last_match_setup()->stage_rule == rule,
                "The remembered rule did not survive the file");
            const MatchSetup restored = reloaded.last_match_setup()->restore(stages);
            require(restored.series_stage_rule() == rule && restored.stage() == "hill" && restored.best_of() == 3,
                "Restore lost the rule or changed the setup around it");
        }
        // Saves from before the rule (version 4, and the bare version 2 setup)
        // have no key and play as they always did: Same stage.
        for (const char* version : { "2", "4" })
        {
            const auto old = load(directory, std::string("{\"save_version\":") + version + "," + last_match("") + "}");
            require(old && old->stage_rule == SeriesStageRule::same && old->stage == "hill" && old->best_of == 3 &&
                old->players.size() == 2, "An old save without a rule did not read as Same with its setup intact");
        }
        // Unknown spellings and wrong types recover to Same and keep the rest.
        for (const char* bad : { ",\"stage_rule\":\"shuffle\"", ",\"stage_rule\":2", ",\"stage_rule\":null",
            ",\"stage_rule\":\"Rotate\"" })
        {
            const auto recovered = load(directory, std::string("{\"save_version\":5,") + last_match(bad) + "}");
            require(recovered && recovered->stage_rule == SeriesStageRule::same && recovered->stage == "hill" &&
                recovered->best_of == 3, std::string("A malformed rule did not recover to Same alone: ") + bad);
        }
        const auto rotate = load(directory, std::string("{\"save_version\":5,") + last_match(",\"stage_rule\":\"rotate\"") + "}");
        require(rotate && rotate->stage_rule == SeriesStageRule::rotate, "A saved Rotate did not load");
        // A newer save's rule still loads in this build.
        const auto newer = load(directory, std::string("{\"save_version\":9,") + last_match(",\"stage_rule\":\"losers_pick\"") + "}");
        require(newer && newer->stage_rule == SeriesStageRule::losers_pick, "A newer save's rule did not load");
        // An invalid in-memory rule is refused by capture and repaired by normalised().
        LastMatchSetup broken = *rotate;
        broken.stage_rule = static_cast<SeriesStageRule>(7);
        bool refused = false;
        try { broken.validate(); }
        catch (const std::invalid_argument&) { refused = true; }
        require(refused && broken.normalised().stage_rule == SeriesStageRule::same,
            "An invalid rule was accepted or not repaired to Same");
    }

    struct World
    {
        RenderResources render;
        AudioResources audio;
        explicit World(const std::filesystem::path& content)
        {
            for (const char* name : { "sprite_sheet_1", "painter", "menu_paint", "weapon_paint", "paint_fx", "terrain" })
            {
                render.add_sprite_sheet(name, read_sprite_sheet(path_to_utf8(content / "textures" /
                    path_from_utf8(std::string(name) + ".json")).c_str(), TextureHandle{}));
            }
            for (const char* name : { "bowlby_one_144", "lato_black_24", "lato_black_36", "lato_black_48",
                "lato_black_72", "lato_black_144", "courier_prime_bold_16" })
            {
                const SpriteFontFile file = read_sprite_font_file(
                    path_to_utf8(content / "fonts" / path_from_utf8(std::string(name) + ".spritefont")));
                auto font = std::make_unique<Font>(TextureHandle{}, file.glyphs, file.line_spacing);
                if (font->contains(U'?')) { font->set_stand_in(U'?'); }
                render.add_font(name, std::move(font));
            }
            audio.add_sound_bank("paint_exe", SoundBank::silent());
        }
    };

    rapidjson::Document read_document(const std::filesystem::path& path)
    {
        std::ifstream stream(path, std::ios::binary);
        const std::string text{ std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
        rapidjson::Document document;
        document.Parse(text.c_str());
        require(!document.HasParseError(), "Cannot parse presentation.json");
        return document;
    }

    void check_text(const std::filesystem::path& content, const std::filesystem::path& scratch)
    {
        World world(content);
        const Presentation shipped(path_to_utf8(content / "presentation.json").c_str());
        const MenuText& text = shipped.menu_text();
        require(text.item(MenuItem::series_stage) == L"Series stage" &&
            text.series_stage_value(SeriesStageRule::same) == L"Same stage" &&
            text.series_stage_value(SeriesStageRule::rotate) == L"Rotate" &&
            text.series_stage_value(SeriesStageRule::losers_pick) == L"Loser's pick" &&
            text.stage.team_a_picks == L"Team A picks the arena" && text.stage.team_b_picks == L"Team B picks the arena" &&
            text.prompt(MenuPrompt::mode) == L"A/Enter Select    Left/Right Change    B/Esc Back",
            "The shipped series stage text changed");
        shipped.validate_resources(world.render, world.audio);
        validate_mode_stage_rules(&world.render, shipped);

        const rapidjson::Document source = read_document(content / "presentation.json");
        int next = 0;
        const auto variant = [&](auto edit)
        {
            rapidjson::Document document;
            document.CopyFrom(source, document.GetAllocator());
            edit(document);
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            require(document.Accept(writer), "Cannot encode a presentation variant");
            const auto path = scratch / ("series-stage-" + std::to_string(next++) + ".json");
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            output.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
            output.close();
            require(output.good(), "Cannot write a presentation variant");
            return path;
        };
        const auto refused = [&](const std::string& key, auto edit, bool resources, bool row)
        {
            const auto path = variant(edit);
            std::string diagnostic = "accepted";
            try
            {
                const Presentation candidate(path_to_utf8(path).c_str());
                if (resources) { candidate.validate_resources(world.render, world.audio); }
                if (row) { validate_mode_stage_rules(&world.render, candidate); }
            }
            catch (const std::runtime_error& error) { diagnostic = error.what(); }
            require(diagnostic.find(key) != std::string::npos,
                "Series stage text " + key + " was not refused with its key: " + diagnostic);
        };
        const std::string wide(120, 'W');
        for (const char* key : { "series_stage_same", "series_stage_rotate", "series_stage_losers_pick" })
        {
            refused(key, [key](rapidjson::Document& d) { d["menu_text"]["values"].RemoveMember(key); }, false, false);
            refused("series stage row", [key, &wide](rapidjson::Document& d)
                { d["menu_text"]["values"][key].SetString(wide.c_str(), d.GetAllocator()); }, false, true);
        }
        for (const char* key : { "team_a_picks", "team_b_picks" })
        {
            refused(key, [key](rapidjson::Document& d) { d["menu_text"]["stage"].RemoveMember(key); }, false, false);
            refused(std::string("stage.") + key, [key, &wide](rapidjson::Document& d)
                { d["menu_text"]["stage"][key].SetString(wide.c_str(), d.GetAllocator()); }, true, false);
        }
        refused("series_stage", [](rapidjson::Document& d) { d["menu_text"]["items"].RemoveMember("series_stage"); },
            false, false);
        refused("items.series_stage", [&wide](rapidjson::Document& d)
            { d["menu_text"]["items"]["series_stage"].SetString(wide.c_str(), d.GetAllocator()); }, true, false);
        // The practice hint follows Single Match's card on its row and must
        // still fit the right margin.
        refused("solo_practice_hint", [&wide](rapidjson::Document& d)
            { d["menu_text"]["solo_practice_hint"].SetString(wide.c_str(), d.GetAllocator()); }, true, false);
    }
}

int wmain(int argc, wchar_t* argv[])
{
    try
    {
        require(argc == 2, "usage: series_stage_rule <content>");
        const std::filesystem::path content = std::filesystem::absolute(argv[1]);
        const IsolatedSaveDirectory directory;
        check_keys();
        check_rotation();
        check_rules();
        check_remembered(directory);
        check_text(content, directory.root());
        std::cout << "PASS series stage rule: Same keeps every round's stage; Rotate takes the next Random-pool stage "
            "in manifest order, wraps, never rotates into an explicit-only stage and joins the pool from one; "
            "Loser's pick hands Stage Select to the losing team with a human seat, rotates for a bot-only loser; "
            "draws replay under every rule; single matches ignore it; Play Series Again follows it; a stage chosen "
            "by hand wins over every rule; the rule round-trips the remembered setup, is written only when not "
            "Same (save version 5), and old, unknown or wrong-typed rules read as Same without touching the rest; "
            "menu text read, mapped, fit-checked and refused when missing or too wide\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL series stage rule: " << error.what() << '\n';
        return 1;
    }
}
