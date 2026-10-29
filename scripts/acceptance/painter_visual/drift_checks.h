// Included inside painter_visual.cpp's private namespace. The isolated real
// cloud frame separates backdrop movement from player/camera/clock animation;
// Scene-versus-direct rasters additionally exercise the engine's actual cull.

Frame drift_scene_frame(Application& app, const Scene& scene)
{
    app.renderer()->begin_frame();
    scene.draw(*app.renderer());
    return read(*app.renderer());
}

Frame drift_direct_frame(Application& app, const Scene& scene, const Canvas& canvas,
    const GameObject* object = nullptr)
{
    Renderer& renderer = *app.renderer();
    renderer.begin_frame();
    renderer.set_view_count(scene.view_count());
    for (int view = 0; view < scene.view_count(); ++view)
    {
        const Scene::View& pane = scene.view(view);
        DrawList list = renderer.view(view);
        list.set_viewport(pane.viewport);
        list.set_camera(pane.camera);
        canvas.draw(list, RectangleF(-100000, -100000, 200000, 200000));
        if (object != nullptr) { object->draw(list); }
    }
    return read(renderer);
}

Frame drift_frozen_frame(Application& app, const Scene& scene, const DriftingVisual& cloud)
{
    const RectangleF rectangle = cloud.rectangle();
    const RectangleF bounds = cloud.bounds();
    std::srand(7153);
    const int expected_random = std::rand();
    std::srand(7153);
    const Frame first = drift_scene_frame(app, scene);
    for (int repeat = 0; repeat < 3; ++repeat)
    {
        require(first.pixels == drift_scene_frame(app, scene).pixels,
            "frozen cloud Scene redraw changed pixels");
    }
    require(std::rand() == expected_random, "cloud Scene drawing consumed match RNG");
    require(rectangle == cloud.rectangle() && bounds == cloud.bounds(),
        "drawing changed cloud geometry");
    return first;
}

void drift_views(Application& app, Scene& scene, int count, const RectangleF& anchor,
    int edge = 0)
{
    scene.clear_views();
    for (int view = 0; view < count; ++view)
    {
        const Viewport pane = app.viewport_manager()->player_viewport(view);
        const float scale = pane.width / 1280.0f;
        Camera camera(Vector2F(17.25f * static_cast<float>(view), 0), scale);
        if (edge > 0) { camera.translation.x = anchor.right() + 4; }
        if (edge < 0) { camera.translation.x = anchor.x - pane.width / scale - 4; }
        scene.add_view(pane, camera);
    }
}

void drift_raster_bounds(const Frame& frame, const Frame& blank, const Scene& scene,
    const DriftingVisual& cloud, const std::filesystem::path& output,
    const std::string& stem, std::ofstream& records)
{
    for (int view = 0; view < scene.view_count(); ++view)
    {
        const Scene::View& pane = scene.view(view);
        Frame crop{};
        const size_t changed = check_projected_bounds(frame, blank,
            pane.camera.calculate_view_rectangle(cloud.cull_bounds(1 / pane.camera.scale)),
            pane.viewport, crop);
        require(changed > 0, "real cloud frame was absent from an expected visible pane");
        save(crop, output / (stem + "-view" + std::to_string(view + 1) + "-detail.ppm"));
        const RectangleF rectangle = cloud.rectangle();
        records << stem << ',' << view + 1 << ',' << changed << ',' << rectangle.x << ','
            << rectangle.y << ',' << rectangle.width << ',' << rectangle.height << '\n';
    }
}

