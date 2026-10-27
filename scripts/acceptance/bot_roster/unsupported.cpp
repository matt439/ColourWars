#include "game/pch.h"
#include "game/debug/dev_tools.h"
#include "game/input_slots.h"
#include "game/last_match_setup.h"
#include "game/objects/level_builder.h"
#include "game/states/menu_level_settings.h"

#include <filesystem>
#include <stdexcept>
#include <string>

#if COLOURWARS_DEV_TOOLS
#include "game/debug/replay.h"
#include <objbase.h>
#include <fstream>
#include <iterator>
#endif

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    template<class Action>
    void rejects_bot_roster(Action action, const char* message)
    {
        bool rejected = false;
        try { action(); }
        catch (const std::invalid_argument& error)
        {
            const std::string diagnostic = error.what();
            rejected = diagnostic.find("bot") != std::string::npos &&
                diagnostic.find("pending") != std::string::npos;
        }
        require(rejected, message);
    }

#if COLOURWARS_DEV_TOOLS
    // Only this new GUID directory and its named test files may be touched.
    class ReplayDirectory
    {
    public:
        ReplayDirectory()
        {
            GUID guid{};
            wchar_t name[40]{};
            require(SUCCEEDED(CoCreateGuid(&guid)) && StringFromGUID2(guid, name, 40) > 0,
                "Cannot name the bot-roster replay directory");
            this->root_ = std::filesystem::temp_directory_path() /
                (std::wstring(L"ColourWars-bot-roster-") + name);
            require(std::filesystem::create_directory(this->root_),
                "Cannot create the isolated bot-roster replay directory");
        }

        ~ReplayDirectory()
        {
            std::error_code ignored;
            std::filesystem::remove(this->missing_path(), ignored);
            std::filesystem::remove(this->missing_path().parent_path(), ignored);
            std::filesystem::remove(this->existing_path(), ignored);
            std::filesystem::remove(this->root_, ignored);
        }

        std::filesystem::path missing_path() const { return this->root_ / L"missing" / L"round.json"; }
        std::filesystem::path existing_path() const { return this->root_ / L"existing.json"; }

    private:
        std::filesystem::path root_;
    };

    std::string contents(const std::filesystem::path& path)
    {
        std::ifstream stream(path, std::ios::binary);
        require(stream.good(), "Cannot read the isolated replay sentinel");
        return { std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
    }
#endif
}

void check_unsupported_bot_consumers(const std::filesystem::path& content)
{
    MenuLevelSettings settings;
    settings.set_player_count(2);
    settings.begin_input_claims(input_slots::keyboard_and_mouse);
    settings.set_player_bot(1, BotDifficulty::normal);

    // Constructor needs tuning metadata only. Every runtime resource is null,
    // proving the unsupported roster is rejected before a level lookup or build.
    const GameTuning tuning((content / "tuning.json").string().c_str());
    LevelBuilder builder(nullptr, nullptr, nullptr, nullptr, nullptr, &tuning,
        nullptr, nullptr, nullptr, nullptr, nullptr);
    rejects_bot_roster([&]
        { builder.build_level(settings, TeamColourPair::blue_orange, OpeningMode::off); },
        "The level builder did not reject bots before touching runtime resources");

    // The otherwise-unresolved setup must produce the capability diagnostic,
    // rather than silently dropping bot identity or reaching ordinary validation.
    rejects_bot_roster([&] { LastMatchSetup::capture(settings); },
        "Remembered setup silently discarded bot metadata or validated it too late");
    const MenuText text;
    rejects_bot_roster([&] { settings.player_label(1, text); },
        "The human roster label formatter mislabeled a bot as waiting for a device");

    MenuLevelSettings dummy = settings;
    dummy.set_player_human(1);
    dummy.set_stage("legacy_debug_dummy");
    dummy.set_screen_layout(labrador::ScreenLayout::two_player);
    dummy.set_player_team(0, PlayerTeam::a);
    dummy.set_player_team(1, PlayerTeam::b);
    const LastMatchSetup captured = LastMatchSetup::capture(dummy);
    require(captured.players.size() == 2 && dummy.player_settings()[1].num == -1,
        "An unclaimed human/Debug dummy was mistaken for a playable bot");

#if COLOURWARS_DEV_TOOLS
    const ReplayDirectory directory;
    ReplayFile replay;
    replay.settings = settings;
    rejects_bot_roster([&] { replay.write(directory.missing_path()); },
        "Replay serialization did not reject a bot roster");
    require(!std::filesystem::exists(directory.missing_path().parent_path()),
        "Rejected bot replay created a destination directory");

    const std::string sentinel = "Existing replay contents must survive rejection.\n";
    {
        std::ofstream stream(directory.existing_path(), std::ios::binary | std::ios::trunc);
        stream << sentinel;
        stream.close();
        require(stream.good(), "Cannot write the isolated replay sentinel");
    }
    rejects_bot_roster([&] { replay.write(directory.existing_path()); },
        "Replay serialization accepted a bot roster over an existing file");
    require(contents(directory.existing_path()) == sentinel,
        "Rejected bot replay changed an existing destination");
#endif
}
