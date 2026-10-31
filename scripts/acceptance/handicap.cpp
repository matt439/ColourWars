#include "game/pch.h"
#include "scripts/acceptance/paint_underfoot/fixtures.h"
#include "scripts/acceptance/bot_navigation/fixtures.h"
#include "game/ai/match_bot.h"
#include "game/ai/match_bot_driver.h"
#include "game/input_slots.h"
#include "game/objects/player.h"
#include "game/objects/projectile_spray.h"
#include "game/objects/weapon_builder.h"
#include "game/presentation.h"
#include "game/match_setup.h"
#include "engine/assets/json.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/file_path.h"
#include "scripts/acceptance/support/probe.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

// ROAD#5: per-seat handicaps. Tuning shape and validation, the roster field,
// the Tools page copy, and the two match effects on a real Player: damage
// taken and paint refill. Level 0 must leave both bit-for-bit unchanged.
// ROAD#39: Boost on paint underfoot, on a test copy of the tuning with the
// underfoot trial enabled. The shipped tuning is not changed.

using namespace labrador;
using namespace mattmath;

namespace
{
    using acceptance::require;

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
        MatchSetup roster;
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
        const MatchSetup copy = roster;
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

    // ROAD#39. The refill a weapon receives is the underfoot multiplier times
    // the level's paint_refill. Built from an enabled copy of the tuning at
    // the shipped trial amounts, through actual floor paint and contacts.
    namespace underfoot = paint_underfoot_acceptance;

    std::uint32_t bits(float value) { return std::bit_cast<std::uint32_t>(value); }

    // A player of team A standing on a floor painted by `floor_team` (or
    // unpainted for none), with the following step's support consumed.
    struct Stand
    {
        Stand(underfoot::World& world, PlayerTeam floor_team, int level) : course(world)
        {
            auto& floor = course.surface();
            if (floor_team != PlayerTeam::none) { world.paint(floor, floor.shape()->bounding_box(), floor_team, -1); }
            course.settle();
            course.tick();
            require(!world.tuning.player().paint_underfoot.enabled ||
                course.player->paint_underfoot_state().step_team == floor_team,
                "The Boost-underfoot fixture did not establish its floor's paint under the player");
            course.player->set_handicap(level);
        }
        underfoot::Course course;
    };

    void check_boost_underfoot(const std::filesystem::path& content)
    {
        underfoot::World world(content);
        require(!world.tuning.player().paint_underfoot.enabled,
            "Paint underfoot ships enabled; this check expects to enable it on a test copy");
        // Off: the shipped tuning gives paint_refill alone on any floor, bit for bit.
        for (const PlayerTeam floor : { PlayerTeam::none, PlayerTeam::a, PlayerTeam::b })
        {
            for (int level = 0; level < handicaps::count; ++level)
            {
                Stand stand(world, floor, level);
                require(bits(stand.course.player->ammo_regen_multiplier()) ==
                    bits(world.tuning.handicap(level).paint_refill),
                    "With paint underfoot off, Boost refill was not paint_refill alone");
            }
        }
        world.configure(true);
        const PaintUnderfootTuning& effect = world.tuning.player().paint_underfoot;
        require(effect.enabled && effect.own_ammo_regen_multiplier > 1.0f && effect.enemy_ammo_regen_multiplier == 0.0f,
            "The test tuning copy lost the trial's own boost or enemy suppression");
        for (int level = 0; level < handicaps::count; ++level)
        {
            const float refill = world.tuning.handicap(level).paint_refill;
            {
                Stand stand(world, PlayerTeam::a, level);
                const float expected = effect.own_ammo_regen_multiplier * refill;
                require(bits(stand.course.player->ammo_regen_multiplier()) == bits(expected),
                    "Own paint did not give its multiplier times the level's paint_refill");
                if (level == 0)
                {
                    require(bits(stand.course.player->ammo_regen_multiplier()) ==
                        bits(effect.own_ammo_regen_multiplier), "Boost Off changed the own-paint refill");
                }
            }
            {
                Stand stand(world, PlayerTeam::b, level);
                const float actual = stand.course.player->ammo_regen_multiplier();
                require(bits(actual) == bits(effect.enemy_ammo_regen_multiplier * refill) && bits(actual) == bits(0.0f),
                    "Enemy paint's zero refill did not stay zero under Boost");
            }
            {
                Stand stand(world, PlayerTeam::none, level);
                require(bits(stand.course.player->ammo_regen_multiplier()) == bits(refill),
                    "Off paint did not give the level's paint_refill alone");
            }
            for (const PlayerTeam floor : { PlayerTeam::a, PlayerTeam::b })
            {
                Stand stand(world, floor, level);
                Player& player = *stand.course.player;
                // An actual enemy projectile, not the debug kill(), so the
                // fatal path is the one a match takes.
                const auto& jet = world.tuning.projectile(ProjectileType::jet);
                require(jet.details.player_damage > 0.0f, "The Boost-underfoot fatal fixture needs positive projectile damage");
                ProjectileJet hit(player.rectangle(), Vector2F::ZERO, PlayerTeam::b, 1, labrador::Colour::white,
                    &world.render, resolve_projectile_strip(jet, world.render), &jet);
                for (int hit_count = 0; hit_count < 1000 && player.health() > 0.0f; ++hit_count)
                { player.on_contact(hit, Vector2F::ZERO, 0.0f); }
                require(player.health() <= 0.0f && player.state() == PlayerState::alive,
                    "The Boost-underfoot fatal fixture did not reach zero health through actual hits");
                require(bits(player.ammo_regen_multiplier()) == bits(refill),
                    "A fatally hit player on paint did not get the level's paint_refill alone");
                stand.course.tick();
                require(player.state() != PlayerState::alive && bits(player.ammo_regen_multiplier()) == bits(refill),
                    "A dead player on paint did not get the level's paint_refill alone");
            }
        }
    }

