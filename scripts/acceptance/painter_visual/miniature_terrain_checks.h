// Included after the shared monitor/raster helpers. The small-view oracle uses
// independently authored pixel rows, not projected_miniature_quads(). The real
// arena matrices supplement it with Scene order/culling and story integration.

class MiniatureFixtureMaterial final : public TerrainMaterial
{
public:
    explicit MiniatureFixtureMaterial(RenderResources* resources) :
        TerrainMaterial("miniature-fixture", resources)
    {
        const RectangleF body(64, 64, 320, 128);
        this->add(body, TerrainRole::ground_fill, body);
        // Primitive projection fixture, deliberately independent of the
        // production sealed-edge recipe. Its exact masked stroke stays fixed.
        this->add_stroke({ 64, 64, 120, 4 }, TerrainRole::keyline, body, StrokeEdge::top);
        this->add_stroke({ 224, 64, 160, 4 }, TerrainRole::keyline, body, StrokeEdge::top);
        this->add_stroke({ 64, 188, 320, 4 }, TerrainRole::keyline, body, StrokeEdge::bottom);
        this->add_stroke({ 64, 64, 4, 128 }, TerrainRole::keyline, body, StrokeEdge::left);
        this->add_stroke({ 380, 64, 4, 128 }, TerrainRole::keyline, body, StrokeEdge::right);
    }
};

size_t miniature_changed_pixels(const Frame& first, const Frame& second)
{
    require(first.width == second.width && first.height == second.height,
        "miniature comparison changed render-target dimensions");
    size_t changed = 0;
    for (size_t offset = 0; offset < first.pixels.size(); offset += 4)
    {
        if (!same_pixel(first, second, offset)) { ++changed; }
    }
    return changed;
}

Frame miniature_material_frame(Application& app, const Canvas& canvas,
    const TerrainMaterial* material, const Camera& camera, bool adapted, int count)
{
    Renderer& renderer = *app.renderer();
    renderer.begin_frame();
    renderer.set_view_count(count);
    for (int view = 0; view < count; ++view)
    {
        DrawList list = renderer.view(view);
        const Viewport pane = app.viewport_manager()->player_viewport(view);
        list.set_viewport(pane);
        list.set_camera(Camera::DEFAULT_CAMERA);
        list.set_filter(TextureFilter::point);
        canvas.draw(list, RectangleF(0, 0, pane.width, pane.height));
        list.set_camera(camera);
        if (material == nullptr) { continue; }
        if (adapted) { material->paint_miniature(list); }
        else { material->paint(list); }
    }
    return read(renderer);
}

void miniature_pixel(const Frame& frame, const Viewport& pane, int x, int y,
    std::uint32_t expected, const char* message)
{
    const RectangleI physical = pane.pixel_rect();
    require(monitor_rgb(frame, physical.x + x, physical.y + y) == expected, message);
}

void miniature_fractional_edges(Application& app, const Canvas& canvas,
    const TerrainMaterial& material, int count, const Vector2I& size, std::ofstream& records)
{
    struct Expected
    {
        Camera camera; RectangleI body; int gap_left, gap_right;
    };
    const std::array<Expected, 3> cases{ {
        { Camera(0.25f, 0.75f, 0.125f), { 7, 7, 40, 16 }, 22, 27 },
        { Camera(0, 0, 0.1875f), { 12, 12, 60, 24 }, 34, 42 },
        { Camera(0.25f, 0.75f, 0.1875f), { 11, 11, 60, 24 }, 34, 41 }
    } };
    for (const Expected& expected : cases)
    {
        const Frame blank = miniature_material_frame(app, canvas, nullptr, expected.camera, false, count);
        const Frame actual = miniature_material_frame(app, canvas, &material, expected.camera, true, count);
        for (int view = 0; view < count; ++view)
        {
            const Viewport pane = app.viewport_manager()->player_viewport(view);
            const RectangleI physical = pane.pixel_rect();
            for (int y = 0; y < physical.height; ++y)
            {
                for (int x = 0; x < physical.width; ++x)
                {
                    const RectangleI& body = expected.body;
                    const bool inside = x >= body.x && x < body.right() && y >= body.y && y < body.bottom();
                    const bool edge = inside && (x == body.x || x == body.right() - 1 || y == body.bottom() - 1 ||
                        (y == body.y && (x < expected.gap_left || x >= expected.gap_right)));
                    miniature_pixel(actual, pane, x, y, inside ? (edge ? 0x000000u : 0x8C8A86u) :
                        monitor_rgb(blank, physical.x + x, physical.y + y),
                        "fractional miniature camera lost an edge, extended a masked face or changed the body silhouette");
                }
            }
        }
        records << "fractional-edge,masked-rectangle," << count << ',' << size.x << ',' << size.y << ',' <<
            expected.camera.scale << ",0\n";
    }
}