void drift_fixture_matrix(Application& app, const Canvas& canvas,
    PlaySettings& preferences, const std::filesystem::path& output, std::ofstream& records)
{
    const RectangleF anchor(512, 192, 256, 96);
    for (const int count : { 1, 4 })
    {
        app.viewport_manager()->set_layout(count == 1 ? ScreenLayout::one_player : ScreenLayout::four_player);
        Frame initial_720{};
        for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
        {
            app.resolution_manager()->set_resolution_exactly(size);
            std::ignore = app.renderer()->window_size_changed(size.x, size.y);
            const std::string stem = "drift-" + std::to_string(count) + "p-" + std::to_string(size.x);
            Scene scene(app.thread_pool(), app.partitioner());
            scene.add(std::make_unique<Visual>("sprite_sheet_1", "pixel",
                RectangleF(-100000, -100000, 200000, 200000), app.render_resources(),
                Colour(0.72f, 0.75f, 0.78f)));
            DriftingVisual* cloud = scene.add(std::make_unique<DriftingVisual>(
                "terrain", "cloud_big", anchor, app.render_resources(), &preferences,
                Colour::white, 48.0f, 48.0f, 0.0f));
            scene.end_tick();
            drift_views(app, scene, count, anchor);
            const Frame blank = drift_direct_frame(app, scene, canvas);
            const Visual static_cloud("terrain", "cloud_big", anchor, app.render_resources());
            const Frame initial = drift_frozen_frame(app, scene, *cloud);
            if (size.x == 1280) { initial_720 = initial; }
            require(initial.pixels == drift_direct_frame(app, scene, canvas, &static_cloud).pixels,
                "zero-phase cloud differs from the original static cloud art");
            save(initial, output / (stem + "-anchor.ppm"));

            std::srand(9527);
            const int expected_update_random = std::rand();
            std::srand(9527);
            scene.update(12);
            require(std::rand() == expected_update_random, "cloud update consumed match RNG");
            const Frame right = drift_frozen_frame(app, scene, *cloud);
            require(right.pixels != initial.pixels && cloud->rectangle().x == anchor.x + 48,
                "cloud quarter-period update did not visibly move the actual frame right");
            drift_raster_bounds(right, blank, scene, *cloud, output, stem + "-right", records);
            save(right, output / (stem + "-right.ppm"));
            preferences.reduce_effects = true;
            require(drift_frozen_frame(app, scene, *cloud).pixels == initial.pixels,
                "applying Reduce effects did not immediately restore the authored cloud anchor");
            scene.update(7);
            require(drift_frozen_frame(app, scene, *cloud).pixels == initial.pixels,
                "reduced-effects cloud retained visible movement");
            preferences.reduce_effects = false;
            require(drift_frozen_frame(app, scene, *cloud).pixels == right.pixels,
                "reduced-effects time advanced the cloud phase or lost its retained position");
            for (const float invalid : { 0.0f, -1.0f,
                std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity() })
            {
                scene.update(invalid);
                require(drift_frozen_frame(app, scene, *cloud).pixels == right.pixels,
                    "nonpositive or nonfinite time changed cloud pixels");
            }

            // The authored rectangle lies entirely beyond the view; only the
            // moved silhouette enters it. A direct draw is an independent
            // reference that bypasses Scene culling.
            drift_views(app, scene, count, anchor, 1);
            const Frame right_edge_blank = drift_direct_frame(app, scene, canvas);
            require(drift_direct_frame(app, scene, canvas, &static_cloud).pixels == right_edge_blank.pixels,
                "right-edge cloud anchor was not wholly outside the view");
            const Frame right_edge = drift_frozen_frame(app, scene, *cloud);
            require(right_edge.pixels == drift_direct_frame(app, scene, canvas, cloud).pixels,
                "Scene culled cloud pixels that had drifted into the left edge");
            drift_raster_bounds(right_edge, right_edge_blank, scene, *cloud, output,
                stem + "-right-edge", records);

            scene.update(24);
            require(cloud->rectangle().x == anchor.x - 48,
                "cloud three-quarter period did not reach the left extreme");
            drift_views(app, scene, count, anchor);
            const Frame left = drift_frozen_frame(app, scene, *cloud);
            require(left.pixels != right.pixels && left.pixels != initial.pixels,
                "opposite cloud extremes did not produce distinct native rasters");
            drift_raster_bounds(left, blank, scene, *cloud, output, stem + "-left", records);
            save(left, output / (stem + "-left.ppm"));
            drift_views(app, scene, count, anchor, -1);
            const Frame left_edge_blank = drift_direct_frame(app, scene, canvas);
            require(drift_direct_frame(app, scene, canvas, &static_cloud).pixels == left_edge_blank.pixels,
                "left-edge cloud anchor was not wholly outside the view");
            const Frame left_edge = drift_frozen_frame(app, scene, *cloud);
            require(left_edge.pixels == drift_direct_frame(app, scene, canvas, cloud).pixels,
                "Scene culled cloud pixels that had drifted into the right edge");
            drift_raster_bounds(left_edge, left_edge_blank, scene, *cloud, output,
                stem + "-left-edge", records);
            scene.update(12);
            drift_views(app, scene, count, anchor);
            require(drift_frozen_frame(app, scene, *cloud).pixels == initial.pixels,
                "a complete cloud period did not return to the authored anchor raster");
            if (size.x == 2560)
            {
                app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
                std::ignore = app.renderer()->window_size_changed(1280, 720);
                drift_views(app, scene, count, anchor);
                require(drift_frozen_frame(app, scene, *cloud).pixels == initial_720.pixels,
                    "cloud native raster changed after a 720p/1080p/QHD/720p resize round trip");
            }
            std::cout << "PASS " << stem << " real cloud movement, both Scene edges, projected bounds, reduction, frozen redraw/RNG\n";
        }
    }
}

