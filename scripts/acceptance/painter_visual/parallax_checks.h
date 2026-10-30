// Comparison fixture only. No shipped object, camera, content or Scene path
// opts into parallax. Scene remains the only owner/updater of the real objects.
enum class ParallaxDepth { fixed, distant, hill };

struct ParallaxSprite
{
    GameObject* object = nullptr;
    ParallaxDepth depth = ParallaxDepth::fixed;
    RectangleF anchor;
    RectangleI source;
};

struct ParallaxWorld
{
    LandmarkWorld actual;
    std::vector<ParallaxSprite> sprites;
    Vector2F anchor;
    const PlaySettings* preferences = nullptr;
};

struct ParallaxPass
{
    bool scene = false;
    bool atlas = false;
    bool distant = false;
    bool hills = false;
    bool reduced = false;
    int foreground = -1; // -1 ordinary, 0 black, 1 white
    int prop = -1;       // isolated prop over the selected foreground canvas
};

float parallax_offset(const Camera& camera, const Viewport& pane, const Vector2F& anchor,
    ParallaxDepth depth, const ParallaxPass& pass)
{
    if (pass.reduced || depth == ParallaxDepth::fixed ||
        (depth == ParallaxDepth::distant && !pass.distant) ||
        (depth == ParallaxDepth::hill && !pass.hills)) { return 0.0f; }
    const float distance = camera.visible_rectangle(pane).center().x - anchor.x;
    const float fraction = depth == ParallaxDepth::distant ? 0.02f : 0.01f;
    const float cap = depth == ParallaxDepth::distant ? 32.0f : 16.0f;
    return std::round(std::clamp(distance * fraction, -cap, cap) / 4.0f) * 4.0f;
}

// Independent destination oracle: no candidate camera/visible-rectangle helper,
// no object draw or culling for the changed atlas sprites. Cloud rectangle()
// contributes only its already-chosen frozen pose, not the candidate offset.
float parallax_atlas_offset(const Camera& camera, const Viewport& pane, float anchor,
    ParallaxDepth depth, const ParallaxPass& pass)
{
    if (pass.reduced || (depth != ParallaxDepth::distant && depth != ParallaxDepth::hill) ||
        (depth == ParallaxDepth::distant && !pass.distant) ||
        (depth == ParallaxDepth::hill && !pass.hills)) { return 0.0f; }
    const float centre = camera.translation.x + pane.width / camera.scale * 0.5f;
    const double raw = static_cast<double>(centre - anchor) /
        (depth == ParallaxDepth::distant ? 50.0 : 100.0);
    const double limit = depth == ParallaxDepth::distant ? 32.0 : 16.0;
    const double bounded = raw < -limit ? -limit : raw > limit ? limit : raw;
    const double magnitude = std::floor(std::abs(bounded) * 0.25 + 0.5) * 4.0;
    return static_cast<float>(bounded < 0.0 ? -magnitude : magnitude);
}

RectangleI parallax_source(const std::string& frame)
{
    if (frame == "sun") { return { 4, 360, 224, 224 }; }
    if (frame == "cloud_big") { return { 1136, 360, 256, 96 }; }
    if (frame == "cloud_small") { return { 1396, 360, 160, 64 }; }
    if (frame == "birds") { return { 1560, 360, 96, 32 }; }
    if (frame == "hill_wide") { return { 392, 4, 1536, 320 }; }
    if (frame == "hill_small") { return { 364, 360, 768, 192 }; }
    throw std::runtime_error("Unexpected parallax atlas role: " + frame);
}