void miniature_ruler_orientations(Application& app, const Canvas& canvas, int count,
    const Vector2I& size, std::ofstream& records)
{
    TerrainFaces horizontal;
    horizontal.top = { { 64.0f, 384.0f } };
    const RulerMaterial two_pixel({ 64, 256, 320, 16 }, horizontal, app.render_resources());
    TerrainFaces vertical;
    // The far edge is buried; the measurement marks remain exposed inside.
    vertical.left = { { 64.0f, 384.0f } };
    const RulerMaterial upright({ 64, 64, 32, 320 }, vertical, app.render_resources());
    const Camera camera(0, 0, 0.125f);
    const Frame board = miniature_material_frame(app, canvas, &two_pixel, camera, true, count);
    const Frame ticks = miniature_material_frame(app, canvas, &upright, camera, true, count);
    for (int view = 0; view < count; ++view)
    {
        const Viewport pane = app.viewport_manager()->player_viewport(view);
        for (int x = 8; x < 48; ++x)
        {
            miniature_pixel(board, pane, x, 32, 0x000000, "two-pixel ruler lost its one leading keyline");
            miniature_pixel(board, pane, x, 33, 0xB5AFA3, "two-pixel ruler lost the light row beneath its sealed leading edge");
        }
        for (int y = 9; y < 47; ++y)
        {
            miniature_pixel(ticks, pane, 8, y, 0x000000, "vertical ruler lost its leading keyline");
            miniature_pixel(ticks, pane, 9, y, 0xB5AFA3, "vertical ruler lost its light body");
            miniature_pixel(ticks, pane, 10, y,
                y == 10 || y == 20 || y == 30 || y == 40 ? 0x45423Eu : 0xB5AFA3u,
                "vertical ruler major ticks or light gaps differed from independent pixels");
        }
    }
    records << "ruler-orientations,two-pixel-and-vertical," << count << ',' << size.x << ',' << size.y << ",0.125,0\n";
}

