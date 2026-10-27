#include "game/pch.h"
#include "game/input_slots.h"
#include "game/objects/player.h"
#include "game/objects/projectile_spray.h"
#include "game/objects/weapon_builder.h"
#include "game/presentation.h"
#include "game/states/menu_level_settings.h"
#include "engine/assets/json.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/file_path.h"
#include <bit>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

// ROAD#5: per-seat handicaps. Tuning shape and validation, the roster field,
// the Tools page copy, and the two match effects on a real Player: damage
// taken and paint refill. Level 0 must leave both bit-for-bit unchanged.

using namespace labrador;
using namespace mattmath;

namespace
{
    void require(bool condition, const char* reason)
    {
        if (!condition) { throw std::runtime_error(reason); }
    }

    template<class Action> bool throws_invalid(Action action)
    {
        try { action(); }
        catch (const std::invalid_argument&) { return true; }
        return false;
    }

    class World final
    {
    public:
        explicit World(const std::filesystem::path& content) :
            tuning(path_to_utf8(content / "tuning.json").c_str()),
            presentation(path_to_utf8(content / "presentation.json").c_str())
        {
            for (const char* name : { "sprite_sheet_1", "painter", "weapon_paint", "paint_fx", "terrain" })
            {
                render.add_sprite_sheet(name, read_sprite_sheet(path_to_utf8(content / "textures" /
                    path_from_utf8(std::string(name) + ".json")).c_str(), TextureHandle{}));
            }
            audio.add_sound_bank("paint_exe", SoundBank::silent());
        }
        std::unique_ptr<Player> player(int handicap)
        {
            const RectangleF rectangle(0.0f, 0.0f, tuning.player().size.x, tuning.player().size.y);
            auto weapon = WeaponBuilder::build_weapon(&tuning, PlayerTeam::a, paint_owners::index(-1, 0),
                0, Colour::white, WeaponType::sprayer, rectangle.center(), &render, &audio, &preferences);
            auto result = std::make_unique<Player>(rectangle, &tuning, &presentation.movement_feedback(),
                &presentation.combat_feedback(), &presentation.respawn_redraw(), &render, &audio,
                &preferences, -1, PlayerTeam::a, WeaponType::sprayer, std::move(weapon), WeaponType::none,
                Colour::white, Viewport{}, rectangle.top_left());
            result->set_handicap(handicap);
            return result;
        }
        std::unique_ptr<ProjectileSpray> enemy_shot()
        {
            return std::make_unique<ProjectileSpray>(RectangleF(0, 0, 200, 200), Vector2F::ZERO, PlayerTeam::b,
                paint_owners::index(-1, 1), Colour::white, &render,
                resolve_projectile_strip(tuning.projectile(ProjectileType::spray), render),
                &tuning.projectile(ProjectileType::spray));
        }
        GameTuning tuning;
        Presentation presentation;
        RenderResources render;
        AudioResources audio;
        PlaySettings preferences;
    };