ParallaxWorld parallax_world(Application& app, const LevelLoadedInfo& info,
    const LandmarkStage& stage, const GameTuning& tuning, PlaySettings& preferences,
    const TeamColour& colours, bool heavy)
{
    ParallaxWorld world;
    world.actual = landmark_world(app, info, stage, tuning, preferences, colours, heavy);
    world.anchor = info.zoom_out_finish_bounds_rectangle().center();
    world.preferences = &preferences;
    const bool course = std::string(stage.name) == "movement_course";
    const size_t cloud_count = course ? 5 : 6, hill_count = course ? 6 : 7;
    const size_t hill_begin = cloud_count + 4;
    const std::array<const char*, 6> cloud_frames{
        "cloud_big", "cloud_small", "cloud_big", "cloud_big", "cloud_small", "cloud_big" };
    const std::array<std::pair<const char*, const char*>, 8> fixed_roles{{
        { "house", "house" }, { "tree_1", "tree_round" }, { "tree_2", "tree_tall" },
        { "tree_3", "tree_round" }, { "tree_4", "tree_tall" },
        { "castle_background", "swatch_bg_mid" }, { "brush_pot", "brush_pot" }, { "tape_roll", "tape_roll" }
    }};
    const SpriteSheet* sheet = app.render_resources()->sprite_sheet("terrain");
    const JsonValue entries = info.non_collision_objects_json();
    require(entries.size() <= world.actual.scenery.size(), "Parallax scenery omitted authored entries");
    for (size_t index = 0; index < entries.size(); ++index)
    {
        const JsonValue entry = entries.at(index);
        const std::string name = entry.string("name"), frame = entry.string("frame_name");
        const JsonValue rectangle = entry.object("rectangle");
        ParallaxSprite sprite{ world.actual.scenery[index], ParallaxDepth::fixed,
            RectangleF(rectangle.number("x"), rectangle.number("y"), rectangle.number("width"), rectangle.number("height")), {} };
        require(entry.string("type") == "Visual" && entry.string("sheet_name") == "terrain" &&
            entry.string("colour") == "WHITE", "Parallax catalog no longer has plain WHITE terrain artwork");
        if (index == 0) { require(name == "paper" && frame == "swatch_paper", "Parallax paper order changed"); }
        else if (index == 1)
        { require(name == "sun" && frame == "sun", "Parallax sun role/order changed"); sprite.depth = ParallaxDepth::distant; }
        else if (index < cloud_count + 2)
        {
            require(name == "cloud_" + std::to_string(index - 1) && frame == cloud_frames[index - 2] && entry.has("drift") &&
                dynamic_cast<const DriftingVisual*>(sprite.object) != nullptr, "Parallax cloud role/order/phase type changed");
            sprite.depth = ParallaxDepth::distant;
        }
        else if (index < hill_begin)
        {
            require(name == "birds_" + std::to_string(index - cloud_count - 1) && frame == "birds", "Parallax bird role/order changed");
            sprite.depth = ParallaxDepth::distant;
        }
        else if (index < hill_begin + hill_count)
        {
            const size_t hill = index - hill_begin;
            require(name == "hill_" + std::to_string(hill + 1) && frame == (hill % 2 == 0 ? "hill_wide" : "hill_small") &&
                !entry.has("drift"), "Parallax shared hill group/order changed"); sprite.depth = ParallaxDepth::hill;
        }
        else
        {
            require(std::any_of(fixed_roles.begin(), fixed_roles.end(), [&](const auto& role)
                { return name == role.first && frame == role.second; }) && !entry.has("drift"),
                "Unclassified static scenery would silently enter the parallax prototype");
        }
        if (sprite.depth != ParallaxDepth::fixed)
        {
            require(entry.has("drift") || dynamic_cast<const Visual*>(sprite.object) != nullptr,
                "Parallax nondrifting role is no longer an ordinary Visual");
            const SpriteFrame& actual = sheet->sprite_frame(sheet->resolve_sprite_frame(frame));
            sprite.source = parallax_source(frame);
            require(actual.source_rectangle() == sprite.source && actual.origin() == Vector2F::ZERO &&
                sprite.anchor.width == static_cast<float>(sprite.source.width) &&
                sprite.anchor.height == static_cast<float>(sprite.source.height),
                "Parallax literal atlas/native-size assumptions changed");
        }
        world.sprites.push_back(sprite);
    }
    require(hill_begin + hill_count <= world.sprites.size(), "Parallax hill group is incomplete");
    return world;
}