void miniature_material_matrix(Application& app, const Canvas& canvas,
    const std::filesystem::path& output, std::ofstream& records)
{
    MiniatureFixtureMaterial fixture(app.render_resources());
    TerrainFaces ruler_faces;
    // Leading edge exposed, far edge and ends buried: isolate surviving ticks.
    ruler_faces.top = { { 64.0f, 384.0f } };
    const RulerMaterial ruler({ 64, 256, 320, 32 }, ruler_faces, app.render_resources());
    TerrainFaces thin_faces;
    thin_faces.top = { { 64.0f, 384.0f } };
    const RulerMaterial thin({ 64, 256, 320, 10 }, thin_faces, app.render_resources());
    const TriangleRightAxisAligned triangle(416, 192, 544, 192, 544, 64);
    TerrainFaces ramp_faces;
    ramp_faces.bottom = { { 416.0f, 544.0f } }; ramp_faces.right = { { 64.0f, 192.0f } };
    const SetSquareMaterial ramp(triangle, ramp_faces, app.render_resources());
    const std::array<const TerrainMaterial*, 4> materials{ &fixture, &ruler, &thin, &ramp };
    const std::array<const char*, 4> names{ "exposed", "ruler", "thin-ruler", "ramp" };
    size_t corrected = 0;
    for (const int count : { 1, 4 })
    {
        app.viewport_manager()->set_layout(count == 1 ? ScreenLayout::one_player : ScreenLayout::four_player);
        for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
        {
            app.resolution_manager()->set_resolution_exactly(size);
            std::ignore = app.renderer()->window_size_changed(size.x, size.y);
            for (size_t material = 0; material < materials.size(); ++material)
            {
                const Camera camera(0, 0, 0.125f);
                const Frame blank = miniature_material_frame(app, canvas, nullptr, camera, false, count);
                const Frame old = miniature_material_frame(app, canvas, materials[material], camera, false, count);
                std::srand(3917); const int expected_random = std::rand(); std::srand(3917);
                const Frame fixed = miniature_material_frame(app, canvas, materials[material], camera, true, count);
                require(fixed.pixels == miniature_material_frame(app, canvas, materials[material], camera, true, count).pixels,
                    "frozen miniature material drawing changed pixels");
                require(std::rand() == expected_random, "miniature material drawing consumed match randomness");
                const size_t changed = miniature_changed_pixels(old, fixed);
                corrected += changed;
                for (int view = 0; view < count; ++view)
                {
                    const Viewport pane = app.viewport_manager()->player_viewport(view);
                    if (material == 0)
                    {
                        // Rectangle(64,64,320,128) at exactly one eighth scale:
                        // [8,48)x[8,24), with a deliberately buried top span.
                        // Every pixel, including the complete outside, is checked.
                        const RectangleI physical = pane.pixel_rect();
                        for (int y = 0; y < physical.height; ++y)
                        {
                            for (int x = 0; x < physical.width; ++x)
                            {
                                const bool inside = x >= 8 && x < 48 && y >= 8 && y < 24;
                                const bool outline = inside && (x == 8 || x == 47 || y == 23 ||
                                    (y == 8 && (x < 23 || x >= 28)));
                                const auto colour = inside ? (outline ? 0x000000u : 0x8C8A86u) :
                                    monitor_rgb(blank, physical.x + x, physical.y + y);
                                miniature_pixel(fixed, pane, x, y, colour,
                                    "miniature exposed outline, masked seam, lit body or exact silhouette differed");
                            }
                        }
                    }
                    if (material == 1)
                    {
                        // The 32-unit ruler is four pixels thick. Its 80-unit
                        // major marks are ten pixels apart; 16-unit marks are
                        // too close here and must not replace the light gaps.
                        for (int x = 9; x < 47; ++x)
                        {
                            miniature_pixel(fixed, pane, x, 32, 0x000000,
                                "miniature ruler lost its continuous leading keyline");
                            miniature_pixel(fixed, pane, x, 33, 0xB5AFA3,
                                "miniature ruler lost its light body");
                            miniature_pixel(fixed, pane, x, 34,
                                x == 10 || x == 20 || x == 30 || x == 40 ? 0x45423Eu : 0xB5AFA3u,
                                "miniature ruler major ticks disappeared or minor ticks crowded their light gaps");
                        }
                    }
                    if (material == 2)
                    {
                        for (int x = 8; x < 48; ++x)
                        {
                            miniature_pixel(fixed, pane, x, 32, 0x6E7275,
                                "one-pixel miniature ruler lost its sealed substrate silhouette");
                        }
                    }
                }
                if (material == 3)
                {
                    size_t lit = 0;
                    for (size_t pixel = 0; pixel < fixed.pixels.size(); pixel += 4)
                    {
                        if (same_pixel(old, blank, pixel))
                        {
                            require(same_pixel(fixed, blank, pixel),
                                "miniature ramp invented a pixel beyond its original filled silhouette");
                        }
                        if (fixed.pixels[pixel] == 0xB8 && fixed.pixels[pixel + 1] == 0xBC &&
                            fixed.pixels[pixel + 2] == 0xBE) { ++lit; }
                    }
                    require(lit > 0, "miniature ramp lost its lit metal interior");
                }
                records << "material," << names[material] << ',' << count << ',' << size.x << ',' << size.y <<
                    ",0.125," << changed << '\n';
                if (size.x == 1280 && count == 1)
                {
                    const RectangleI crop(material == 3 ? 48 : 4, material == 1 || material == 2 ? 28 : 4,
                        material == 3 ? 24 : 48, material == 1 || material == 2 ? 12 : 24);
                    const std::string stem = "miniature-terrain-" + std::string(names[material]);
                    save(foreground_crop(old, crop, 0), output / (stem + "-legacy-detail.ppm"));
                    save(foreground_crop(fixed, crop, 0), output / (stem + "-adapted-detail.ppm"));
                }
                for (const float scale : { 0.25f, 0.5f, 1.0f })
                {
                    const Camera native_camera(0.75f, 0.25f, scale);
                    require(miniature_material_frame(app, canvas, materials[material], native_camera, false, count).pixels ==
                        miniature_material_frame(app, canvas, materials[material], native_camera, true, count).pixels,
                        "miniature treatment changed point-sampled material at one cell per pixel or above");
                    records << "native-material," << names[material] << ',' << count << ',' << size.x << ',' <<
                        size.y << ',' << scale << ",0\n";
                }
            }
            miniature_fractional_edges(app, canvas, fixture, count, size, records);
            miniature_ruler_orientations(app, canvas, count, size, records);
        }
    }
    require(corrected > 0, "miniature treatment never repaired a disappearing stroke");
}

Frame miniature_structure_frame(Application& app, const Canvas& canvas,
    const Structure& structure, const Camera& camera, bool adapted)
{
    Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
    DrawList list = renderer.view(0);
    list.set_viewport(app.viewport_manager()->fullscreen_viewport());
    list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
    canvas.draw(list, RectangleF(Vector2F::ZERO, app.resolution_manager()->resolution_vec()));
    list.set_camera(camera);
    if (adapted) { structure.draw_miniature(list); } else { structure.draw(list); }
    return read(renderer);
}

