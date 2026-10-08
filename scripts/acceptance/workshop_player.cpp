#include "game/pch.h"
#include "game/objects/player.h"
#include "game/objects/projectile_spray.h"
#include "game/objects/structure.h"
#include "game/objects/weapon_builder.h"
#include "game/presentation.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/scene/scene.h"

#include <filesystem>
#include <iostream>

using namespace labrador;
using namespace mattmath;

namespace
{
    constexpr float dt = 1.0f / 60.0f;

    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    bool same(const RectangleF& a, const RectangleF& b)
    {
        return a.position() == b.position() && a.size() == b.size();
    }

    // The actual Player, Weapon subclasses and collision resolver run in the
    // same phase order as Level. Cosmetic modes receive identical input streams.
    struct MatchPlayer
    {
        Scene scene{ nullptr, nullptr };
        Player* player = nullptr;

        MatchPlayer(const GameTuning& tuning, const Presentation& presentation,
            RenderResources& render, const AudioResources& audio,
            const PlaySettings& preferences, WeaponType type, PlayerVisualMode mode,
            float floor_right = 30000.0f)
        {
            const RectangleF body({ 0.0f, -tuning.player().size.y }, tuning.player().size);
            auto weapon = WeaponBuilder::build_weapon(&tuning, PlayerTeam::a,
                4, 0, Colour::blue, type, body.center(), &render, &audio, &preferences);
            auto actor = std::make_unique<Player>(body, &tuning,
                &presentation.movement_feedback(), &render, &audio, &preferences,
                4, PlayerTeam::a, type, std::move(weapon), WeaponType::none,
                Colour::blue, Viewport{}, body.position());
            if (mode != PlayerVisualMode::original)
            {
                // Keyboard slot 4 is deliberately outside the 0..3 badge range:
                // the supplied roster index determines the visible identity.
                actor->enable_workshop_visual(&presentation.workshop_player(),
                    &render, mode, 0);
            }
            this->player = this->scene.add(std::move(actor));
            const RectangleF floor(-30000.0f, 0.0f, floor_right + 30000.0f, 100.0f);
            this->scene.add(std::make_unique<Structure>("sprite_sheet_1", "square_white_4",
                floor, std::make_unique<RectangleF>(floor), &render,
                CollisionObjectType::structure));
            this->scene.end_tick();
        }

        std::vector<std::unique_ptr<Projectile>> tick(const PlayerInputData& input)
        {
            this->player->set_player_input(input);
            this->scene.update(dt);
            auto shots = this->player->update_weapon_and_get_projectiles(dt,
                this->scene.collision_objects());
            this->scene.resolve();
            this->player->end_contacts();
            this->player->update_weapon_position();
            this->player->update_prev_rectangle();
            this->scene.end_tick();
            return shots;
        }
    };

    void compare(const Player& baseline, const Player& candidate)
    {
        require(same(baseline.rectangle(), candidate.rectangle()),
            "workshop visuals changed the player collision rectangle");
        require(baseline.velocity() == candidate.velocity(),
            "workshop visuals changed player velocity");
        require(baseline.state() == candidate.state() &&
            baseline.move_state() == candidate.move_state(),
            "workshop visuals changed movement or life state");
        require(baseline.health() == candidate.health() &&
            baseline.weapon_ammo() == candidate.weapon_ammo() &&
            baseline.respawn_timer() == candidate.respawn_timer(),
            "workshop visuals changed health, ammo or respawn timing");
        const auto original_cues = baseline.movement_feedback().effects();
        const auto new_cues = candidate.movement_feedback().effects();
        require(original_cues.size() == new_cues.size(),
            "workshop visuals changed movement feedback count");
        for (size_t index = 0; index < original_cues.size(); ++index)
        {
            require(original_cues[index].kind == new_cues[index].kind &&
                original_cues[index].position == new_cues[index].position &&
                original_cues[index].age == new_cues[index].age,
                "workshop visuals changed feedback timing or placement");
        }
    }

