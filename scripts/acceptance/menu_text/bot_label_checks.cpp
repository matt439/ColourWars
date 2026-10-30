#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"
#include "game/input_slots.h"
#include "game/states/player_label.h"

namespace menu_text_acceptance
{
    void check_bot_label_text(Variants& variants, World& world, const MenuText& defaults)
    {
        constexpr std::array difficulties{ BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard };
        constexpr std::array words{ L"Easy", L"Normal", L"Hard" };
        constexpr std::array roles{ "bot_easy", "bot_normal", "bot_hard" };
        for (size_t index = 0; index < difficulties.size(); ++index)
        {
            for (int seat = 1; seat < 4; ++seat)
            {
                require(defaults.bot_roster_label(seat, difficulties[index]) ==
                    L"P" + std::to_wstring(seat + 1) + L" - Bot (" + words[index] + L")",
                    "Default bot identity lost the participant number, kind or difficulty");
            }
            const std::string key = std::string("labels.") + roles[index];
            for (const char* invalid : { "", "   ", "first\nsecond", "{difficulty}", "unknown}" })
            {
                variants.rejects(key, [&](rapidjson::Document& d)
                    { d["menu_text"]["labels"][roles[index]].SetString(invalid, d.GetAllocator()); });
            }
            // Every difficulty must be drawable and fit the actual composed
            // Join/Team/Weapon/Results labels, even while setup remains gated.
            variants.rejects(key, [&](rapidjson::Document& d)
            {
                d["menu_text"]["labels"][roles[index]].SetString(std::string(2000, 'W').c_str(), d.GetAllocator());
            }, &world);
        }
        for (const char* malformed : { "Bot", "{Difficulty}", "{difficulty}{difficulty}", "{{difficulty}}", "{difficulty} {other}" })
        {
            variants.rejects("labels.bot", [&](rapidjson::Document& d)
                { d["menu_text"]["labels"]["bot"].SetString(malformed, d.GetAllocator()); });
        }
        const Presentation authored = variants.load([](rapidjson::Document& d)
        {
            auto& labels = d["menu_text"]["labels"];
            labels["bot"].SetString("{difficulty} AI");
            labels["bot_easy"].SetString("Calm");
            labels["bot_normal"].SetString("Steady");
            labels["bot_hard"].SetString("Sharp");
            labels["roster_row"].SetString("{device} / {player}");
        });
        authored.validate_resources(world.render, world.audio);
        constexpr std::array custom{ L"Calm", L"Steady", L"Sharp" };
        MatchSetup roster;
        roster.set_player_count(4);
        roster.begin_input_claims(input_slots::keyboard_and_mouse);
        for (int seat = 1; seat < 4; ++seat)
        {
            roster.set_player_bot(seat, difficulties[static_cast<size_t>(seat - 1)]);
            require(player_label(roster, seat, authored.menu_text()) ==
                std::wstring(custom[static_cast<size_t>(seat - 1)]) + L" AI / P" + std::to_wstring(seat + 1),
                "The roster ignored captured bot copy or token order");
        }
        for (const BotDifficulty invalid : { static_cast<BotDifficulty>(-1), static_cast<BotDifficulty>(3) })
        {
            bool rejected = false;
            try { defaults.bot_roster_label(1, invalid); }
            catch (const std::invalid_argument&) { rejected = true; }
            require(rejected, "Bot label accepted an unknown difficulty");
        }
        std::cout << "PASS bot labels: stable seats, all difficulties, authored templates and resource bounds\n";
    }
}