void miniature_paint_matrix(Application& app, const Canvas& canvas, const GameTuning& tuning,
    const Presentation& presentation, PlaySettings& preferences, const std::filesystem::path& output,
    std::ofstream& records)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    const RectangleF body(64, 64, 320, 128);
    const TeamColour colours = TeamColourTools::team_colours(TeamColourPair::blue_orange, presentation.team_palette());
    const auto make = [&](bool material)
    {
        return std::make_unique<StructurePaintable>("sprite_sheet_1", "square_white_4", body,
            std::make_unique<RectangleF>(body), app.render_resources(), app.audio_resources(), &preferences,
            &tuning.paint(), CollisionObjectType::structure_paintable, colours,
            PaintableFaces{ false, true, false, false }, std::span<const RectangleF>{}, Colour::white,
            0.0f, Vector2F::ZERO, SpriteFlip::none, 0.0f,
            material ? std::make_unique<MiniatureFixtureMaterial>(app.render_resources()) : nullptr);
    };
    auto painted = make(true); auto overlay_reference = make(false);
    RectangleF splash(64, 60, 320, 16);
    const Vector2F half = tuning.projectile(ProjectileType::spray).details.col_rect_size / 2;
    splash.offset(half.x, half.y);
    const ProjectileSpray shot(splash, Vector2F::ZERO, PlayerTeam::a, 3, colours.team_a,
        app.render_resources(), resolve_projectile_strip(tuning.projectile(ProjectileType::spray), *app.render_resources()), &tuning.projectile(ProjectileType::spray));
    painted->on_contact(shot, Vector2F::ZERO, 0); overlay_reference->on_contact(shot, Vector2F::ZERO, 0);
    painted->update(10); overlay_reference->update(10);
    const PaintTotal before = painted->paint_total();
    require(before.team_a > 0 && before.team_b == 0, "miniature paint fixture did not establish actual owned paint");
    for (const Camera camera : { Camera(0, 0, 0.125f), Camera(0.75f, 0.25f, 0.25f), Camera(0, 0, 0.5f) })
    {
        const Frame old = miniature_structure_frame(app, canvas, *painted, camera, false);
        const Frame fixed = miniature_structure_frame(app, canvas, *painted, camera, true);
        const Frame paint_only = miniature_structure_frame(app, canvas, *overlay_reference, camera, false);
        const RectangleF band = camera.calculate_view_rectangle({ 64, 64, 320, tuning.paint().thickness });
        const int top = static_cast<int>(band.y), bottom = static_cast<int>(band.bottom());
        const int left = static_cast<int>(band.x), right = static_cast<int>(band.right());
        require(bottom > top && right > left, "owned miniature paint band quantized away in the fixture");
        for (int y = top; y < bottom; ++y)
        {
            for (int x = left; x < right; ++x)
            {
                require(monitor_rgb(old, x, y) == monitor_rgb(fixed, x, y) &&
                    monitor_rgb(fixed, x, y) == monitor_rgb(paint_only, x, y),
                    "miniature terrain painted over owned surface colour or changed its inward edge");
            }
        }
        const PaintTotal after = painted->paint_total();
        require(before.total == after.total && before.team_a == after.team_a && before.team_b == after.team_b &&
            before.player_areas == after.player_areas, "miniature drawing changed paint score or attribution");
        if (camera.scale >= 0.25f) { require(old.pixels == fixed.pixels, "native painted terrain pixels changed"); }
        records << "paint,owned-top,1,1280,720," << camera.scale << ',' << miniature_changed_pixels(old, fixed) << '\n';
        if (camera.scale == 0.125f)
        {
            save(foreground_crop(old, { 4, 4, 48, 24 }, 0), output / "miniature-terrain-painted-legacy-detail.ppm");
            save(foreground_crop(fixed, { 4, 4, 48, 24 }, 0), output / "miniature-terrain-painted-adapted-detail.ppm");
        }
    }
}

Frame miniature_scene_reference(Application& app, const Scene& scene,
    const std::vector<GameObject*>& scenery, bool unculled)
{
    Renderer& renderer = *app.renderer(); renderer.begin_frame();
    renderer.set_view_count(scene.view_count());
    for (int view = 0; view < scene.view_count(); ++view)
    {
        DrawList list = renderer.view(view); list.set_filter(TextureFilter::point);
        const Scene::View& pane = scene.view(view);
        if (!unculled)
        { draw_legacy_miniature_view(list, pane.camera, pane.viewport, scenery, scene.collision_objects()); }
        else
        {
            list.set_viewport(pane.viewport); list.set_camera(pane.camera);
            for (const GameObject* object : scenery) { object->draw(list); }
            for (const CollisionObject* object : scene.collision_objects())
            {
                if (const auto* structure = dynamic_cast<const Structure*>(object)) { structure->draw_miniature(list); }
                else { object->draw(list); }
            }
        }
    }
    return read(renderer);
}

