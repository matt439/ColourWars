// Actual Level integration: painting is a role, while one-way collision keeps
// its existing jump/drop and projectile-blocking semantics.
#include "game/input_slots.h"
#include "game/objects/level.h"
#include "game/objects/player_builder.h"
#include "engine/core/step_timer.h"
#include "engine/collision/partitioner.h"
#include "engine/render/sprite_font_file.h"

namespace
{
    void check_paintable_jump_through(const std::filesystem::path& content)
    {
        RenderResources render;
        for (const char* sheet : { "sprite_sheet_1", "painter", "menu_paint", "weapon_paint", "paint_fx", "terrain" })
        {
            render.add_sprite_sheet(sheet, read_sprite_sheet(
                (content / "textures" / (std::string(sheet) + ".json")).string().c_str(), TextureHandle{}));
        }
        for (const char* font : { "gill_sans_ultra_bold_144", "gill_sans_mt_bold_24",
            "gill_sans_mt_bold_36", "gill_sans_mt_bold_48", "gill_sans_mt_bold_72",
            "gill_sans_mt_bold_144", "courier_new_bold_16" })
        {
            const SpriteFontFile file = read_sprite_font_file(
                (content / "fonts" / (std::string(font) + ".spritefont")).string());
            auto loaded = std::make_unique<Font>(TextureHandle{}, file.glyphs, file.line_spacing);
            if (loaded->contains(U'?')) { loaded->set_stand_in(U'?'); }
            render.add_font(font, std::move(loaded));
        }
        AudioResources audio;
        audio.add_sound_bank("paint_exe", SoundBank::silent());
        const PlaySettings preferences;
        const GameTuning tuning((content / "tuning.json").string().c_str());
        const Presentation presentation((content / "presentation.json").string().c_str());
        ResolutionManager resolution;
        ViewportManager viewports(&resolution);
        Mouse mouse;
        Partitioner partitioner;
        const LevelLoadedInfo info((content / "levels/crossfeed.json").string().c_str());
        const TeamColour colours;
        const LevelObjectBuilder objects(&render, &audio, &preferences, &tuning.paint());
        auto scene = std::make_unique<Scene>(nullptr, &partitioner);
        scene->set_bounds(info.out_of_bounds_rectangle());
        Scene* observed_scene = scene.get();
        std::vector<StructurePaintable*> paintables;
        StructurePaintable* floor = nullptr;
        StructurePaintable* roof = nullptr;
        const JsonValue entries = info.collision_objects_json();
        auto built = objects.build_collision_objects(entries, colours);
        for (size_t index = 0; index < built.size(); ++index)
        {
            if (auto* paintable = dynamic_cast<StructurePaintable*>(built[index].get()))
            {
                paintables.push_back(paintable);
                const std::string name = entries.at(index).string("name");
                if (name == "spawn_a_floor") { floor = paintable; }
                if (name == "spawn_a_roof") { roof = paintable; }
            }
            scene->add(std::move(built[index]));
        }
        require(floor != nullptr && roof != nullptr,
            "Crossfeed home supports must implement the paintable role");
        const RectangleF floor_bounds = floor->shape()->bounding_box();
        const RectangleF roof_bounds = roof->shape()->bounding_box();
        require(floor->tag() == to_collision_tag(CollisionObjectType::structure_jump_through) &&
            roof->tag() == floor->tag() && floor_bounds.height == 10.0f && roof_bounds.height == 10.0f,
            "Paintable rulers changed their ten-unit one-way collision classification");
        area_is(floor->paint_total().total, floor_bounds.width * tuning.paint().thickness);
        MenuLevelSettings settings;
        settings.set_player_count(1);
        settings.set_screen_layout(ScreenLayout::one_player);
        settings.set_stage("crossfeed");
        settings.begin_input_claims(3);
        settings.set_player_team(0, PlayerTeam::a);
        settings.set_player_weapon(0, WeaponType::sprayer);
        viewports.set_layout(settings.screen_layout());
        const PlayerBuilder players(&tuning, &presentation);
        std::vector<Player*> roster;
        for (auto& player : players.build_players(settings, &info, colours, &render,
            &audio, &preferences, &viewports))
        { roster.push_back(scene->add(std::move(player))); }
        Level level(std::move(scene), {}, std::move(roster), objects.build_viewport_dividers(&viewports),
            colours, info.camera_bounds_rectangle(), info.zoom_out_start_bounds_rectangle(),
            info.zoom_out_finish_bounds_rectangle(), &tuning, &presentation, L"One-way paint check",
            info.sound_bank_name(), info.music_name(), info.music_volume(), info.end_sting_name(),
            info.end_sting_volume(), &resolution, &viewports, &render, &audio, &preferences,
            &mouse, info.stations(), OpeningMode::off, 300.0f);
        const float step = static_cast<float>(StepTimer::TicksToSeconds(
            StepTimer::SecondsToTicks(1.0 / 60.0)));
        std::vector<PlayerInputData> inputs(input_slots::count);
        inputs[3].connected = true;
        const auto tick = [&] { level.update(inputs, step); };
        for (int ticks = 0; ticks < 1000 && !level.match_started(); ++ticks) { tick(); }
        require(level.match_started(), "One-way paint check did not finish its countdown");
        for (int ticks = 0; ticks < 30; ++ticks) { tick(); }
        require(level.player(0).move_state() == PlayerMoveState::on_drop_down_ground &&
            std::fabs(level.player(0).rectangle().bottom() - floor_bounds.top()) < 0.1f,
            "One-way paint check did not settle on the authored home floor");
        float expected_total = 0.0f;
        for (const StructurePaintable* surface : paintables) { expected_total += surface->paint_total().total; }
        area_is(level.level_end_info().total_area, expected_total);
        area_is(level.level_end_info().team_a_area, 0.0f);
        inputs[3].shoot_direction_requested = true;
        inputs[3].shoot_angle = PI / 2.0f;
        inputs[3].shoot_direction = Vector2F::DIRECTION_DOWN;
        inputs[3].primary_shoot = true;
        int paid = 0;
        for (int ticks = 0; ticks < 60; ++ticks)
        {
            const float before = level.player(0).weapon_ammo();
            tick();
            paid += level.player(0).weapon_ammo() < before ? 1 : 0;
            for (const CollisionObject* object : observed_scene->collision_objects())
            {
                if (!is_projectile(to_collision_type(object->tag())) || object->for_deletion()) { continue; }
                require(object->shape()->bounding_box().center().y <= floor_bounds.bottom(),
                    "A downward paid projectile passed through the paintable one-way support");
            }
        }
        require(paid > 0 && floor->paint_total().team_a > 0.0f,
            "Real paid fire did not paint the one-way support");
        float expected_owned = 0.0f;
        for (const StructurePaintable* surface : paintables) { expected_owned += surface->paint_total().team_a; }
        const LevelEndInfo painted = level.level_end_info();
        area_is(painted.total_area, expected_total);
        area_is(painted.team_a_area, expected_owned);
        area_is(painted.player_areas[3], expected_owned);
        inputs[3].primary_shoot = false;
        inputs[3].jump_pressed = true;
        inputs[3].jump_held = true;
        bool above_roof = false;
        bool landed_on_roof = false;
        for (int ticks = 0; ticks < 120; ++ticks)
        {
            tick();
            inputs[3].jump_pressed = false;
            if (ticks >= 15) { inputs[3].jump_held = false; }
            above_roof = above_roof || level.player(0).rectangle().bottom() < roof_bounds.top();
            landed_on_roof = above_roof && level.player(0).move_state() == PlayerMoveState::on_drop_down_ground &&
                std::fabs(level.player(0).rectangle().bottom() - roof_bounds.top()) < 0.1f;
            if (landed_on_roof) { break; }
        }
        require(above_roof && landed_on_roof,
            "Paintable one-way roof blocked upward passage or lost its landing");
        inputs[3].jump_held = false;
        inputs[3].drop_through = true;
        bool below_roof = false;
        bool below_floor = false;
        for (int ticks = 0; ticks < 120; ++ticks)
        {
            tick();
            below_roof = below_roof || level.player(0).rectangle().top() > roof_bounds.bottom();
            below_floor = below_floor || level.player(0).rectangle().top() > floor_bounds.bottom();
        }
        require(below_roof && below_floor && level.player(0).move_state() == PlayerMoveState::on_ground,
            "Down failed to drop through the painted roof/home floor to solid ground");
        require(floor->shape()->bounding_box() == floor_bounds && roof->shape()->bounding_box() == roof_bounds,
            "Painting or traversal changed one-way collision bounds");
        std::cout << "PASS paintable one-way Level: neutral/owned/player-credit score, actual paid paint, projectile blocking, "
            "upward passage/landing and Down through roof/floor; exact ten-unit collision bounds, no devices or save\n";
    }
}
