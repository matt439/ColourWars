#include "game/pch.h"
#include "game/objects/player.h"
#include "game/objects/projectile_spray.h"
#include "game/objects/structure.h"
#include "game/objects/weapon_builder.h"
#include "game/presentation.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/step_timer.h"
#include "engine/scene/scene.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
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

    bool approximately_equal(float actual, float expected)
    {
        return std::abs(actual - expected) < 0.00001f;
    }

    const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));

    PlayerInputData input(bool jump = false, float movement = 0.0f, bool fire = false)
    {
        PlayerInputData result;
        result.connected = true;
        result.jump_pressed = jump;
        result.jump_held = jump;
        result.x_movement = movement;
        result.primary_shoot = fire;
        result.shoot_direction_requested = true;
        result.shoot_direction = Vector2F(1, 0);
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
            for (const char* sheet : { "sprite_sheet_1", "painter", "weapon_paint", "paint_fx" })
            {
                this->render.add_sprite_sheet(sheet, read_sprite_sheet(
                    (content / "textures" / (std::string(sheet) + ".json")).string().c_str(),
                    TextureHandle{}));
            }
            this->audio.add_sound_bank("paint_exe", SoundBank::silent());
        }

        ProjectileSpray shot()
        {
            return ProjectileSpray(RectangleF(0, 0, 10, 10), Vector2F::ZERO,
                PlayerTeam::b, 1, Colour::white, &this->render,
                &this->tuning.projectile(ProjectileType::spray));
        }
    };

    // The real update/contact ordering, with no graphics/audio device or save.
    struct Course
    {
        Scene scene{ nullptr, nullptr };
        Player* player;
        size_t emitted_projectiles = 0;

        Course(World& world, const CombatFeedbackPresentation& feedback)
        {
            const RectangleF rectangle(0, -world.tuning.player().size.y,
                world.tuning.player().size.x, world.tuning.player().size.y);
            auto weapon = WeaponBuilder::build_weapon(&world.tuning, PlayerTeam::a,
                0, 0, Colour::white, WeaponType::sprayer, rectangle.center(),
                &world.render, &world.audio, &world.preferences);
            this->player = this->scene.add(std::make_unique<Player>(rectangle,
                &world.tuning, &world.presentation.movement_feedback(), &feedback,
                &world.presentation.respawn_redraw(), &world.render, &world.audio,
                &world.preferences, 0, PlayerTeam::a, WeaponType::sprayer,
                std::move(weapon), WeaponType::none, Colour::white, Viewport{},
                Vector2F(180, -world.tuning.player().size.y)));
            const RectangleF floor(-10000, 0, 20000, 100);
            this->scene.add(std::make_unique<Structure>("sprite_sheet_1", "square_white_4",
                floor, std::make_unique<RectangleF>(floor), &world.render,
                CollisionObjectType::structure));
            this->scene.end_tick();
        }

        void tick(const PlayerInputData& actions = input())
        {
            this->player->set_player_input(actions);
            this->scene.update(dt);
            this->emitted_projectiles += this->player->update_weapon_and_get_projectiles(
                dt, this->scene.collision_objects()).size();
            this->scene.resolve();
            this->player->end_contacts(this->scene.contacts());
            this->player->update_prev_rectangle();
            this->scene.end_tick();
        }

        void hit(const ProjectileSpray& shot)
        {
            this->player->on_contact(shot, Vector2F::ZERO, 0.0f);
        }

        void eliminate(const ProjectileSpray& shot)
        {
            // Contact damage is deliberately not replaced with the developer
            // kill hook: the fatal event must originate in the production path.
            for (int hit = 0; hit < 10000 && this->player->health() > 0.0f; ++hit)
            {
                this->hit(shot);
            }
            require(this->player->health() <= 0.0f, "Projectile fixture did not eliminate player");
        }
    };

    void check_hurt(World& world)
    {
        const auto& look = world.presentation.combat_feedback();
        Course course(world, look);
        const ProjectileSpray shot = world.shot();
        const float initial_health = course.player->health();
        course.player->set_damage_immune(true);
        course.hit(shot);
        require(course.player->health() == initial_health &&
            course.player->combat_feedback().hurt_remaining() == 0.0f,
            "Ignored immune contact emitted hurt feedback or changed health");
        course.player->set_damage_immune(false);
        course.hit(shot);
        require(course.player->health() < initial_health && course.player->health() > 0.0f &&
            approximately_equal(course.player->combat_feedback().hurt_remaining(), look.hurt_duration),
            "Accepted nonfatal contact did not immediately start its hurt cue");
        require(course.player->animation_state() == PlayerAnimationState::hurt,
            "Accepted nonfatal contact lost its immediate hurt action");

        course.tick();
        require(approximately_equal(course.player->combat_feedback().hurt_remaining(),
            std::max(0.0f, look.hurt_duration - dt)),
            "Player aged its hurt cue more or less than once per simulation tick");
        const float remaining = course.player->combat_feedback().hurt_remaining();
        const float health = course.player->health();
        course.hit(shot);
        require(course.player->health() < health &&
            course.player->combat_feedback().hurt_remaining() == remaining,
            "Repeated damage restarted hurt feedback during its repeat delay");

        // The accent expires before the contact limiter; an intervening hit
        // still does damage, but cannot turn a continuous spray into a strobe.
        int ticks = 1;
        while (static_cast<float>(ticks) * dt <= look.hurt_duration)
        {
            course.tick();
            ++ticks;
        }
        require(course.player->combat_feedback().hurt_remaining() == 0.0f,
            "Hurt accent outlived its configured duration");
        if (static_cast<float>(ticks) * dt < look.hurt_repeat_delay)
        {
            course.hit(shot);
            require(course.player->combat_feedback().hurt_remaining() == 0.0f,
                "Expired accent bypassed the longer repeated-contact delay");
        }
        while (static_cast<float>(ticks) * dt <= look.hurt_repeat_delay + dt)
        {
            course.tick();
            ++ticks;
        }
        course.hit(shot);
        require(approximately_equal(course.player->combat_feedback().hurt_remaining(), look.hurt_duration),
            "Fresh accepted damage did not rearm hurt feedback after its delay");
        std::cout << "PASS combat hurt: immediate accepted damage, immune rejection, one update and repeat limiter\n";
    }

    void check_elimination(World& world)
    {
        const auto& look = world.presentation.combat_feedback();
        Course course(world, look);
        const ProjectileSpray shot = world.shot();
        course.tick();
        course.eliminate(shot);
        const RectangleF death = course.player->rectangle();
        const Vector2F velocity = course.player->velocity();
        course.tick(input(true, 1.0f, true));
        const auto& cue = course.player->combat_feedback();
        require(course.player->state() == PlayerState::dead &&
            course.player->rectangle() == death && course.player->velocity() == velocity &&
            course.emitted_projectiles == 0,
            "Fatal tick allowed a final movement, jump or paid shot");
        require(cue.death_rectangle() == death &&
            approximately_equal(cue.elimination_remaining(), look.elimination_duration) &&
            cue.hurt_remaining() == 0.0f,
            "Alive-to-dead transition did not emit one fresh burst at the death rectangle");
        require(course.player->cull_bounds(1.0f).contains(cue.bounds(death)) &&
            course.player->cull_bounds(1.0f).contains(course.player->spawn_rectangle()),
            "Dead player culling omitted the detached burst or spawn redraw");

        course.tick();
        const float remaining = cue.elimination_remaining();
        course.hit(shot);
        course.hit(shot);
        require(cue.elimination_remaining() == remaining && cue.death_rectangle() == death,
            "Repeated dead contacts restarted or relocated the elimination burst");
        course.tick();
        require(approximately_equal(cue.elimination_remaining(),
            std::max(0.0f, look.elimination_duration - 2.0f * dt)),
            "Dead update duplicated the burst or advanced it more than once");
        for (int tick = 0; tick < 600 && course.player->state() == PlayerState::dead; ++tick)
        {
            course.tick(input(true, 1.0f, true));
        }
        require(course.player->state() == PlayerState::alive &&
            cue.hurt_remaining() == 0.0f && cue.elimination_remaining() == 0.0f &&
            cue.death_rectangle() == RectangleF::ZERO,
            "Natural respawn retained a previous life's combat cue");
        std::cout << "PASS combat elimination: fatal input suppression, one detached burst, dead contacts and natural respawn\n";
    }

    void check_timers_and_resets(World& world)
    {
        const auto& look = world.presentation.combat_feedback();
        const RectangleF body(20, -120, 60, 120);
        CombatFeedback feedback(&look, &world.render);
        require(feedback.hurt(), "Fresh combat fixture did not accept hurt");
        const float hurt = feedback.hurt_remaining();
        for (const float frozen : { 0.0f, -1.0f,
            std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() })
        {
            feedback.update(frozen);
            require(feedback.hurt_remaining() == hurt,
                "Frozen or invalid delta rewound or corrupted hurt feedback");
        }
        feedback.update(1000.0f);
        require(feedback.hurt_remaining() == 0.0f && feedback.hurt(),
            "Long update did not expire the hurt cue and its repeat delay");
        feedback.eliminate(body);
        require(feedback.hurt_remaining() == 0.0f && !feedback.hurt(),
            "Elimination retained or admitted a simultaneous hurt accent");
        const float elimination = feedback.elimination_remaining();
        for (const float frozen : { 0.0f, -1.0f })
        {
            feedback.update(frozen);
            require(feedback.elimination_remaining() == elimination &&
                feedback.death_rectangle() == body,
                "Nonpositive delta advanced or relocated elimination feedback");
        }
        feedback.update(1000.0f);
        require(feedback.elimination_remaining() == 0.0f && feedback.bounds(body) == body,
            "Long update retained expired elimination bounds");
        feedback.clear();
        feedback.clear();
        require(feedback.death_rectangle() == RectangleF::ZERO && feedback.hurt(),
            "Idempotent clear retained the death snapshot or contact limiter");

        Course course(world, look);
        const ProjectileSpray shot = world.shot();
        course.hit(shot);
        course.player->place_at(Vector2F(50, -200));
        require(course.player->combat_feedback().hurt_remaining() == 0.0f &&
            course.player->combat_feedback().elimination_remaining() == 0.0f,
            "Developer placement retained combat feedback");
        course.hit(shot);
        require(course.player->combat_feedback().hurt_remaining() > 0.0f,
            "Placement retained a previous contact's repeat limiter");
        course.eliminate(shot);
        course.tick();
        require(course.player->combat_feedback().elimination_remaining() > 0.0f,
            "Forced-respawn fixture did not create a live elimination burst");
        course.player->force_respawn();
        require(course.player->state() == PlayerState::alive &&
            course.player->combat_feedback().hurt_remaining() == 0.0f &&
            course.player->combat_feedback().elimination_remaining() == 0.0f &&
            course.player->combat_feedback().death_rectangle() == RectangleF::ZERO,
            "Forced respawn retained combat feedback");
        std::cout << "PASS combat lifecycle: frozen/invalid/long deltas, idempotent clear, placement and forced respawn\n";
    }

    void check_simulation_equivalence(World& world)
    {
        CombatFeedbackPresentation disabled = world.presentation.combat_feedback();
        disabled.enabled = false;
        Course visible(world, world.presentation.combat_feedback());
        Course quiet(world, disabled);
        const ProjectileSpray shot = world.shot();
        bool hurt_seen = false;
        bool burst_seen = false;
        bool respawned = false;
        for (int tick = 0; tick < 360; ++tick)
        {
            if (tick == 2 || tick == 4 || tick == 300)
            {
                visible.hit(shot);
                quiet.hit(shot);
            }
            if (tick == 70)
            {
                visible.eliminate(shot);
                quiet.eliminate(shot);
            }
            hurt_seen = hurt_seen || visible.player->combat_feedback().hurt_remaining() > 0.0f;
            const PlayerInputData actions = input(tick == 10 || tick == 290,
                tick < 45 ? 1.0f : -1.0f, tick >= 2 && tick < 80);
            visible.tick(actions);
            quiet.tick(actions);
            const Player& a = *visible.player;
            const Player& b = *quiet.player;
            require(a.rectangle() == b.rectangle() && a.velocity() == b.velocity() &&
                a.move_state() == b.move_state() && a.state() == b.state() &&
                a.health() == b.health() && a.weapon_ammo() == b.weapon_ammo() &&
                a.respawn_timer() == b.respawn_timer() && a.facing_right() == b.facing_right() &&
                a.primary_weapon().rotation() == b.primary_weapon().rotation() &&
                a.primary_weapon().nozzle_pos() == b.primary_weapon().nozzle_pos() &&
                visible.emitted_projectiles == quiet.emitted_projectiles,
                "Combat feedback changed movement, weapon, damage or respawn simulation");
            require(b.combat_feedback().hurt_remaining() == 0.0f &&
                b.combat_feedback().elimination_remaining() == 0.0f,
                "Disabled combat presentation emitted a cue");
            burst_seen = burst_seen || a.combat_feedback().elimination_remaining() > 0.0f;
            respawned = respawned || (tick > 70 && a.state() == PlayerState::alive);
        }
        require(hurt_seen && burst_seen && respawned && visible.emitted_projectiles > 0,
            "Combat equivalence route missed hurt, elimination, respawn or paid shots");
        std::cout << "PASS combat on/off equivalence through movement, contacts, paid shots, death and respawn\n";
    }

    void check_validation(const std::filesystem::path& content)
    {
        std::ifstream stream(content / "presentation.json");
        require(stream.good(), "Cannot read combat presentation content");
        const std::string source(std::istreambuf_iterator<char>(stream), {});
        wchar_t temporary_name[MAX_PATH]{};
        const auto temporary_directory = std::filesystem::temp_directory_path();
        require(GetTempFileNameW(temporary_directory.c_str(), L"cwc", 0, temporary_name) != 0,
            "Cannot reserve combat presentation variant");
        const std::filesystem::path path(temporary_name);
        struct RemoveOnExit
        {
            std::filesystem::path path;
            ~RemoveOnExit()
            {
                std::error_code ignored;
                std::filesystem::remove(this->path, ignored);
            }
        } cleanup{ path };

        const auto reject = [&](auto edit)
        {
            rapidjson::Document json;
            json.Parse(source.c_str());
            require(!json.HasParseError(), "Cannot parse combat presentation fixture");
            edit(json["combat_feedback"]);
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            json.Accept(writer);
            std::ofstream variant(path);
            variant.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
            variant.close();
            require(variant.good(), "Cannot write combat presentation fixture");
            bool rejected = false;
            try { const Presentation invalid(path.string().c_str()); }
            catch (const std::exception& error)
            {
                rejected = std::string(error.what()).find("combat_feedback") != std::string::npos;
            }
            require(rejected, "Malformed combat presentation was accepted or lost its authored section");
        };
        for (const char* field : { "hurt_duration", "hurt_repeat_delay", "hurt_width", "hurt_reach",
            "elimination_duration", "elimination_spread", "elimination_lift", "particle_size", "keyline_width" })
        {
            reject([field](rapidjson::Value& look) { look[field].SetFloat(0.0f); });
            reject([field](rapidjson::Value& look) { look[field].SetFloat(-1.0f); });
        }
        reject([](rapidjson::Value& look) { look["hurt_duration"].SetString("quick"); });
        reject([](rapidjson::Value& look) { look["hurt_repeat_delay"].SetFloat(0.001f); });
        reject([](rapidjson::Value& look) { look["keyline_width"].SetFloat(10000.0f); });
        std::cout << "PASS combat content validation: numeric types, positive geometry/times and readable dimensions\n";
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: combat_feedback.exe <content directory>");
        const std::filesystem::path content(argv[1]);
        World world(content);
        require(world.presentation.combat_feedback().enabled,
            "Reviewed combat feedback must be enabled in staged content");
        check_hurt(world);
        check_elimination(world);
        check_timers_and_resets(world);
        check_simulation_equivalence(world);
        check_validation(content);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL combat feedback: " << error.what() << '\n';
        return 1;
    }
}