void drift_context_capture(Application& app, const Level& level,
    const std::filesystem::path& output, const std::string& stem)
{
    const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
    const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
    std::srand(5719);
    const int expected_random = std::rand();
    std::srand(5719);
    const Frame first = render(level, *app.renderer());
    for (int repeat = 0; repeat < 3; ++repeat)
    {
        require(first.pixels == render(level, *app.renderer()).pixels,
            "frozen real-arena cloud capture changed pixels");
    }
    require(std::rand() == expected_random && before == observation(level),
        "real-arena cloud drawing changed RNG or simulation state");
#if COLOURWARS_DEV_TOOLS
    require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
        "real-arena cloud drawing changed the match fingerprint");
#endif
    save(first, output / (stem + ".ppm"));
}

void drift_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    LevelBuilder& builder, PlaySettings& preferences, const std::filesystem::path& output)
{
    const RestoreFlag restore{ preferences.reduce_effects, preferences.reduce_effects };
    preferences.reduce_effects = false;
    std::ofstream records(output / "drift-detail.csv");
    records << "capture,view,cloud_pixels,x,y,width,height\n";
    drift_fixture_matrix(app, canvas, preferences, output, records);

    // Real builder-owned arena contexts supplement the isolated raster proof.
    // These are review captures; moving players/clock must not stand in for
    // the cloud-only pixel differences checked above.
    for (const char* stage : { "close_quarters", "king_of_the_hill", "turbulence", "movement_course" })
    {
        require(levels.contains(stage), "cloud context stage was absent from the manifest");
        for (const int count : { 1, 4 })
        {
            if (count == 4 && std::string(stage) != "close_quarters") { continue; }
            MatchSetup settings;
            settings.set_player_count(count);
            settings.set_stage(stage);
            settings.set_screen_layout(count == 1 ? ScreenLayout::one_player : ScreenLayout::four_player);
            settings.begin_input_claims(3);
            for (int view = 1; view < count; ++view)
            {
                require(settings.claim_input_slot(view - 1), "cloud context input claim failed");
            }
            for (int view = 0; view < count; ++view)
            {
                settings.set_player_team(view, view % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
                settings.set_player_weapon(view, WeaponType::sprayer);
            }
            app.viewport_manager()->set_layout(settings.screen_layout());
            auto level = builder.build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
            const auto inputs = neutral_inputs();
            settle(*level, inputs);
            resize(app, *level, count == 1 ? Vector2I(1280, 720) : Vector2I(1920, 1080));
            const auto compose_sky = [&]()
            {
                if (count != 4) { return; }
                // Deliberately place the real roster above the platforms, to
                // the right of Close Quarters cloud_4, so normal player-camera
                // following shows that cloud at native 4P size clear of the
                // roster. Repeat after advancing time to retain the
                // composition. This is an art fixture, not a route or
                // jump-reachability claim; no camera is replaced or edited.
                // The 4P minimum world width (presentation camera, da041c60)
                // shows 720 world units of height per 1080p pane, so with the
                // camera_bounds top at y 4300 no sky cloud can project lower
                // than (y - 4300) * 0.75; cloud_1/3/5 always sit within the top
                // 120 px. cloud_4 (y 4551) is the framable big cloud.
                for (int view = 0; view < count; ++view)
                {
                    require(level->place_player(view, Vector2F(
                        3840.0f + 120.0f * static_cast<float>(view), 4470.0f)),
                        "could not place the four-player sky composition fixture");
                }
                // Independent authored cloud_4 rectangle, widened by its
                // complete 32-unit drift, must remain fully inside pane 1
                // and below its HUD rather than merely intersecting a view.
                const RectangleF cloud_span(3461, 4551, 320, 96);
                const Viewport pane = app.viewport_manager()->player_viewport(0);
                const Camera camera = level->view_camera(0);
                require(camera.visible_rectangle(pane).contains(cloud_span),
                    "four-player sky composition did not fully frame cloud_4's drift");
                const RectangleF projected = camera.calculate_view_rectangle(cloud_span);
                require(projected.y >= 120.0f && projected.bottom() <= pane.height - 100.0f,
                    "four-player cloud composition placed cloud_4 behind HUD or pane edges");
            };
            const std::string stem = "drift-arena-" + std::string(stage) + '-' + std::to_string(count) +
                "p" + (count == 4 ? "-sky-composition" : "");
            compose_sky();
            drift_context_capture(app, *level, output, stem + "-early");
            for (int tick = 0; tick < 720; ++tick) { level->update(inputs, dt); }
            compose_sky();
            drift_context_capture(app, *level, output, stem + "-later");
        }
    }
    require(records.good(), "cloud drift record write failed");
    std::cout << "PASS drift-only actual D3D11 cloud Scene matrix and all four arena contexts; 4P Close Quarters uses explicit player placement for sky composition, no gameplay acceptance; human motion/readability review remains separate\n";
}
