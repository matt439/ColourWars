#include "game/pch.h"
#include "game/objects/player.h"
#include "game/objects/structure.h"
#include "game/objects/weapon_builder.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/scene/scene.h"
#include "scripts/acceptance/support/probe.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>

using namespace labrador;
using namespace mattmath;

namespace
{
    constexpr float dt = 1.0f / 60.0f;

    using acceptance::require;

    void check_rest_and_interruptions(const FigurePresentation& look)
    {
        std::array<QuietCharacter, 4> crowd{ QuietCharacter(&look, 0),
            QuietCharacter(&look, 1), QuietCharacter(&look, 2), QuietCharacter(&look, 3) };
        const RectangleF body(100.0f, 200.0f, 52.0f, 120.0f);
        std::array<int, 4> first_blinks{ -1, -1, -1, -1 };
        bool breathed = false;
        bool shifted = false;
        for (int tick = 0; tick < 900; ++tick)
        {
            for (size_t member = 0; member < crowd.size(); ++member)
            {
                auto& character = crowd[member];
                character.update(true, true, true, false, 0.0f, 0.0f, dt);
                const auto& state = character.state();
                const auto posed = character.rectangle(body);
                require(std::abs(posed.bottom() - body.bottom()) < 0.0001f && posed.width > 0.0f &&
                    posed.height > 0.0f, "Quiet pose moved the feet above/below their support");
                if (static_cast<float>(tick + 1) * dt < look.quiet.settle_time)
                {
                    require(state.breath == 0.0f && !state.blinking,
                        "Quiet details began before the stationary settling interval");
                }
                breathed = breathed || state.breath > 0.0f;
                shifted = shifted || state.lean != 0.0f;
                if (state.blinking && first_blinks[member] < 0) { first_blinks[member] = tick; }
                const auto snapshot = state;
                character.update(false, false, false, true, -100.0f, 100.0f, 0.0f);
                character.update(false, false, false, true, -100.0f, 100.0f, -1.0f);
                character.update(false, false, false, true, -100.0f, 100.0f,
                    std::numeric_limits<float>::infinity());
                require(character.state() == snapshot, "Frozen delta aged character detail");
            }
        }
        require(breathed && shifted, "Resting crowd did not breathe and shift weight");
        for (size_t member = 0; member < crowd.size(); ++member)
        {
            require(first_blinks[member] >= 0, "A resting character never blinked");
            for (size_t previous = 0; previous < member; ++previous)
            {
                require(first_blinks[member] != first_blinks[previous],
                    "Roster members started their idle blink in synchrony");
            }
        }

        QuietCharacter accelerated(&look, 0);
        accelerated.update(true, true, false, false, 0.0f, 200.0f, dt);
        require(accelerated.state().lean > 0.0f, "Acceleration produced no directional lean");
        for (int tick = 0; tick < 40; ++tick)
        {
            accelerated.update(true, true, false, false, 200.0f, 200.0f, dt);
        }
        const float recovered = accelerated.state().lean;
        require(recovered < look.quiet.lean_distance * 0.05f,
            "Lean failed to settle during steady travel");
        accelerated.update(true, true, false, false, 200.0f, -200.0f, dt);
        require(accelerated.state().lean < 0.0f, "A turn did not reverse the body lean");
        accelerated.update(true, false, false, false, -200.0f, -200.0f, dt);
        require(accelerated.state() == QuietCharacter::State{}, "Airborne character retained quiet posing");

        for (const auto interruption : { 0, 1, 2 })
        {
            QuietCharacter character(&look, 0);
            for (int tick = 0; tick < 300; ++tick)
            {
                character.update(true, true, true, false, 0.0f, 0.0f, dt);
            }
            character.update(interruption != 0, true, false, interruption == 1, 0.0f, 0.0f, dt);
            require(character.state().breath == 0.0f && !character.state().blinking,
                "Elimination, action or firing left a quiet eye/breath pose active");
            character.reset();
            require(character.state() == QuietCharacter::State{} && character.rectangle(body) == body,
                "Lifecycle reset did not restore the unposed figure");
        }
        std::cout << "PASS quiet character: grounded rest, independent phases, blink, freeze, acceleration/turn settling and interruptions\n";
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
                render.add_sprite_sheet(sheet, read_sprite_sheet(
                    (content / "textures" / (std::string(sheet) + ".json")).string().c_str(),
                    TextureHandle{}));
            }
            audio.add_sound_bank("paint_exe", SoundBank::silent());
        }
    };

    struct Course
    {
        PlaySettings preferences;
        Scene scene{ nullptr, nullptr };
        Player* player;

        Course(World& world, const FigurePresentation* figure, WeaponType type) :
            preferences(world.preferences)
        {
            const RectangleF body(0.0f, -world.tuning.player().size.y,
                world.tuning.player().size.x, world.tuning.player().size.y);
            auto weapon = WeaponBuilder::build_weapon(&world.tuning, PlayerTeam::a, 0, 0,
                Colour::white, type, body.center(), &world.render, &world.audio, &this->preferences);
            player = scene.add(std::make_unique<Player>(body, &world.tuning,
                &world.presentation.movement_feedback(), &world.presentation.combat_feedback(),
                &world.presentation.respawn_redraw(), &world.render, &world.audio, &this->preferences,
                0, PlayerTeam::a, type, std::move(weapon), WeaponType::none, Colour::white,
                Viewport{}, body.position(), Vector2F::ZERO, 0.0f, Vector2F::ZERO,
                SpriteFlip::none, 0.0f, 0, figure));
            const RectangleF floor(-10000.0f, 0.0f, 20000.0f, 100.0f);
            scene.add(std::make_unique<Structure>("sprite_sheet_1", "square_white_4", floor,
                std::make_unique<RectangleF>(floor), &world.render, CollisionObjectType::structure));
            scene.end_tick();
        }

        std::vector<std::unique_ptr<Projectile>> tick(const PlayerInputData& input)
        {
            player->set_player_input(input);
            scene.update(dt);
            auto projectiles = player->update_weapon_and_get_projectiles(dt, scene.collision_objects());
            scene.resolve();
            player->end_contacts(scene.contacts());
            player->update_weapon_position();
            player->update_prev_rectangle();
            scene.end_tick();
            return projectiles;
        }
    };

    void check_real_player(World& world)
    {
        bool seen_rest = false;
        bool seen_lean = false;
        size_t shots = 0;
        for (const auto type : { WeaponType::sprayer, WeaponType::sniper, WeaponType::roller,
            WeaponType::mister, WeaponType::bucket })
        {
            Course animated(world, &world.presentation.figure(), type);
            Course plain(world, &world.presentation.figure(), type);
            plain.preferences.reduce_effects = true;
            for (int tick = 0; tick < 1200; ++tick)
            {
                PlayerInputData input;
                input.connected = true;
                if (tick >= 300 && tick < 650) { input.x_movement = tick < 480 ? 1.0f : -1.0f; }
                input.jump_pressed = tick == 650;
                input.jump_held = tick >= 650 && tick < 660;
                input.primary_shoot = tick >= 850 && tick < 1000;
                input.shoot_direction_requested = true;
                input.shoot_angle = static_cast<float>(tick % 9 - 4) * PI / 4.0f;
                input.shoot_direction = Vector2F::unit_vec_from_angle(input.shoot_angle);
                if (tick == 1050) { animated.player->kill(); plain.player->kill(); }
                if (tick == 1100) { animated.player->force_respawn(); plain.player->force_respawn(); }
                auto first = animated.tick(input);
                auto second = plain.tick(input);
                shots += first.size();
                require(first.size() == second.size(), "Character motion changed shot cadence/volley size");
                for (size_t shot = 0; shot < first.size(); ++shot)
                {
                    require(first[shot]->shape()->bounding_box() == second[shot]->shape()->bounding_box() &&
                        first[shot]->tag() == second[shot]->tag(), "Character motion changed projectile launch/ownership");
                    for (int travel = 0; travel < 3; ++travel)
                    {
                        first[shot]->update(dt);
                        second[shot]->update(dt);
                        require(first[shot]->shape()->bounding_box() == second[shot]->shape()->bounding_box(),
                            "Character motion changed projectile travel");
                    }
                }
                const Player& a = *animated.player;
                const Player& b = *plain.player;
                require(a.rectangle() == b.rectangle() && a.velocity() == b.velocity() &&
                    a.health() == b.health() && a.state() == b.state() &&
                    a.weapon_ammo() == b.weapon_ammo() && a.move_state() == b.move_state() &&
                    a.primary_weapon().rotation() == b.primary_weapon().rotation() &&
                    a.primary_weapon().nozzle_pos() == b.primary_weapon().nozzle_pos() &&
                    a.primary_weapon().ready_to_shoot() == b.primary_weapon().ready_to_shoot(),
                    "Quiet character changed simulation, ammunition, readiness, aim or muzzle");
                require(std::abs(a.posed_rectangle().bottom() - a.rectangle().bottom()) < 0.0001f,
                    "Real player quiet pose lifted its foot baseline");
                seen_rest = seen_rest || a.quiet_character().state().breath > 0.0f;
                seen_lean = seen_lean || a.quiet_character().state().lean != 0.0f;
                require(b.quiet_character().state() == QuietCharacter::State{},
                    "Reduced-effects player retained quiet posing");
            }
            animated.player->set_pose(FigurePose{ Vector2F(1.0f, 2.0f), 0.1f, false, false });
            const RectangleF story = animated.player->posed_rectangle();
            require(story.x == animated.player->rectangle().x + 1.0f &&
                std::abs(story.height - animated.player->rectangle().height * 0.9f) < 0.0001f,
                "Story figure inherited ordinary idle motion");
        }
        require(seen_rest && seen_lean && shots > 0, "Real-player comparison missed quiet posing or paid fire");
        std::cout << "PASS quiet character: reduced-effects Player preserves five tools' shots, travel, collision, ammo, health, aim and muzzle; story/reset remain clean\n";
    }

    void check_live_preference(World& world)
    {
        Course course(world, &world.presentation.figure(), WeaponType::sprayer);
        PlayerInputData idle;
        idle.connected = true;
        for (int tick = 0; tick < 600 &&
            course.player->quiet_character().state().breath == 0.0f; ++tick)
        {
            std::ignore = course.tick(idle);
        }
        const auto before = course.player->quiet_character().state();
        require(before.breath > 0.0f &&
            course.player->posed_rectangle() != course.player->rectangle(),
            "Live reduced-effects fixture never reached a quiet body pose");

        course.preferences.reduce_effects = true;
        require(course.player->posed_rectangle() == course.player->rectangle() &&
            course.player->quiet_character().state() == before,
            "Reducing effects did not immediately suppress the frozen pose without mutating it");
        std::ignore = course.tick(idle);
        require(course.player->quiet_character().state() == QuietCharacter::State{},
            "Active reduced-effects update retained quiet character state");
        course.preferences.reduce_effects = false;
        require(course.player->posed_rectangle() == course.player->rectangle(),
            "Restoring effects resurrected an old quiet body pose");
        for (int tick = 0; static_cast<float>(tick + 1) * dt <
            world.presentation.figure().quiet.settle_time; ++tick)
        {
            std::ignore = course.tick(idle);
            require(course.player->quiet_character().state().breath == 0.0f &&
                !course.player->quiet_character().state().blinking,
                "Restoring effects bypassed quiet-character settling");
        }
        bool resumed = false;
        for (int tick = 0; tick < 600; ++tick)
        {
            std::ignore = course.tick(idle);
            resumed = resumed || course.player->quiet_character().state().breath > 0.0f;
        }
        require(resumed, "Restored effects never resumed fresh quiet character motion");
        std::cout << "PASS quiet character preference: frozen suppression, active clear, fresh settling and restored motion\n";
    }

    void check_validation(const std::filesystem::path& content)
    {
        std::ifstream file(content / "presentation.json");
        const std::string source((std::istreambuf_iterator<char>(file)), {});
        const auto path = std::filesystem::temp_directory_path() /
            ("colourwars-quiet-character-" + std::to_string(GetCurrentProcessId()) + ".json");
        struct Cleanup
        {
            std::filesystem::path path;
            ~Cleanup() { std::error_code ignored; std::filesystem::remove(path, ignored); }
        } cleanup{ path };
        const auto reject = [&](auto edit)
        {
            rapidjson::Document json;
            json.Parse(source.c_str());
            require(!json.HasParseError(), "Invalid source presentation for character fixture");
            edit(json["figure"]["quiet"]);
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            json.Accept(writer);
            std::ofstream output(path);
            output.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
            output.close();
            require(output.good(), "Cannot write character presentation fixture");
            bool rejected = false;
            try { const Presentation invalid(path.string().c_str()); }
            catch (const std::exception& error)
            {
                rejected = std::string(error.what()).find("figure.quiet") != std::string::npos;
            }
            require(rejected, "Malformed quiet character presentation lacked a contextual rejection");
        };
        reject([](rapidjson::Value& quiet) { quiet["breath_period"].SetInt(0); });
        reject([](rapidjson::Value& quiet) { quiet["lean_settle_time"].SetInt(-1); });
        reject([](rapidjson::Value& quiet) { quiet["blink_max_interval"].SetFloat(0.1f); });
        reject([](rapidjson::Value& quiet) { quiet["blink_duration"].SetFloat(20.0f); });
        reject([](rapidjson::Value& quiet) { quiet["breath_height"].SetFloat(120.0f); });
        reject([](rapidjson::Value& quiet) { quiet["lean_distance"].SetFloat(52.0f); });
        reject([](rapidjson::Value& quiet) { quiet["eyes"][0]["x"].SetFloat(-1.0f); });
        reject([](rapidjson::Value& quiet) { quiet["eyes"][1]["height"].SetFloat(120.0f); });
        reject([](rapidjson::Value& quiet) { quiet["enabled"].SetInt(1); });
        std::cout << "PASS quiet character content: positive timing, bounded body/eye geometry and blink intervals\n";
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("quiet character", [&]
    {
        require(argc == 2, "Usage: quiet_character.exe <content directory>");
        const std::filesystem::path content(argv[1]);
        World world(content);
        require(world.presentation.figure().quiet.enabled, "Reviewed quiet character detail must be enabled");
        check_rest_and_interruptions(world.presentation.figure());
        check_real_player(world);
        check_live_preference(world);
        check_validation(content);
        return 0;
    });
}
