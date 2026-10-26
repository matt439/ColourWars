#include "game/pch.h"
#include "game/objects/fixed_slab_paintable.h"
#include "game/objects/player.h"
#include "game/objects/projectile_jet.h"
#include "game/objects/weapon_builder.h"
#include "game/presentation.h"
#include "engine/assets/json.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/file_path.h"
#include "engine/core/step_timer.h"
#include "engine/scene/scene.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace fixed_slab_acceptance
{
    namespace
    {
        using namespace labrador;
        using namespace mattmath;
        const float dt = static_cast<float>(StepTimer::TicksToSeconds(StepTimer::SecondsToTicks(1.0 / 60.0)));
        const TeamColour colours{ Colour::orange, Colour::cyan };

        void require(bool value, const char* message)
        { if (!value) { throw std::runtime_error(message); } }

        void close_value(float actual, float expected, const char* message)
        { require(std::isfinite(actual) && std::abs(actual - expected) <= 0.002f, message); }

        PlayerInputData input(float movement = 0.0f, bool jump = false)
        {
            PlayerInputData result;
            result.connected = true; result.x_movement = movement;
            result.jump_pressed = jump; result.jump_held = jump;
            result.shoot_direction_requested = true; result.shoot_direction = { 1.0f, 0.0f };
            return result;
        }

        bool on_slope(const Player& player)
        {
            return player.move_state() == PlayerMoveState::on_ramp_left ||
                player.move_state() == PlayerMoveState::on_ramp_right;
        }

        struct World
        {
            RenderResources render;
            AudioResources audio;
            PlaySettings preferences;
            GameTuning tuning;
            Presentation presentation;
            std::string tuning_source;

            explicit World(const std::filesystem::path& content) :
                tuning(path_to_utf8(content / "tuning.json").c_str()),
                presentation(path_to_utf8(content / "presentation.json").c_str())
            {
                for (const char* sheet : { "sprite_sheet_1", "painter", "weapon_paint", "paint_fx", "terrain" })
                {
                    render.add_sprite_sheet(sheet, read_sprite_sheet(path_to_utf8(content / "textures" /
                        (std::string(sheet) + ".json")).c_str(), TextureHandle{}));
                }
                audio.add_sound_bank("paint_exe", SoundBank::silent());
                std::ifstream source(content / "tuning.json", std::ios::binary);
                require(source.good(), "Cannot read fixed-slab support tuning");
                tuning_source.assign(std::istreambuf_iterator<char>(source), {});
            }

            GameTuning underfoot_tuning() const
            {
                rapidjson::Document document; document.Parse(tuning_source.c_str());
                require(!document.HasParseError(), "Cannot parse fixed-slab support tuning");
                document["player"]["paint_underfoot"]["enabled"].SetBool(true);
                rapidjson::StringBuffer buffer; rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                require(document.Accept(writer), "Cannot encode fixed-slab support tuning");
                return GameTuning(parse_json(buffer.GetString(), "fixed-slab-underfoot-fixture.json"));
            }
        };

        FixedSlabSpec spec(bool mirrored)
        { return { mirrored ? Vector2F(2600.0f, 800.0f) : Vector2F(1240.0f, 880.0f),
            mirrored ? FixedSlabRise::left : FixedSlabRise::right }; }

        RectangleF mirror(const RectangleF& rectangle)
        { return { 4000.0f - rectangle.right(), rectangle.y, rectangle.width, rectangle.height }; }

        // Independent authored top equations. These do not reconstruct the
        // shape's bounding box, which includes the normal extrusion.
        Vector2F support_point(bool mirrored, const RectangleF& body)
        {
            const float x = mirrored ? std::clamp(body.left(), 2600.0f, 2760.0f) :
                std::clamp(body.right(), 1240.0f, 1400.0f);
            return { x, mirrored ? 800.0f + (x - 2600.0f) * 0.5f : 880.0f - (x - 1240.0f) * 0.5f };
        }

        struct Course
        {
            World& world;
            const GameTuning& tuning;
            Scene scene{ nullptr, nullptr };
            Player* player;
            FixedSlabPaintable* slab = nullptr;
            bool mirrored;
            int ticks = 0;
            int slope_ticks = 0;
            int ceiling_ticks = 0;

            Course(World& resources, bool reflected, float center_x, float feet,
                const std::vector<RectangleF>& flats, bool slab_first = true, const GameTuning* variant = nullptr) :
                world(resources), tuning(variant ? *variant : resources.tuning), mirrored(reflected)
            {
                const RectangleF rectangle(center_x - tuning.player().size.x / 2.0f,
                    feet - tuning.player().size.y, tuning.player().size.x, tuning.player().size.y);
                auto weapon = WeaponBuilder::build_weapon(&tuning, PlayerTeam::a, 0, 0, colours.team_a,
                    WeaponType::sprayer, rectangle.center(), &world.render, &world.audio, &world.preferences);
                player = scene.add(std::make_unique<Player>(rectangle, &tuning,
                    &world.presentation.movement_feedback(), &world.presentation.combat_feedback(),
                    &world.presentation.respawn_redraw(), &world.render, &world.audio, &world.preferences,
                    0, PlayerTeam::a, WeaponType::sprayer, std::move(weapon), WeaponType::none,
                    colours.team_a, Viewport{}, rectangle.top_left()));
                const auto add_slab = [&]
                {
                    slab = scene.add(std::make_unique<FixedSlabPaintable>(spec(mirrored), &world.render,
                        &world.audio, &world.preferences, &tuning.paint(), colours,
                        PaintableFaces{ true, true, true, true }, flats));
                };
                if (slab_first) { add_slab(); }
                for (const auto& flat : flats)
                {
                    scene.add(std::make_unique<Structure>("sprite_sheet_1", "square_white_4", flat,
                        std::make_unique<RectangleF>(flat), &world.render, CollisionObjectType::structure));
                }
                if (!slab_first) { add_slab(); }
                scene.end_tick();
            }

            void tick(const PlayerInputData& action = input())
            {
                require(ticks < 1600, "Fixed-slab support course exceeded its bounded update budget");
                ++ticks; player->set_player_input(action); scene.update(dt);
                require(player->update_weapon_and_get_projectiles(dt, scene.collision_objects()).empty(),
                    "Neutral fixed-slab support input unexpectedly fired");
                scene.resolve(); player->end_contacts(scene.contacts());
                player->update_prev_rectangle(); scene.end_tick();
                require(player->state() == PlayerState::alive && player->health() == tuning.player().starting_health,
                    "Fixed-slab support trial lost its live neutral player");
                require(std::isfinite(player->rectangle().x) && std::isfinite(player->rectangle().y),
                    "Fixed-slab support trial produced a nonfinite pose");
                if (on_slope(*player))
                {
                    ++slope_ticks;
                    close_value(player->rectangle().bottom(), support_point(mirrored, player->rectangle()).y,
                        "Fixed slab grounded feet did not follow the actual top segment");
                }
                if (player->move_state() == PlayerMoveState::on_ceiling) { ++ceiling_ticks; }
            }

            void neutral(int count)
            { for (int index = 0; index < count; ++index) { tick(); } }
        };

        std::vector<RectangleF> joined_flats(bool mirrored)
        {
            std::vector<RectangleF> result{ { 800.0f, 880.0f, 440.0f, 40.0f },
                { 1400.0f, 800.0f, 540.0f, 80.0f } };
            if (mirrored) { for (auto& flat : result) { flat = mirror(flat); } }
            return result;
        }

        void require_route_support(const Course& course)
        {
            const float corner = course.mirrored ? course.player->rectangle().left() :
                course.player->rectangle().right();
            const float low = course.mirrored ? 2600.0f : 1240.0f;
            const float high = low + 160.0f;
            if (corner > low + 0.002f && corner < high - 0.002f)
            {
                require(on_slope(*course.player), "Walking slab route lost support between its toe and crest");
                close_value(course.player->velocity().y, 0.0f, "Supported slab walk retained fall velocity");
            }
        }

        int check_route(World& world, bool mirrored, bool slab_first)
        {
            const float sign = mirrored ? -1.0f : 1.0f;
            Course course(world, mirrored, mirrored ? 2840.0f : 1160.0f, 880.0f,
                joined_flats(mirrored), slab_first);
            course.neutral(30);
            require(course.player->move_state() == PlayerMoveState::on_ground, "Route did not start on real lower flat");
            int count = 0;
            while (sign * (course.player->center().x - (mirrored ? 2460.0f : 1540.0f)) < 0.0f && count++ < 180)
            { course.tick(input(sign)); require_route_support(course); }
            require(count <= 180 && course.slope_ticks > 0, "Uphill route failed to cross the fixed slab through input");
            course.neutral(40);
            require(course.player->move_state() == PlayerMoveState::on_ground,
                "Uphill route failed to settle on the actual upper flat");
            close_value(course.player->rectangle().bottom(), 800.0f, "Uphill route ended on wrong support plane");
            close_value(course.player->velocity().x, 0.0f, "Uphill route did not stop through release braking");
            const int uphill_slope_ticks = course.slope_ticks;
            count = 0;
            while (sign * (course.player->center().x - (mirrored ? 2900.0f : 1100.0f)) > 0.0f && count++ < 240)
            { course.tick(input(-sign)); require_route_support(course); }
            require(count <= 240 && course.slope_ticks > uphill_slope_ticks,
                "Downhill route failed to retain the crest and descend through input");
            course.neutral(40);
            require(course.player->move_state() == PlayerMoveState::on_ground,
                "Downhill route failed to settle on the actual lower flat");
            close_value(course.player->rectangle().bottom(), 880.0f, "Downhill route ended on wrong support plane");
            close_value(course.player->velocity().x, 0.0f, "Downhill route did not stop through release braking");
            return course.ticks;
        }

        int check_rest_jump(World& world, bool mirrored)
        {
            Course course(world, mirrored, mirrored ? 2680.0f : 1320.0f, 827.0f, joined_flats(mirrored));
            course.neutral(60);
            require(on_slope(*course.player), "Mid-slope neutral actor did not retain exact top support");
            close_value(course.player->center().x, mirrored ? 2680.0f : 1320.0f, "Idle slab support slid the player sideways");
            const float old_feet = course.player->rectangle().bottom();
            course.tick(input(0.0f, true));
            require(course.player->move_state() == PlayerMoveState::jumping && course.player->rectangle().bottom() < old_feet,
                "Jump did not release the established fixed-slab support");
            bool airborne = true;
            for (int count = 0; count < 240; ++count)
            {
                course.tick();
                if (on_slope(*course.player)) { airborne = false; break; }
            }
            require(!airborne, "Fixed-slab jump failed to re-land within the bounded observation");
            course.neutral(30);
            close_value(course.player->rectangle().bottom(), old_feet, "Fixed-slab jump did not recover the same local support");
            return course.ticks;
        }

        int check_caps(World& world, bool mirrored, bool high_cap)
        {
            const RectangleF authored_floor = high_cap ? RectangleF(1412.0f, 925.0f, 488.0f, 30.0f) :
                RectangleF(800.0f, 1010.0f, 430.0f, 30.0f);
            const float authored_x = high_cap ? 1540.0f : 1120.0f;
            const float direction = (high_cap ? -1.0f : 1.0f) * (mirrored ? -1.0f : 1.0f);
            Course course(world, mirrored, mirrored ? 4000.0f - authored_x : authored_x,
                authored_floor.top(), { mirrored ? mirror(authored_floor) : authored_floor });
            course.neutral(30);
            for (int count = 0; count < 150; ++count) { course.tick(input(direction)); }
            require(course.slope_ticks == 0, "A solid slab end cap was mistaken for top support");
            const float canonical_x = mirrored ? 4000.0f - course.player->center().x : course.player->center().x;
            require(high_cap ? canonical_x > 1400.0f : canonical_x < 1240.0f,
                "Player crossed a solid fixed-slab end cap");
            close_value(course.player->velocity().x, 0.0f, "Solid slab cap did not stop inward personal velocity");
            return course.ticks;
        }

        int check_underside(World& world, bool mirrored)
        {
            // This isolated floor is close enough for the unchanged held
            // jump to reach the underside. It is not the arena's lower bed.
            Course course(world, mirrored, mirrored ? 2680.0f : 1320.0f, 1120.0f,
                { RectangleF(800.0f, 1120.0f, 2400.0f, 40.0f) });
            course.neutral(30);
            course.tick(input(0.0f, true));
            PlayerInputData held = input(); held.jump_held = true;
            for (int count = 0; count < 12; ++count) { course.tick(held); }
            for (int count = 0; count < 150; ++count) { course.tick(); }
            require(course.ceiling_ticks > 0 && course.slope_ticks == 0,
                "Jump from below did not meet a solid underside or invented top support");
            close_value(course.player->rectangle().bottom(), 1120.0f, "Underside jump did not return to its actual floor");
            return course.ticks;
        }

        int check_low_ceiling(World& world, bool mirrored, bool slab_first)
        {
            auto flats = joined_flats(mirrored);
            const RectangleF authored_roof(1240.0f, 650.0f, 300.0f, 70.0f);
            const RectangleF roof = mirrored ? mirror(authored_roof) : authored_roof;
            flats.push_back(roof);
            const float sign = mirrored ? -1.0f : 1.0f;
            Course course(world, mirrored, mirrored ? 2840.0f : 1160.0f, 880.0f, flats, slab_first);
            course.neutral(30);
            const auto outside_roof = [&]
            {
                const RectangleF body = course.player->rectangle();
                if (body.right() > roof.left() && body.left() < roof.right())
                {
                    require(body.top() >= roof.bottom() - 0.002f,
                        "Blocked uphill slab step penetrated the low ceiling");
                }
            };
            std::optional<RectangleF> stopped;
            int held_blocked_updates = 0;
            for (int count = 0; count < 180; ++count)
            {
                course.tick(input(sign)); outside_roof(); require_route_support(course);
                const float canonical_x = mirrored ? 4000.0f - course.player->center().x : course.player->center().x;
                if (!stopped && canonical_x > 1270.0f && on_slope(*course.player) &&
                    std::abs(course.player->velocity().x) < 0.001f)
                {
                    require(canonical_x < 1320.0f, "Low-ceiling fixture missed its intended slab station");
                    stopped = course.player->rectangle();
                }
                if (stopped)
                {
                    ++held_blocked_updates;
                    require(course.player->rectangle().x == stopped->x && course.player->rectangle().y == stopped->y,
                        "Continued blocked uphill input crept or jittered away from its restored supported pose");
                    require(on_slope(*course.player), "Low-ceiling rollback lost actual slab support");
                    close_value(course.player->velocity().x, 0.0f, "Blocked uphill input retained horizontal velocity");
                    close_value(course.player->velocity().y, 0.0f, "Blocked uphill input retained vertical velocity");
                }
            }
            require(stopped && held_blocked_updates >= 60,
                "Low-ceiling fixture failed to exercise repeated supported uphill rejection");
            for (int count = 0; count < 60; ++count)
            {
                course.tick(); outside_roof();
                require(on_slope(*course.player) && course.player->rectangle().x == stopped->x &&
                    course.player->rectangle().y == stopped->y,
                    "Neutral input lost the restored low-ceiling slab stop");
            }
            int recovery = 0;
            while (sign * (course.player->center().x - (mirrored ? 2860.0f : 1140.0f)) > 0.0f && recovery++ < 180)
            {
                course.tick(input(-sign)); outside_roof(); require_route_support(course);
            }
            require(recovery <= 180, "Low-ceiling slab stop failed to recover through ordinary downhill input");
            course.neutral(40); outside_roof();
            require(course.player->move_state() == PlayerMoveState::on_ground,
                "Low-ceiling reverse recovery failed to stop on the actual lower flat");
            close_value(course.player->rectangle().bottom(), 880.0f, "Low-ceiling recovery ended on the wrong floor");
            close_value(course.player->velocity().x, 0.0f, "Low-ceiling recovery did not finish release braking");
            return course.ticks;
        }

        int check_underfoot(World& world, bool mirrored)
        {
            const GameTuning tuning = world.underfoot_tuning();
            Course course(world, mirrored, mirrored ? 2680.0f : 1320.0f, 827.0f,
                joined_flats(mirrored), true, &tuning);
            course.neutral(30);
            require(on_slope(*course.player), "Underfoot control did not settle on the slab");
            const Vector2F point = support_point(mirrored, course.player->rectangle());
            const RectangleF contact(point.x - 2.0f, point.y - 2.0f, 4.0f, 4.0f);
            const auto& shot_tuning = tuning.projectile(ProjectileType::jet);
            const RectangleF supplied(contact.x + shot_tuning.details.col_rect_size.x / 2.0f,
                contact.y + shot_tuning.details.col_rect_size.y / 2.0f, contact.width, contact.height);
            ProjectileJet shot(supplied, Vector2F::ZERO, PlayerTeam::a, 0, colours.team_a,
                &world.render, resolve_projectile_strip(shot_tuning, world.render), &shot_tuning);
            course.slab->on_contact(shot, Vector2F::ZERO, 0.0f);
            require(course.slab->top_paint_team(point.x) == PlayerTeam::a &&
                course.slab->top_paint_team(course.player->center().x) == PlayerTeam::none,
                "Underfoot fixture did not distinguish the true high corner from the floating foot centre");
            course.neutral(2);
            const PlayerPaintUnderfootState state = course.player->paint_underfoot_state();
            require(state.enabled && state.support_team == PlayerTeam::a && state.step_team == PlayerTeam::a,
                "Projected slab support lost actual top paint outside the penetrating contact phase");
            return course.ticks;
        }
    }

    void support_checks(const std::filesystem::path& content)
    {
        World world(content);
        int updates = 0;
        for (bool mirrored : { false, true })
        {
            for (bool slab_first : { false, true }) { updates += check_route(world, mirrored, slab_first); }
            updates += check_rest_jump(world, mirrored);
            for (bool high : { false, true }) { updates += check_caps(world, mirrored, high); }
            updates += check_underside(world, mirrored);
            for (bool slab_first : { false, true }) { updates += check_low_ceiling(world, mirrored, slab_first); }
            updates += check_underfoot(world, mirrored);
        }
        std::cout << "PASS fixed-slab Player support: 18 actual Scene courses, " << updates
            << " updates; mirrored driven joins, rest/jump, solid caps/undersides, repeated low-ceiling uphill stops, "
            "neutral holds/downhill recovery and true-corner underfoot. "
            << "Public builder/editor/native and human acceptance remain separate\n";
    }
}
