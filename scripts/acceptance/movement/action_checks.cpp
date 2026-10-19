#include "game/pch.h"
#include "scripts/acceptance/movement/fixtures.h"

#include <cmath>
#include <iostream>

// Death and respawn, the five paid tools, cooldown, empty tanks, hurt, and
// cosmetic action equivalence.
namespace movement_acceptance
{
    void check_action_events(const Suite& suite)
    {
        const GameTuning& tuning = suite.tuning;
        const MovementFeedbackPresentation& feedback = suite.feedback;
        const Presentation& presentation = suite.presentation;
        RenderResources& render = suite.render;
        AudioResources& audio = suite.audio;
        const PlaySettings& preferences = suite.preferences;
        const std::filesystem::path& content = suite.content;
        const ProjectileSpray& shot = suite.shot;
        const int fatal_hits = suite.fatal_hits;
        const auto course = [&](float bottom = 0.0f,
            const Vector2F& velocity = Vector2F::ZERO)
        {
            return suite.course(bottom, velocity);
        };

        auto killed = course(-4, { 0, 100 });
        killed->floor();
        killed->tick(input(true, true));
        for (int hit = 0; hit < fatal_hits && killed->player->health() > 0; ++hit)
        {
            killed->player->on_contact(shot, Vector2F::ZERO, 0);
        }
        require(killed->player->health() <= 0, "damage fixture did not eliminate player");
        const Vector2F death_position = killed->player->rectangle().position();
        killed->tick(input(true, true));
        require(killed->player->state() == PlayerState::dead &&
            killed->player->rectangle().position() == death_position,
            "fatal damage allowed one final movement/jump tick");
        require(killed->player->animation_state() == PlayerAnimationState::elimination &&
            killed->player->action_age() == 0.0f && killed->player->action_frame() == 0,
            "death update did not start the elimination clip at its first frame");
        // The redraw at the spawn: drawn there, culled with it, and watched.
        const RespawnRedrawPresentation& redraw = presentation.respawn_redraw();
        const RectangleF spawn = killed->player->spawn_rectangle();
        require(spawn.position() == Vector2F(0.0f, -120.0f) &&
            spawn.size() == killed->player->rectangle().size(),
            "the redraw is not at the player's spawn");
        require(killed->player->cull_bounds(1.0f).contains(spawn) &&
            killed->player->cull_bounds(1.0f).contains(killed->player->rectangle()),
            "a dead player's cull bounds miss the spawn where it is redrawn");
        require(killed->player->camera_focus() == killed->player->center(),
            "the camera left the fall before camera_hold");
        float previous_progress = killed->player->respawn_progress();
        bool watched_spawn = false;
        bool elimination_second_frame = false;
        bool dead_idle = false;
        for (int tick = 0; tick < 300 && killed->player->state() == PlayerState::dead; ++tick)
        {
            const float waited = tuning.player().respawn_delay - killed->player->respawn_timer();
            require((killed->player->camera_focus() == spawn.center()) ==
                (waited >= redraw.camera_hold),
                "the camera did not cut to the spawn at camera_hold");
            watched_spawn = watched_spawn || killed->player->camera_focus() == spawn.center();
            killed->tick(input(true, true));
            elimination_second_frame = elimination_second_frame ||
                (killed->player->animation_state() == PlayerAnimationState::elimination &&
                    killed->player->action_frame() == 1);
            dead_idle = dead_idle || (killed->player->state() == PlayerState::dead &&
                killed->player->animation_state() == PlayerAnimationState::idle);
            const float progress = killed->player->respawn_progress();
            require(killed->player->state() == PlayerState::alive ||
                (progress >= previous_progress && progress <= 1.0f),
                "the redraw went backwards or past the end");
            previous_progress = progress;
        }
        require(watched_spawn, "the camera never watched the redraw");
        require(elimination_second_frame && dead_idle,
            "elimination skipped its second frame or remained active throughout the redraw");
        require(killed->player->state() == PlayerState::alive,
            "eliminated player did not respawn");
        for (int tick = 0; tick < 30; ++tick) { killed->tick(input(false, true)); }
        require(killed->player->velocity().y == 0,
            "dead or respawning player carried a jump into its new life");
        killed->tick();
        killed->tick(input(true, true));
        require(killed->player->velocity().y < 0, "respawn failed to rearm after release");
        std::cout << "PASS lethal contact, death, respawn and held-jump isolation\n";

        // Every tool enters the action through its actual paid-shot path. In
        // particular a roller's contact presentation is not a shot event.
        for (const WeaponType tool : { WeaponType::sprayer, WeaponType::sniper,
            WeaponType::mister, WeaponType::bucket, WeaponType::roller })
        {
            Course armed(tuning, feedback, presentation.combat_feedback(), redraw, render, audio, preferences,
                0.0f, Vector2F::ZERO, { 0, -120 }, tool);
            armed.floor();
            for (int tick = 0; tick < 65; ++tick) { armed.tick(); }
            PlayerInputData fire = input();
            fire.primary_shoot = true;
            const float ammo_before = armed.player->weapon_ammo();
            armed.tick(fire);
            require(armed.emitted_projectiles > 0 && armed.player->weapon_ammo() < ammo_before &&
                armed.player->animation_state() == PlayerAnimationState::firing &&
                armed.player->action_age() == 0.0f && armed.player->action_frame() == 0,
                "paid weapon shot did not display its immediate first firing frame");
            bool second_frame = false;
            for (int tick = 0; tick < 12; ++tick)
            {
                armed.tick();
                second_frame = second_frame ||
                    (armed.player->animation_state() == PlayerAnimationState::firing &&
                        armed.player->action_frame() == 1);
            }
            require(second_frame && armed.player->animation_state() == PlayerAnimationState::idle,
                "firing skipped its recovery frame or failed to finish after release");
        }
        Course cooldown(tuning, feedback, presentation.combat_feedback(), redraw, render, audio, preferences,
            0.0f, Vector2F::ZERO, { 0, -120 }, WeaponType::sniper);
        cooldown.floor();
        for (int tick = 0; tick < 65; ++tick) { cooldown.tick(); }
        PlayerInputData hold_fire = input();
        hold_fire.primary_shoot = true;
        cooldown.tick(hold_fire);
        const size_t paid_projectiles = cooldown.emitted_projectiles;
        bool cooldown_second_frame = false;
        for (int tick = 0; tick < 20; ++tick)
        {
            cooldown.tick(hold_fire);
            cooldown_second_frame = cooldown_second_frame ||
                (cooldown.player->animation_state() == PlayerAnimationState::firing &&
                    cooldown.player->action_frame() == 1);
        }
        require(cooldown_second_frame && cooldown.emitted_projectiles == paid_projectiles &&
            cooldown.player->animation_state() == PlayerAnimationState::idle,
            "held input during a paid shot's cooldown restarted the firing clip");
        for (const WeaponType tool : { WeaponType::sprayer, WeaponType::roller })
        {
            Course automatic(tuning, feedback, presentation.combat_feedback(), redraw, render, audio, preferences,
                0.0f, Vector2F::ZERO, { 0, -120 }, tool);
            automatic.floor();
            automatic.tick();
            bool held_second_frame = false;
            for (int tick = 0; tick < 30; ++tick)
            {
                automatic.tick(hold_fire);
                held_second_frame = held_second_frame ||
                    (automatic.player->animation_state() == PlayerAnimationState::firing &&
                        automatic.player->action_frame() == 1);
            }
            require(automatic.emitted_projectiles > 1 && held_second_frame,
                "sustained paid automatic fire reset the firing clip before its second frame");
        }

        ContentVariant action_variant(content / "tuning.json");
        const GameTuning empty_tank = action_variant.tuning([](rapidjson::Document& json)
        {
            json["weapons"]["starting_ammo"].SetFloat(0.001f);
            json["weapons"]["ammo_regen_delay"].SetFloat(1000.0f);
            json["weapons"]["ammo_regen_rate"].SetFloat(0.0f);
        });
        for (const WeaponType tool : { WeaponType::sprayer, WeaponType::sniper,
            WeaponType::mister, WeaponType::bucket, WeaponType::roller })
        {
            Course empty(empty_tank, feedback, presentation.combat_feedback(), redraw, render, audio, preferences,
                0.0f, Vector2F::ZERO, { 0, -120 }, tool);
            empty.floor();
            for (int tick = 0; tick < 90; ++tick)
            {
                empty.tick(hold_fire);
                require(empty.player->animation_state() != PlayerAnimationState::firing,
                    "unaffordable held shot displayed a firing clip");
            }
            require(empty.emitted_projectiles == 0,
                "empty-paint fixture unexpectedly emitted a projectile");
        }
        auto hurt = course();
        hurt->floor();
        hurt->tick();
        const float health_before_hit = hurt->player->health();
        hurt->player->on_contact(shot, Vector2F::ZERO, 0);
        require(hurt->player->health() < health_before_hit && hurt->player->health() > 0 &&
            hurt->player->animation_state() == PlayerAnimationState::hurt &&
            hurt->player->action_age() == 0.0f && hurt->player->action_frame() == 0,
            "accepted nonfatal paint damage did not display immediate hurt");
        bool hurt_second_frame = false;
        for (int tick = 0; tick < 15; ++tick)
        {
            hurt->tick();
            hurt_second_frame = hurt_second_frame ||
                (hurt->player->animation_state() == PlayerAnimationState::hurt &&
                    hurt->player->action_frame() == 1);
        }
        require(hurt_second_frame && hurt->player->animation_state() == PlayerAnimationState::idle,
            "hurt skipped recovery or failed to finish");
        hurt->player->set_damage_immune(true);
        hurt->player->on_contact(shot, Vector2F::ZERO, 0);
        require(hurt->player->animation_state() == PlayerAnimationState::idle,
            "ignored paint damage displayed hurt");
        std::cout << "PASS action events: five paid tools and releases, held cooldown, empty tanks, accepted/ignored hurt\n";

        // Change only action durations, then run the same interrupted match.
        // Different visible clips must leave physics, resources and launch
        // results identical through jumps, shots, damage, death and respawn.
        const GameTuning brisk_actions = action_variant.tuning([](rapidjson::Document& json)
        {
            for (const char* key : { "takeoff", "land", "fire", "hurt", "elimination" })
            {
                auto& clip = json["player"]["animations"][key];
                if (clip.HasMember("frame_time")) { clip["frame_time"].SetFloat(0.005f); }
                else { clip.AddMember("frame_time", 0.005f, json.GetAllocator()); }
            }
        });
        Course normal_actions(tuning, feedback, presentation.combat_feedback(), redraw, render, audio, preferences);
        Course brief_actions(brisk_actions, feedback, presentation.combat_feedback(), redraw, render, audio, preferences);
        normal_actions.floor();
        brief_actions.floor();
        bool distinct_pose = false;
        for (int tick = 0; tick < 330; ++tick)
        {
            if (tick == 11)
            {
                normal_actions.player->on_contact(shot, Vector2F::ZERO, 0);
                brief_actions.player->on_contact(shot, Vector2F::ZERO, 0);
            }
            if (tick == 70)
            {
                normal_actions.player->kill();
                brief_actions.player->kill();
            }
            PlayerInputData actions = input(tick == 10 || tick == 290, tick >= 10 && tick < 35,
                tick < 45 ? 1.0f : -1.0f);
            actions.primary_shoot = tick >= 2 && tick < 80;
            actions.shoot_direction_requested = true;
            actions.shoot_direction = Vector2F(1, 0);
            normal_actions.tick(actions);
            brief_actions.tick(actions);
            const Player& a = *normal_actions.player;
            const Player& b = *brief_actions.player;
            distinct_pose = distinct_pose || a.animation_state() != b.animation_state();
            require(a.rectangle() == b.rectangle() && a.velocity() == b.velocity() &&
                a.move_state() == b.move_state() && a.state() == b.state() &&
                a.health() == b.health() && a.weapon_ammo() == b.weapon_ammo() &&
                a.respawn_timer() == b.respawn_timer() && a.facing_right() == b.facing_right() &&
                a.primary_weapon().rotation() == b.primary_weapon().rotation() &&
                a.primary_weapon().nozzle_pos() == b.primary_weapon().nozzle_pos() &&
                normal_actions.emitted_projectiles == brief_actions.emitted_projectiles,
                "action duration or interruption changed movement, weapon, damage or respawn");
        }
        require(distinct_pose && normal_actions.emitted_projectiles > 0,
            "action equivalence route failed to exercise different visible clips and paid shots");
        std::cout << "PASS cosmetic action equivalence through takeoff/fire/hurt interruption, death and respawn\n";
    }
}
