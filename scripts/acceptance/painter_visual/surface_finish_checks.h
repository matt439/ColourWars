// Focused surface readability evidence. Synthetic pixel checks isolate the
// finish; real Level panes retain the authored material, paint and camera paths.
Frame surface_finish_board(Application& app, const Canvas& canvas,
    const std::vector<std::unique_ptr<CollisionObject>>& objects, const Camera& camera, int count)
{
    Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(count);
    for (int view = 0; view < count; ++view)
    {
        DrawList list = renderer.view(view);
        const Viewport pane = app.viewport_manager()->player_viewport(view);
        list.set_viewport(pane); list.set_camera(camera); list.set_filter(TextureFilter::point);
        canvas.draw(list, camera.visible_rectangle(pane));
        for (const auto& object : objects)
        {
            if (camera.scale < 0.25f) { dynamic_cast<const Structure&>(*object).draw_miniature(list); }
            else { object->draw(list); }
        }
    }
    return read(renderer);
}

void surface_finish_pixels(Application& app, const Canvas& canvas, const GameTuning& tuning,
    const Presentation& presentation, PlaySettings& preferences, const std::filesystem::path& output,
    size_t& readbacks)
{
    const JsonDocument document = parse_json(R"({"objects":[
        {"name":"paint-ruler","type":"StructurePaintable","material":"ruler","sheet_name":"sprite_sheet_1","frame_name":"square_white_4","rectangle":{"x":64,"y":64,"width":320,"height":10},"collision_type":"STRUCTURE_JUMP_THROUGH","colour":"WHITE","paintable_faces":{"top":true,"bottom":false,"left":false,"right":false}},
        {"name":"bare-ruler","type":"Structure","material":"ruler","sheet_name":"sprite_sheet_1","frame_name":"square_white_4","rectangle":{"x":448,"y":64,"width":320,"height":10},"collision_type":"STRUCTURE_JUMP_THROUGH","colour":"WHITE"},
        {"name":"top-only","type":"StructurePaintable","material":"ground","sheet_name":"sprite_sheet_1","frame_name":"square_white_4","rectangle":{"x":64,"y":160,"width":240,"height":120},"collision_type":"STRUCTURE_PAINTABLE","colour":"WHITE","paintable_faces":{"top":true,"bottom":false,"left":false,"right":false}},
        {"name":"all-faces","type":"StructurePaintable","material":"ground","sheet_name":"sprite_sheet_1","frame_name":"square_white_4","rectangle":{"x":384,"y":160,"width":240,"height":120},"collision_type":"STRUCTURE_PAINTABLE","colour":"WHITE","paintable_faces":{"top":true,"bottom":true,"left":true,"right":true}},
        {"name":"buried-top","type":"StructurePaintable","material":"ground","sheet_name":"sprite_sheet_1","frame_name":"square_white_4","rectangle":{"x":64,"y":384,"width":320,"height":96},"collision_type":"STRUCTURE_PAINTABLE","colour":"WHITE","paintable_faces":{"top":true,"bottom":false,"left":false,"right":false}},
        {"name":"burial","type":"Structure","material":"ground","sheet_name":"sprite_sheet_1","frame_name":"square_white_4","rectangle":{"x":184,"y":352,"width":80,"height":32},"collision_type":"STRUCTURE","colour":"WHITE"},
        {"name":"owned-corner","type":"StructurePaintable","material":"ground","sheet_name":"sprite_sheet_1","frame_name":"square_white_4","rectangle":{"x":800,"y":160,"width":240,"height":120},"collision_type":"STRUCTURE_PAINTABLE","colour":"WHITE","paintable_faces":{"top":true,"bottom":true,"left":true,"right":true}}
    ]})", "surface-finish-native-fixture");
    const TeamColour colours = TeamColourTools::team_colours(TeamColourPair::blue_orange, presentation.team_palette());
    const LevelObjectBuilder builder(app.render_resources(), app.audio_resources(), &preferences, &tuning.paint());
    auto objects = builder.build_collision_objects(document.root().array("objects"), colours);
    auto* corner = dynamic_cast<StructurePaintable*>(objects.back().get());
    require(corner != nullptr, "Surface corner fixture lost its scoring role");
    RectangleF impact(800, 160, 240, 3);
    impact.offset(tuning.projectile(ProjectileType::spray).details.col_rect_size / 2.0f);
    const ProjectileSpray shot(impact, Vector2F::ZERO, PlayerTeam::a, 3, colours.team_a,
        app.render_resources(), resolve_projectile_strip(tuning.projectile(ProjectileType::spray), *app.render_resources()),
        &tuning.projectile(ProjectileType::spray));
    corner->on_contact(shot, Vector2F::ZERO, 0.0f); corner->update(10.0f);
    const PaintTotal before = corner->paint_total();
    require(before.team_a > 0.0f && before.team_a < before.total,
        "Surface corner fixture must retain neutral side paint below its owned top");
    for (const int count : { 1, 4 })
    {
        app.viewport_manager()->set_layout(count == 1 ? ScreenLayout::one_player : ScreenLayout::four_player);
        for (const Camera camera : { Camera(0, 0, 0.5f), Camera(0.25f, 0.75f, 0.25f), Camera(0, 0, 0.125f) })
        {
            const Frame actual = surface_finish_board(app, canvas, objects, camera, count); ++readbacks;
            require(actual.pixels == surface_finish_board(app, canvas, objects, camera, count).pixels,
                "Frozen surface finish drawing changed pixels"); ++readbacks;
            for (int view = 0; view < count; ++view)
            {
                const RectangleI pane = app.viewport_manager()->player_viewport(view).pixel_rect();
                const auto at = [&](float x, float y)
                {
                    const Vector2F projected = camera.calculate_view_rectangle(RectangleF(x, y, 1, 1)).position();
                    return monitor_rgb(actual, pane.x + static_cast<int>(projected.x), pane.y + static_cast<int>(projected.y));
                };
                require(at(100, 66) == 0xFFFFFFu && at(500, 66) != 0xFFFFFFu,
                    "Paintable and sealed thin rulers lost their pre-hit distinction");
                require(at(120, 162) == 0xFFFFFFu && at(66, 212) != 0xFFFFFFu && at(120, 278) != 0xFFFFFFu,
                    "Top-only primer appeared on a disabled side or underside");
                require(at(400, 162) == 0xFFFFFFu && at(386, 212) == 0xFFFFFFu && at(618, 212) == 0xFFFFFFu && at(400, 274) == 0xFFFFFFu,
                    "Enabled side or underside primer disappeared");
                require(at(100, 386) == 0xFFFFFFu && at(220, 386) != 0xFFFFFFu && at(300, 386) == 0xFFFFFFu,
                    "Primer crossed an interval buried by an adjoining support");
                require(at(802, 166) == at(900, 166) && at(802, 166) != 0xFFFFFFu,
                    "A neutral side overwrote already-owned top paint at a corner");
            }
            if (camera.scale == 0.5f)
            {
                save(crossfeed_crop(actual, { 0, 0, 640, 360 }), output /
                    ("surface-finish-board-" + std::to_string(count) + "p-detail.ppm"));
            }
        }
    }
    const PaintTotal after = corner->paint_total();
    require(before.total == after.total && before.team_a == after.team_a && before.team_b == after.team_b &&
        before.player_areas == after.player_areas, "Surface finish rendering changed paint score or player credit");
}