    // A Hard sprayer bot placed riding its side's paintable mover on the
    // explicit Turbulence mover stage. Each ride finishing window is budgeted
    // from the bot's observation; once the bot is refilling, that budget
    // counts the refill at the observation's paint_refill. The window length
    // therefore shows which paint_refill the driver observed for the seat.
    namespace navigation = bot_navigation_acceptance;

    MatchSetup ride_roster(int level)
    {
        MatchSetup settings;
        settings.set_player_count(2);
        settings.set_player_bot(1, BotDifficulty::hard);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_screen_layout(ScreenLayout::one_player);
        settings.set_stage("turbulence_mover_trial"); settings.set_duration(120.0f);
        settings.set_player_team(0, PlayerTeam::b); settings.set_player_team(1, PlayerTeam::a);
        settings.set_player_weapon(0, WeaponType::sprayer); settings.set_player_weapon(1, WeaponType::sprayer);
        settings.set_player_handicap(1, level);
        return settings;
    }

    // The actual driver at Level's input boundary, reading the bot's public
    // finishing state either side of its step and the ammo it observes.
    class RideProbe final : public SeatInputFilter
    {
    public:
        RideProbe(const GameTuning& tuning, const MatchSetup& settings) : driver(&tuning, settings, 7751) {}
        bool replaces_bot_inputs() const override { return true; }
        void filter_seats(std::vector<PlayerInputData>& seats, const Level& level) override
        {
            before = driver.bot(1)->finishing();
            ammo = level.player(1).weapon_ammo();
            driver.drive(seats, level.bot_world());
            after = driver.bot(1)->finishing();
            seats[0] = {}; seats[0].connected = true;
        }
        MatchBotDriver driver;
        bool before = false, after = false;
        float ammo = 0.0f;
    };

    // MatchBot::open_finish's budget for a refilling sprayer, from the same
    // observed values. finishing() reads one step short of it, because the
    // opening step already spends one.
    int refilling_window(const GameTuning& tuning, float observed_ammo, float paint_refill)
    {
        const double ammo = observed_ammo / tuning.starting_ammo();
        const double per_step = static_cast<double>(tuning.ammo_regen_rate()) * paint_refill /
            tuning.starting_ammo() / 60.0;
        const double refill = std::ceil(static_cast<double>(tuning.ammo_regen_delay()) * 60.0) +
            std::ceil(std::max(0.0, static_cast<double>(tuning.bots().reserve_resume) - ammo) / per_step);
        const double budget = refill + std::ceil(static_cast<double>(
            tuning.weapon(WeaponType::sprayer).details.shoot_interval) * 60.0) + tuning.bots().finish_dwell_steps;
        return static_cast<int>(std::min(budget, 3600.0)) - 1;
    }

