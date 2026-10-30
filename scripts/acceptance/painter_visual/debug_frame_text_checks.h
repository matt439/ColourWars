// Included inside painter_visual.cpp's private namespace, after resume_fixture.h.
// Debug render-FPS text matrix.

void debug_frame_text_matrix(Application& app, GameData& data, const Presentation& presentation,
    const std::filesystem::path& output)
{
    static_cast<StateContext&>(app).clear();
    struct RestoreLayout
    {
        ViewportManager& viewports;
        ScreenLayout layout;
        ~RestoreLayout() { this->viewports.set_layout(this->layout); }
    } restore_layout{ *app.viewport_manager(), app.viewport_manager()->layout() };
    LevelBuilder builder(app.viewport_manager(), app.render_resources(), app.audio_resources(),
        &data.save()->play_settings(), data.level_infos(), data.tuning(), &presentation,
        app.resolution_manager(), app.thread_pool(), app.partitioner());
    auto& look = const_cast<DebugOverlayPresentation&>(presentation.debug_overlay());
    const auto font = app.render_resources()->resolve_sprite_font(look.font);
    const std::wstring simulation = L"sim dt: " + std::to_wstring(dt);
    const std::wstring thirty_text = L"render fps: 30.0";
    const std::wstring sixty_text = L"render fps: 60.0";
    // These are the actual DebugText timing rows, following its twelve
    // player/input lines. The pixels below verify both labels and values.
    constexpr int simulation_row = 12;
    constexpr int frame_row = 13;
    const auto position = [&](int row)
        { return look.position + Vector2F(0.0f, look.line_spacing * static_cast<float>(row)); };
    float width = 0.0f;
    float height = 0.0f;
    for (const std::wstring& text : { simulation, thirty_text, sixty_text })
    {
        const Vector2F measured = app.render_resources()->measure_text(font, text) *
            std::max(look.font_scale, look.shadow_scale);
        width = std::max(width, measured.x);
        height = std::max(height, measured.y);
    }
    width += std::abs(look.shadow_offset.x) + 3.0f;
    height += std::abs(look.shadow_offset.y) + 1.0f;
    std::ofstream evidence(output / "debug-frame-text.csv");
    evidence << "players,view,simulation_dt,first_frame_ms,second_frame_ms,changed_pixels\n";
    const auto inputs = neutral_inputs();
    for (const int count : { 1, 2, 4 })
    {
        app.viewport_manager()->set_layout(count == 1 ? ScreenLayout::one_player :
            count == 2 ? ScreenLayout::two_player : ScreenLayout::four_player);
        MatchSetup settings;
        settings.set_stage("movement_course");
        settings.set_duration(60.0f);
        settings.set_player_count(count);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_screen_layout(app.viewport_manager()->layout());
        for (int view = 0; view < count; ++view)
        {
            settings.set_player_team(view, view == 0 ? PlayerTeam::a : PlayerTeam::b);
            settings.set_player_weapon(view, WeaponType::sprayer);
        }
        auto level = builder.build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
        resize(app, *level, { 1280, 720 });
        settle(*level, inputs);
        level->set_players_showing_debug(true);
        const auto frozen = resume_observation(*level);
#if COLOURWARS_DEV_TOOLS
        const auto fingerprint = MatchFingerprint::of(*level, 0);
#endif
        const auto expected = [&](const std::wstring& fps)
        {
            struct RestoreColours
            {
                DebugOverlayPresentation& look;
                Colour ink;
                Colour shadow;
                ~RestoreColours() { this->look.colour = this->ink; this->look.shadow_colour = this->shadow; }
            } restore{ look, look.colour, look.shadow_colour };
            // Suppress the actual text while retaining the identical world,
            // HUD and weapon-debug geometry, then add the expected timing
            // lines and their immediate neighbours. Courier's measured glyph
            // height exceeds the authored row spacing: adjacent descenders
            // also occupy the timing crop and must retain their draw order.
            // All style writes precede Scene's worker fan-out and restore only
            // after it has joined; production drawing remains read-only.
            look.colour = look.shadow_colour = Colour(0.0f, 0.0f, 0.0f, 0.0f);
            Renderer& renderer = *app.renderer();
            renderer.begin_frame();
            level->draw(renderer);
            for (int view = 0; view < count; ++view)
            {
                DrawList list = renderer.view(view);
                list.set_viewport(level->player(view).viewport());
                list.set_camera(Camera::DEFAULT_CAMERA);
                list.set_filter(TextureFilter::linear);
                const auto line = [&](const std::wstring& text, int row)
                {
                    list.draw_text(font, text, position(row) + look.shadow_offset,
                        restore.shadow, look.shadow_scale, 0.0f, Vector2F::ZERO, 0.0f);
                    list.draw_text(font, text, position(row), restore.ink, look.font_scale,
                        0.0f, Vector2F::ZERO, 0.0f);
                };
                line(level->player(view).input_primary_shoot() ?
                    L"primary-shooting: true" : L"primary-shooting: false", simulation_row - 1);
                line(simulation, simulation_row);
                line(fps, frame_row);
                line(L"proj_count: " + std::to_wstring(level->projectile_count()), frame_row + 1);
                list.set_filter(TextureFilter::point);
            }
            return read(renderer);
        };
        std::srand(6197);
        const int next_random = std::rand();
        std::srand(6197);
        level->set_debug_frame_time(1000.0 / 30.0);
        const Frame thirty = render(*level, *app.renderer());
        const Frame expected_thirty = expected(thirty_text);
        require(thirty.pixels == render(*level, *app.renderer()).pixels,
            "Repeated 30 FPS Level drawing changed its clock snapshot or pixels");
        level->set_debug_frame_time(1000.0 / 60.0);
        const Frame sixty = render(*level, *app.renderer());
        const Frame expected_sixty = expected(sixty_text);
        require(sixty.pixels == render(*level, *app.renderer()).pixels &&
            frozen == resume_observation(*level) && std::rand() == next_random,
            "Debug timing draws changed the snapshot, simulation/cosmetics or draw RNG");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(*level, 0)).empty(),
            "Debug timing draws changed the replay fingerprint");
