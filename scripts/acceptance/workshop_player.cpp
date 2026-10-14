#include "game/pch.h"
#include "game/objects/player.h"
#include "game/objects/team_colour.h"
#include "game/objects/projectile_spray.h"
#include "game/objects/structure.h"
#include "game/objects/weapon_builder.h"
#include "game/presentation.h"
#include "engine/assets/json.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/scene/scene.h"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>

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
            float floor_right = 30000.0f, const WorkshopPlayerPresentation* art = nullptr)
        {
            const RectangleF body({ 0.0f, -tuning.player().size.y }, tuning.player().size);
            auto weapon = WeaponBuilder::build_weapon(&tuning, PlayerTeam::a,
                4, 0, Colour::blue, type, body.center(), &render, &audio, &preferences);
            auto actor = std::make_unique<Player>(body, &tuning,
                &presentation.movement_feedback(), &presentation.respawn_redraw(), &render, &audio, &preferences,
                4, PlayerTeam::a, type, std::move(weapon), WeaponType::none,
                Colour::blue, Viewport{}, body.position());
            if (mode != PlayerVisualMode::original)
            {
                // Keyboard slot 4 is deliberately outside the 0..3 badge range:
                // the supplied roster index determines the visible identity.
                actor->enable_workshop_visual(art != nullptr ? art :
                    &presentation.workshop_player(), &render, mode, 0);
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

    PlayerInputData aim_at(float angle, float x_movement = 0.0f)
    {
        PlayerInputData input;
        input.connected = true;
        input.x_movement = x_movement;
        input.shoot_direction_requested = true;
        input.shoot_angle = angle;
        input.shoot_direction = Vector2F::unit_vec_from_angle(angle);
        return input;
    }

    // The authored pose a history-free choice would draw for this aim.
    size_t nearest_pose(const WorkshopPlayerPresentation& art, float aim)
    {
        const float right_half_aim = std::atan2(std::sin(aim), std::abs(std::cos(aim)));
        size_t nearest = 0;
        for (size_t pose = 1; pose < art.sprayer_poses.size(); ++pose)
        {
            if (std::abs(right_half_aim - art.sprayer_poses[pose].aim_angle) <
                std::abs(right_half_aim - art.sprayer_poses[nearest].aim_angle))
            {
                nearest = pose;
            }
        }
        return nearest;
    }

    size_t drawn_pose(const Player& player)
    {
        return player.workshop_visual()->sprayer_pose_index(player.primary_weapon());
    }

    struct PoseTrace
    {
        float largest_grip_step = 0.0f;
        int pose_changes = 0;
    };

    // Front grip relative to the player centre, so walking is not counted.
    PoseTrace trace_aim(MatchPlayer& fixture, const std::vector<float>& angles)
    {
        PoseTrace result;
        std::optional<Vector2F> previous_grip;
        std::optional<size_t> previous_pose;
        for (const float angle : angles)
        {
            fixture.tick(aim_at(angle));
            const Player& player = *fixture.player;
            const Vector2F grip = player.workshop_visual()->arms(player.rectangle(),
                player.facing_right(), player.primary_weapon())[1].grip -
                player.rectangle().center();
            const size_t pose = drawn_pose(player);
            if (previous_grip.has_value())
            {
                result.largest_grip_step = std::max(result.largest_grip_step,
                    (grip - *previous_grip).length());
                result.pose_changes += pose != *previous_pose ? 1 : 0;
            }
            previous_grip = grip;
            previous_pose = pose;
        }
        return result;
    }

    struct ArmTally
    {
        int bent = 0;
        int straight = 0;
    };

    // The arm contract, from data alone: two equal bones of arm_length / 2 sag
    // off the span by at most elbow_drop, a reach at least arm_length long
    // draws straight, and a mostly level span bends below itself. Shoulder
    // and grip are exact; the elbow is snapped, which moves it at most half a
    // diagonal texel.
    void check_arm(const WorkshopPlayerPresentation& art, const PlayerVisual::ArmPose& arm,
        ArmTally& tally)
    {
        const float snap = art.pixel_size * 0.75f;
        const Vector2F span = arm.grip - arm.shoulder;
        const float length = span.length();
        const Vector2F offset = arm.elbow - (arm.shoulder + arm.grip) / 2.0f;
        if (length >= art.arm_length)
        {
            require(std::abs(span.x * offset.y - span.y * offset.x) <= snap * length,
                "a reach longer than arm_length did not draw straight");
            ++tally.straight;
            return;
        }
        const float half_bone = art.arm_length / 2.0f;
        const float sag_limit = std::min(art.elbow_drop,
            std::sqrt(half_bone * half_bone - length * length / 4.0f));
        require(offset.length() <= sag_limit + snap, "workshop elbow left its capped sag");
        const float upper = (arm.elbow - arm.shoulder).length();
        const float lower = (arm.grip - arm.elbow).length();
        require(std::abs(upper - lower) <= snap * 2.0f && std::max(upper, lower) <= half_bone + snap,
            "workshop arm bones are not two equal halves of arm_length");
        if (std::abs(span.x) >= std::abs(span.y))
        {
            require(offset.y >= -snap, "a mostly level workshop arm bent upward");
        }
        tally.bent += offset.length() > snap ? 1 : 0;
    }

    struct ElbowTrace
    {
        float largest_step = 0.0f;
        int compared = 0;
    };

    // Elbows relative to the player centre between adjacent aim ticks that keep
    // the facing, the tool's mirrored half and the drawn pose: those must move,
    // never jump sides.
    ElbowTrace trace_elbows(MatchPlayer& fixture, const WorkshopPlayerPresentation& art,
        const std::vector<float>& angles, ArmTally& tally)
    {
        ElbowTrace result;
        std::optional<std::array<Vector2F, 2>> previous;
        bool previous_facing = false;
        bool previous_mirrored = false;
        size_t previous_pose = 0;
        for (const float angle : angles)
        {
            fixture.tick(aim_at(angle));
            const Player& player = *fixture.player;
            const auto arms = player.workshop_visual()->arms(player.rectangle(),
                player.facing_right(), player.primary_weapon());
            std::array<Vector2F, 2> elbows;
            for (size_t side = 0; side < arms.size(); ++side)
            {
                check_arm(art, arms[side], tally);
                elbows[side] = arms[side].elbow - player.rectangle().center();
            }
            const size_t pose = drawn_pose(player);
            const float aim = player.primary_weapon().rotation();
            const bool mirrored = aim > PI / 2.0f || aim < -PI / 2.0f;
            if (previous.has_value() && player.facing_right() == previous_facing &&
                mirrored == previous_mirrored && pose == previous_pose)
            {
                for (size_t side = 0; side < elbows.size(); ++side)
                {
                    const Vector2F step = elbows[side] - (*previous)[side];
                    result.largest_step = std::max({ result.largest_step,
                        std::abs(step.x), std::abs(step.y) });
                    if (std::max(std::abs(step.x), std::abs(step.y)) > art.pixel_size)
                    {
                        throw std::runtime_error("an elbow jumped across its span between "
                            "neighbouring aims: arm " + std::to_string(side) + " at " +
                            std::to_string(angle * 180.0f / PI) + " degrees moved (" +
                            std::to_string(step.x) + ", " + std::to_string(step.y) + ")");
                    }
                }
                ++result.compared;
            }
            previous = elbows;
            previous_facing = player.facing_right();
            previous_mirrored = mirrored;
            previous_pose = pose;
        }
        return result;
    }

    float contrast(float a, float b)
    {
        return (std::max(a, b) + 0.05f) / (std::min(a, b) + 0.05f);
    }

    Colour palette_colour(const JsonValue& palette, const char* symbol)
    {
        std::string text = palette.string(symbol);
        if (!text.empty() && text.front() == '#') { text.erase(0, 1); }
        return Colour(text);
    }

    // The editable source lives with the repository, not in staged content.
    std::filesystem::path find_kit_source(const std::filesystem::path& content)
    {
        for (std::filesystem::path directory = std::filesystem::absolute(content);
            !directory.empty(); directory = directory.parent_path())
        {
            const std::filesystem::path candidate =
                directory / "game" / "art" / "polish" / "kit" / "source.json";
            if (std::filesystem::exists(candidate)) { return candidate; }
            if (directory == directory.parent_path()) { break; }
        }
        throw std::runtime_error("workshop kit source.json is not above the content directory");
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
        ArmTally arm_tally;
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
                    const RectangleF culled = candidate.player->bounds();
                    const PlayerVisual& shown = *candidate.player->workshop_visual();
                    const bool facing_right = candidate.player->facing_right();
                    require(culled.contains(body),
                        "workshop culling omitted the player body");
                    // Recompute the mirrored badge from data, not the accessor alone.
                    const Vector2F badge_center(
                        body.center().x + art.badge_offset.x * (facing_right ? 1.0f : -1.0f),
                        body.bottom() - art.body_size.y + art.badge_offset.y);
                    const RectangleF badge(badge_center - art.badge_size / 2.0f, art.badge_size);
                    require(culled.contains(badge) &&
                        same(badge, shown.badge_bounds(body, facing_right)),
                        "workshop culling omitted the mirrored player number badge");
                    for (const PlayerVisual::ArmPose& arm : shown.arms(body, facing_right,
                        candidate.player->primary_weapon()))
                    {
                        for (const Vector2F& joint : { arm.shoulder, arm.elbow, arm.grip })
                        {
                            require(culled.contains(RectangleF(joint - Vector2F(art.sleeve_width),
                                Vector2F(art.sleeve_width * 2.0f))),
                                "workshop culling omitted an arm joint");
                        }
                        check_arm(art, arm, arm_tally);
                    }
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
                    &render, &tuning.projectile(ProjectileType::spray));
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
                        require(type != WeaponType::sprayer || mode != PlayerVisualMode::authored ||
                            drawn_pose(*candidate.player) == nearest_pose(art,
                                candidate.player->primary_weapon().rotation()),
                            "respawn retained an old held sprayer pose");
                        respawned = true;
                        break;
                    }
                    require(visual->animation_time() == dead_time,
                        "dead player's body animation continued to advance");
                }
                require(respawned, "workshop death fixture did not respawn");
            }
        }
        // Posed sprayer: hold the drawn pose with hysteresis. Compare against a
        // copy of the same data with no band, which is the nearest-pose rule.
        WorkshopPlayerPresentation no_band = art;
        no_band.pose_hysteresis = 0.0f;
        {
            MatchPlayer fresh(tuning, presentation, render, audio, preferences,
                WeaponType::sprayer, PlayerVisualMode::authored);
            require(drawn_pose(*fresh.player) == nearest_pose(art,
                fresh.player->primary_weapon().rotation()),
                "before its first update the posed sprayer did not follow the weapon");
        }
        std::vector<float> sweep;
        for (int step = 0; step <= 1440; ++step)
        {
            sweep.push_back(-PI + static_cast<float>(step) * (PI / 720.0f));
        }
        MatchPlayer banded_sweep(tuning, presentation, render, audio, preferences,
            WeaponType::sprayer, PlayerVisualMode::authored);
        MatchPlayer nearest_sweep(tuning, presentation, render, audio, preferences,
            WeaponType::sprayer, PlayerVisualMode::authored, 30000.0f, &no_band);
        MatchPlayer rotating_sweep(tuning, presentation, render, audio, preferences,
            WeaponType::sprayer, PlayerVisualMode::continuous);
        const PoseTrace banded = trace_aim(banded_sweep, sweep);
        const PoseTrace nearest = trace_aim(nearest_sweep, sweep);
        const PoseTrace rotating = trace_aim(rotating_sweep, sweep);
        // Hysteresis moves where a pose changes, not how far its grip jumps.
        require(banded.largest_grip_step <= nearest.largest_grip_step + 0.01f,
            "pose hysteresis enlarged the posed grip jump");
        require(banded.pose_changes == nearest.pose_changes,
            "a slow monotonic sweep changed pose a different number of times");

        // A tenth-degree sweep of every aimed tool: an elbow may not cross its
        // span between neighbouring aims inside one pose and facing. An honest
        // step stays within a texel; a side flip is about six.
        std::vector<float> fine_sweep;
        for (int step = 0; step <= 3600; ++step)
        {
            fine_sweep.push_back(-PI + static_cast<float>(step) * (PI / 1800.0f));
        }
        ElbowTrace elbow_trace;
        for (const WeaponType type : { WeaponType::sprayer, WeaponType::sniper,
            WeaponType::mister, WeaponType::bucket })
        {
            for (const PlayerVisualMode mode : { PlayerVisualMode::continuous,
                PlayerVisualMode::authored })
            {
                MatchPlayer fixture(tuning, presentation, render, audio, preferences, type, mode);
                const ElbowTrace trace = trace_elbows(fixture, art, fine_sweep, arm_tally);
                elbow_trace.largest_step = std::max(elbow_trace.largest_step, trace.largest_step);
                elbow_trace.compared += trace.compared;
            }
        }
        require(elbow_trace.compared > 20000, "the elbow sweep compared too few aim ticks");
        require(elbow_trace.largest_step <= art.pixel_size,
            "an elbow jumped across its span between neighbouring aims");
        require(arm_tally.bent > 0 && arm_tally.straight > 0,
            "the arm fixtures did not cover both bent and straight reaches");

        // Dither +-2 degrees about the 0/45 boundary, like stick noise.
        const float boundary = (art.sprayer_poses[2].aim_angle + art.sprayer_poses[3].aim_angle) / 2.0f;
        const float two_degrees = 2.0f * PI / 180.0f;
        std::vector<float> dither(8, boundary - two_degrees);
        for (int tick = 0; tick < 240; ++tick)
        {
            dither.push_back(boundary + (tick % 2 == 0 ? two_degrees : -two_degrees));
        }
        MatchPlayer banded_dither(tuning, presentation, render, audio, preferences,
            WeaponType::sprayer, PlayerVisualMode::authored);
        MatchPlayer nearest_dither(tuning, presentation, render, audio, preferences,
            WeaponType::sprayer, PlayerVisualMode::authored, 30000.0f, &no_band);
        const PoseTrace held = trace_aim(banded_dither, dither);
        const PoseTrace flicker = trace_aim(nearest_dither, dither);
        require(held.pose_changes == 0, "aim noise at a pose boundary flipped the held pose");
        require(flicker.pose_changes >= 200, "the dither fixture does not cross a pose boundary");
        // Past the band the pose still changes, and back again.
        const float beyond = art.pose_hysteresis + 1.0f * PI / 180.0f;
        banded_dither.tick(aim_at(boundary + beyond));
        require(drawn_pose(*banded_dither.player) == 3, "aim past the hysteresis band kept the old pose");
        banded_dither.tick(aim_at(boundary - beyond));
        require(drawn_pose(*banded_dither.player) == 2, "aim back past the band kept the new pose");
        // Inside the band the held pose differs from the nearest; respawn
        // must forget it rather than carry it into the next life.
        const float inside = boundary + art.pose_hysteresis / 2.0f;
        banded_dither.tick(aim_at(inside));
        require(drawn_pose(*banded_dither.player) == 2 && nearest_pose(art, inside) == 3,
            "the hysteresis band did not hold the previous pose");
        ProjectileSpray fatal(RectangleF(0.0f, 0.0f, 15.0f, 15.0f),
            Vector2F::ZERO, PlayerTeam::b, 1, Colour::orange, &render, &tuning.projectile(ProjectileType::spray));
        for (int shot = 0; shot < 200; ++shot)
        {
            banded_dither.player->on_contact(fatal, Vector2F::ZERO, 0.0f);
        }
        bool reborn = false;
        for (int tick = 0; tick < 1200 && !reborn; ++tick)
        {
            banded_dither.tick(aim_at(inside));
            reborn = tick > 0 && banded_dither.player->state() == PlayerState::alive;
        }
        require(reborn && drawn_pose(*banded_dither.player) == 3,
            "respawn kept a held sprayer pose instead of the nearest");

        // Straight up, for either facing: the badge keeps clear of the muzzle
        // (and is drawn before the tool, so the tool would cover it anyway).
        size_t badge_checks = 0;
        for (const PlayerVisualMode mode : { PlayerVisualMode::continuous,
            PlayerVisualMode::authored })
        {
            for (const float walk : { 1.0f, -1.0f })
            {
                MatchPlayer upward(tuning, presentation, render, audio, preferences,
                    WeaponType::sprayer, mode);
                upward.tick(aim_at(-PI / 2.0f, walk));
                upward.tick(aim_at(-PI / 2.0f));
                const Player& player = *upward.player;
                require(player.facing_right() == (walk > 0.0f), "badge fixture did not set facing");
                const RectangleF badge = player.workshop_visual()->badge_bounds(
                    player.rectangle(), player.facing_right());
                const Vector2F muzzle = player.primary_weapon().nozzle_pos();
                require(!badge.intersects(RectangleF(muzzle - Vector2F(art.pixel_size),
                    Vector2F(art.pixel_size * 2.0f))), "the badge covers an upward muzzle");
                require(player.bounds().contains(badge), "workshop culling omitted the badge");
                ++badge_checks;
            }
        }

        // Dark-team contrast: every palette team times every mask tone after
        // the cosmetic luminance floor, against the outline and the Movement
        // Course background read from data. Thresholds (WCAG ratio):
        //   every tone vs outline     >= 1.40 (unfloored blue shadow was 1.02)
        //   lit tone 't' vs background >= 2.00 (unfloored blue was 1.01)
        // A mid-value background still meets some tone of every ramp; the
        // table records those rows rather than asserting them.
        constexpr float minimum_outline_contrast = 1.40f;
        constexpr float minimum_lit_background_contrast = 2.00f;
        const JsonDocument kit = read_json_file(find_kit_source(content).string().c_str());
        const JsonValue palette = kit.root().object("palette");
        const Colour outline = palette_colour(palette, "o");
        require(outline == art.arm_outline_colour, "arm outline differs from the kit outline");
        const JsonDocument course = read_json_file(
            (content / "levels" / "movement_course.json").string().c_str());
        std::optional<Colour> background;
        const JsonValue scenery = course.root().array("non_collision_objects");
        for (size_t index = 0; index < scenery.size(); ++index)
        {
            if (scenery.at(index).string("name") == "course_background")
            {
                const std::string name = scenery.at(index).string("colour");
                background = colour_from_name(name);
                if (!background && name.size() == 6) { background = Colour(name); }
            }
        }
        require(background.has_value(), "Movement Course background colour was not found");
        const float floor = art.team_mask_minimum_luminance;
        const float outline_luminance = PlayerVisual::relative_luminance(outline);
        const float background_luminance = PlayerVisual::relative_luminance(*background);
        std::cout << std::fixed << std::setprecision(2)
            << "Workshop mask contrast (floor " << floor << "; outline/background):\n";
        float weakest_outline = std::numeric_limits<float>::max();
        float weakest_lit = std::numeric_limits<float>::max();
        for (const NamedTeamColour& entry : presentation.team_palette().colours)
        {
            const std::string& name = entry.name;
            const Colour team = entry.colour;
            const Colour tint = PlayerVisual::mask_tint(team, floor);
            require(PlayerVisual::relative_luminance(tint) >= floor - 0.0005f,
                "the mask tint did not reach its luminance floor");
            require(floor > 0.0f || tint == team, "a zero floor changed the team colour");
            std::cout << "  " << std::left << std::setw(7) << name << std::right;
            for (const char* tone : { "t", "u", "v" })
            {
                const Colour shade = palette_colour(palette, tone);
                const float shown = PlayerVisual::relative_luminance(Colour(
                    shade.r * tint.r, shade.g * tint.g, shade.b * tint.b));
                const float bare = PlayerVisual::relative_luminance(Colour(
                    shade.r * team.r, shade.g * team.g, shade.b * team.b));
                const float against_outline = contrast(shown, outline_luminance);
                const float against_background = contrast(shown, background_luminance);
                weakest_outline = std::min(weakest_outline, against_outline);
                if (std::string(tone) == "t")
                {
                    weakest_lit = std::min(weakest_lit, against_background);
                }
                std::cout << "  " << tone << ' ' << against_outline << '/' << against_background
                    << " (unfloored " << contrast(bare, outline_luminance) << '/'
                    << contrast(bare, background_luminance) << ')';
            }
            std::cout << '\n';
        }
        require(weakest_outline >= minimum_outline_contrast,
            "a team mask tone merges with the outline");
        require(weakest_lit >= minimum_lit_background_contrast,
            "a lit team mask merges with the Movement Course background");
        std::cout << std::setprecision(1)
            << "Posed grip step per tick, quarter-degree sweep: " << banded.largest_grip_step
            << " with " << art.pose_hysteresis * 180.0f / PI << "-degree hysteresis, "
            << nearest.largest_grip_step << " nearest-pose, "
            << rotating.largest_grip_step << " rotating; " << banded.pose_changes
            << " pose changes each.\n"
            << "Elbow step per tenth-degree aim tick within a pose and facing: "
            << elbow_trace.largest_step << " over " << elbow_trace.compared
            << " ticks; arms checked " << arm_tally.bent << " bent, " << arm_tally.straight
            << " straight.\n"
            << "Boundary dither +-2 degrees, 240 ticks: " << held.pose_changes
            << " pose changes with hysteresis, " << flicker.pose_changes << " without.\n";
        std::cout << "PASS workshop player: " << comparisons
            << " exact simulation/shot comparisons, all five tools, two modes, movement poses, "
               "same-tick ledge departure, pause/death/respawn, joint and badge culling, "
            << badge_checks << " upward-muzzle badge checks, pose hysteresis and mask contrast.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL workshop player: " << error.what() << '\n';
        return 1;
    }
}