Frame parallax_frame(Application& app, const ParallaxWorld& world, const ParallaxPass& pass, size_t& rendered)
{
    // Every comparison remains an ordinary gameplay draw. The separate
    // actual StagePreview/reference below selects miniature terrain.
    configure_miniature_fixture(*world.actual.scene, false);
    const auto before_modes = miniature_fixture_modes(*world.actual.scene);
    ParallaxPass visible = pass;
    if (world.preferences != nullptr && world.preferences->reduce_effects) { visible.reduced = true; }
    ++rendered; require(rendered <= 372, "Parallax prototype exceeded its approved readback budget");
    const auto before = landmark_world_state(world.actual);
    std::srand(21479); const int expected_random = std::rand(); std::srand(21479);
    Renderer& renderer = *app.renderer(); renderer.begin_frame();
    if (pass.scene) { world.actual.scene->draw(renderer); }
    else
    {
        renderer.set_view_count(world.actual.scene->view_count());
        const Canvas canvas(app.render_resources(), pass.foreground == 1 ? Colour::white : Colour::black);
        const SpriteSheet* sheet = app.render_resources()->sprite_sheet("terrain");
        for (int view = 0; view < world.actual.scene->view_count(); ++view)
        {
            const Scene::View& pane = world.actual.scene->view(view); DrawList list = renderer.view(view);
            list.set_viewport(pane.viewport); list.set_filter(TextureFilter::point);
            if (pass.foreground >= 0)
            {
                list.set_camera(Camera::DEFAULT_CAMERA); canvas.draw(list, RectangleF(0, 0, pane.viewport.width, pane.viewport.height));
            }
            list.set_camera(pane.camera);
            if (pass.prop >= 0)
            { world.actual.props[static_cast<size_t>(pass.prop)]->draw(list); continue; }
            for (size_t index = 0; index < world.actual.scenery.size(); ++index)
            {
                const GameObject* object = world.actual.scenery[index];
                const ParallaxSprite* sprite = index < world.sprites.size() ? &world.sprites[index] : nullptr;
                const ParallaxDepth depth = sprite != nullptr ? sprite->depth : ParallaxDepth::fixed;
                if (pass.foreground >= 0 && (index == 0 || depth != ParallaxDepth::fixed)) { continue; }
                if (pass.atlas && depth != ParallaxDepth::fixed)
                {
                    RectangleF rectangle = sprite->anchor;
                    if (const auto* cloud = dynamic_cast<const DriftingVisual*>(object)) { rectangle = cloud->rectangle(); }
                    rectangle.x += parallax_atlas_offset(pane.camera, pane.viewport, world.anchor.x, depth, visible);
                    list.set_camera(pane.camera); sheet->draw(list, sprite->source, rectangle, Colour::white);
                }
                else
                {
                    Camera camera = pane.camera;
                    camera.translation.x -= parallax_offset(pane.camera, pane.viewport, world.anchor, depth, visible);
                    list.set_camera(camera);
                    if (object->cull_bounds(1.0f / camera.scale).intersects(camera.visible_rectangle(pane.viewport))) { object->draw(list); }
                    list.set_camera(pane.camera);
                }
            }
            // Active-play reference: ordinary draws, never draw_miniature.
            list.set_camera(pane.camera);
            for (const CollisionObject* object : world.actual.scene->collision_objects())
            {
                if (object->cull_bounds(1.0f / pane.camera.scale).intersects(pane.camera.visible_rectangle(pane.viewport))) { object->draw(list); }
            }
        }
    }
    Frame result = read(renderer);
    require(before == landmark_world_state(world.actual) &&
        before_modes == miniature_fixture_modes(*world.actual.scene) && std::rand() == expected_random,
        "Parallax drawing mutated object geometry, phase, paint ownership or gameplay RNG");
    return result;
}

