#include "game/pch.h"
#include "scripts/acceptance/moving_structure/fixtures.h"
#include "game/objects/level_object_builder.h"
#include <iostream>

namespace moving_structure_acceptance
{
    namespace
    {
        const char* loaded_objects = R"json({"objects":[
            {"type":"StructurePaintable","sheet_name":"sprite_sheet_1","frame_name":"square_white_4",
             "colour":"WHITE","collision_type":"STRUCTURE_PAINTABLE","material":"ground",
             "rectangle":{"x":0,"y":1200,"width":2400,"height":20},
             "paintable_faces":{"left":true,"top":true,"right":true,"bottom":true}},
            {"type":"StructurePaintable","sheet_name":"sprite_sheet_1","frame_name":"square_white_4",
             "colour":"WHITE","collision_type":"STRUCTURE_PAINTABLE","material":"pencil",
             "rectangle":{"x":512,"y":700,"width":320,"height":20},
             "paintable_faces":{"left":true,"top":true,"right":true,"bottom":true},
             "motion":{"type":"horizontal","travel_x":120,"period":6.4,"phase":0.25}}
        ]})json";

        void add_built(Fixture& fixture, std::vector<std::unique_ptr<labrador::CollisionObject>>& objects)
        {
            for (auto& object : objects) { fixture.scene->add(std::move(object)); }
        }
    }

    void check_loader(World& world)
    {
        using namespace labrador;
        using namespace mattmath;
        const JsonDocument document = parse_json(loaded_objects, "moving-loader-fixture.json");
        const JsonValue entries = document.root().array("objects");
        const LevelObjectBuilder builder(&world.render, &world.audio, &world.preferences, &world.tuning.paint());
        auto objects = builder.build_collision_objects(entries, world.palette);
        require(objects.size() == 2 && dynamic_cast<TranslatingStructurePaintable*>(objects[0].get()) == nullptr,
            "Unmarked static object changed its runtime type");
        auto* mover = dynamic_cast<TranslatingStructurePaintable*>(objects[1].get());
        require(mover != nullptr && mover->motion_id() == 0 && mover->bounds() == RectangleF(572, 700, 320, 20),
            "JSON mover ignored its phase, anchor or unbound admission state");
        const auto authored = motion_bits(mover->motion_state());
        auto rebuilt = builder.build_collision_objects(entries, world.palette);
        require(motion_bits(dynamic_cast<TranslatingStructurePaintable&>(*rebuilt[1]).motion_state()) == authored,
            "Fresh builder construction changed authored motion bits");
        auto static_shadows = builder.build_shadows(entries);
        auto shadows = builder.build_moving_shadows(entries, objects);
        require(static_shadows.size() == 1 && shadows.size() == 1 &&
            dynamic_cast<const TranslatedAirbrushShadow*>(shadows[0].get()) != nullptr,
            "Moving material left a static shadow or lost its translated composition");
        GameObject* shadow = shadows[0].get();
        const RectangleF shadow_before = shadow->bounds();
        const RectangleF mover_before = mover->bounds();

        Fixture fixture(world);
        fixture.tuning = world.variant([](rapidjson::Document& source) { set_countdown(source, dt / 4.0f); });
        for (auto& value : static_shadows) { fixture.scene->add(std::move(value)); }
        for (auto& value : shadows) { fixture.scene->add(std::move(value)); }
        add_built(fixture, objects);
        fixture.movers.push_back(mover);
        fixture.add_player(mover->bounds().center().x, mover->bounds().top());
        fixture.build();
        require(mover->motion_id() == 1, "Built mover did not receive the Level's stable identity");
        fixture.start();
        for (int step = 0; step < 30; ++step) { fixture.tick(input()); }
        const float moved = mover->bounds().x - mover_before.x;
        require(moved != 0.0f, "Loaded mover never advanced during actual Level simulation");
        require_near(shadow->bounds().x - shadow_before.x, moved);
        require_near(shadow->bounds().y, shadow_before.y);

        // An anchor touching another flat object must not permanently erase
        // that object's paint. Such a placement is rejected on Level admission,
        // but merely constructing it must not turn the anchor into static cover.
        const JsonDocument overlap = parse_json(R"json({"objects":[
            {"type":"StructurePaintable","sheet_name":"sprite_sheet_1","frame_name":"square_white_4",
             "colour":"WHITE","collision_type":"STRUCTURE_PAINTABLE",
             "rectangle":{"x":512,"y":720,"width":320,"height":20},
             "paintable_faces":{"left":false,"top":true,"right":false,"bottom":false}},
            {"type":"StructurePaintable","sheet_name":"sprite_sheet_1","frame_name":"square_white_4",
             "colour":"WHITE","collision_type":"STRUCTURE_PAINTABLE",
             "rectangle":{"x":512,"y":700,"width":320,"height":20},
             "paintable_faces":{"left":true,"top":true,"right":true,"bottom":true},
             "motion":{"type":"horizontal","travel_x":120,"period":6.4,"phase":0.25}}
        ]})json", "moving-occlusion-fixture.json");
        auto touching = builder.build_collision_objects(overlap.root().array("objects"), world.palette);
        auto control = world.surface({ 512, 720, 320, 20 }, { false, true, false, false });
        same_total(dynamic_cast<PaintableObject&>(*touching[0]).paint_total(), control->paint_total());

        auto refused = builder.build_collision_objects(entries, world.palette);
        auto* refused_mover = dynamic_cast<TranslatingStructurePaintable*>(refused[1].get());
        Fixture blocked(world);
        add_built(blocked, refused);
        blocked.add_player(refused_mover->bounds().center().x, refused_mover->bounds().top());
        blocked.plain({ 650, 620, 40, 160 });
        throws_with([&] { blocked.build(); }, "envelope intersects");

        std::cout << "PASS runtime-built horizontal movers: authored phase/rebuild, admission, live shadow and static occlusion\n";
    }
}