    void compare_shots(std::vector<std::unique_ptr<Projectile>>& baseline,
        std::vector<std::unique_ptr<Projectile>>& candidate)
    {
        require(baseline.size() == candidate.size(), "visual comparison changed shot count");
        for (size_t index = 0; index < baseline.size(); ++index)
        {
            require(same(baseline[index]->shape()->bounding_box(),
                candidate[index]->shape()->bounding_box()),
                "visual comparison moved the simulation muzzle");
            for (int step = 0; step < 4; ++step)
            {
                baseline[index]->update(dt);
                candidate[index]->update(dt);
                require(same(baseline[index]->shape()->bounding_box(),
                    candidate[index]->shape()->bounding_box()),
                    "authored poses quantised or changed projectile trajectories");
                require(baseline[index]->delete_timer() == candidate[index]->delete_timer(),
                    "visual comparison changed projectile lifetime");
            }
        }
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: workshop_player.exe <content directory>");
        const std::filesystem::path content(argv[1]);
        RenderResources render;
        for (const char* name : { "sprite_sheet_1", "weapon_paint", "workshop_kit" })
        {
            render.add_sprite_sheet(name, read_sprite_sheet(
                (content / "textures" / (std::string(name) + ".json")).string().c_str(),
                TextureHandle{}));
        }
        AudioResources audio;
        audio.add_sound_bank("sound_bank_1", SoundBank::silent());
        const PlaySettings preferences;
        const GameTuning tuning((content / "tuning.json").string().c_str());
        const Presentation presentation((content / "presentation.json").string().c_str());
        const WorkshopPlayerPresentation& art = presentation.workshop_player();

        for (const PlayerVisualMode mode : { PlayerVisualMode::continuous,
            PlayerVisualMode::authored })
        {
            MatchPlayer ledge(tuning, presentation, render, audio, preferences,
                WeaponType::sprayer, mode, 60.0f);
            PlayerInputData walk;
            walk.connected = true;
            ledge.tick(walk);
            require(ledge.player->move_state() == PlayerMoveState::on_ground,
                "workshop ledge fixture did not establish ground support");
            walk.x_movement = 1.0f;
            bool left_ledge = false;
            for (int tick = 0; tick < 120; ++tick)
            {
                // tick ends immediately after end_contacts; no following
                // Player::update may conceal a one-tick stale grounded pose.
                ledge.tick(walk);
                for (const auto& effect : ledge.player->movement_feedback().effects())
                {
                    require(effect.kind != MovementFeedbackKind::takeoff,
                        "walking off a ledge emitted a jump puff");
                }
                if (ledge.player->move_state() == PlayerMoveState::in_air)
                {
                    require(ledge.player->workshop_visual()->body_frame_index() == 6,
                        "walking off support did not select the air pose on its contact tick");
                    left_ledge = true;
                    break;
                }
                require(ledge.player->move_state() == PlayerMoveState::on_ground,
                    "workshop ledge fixture entered an unexpected movement state");
            }
            require(left_ledge, "workshop ledge fixture never walked off its support");
        }

        size_t comparisons = 0;
        for (const WeaponType type : { WeaponType::sprayer, WeaponType::roller,
            WeaponType::sniper, WeaponType::mister, WeaponType::bucket })
        {
            for (const PlayerVisualMode mode : { PlayerVisualMode::continuous,
                PlayerVisualMode::authored })
            {
                MatchPlayer baseline(tuning, presentation, render, audio, preferences,
                    type, PlayerVisualMode::original);
                MatchPlayer candidate(tuning, presentation, render, audio, preferences, type, mode);
                bool saw_air = false;
                bool saw_landing = false;
                bool saw_run_change = false;
                size_t previous_frame = candidate.player->workshop_visual()->body_frame_index();
                for (int tick = 0; tick < 720; ++tick)
                {
                    PlayerInputData input;
                    input.connected = true;
                    input.x_movement = tick % 240 < 100 ? 1.0f :
                        (tick % 240 < 200 ? -1.0f : 0.0f);
                    input.jump_pressed = tick % 120 == 30;
                    input.jump_held = tick % 120 >= 30 && tick % 120 < 48;
                    input.primary_shoot = tick % 180 < 120;
                    input.shoot_direction_requested = true;
                    input.shoot_angle = -PI + static_cast<float>(tick) * (2.0f * PI / 719.0f);
                    input.shoot_direction = Vector2F::unit_vec_from_angle(input.shoot_angle);
                    auto original_shots = baseline.tick(input);
                    auto new_shots = candidate.tick(input);
                    compare(*baseline.player, *candidate.player);
                    compare_shots(original_shots, new_shots);
                    const size_t frame = candidate.player->workshop_visual()->body_frame_index();
                    saw_air |= frame == 6;
                    saw_landing |= frame == 7;
                    saw_run_change |= frame >= 2 && frame <= 5 && previous_frame != frame;
                    previous_frame = frame;
                    const RectangleF body = candidate.player->rectangle();
                    require(candidate.player->bounds().contains(body),
                        "workshop culling omitted the player body");
                    const Vector2F badge_center(body.center().x + art.badge_offset.x,
                        body.bottom() - art.body_size.y + art.badge_offset.y);
                    require(candidate.player->bounds().contains(RectangleF(
                        badge_center - art.badge_size / 2.0f, art.badge_size)),
                        "workshop culling omitted the player number badge");
                    ++comparisons;
                }
                require(saw_air && saw_landing && saw_run_change,
                    "real movement did not select airborne, landing and animated run poses");

                const PlayerVisual* visual = candidate.player->workshop_visual();
                const float paused_time = visual->animation_time();
                const size_t paused_frame = visual->body_frame_index();
                candidate.player->pause_sounds();
                candidate.player->reset_jump_input();
                for (int view = 0; view < 4; ++view)
                {
                    std::ignore = candidate.player->bounds();
                    require(visual->animation_time() == paused_time &&
                        visual->body_frame_index() == paused_frame,
                        "paused observation or additional views advanced body animation");
                }
                candidate.player->resume_sounds();
                ProjectileSpray hit(RectangleF(0.0f, 0.0f, 15.0f, 15.0f),
                    Vector2F::ZERO, PlayerTeam::b, 1, Colour::orange,
                    &render, &tuning.projectile(SPRAY));
                for (int shot = 0; shot < 200; ++shot)
                {
                    baseline.player->on_contact(hit, Vector2F::ZERO, 0.0f);
                    candidate.player->on_contact(hit, Vector2F::ZERO, 0.0f);
                }
                PlayerInputData released;
                released.connected = true;
                baseline.tick(released);
                candidate.tick(released);
                require(candidate.player->state() == PlayerState::dead,
                    "workshop death fixture did not eliminate the player");
                const float dead_time = visual->animation_time();
                bool respawned = false;
                for (int tick = 0; tick < 1200; ++tick)
                {
                    baseline.tick(released);
                    candidate.tick(released);
                    compare(*baseline.player, *candidate.player);
                    if (candidate.player->state() == PlayerState::alive)
                    {
                        require(visual->animation_time() == 0.0f &&
                            visual->body_frame_index() == 6,
                            "respawn retained an old workshop body animation");
                        respawned = true;
                        break;
                    }
                    require(visual->animation_time() == dead_time,
                        "dead player's body animation continued to advance");
                }
                require(respawned, "workshop death fixture did not respawn");
            }
        }
        std::cout << "PASS workshop player: " << comparisons
            << " exact simulation/shot comparisons, all five tools, two modes, movement poses, "
               "same-tick ledge departure, pause/death/respawn and badge culling.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL workshop player: " << error.what() << '\n';
        return 1;
    }
}