void parallax_views(Application& app, ParallaxWorld& world, const LevelLoadedInfo& info,
    const LandmarkStage& stage, const GameTuning& tuning, const Presentation& presentation, int count, int pose)
{
    const Vector2I resolution = app.resolution_manager()->resolution_ivec();
    require((count == 1 || count == 4) &&
        ((resolution.x == 1280 && resolution.y == 720) ||
            (resolution.x == 1920 && resolution.y == 1080)) &&
        presentation.camera().reference_height == 1080.0f &&
        presentation.camera().minimum_world_width == 1280.0f,
        "Parallax fixture changed its declared C2 camera profile or native layouts");
    app.viewport_manager()->set_layout(count == 1 ? ScreenLayout::one_player : ScreenLayout::four_player);
    world.actual.scene->clear_views(); const PlayerCamera framing{};
    for (int view = 0; view < count; ++view)
    {
        const Viewport pane = app.viewport_manager()->player_viewport(view);
        const RectangleI pixels = pane.pixel_rect();
        const int divisor = count == 1 ? 1 : 2;
        require(pixels.width == resolution.x / divisor && pixels.height == resolution.y / divisor &&
            pane.width == static_cast<float>(pixels.width) && pane.height == static_cast<float>(pixels.height),
            "Parallax fixture changed its exact whole-pixel gameplay pane");
        // Current C2 law: full-window height, limited by physical pane width.
        // Literal720p/1080p anchors are 1P2/3,1 and 4P1/2,3/4 respectively.
        const float scale = std::min(static_cast<float>(resolution.y) / 1080.0f,
            static_cast<float>(pixels.width) / 1280.0f);
        const float literal_scale = resolution.y == 720 ?
            (count == 1 ? 2.0f / 3.0f : 0.5f) : (count == 1 ? 1.0f : 0.75f);
        require(scale == literal_scale, "Parallax expected C2 scale differs from its literal native anchors");
        const bool right = view % 2 != 0;
        // Pose 0 is an authored spawn; pose 1 is explicitly placed near a
        // landmark at the same gameplay zoom. Neither proves route reachability.
        const Vector2F focus = pose == 0 ?
            (right ? info.team_b_spawns().front() : info.team_a_spawns().front()) + tuning.player().size / 2.0f :
            Vector2F(stage.props[right ? 1 : 0].center().x, stage.props[right ? 1 : 0].bottom() - tuning.player().size.y / 2.0f);
        // Seed the previous camera with this scale too: an obsolete seed can
        // bias the centre retained by the camera deadzone after a width clamp.
        const Camera previous(focus - pane.size() / (scale * 2.0f), scale);
        const Camera camera = framing.follow(focus, pane, static_cast<float>(resolution.y),
            presentation.camera(), previous, info.camera_bounds_rectangle());
        require(camera.scale == scale, "Parallax fixture changed the game's C2 height/pane-width camera scale");
        world.actual.scene->add_view(pane, camera);
    }
}
size_t parallax_preserved(const Frame& baseline, const Frame& first, const Frame& second,
    const Frame& black, const Frame& white, bool prop)
{
    size_t samples = 0;
    for (size_t pixel = 0; pixel < baseline.pixels.size(); pixel += 4)
    {
        // Equal black/white composites identify opaque ink; atlas holes and
        // partially transparent shadows never become bounding-box assertions.
        bool opaque = true, visible = true;
        for (size_t channel = 0; channel < 3; ++channel)
        {
            opaque = opaque && black.pixels[pixel + channel] == white.pixels[pixel + channel];
            visible = visible && baseline.pixels[pixel + channel] == black.pixels[pixel + channel];
        }
        if (!opaque || (prop && !visible)) { continue; }
        require(same_pixel(baseline, first, pixel) && same_pixel(baseline, second, pixel),
            "Parallax moved or repainted opaque static foreground/landmark pixels"); ++samples;
    }
    // A matching opaque prop colour is useful preservation evidence, but a
    // later same-colour occluder can match too. This is not unique contribution.
    require(samples > 0, "Parallax opaque foreground/prop-reference sample check was vacuous");
    return samples;
}

