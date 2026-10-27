#include "game/pch.h"
#include "game/objects/weapon_builder.h"
#include "game/objects/weapon_feedback.h"
#include "game/presentation.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/step_timer.h"

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

// POL#38: the advance low-paint warning. Every decision runs through the
// production weapon update; the fixture only reads its results.
using namespace labrador;
using namespace mattmath;

namespace
{
    void require(bool condition, const std::string& message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));

    constexpr WeaponType tools[] = { WeaponType::sprayer, WeaponType::sniper,
        WeaponType::roller, WeaponType::mister, WeaponType::bucket };
    constexpr const char* names[] = { "sprayer", "sniper", "roller", "mister", "bucket" };

    PlayerInputData input(bool held)
    {
        PlayerInputData result;
        result.connected = true;
        result.primary_shoot = held;
        result.shoot_direction_requested = true;
        result.shoot_direction = Vector2F::unit_vec_from_angle(0.0f);
        return result;
    }

    struct World
    {
        RenderResources render;
        AudioResources audio;
        PlaySettings preferences;
        GameTuning tuning;
        Presentation presentation;

        explicit World(const std::filesystem::path& content) :
            tuning((content / "tuning.json").string().c_str()),
            presentation((content / "presentation.json").string().c_str())
        {
            for (const char* sheet : { "sprite_sheet_1", "painter", "weapon_paint", "paint_fx", "terrain" })
            {
                this->render.add_sprite_sheet(sheet, read_sprite_sheet(
                    (content / "textures" / (std::string(sheet) + ".json")).string().c_str(),
                    TextureHandle{}));
            }
            this->audio.add_sound_bank("paint_exe", SoundBank::silent());
        }

        std::unique_ptr<Weapon> weapon(WeaponType type)
        {
            return WeaponBuilder::build_weapon(&this->tuning, PlayerTeam::a,
                2, 0, Colour::white, type, Vector2F::ZERO, &this->render, &this->audio,
                &this->preferences, &this->presentation.tool_motion(),
                &this->presentation.weapon_feedback());
        }
    };

    bool update(Weapon& weapon, bool held, float elapsed = dt)
    {
        return !weapon.update_and_get_projectiles(input(held), Vector2F::ZERO,
            Vector2F::ZERO, true, elapsed).empty();
    }

    void check_lines(World& world)
    {
        const LowPaintTuning& low = world.tuning.low_paint();
        const float full = world.tuning.starting_ammo();
        require(low.shots >= 2.0f && low.supply_fraction >= 0.0f && low.rearm_fraction >= 0.0f,
            "Shipped low-paint tuning is outside its validated meaning");
        for (const WeaponType type : tools)
        {
            const auto weapon = world.weapon(type);
            const float cost = world.tuning.weapon(type).details.ammo_usage;
            const float expected = std::min(full, std::max(low.supply_fraction * full, low.shots * cost));
            const float rearm = std::min(full, expected + std::max(low.rearm_fraction * full, cost));
            require(weapon->low_paint_rearm_level() == rearm && (rearm > expected || expected >= full),
                std::string(names[static_cast<size_t>(type)]) + " rearm line is not a band or a shot above the low line");
            require(weapon->low_paint_level() == expected,
                std::string(names[static_cast<size_t>(type)]) + " low line is not the larger of the tank share and shot reserve");
            require(!weapon->low_paint() && !weapon->low_paint_warned() && weapon->low_paint_elapsed() == 0.0f,
                "A full tank started low or warned");
            require(world.tuning.weapon(type).feedback_sounds.low.has_value(),
                std::string(names[static_cast<size_t>(type)]) + " has no low-paint sound");
        }
        std::cout << "PASS low paint: five-tool lines are the larger of the tank share and shot reserve; full tanks are quiet\n";
    }

    // Hold fire from full to dry: exactly one warning, raised by a paid shot
    // while another whole shot remains, then held until real recovery.
    void check_warning(World& world)
    {
        const float full = world.tuning.starting_ammo();
        for (const WeaponType type : tools)
        {
            const std::string name = names[static_cast<size_t>(type)];
            auto weapon = world.weapon(type);
            int warnings = 0;
            int shots_after_warning = 0;
            bool was_warned = false;
            float previous_elapsed = 0.0f;
            int tick = 0;
            for (; tick < 60 * 60 && weapon->readiness() != WeaponReadiness::empty; ++tick)
            {
                const float before = weapon->ammo();
                const bool shot = update(*weapon, true);
                if (weapon->low_paint_warned() && !was_warned)
                {
                    ++warnings;
                    require(shot && before >= weapon->low_paint_level() && weapon->low_paint(),
                        name + " warned without a paid shot crossing the low line");
                    require(weapon->readiness() != WeaponReadiness::empty,
                        name + " warned only after it could no longer afford a shot");
                }
                else if (was_warned && shot)
                {
                    ++shots_after_warning;
                }
                was_warned = weapon->low_paint_warned();
                if (weapon->low_paint())
                {
                    require(weapon->low_paint_elapsed() > previous_elapsed,
                        name + " low-paint age did not advance with simulated time");
                }
                else
                {
                    require(weapon->low_paint_elapsed() == 0.0f, name + " aged a warning while not low");
                }
                previous_elapsed = weapon->low_paint_elapsed();
            }
            require(weapon->readiness() == WeaponReadiness::empty, name + " never ran dry under held fire");
            require(warnings == 1 && shots_after_warning >= 1,
                name + " did not warn exactly once, at least one paid shot before exhaustion (warnings=" +
                std::to_string(warnings) + ", later shots=" + std::to_string(shots_after_warning) + ")");

            // Dry fire keeps the latch; it is not repeated by the empty cue.
            for (int held = 0; held < 120; ++held)
            {
                std::ignore = update(*weapon, true);
                require(weapon->low_paint_warned(), name + " dropped its warning while still low");
                if (weapon->low_paint() && weapon->readiness() != WeaponReadiness::empty) { break; }
            }

            // Release and recover. The latch survives until the rearm line,
            // even after the tool is no longer low.
            const float rearm = weapon->low_paint_rearm_level();
            bool left_low_while_warned = false;
            for (tick = 0; tick < 60 * 30 && weapon->ammo() < full; ++tick)
            {
                std::ignore = update(*weapon, false);
                if (weapon->ammo() < rearm)
                {
                    require(weapon->low_paint_warned(), name + " rearmed below its rearm line");
                    left_low_while_warned = left_low_while_warned || !weapon->low_paint();
                }
                else
                {
                    require(!weapon->low_paint_warned(), name + " stayed warned past its rearm line");
                }
            }
            require(weapon->ammo() == full && !weapon->low_paint() && weapon->low_paint_elapsed() == 0.0f,
                name + " did not recover to a full, quiet tank");
            require(rearm == weapon->low_paint_level() || left_low_while_warned || rearm >= full,
                name + " rearm band was never exercised");

            // Recovered fully, the next descent warns again, once.
            warnings = 0;
            was_warned = false;
            for (tick = 0; tick < 60 * 60 && weapon->readiness() != WeaponReadiness::empty; ++tick)
            {
                std::ignore = update(*weapon, true);
                if (weapon->low_paint_warned() && !was_warned) { ++warnings; }
                was_warned = weapon->low_paint_warned();
            }
            require(warnings == 1, name + " did not warn again exactly once after recovery");
        }
        std::cout << "PASS low paint: five tools warn once per descent with a paid shot still to come, hold through dry fire and rearm only past recovery\n";
    }

    // Partial recovery inside the rearm band must not let held fire chatter.
    void check_no_chatter(World& world)
    {
        for (const WeaponType type : tools)
        {
            const std::string name = names[static_cast<size_t>(type)];
            auto weapon = world.weapon(type);
            const float rearm = weapon->low_paint_rearm_level();
            int warnings = 0;
            bool was_warned = false;
            float highest_since_warning = 0.0f;
            for (int tick = 0; tick < 60 * 120; ++tick)
            {
                // Three press/gap rhythms, so some gaps end inside the rearm
                // band; the last (5 s each way) drains and refills every tool.
                const int period = tick < 2400 ? 50 : tick < 4800 ? 100 : 600;
                const bool held = (tick % period) < period / 2;
                std::ignore = update(*weapon, held);
                if (weapon->low_paint_warned() && !was_warned)
                {
                    require(warnings == 0 || highest_since_warning >= rearm,
                        name + " warned again without recovering to its rearm line");
                    ++warnings;
                    highest_since_warning = weapon->ammo();
                }
                highest_since_warning = std::max(highest_since_warning, weapon->ammo());
                was_warned = weapon->low_paint_warned();
            }
            require(warnings >= 2, name + " pulsed fire never exercised a second descent");
        }
        std::cout << "PASS low paint: pulsed fire only warns again after recovering past the rearm band\n";
    }

    // Unions taken in a different order can differ in the last bit of an edge.
    bool within(RectangleF outer, const RectangleF& inner)
    {
        outer.inflate(Vector2F(0.01f, 0.01f));
        return outer.contains(inner);
    }

    void check_refill_and_frozen(World& world)
    {
        for (const WeaponType type : tools)
        {
            const std::string name = names[static_cast<size_t>(type)];
            auto weapon = world.weapon(type);
            for (int tick = 0; tick < 60 * 60 && !weapon->low_paint_warned(); ++tick)
            {
                std::ignore = update(*weapon, true);
            }
            require(weapon->low_paint_warned() && weapon->low_paint(), name + " never warned for the refill check");
            const float elapsed = weapon->low_paint_elapsed();
            const float ammo = weapon->ammo();
            std::ignore = update(*weapon, true, 0.0f);
            require(weapon->low_paint_elapsed() == elapsed && weapon->low_paint_warned(),
                name + " frozen update aged or cleared the warning");
            require(weapon->ammo() <= ammo, name + " frozen update recovered paint");

            // The low tank joins the culling bounds only while it is shown.
            const Vector2F body(37.0f, -11.0f);
            weapon->set_player_center(body);
            const LowPaintCue cue = weapon->low_paint_cue();
            require(cue.low && cue.body_x == body.x && cue.level >= 0.0f && cue.level < 1.0f,
                name + " low cue does not describe the tank");
            const RectangleF quiet_bounds = weapon->feedback()->bounds(weapon->nozzle_pos(),
                weapon->draw_pos(), type);
            const RectangleF low_bounds = weapon->feedback()->bounds(weapon->nozzle_pos(),
                weapon->draw_pos(), type, cue);
            const auto& look = world.presentation.weapon_feedback();
            const Vector2F half = look.low_paint_size * 0.5f + Vector2F(look.keyline_width, look.keyline_width);
            const Vector2F centre = Vector2F(body.x,
                weapon->draw_pos().y - look.status_offset) + look.low_paint_offset;
            require(within(low_bounds, quiet_bounds) && within(low_bounds, RectangleF(centre - half, half * 2.0f)) &&
                within(weapon->draw_bounds(), low_bounds),
                name + " low tank escaped its weapon's draw bounds");

            weapon->reset_ammo();
            require(!weapon->low_paint() && !weapon->low_paint_warned() &&
                weapon->low_paint_elapsed() == 0.0f, name + " refill retained the warning");
        }
        std::cout << "PASS low paint: frozen updates keep the warning; refill clears it; the tank stays inside draw bounds\n";
    }

    std::string read_file(const std::filesystem::path& path)
    {
        std::ifstream stream(path, std::ios::binary);
        require(stream.good(), "Cannot read " + path.string());
        return std::string(std::istreambuf_iterator<char>(stream), {});
    }

    // Writes an edited copy beside nothing else and expects its loader to
    // reject it with a diagnostic naming `where`.
    void reject(const std::string& source, const std::function<void(rapidjson::Document&)>& edit,
        const std::function<void(const std::filesystem::path&)>& load, const char* where,
        const char* label)
    {
        rapidjson::Document json;
        json.Parse(source.c_str());
        require(!json.HasParseError(), "Cannot parse shipped content");
        edit(json);
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        json.Accept(writer);
        wchar_t temporary_name[MAX_PATH]{};
        const auto directory = std::filesystem::temp_directory_path();
        require(GetTempFileNameW(directory.c_str(), L"cwl", 0, temporary_name) != 0,
            "Cannot reserve a low-paint content variant");
        const std::filesystem::path path(temporary_name);
        {
            std::ofstream variant(path, std::ios::binary);
            variant.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
        }
        bool rejected = false;
        try { load(path); }
        catch (const std::exception& error)
        {
            rejected = std::string(error.what()).find(where) != std::string::npos;
        }
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
        require(rejected, std::string("Malformed low-paint content was accepted: ") + label);
    }

    void check_validation(const std::filesystem::path& content)
    {
        const std::string tuning = read_file(content / "tuning.json");
        const auto load_tuning = [](const std::filesystem::path& path) { GameTuning loaded(path.string().c_str()); };
        const auto low = [](rapidjson::Document& json) -> rapidjson::Value& { return json["weapons"]["low_paint"]; };
        reject(tuning, [&](auto& json) { low(json)["shots"].SetFloat(1.5f); }, load_tuning, "low_paint", "one-shot reserve");
        reject(tuning, [&](auto& json) { low(json)["supply_fraction"].SetFloat(1.5f); }, load_tuning, "low_paint", "supply above a tank");
        reject(tuning, [&](auto& json) { low(json)["supply_fraction"].SetFloat(-0.1f); }, load_tuning, "low_paint", "negative supply");
        reject(tuning, [&](auto& json) { low(json)["rearm_fraction"].SetFloat(2.0f); }, load_tuning, "low_paint", "rearm above a tank");
        reject(tuning, [&](auto& json) { low(json).RemoveMember("shots"); }, load_tuning, "low_paint", "missing shots");
        reject(tuning, [&](auto& json) { json["weapons"].RemoveMember("low_paint"); }, load_tuning, "weapons", "missing block");
        reject(tuning, [&](auto& json) { json["weapons"]["sniper"]["feedback_sounds"]["low"]["volume"].SetFloat(2.0f); },
            load_tuning, "low", "sound gain above one");

        const std::string presentation = read_file(content / "presentation.json");
        const auto load_presentation = [](const std::filesystem::path& path) { Presentation loaded(path.string().c_str()); };
        const auto tank = [](rapidjson::Document& json) -> rapidjson::Value& { return json["weapon_feedback"]["low_paint"]; };
        const auto tag = [](rapidjson::Document& json) -> rapidjson::Value& { return json["hud"]["low_paint"]; };
        reject(presentation, [&](auto& json) { tank(json)["size"]["x"].SetFloat(1.0f); }, load_presentation, "low_paint", "tank without fill");
        reject(presentation, [&](auto& json) { tank(json)["size"]["y"].SetFloat(100.0f); }, load_presentation, "low_paint", "oversized tank");
        reject(presentation, [&](auto& json) { tank(json)["offset"]["x"].SetFloat(200.0f); }, load_presentation, "low_paint", "detached tank");
        reject(presentation, [&](auto& json) { tank(json)["blink_period"].SetFloat(0.05f); }, load_presentation, "low_paint", "strobing tank");
        reject(presentation, [&](auto& json) { tank(json)["dim_alpha"].SetFloat(1.0f); }, load_presentation, "low_paint", "tank that never dims");
        reject(presentation, [&](auto& json) { tag(json)["tag"].SetString("FFF5DF"); }, load_presentation, "low_paint", "tag matching its text");
        reject(presentation, [&](auto& json) { tag(json)["text"].SetString("FFF5DF80"); }, load_presentation, "low_paint", "translucent text");
        reject(presentation, [&](auto& json) { tag(json)["padding"]["y"].SetFloat(-1.0f); }, load_presentation, "low_paint", "negative padding");
        reject(presentation, [&](auto& json) { tag(json)["blink_period"].SetFloat(9.0f); }, load_presentation, "low_paint", "slow tag blink");
        std::cout << "PASS low paint content: tuning lines, reserve, rearm and gain; tank and tag geometry, blink and contrast are validated\n";
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: low_paint.exe <content directory>");
        const std::filesystem::path content(argv[1]);
        World world{ content };
        check_lines(world);
        check_warning(world);
        check_no_chatter(world);
        check_refill_and_frozen(world);
        check_validation(content);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL low paint: " << error.what() << '\n';
        return 1;
    }
}