Frame miniature_scene_frame(Application& app, const Scene& scene, bool adapted,
    const std::vector<GameObject*>& scenery, bool unculled = false)
{
    if (unculled) { return miniature_scene_reference(app, scene, scenery, true); }
    configure_miniature_fixture(scene, adapted);
    const auto before_modes = miniature_fixture_modes(scene);
    Renderer& renderer = *app.renderer(); renderer.begin_frame();
    // Production owns the view fan-out, culling and scenery/collision order.
    scene.draw(renderer);
    const Frame frame = read(renderer);
    require(before_modes == miniature_fixture_modes(scene),
        "Scene drawing changed a path-selected structure miniature flag");
    return frame;
}

void miniature_preview_matrix(Application& app, Registry<LevelLoadedInfo>& levels, Scene& scene,
    const std::vector<GameObject*>& scenery, const char* stage, const GameTuning& tuning,
    const Presentation& presentation, const std::filesystem::path& output, std::ofstream& records)
{
    Save preferences;
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_level_infos(&levels);
    data.set_tuning(&tuning); data.set_presentation(&presentation);
    StagePreview preview("miniature-test-preview", { 960, 270, 960, 540 }, &data);
    preview.set_stage(stage);
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    Frame first_720; size_t changed = 0;
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440), Vector2I(1280, 720) })
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        // Independent physical pane for this authored half-width/half-height
        // design rectangle at the three 16:9 sizes; no StagePreview geometry
        // or production menu fit helper is used to obtain the reference pane.
        const Viewport pane(static_cast<float>(size.x / 2), static_cast<float>(size.y / 4),
            static_cast<float>(size.x / 2), static_cast<float>(size.y / 2));
        scene.clear_views(); scene.add_view(pane,
            Camera::frame(levels.get(stage)->zoom_out_finish_bounds_rectangle(), pane));
        const Frame old = miniature_scene_frame(app, scene, false, scenery);
        const Frame reference = miniature_scene_reference(app, scene, scenery, false);
        const auto actual_frame = [&]()
        {
            Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
            DrawList list = renderer.view(0); list.set_viewport(app.viewport_manager()->fullscreen_viewport());
            list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point); preview.draw(list);
            return read(renderer);
        };
        std::srand(1469); const int expected_random = std::rand(); std::srand(1469);
        const Frame actual = actual_frame();
        require(actual.pixels == reference.pixels,
            "actual StagePreview did not adapt terrain at its independently specified physical pane and camera");
        require(actual.pixels == actual_frame().pixels && std::rand() == expected_random,
            "actual miniature StagePreview changed frozen pixels or consumed match randomness");
        const size_t difference = miniature_changed_pixels(old, actual); changed += difference;
        records << "stage-preview," << stage << ",1," << size.x << ',' << size.y << ',' <<
            scene.view(0).camera.scale << ',' << difference << '\n';
        if (size.x == 1280 && first_720.pixels.empty())
        {
            first_720 = actual;
            const RectangleI crop = pane.pixel_rect();
            save(foreground_crop(old, crop, 0), output / ("miniature-terrain-preview-" + std::string(stage) + "-legacy-detail.ppm"));
            save(foreground_crop(actual, crop, 0), output / ("miniature-terrain-preview-" + std::string(stage) + "-adapted-detail.ppm"));
        }
        else if (size.x == 1280)
        { require(first_720.pixels == actual.pixels, "actual StagePreview changed after native resize return"); }
    }
    // The original four stages remain positive visual-change controls.
    // Crossfeed and Press Tower retain exact reference/repeat/resize coverage:
    // authored primer can mask miniature strokes, and larger previews use
    // ordinary art. Every authored stage need not change pixels from legacy.
    const std::string_view stage_name(stage);
    if (stage_name == "close_quarters" || stage_name == "king_of_the_hill" ||
        stage_name == "turbulence" || stage_name == "movement_course")
    { require(changed > 0, "actual StagePreview never differed from disappearing-stroke legacy terrain"); }
}

