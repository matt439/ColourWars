#include "game/pch.h"
#include "game/input_slots.h"
#include "game/objects/level_builder.h"
#include "game/objects/weapon_feedback.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/step_timer.h"
#include "engine/render/sprite_font_file.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <type_traits>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace labrador;
using namespace mattmath;

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));

    struct World
    {
        RenderResources render;
        AudioResources audio;
        PlaySettings preferences;
        GameTuning tuning;
        Presentation presentation;
        Registry<LevelLoadedInfo> levels{ "Level" };
        ResolutionManager resolution;
        ViewportManager viewports{ &resolution };
        labrador::Mouse mouse;
        Partitioner partitioner;
        std::unique_ptr<LevelBuilder> builder;

        explicit World(const std::filesystem::path& content) :
            tuning((content / "tuning.json").string().c_str()),
            presentation((content / "presentation.json").string().c_str())
        {
            for (const char* sheet : { "sprite_sheet_1", "painter", "menu_paint", "weapon_paint", "paint_fx", "terrain" })
            {
                this->render.add_sprite_sheet(sheet, read_sprite_sheet(
                    (content / "textures" / (std::string(sheet) + ".json")).string().c_str(), TextureHandle{}));
            }
            for (const char* font : { "bowlby_one_144", "lato_black_24",
                "lato_black_36", "lato_black_48", "lato_black_72",
                "lato_black_144", "courier_prime_bold_16" })
            {
                const SpriteFontFile file = read_sprite_font_file(
                    (content / "fonts" / (std::string(font) + ".spritefont")).string());
                auto loaded = std::make_unique<Font>(TextureHandle{}, file.glyphs, file.line_spacing);
                if (loaded->contains(U'?')) { loaded->set_stand_in(U'?'); }
                this->render.add_font(font, std::move(loaded));
            }
            this->audio.add_sound_bank("paint_exe", SoundBank::silent());
            auto info = std::make_unique<LevelLoadedInfo>(
                (content / "levels/movement_course.json").string().c_str());
            info->validate();
            this->levels.add("movement_course", std::move(info));
            this->builder = std::make_unique<LevelBuilder>(&this->viewports, &this->render,
                &this->audio, &this->preferences, &this->levels, &this->tuning,
                &this->presentation, &this->resolution, nullptr, &this->partitioner, &this->mouse);
        }

        std::unique_ptr<Level> build(WeaponType first = WeaponType::sprayer,
            OpeningMode opening = OpeningMode::off, float duration = 300.0f)
        {
            MenuLevelSettings settings;
            settings.set_stage("movement_course");
            settings.set_player_count(4);
            settings.set_screen_layout(ScreenLayout::four_player);
            settings.set_duration(duration);
            const std::array tools{ first, WeaponType::sniper, WeaponType::mister, WeaponType::bucket };
            constexpr std::array slots{ 3, 1, 2, 0 };
            settings.begin_input_claims(slots[0]);
            for (int view = 1; view < 4; ++view)
            {
                require(settings.claim_input_slot(slots.at(static_cast<size_t>(view))), "Could not claim sparse aim-marker slot");
            }
            for (int view = 0; view < 4; ++view)
            {
                settings.set_player_team(view, PlayerTeam::a);
                settings.set_player_weapon(view, tools.at(static_cast<size_t>(view)));
            }
            this->viewports.set_layout(settings.screen_layout());
            return this->builder->build_level(settings, TeamColourPair::blue_orange, opening);
        }
    };

    class ProbeWeapon final : public Weapon
    {
    public:
        using Weapon::Weapon;
        void prime(float paint, float cooldown)
        {
            this->set_ammo(paint);
            this->set_ammo_timer(0.0f);
            this->set_shoot_timer(cooldown);
        }
    };

    PlayerInputData input(bool fire = false, float angle = 0.0f)
    {
        PlayerInputData result;
        result.connected = true;
        result.primary_shoot = fire;
        result.shoot_direction_requested = true;
        result.shoot_angle = angle;
        result.shoot_direction = Vector2F::unit_vec_from_angle(angle);
        return result;
    }

    auto update(Weapon& weapon, const PlayerInputData& actions, float elapsed = 0.0f)
    {
        return weapon.update_and_get_projectiles(actions, Vector2F::ZERO,
            Vector2F::ZERO, true, elapsed);
    }

    void check_readiness(World& world)
    {
        for (const auto type : { WeaponType::sprayer, WeaponType::sniper,
            WeaponType::roller, WeaponType::mister, WeaponType::bucket })
        {
            ProbeWeapon weapon(&world.tuning, PlayerTeam::a, 3, 0, Colour::white,
                type, Vector2F::ZERO, &world.render, &world.audio, &world.preferences);
            weapon.set_feedback_presentation(&world.presentation.weapon_feedback());
            const auto& details = world.tuning.weapon(type).details;
            const float interval = details.shoot_interval;
            const float ready = std::nextafter(interval, std::numeric_limits<float>::infinity());
            const float cost = details.ammo_usage;
            for (const float paint : { cost, std::nextafter(cost, 0.0f),
                std::nextafter(cost, std::numeric_limits<float>::infinity()) })
            {
                weapon.prime(paint, ready);
                require(weapon.readiness() == WeaponReadiness::ready && weapon.ready_to_shoot(),
                    "Readiness disagreed with affordable full-shot rounding boundary");
                require(update(weapon, input(false)).empty() && weapon.ammo() == paint &&
                    weapon.readiness() == WeaponReadiness::ready,
                    "Released trigger changed readiness or spent paint");
                require(!update(weapon, input(true)).empty() && !weapon.ready_to_shoot() &&
                    weapon.readiness() == WeaponReadiness::empty,
                    "Final paid shot retained readiness or cooldown overrode empty state");
            }
            for (const float cooldown : { 0.0f, interval,
                std::nextafter(interval, -std::numeric_limits<float>::infinity()) })
            {
                weapon.prime(world.tuning.starting_ammo(), cooldown);
                require(weapon.readiness() == WeaponReadiness::recovering && !weapon.ready_to_shoot() &&
                    update(weapon, input(true)).empty(), "Readiness bypassed strict cooldown boundary");
            }
            weapon.prime(world.tuning.starting_ammo(), ready);
            require(weapon.readiness() == WeaponReadiness::ready && !update(weapon, input(true)).empty(),
                "First representable cooldown above the interval was not ready");
            for (const float paint : { 0.0f, cost * 0.5f, cost * 0.999f })
            {
                for (const float cooldown : { 0.0f, interval, ready })
                {
                    weapon.prime(paint, cooldown);
                    require(weapon.readiness() == WeaponReadiness::empty && !weapon.ready_to_shoot() &&
                        update(weapon, input(true)).empty() && weapon.ammo() == paint,
                        "Insufficient paint did not take precedence over cooldown or refused shot spent paint");
                }
            }
        }
        std::cout << "PASS aim marker: five real weapon gates, exact/adjacent affordability, strict cooldown, empty precedence and released fire\n";
    }

    void check_bounds(World& world)
    {
        static_assert(std::is_same_v<decltype(&Weapon::draw_aim_marker), void (Weapon::*)(DrawList&) const>);
        static_assert(std::is_same_v<decltype(&Weapon::readiness), WeaponReadiness (Weapon::*)() const>);
        const auto& look = world.presentation.weapon_feedback();
        WeaponFeedback feedback(&look, &world.render);
        const auto initial = feedback.state();
        const Vector2F nozzle(123.0f, -87.0f);
        for (int direction = 0; direction < 32; ++direction)
        {
            const float angle = static_cast<float>(direction) * PI / 16.0f;
            const Vector2F center = nozzle + Vector2F::unit_vec_from_angle(angle) * look.aim_marker_distance;
            const RectangleF bounds = feedback.aim_marker_bounds(nozzle, angle);
            require(bounds.width > 0.0f && bounds.height > 0.0f &&
                bounds.contains(RectangleF(center, Vector2F::ZERO)) &&
                std::isfinite(bounds.x) && std::isfinite(bounds.y),
                "Marker bounds omitted the continuous aim-ray center or became invalid");
            require(!bounds.contains(RectangleF(nozzle, Vector2F::ZERO)),
                "Marker bounds included its muzzle instead of remaining beyond it");
        }
        require(feedback.state() == initial, "Marker bounds mutated event feedback");
        auto& disabled = const_cast<WeaponFeedbackPresentation&>(world.presentation.weapon_feedback());
        disabled.enabled = false;
        DrawList unused;
        for (const auto state : { WeaponReadiness::ready, WeaponReadiness::recovering, WeaponReadiness::empty })
        {
            feedback.draw_aim_marker(unused, nozzle, PI, state, Colour::white);
        }
        require(feedback.state() == initial, "Disabled marker drawing mutated feedback state");
        disabled.enabled = true;
        std::cout << "PASS aim marker: const draw/readiness, continuous 32-direction bounds and disabled draw purity\n";
    }

    void activate(Level& level, const std::vector<PlayerInputData>& inputs)
    {
        for (int tick = 0; tick < 600 && level.state() != LevelState::active; ++tick) { level.update(inputs, dt); }
        require(level.state() == LevelState::active, "Marker fixture did not reach active play");
    }

    void check_lifecycle(World& world)
    {
        const std::vector<PlayerInputData> inputs(input_slots::count, input());
        auto opening = world.build(WeaponType::sprayer, OpeningMode::full);
        require(!opening->aim_marker_visible(0), "Story opening showed an aim marker");
        opening.reset();
        auto level = world.build();
        require(!level->aim_marker_visible(-1) && !level->aim_marker_visible(4) &&
            !level->aim_marker_visible(0), "Invalid view or countdown showed a marker");
        activate(*level, inputs);
        for (int view = 0; view < 4; ++view)
        {
            require(level->aim_marker_visible(view), "Active aimed tool or sparse input roster lost its marker");
        }
        world.preferences.aim_marker = false;
        require(!level->aim_marker_visible(0), "Live preference disable retained the marker");
        world.preferences.aim_marker = true;
        // ROAD#7: views 0-3 hold pads 4, 2, 3 and 1. Each pane follows its own
        // pad's override, and a pad left on Shared follows the Options switch.
        world.preferences.pad_comfort[1].aim_marker = SeatSwitch::off;
        require(level->aim_marker_visible(0) && !level->aim_marker_visible(1) &&
            level->aim_marker_visible(2) && level->aim_marker_visible(3),
            "Pad 2's aim-marker opt-out did not hide only its own pane's marker");
        world.preferences.aim_marker = false;
        world.preferences.pad_comfort[2].aim_marker = SeatSwitch::on;
        require(!level->aim_marker_visible(0) && !level->aim_marker_visible(1) &&
            level->aim_marker_visible(2) && !level->aim_marker_visible(3),
            "Pad 3's aim-marker opt-in did not win over the shared Off");
        world.preferences.pad_comfort[2].reduce_effects = SeatSwitch::on;
        require(level->aim_marker_visible(2), "A pad's Reduce effects hid its separate aim-marker choice");
        world.preferences.pad_comfort = {};
        world.preferences.aim_marker = true;
        for (int view = 0; view < 4; ++view)
        {
            require(level->aim_marker_visible(view), "Returning every pad to Shared did not restore the markers");
        }
        auto& look = const_cast<WeaponFeedbackPresentation&>(world.presentation.weapon_feedback());
        look.enabled = false;
        require(!level->aim_marker_visible(0), "Disabled presentation retained the marker");
        look.enabled = true;
        Player& player = const_cast<Player&>(level->player(0));
        for (const FigurePose pose : { FigurePose{ Vector2F::ZERO, 0.0f, false, false },
            FigurePose{ Vector2F::ZERO, 0.0f, true, true } })
        {
            player.set_pose(pose);
            require(!level->aim_marker_visible(0), "Tool-free or whole story pose showed a marker");
        }
        player.set_pose({});
        player.set_coloured(false, Colour::gray);
        require(!level->aim_marker_visible(0), "Grey story figure showed a marker");
        player.set_coloured(true, Colour::gray);
        const auto state = player.primary_weapon().readiness();
        const auto ammo = player.weapon_ammo();
        level->suspend();
        for (int repeat = 0; repeat < 10; ++repeat)
        {
            level->refresh_layout();
            level->update(inputs, 0.0f);
            require(level->aim_marker_visible(0) && player.primary_weapon().readiness() == state &&
                player.weapon_ammo() == ammo, "Suspended/frozen match changed marker readiness or ammunition");
        }
        level->resume();
        require(level->kill_player(0) && !level->aim_marker_visible(0),
            "Health-zero player showed marker before dead-state transition");
        level->update(inputs, dt);
        require(player.state() == PlayerState::dead && !level->aim_marker_visible(0), "Dead player showed marker");
        require(level->respawn_player(0) && level->aim_marker_visible(0), "Respawn did not restore marker");
        auto roller = world.build(WeaponType::roller);
        activate(*roller, inputs);
        require(!roller->aim_marker_visible(0) && roller->aim_marker_visible(1), "Roller exclusion hid another aimed tool");
        auto short_round = world.build(WeaponType::sprayer, OpeningMode::off, 0.5f);
        activate(*short_round, inputs);
        for (int tick = 0; tick < 120 && short_round->state() == LevelState::active; ++tick)
        {
            short_round->update(inputs, dt);
        }
        require(short_round->state() == LevelState::zoom_out && !short_round->aim_marker_visible(0),
            "Time-up retained the marker in the shared overview");
        std::cout << "PASS aim marker: real Level opening/countdown, sparse views, live preference/presentation, story poses, pause, fatal edge, death/respawn, roller and time-up\n";
    }

    void compare_gameplay(const Level& first, const Level& second)
    {
        require(first.state() == second.state() && first.projectile_count() == second.projectile_count() &&
            first.collision_object_count() == second.collision_object_count(), "Marker changed world state or object counts");
        for (int view = 0; view < first.player_count(); ++view)
        {
            const auto& a = first.player(view);
            const auto& b = second.player(view);
            require(a.rectangle() == b.rectangle() && a.velocity() == b.velocity() && a.health() == b.health() &&
                a.state() == b.state() && a.move_state() == b.move_state() && a.weapon_ammo() == b.weapon_ammo() &&
                a.primary_weapon().readiness() == b.primary_weapon().readiness() &&
                a.primary_weapon().rotation() == b.primary_weapon().rotation() &&
                a.primary_weapon().nozzle_pos() == b.primary_weapon().nozzle_pos(),
                "Marker changed movement, paint, health, aim, readiness or respawn");
        }
        const auto a = first.level_end_info(), b = second.level_end_info();
        require(a.player_areas == b.player_areas && a.team_a_area == b.team_a_area &&
            a.team_b_area == b.team_b_area && a.total_area == b.total_area, "Marker changed painted coverage or score");
    }

    void check_gameplay(World& enabled_world, const std::filesystem::path& content)
    {
        World disabled_world(content);
        disabled_world.preferences.aim_marker = false;
        std::srand(734);
        auto enabled = enabled_world.build();
        std::srand(734);
        auto disabled = disabled_world.build();
        std::vector<PlayerInputData> inputs(input_slots::count, input());
        activate(*enabled, inputs);
        activate(*disabled, inputs);
        bool shots = false;
        for (int tick = 0; tick < 360; ++tick)
        {
            for (auto& actions : inputs)
            {
                actions = input(tick < 150, static_cast<float>(tick % 16) * PI / 8.0f);
                actions.x_movement = tick < 40 ? 1.0f : 0.0f;
                actions.jump_pressed = tick == 15;
                actions.jump_held = tick >= 15 && tick < 30;
            }
            if (tick == 170)
            {
                require(enabled->kill_player(0) && disabled->kill_player(0), "Could not eliminate equivalence player");
            }
            const unsigned seed = static_cast<unsigned>(tick + 123);
            std::srand(seed); enabled->update(inputs, dt);
            std::srand(seed); disabled->update(inputs, dt);
            compare_gameplay(*enabled, *disabled);
            shots = shots || enabled->projectile_count() > 0;
            require(!disabled->aim_marker_visible(0), "Disabled preference reappeared during gameplay");
        }
        require(shots && enabled->player(0).state() == PlayerState::alive,
            "Marker equivalence missed paid shots or natural respawn");
        std::cout << "PASS aim marker on/off: actual Level movement, aiming, firing cadence, ammo, paint coverage, health/death and natural respawn unchanged\n";
    }

    void check_validation(const std::filesystem::path& content, const std::filesystem::path& scratch)
    {
        std::ifstream input_file(content / "presentation.json");
        require(input_file.good(), "Cannot read marker presentation");
        const std::string source(std::istreambuf_iterator<char>(input_file), {});
        const auto path = scratch / "aim-marker-invalid.json";
        struct Cleanup
        {
            std::filesystem::path path;
            ~Cleanup() { std::error_code ignored; std::filesystem::remove(this->path, ignored); }
        } cleanup{ path };
        const auto reject = [&](auto edit)
        {
            rapidjson::Document json;
            json.Parse(source.c_str());
            require(!json.HasParseError(), "Cannot parse marker presentation fixture");
            edit(json["weapon_feedback"]);
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            json.Accept(writer);
            { std::ofstream variant(path); variant << buffer.GetString(); require(variant.good(), "Cannot write marker variant"); }
            bool rejected = false;
            try { const Presentation invalid(path.string().c_str()); }
            catch (const std::exception& error)
            {
                rejected = std::string(error.what()).find("weapon_feedback") != std::string::npos;
            }
            require(rejected, "Malformed marker presentation was accepted or lost its section name");
        };
        for (const char* field : { "aim_marker_distance", "aim_marker_size", "aim_marker_stroke" })
        {
            reject([field](rapidjson::Value& look) { look.RemoveMember(field); });
            reject([field](rapidjson::Value& look) { look[field].SetString("small"); });
            reject([field](rapidjson::Value& look) { look[field].SetFloat(0.0f); });
            reject([field](rapidjson::Value& look) { look[field].SetFloat(-1.0f); });
            reject([field](rapidjson::Value& look) { look[field].SetFloat(10000.0f); });
        }
        reject([](rapidjson::Value& look) { look["aim_marker_distance"].SetFloat(look["aim_marker_size"].GetFloat()); });
        reject([](rapidjson::Value& look) { look["aim_marker_stroke"].SetFloat(look["aim_marker_size"].GetFloat() * 0.3f); });
        reject([](rapidjson::Value& look) { look["aim_marker_stroke"].SetFloat(look["keyline_width"].GetFloat()); });
        std::cout << "PASS aim marker content: required numeric positive bounded geometry, muzzle clearance and stroke/keyline relationships\n";
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: aim_marker.exe <content directory>");
        const std::filesystem::path content(argv[1]);
        World world(content);
        check_readiness(world);
        check_bounds(world);
        check_lifecycle(world);
        check_gameplay(world, content);
        check_validation(content, std::filesystem::absolute(argv[0]).parent_path());
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL aim marker: " << error.what() << '\n';
        return 1;
    }
}
