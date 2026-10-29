#include "game/pch.h"
#include "scripts/acceptance/moving_structure/loader_fixture.h"

namespace moving_structure_acceptance
{
    void write_loaded_stage(const std::filesystem::path& path, bool jump_through, bool blocked)
    {
        require(!std::filesystem::exists(path), "Refusing to overwrite an earlier public-loader stage receipt");
        rapidjson::Document stage;
        stage.Parse(R"json({
            "format":1,"display_name":"Public moving fixture","icon_frame":"square_white_4",
            "include_in_random":false,"sound":{"sound_bank_name":"paint_exe"},
            "spawn_locations":{
                "team_a":[{"x":1194,"y":680},{"x":100,"y":1080},{"x":250,"y":1080},{"x":400,"y":1080}],
                "team_b":[{"x":3000,"y":1080},{"x":3150,"y":1080},{"x":3300,"y":1080},{"x":3450,"y":1080}]},
            "level_bounds":{
                "out_of_bounds":{"x":0,"y":0,"width":4000,"height":1600},
                "camera_bounds":{"x":0,"y":0,"width":4000,"height":1600},
                "zoom_out_start_bounds":{"x":0,"y":0,"width":4000,"height":1600},
                "zoom_out_finish_bounds":{"x":0,"y":0,"width":4000,"height":1600}},
            "collision_objects":[
                {"type":"StructurePaintable","sheet_name":"sprite_sheet_1","frame_name":"square_white_4",
                 "colour":"WHITE","collision_type":"STRUCTURE_PAINTABLE","material":"ground",
                 "rectangle":{"x":0,"y":1200,"width":4000,"height":400},
                 "paintable_faces":{"left":false,"top":true,"right":false,"bottom":false}},
                {"type":"StructurePaintable","sheet_name":"sprite_sheet_1","frame_name":"square_white_4",
                 "colour":"WHITE","collision_type":"STRUCTURE_PAINTABLE","material":"pencil",
                 "rectangle":{"x":1000,"y":800,"width":320,"height":20},
                 "paintable_faces":{"left":true,"top":true,"right":true,"bottom":true},
                 "motion":{"type":"horizontal","travel_x":120,"period":6.4,"phase":0.25}},
                {"type":"StructurePaintable","sheet_name":"sprite_sheet_1","frame_name":"square_white_4",
                 "colour":"WHITE","collision_type":"STRUCTURE_PAINTABLE","material":"eraser",
                 "rectangle":{"x":2500,"y":650,"width":160,"height":20},
                 "paintable_faces":{"left":true,"top":true,"right":true,"bottom":true},
                 "motion":{"type":"horizontal","travel_x":64,"period":4,"phase":0.75}}],
            "non_collision_objects":[]})json");
        require(!stage.HasParseError(), "Cannot parse the public-loader stage recipe");
        if (jump_through)
        {
            stage["collision_objects"][1]["collision_type"].SetString("STRUCTURE_JUMP_THROUGH");
            stage["collision_objects"][1]["material"].SetString("ruler");
        }
        if (blocked)
        {
            rapidjson::Value obstacle;
            obstacle.CopyFrom(stage["collision_objects"][0], stage.GetAllocator());
            obstacle["rectangle"]["x"].SetInt(1250);
            obstacle["rectangle"]["y"].SetInt(700);
            obstacle["rectangle"]["width"].SetInt(40);
            obstacle["rectangle"]["height"].SetInt(120);
            stage["collision_objects"].PushBack(obstacle, stage.GetAllocator());
        }
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        require(stage.Accept(writer), "Cannot encode the public-loader stage recipe");
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
        output.close();
        require(output.good(), "Cannot retain the public-loader stage receipt");
    }

    MatchSetup loaded_settings()
    {
        MatchSetup settings;
        settings.set_player_count(1);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_player_team(0, PlayerTeam::a);
        settings.set_player_weapon(0, WeaponType::sprayer);
        settings.set_stage("public_moving_fixture");
        settings.set_duration(120.0f);
        settings.set_screen_layout(labrador::ScreenLayout::one_player);
        return settings;
    }

    LoadedFixture::LoadedFixture(World& world, const std::filesystem::path& stage,
        const MatchSetup& settings, unsigned int seed) :
        tuning(world.variant([](rapidjson::Document& source) { set_countdown(source, dt / 4.0f); }))
    {
        viewports.set_layout(settings.screen_layout());
        auto info = std::make_unique<LevelLoadedInfo>(labrador::path_to_utf8(stage).c_str());
        info->validate();
        const LevelObjectBuilder objects(&world.render, &world.audio, &world.preferences, &tuning.paint());
        info->validate_content(objects, world.render, world.audio);
        levels.add(settings.stage(), std::move(info));
        LevelBuilder builder(&viewports, &world.render, &world.audio, &world.preferences,
            &levels, &tuning, &world.presentation, &resolution, nullptr, &partitioner, &mouse);
        std::srand(seed);
        level = builder.build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
    }

    void LoadedFixture::tick(const PlayerInputData& action, float seconds, SeatInputFilter* next)
    {
        class Inputs final : public SeatInputFilter
        {
        public:
            Inputs(const PlayerInputData& value, SeatInputFilter* inner) : value_(value), inner_(inner) {}
            void filter_seats(std::vector<PlayerInputData>& values, const Level& source) override
            {
                for (auto& value : values) { value = value_; }
                if (inner_ != nullptr) { inner_->filter_seats(values, source); }
            }
        private:
            PlayerInputData value_;
            SeatInputFilter* inner_;
        } filter(action, next);
        level->update({}, seconds, &filter);
    }

    void LoadedFixture::start()
    {
        for (int step = 0; !level->match_started() && step < 16; ++step) { tick(); }
        require(level->match_started(), "Public-loader fixture did not finish its bounded count-in");
        for (int step = 0; step < 3; ++step) { tick(); }
    }
}