void parallax_cell(Frame& sheet, const Frame& frame, int column, int row, const RectangleI& crop)
{
    require(crop.width == 512 && crop.height == 360 && crop.x >= 0 && crop.y >= 0 &&
        crop.right() <= frame.width && crop.bottom() <= frame.height, "Parallax native contact crop exceeds its source");
    for (int y = 0; y < crop.height; ++y)
    {
        const size_t source = (static_cast<size_t>(crop.y + y) * static_cast<size_t>(frame.width) + static_cast<size_t>(crop.x)) * 4;
        const size_t target = (static_cast<size_t>(row * 360 + y) * static_cast<size_t>(sheet.width) + static_cast<size_t>(column * 512)) * 4;
        std::copy_n(frame.pixels.begin() + static_cast<std::ptrdiff_t>(source), static_cast<size_t>(crop.width) * 4,
            sheet.pixels.begin() + static_cast<std::ptrdiff_t>(target));
    }
}

void parallax_literal_math()
{
    const ParallaxPass both{ false, false, true, true };
    const Viewport pane(0, 0, 1280, 720); const Vector2F anchor(3000, 4800);
    const std::array<float, 13> distance{ -3000, -1600, -800, -600, -200, -100, 0, 100, 200, 600, 800, 1600, 3000 };
    const std::array<float, 13> distant_offsets{ -32, -32, -16, -12, -4, -4, 0, 4, 4, 12, 16, 32, 32 };
    const std::array<float, 13> hill{ -16, -16, -8, -8, -4, 0, 0, 0, 4, 8, 8, 16, 16 };
    for (size_t index = 0; index < distance.size(); ++index)
    {
        const Camera camera(anchor.x + distance[index] - 640.0f, -0.625f, 1.0f);
        require(parallax_offset(camera, pane, anchor, ParallaxDepth::distant, both) == distant_offsets[index] &&
            parallax_offset(camera, pane, anchor, ParallaxDepth::hill, both) == hill[index],
            "Parallax sign/cap/four-unit half-step differs from literal endpoints");
    }
}

void parallax_edges(Application& app, const Vector2F& anchor, size_t& rendered, std::ofstream& details, const char* stage)
{
    const SpriteSheet* atlas = app.render_resources()->sprite_sheet("terrain");
    for (const ParallaxDepth depth : { ParallaxDepth::distant, ParallaxDepth::hill })
    {
        const bool is_distant = depth == ParallaxDepth::distant;
        const char* frame = is_distant ? "sun" : "hill_wide"; const RectangleI source = parallax_source(frame);
        for (int edge = 0; edge < 2; ++edge)
        {
            const Viewport pane = app.viewport_manager()->fullscreen_viewport();
            const float cap = is_distant ? 32.0f : 16.0f, literal = edge == 0 ? cap : -cap;
            const Camera camera(edge == 0 ? anchor.x + 2400.375f : -0.375f, -0.625f, 1.0f);
            const RectangleF visible = camera.visible_rectangle(pane); const float entered = is_distant ? 20.25f : 12.25f;
            RectangleF destination(edge == 0 ? visible.left() + entered - literal - static_cast<float>(source.width) :
                visible.right() - entered - literal, 90.0f, static_cast<float>(source.width), static_cast<float>(source.height));
            ParallaxWorld isolated; isolated.anchor = anchor;
            isolated.actual.scene = std::make_unique<Scene>(app.thread_pool(), app.partitioner());
            GameObject* object = isolated.actual.scene->add(std::make_unique<Visual>("terrain", frame, destination, app.render_resources()));
            isolated.actual.scenery.push_back(object); isolated.sprites.push_back({ object, depth, destination, source });
            isolated.actual.scene->end_tick(); isolated.actual.scene->add_view(pane, camera);
            const ParallaxPass both{ false, false, true, true };
            require(parallax_offset(camera, pane, anchor, depth, both) == literal &&
                !object->cull_bounds(1.0f).intersects(visible), "Isolated parallax entry did not start outside the ordinary cull");
            const Frame actual = parallax_frame(app, isolated, both, rendered);
            ++rendered; require(rendered <= 372, "Parallax edge reference exceeded readback budget");
            Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
            DrawList list = renderer.view(0); list.set_viewport(pane); list.set_camera(camera); list.set_filter(TextureFilter::point);
            destination.x += literal; atlas->draw(list, source, destination, Colour::white);
            const Frame expected = read(renderer);
            size_t ink = 0;
            for (size_t pixel = 0; pixel < expected.pixels.size(); pixel += 4)
            {
                if (!std::equal(expected.pixels.begin() + static_cast<std::ptrdiff_t>(pixel),
                    expected.pixels.begin() + static_cast<std::ptrdiff_t>(pixel + 4), expected.pixels.begin())) { ++ink; }
            }
            require(actual.pixels == expected.pixels && ink > 0, "Fractional/negative parallax entry differs from literal unculled atlas or is blank");
            details << "edge," << stage << ',' << (is_distant ? "far" : "hill") << ',' << edge << ',' << camera.translation.x << ',' <<
                camera.translation.y << ',' << literal << ',' << ink << '\n';
        }
    }
}

