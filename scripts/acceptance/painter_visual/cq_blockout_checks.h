// Included inside painter_visual.cpp's private namespace after painter_fixtures.h.
// A bounded inspection of the first CQ blockout, using real builder-owned
// Levels and the actual StagePreview. These hidden native captures support
// layout review; attended gameplay, readability and balance remain separate.

RectangleF cq_blockout_rectangle(const LevelLoadedInfo& info, const char* name)
{
    const JsonValue objects = info.collision_objects_json();
    for (size_t index = 0; index < objects.size(); ++index)
    {
        const JsonValue object = objects.at(index);
        if (object.string("name") != name) { continue; }
        const JsonValue box = object.object("rectangle");
        return { box.number("x"), box.number("y"), box.number("width"), box.number("height") };
    }
    throw std::runtime_error(std::string("CQ capture requires authored surface '") + name + "'");
}

Frame cq_blockout_crop(const Frame& frame, const RectangleI& box)
{
    require(box.width > 0 && box.height > 0 && box.x >= 0 && box.y >= 0 &&
        box.right() <= frame.width && box.bottom() <= frame.height,
        "CQ native crop escaped its captured frame");
    Frame crop{ box.width, box.height, {} };
    for (int y = box.y; y < box.bottom(); ++y)
    {
        const size_t start = (static_cast<size_t>(y) * static_cast<size_t>(frame.width) +
            static_cast<size_t>(box.x)) * 4;
        crop.pixels.insert(crop.pixels.end(), frame.pixels.begin() + start,
            frame.pixels.begin() + start + static_cast<size_t>(box.width) * 4);
    }
    return crop;
}

RectangleI cq_blockout_projected_crop(const RectangleF& world, const Camera& camera,
    const Viewport& viewport)
{
    const RectangleI pane = viewport.pixel_rect();
    RectangleF projected = camera.calculate_view_rectangle(world);
    projected.offset(Vector2F(pane.position()));
    const int left = std::max(pane.x, static_cast<int>(std::floor(projected.x)));
    const int top = std::max(pane.y, static_cast<int>(std::floor(projected.y)));
    const int right = std::min(pane.right(), static_cast<int>(std::ceil(projected.right())));
    const int bottom = std::min(pane.bottom(), static_cast<int>(std::ceil(projected.bottom())));
    require(right > left && bottom > top, "CQ authored context is outside its native pane");
    return { left, top, right - left, bottom - top };
}

Frame cq_blockout_frozen_level(Application& app, const Level& level, size_t& readbacks)
{
    const auto before = observation(level);
    const int collisions = level.collision_object_count(), projectiles = level.projectile_count();
#if COLOURWARS_DEV_TOOLS
    const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
    std::srand(17102026); const int expected_random = std::rand(); std::srand(17102026);
    ++readbacks; const Frame first = render(level, *app.renderer());
    ++readbacks; const Frame repeat = render(level, *app.renderer());
    require(first.width == 1280 && first.height == 720 && first.pixels == repeat.pixels &&
        before == observation(level) && collisions == level.collision_object_count() &&
        projectiles == level.projectile_count() && std::rand() == expected_random,
        "CQ frozen native draw changed pixels, observed match state, object counts or gameplay RNG");
#if COLOURWARS_DEV_TOOLS
    require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
        "CQ frozen native draw changed the Debug replay fingerprint");
#endif
    return first;
}