    void check_tuning(const std::filesystem::path& content, const GameTuning& tuning)
    {
        const HandicapTuning& none = tuning.handicap(0);
        require(none.damage_taken == 1.0f && none.paint_refill == 1.0f, "Handicap none is not neutral");
        for (int level = 1; level < handicaps::count; ++level)
        {
            const HandicapTuning& below = tuning.handicap(level - 1);
            const HandicapTuning& boost = tuning.handicap(level);
            require(boost.damage_taken > 0.0f && boost.damage_taken <= below.damage_taken &&
                boost.paint_refill >= below.paint_refill, "A shipped handicap level boosts less than the one below");
        }
        require(tuning.handicap(handicaps::count - 1).damage_taken < 1.0f &&
            tuning.handicap(handicaps::count - 1).paint_refill > 1.0f, "The top shipped handicap does nothing");
        for (const int level : { -1, handicaps::count })
        {
            bool rejected = false;
            try { std::ignore = tuning.handicap(level); }
            catch (const std::out_of_range&) { rejected = true; }
            require(rejected, "An unknown handicap level resolved to tuning");
        }

        std::ifstream input(content / "tuning.json", std::ios::binary);
        const std::string source{ std::istreambuf_iterator<char>(input), {} };
        const auto rejects = [&](const char* expected, const auto& edit)
        {
            rapidjson::Document document; document.Parse(source.c_str());
            require(!document.HasParseError(), "Cannot parse handicap tuning source fixture");
            edit(document);
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer); document.Accept(writer);
            bool rejected = false;
            try { const GameTuning invalid(labrador::parse_json(buffer.GetString(), "handicap-invalid")); }
            catch (const std::runtime_error& error)
            {
                const std::string message = error.what();
                rejected = message.find("handicap-invalid") != std::string::npos &&
                    message.find(expected) != std::string::npos;
            }
            require(rejected, "Handicap tuning accepted malformed data or lost source/field context");
        };
        rejects("handicaps", [](auto& d) { d.RemoveMember("handicaps"); });
        for (const char* level : { "none", "plus_one", "plus_two" })
        {
            rejects(level, [&](auto& d) { d["handicaps"].RemoveMember(level); });
            rejects("damage_taken", [&](auto& d) { d["handicaps"][level]["damage_taken"].SetDouble(0.0); });
            rejects("damage_taken", [&](auto& d) { d["handicaps"][level]["damage_taken"].SetDouble(1.5); });
            rejects("paint_refill", [&](auto& d) { d["handicaps"][level]["paint_refill"].SetDouble(0.9); });
            rejects("paint_refill", [&](auto& d) { d["handicaps"][level]["paint_refill"].SetDouble(1e300); });
        }
        rejects("none", [](auto& d) { d["handicaps"]["none"]["damage_taken"].SetDouble(0.9); });
        rejects("none", [](auto& d) { d["handicaps"]["none"]["paint_refill"].SetDouble(1.1); });
        rejects("plus_two", [](auto& d) { d["handicaps"]["plus_two"]["damage_taken"].SetDouble(0.95); });
        rejects("plus_two", [](auto& d) { d["handicaps"]["plus_two"]["paint_refill"].SetDouble(1.0); });
    }

    void check_roster()
    {
        MenuLevelSettings roster;
        roster.set_player_count(2);
        roster.begin_input_claims(input_slots::keyboard_and_mouse);
        require(roster.player_handicap(0) == 0 && roster.player_handicap(1) == 0, "A new seat starts handicapped");
        roster.set_player_handicap(1, 2);
        roster.set_player_bot(1, BotDifficulty::hard);
        require(roster.player_handicap(1) == 2, "Becoming a bot dropped the seat's handicap");
        roster.set_player_bot(1, BotDifficulty::easy);
        roster.set_player_human(1);
        require(roster.player_handicap(1) == 2, "Changing difficulty or back to human dropped the handicap");
        for (const int invalid : { -1, handicaps::count })
        {
            require(throws_invalid([&] { roster.set_player_handicap(0, invalid); }) && roster.player_handicap(0) == 0,
                "An invalid handicap was accepted or changed the seat");
        }
        bool out_of_roster = false;
        try { roster.set_player_handicap(2, 1); }
        catch (const std::out_of_range&) { out_of_roster = true; }
        require(out_of_roster, "A handicap was set outside the roster");
        roster.set_player_count(4);
        require(roster.player_handicap(1) == 2 && roster.player_handicap(3) == 0,
            "Growing the roster changed a kept seat or handicapped a new one");
        require(roster.claim_input_slot(0) && roster.claim_input_slot(1) && roster.claim_input_slot(2),
            "Cannot complete the handicap roster");
        roster.shuffle_teams();
        const MenuLevelSettings copy = roster;
        require(copy.player_handicap(1) == 2, "Shuffle or copy dropped a handicap");
        roster.begin_input_claims(3);
        require(roster.player_handicap(1) == 2, "Fresh device claims dropped a handicap");
    }

    void check_labels(const MenuText& text)
    {
        require(text.handicap_label(0) == L"Boost: Off" && text.handicap_label(1) == L"Boost: +1" &&
            text.handicap_label(2) == L"Boost: +2", "Default handicap copy changed");
        require(throws_invalid([&] { std::ignore = text.handicap_label(-1); }) &&
            throws_invalid([&] { std::ignore = text.handicap_label(handicaps::count); }),
            "An unknown handicap produced a label");
        require(text.prompt(MenuPrompt::weapon).find(L"X/R Boost") != std::wstring::npos &&
            text.prompt(MenuPrompt::bot_weapon).find(L"X/R Boost") != std::wstring::npos,
            "Weapon Select prompts do not offer the handicap button");
    }

    void check_match_effects(World& world)
    {
        const float damage = world.tuning.projectile(ProjectileType::spray).details.player_damage;
        auto shot = world.enemy_shot();
        for (int level = 0; level < handicaps::count; ++level)
        {
            auto player = world.player(level);
            require(player->handicap() == level, "The player did not keep its handicap");
            const float before = player->health();
            player->on_contact(*shot, Vector2F::ZERO, 0.0f);
            const float expected = before - damage * world.tuning.handicap(level).damage_taken;
            require(std::bit_cast<std::uint32_t>(player->health()) == std::bit_cast<std::uint32_t>(expected),
                "A hit did not scale by the handicap's damage_taken");
            if (level == 0)
            {
                require(std::bit_cast<std::uint32_t>(player->health()) ==
                    std::bit_cast<std::uint32_t>(before - damage), "No handicap changed the damage arithmetic");
            }
            require(player->ammo_regen_multiplier() == world.tuning.handicap(level).paint_refill,
                "Paint refill did not scale by the handicap off paint");
            player->force_respawn();
            require(player->handicap() == level, "Respawning dropped the handicap");
        }
        auto player = world.player(1);
        require(throws_invalid([&] { player->set_handicap(handicaps::count); }) && player->handicap() == 1,
            "An invalid handicap was accepted or changed the player");
        // Two hits on +2 take less than two on none, which is the point.
        auto plain = world.player(0);
        auto boosted = world.player(handicaps::count - 1);
        for (int hit = 0; hit < 2; ++hit)
        {
            plain->on_contact(*shot, Vector2F::ZERO, 0.0f);
            boosted->on_contact(*shot, Vector2F::ZERO, 0.0f);
        }
        require(boosted->health() > plain->health(), "The top handicap did not help survival");
    }
}

int wmain(int argc, wchar_t* argv[])
{
    try
    {
        require(argc == 2, "usage: handicap <content>");
        const std::filesystem::path content = std::filesystem::absolute(argv[1]);
        World world(content);
        check_tuning(content, world.tuning);
        check_roster();
        check_labels(world.presentation.menu_text());
        check_match_effects(world);
        std::cout << "PASS handicap: neutral none and monotonic shipped boosts, malformed tuning refused with context, "
            "roster field kept through bot/human, difficulty, resize, shuffle, copy and claims, invalid levels refused, "
            "Tools page copy and prompts, damage scaled bit-exactly per level (none unchanged), refill multiplier, "
            "kept through respawn\n";
        return 0;
    }
    catch (const std::exception& error) { std::cerr << "FAIL handicap: " << error.what() << '\n'; return 1; }
}
