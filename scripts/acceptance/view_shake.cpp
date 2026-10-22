#include "game/pch.h"
#include "game/objects/player.h"
#include "game/objects/player_camera.h"
#include "game/objects/projectile_spray.h"
#include "game/objects/structure.h"
#include "game/objects/view_shake.h"
#include "game/objects/weapon_builder.h"
#include "game/presentation.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/step_timer.h"
#include "engine/render/resolution_manager.h"
#include "engine/render/viewport_manager.h"
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

    bool close_to(float a, float b) { return std::abs(a - b) < 0.0001f; }

    const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));

    float magnitude(const Vector2F& vector) { return std::hypot(vector.x, vector.y); }

    void check_envelope(const ViewShakePresentation& look)
    {
        ViewShake inert;
        inert.hurt();
        inert.eliminate();
        inert.land(look.landing_full_speed);
        inert.update(dt, true);
        require(inert.remaining() == 0.0f && inert.offset() == Vector2F::ZERO,
            "Missing presentation emitted view shake");

        ViewShake a(&look), b(&look);
        a.hurt();
        b.hurt();
        require(close_to(a.remaining(), look.duration), "Fresh hurt did not start view shake");
        bool moved = false;
        for (int tick = 0; tick < 120; ++tick)
        {
            a.update(dt, true);
            b.update(dt, true);
            const Vector2F offset = a.offset();
            require(std::isfinite(offset.x) && std::isfinite(offset.y) &&
                magnitude(offset) <= look.hurt_amplitude + 0.0001f,
                "Hurt view shake escaped its authored amplitude");
            require(offset == b.offset() && a.remaining() == b.remaining(),
                "Equal simulation events produced different camera displacement");
            moved = moved || offset != Vector2F::ZERO;
        }
        require(moved && a.remaining() == 0.0f && a.offset() == Vector2F::ZERO,
            "View shake did not move and return exactly to its base camera");

        a.hurt();
        a.update(dt, true);
        const float remaining = a.remaining();
        const Vector2F offset = a.offset();
        for (const float frozen : { 0.0f, -dt, std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::quiet_NaN() })
        {
            a.update(frozen, true);
            a.update(frozen, false);
            require(a.remaining() == remaining && a.offset() == offset,
                "Frozen or invalid delta mutated view shake");
        }
        for (int hit = 0; hit < 1000; ++hit) { a.hurt(); }
        require(a.remaining() == remaining && a.offset() == offset,
            "Repeated damage restarted or stacked view shake inside its limiter");
        a.eliminate();
        require(close_to(a.remaining(), look.duration),
            "Stronger elimination failed to replace active hurt shake");
        a.update(dt, true);
        const float elimination_remaining = a.remaining();
        const Vector2F elimination_offset = a.offset();
        a.hurt();
        a.land(look.landing_full_speed);
        require(a.remaining() == elimination_remaining && a.offset() == elimination_offset,
            "Weaker event extended or replaced an active elimination shake");
        for (int tick = 0; tick < 60; ++tick)
        {
            a.hurt();
            a.land(look.landing_full_speed);
            a.update(dt, true);
            require(magnitude(a.offset()) <= std::max({ look.hurt_amplitude,
                look.elimination_amplitude, look.landing_amplitude }) + 0.0001f,
                "Combined events accumulated unbounded camera displacement");
        }
        a.update(1000.0f, true);
        require(a.remaining() == 0.0f && a.offset() == Vector2F::ZERO,
            "Long simulation tick retained view shake");
        a.hurt();
        require(close_to(a.remaining(), look.duration), "Expired hurt limiter failed to rearm");
        a.update(dt, false);
        require(a.remaining() == 0.0f && a.offset() == Vector2F::ZERO,
            "Disabling view shake retained active displacement");
        a.clear();
        a.clear();
        a.hurt();
        require(close_to(a.remaining(), look.duration), "Clear retained a prior life's hurt limiter");
        std::cout << "PASS view shake envelope: deterministic bounded events, repeat limiter, frozen/invalid deltas, expiry and reset\n";
    }

    void check_live_presentation(const ViewShakePresentation& look)
    {
        ViewShakePresentation editable = look;
        ViewShake active(&editable), reference(&look);
        active.hurt();
        reference.hurt();
        active.update(dt, true);
        reference.update(dt, true);
        // A developer reload changes the borrowed data in place. An in-flight
        // accent must retain its original envelope rather than dividing its
        // remaining time by the new, shorter duration and amplifying itself.
        editable.duration = 0.001f;
        editable.cycles = 3.0f;
        editable.hurt_amplitude = 8.0f;
        for (int tick = 0; tick < 120; ++tick)
        {
            require(active.offset() == reference.offset() &&
                active.remaining() == reference.remaining() &&
                magnitude(active.offset()) <= look.hurt_amplitude + 0.0001f,
                "Live presentation edit changed or amplified an active view shake envelope");
            active.update(dt, true);
            reference.update(dt, true);
        }
        active.clear();
        active.hurt();
        require(close_to(active.remaining(), editable.duration) &&
            magnitude(active.offset()) <= editable.hurt_amplitude + 0.0001f,
            "New shake did not adopt the edited presentation");
        std::cout << "PASS view shake live presentation: active envelope retained, next event uses edited values\n";
    }

    void check_landings(const ViewShakePresentation& look)
    {
        ViewShake shake(&look);
        for (const float speed : { -1.0f, 0.0f, look.landing_min_speed,
            std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() })
        {
            shake.land(speed);
            require(shake.remaining() == 0.0f && shake.offset() == Vector2F::ZERO,
                "Small, invalid or threshold landing emitted view shake");
        }
        ViewShake half(&look), full(&look), extreme(&look);
        half.land((look.landing_min_speed + look.landing_full_speed) / 2.0f);
        full.land(look.landing_full_speed);
        extreme.land(look.landing_full_speed * 1000.0f);
        bool moved = false;
        for (int tick = 0; tick < 120; ++tick)
        {
            half.update(dt, true);
            full.update(dt, true);
            extreme.update(dt, true);
            require(magnitude(half.offset()) <= magnitude(full.offset()) + 0.0001f &&
                magnitude(full.offset()) <= look.landing_amplitude + 0.0001f &&
                full.offset() == extreme.offset(),
                "Landing strength failed to scale and clamp to its authored maximum");
            moved = moved || full.offset() != Vector2F::ZERO;
        }
        require(moved && full.offset() == Vector2F::ZERO && half.offset() == Vector2F::ZERO,
            "Accepted landing failed to move and settle");
        std::cout << "PASS view shake landing: speed threshold, proportional strength and maximum impact clamp\n";
    }

    void check_render_camera(const CameraPresentation& framing)
    {
        ResolutionManager resolution;
        ViewportManager panes(&resolution);
        for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080),
            Vector2I(2560, 1440), Vector2I(1281, 721) })
        {
            resolution.set_resolution_exactly(size);
            for (int players = 1; players <= 4; ++players)
            {
                const ScreenLayout layouts[] = { ScreenLayout::one_player, ScreenLayout::two_player,
                    ScreenLayout::three_player, ScreenLayout::four_player };
                panes.set_layout(layouts[players - 1]);
                for (int view = 0; view < players; ++view)
                {
                    const Viewport pane(panes.player_viewport(view).pixel_rect());
                    const float scale = static_cast<float>(size.y) / framing.reference_height;
                    const Vector2F world_size(pane.width / scale, pane.height / scale);
                    for (const RectangleF bounds : { RectangleF(-120, 80, 6000, 3600),
                        RectangleF(-120, 80, 600, 300), RectangleF(-120, 80, 600, 3600) })
                    {
                        const Camera base(bounds.x + (bounds.width - world_size.x) / 2.0f,
                            bounds.y + (bounds.height - world_size.y) / 2.0f, scale);
                        const Camera saved = base;
                        for (const Vector2F offset : { Vector2F(3, -2), Vector2F(-10000, -10000),
                            Vector2F(10000, 10000), Vector2F::ZERO })
                        {
                            const Camera rendered = PlayerCamera::displaced(base, pane, offset, bounds);
                            const RectangleF visible = rendered.visible_rectangle(pane);
                            require(base == saved && rendered.scale == base.scale,
                                "Render displacement changed base follow state or view scale");
                            if (world_size.x >= bounds.width)
                            {
                                require(close_to(visible.x + visible.width / 2.0f,
                                    bounds.x + bounds.width / 2.0f),
                                    "Shake moved an oversized horizontal view off centre");
                            }
                            else
                            {
                                require(visible.left() >= bounds.left() - 0.001f &&
                                    visible.right() <= bounds.right() + 0.001f,
                                    "Displaced horizontal view escaped arena bounds");
                                if (offset == Vector2F(3, -2))
                                {
                                    require(close_to(rendered.translation.x - base.translation.x, offset.x),
                                        "Resolution or split layout changed world-space shake strength");
                                }
                            }
                            if (world_size.y >= bounds.height)
                            {
                                require(close_to(visible.y + visible.height / 2.0f,
                                    bounds.y + bounds.height / 2.0f),
                                    "Shake moved an oversized vertical view off centre");
                            }
                            else
                            {
                                require(visible.top() >= bounds.top() - 0.001f &&
                                    visible.bottom() <= bounds.bottom() + 0.001f,
                                    "Displaced vertical view escaped arena bounds");
                            }
                            require(PlayerCamera::displaced(base, pane, Vector2F::ZERO, bounds) == saved,
                                "Zero shake failed to return exactly to the base camera");
                        }
                    }
                }
            }
        }
        std::cout << "PASS displaced camera: 1-4 views at 720p/1080p/QHD/odd sizes, independent base, arena bounds and small arenas\n";
    }

    struct World
    {
        RenderResources render;
        AudioResources audio;
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

        ProjectileSpray shot()
        {
            return ProjectileSpray(RectangleF(0, 0, 10, 10), Vector2F::ZERO,
                PlayerTeam::b, 1, Colour::white, &this->render,
                resolve_projectile_strip(this->tuning.projectile(ProjectileType::spray), this->render),
                &this->tuning.projectile(ProjectileType::spray));
        }
    };

    // Separate preferences model two independently owned player views. These
    // fixtures use real contacts and paid shots, without devices or a save.
    struct Course
    {
        PlaySettings preferences;
        Scene scene{ nullptr, nullptr };
        Player* player;
        size_t emitted_projectiles = 0;

        Course(World& world, bool enabled)
        {
            this->preferences.view_shake = enabled;
            const RectangleF rectangle(0, -world.tuning.player().size.y,
                world.tuning.player().size.x, world.tuning.player().size.y);
            auto weapon = WeaponBuilder::build_weapon(&world.tuning, PlayerTeam::a,
                0, 0, Colour::white, WeaponType::sprayer, rectangle.center(),
                &world.render, &world.audio, &this->preferences);
            this->player = this->scene.add(std::make_unique<Player>(rectangle,
                &world.tuning, &world.presentation.movement_feedback(),
                &world.presentation.combat_feedback(), &world.presentation.respawn_redraw(),
                &world.render, &world.audio, &this->preferences, 0, PlayerTeam::a,
                WeaponType::sprayer, std::move(weapon), WeaponType::none, Colour::white,
                Viewport{}, rectangle.top_left(), Vector2F::ZERO, 0.0f, Vector2F::ZERO,
                SpriteFlip::none, 0.0f, -1, nullptr, &world.presentation.camera().shake));
            const RectangleF floor(-10000, 0, 20000, 100);
            this->scene.add(std::make_unique<Structure>("sprite_sheet_1", "square_white_4",
                floor, std::make_unique<RectangleF>(floor), &world.render,
                CollisionObjectType::structure));
            this->scene.end_tick();
        }

        void tick(bool jump = false, float movement = 0.0f, bool fire = false, bool held = false)
        {
            PlayerInputData actions;
            actions.connected = true;
            actions.jump_pressed = jump;
            actions.jump_held = jump || held;
            actions.x_movement = movement;
            actions.primary_shoot = fire;
            actions.shoot_direction_requested = true;
            actions.shoot_direction = { 1.0f, 0.0f };
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
            for (int hit = 0; hit < 10000 && this->player->health() > 0.0f; ++hit)
            {
                this->hit(shot);
            }
            require(this->player->health() <= 0.0f, "View shake fixture did not eliminate player");
        }
    };

    void check_player_events(World& world)
    {
        Course affected(world, true), neighbour(world, true);
        affected.tick();
        neighbour.tick();
        require(affected.player->view_shake().remaining() == 0.0f,
            "Initial spawn support emitted landing shake");
        const ProjectileSpray shot = world.shot();
        affected.player->set_damage_immune(true);
        affected.hit(shot);
        require(affected.player->view_shake().remaining() == 0.0f,
            "Rejected immune damage emitted view shake");
        affected.player->set_damage_immune(false);
        affected.hit(shot);
        require(affected.player->view_shake().remaining() > 0.0f &&
            neighbour.player->view_shake().remaining() == 0.0f,
            "Accepted damage failed to shake only its own player view");
        affected.player->place_at({ 0, -world.tuning.player().size.y });
        require(affected.player->view_shake().remaining() == 0.0f,
            "Developer placement retained view shake");
        affected.tick();
        affected.tick(true);
        // A held jump returns above the authored impact threshold; the short
        // jump used by the equivalence route intentionally need not shake.
        for (int tick = 0; tick < 20; ++tick) { affected.tick(false, 0.0f, false, true); }
        bool airborne = false;
        bool landed = false;
        for (int tick = 0; tick < 240; ++tick)
        {
            affected.tick();
            airborne = airborne || affected.player->move_state() == PlayerMoveState::in_air ||
                affected.player->move_state() == PlayerMoveState::jumping;
            if (airborne && affected.player->move_state() == PlayerMoveState::on_ground)
            {
                landed = true;
                require(affected.player->view_shake().remaining() > 0.0f,
                    "Established high-speed landing failed to emit view shake");
                break;
            }
        }
        require(landed, "View shake landing fixture never returned to support");
        affected.eliminate(shot);
        affected.tick();
        require(affected.player->state() == PlayerState::dead &&
            close_to(affected.player->view_shake().remaining(), world.presentation.camera().shake.duration),
            "Fatal transition failed to replace shake with one elimination event");
        affected.tick();
        const float remaining = affected.player->view_shake().remaining();
        affected.hit(shot);
        require(affected.player->view_shake().remaining() == remaining,
            "Dead contact restarted view shake");
        affected.player->force_respawn();
        require(affected.player->view_shake().remaining() == 0.0f &&
            affected.player->view_shake().offset() == Vector2F::ZERO,
            "Forced respawn retained previous-life displacement");
        affected.hit(shot);
        affected.player->clear_view_shake();
        require(affected.player->view_shake().remaining() == 0.0f,
            "Level lifecycle clear retained view shake");
        affected.hit(shot);
        affected.preferences.view_shake = false;
        affected.tick();
        affected.hit(shot);
        require(affected.player->view_shake().remaining() == 0.0f &&
            affected.player->view_shake().offset() == Vector2F::ZERO,
            "Live preference disable retained or admitted a player shake event");
        affected.preferences.view_shake = true;
        affected.tick();
        require(affected.player->view_shake().remaining() == 0.0f,
            "Re-enabling view shake replayed a disabled event");
        std::cout << "PASS player view shake: accepted damage, private view, spawn/landing, dead contacts and lifecycle resets\n";
    }

    void check_reduced_preference(World& world)
    {
        PlaySettings preferences;
        for (const bool enabled : { false, true })
        {
            for (const bool reduced : { false, true })
            {
                preferences.view_shake = enabled;
                preferences.reduce_effects = reduced;
                require(preferences.view_shake_enabled() == (enabled && !reduced),
                    "Reduced-effects camera policy lost the separate view-shake preference");
            }
        }
        Course course(world, true);
        course.tick();
        const ProjectileSpray shot = world.shot();
        course.hit(shot);
        require(course.player->view_shake().remaining() > 0.0f,
            "Live reduced-effects fixture did not begin a damage accent");
        course.preferences.reduce_effects = true;
        course.tick();
        course.hit(shot);
        require(course.preferences.view_shake && course.player->view_shake().remaining() == 0.0f &&
            course.player->view_shake().offset() == Vector2F::ZERO,
            "Reduced effects retained or admitted shake, or overwrote the independent preference");
        course.eliminate(shot);
        course.tick();
        require(course.player->state() == PlayerState::dead &&
            course.player->view_shake().remaining() == 0.0f &&
            course.player->combat_feedback().elimination_remaining() > 0.0f,
            "Reduced elimination shook the view or removed useful elimination feedback");
        course.preferences.reduce_effects = false;
        course.tick();
        require(course.player->view_shake().remaining() == 0.0f,
            "Restoring effects replayed a suppressed camera event");
        course.player->force_respawn();
        course.hit(shot);
        require(course.player->view_shake().remaining() > 0.0f,
            "Restored effects failed to honour the preserved view-shake preference");
        std::cout << "PASS view shake reduced-effects preference: independent setting, live clear, gated damage/death and fresh restore\n";
    }

    void check_simulation_equivalence(World& world)
    {
        Course moving(world, true), quiet(world, true);
        quiet.preferences.reduce_effects = true;
        const ProjectileSpray shot = world.shot();
        bool shake_seen = false;
        bool respawned = false;
        for (int tick = 0; tick < 360; ++tick)
        {
            if (tick == 2 || tick == 4 || tick == 300) { moving.hit(shot); quiet.hit(shot); }
            if (tick == 70) { moving.eliminate(shot); quiet.eliminate(shot); }
            const bool jump = tick == 10 || tick == 290;
            const float direction = tick < 45 ? 1.0f : -1.0f;
            const bool fire = tick >= 2 && tick < 80;
            moving.tick(jump, direction, fire);
            quiet.tick(jump, direction, fire);
            const Player& a = *moving.player;
            const Player& b = *quiet.player;
            require(a.rectangle() == b.rectangle() && a.velocity() == b.velocity() &&
                a.move_state() == b.move_state() && a.state() == b.state() &&
                a.health() == b.health() && a.weapon_ammo() == b.weapon_ammo() &&
                a.respawn_timer() == b.respawn_timer() && a.facing_right() == b.facing_right() &&
                a.primary_weapon().rotation() == b.primary_weapon().rotation() &&
                a.primary_weapon().nozzle_pos() == b.primary_weapon().nozzle_pos() &&
                moving.emitted_projectiles == quiet.emitted_projectiles,
                "View shake changed movement, aiming, damage, ammunition or respawn simulation");
            require(b.view_shake().remaining() == 0.0f && b.view_shake().offset() == Vector2F::ZERO,
                "Disabled player view accumulated shake");
            shake_seen = shake_seen || a.view_shake().offset() != Vector2F::ZERO;
            respawned = respawned || (tick > 70 && a.state() == PlayerState::alive);
        }
        require(shake_seen && respawned && moving.emitted_projectiles > 0,
            "On/off equivalence route missed camera motion, respawn or paid shots");
        std::cout << "PASS view shake on/off equivalence: movement, aiming, paid shots, damage, death and natural respawn\n";
    }

    void check_validation(const std::filesystem::path& content)
    {
        std::ifstream stream(content / "presentation.json");
        require(stream.good(), "Cannot read view shake presentation content");
        const std::string source(std::istreambuf_iterator<char>(stream), {});
        wchar_t temporary_name[MAX_PATH]{};
        const auto directory = std::filesystem::temp_directory_path();
        require(GetTempFileNameW(directory.c_str(), L"cws", 0, temporary_name) != 0,
            "Cannot reserve view shake presentation variant");
        const std::filesystem::path path(temporary_name);
        struct RemoveOnExit
        {
            std::filesystem::path path;
            ~RemoveOnExit() { std::error_code ignored; std::filesystem::remove(this->path, ignored); }
        } cleanup{ path };
        const auto reject = [&](auto edit)
        {
            rapidjson::Document json;
            json.Parse(source.c_str());
            require(!json.HasParseError(), "Cannot parse view shake fixture");
            edit(json["camera"]["shake"]);
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            json.Accept(writer);
            std::ofstream variant(path);
            variant.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
            variant.close();
            require(variant.good(), "Cannot write view shake fixture");
            bool rejected = false;
            try { const Presentation invalid(path.string().c_str()); }
            catch (const std::exception& error)
            {
                rejected = std::string(error.what()).find("camera.shake") != std::string::npos;
            }
            require(rejected, "Malformed view shake presentation was accepted or lost its section");
        };
        for (const char* field : { "hurt_amplitude", "elimination_amplitude", "landing_amplitude",
            "duration", "cycles", "repeat_delay" })
        {
            reject([field](rapidjson::Value& look) { look[field].SetFloat(0.0f); });
            reject([field](rapidjson::Value& look) { look[field].SetFloat(-1.0f); });
            reject([field](rapidjson::Value& look) { look[field].SetFloat(10000.0f); });
        }
        reject([](rapidjson::Value& look) { look["duration"].SetString("brief"); });
        reject([](rapidjson::Value& look) { look["landing_min_speed"].SetFloat(-1.0f); });
        reject([](rapidjson::Value& look) { look["landing_full_speed"].SetFloat(look["landing_min_speed"].GetFloat()); });
        reject([](rapidjson::Value& look) { look["repeat_delay"].SetFloat(0.001f); });
        std::cout << "PASS view shake content validation: types, bounded amplitudes/timing and ordered landing speeds\n";
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: view_shake.exe <content directory>");
        const std::filesystem::path content(argv[1]);
        World world(content);
        check_envelope(world.presentation.camera().shake);
        check_live_presentation(world.presentation.camera().shake);
        check_landings(world.presentation.camera().shake);
        check_render_camera(world.presentation.camera());
        check_player_events(world);
        check_reduced_preference(world);
        check_simulation_equivalence(world);
        check_validation(content);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL view shake: " << error.what() << '\n';
        return 1;
    }
}