#endif
        std::vector<RectangleI> frame_rows;
        for (int view = 0; view < count; ++view)
        {
            const Viewport viewport = level->player(view).viewport();
            const RectangleI band = countdown_crop_bounds(RectangleF(viewport.position() + position(simulation_row),
                Vector2F(width, look.line_spacing + height)));
            const RectangleI frame = countdown_crop_bounds(RectangleF(viewport.position() + position(frame_row),
                Vector2F(width, height)));
            require(viewport.pixel_rect().contains(band), "Debug timing lines escaped their owning pane");
            const Frame thirty_detail = foreground_crop(thirty, band, 0);
            const Frame expected_thirty_detail = foreground_crop(expected_thirty, band, 0);
            const Frame sixty_detail = foreground_crop(sixty, band, 0);
            const Frame expected_sixty_detail = foreground_crop(expected_sixty, band, 0);
            if (view == 0 || thirty_detail.pixels != expected_thirty_detail.pixels ||
                sixty_detail.pixels != expected_sixty_detail.pixels)
            {
                const std::string stem = "debug-fps-" + std::to_string(count) + "p-view" + std::to_string(view);
                save(thirty_detail, output / (stem + "-30-detail.ppm"));
                save(expected_thirty_detail, output / (stem + "-30-expected-detail.ppm"));
                save(sixty_detail, output / (stem + "-60-detail.ppm"));
                save(expected_sixty_detail, output / (stem + "-60-expected-detail.ppm"));
            }
            require(thirty_detail.pixels == expected_thirty_detail.pixels &&
                sixty_detail.pixels == expected_sixty_detail.pixels,
                "Actual per-pane text did not show fixed simulation dt alongside the requested 30/60 render FPS");
            const size_t changed = pane_delta(thirty, sixty, frame);
            require(changed > 0, "Changing rendered frame time left a pane's FPS text constant");
            frame_rows.push_back(frame);
            evidence << count << ',' << view << ',' << dt << ',' << 1000.0 / 30.0 << ',' <<
                1000.0 / 60.0 << ',' << changed << '\n';
        }
        for (size_t pixel = 0; pixel < thirty.pixels.size(); pixel += 4)
        {
            if (same_pixel(thirty, sixty, pixel)) { continue; }
            const int x = static_cast<int>((pixel / 4) % static_cast<size_t>(thirty.width));
            const int y = static_cast<int>((pixel / 4) / static_cast<size_t>(thirty.width));
            require(std::any_of(frame_rows.begin(), frame_rows.end(), [&](const RectangleI& row)
                { return x >= row.x && x < row.right() && y >= row.y && y < row.bottom(); }),
                "Changing the render FPS snapshot changed pixels outside an owning FPS row");
        }
        save(thirty, output / ("debug-fps-" + std::to_string(count) + "p-30-context.ppm"));
        save(sixty, output / ("debug-fps-" + std::to_string(count) + "p-60-context.ppm"));
    }
    require(evidence.good(), "Debug frame-text evidence write failed");
    std::cout << "PASS actual 1/2/4-pane render FPS text versus fixed simulation dt; frozen repeat draws and RNG\n";
}