void surface_finish_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    const GameTuning& tuning, const Presentation& presentation, PlaySettings& preferences,
    const std::filesystem::path& output)
{
    const RestoreFlag restore{ preferences.reduce_effects, preferences.reduce_effects }; preferences.reduce_effects = false;
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    size_t readbacks = 0, contacts = 0, updates = 0, frames = 0, panes = 0;
    surface_finish_pixels(app, canvas, tuning, presentation, preferences, output, readbacks);
    std::ofstream records(output / "surface-finish.csv");
    records << "stage,players,paint,view,owned_fraction,camera_x,camera_y,scale,pane_x,pane_y,pane_width,pane_height\n";
    for (const char* stage : { "close_quarters", "king_of_the_hill", "turbulence", "movement_course", "crossfeed", "press_tower" })
    {
        const LevelLoadedInfo& info = *levels.get(stage);
        std::vector<RectangleF> targets;
        const JsonValue authored = info.collision_objects_json();
        for (size_t index = 0; index < authored.size(); ++index)
        {
            const JsonValue entry = authored.at(index);
            if (entry.string("type") != "StructurePaintable" || !entry.object("paintable_faces").boolean("top")) { continue; }
            if (entry.has("rectangle"))
            {
                const JsonValue r = entry.object("rectangle"); targets.emplace_back(r.number("x"), r.number("y"), r.number("width"), r.number("height"));
            }
            else
            {
                const JsonValue r = entry.object("rectangle_tlbr"); targets.push_back(RectangleF::from_top_left_bottom_right(
                    { r.number("left"), r.number("top") }, { r.number("right"), r.number("bottom") }));
            }
        }
        require(!targets.empty(), "Surface native arena has no authored scoring top");
        for (const int count : { 1, 4 })
        {
            for (const bool heavy : { false, true })
            {
                std::srand(2491);
                auto fixture = crossfeed_fixture(app, info, tuning, presentation, preferences, count, heavy, 30.0f, contacts, stage);
                const auto inputs = neutral_inputs();
                for (int tick = 0; tick < 600 && fixture.level->state() != LevelState::active; ++tick)
                { fixture.level->update(acceptance::slot_seats(*fixture.level, inputs), dt); ++updates; }
                require(fixture.level->state() == LevelState::active, "Surface native arena did not reach active play");
                // Retire the countdown's GO artwork before inspecting terrain.
                for (int tick = 0; tick < 90; ++tick) { fixture.level->update(acceptance::slot_seats(*fixture.level, inputs), dt); ++updates; }
                std::vector<Vector2F> stations;
                for (const RectangleF& target : targets)
                {
                    for (const float fraction : { 0.5f, 0.25f, 0.75f, 0.125f, 0.875f, 0.0625f, 0.9375f })
                    {
                        const RectangleF body(target.left() + target.width * fraction - tuning.player().size.x / 2.0f,
                            target.top() - tuning.player().size.y - 1.0f, tuning.player().size.x, tuning.player().size.y);
                        bool blocked = false;
                        for (const CollisionObject* object : fixture.structures)
                        {
                            const RectangleF other = object->shape()->bounding_box();
                            blocked |= body.left() < other.right() && body.right() > other.left() &&
                                body.top() < other.bottom() && body.bottom() > other.top();
                        }
                        if (!blocked) { stations.push_back(body.position()); break; }
                    }
                }
                require(!stations.empty(), "Surface native arena has no unobstructed controlled station");
                for (int view = 0; view < count; ++view)
                {
                    require(fixture.level->place_player(view, stations[static_cast<size_t>(view) * stations.size() / static_cast<size_t>(count)]),
                        "Surface native controlled station placement failed");
                }
                for (int tick = 0; tick < 6; ++tick) { fixture.level->update(acceptance::slot_seats(*fixture.level, inputs), dt); ++updates; }
                const Frame frame = crossfeed_frozen(app, fixture, readbacks);
                const std::string stem = "surface-finish-" + std::string(stage) + '-' + std::to_string(count) +
                    "p-" + (heavy ? "heavy" : "clean");
                save(frame, output / (stem + "-context.ppm")); ++frames;
                const auto ownership = crossfeed_ownership(fixture);
                for (int view = 0; view < count; ++view)
                {
                    const RectangleI pane = app.viewport_manager()->player_viewport(view).pixel_rect();
                    const Camera camera = fixture.level->view_camera(view);
                    save(crossfeed_crop(frame, pane), output / (stem + "-player" + std::to_string(view + 1) + ".ppm")); ++panes;
                    records << stage << ',' << count << ',' << (heavy ? "heavy" : "clean") << ',' << view << ',' <<
                        ownership.second << ',' << camera.translation.x << ',' << camera.translation.y << ',' << camera.scale << ',' <<
                        pane.x << ',' << pane.y << ',' << pane.width << ',' << pane.height << '\n';
                }
            }
        }
    }
    require(frames == 24 && panes == 60 && readbacks == 60 && updates < 7000 && records.good(),
        "Surface finish native evidence missed its bounded frame, pane or update budget");
    std::ofstream summary(output / "surface-finish-summary.json");
    summary << "{\"outcome\":\"passed\",\"full_frames\":24,\"pane_crops\":60,\"synthetic_crops\":2,\"readbacks\":60,\"level_updates\":" << updates <<
        ",\"heavy_contacts\":" << contacts << "}\n";
    require(summary.good(), "Surface finish native summary write failed");
    std::cout << "PASS surface finish: independent ruler/face/burial/corner pixels, all six selected real arenas clean/heavy 1P/4P, "
        "24 native frames/60 pane crops, frozen state/RNG/fingerprint; controlled stations and paint contacts, no attended readability acceptance\n";
}