void cq_blockout_matrix(Application& app, Registry<LevelLoadedInfo>& levels, LevelBuilder& builder,
    const Canvas& canvas, const GameTuning& tuning, const Presentation& presentation,
    PlaySettings& preferences, const std::filesystem::path& output)
{
    const RestoreFlag restore{ preferences.reduce_effects, preferences.reduce_effects };
    preferences.reduce_effects = false;
    const LevelLoadedInfo& info = *levels.get("close_quarters");
    const RectangleF main = cq_blockout_rectangle(info, "lower_paint");
    const RectangleF upper = cq_blockout_rectangle(info, "upper_paint");
    const RectangleF floor = cq_blockout_rectangle(info, "boundary_floor");
    const RectangleF cover = cq_blockout_rectangle(info, "centre_cover");
    const RectangleF overview = info.zoom_out_finish_bounds_rectangle();
    const Vector2F body_size = tuning.player().size;
    for (const auto& team : { info.team_a_spawns(), info.team_b_spawns() })
    {
        require(!team.empty(), "CQ native inspection needs both authored spawn groups");
        for (const Vector2F& spawn : team)
        {
            require(overview.contains(RectangleF(spawn, body_size)),
                "CQ authored overview clips a spawn and cannot show the complete blockout");
        }
    }
    require(main.y < floor.y && upper.y < main.y &&
        floor.y - main.bottom() >= body_size.y && cover.bottom() <= main.y,
        "CQ capture no longer has its authored main/upper/lower-bypass/cover arrangement");

    std::ofstream records(output / "cq-blockout.csv");
    records << "capture,source,route,view,input_slot,frame_width,frame_height,player_x,player_y,"
        "camera_x,camera_y,camera_scale,crop_x,crop_y,crop_width,crop_height,"
        "frozen_pixels,public_state,debug_fingerprint,rng,evidence_scope\n";
    size_t readbacks = 0, full_frames = 0, crops = 0, updates = 0;
    const auto record = [&](const char* stem, const char* source, const char* route, int view,
        int slot, const Vector2F& position, const Camera& camera, const RectangleI& crop, bool match)
    {
        records << stem << ',' << source << ',' << route << ',' << view + 1 << ',' << slot <<
            ",1280,720," << position.x << ',' << position.y << ',' << camera.translation.x << ',' <<
            camera.translation.y << ',' << camera.scale << ',' << crop.x << ',' << crop.y << ',' <<
            crop.width << ',' << crop.height << ",1," << (match ? "1" : "static-widget-bounds") << ','
#if COLOURWARS_DEV_TOOLS
            << (match ? "1" : "not-applicable-widget")
#else
            << "not-available-in-Release"
#endif
            << ",1,hidden-native-inspection\n";
    };
    const auto build_cq = [&](int count, float duration, WeaponType solo_tool)
    {
        MatchSetup settings; settings.set_stage("close_quarters");
        settings.set_player_count(count); settings.set_duration(duration);
        settings.set_screen_layout(count == 1 ? ScreenLayout::one_player : ScreenLayout::four_player);
        constexpr std::array<int, 4> slots{ 3, 1, 2, 0 };
        constexpr std::array<WeaponType, 4> tools{ WeaponType::sprayer, WeaponType::sniper,
            WeaponType::roller, WeaponType::bucket };
        settings.begin_input_claims(slots[0]);
        for (int view = 1; view < count; ++view)
        { require(settings.claim_input_slot(slots[static_cast<size_t>(view)]), "CQ native roster claim failed"); }
        for (int view = 0; view < count; ++view)
        {
            settings.set_player_team(view, view % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
            settings.set_player_weapon(view, count == 1 ? solo_tool : tools[static_cast<size_t>(view)]);
        }
        app.viewport_manager()->set_layout(settings.screen_layout());
        auto level = builder.build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
        resize(app, *level, { 1280, 720 });
        require(level->player_count() == count, "CQ native builder changed the requested roster");
        for (int view = 0; view < count; ++view)
        { require(level->player(view).player_num() == slots[static_cast<size_t>(view)], "CQ native roster lost its sparse slot mapping"); }
        return level;
    };
    const auto inputs = neutral_inputs();
    const auto step = [&](Level& level)
    {
        require(++updates <= 1800, "CQ focused native inspection exceeded its bounded update budget");
        level.update(acceptance::slot_seats(level, inputs), dt);
    };
    const auto active = [&](Level& level)
    {
        for (int tick = 0; tick < 600 && level.state() != LevelState::active; ++tick) { step(level); }
        require(level.state() == LevelState::active, "CQ native match never reached active play");
        for (int tick = 0; tick < 90; ++tick) { step(level); }
    };
    const Vector2F main_position(cover.x - body_size.x - 100.0f, main.y - body_size.y);
    const Vector2F lower_position(floor.center().x - body_size.x / 2.0f, floor.y - body_size.y);
    const Vector2F upper_position(upper.center().x - body_size.x / 2.0f, upper.y - body_size.y);
    const RectangleF main_context(cover.x - 400.0f, upper.y - 100.0f,
        cover.width + 800.0f, floor.y - upper.y + 180.0f);
    const RectangleF lower_context(lower_position.x - 500.0f, main.y - 80.0f,
        body_size.x + 1000.0f, floor.y - main.y + 160.0f);
    const auto capture_live = [&](Level& level, const char* stem, const std::vector<const char*>& routes,
        const std::vector<RectangleF>& contexts)
    {
        require(level.state() == LevelState::active && routes.size() == static_cast<size_t>(level.player_count()) &&
            contexts.size() == routes.size(), "CQ live inspection lost its active route roster");
        const Frame frame = cq_blockout_frozen_level(app, level, readbacks);
        save(frame, output / (std::string(stem) + ".ppm")); ++full_frames;
        const auto before = observation(level);
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Player& player = level.player(view);
            const Viewport pane = app.viewport_manager()->player_viewport(view);
            require(player.state() == PlayerState::alive, "CQ route inspection contains a dead player");
            ++readbacks; const Frame blank = isolated(nullptr, player, pane, *app.renderer(), canvas);
            ++readbacks; const Frame figure = isolated(&player, player, pane, *app.renderer(), canvas);
            Frame figure_crop{};
            require(check_bounds(figure, blank, player, pane, figure_crop) > 0,
                "CQ route player is absent or escaped its established native raster bounds");
            const RectangleI crop = cq_blockout_projected_crop(contexts[static_cast<size_t>(view)],
                level.view_camera(view), pane);
            save(cq_blockout_crop(frame, crop), output / (std::string(stem) + "-" +
                routes[static_cast<size_t>(view)] + "-native.ppm")); ++crops;
            record(stem, "actual-Level", routes[static_cast<size_t>(view)], view, player.player_num(),
                player.rectangle().position(), level.view_camera(view), crop, true);
        }
        require(before == observation(level), "CQ isolated native raster checks changed observed match state");
    };

    {
        auto level = build_cq(1, 30.0f, WeaponType::sprayer); active(*level);
        require(level->place_player(0, main_position), "CQ cannot place the main-contest inspector");
        for (int tick = 0; tick < 18; ++tick) { step(*level); }
        capture_live(*level, "hidden-inspection-cq-1p-main-720p", { "main-contest" }, { main_context });
    }
    {
        auto level = build_cq(1, 30.0f, WeaponType::mister); active(*level);
        require(level->place_player(0, lower_position), "CQ cannot place the lower-bypass inspector");
        for (int tick = 0; tick < 18; ++tick) { step(*level); }
        capture_live(*level, "hidden-inspection-cq-1p-lower-720p", { "lower-bypass" }, { lower_context });
    }
    {
        auto level = build_cq(4, 30.0f, WeaponType::none); active(*level);
        const std::array<Vector2F, 4> positions{ main_position, lower_position, upper_position,
            info.team_b_spawns().front() };
        for (int view = 0; view < level->player_count(); ++view)
        { require(level->place_player(view, positions[static_cast<size_t>(view)]), "CQ cannot distribute its native route inspectors"); }
        for (int tick = 0; tick < 18; ++tick) { step(*level); }
        std::vector<RectangleF> contexts;
        for (int view = 0; view < level->player_count(); ++view)
        { contexts.push_back(level->view_camera(view).visible_rectangle(app.viewport_manager()->player_viewport(view))); }
        capture_live(*level, "hidden-inspection-cq-4p-routes-720p",
            { "main-contest", "lower-bypass", "upper-route", "spawn-b" }, contexts);
    }
    {
        auto level = build_cq(1, 0.75f, WeaponType::sprayer);
        for (int tick = 0; tick < 600 && level->state() != LevelState::overview; ++tick) { step(*level); }
        require(level->state() == LevelState::overview, "CQ short solo match did not reach its actual runtime overview");
        const Frame frame = cq_blockout_frozen_level(app, *level, readbacks);
        const Camera camera = Camera::frame(overview, app.viewport_manager()->fullscreen_viewport());
        const RectangleI crop = cq_blockout_projected_crop(overview, camera, app.viewport_manager()->fullscreen_viewport());
        save(frame, output / "hidden-inspection-cq-runtime-overview-720p.ppm"); ++full_frames;
        save(cq_blockout_crop(frame, crop), output / "hidden-inspection-cq-runtime-overview-native.ppm"); ++crops;
        record("hidden-inspection-cq-runtime-overview-720p", "actual-Level-overview", "whole-stage-and-spawns",
            0, level->player(0).player_num(), level->player(0).rectangle().position(), camera, crop, true);
    }
    {
        Save saved; GameData data; data.set_application(&app); data.set_save(&saved);
        data.set_level_infos(&levels); data.set_tuning(&tuning); data.set_presentation(&presentation);
        const StageSelectPresentation& look = presentation.main_menu().stage_select;
        const RectangleF box(look.preview_position, look.preview_size);
        StagePreview preview("cq-blockout-actual-preview", box, &data); preview.set_stage("close_quarters");
        const Viewport fullscreen = app.viewport_manager()->fullscreen_viewport();
        const Camera menu_camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), fullscreen);
        const auto draw = [&](bool visible)
        {
            Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
            DrawList list = renderer.view(0); list.set_viewport(fullscreen); list.set_camera(menu_camera);
            list.set_filter(TextureFilter::point); canvas.draw(list, RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION));
            if (visible) { preview.draw(list); }
            ++readbacks; return read(renderer);
        };
        std::srand(172017); const int expected_random = std::rand(); std::srand(172017);
        const Frame frame = draw(true);
        require(frame.width == 1280 && frame.height == 720 && frame.pixels == draw(true).pixels &&
            std::rand() == expected_random && preview.bounds() == box,
            "CQ actual StagePreview changed frozen pixels, authored bounds or gameplay RNG");
        const RectangleI crop = cq_blockout_projected_crop(box, menu_camera, fullscreen);
        require(pane_delta(frame, draw(false), crop) > 100,
            "CQ actual StagePreview supplied no visible native miniature");
        save(frame, output / "hidden-inspection-cq-actual-stage-preview-720p.ppm"); ++full_frames;
        save(cq_blockout_crop(frame, crop), output / "hidden-inspection-cq-actual-stage-preview-native.ppm"); ++crops;
        record("hidden-inspection-cq-actual-stage-preview-720p", "actual-StagePreview-isolated", "menu-miniature",
            0, -1, Vector2F::ZERO, Camera::frame(overview, Viewport(crop)), crop, false);
    }
    require(full_frames == 5 && crops == 8 && readbacks == 23 && records.good(),
        "CQ focused capture set exceeded or missed its five-frame/eight-crop/twenty-three-readback budget");
    std::ofstream scope(output / "cq-blockout-scope.txt");
    scope << "Hidden native D3D11 inspection of the authored Close Quarters first blockout at 1280x720.\n"
        "Five full frames and eight crops retain native pixels; no crop is enlarged.\n"
        "Actual Level: 1P main contest, 1P lower bypass, 4P main/lower/upper/spawn routes, solo runtime overview.\n"
        "Actual StagePreview: isolated widget at the current stage menu's authored miniature dimensions.\n"
        "Frozen repeated draws preserve pixels, observed match state, object counts and gameplay RNG; Debug also checks replay fingerprints.\n"
        "Players are deliberately placed for composition inspection; this does not establish route traversal, human readability or balance acceptance.\n"
        "Attended gameplay and all-five-tool comparison remain part of iteration with the author.\n"
        "Updates: " << updates << "; readbacks: " << readbacks << ".\n";
    require(scope.good(), "CQ capture scope record write failed");
    std::cout << "PASS CQ-only hidden native 720p inspection: actual runtime overview, main/lower 1P, distributed 4P, "
        "actual StagePreview; five full frames/eight native crops; frozen state/fingerprint/RNG; "
        "attended gameplay/readability/balance acceptance remains open\n";
}