void parallax_preview(Application& app, Registry<LevelLoadedInfo>& levels, const ParallaxWorld& clean,
    const LandmarkStage& stage, const GameTuning& tuning, const Presentation& presentation,
    const std::filesystem::path& output, size_t& rendered)
{
    const NestedSaveProfile profile(output / "parallax-preview-profile");
    Save saved; SettingsSession saved_session(saved); GameData data; data.set_application(&app); data.set_save(&saved); data.set_settings_session(&saved_session); const StageCatalog stage_catalog = acceptance::catalog_of(levels, {}, { "close_quarters", "movement_course" }); data.set_stage_catalog(&stage_catalog);
    data.set_tuning(&tuning); data.set_presentation(&presentation);
    StagePreview preview("parallax-static-preview", { 960, 270, 960, 540 }, &data); preview.set_stage(stage.name);
    Renderer& renderer = *app.renderer();
    ++rendered; renderer.begin_frame(); renderer.set_view_count(1); DrawList actual = renderer.view(0);
    actual.set_filter(TextureFilter::point); preview.draw(actual); const Frame production = read(renderer);
    ++rendered; renderer.begin_frame(); renderer.set_view_count(1); DrawList expected = renderer.view(0);
    expected.set_filter(TextureFilter::point); const Viewport pane(640, 180, 640, 360);
    const Camera camera = Camera::frame(levels.get(stage.name)->zoom_out_finish_bounds_rectangle(), pane);
    draw_legacy_miniature_view(expected, camera, pane, clean.actual.scenery, clean.actual.scene->collision_objects());
    require(production.pixels == read(renderer).pixels, "Comparison fixture changed actual StagePreview's original static miniature path");
}