void miniature_stage_matrix(Application& app, Registry<LevelLoadedInfo>& levels,
    const GameTuning& tuning, const Presentation& presentation, PlaySettings& preferences, const std::filesystem::path& output,
    std::ofstream& records)
{
    const LevelObjectBuilder builder(app.render_resources(), app.audio_resources(), &preferences, &tuning.paint());
    size_t changed_overviews = 0;
    for (const char* stage : { "close_quarters", "king_of_the_hill", "turbulence", "movement_course", "crossfeed", "press_tower" })
    {
        const LevelLoadedInfo* info = levels.get(stage);
        Scene scene(app.thread_pool(), app.partitioner()); std::vector<GameObject*> scenery;
        for (auto& object : builder.build_non_collision_objects(info->non_collision_objects_json()))
        { scenery.push_back(scene.add(std::move(object))); }
        for (auto& object : builder.build_shadows(info->collision_objects_json()))
        { scenery.push_back(scene.add(std::move(object))); }
        for (auto& object : builder.build_collision_objects(info->collision_objects_json(), TeamColour{}))
        { scene.add(std::move(object)); }
        scene.end_tick();
        miniature_preview_matrix(app, levels, scene, scenery, stage, tuning, presentation, output, records);
        const Vector2F spawn = info->team_a_spawns().front();
        scenery.push_back(scene.add(std::make_unique<Visual>("sprite_sheet_1", "pixel",
            RectangleF(spawn - Vector2F(32, 24), Vector2F(96, 96)), app.render_resources(), Colour(0.7f, 0.2f, 0.6f))));
        MenuLevelSettings player_settings; player_settings.set_stage(stage); player_settings.set_player_count(1);
        player_settings.set_screen_layout(ScreenLayout::one_player); player_settings.begin_input_claims(3);
        player_settings.set_player_team(0, PlayerTeam::a); player_settings.set_player_weapon(0, WeaponType::roller);
        const PlayerBuilder player_builder(&tuning, &presentation);
        for (auto& player : player_builder.build_players(player_settings, info, TeamColour{},
            app.render_resources(), app.audio_resources(), &preferences, app.viewport_manager()))
        { scene.add(std::move(player)); }
        scene.add(std::make_unique<ProjectileSpray>(RectangleF(spawn + Vector2F(24, 48), Vector2F(64, 64)),
            Vector2F::ZERO, PlayerTeam::a, 3, Colour(0.2f, 0.5f, 0.9f), app.render_resources(),
            resolve_projectile_strip(tuning.projectile(ProjectileType::spray), *app.render_resources()),
            &tuning.projectile(ProjectileType::spray)));
        scene.end_tick();
        for (const int count : { 1, 4 })
        {
            app.viewport_manager()->set_layout(count == 1 ? ScreenLayout::one_player : ScreenLayout::four_player);
            Frame first_720;
            for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440), Vector2I(1280, 720) })
            {
                app.resolution_manager()->set_resolution_exactly(size);
                std::ignore = app.renderer()->window_size_changed(size.x, size.y);
                scene.clear_views();
                for (int view = 0; view < count; ++view)
                {
                    const Viewport pane = app.viewport_manager()->player_viewport(view);
                    scene.add_view(pane, Camera::frame(info->zoom_out_finish_bounds_rectangle(), pane));
                }
                std::srand(7091); const int expected_random = std::rand(); std::srand(7091);
                const Frame old = miniature_scene_frame(app, scene, false, scenery);
                const Frame fixed = miniature_scene_frame(app, scene, true, scenery);
                require(fixed.pixels == miniature_scene_reference(app, scene, scenery, false).pixels,
                    "Miniature Scene differs from the independent legacy painter/cull traversal");
                require(fixed.pixels == miniature_scene_frame(app, scene, true, scenery).pixels,
                    "frozen real miniature terrain changed pixels");
                require(fixed.pixels == miniature_scene_frame(app, scene, true, scenery, true).pixels,
                    "miniature Scene culling dropped visible scenery, terrain, player or projectile pixels");
                require(std::rand() == expected_random, "real miniature terrain consumed match randomness");
                const size_t changed = miniature_changed_pixels(old, fixed); changed_overviews += changed;
                records << "overview," << stage << ',' << count << ',' << size.x << ',' << size.y << ',' <<
                    scene.view(0).camera.scale << ',' << changed << '\n';
                if (size.x == 1280 && first_720.pixels.empty())
                {
                    first_720 = fixed;
                    const std::string stem = "miniature-terrain-" + std::string(stage) + '-' + std::to_string(count) + "p";
                    const RectangleI crop = count == 1 ? RectangleI(0, 0, 1280, 720) : RectangleI(0, 0, 640, 360);
                    save(foreground_crop(old, crop, 0), output / (stem + "-legacy-detail.ppm"));
                    save(foreground_crop(fixed, crop, 0), output / (stem + "-adapted-detail.ppm"));
                }
                else if (size.x == 1280)
                { require(fixed.pixels == first_720.pixels, "miniature overview changed after its native resize round trip"); }

                // Ordinary Scene is the independent ordering/culling oracle at
                // native scale, including scenery overlapping solid terrain.
                scene.clear_views();
                for (int view = 0; view < count; ++view)
                {
                    const Viewport pane = app.viewport_manager()->player_viewport(view);
                    const auto spawns = view % 2 == 0 ? info->team_a_spawns() : info->team_b_spawns();
                    scene.add_view(pane, Camera(spawns.front() - Vector2F(200, 120), 0.5f));
                }
                require(miniature_scene_frame(app, scene, false, scenery).pixels ==
                    miniature_scene_frame(app, scene, true, scenery).pixels,
                    "path-selected miniature Scene changed painter order, culling or ordinary point pixels");
                records << "native-scene," << stage << ',' << count << ',' << size.x << ',' << size.y << ",0.5,0\n";
            }
        }
    }
    require(changed_overviews > 0, "no shipped miniature overview changed from its disappearing-stroke baseline");
}

