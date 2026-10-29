#include "game/pch.h"
#include "game/input_slots.h"
#include "game/last_match_setup.h"
#include "game/objects/level_builder.h"
#include "game/presentation.h"
#include "game/match_setup.h"
#include "game/states/player_label.h"

#include <filesystem>
#include <stdexcept>
#include <string>

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }
}

void check_unsupported_bot_consumers(const std::filesystem::path& content)
{
    MatchSetup settings;
    settings.set_player_count(2);
    settings.begin_input_claims(input_slots::keyboard_and_mouse);
    settings.set_player_bot(1, BotDifficulty::normal);

    const Presentation presentation((content / "presentation.json").string().c_str());
    const MenuText& text = presentation.menu_text();
    constexpr BotDifficulty difficulties[]{ BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard };
    constexpr const wchar_t* words[]{ L"Easy", L"Normal", L"Hard" };
    for (size_t index = 0; index < std::size(difficulties); ++index)
    {
        settings.set_player_bot(1, difficulties[index]);
        require(player_label(settings, 1, text) == L"P2 - Bot (" + std::wstring(words[index]) + L")",
            "A bot label lost its stable participant number or difficulty");
    }
    require(player_label(settings, 0, text) == L"P1 - Keyboard/mouse",
        "Bot metadata changed the human host label");

    settings.set_player_human(1);
    settings.set_stage("legacy_debug_dummy");
    settings.set_screen_layout(labrador::ScreenLayout::two_player);
    settings.set_player_team(0, PlayerTeam::a);
    settings.set_player_team(1, PlayerTeam::b);
    const LastMatchSetup captured = LastMatchSetup::capture(settings);
    require(captured.players.size() == 2 && settings.player_settings()[1].num == -1 &&
        player_label(settings, 1, text) == L"P2 - Waiting to join",
        "An unclaimed human/Debug dummy was mistaken for a playable bot");
}