void parallax_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const GameTuning& tuning,
    const Presentation& presentation, PlaySettings& preferences, const std::filesystem::path& output)
{
    const RestoreFlag restore{ preferences.reduce_effects, preferences.reduce_effects };
    parallax_literal_math(); preferences.reduce_effects = false;
    const ParallaxPass native{ true }, zero{}, distant{ false, false, true }, both{ false, false, true, true };
    const ParallaxPass atlas_far{ false, true, true }, atlas_both{ false, true, true, true };
    const ParallaxPass mask_black{ false, false, false, false, false, 0 }, mask_white{ false, false, false, false, false, 1 };
    std::ofstream records(output / "parallax-comparison.csv"), details(output / "parallax-extra.csv"), counts(output / "parallax-counts.csv");
    // Cull-candidate counts are conservative, especially for cloud excursions.
    // Each view row repeats framewide A/static and B/A deltas; totals below
    // accumulate those deltas once per primary case, not once per view.
    records << "stage,coverage,players,pose,view,camera_x,camera_y,scale,far_offset,hill_offset,far_cull_candidates,hill_cull_candidates,frame_static_to_far_changed,frame_far_to_both_changed,opaque_foreground_samples\n";
    details << "check,stage,role,case,x,y,offset,samples\n";
    size_t rendered = 0, cases = 0, captures = 0, global_far = 0, global_hill = 0, opaque_props = 0;
    const TeamColour colours = TeamColourTools::team_colours(TeamColourPair::blue_orange, presentation.team_palette());
    for (const LandmarkStage& stage : landmark_stages)
    {
        const LevelLoadedInfo& info = *levels.get(stage.name);
        for (const bool heavy : { false, true })
        {
            std::srand(7319);
            ParallaxWorld world = parallax_world(app, info, stage, tuning, preferences, colours, heavy);
            Frame contact{ 1536, 720, std::vector<unsigned char>(static_cast<size_t>(1536 * 720 * 4), 255) };
            Frame return_static, return_both;
            for (const int players : { 1, 4 })
            {
                for (int pose = 0; pose < 2; ++pose)
                {
                    parallax_views(app, world, info, stage, tuning, presentation, players, pose);
                    const Frame baseline = parallax_frame(app, world, native, rendered);
                    require(baseline.pixels == parallax_frame(app, world, zero, rendered).pixels,
                        "Explicit comparison traversal at factor zero differs from ordinary active Scene");
                    const Frame a = parallax_frame(app, world, distant, rendered);
                    require(a.pixels == parallax_frame(app, world, atlas_far, rendered).pixels,
                        "Far prototype differs from independent unculled atlas/destination reference");
                    const Frame b = parallax_frame(app, world, both, rendered);
                    require(b.pixels == parallax_frame(app, world, atlas_both, rendered).pixels,
                        "Far/hill prototype differs from independent unculled atlas/destination reference");
                    const Frame black = parallax_frame(app, world, mask_black, rendered), white = parallax_frame(app, world, mask_white, rendered);
                    const size_t protected_pixels = parallax_preserved(baseline, a, b, black, white, false);
                    const size_t far_changed = miniature_changed_pixels(baseline, a), both_changed = miniature_changed_pixels(a, b);
                    global_far += far_changed; global_hill += both_changed; ++cases;
                    for (int view = 0; view < players; ++view)
                    {
                        const Scene::View& pane = world.actual.scene->view(view); size_t visible_far = 0, visible_hills = 0;
                        for (const ParallaxSprite& sprite : world.sprites)
                        {
                            Camera camera = pane.camera; camera.translation.x -= parallax_offset(camera, pane.viewport, world.anchor, sprite.depth, both);
                            if (!sprite.object->cull_bounds(1.0f / camera.scale).intersects(camera.visible_rectangle(pane.viewport))) { continue; }
                            if (sprite.depth == ParallaxDepth::distant) { ++visible_far; }
                            if (sprite.depth == ParallaxDepth::hill) { ++visible_hills; }
                        }
                        records << stage.name << ',' << (heavy ? "heavy" : "clean") << ',' << players << ',' << pose << ',' << view << ',' <<
                            pane.camera.translation.x << ',' << pane.camera.translation.y << ',' << pane.camera.scale << ',' <<
                            parallax_offset(pane.camera, pane.viewport, world.anchor, ParallaxDepth::distant, both) << ',' <<
                            parallax_offset(pane.camera, pane.viewport, world.anchor, ParallaxDepth::hill, both) << ',' << visible_far << ',' << visible_hills << ',' <<
                            far_changed << ',' << both_changed << ',' << protected_pixels << '\n';
                    }
                    if ((players == 1 && pose == 0) || (players == 4 && pose == 1))
                    {
                        const int row = players == 1 ? 0 : 1;
                        const int top = std::string(stage.name) == "close_quarters" ? 120 : 360;
                        const RectangleI crop = players == 1 ? RectangleI(768, top, 512, 360) : RectangleI(0, 0, 512, 360);
                        parallax_cell(contact, baseline, 0, row, crop); parallax_cell(contact, a, 1, row, crop); parallax_cell(contact, b, 2, row, crop);
                        details << "capture," << stage.name << ',' << (heavy ? "heavy" : "clean") << ',' << players << ',' << crop.x << ',' << crop.y << ",0," << pose << '\n';
                    }
                    if (players == 4 && pose == 1)
                    {
                        require(b.pixels == parallax_frame(app, world, both, rendered).pixels, "Frozen prototype repeat changed its raster");
                        return_static = baseline; return_both = b;
                        if (!heavy)
                        {
                            for (int prop = 0; prop < 2; ++prop)
                            {
                                ParallaxPass prop_black = mask_black, prop_white = mask_white; prop_black.prop = prop_white.prop = prop;
                                const Frame ink_black = parallax_frame(app, world, prop_black, rendered), ink_white = parallax_frame(app, world, prop_white, rendered);
                                const size_t samples = parallax_preserved(baseline, a, b, ink_black, ink_white, true); opaque_props += samples;
                                details << "matching-opaque-prop-reference," << stage.name << ',' << landmark_names[static_cast<size_t>(prop)] << ",0,0,0,0," << samples << '\n';
                            }
                        }
                    }
                }
            }
            save(contact, output / (std::string("parallax-") + stage.name + (heavy ? "-heavy.ppm" : "-clean.ppm"))); ++captures;
            if (!heavy)
            {
                parallax_edges(app, world.anchor, rendered, details, stage.name);
                parallax_preview(app, levels, world, stage, tuning, presentation, output, rendered);
            }
            else
            {
                for (const Vector2I size : { Vector2I(1920, 1080), Vector2I(1280, 720) })
                {
                    app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y);
                    parallax_views(app, world, info, stage, tuning, presentation, 4, 1);
                    const Frame baseline = parallax_frame(app, world, native, rendered), b = parallax_frame(app, world, both, rendered);
                    require(baseline.pixels == parallax_frame(app, world, zero, rendered).pixels &&
                        b.pixels == parallax_frame(app, world, atlas_both, rendered).pixels,
                        "Parallax resize changed Scene parity or independent atlas placement");
                    if (size.x == 1280) { require(baseline.pixels == return_static.pixels && b.pixels == return_both.pixels, "Parallax resize return changed pixels"); }
                    details << "resize," << stage.name << ",heavy,4," << size.x << ',' << size.y << ",0,0\n";
                }
                const auto held_phase = landmark_world_state(world.actual);
                preferences.reduce_effects = true;
                const Frame reduced_static = parallax_frame(app, world, native, rendered);
                require(reduced_static.pixels == parallax_frame(app, world, both, rendered).pixels &&
                    reduced_static.pixels == parallax_frame(app, world, atlas_both, rendered).pixels,
                    "Reduce effects did not retain exact static parallax plus the cloud's own anchor policy");
                preferences.reduce_effects = false;
                require(held_phase == landmark_world_state(world.actual), "Reduce effects drawing changed the held cloud phase or world state");
                world.actual.scene->clear_views(); const Viewport pane = app.viewport_manager()->fullscreen_viewport();
                const float scale = 720.0f / presentation.camera().reference_height;
                world.actual.scene->add_view(pane, Camera(world.anchor - pane.size() / (scale * 2.0f), scale));
                require(parallax_frame(app, world, native, rendered).pixels == parallax_frame(app, world, both, rendered).pixels,
                    "Overview-centre reference camera should have zero parallax displacement");
            }
        }
    }
    require(cases == 32 && captures == 8 && rendered == 372 && opaque_props > 0,
        "Parallax comparison matrix/capture/readback accounting changed");
    counts << "primary_cases,edge_cases,prop_masks,resize_states,reduced_cases,anchor_cases,preview_cases,frozen_repeats,readbacks,composites,far_changed_pixels,hill_changed_pixels,matching_opaque_prop_reference_samples\n" <<
        cases << ",16,16,8,4,4,4,8," << rendered << ',' << captures << ',' << global_far << ',' << global_hill << ',' << opaque_props << '\n';
    require(records.good() && details.good() && counts.good(), "Parallax comparison evidence write failed");
    std::cout << "PASS parallax comparison only:32 current-arena clean/heavy gameplay-camera cases,16 literal clipped edges,372 readbacks,8 native512x360-cell contact sheets; far changed=" <<
        global_far << " hill changed=" << global_hill << ". No shipped parallax, route/readability/performance acceptance.\n";
}