Frame miniature_story_frozen(Application& app, Level& level)
{
    const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
    const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
    std::srand(9233); const int expected_random = std::rand(); std::srand(9233);
    const Frame first = render(level, *app.renderer());
    const int view_count = app.renderer()->view_count();
    require(first.pixels == render(level, *app.renderer()).pixels,
        "frozen actual miniature story changed pixels");
    // Two declared prefix views leave one earlier empty view and append this
    // Level to the last. This observes the real Level path, including monitor
    // scenery/foreground, rather than recreating its offset arithmetic.
    app.renderer()->begin_frame(); app.renderer()->set_view_count(2);
    level.draw(*app.renderer());
    require(app.renderer()->view_count() == view_count + 1,
        "actual miniature Level discarded an earlier renderer view while appending");
    require(first.pixels == read(*app.renderer()).pixels,
        "actual miniature Level changed pixels when appended after an empty prefix view");
    require(std::rand() == expected_random && before == observation(level),
        "actual miniature story drawing changed simulation state or randomness");
#if COLOURWARS_DEV_TOOLS
    require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
        "actual miniature story drawing changed the replay fingerprint");
#endif
    return first;
}

void miniature_story_matrix(Application& app, LevelBuilder& builder, const GameTuning& tuning,
    const std::filesystem::path& output, std::ofstream& records)
{
    const NestedSaveProfile isolated(output / "miniature-story-profile");
    for (const char* stage : { "close_quarters", "king_of_the_hill", "turbulence", "movement_course", "crossfeed", "press_tower" })
    {
        for (const int count : { 1, 4 })
        {
            std::cout << "CHECK miniature story " << stage << ' ' << count << "P\n";
            MenuLevelSettings settings; settings.set_stage(stage); settings.set_player_count(count); settings.set_duration(0.75f);
            settings.set_screen_layout(count == 1 ? ScreenLayout::one_player : ScreenLayout::four_player);
            constexpr int slots[] = { 3, 1, 2, 0 }; settings.begin_input_claims(slots[0]);
            for (int view = 1; view < count; ++view)
            { require(settings.claim_input_slot(slots[view]), "miniature story could not claim its sparse roster"); }
            for (int view = 0; view < count; ++view)
            { settings.set_player_team(view, view % 2 ? PlayerTeam::b : PlayerTeam::a); settings.set_player_weapon(view, WeaponType::roller); }
            app.viewport_manager()->set_layout(settings.screen_layout());
            std::srand(533 + count);
            auto level = builder.build_level(settings, TeamColourPair::blue_orange,
                count == 1 ? OpeningMode::off : OpeningMode::full);
            auto inputs = neutral_inputs();
            const auto sample = [&](const char* beat)
            {
                Frame first; std::vector<Camera> first_cameras, first_player_cameras;
                for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440), Vector2I(1280, 720) })
                {
                    resize(app, *level, size);
                    const Frame frame = miniature_story_frozen(app, *level);
                    records << "story-" << beat << ',' << stage << ',' << count << ',' << size.x << ',' << size.y << ',' <<
                        static_cast<int>(level->state()) << ",0\n";
                    if (size.x == 1280 && first.pixels.empty())
                    {
                        first = frame;
                        for (int view = 0; view < count; ++view)
                        { first_cameras.push_back(level->view_camera(view)); first_player_cameras.push_back(level->player(view).camera()); }
                        if (std::string(beat) == "overview" || (count == 4 && std::string(beat) == "preview"))
                        {
                            const std::string stem = "miniature-terrain-story-" + std::string(stage) + '-' +
                                std::to_string(count) + "p-" + beat + "-detail.ppm";
                            save(foreground_crop(frame, { 160, 90, 960, 540 }, 0), output / stem);
                        }
                    }
                    else if (size.x == 1280)
                    {
                        if (frame.pixels != first.pixels)
                        {
                            // The accepted hysteretic cameras can return two ULPs
                            // away at the resized course's active deadzone edge.
                            // Normalize only that measured camera state, then keep
                            // the full-frame byte oracle (including the player).
                            require(std::string(stage) == "movement_course" && std::string(beat) == "active",
                                "Unexpected miniature resize return pixels outside the active course camera");
                            std::vector<Camera> returned; bool changed = false;
                            for (int view = 0; view < count; ++view)
                            {
                                const Camera original = first_player_cameras[static_cast<size_t>(view)];
                                const Camera current = level->player(view).camera(); returned.push_back(current);
                                float low = original.translation.y, high = low;
                                for (int ulp = 0; ulp < 4; ++ulp)
                                {
                                    low = std::nextafter(low, -std::numeric_limits<float>::infinity());
                                    high = std::nextafter(high, std::numeric_limits<float>::infinity());
                                }
                                std::cout << "CHECK miniature active resize camera pane " << view << ' ' << std::hexfloat <<
                                    original.translation.x << ',' << original.translation.y << ',' << original.scale << " -> " <<
                                    current.translation.x << ',' << current.translation.y << ',' << current.scale << std::defaultfloat << '\n';
                                require(level->view_camera(view) == current && first_cameras[static_cast<size_t>(view)] == original &&
                                    current.scale == original.scale && current.translation.x == original.translation.x &&
                                    current.translation.y >= low && current.translation.y <= high,
                                    "Unexpected miniature active resize camera state");
                                changed |= current != original;
                            }
                            require(changed, "Miniature resize pixels changed without the measured camera drift");
                            for (int view = 0; view < count; ++view)
                            { const_cast<Player&>(level->player(view)).set_camera(first_player_cameras[static_cast<size_t>(view)]); }
                            level->refresh_layout(); const Frame normalized = miniature_story_frozen(app, *level);
                            for (int view = 0; view < count; ++view)
                            { const_cast<Player&>(level->player(view)).set_camera(returned[static_cast<size_t>(view)]); }
                            level->refresh_layout();
                            require(normalized.pixels == first.pixels && frame.pixels == miniature_story_frozen(app, *level).pixels,
                                "Miniature resize return changed pixels beyond its measured camera state");
                            records << "resize-camera-normalized," << stage << ',' << count << ",1280,720,0," <<
                                miniature_changed_pixels(first, frame) << '\n';
                            std::cout << "CHECK miniature active normalized full-frame equality and restored cameras passed\n";
                        }
                    }
                }
            };
            const auto advance = [&](float seconds)
            { for (int tick = 0; tick < static_cast<int>(std::ceil(seconds / dt)); ++tick) { level->update(inputs, dt); } };
            if (count == 4)
            {
                sample("opening");
                advance(tuning.match().opening.settle + tuning.match().opening.preview * 0.5f);
                require(level->state() == LevelState::opening, "miniature story missed its monitor preview");
                sample("preview");
            }
            for (int tick = 0; tick < 2400 && level->state() == LevelState::opening; ++tick) { level->update(inputs, dt); }
            require(level->state() == LevelState::start_countdown, "miniature story missed the countdown"); sample("countdown");
            advance(std::max(0.0f, tuning.match().countdown - tuning.match().opening.push * 0.5f));
            if (count == 4) { sample("push"); }
            for (int tick = 0; tick < 1200 && level->state() != LevelState::active; ++tick) { level->update(inputs, dt); }
            require(level->state() == LevelState::active, "miniature story did not reach ordinary gameplay"); sample("active");
            for (int tick = 0; tick < 1200 && level->state() == LevelState::active; ++tick) { level->update(inputs, dt); }
            require(level->state() == LevelState::zoom_out, "miniature story missed pullback");
            advance(tuning.match().zoom_out * 0.5f); sample("pullback");
            for (int tick = 0; tick < 1200 && level->state() == LevelState::zoom_out; ++tick) { level->update(inputs, dt); }
            require(level->state() == LevelState::overview, "miniature story did not reach overview"); sample("overview");
        }
    }
}

