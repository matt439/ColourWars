#include "game/pch.h"
#include "game/objects/player.h"
#include "game/objects/structure.h"
#include "game/objects/structure_paintable.h"
#include "game/objects/projectile_jet.h"
#include "game/objects/projectile_spray.h"
#include "game/objects/weapon_builder.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/step_timer.h"
#include "engine/scene/scene.h"
#include "scripts/acceptance/support/probe.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace labrador;
using namespace mattmath;

namespace
{
    using acceptance::require;

    const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));
    const TeamColour surface_colours{ Colour::orange, Colour::cyan };

    PlayerInputData input(float movement = 0.0f, bool jump = false, bool fire = false)
    {
        PlayerInputData result;
        result.connected = true;
        result.x_movement = movement;
        result.jump_pressed = jump;
        result.jump_held = jump;
        result.primary_shoot = fire;
        result.shoot_direction_requested = true;
        result.shoot_direction = { 1.0f, 0.0f };
        return result;
    }

    bool contact_kind(MovementFeedbackKind kind)
    {
        return kind == MovementFeedbackKind::step || kind == MovementFeedbackKind::skid;
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
    };

    struct Course
    {
        struct Event { MovementFeedback::Effect effect; int tick; };
        World& world;
        Scene scene{ nullptr, nullptr };
        Player* player;
        std::vector<Event> events;
        int ticks = 0;
        size_t emitted_projectiles = 0;

        explicit Course(World& resources, const MovementFeedbackPresentation* look = nullptr,
            float x = 0.0f, float top = 0.0f) : world(resources)
        {
            const RectangleF rectangle(x, top - world.tuning.player().size.y,
                world.tuning.player().size.x, world.tuning.player().size.y);
            auto weapon = WeaponBuilder::build_weapon(&world.tuning, PlayerTeam::a,
                0, 0, Colour::white, WeaponType::sprayer, rectangle.center(),
                &world.render, &world.audio, &world.preferences);
            this->player = this->scene.add(std::make_unique<Player>(rectangle,
                &world.tuning, look ? look : &world.presentation.movement_feedback(),
                &world.presentation.combat_feedback(), &world.presentation.respawn_redraw(),
                &world.render, &world.audio, &world.preferences, 0, PlayerTeam::a,
                WeaponType::sprayer, std::move(weapon), WeaponType::none,
                Colour::white, Viewport{}, rectangle.top_left()));
            this->scene.end_tick();
        }

        void surface(const RectangleF& rectangle)
        {
            this->scene.add(std::make_unique<Structure>("sprite_sheet_1", "square_white_4",
                rectangle, std::make_unique<RectangleF>(rectangle), &world.render,
                CollisionObjectType::structure));
            this->scene.end_tick();
        }

        StructurePaintable& paintable(const RectangleF& rectangle = { -10000, 0, 20000, 100 },
            const PaintableFaces& faces = { false, true, false, false },
            std::span<const RectangleF> obstacles = {})
        {
            auto* result = this->scene.add(std::make_unique<StructurePaintable>(
                "sprite_sheet_1", "square_white_4", rectangle,
                std::make_unique<RectangleF>(rectangle), &world.render, &world.audio,
                &world.preferences, &world.tuning.paint(), CollisionObjectType::structure_paintable,
                surface_colours, faces, obstacles));
            this->scene.end_tick();
            return *result;
        }

        void begin(const PlayerInputData& actions = input())
        {
            ++this->ticks;
            this->player->set_player_input(actions);
            this->scene.update(dt);
            // Keep the real paid-shot path; projectiles are accounted for and
            // retired by this fixture rather than introducing unrelated hits.
            this->emitted_projectiles += this->player->update_weapon_and_get_projectiles(
                dt, this->scene.collision_objects()).size();
        }

        void finish()
        {
            this->player->end_contacts(this->scene.contacts());
            this->player->update_prev_rectangle();
            this->scene.end_tick();
            for (const auto& effect : this->player->movement_feedback().effects())
            {
                if (effect.age == 0.0f) { this->events.push_back({ effect, this->ticks }); }
            }
        }

        void tick(const PlayerInputData& actions = input())
        {
            this->begin(actions);
            this->scene.resolve();
            this->finish();
        }

        size_t count(MovementFeedbackKind kind) const
        {
            return static_cast<size_t>(std::count_if(this->events.begin(), this->events.end(),
                [kind](const Event& event) { return event.effect.kind == kind; }));
        }

        void walk(int count = 60, float direction = 1.0f)
        {
            for (int tick = 0; tick < count; ++tick) { this->tick(input(direction)); }
        }
    };

    void paint(World& world, StructurePaintable& surface, const RectangleF& box,
        PlayerTeam team = PlayerTeam::a)
    {
        const auto& tuning = world.tuning.projectile(ProjectileType::jet);
        const RectangleF supplied(box.x + tuning.details.col_rect_size.x / 2.0f,
            box.y + tuning.details.col_rect_size.y / 2.0f, box.width, box.height);
        ProjectileJet shot(supplied, Vector2F::ZERO, team, team == PlayerTeam::a ? 0 : 1,
            surface_colours.team_colour(team), &world.render, resolve_projectile_strip(tuning, world.render), &tuning);
        surface.on_contact(shot, Vector2F::ZERO, 0.0f);
        shot.on_contact(surface, Vector2F::ZERO, 0.0f);
    }

    const Course::Event& first_step(const Course& course)
    {
        const auto found = std::find_if(course.events.begin(), course.events.end(),
            [](const Course::Event& event) { return event.effect.kind == MovementFeedbackKind::step; });
        require(found != course.events.end(), "Walking fixture emitted no step");
        return *found;
    }

    void check_decisions(World& world)
    {
        const auto& sounds = world.tuning.player().movement_sounds;
        const int initial_volume = world.preferences.sfx_volume;
        world.preferences.sfx_volume = 0;
        MovementAudio cues(&sounds, world.audio.sound_bank("paint_exe"), 0);
        const auto request = [&](bool supported, bool grounded, float distance,
            float speed = 0.0f, float direction = 0.0f, float elapsed = dt)
        {
            return cues.end_contacts(true, supported, grounded, 0.0f, distance,
                speed, direction, elapsed, true);
        };
        auto result = request(true, true, sounds.stride_distance * 100.0f);
        require(!result.step && !result.skid, "Spawn settling requested contact particles");
        result = request(true, true, sounds.stride_distance * 0.6f);
        require(!result.step && !result.skid, "Short stride requested a step early");
        result = request(true, true, sounds.stride_distance * 0.5f, 0, 0, sounds.min_step_interval);
        require(result.step && !result.skid, "Completed stride did not request exactly a step");
        result = request(true, true, sounds.stride_distance * 100.0f, 0, 0, 0.001f);
        require(!result.step, "Contact particles bypassed the shared step cooldown");
        result = request(true, true, sounds.stride_distance * 100.0f, 0, 0, 100.0f);
        require(result.step, "Long tick lost its single step request");
        result = request(true, true, 0.1f, 0, 0, sounds.min_step_interval);
        require(!result.step, "Long tick retained a catch-up backlog of contact particles");
        result = request(true, true, 1.0f, sounds.skid_min_speed, -1.0f);
        require(result.skid, "Accepted braking edge did not request a skid");
        result = request(true, true, 1.0f, sounds.skid_min_speed, -1.0f, sounds.skid_repeat_delay);
        require(!result.skid, "Sustained braking repeatedly requested skids");
        request(true, true, 1.0f, sounds.skid_min_speed, 1.0f);
        result = request(true, true, 1.0f, sounds.skid_min_speed, -1.0f);
        require(result.skid, "Released and rearmed braking failed to request a skid");
        result = request(false, false, sounds.stride_distance * 100.0f, sounds.skid_min_speed, -1.0f);
        require(!result.step && !result.skid, "Airborne contact requested particles");
        result = request(true, false, sounds.stride_distance * 100.0f, sounds.skid_min_speed, -1.0f);
        require(!result.step && !result.skid, "Landing/takeoff contact requested a walked step or skid");
        for (const float elapsed : { 0.0f, -dt })
        {
            result = request(true, true, sounds.stride_distance * 100.0f,
                sounds.skid_min_speed, -1.0f, elapsed);
            require(!result.step && !result.skid, "Nonpositive contact tick requested particles");
        }
        world.preferences.sfx_volume = initial_volume;
        std::cout << "PASS movement contacts: shared muted stride/braking budget, cooldown, no backlog or nonwalking cues\n";
    }

    void check_scene_cadence(World& world)
    {
        Course course(world);
        course.surface({ -10000, 0, 20000, 100 });
        for (int tick = 0; tick < 120; ++tick) { course.tick(); }
        require(course.events.empty(), "Spawn or idle support emitted movement particles");
        course.walk(180);
        require(course.count(MovementFeedbackKind::step) > 3 &&
            course.count(MovementFeedbackKind::skid) == 0, "Actual walking lost steps or invented skids");
        int previous = -10000;
        for (const auto& event : course.events)
        {
            if (event.effect.kind != MovementFeedbackKind::step) { continue; }
            require((event.tick - previous) * dt + 0.0001f >=
                world.tuning.player().movement_sounds.min_step_interval,
                "Actual Player emitted contact particles faster than the stride cooldown");
            require(event.effect.direction > 0.0f && !event.effect.paint_colour &&
                std::abs(event.effect.position.y) < 0.001f,
                "Dry walking lost its movement direction or resolved support anchor");
            previous = event.tick;
        }
        course.tick(input(-1.0f));
        require(course.count(MovementFeedbackKind::skid) == 1,
            "Actual high-speed reversal did not emit one skid");
        const auto& skid = course.events.back().effect;
        require(skid.kind == MovementFeedbackKind::skid && skid.direction > 0.0f,
            "Skid followed requested input instead of the braking velocity");
        course.walk(90, -1.0f);
        require(course.count(MovementFeedbackKind::skid) == 1,
            "Holding reverse movement spammed skid particles");

        Course wall(world);
        wall.surface({ -1000, 0, 2000, 100 });
        wall.surface({ 80, -1000, 100, 1000 });
        wall.walk(120);
        const size_t stopped = wall.events.size();
        wall.walk(120);
        require(wall.events.size() == stopped, "Movement into a stationary wall spammed step/skid particles");
        Course air(world, nullptr, 0, -5000);
        air.walk(60);
        require(air.events.empty(), "Air control emitted grounded contact particles");
        course.tick(input(0, true));
        const size_t before_air = course.events.size();
        for (int tick = 0; tick < 10; ++tick) { course.tick(input(-1)); }
        require(course.events.size() == before_air,
            "Actual airborne movement emitted a contact particle");
        std::cout << "PASS movement contacts: actual Player cadence, braking direction, idle/air/wall suppression\n";
    }

    void check_tint(World& world)
    {
        for (const auto team : { PlayerTeam::none, PlayerTeam::a, PlayerTeam::b })
        {
            Course course(world);
            auto& floor = course.paintable();
            if (team != PlayerTeam::none) { paint(world, floor, floor.bounds(), team); }
            course.tick();
            course.walk();
            course.tick(input(-1));
            require(course.count(MovementFeedbackKind::step) > 0 &&
                course.count(MovementFeedbackKind::skid) == 1,
                "Painted support route missed a step or skid");
            const std::optional<Colour> expected = team == PlayerTeam::none ?
                std::nullopt : std::optional<Colour>(surface_colours.team_colour(team));
            for (const auto& event : course.events)
            {
                if (!contact_kind(event.effect.kind)) { continue; }
                require(event.effect.paint_colour == expected,
                    "Contact fleck tint did not use the actual neutral/friendly/enemy surface palette");
            }
            if (team == PlayerTeam::a)
            {
                const auto old = course.events.front().effect.paint_colour;
                const size_t before = course.events.size();
                paint(world, floor, floor.bounds(), PlayerTeam::b);
                course.walk(60, -1);
                require(course.events.size() > before && old == surface_colours.team_a,
                    "Repaint fixture emitted no later contact");
                for (size_t index = before; index < course.events.size(); ++index)
                {
                    require(course.events[index].effect.paint_colour == surface_colours.team_b,
                        "A later contact retained the former owner's tint after repaint");
                }
            }
        }

        // A side, ceiling, buried strip or unrelated lower surface is never
        // paint beneath the player's actual top support.
        for (int type = 0; type < 3; ++type)
        {
            Course course(world);
            const RectangleF obstacle(-1000, -100, 20000, 100);
            auto& floor = course.paintable({ -10000, type == 1 ? 100.0f : 0.0f, 20000, 100 },
                type == 0 ? PaintableFaces{ true, false, true, true } : PaintableFaces{ false, true, false, false },
                type == 2 ? std::span<const RectangleF>(&obstacle, 1) : std::span<const RectangleF>{});
            paint(world, floor, floor.bounds());
            if (type == 1) { course.surface({ -10000, 0, 20000, 100 }); }
            course.tick();
            course.walk();
            require(first_step(course).effect.paint_colour == std::nullopt,
                "Paint from a side, clipped strip or unrelated support tinted a contact fleck");
        }
        std::cout << "PASS movement contacts: neutral/friendly/enemy palette tint, repaint, side and clipped support\n";
    }

    void check_queries_and_seams(World& world)
    {
        Course query(world);
        const float pitch = world.tuning.paint().size.x;
        auto& floor = query.paintable({ 0, 0, pitch * 3, 100 });
        paint(world, floor, { 0.1f, 0.1f, 0.5f, 0.5f });
        paint(world, floor, { pitch + 0.1f, 0.1f, 0.5f, 0.5f }, PlayerTeam::b);
        require(floor.top_paint_colour(0) == surface_colours.team_a &&
            floor.top_paint_colour(pitch - 0.001f) == surface_colours.team_a &&
            floor.top_paint_colour(pitch) == surface_colours.team_b &&
            !floor.top_paint_colour(pitch * 2) && !floor.top_paint_colour(-0.001f) &&
            !floor.top_paint_colour(pitch * 3),
            "Top tint query lost exact half-open tile/surface boundaries");
        auto& underside = query.paintable({ 0, -400, 100, 100 }, { true, false, true, true });
        paint(world, underside, underside.bounds());
        require(!underside.top_paint_colour(50), "Painted underside/side became a painted top tint");

        Course baseline(world);
        baseline.surface({ -10000, 0, 20000, 100 });
        baseline.tick();
        baseline.walk();
        const float seam = first_step(baseline).effect.position.x + world.tuning.player().size.x / 4.0f;
        for (const bool wet_first : { false, true })
        {
            Course course(world);
            const auto add_wet = [&]
            {
                auto& wet = course.paintable({ seam, 0, 10000, 100 });
                paint(world, wet, wet.bounds());
            };
            const auto add_dry = [&] { course.surface({ -10000, 0, 10000 + seam, 100 }); };
            if (wet_first) { add_wet(); add_dry(); } else { add_dry(); add_wet(); }
            course.tick();
            course.walk();
            const auto& effect = first_step(course).effect;
            require(effect.paint_colour == surface_colours.team_a &&
                effect.position.x >= seam && effect.position.x < seam + 10000.0f &&
                std::abs(effect.position.y) < 0.001f,
                "Straddled dry/painted support tint depended on contact dispatch order");
        }

        // Arrange paint on the tick of the first stride, on either side of
        // collision dispatch. The completed contact phase owns the tint.
        const int event_tick = first_step(baseline).tick;
        for (const bool paint_first : { false, true })
        {
            Course course(world);
            auto& surface = course.paintable();
            course.tick();
            while (course.ticks < event_tick - 1) { course.tick(input(1)); }
            course.begin(input(1));
            if (paint_first) { paint(world, surface, surface.bounds(), PlayerTeam::b); }
            course.scene.resolve();
            if (!paint_first) { paint(world, surface, surface.bounds(), PlayerTeam::b); }
            course.finish();
            require(first_step(course).effect.paint_colour == surface_colours.team_b,
                "Contact tint was sampled before all paint callbacks completed");
        }
        std::cout << "PASS movement contacts: exact tint lookup, dry/wet seam order and same-tick paint callbacks\n";
    }

    void check_lifecycle(World& world)
    {
        const auto& look = world.presentation.movement_feedback();
        for (const auto kind : { MovementFeedbackKind::step, MovementFeedbackKind::skid })
        {
            MovementFeedback effect(&look, &world.render);
            effect.contact(kind, { 1000, 500 }, -1, surface_colours.team_b);
            effect.update(0);
            effect.update(-dt);
            const auto frozen = effect.effects().front();
            require(frozen.age == 0 && frozen.position == Vector2F(1000, 500) &&
                frozen.direction < 0 && frozen.paint_colour == surface_colours.team_b,
                "Frozen contact snapshot rewound, moved or lost its surface tint");
            const RectangleF player_bounds(-26, -120, 52, 120);
            const RectangleF expanded = effect.bounds(player_bounds);
            require(expanded.contains(player_bounds) &&
                expanded.contains(RectangleF(999, 499, 2, 2)),
                "Contact culling bounds omitted the old world-space footfall");
            effect.update(std::max(look.step.duration, look.skid.duration) + dt);
            require(effect.effects().empty() && effect.bounds(player_bounds) == player_bounds,
                "Expired contact particles retained their lifetime or stale culling bounds");
        }
        Course course(world);
        course.surface({ -10000, 0, 20000, 100 });
        course.tick();
        while (course.count(MovementFeedbackKind::step) == 0 && course.ticks < 120)
        {
            course.tick(input(1));
        }
        require(!course.player->movement_feedback().effects().empty(), "Lifecycle route missed a live step");
        const auto snapshot = course.player->movement_feedback().effects().back();
        course.player->reset_jump_input();
        const auto paused = course.player->movement_feedback().effects().back();
        require(paused.age == snapshot.age && paused.position == snapshot.position &&
            paused.paint_colour == snapshot.paint_colour && paused.direction == snapshot.direction,
            "Pause input reset cleared, moved or aged a contact effect");
        course.tick();
        require(course.player->movement_feedback().effects().back().age > snapshot.age &&
            course.player->movement_feedback().effects().back().position == snapshot.position,
            "Resumed contact effect failed to age at its frozen world anchor");
        course.player->place_at({ 0, -world.tuning.player().size.y });
        require(course.player->movement_feedback().effects().empty(), "Placement retained stale contact effects");
        course.events.clear();
        course.tick();
        require(course.events.empty(), "Placement settling emitted contact effects");
        course.walk();
        require(course.count(MovementFeedbackKind::step) > 0, "Placement failed to rearm walking effects");
        const ProjectileSpray shot(RectangleF(0, 0, 10, 10), Vector2F::ZERO,
            PlayerTeam::b, 1, Colour::white, &world.render,
            resolve_projectile_strip(world.tuning.projectile(ProjectileType::spray), world.render),
            &world.tuning.projectile(ProjectileType::spray));
        for (int hit = 0; hit < 10000 && course.player->health() > 0; ++hit)
        {
            course.player->on_contact(shot, Vector2F::ZERO, 0);
        }
        course.player->end_contacts();
        require(course.player->health() <= 0 && course.player->movement_feedback().effects().empty(),
            "Fatal contact retained a live contact particle until next update");
        course.events.clear();
        for (int tick = 0; tick < 360; ++tick) { course.tick(); }
        require(course.player->state() == PlayerState::alive && course.events.empty(),
            "Death/respawn settling emitted contact particles");
        course.walk();
        require(course.count(MovementFeedbackKind::step) > 0, "Respawn failed to rearm contact particles");
        std::cout << "PASS movement contacts: pause/resume world anchors, placement, fatal cleanup and respawn rearming\n";
    }

    void check_equivalence(World& world)
    {
        MovementFeedbackPresentation disabled = world.presentation.movement_feedback();
        disabled.enabled = false;
        Course visible(world);
        Course quiet(world, &disabled);
        auto& a_floor = visible.paintable();
        auto& b_floor = quiet.paintable();
        paint(world, a_floor, a_floor.bounds());
        paint(world, b_floor, b_floor.bounds());
        for (int tick = 0; tick < 360; ++tick)
        {
            if (tick == 100)
            {
                paint(world, a_floor, a_floor.bounds(), PlayerTeam::b);
                paint(world, b_floor, b_floor.bounds(), PlayerTeam::b);
            }
            if (tick == 180) { visible.player->kill(); quiet.player->kill(); }
            if (tick == 210) { visible.player->force_respawn(); quiet.player->force_respawn(); }
            const auto actions = input(tick < 80 || tick >= 240 ? 1.0f : -1.0f,
                tick == 130, tick % 60 < 24);
            visible.tick(actions);
            quiet.tick(actions);
            const Player& a = *visible.player;
            const Player& b = *quiet.player;
            const PaintTotal a_score = a_floor.paint_total();
            const PaintTotal b_score = b_floor.paint_total();
            require(a.rectangle() == b.rectangle() && a.velocity() == b.velocity() &&
                a.move_state() == b.move_state() && a.state() == b.state() &&
                a.health() == b.health() && a.weapon_ammo() == b.weapon_ammo() &&
                a.respawn_timer() == b.respawn_timer() &&
                visible.emitted_projectiles == quiet.emitted_projectiles &&
                a_score.team_a == b_score.team_a && a_score.team_b == b_score.team_b &&
                a_score.total == b_score.total && a_score.player_areas == b_score.player_areas,
                "Enabled contact effects changed movement, health, ammo, paid shots, respawn or paint score");
            require(b.movement_feedback().effects().empty(), "Disabled movement presentation emitted contact effects");
        }
        require(visible.count(MovementFeedbackKind::step) > 0 &&
            visible.count(MovementFeedbackKind::skid) > 0 && visible.emitted_projectiles > 0,
            "Equivalence route omitted contact effects or paid shots");
        std::cout << "PASS movement contacts: enabled/disabled equivalence through paid shots, repaint, death and respawn\n";
    }

    void check_content(const std::filesystem::path& content)
    {
        std::ifstream stream(content / "presentation.json");
        require(stream.good(), "Cannot read movement contact presentation");
        const std::string source(std::istreambuf_iterator<char>(stream), {});
        wchar_t name[MAX_PATH]{};
        const auto temporary = std::filesystem::temp_directory_path();
        require(GetTempFileNameW(temporary.c_str(), L"cwc", 0, name) != 0,
            "Cannot reserve movement contact presentation variant");
        struct Cleanup
        {
            std::filesystem::path path;
            ~Cleanup() { std::error_code ignored; std::filesystem::remove(this->path, ignored); }
        } cleanup{ name };
        const auto reject = [&](auto edit)
        {
            rapidjson::Document json;
            json.Parse(source.c_str());
            require(!json.HasParseError(), "Cannot parse movement contact presentation fixture");
            edit(json["movement_feedback"], json.GetAllocator());
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            json.Accept(writer);
            std::ofstream variant(cleanup.path);
            variant.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
            variant.close();
            require(variant.good(), "Cannot write movement contact presentation variant");
            bool rejected = false;
            try { const Presentation invalid(cleanup.path.string().c_str()); }
            catch (const std::exception& error)
            {
                rejected = std::string(error.what()).find("movement_feedback") != std::string::npos;
            }
            require(rejected, "Invalid contact presentation was accepted or lost its authored section");
        };
        for (const char* burst : { "step", "skid" })
        {
            for (const char* field : { "duration", "spread", "lift", "particle_size" })
            {
                reject([=](auto& look, auto&) { look[burst][field].SetFloat(-1); });
                reject([=](auto& look, auto&) { look[burst][field].SetString("large"); });
                reject([=](auto& look, auto&) { look[burst][field].SetDouble(1e40); });
            }
            reject([=](auto& look, auto&) { look[burst]["duration"].SetFloat(0); });
            reject([=](auto& look, auto&) { look[burst]["particle_size"].SetFloat(1); });
            reject([=](auto& look, auto&) { look.RemoveMember(burst); });
        }
        reject([](auto& look, auto&) { look["keyline_width"].SetFloat(0); });
        reject([](auto& look, auto&) { look["keyline_width"].SetFloat(10000); });
        reject([](auto& look, auto& allocator) { look["keyline"].SetString("not-a-colour", allocator); });
        std::cout << "PASS movement contacts: required bursts, finite numeric geometry, readable particles and keyline content\n";
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("movement contact", [&]
    {
        require(argc == 2, "Usage: movement_contact.exe <content directory>");
        const std::filesystem::path content(argv[1]);
        World world(content);
        check_decisions(world);
        check_scene_cadence(world);
        check_tint(world);
        check_queries_and_seams(world);
        check_lifecycle(world);
        check_equivalence(world);
        check_content(content);
        return 0;
    });
}