    // Returns the first refilling window's length at this tuning and level.
    int observe_refilling_window(navigation::World& world, int level)
    {
        const MatchSetup settings = ride_roster(level);
        std::unique_ptr<Level> match = world.build(settings);
        const std::vector<PlayerInputData> physical = navigation::physical_inputs();
        for (int tick = 0; tick < 900 && match->state() != LevelState::active; ++tick)
        { match->update(physical, navigation::dt); }
        require(match->state() == LevelState::active, "The Boost bot fixture did not finish its countdown");
        require(match->player(1).handicap() == level, "The Boost bot seat lost its roster handicap");
        std::optional<RectangleF> mover;
        for (const MatchBotSupport& support : match->bot_supports())
        {
            if (support.one_way && support.paintable && support.bounds.width == 320.0f &&
                support.bounds.height == 20.0f && support.bounds.center().x < 2000.0f) { mover = support.bounds; }
        }
        require(mover.has_value(), "The Boost bot fixture lost Team A's mover");
        const Vector2F size = world.assets.tuning.player().size;
        require(match->place_player(1, Vector2F(mover->center().x - size.x * 0.5f, mover->top() - size.y)),
            "The Boost bot fixture could not place its bot on the mover");
        RideProbe probe(world.assets.tuning, settings);
        int open = -1;
        float open_ammo = 0.0f;
        for (int tick = 0; tick < 1200; ++tick)
        {
            match->update(physical, navigation::dt, &probe);
            if (!probe.before && probe.after) { open = tick; open_ammo = probe.ammo; }
            if (probe.before && !probe.after && open >= 0 &&
                open_ammo / world.assets.tuning.starting_ammo() <= world.assets.tuning.bots().reserve_low)
            {
                const int length = tick - open;
                for (int other = 0; other < handicaps::count; ++other)
                {
                    const int expected = refilling_window(world.assets.tuning, open_ammo,
                        world.assets.tuning.handicap(other).paint_refill);
                    require((length == expected) == (other == level),
                        "A boosted bot's refilling window did not use its own level's paint_refill");
                }
                std::cout << "MEASURE boost bot underfoot=" << world.assets.tuning.player().paint_underfoot.enabled
                    << " level=" << level << " observed_ammo=" << open_ammo << " window=" << length << '\n';
                return length;
            }
        }
        throw std::runtime_error("The Boost bot fixture never opened a refilling finishing window");
    }

    void check_bot_refill(const std::filesystem::path& content)
    {
        navigation::World world(content, "handicap-bot", "Boost bot fixture needs authored support '",
            "turbulence_mover_trial");
        underfoot::World enabled(content);
        enabled.configure(true);
        for (const bool trial : { false, true })
        {
            if (trial) { world.assets.tuning = enabled.tuning; }
            require(world.assets.tuning.player().paint_underfoot.enabled == trial,
                "The Boost bot fixture did not use the intended tuning copy");
            for (int level = 0; level < handicaps::count; ++level) { observe_refilling_window(world, level); }
        }
    }
}

int wmain(int argc, wchar_t* argv[])
{
    return acceptance::run_probe("handicap", [&]
    {
        require(argc == 2, "usage: handicap <content>");
        const std::filesystem::path content = std::filesystem::absolute(argv[1]);
        World world(content);
        check_tuning(content, world.tuning);
        check_roster();
        check_labels(world.presentation.menu_text());
        check_match_effects(world);
        check_boost_underfoot(content);
        check_bot_refill(content);
        std::cout << "PASS handicap: neutral none and monotonic shipped boosts, malformed tuning refused with context, "
            "roster field kept through bot/human, difficulty, resize, shuffle, copy and claims, invalid levels refused, "
            "Tools page copy and prompts, damage scaled bit-exactly per level (none unchanged), refill multiplier, "
            "kept through respawn; Boost on paint underfoot (test tuning copy): own and enemy multiplier times "
            "paint_refill bit-exact per level, enemy zero kept, off paint and dead give paint_refill alone, Off unchanged; "
            "a boosted bot's refilling finish window uses its level's paint_refill, underfoot off and on\n";
        return 0;
    });
}