void miniature_terrain_matrix(Application& app, Registry<LevelLoadedInfo>& levels, LevelBuilder& builder,
    const Canvas& canvas, const GameTuning& tuning, const Presentation& presentation,
    PlaySettings& preferences, const std::filesystem::path& output)
{
    const RestoreFlag restore{ preferences.reduce_effects, preferences.reduce_effects };
    preferences.reduce_effects = false;
    std::ofstream records(output / "miniature-terrain.csv");
    records << "kind,fixture,players,width,height,scale_or_state,changed_pixels\n";
    miniature_material_matrix(app, canvas, output, records);
    miniature_paint_matrix(app, canvas, tuning, presentation, preferences, output, records);
    miniature_stage_matrix(app, levels, tuning, presentation, preferences, output, records);
    miniature_story_matrix(app, builder, tuning, output, records);
    miniature_path_checks(app, levels, tuning, presentation, preferences, output, records);
    require(records.good(), "miniature terrain comparison record write failed");
    std::cout << "PASS miniature terrain: independent exposed edge/masked seam/lit body/major ruler tick and ramp silhouette "
        "pixel oracles, legacy comparisons, actual owned paint overlay, exact native material and Scene parity; "
        "all six selected stage 1P/4P overview/story contexts at720p/1080p/QHD, resize return, frozen state/RNG and Debug fingerprints. "
        "Muted renderer evidence; human readability, physical pads and performance acceptance remain separate.\n";
}
