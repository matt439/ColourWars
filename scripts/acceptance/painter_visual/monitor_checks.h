// Included inside painter_visual.cpp's private namespace, after its shared
// renderer, builder and raster helpers. The fixtures exercise composition and
// story timing; they do not copy the monitor's decorative geometry.

enum class MonitorCapture { plain, preview, saved, closed };

bool monitor_pixel_equal(const Frame& first, const Frame& second, const Vector2F& at)
{
    const int x = std::clamp(static_cast<int>(at.x), 0, first.width - 1);
    const int y = std::clamp(static_cast<int>(at.y), 0, first.height - 1);
    return same_pixel(first, second,
        (static_cast<size_t>(y) * static_cast<size_t>(first.width) + static_cast<size_t>(x)) * 4);
}

std::vector<float> monitor_persona_observation(const MonitorPersona& persona)
{
    std::vector<float> result{ static_cast<float>(persona.shade), persona.child ? 1.0f : 0.0f,
        persona.mug ? 1.0f : 0.0f, persona.coffee_ring.x, persona.coffee_ring.y };
    for (const auto& note : persona.sticky_notes)
    {
        result.insert(result.end(), { note.at.x, note.at.y, static_cast<float>(note.colour), note.tilt });
    }
    for (const auto& sticker : persona.stickers)
    {
        result.insert(result.end(), { sticker.at.x, sticker.at.y, static_cast<float>(sticker.colour), sticker.tilt });
    }
    for (const auto& point : persona.crack) { result.insert(result.end(), { point.x, point.y }); }
    return result;
}

Frame monitor_fixture(Application& app, const Monitor& monitor, const RectangleF& screen,
    const Canvas& canvas, const Canvas& picture, MonitorCapture phase,
    bool front_details = true, const Silhouette* silhouette = nullptr)
{
    Renderer& renderer = *app.renderer();
    const RectangleF window(Vector2F::ZERO, app.resolution_manager()->resolution_vec());
    renderer.begin_frame();
    renderer.set_view_count(1);
    DrawList list = renderer.view(0);
    list.set_viewport(app.viewport_manager()->fullscreen_viewport());
    list.set_camera(Camera::DEFAULT_CAMERA);
    list.set_filter(TextureFilter::point);
    canvas.draw(list, window);
    monitor.draw(list, screen);
    picture.draw(list, monitor.picture(screen));
    if (phase == MonitorCapture::preview)
    {
        monitor.draw_preview(list, monitor.picture(screen), Colour::red);
    }
    if (phase == MonitorCapture::closed) { monitor.draw_closed(list, screen); }
    if (front_details) { monitor.draw_front_details(list, screen); }
    if (phase != MonitorCapture::closed) { monitor.draw_close_button(list, screen); }
    if (phase == MonitorCapture::saved) { monitor.draw_saved(list, screen, Colour::red); }
    if (silhouette != nullptr) { silhouette->draw(list, window, monitor.persona(), {}); }
    return read(renderer);
}

void monitor_frozen(Application& app, Level& level, const std::filesystem::path& output,
    const std::string& stem, std::ofstream& records)
{
    const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
    const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
    // Drawing may neither advance its seeded persona nor consume randomness
    // that a subsequent simulation update would otherwise receive.
    std::srand(19481);
    const int expected_random = std::rand();
    std::srand(19481);
    const Frame first = render(level, *app.renderer());
    for (int repeat = 0; repeat < 3; ++repeat)
    {
        require(first.pixels == render(level, *app.renderer()).pixels,
            "frozen story redraw changed monitor, decorations or scene pixels");
    }
    require(std::rand() == expected_random, "story drawing consumed random numbers");
    require(before == observation(level), "story drawing changed observed simulation state");
#if COLOURWARS_DEV_TOOLS
    require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
        "story drawing changed the replay fingerprint");
#endif
    save(first, output / (stem + ".ppm"));
    records << stem << ',' << level.player_count() << ',' << static_cast<int>(level.state()) << ','
        << level.level_end_info().team_a_area << ',' << level.level_end_info().team_b_area << '\n';
}

std::unique_ptr<Level> monitor_story(Application& app, LevelBuilder& builder, int count)
{
    MenuLevelSettings settings;
    settings.set_player_count(count);
    settings.set_stage("movement_course");
    settings.set_duration(1.25f);
    settings.set_screen_layout(count == 2 ? ScreenLayout::two_player : ScreenLayout::four_player);
    settings.begin_input_claims(3);
    constexpr int slots[] = { 3, 1, 2, 0 };
    for (int view = 1; view < count; ++view)
    {
        require(settings.claim_input_slot(slots[view]), "monitor story input claim failed");
    }
    for (int view = 0; view < count; ++view)
    {
        settings.set_player_team(view, view % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
        settings.set_player_weapon(view, WeaponType::roller);
    }
    app.viewport_manager()->set_layout(settings.screen_layout());
    std::srand(409 + count);
    return builder.build_level(settings, TeamColourPair::blue_orange, OpeningMode::full);
}

void monitor_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
    const GameTuning& tuning, const Presentation& presentation, const std::filesystem::path& output)
{
    std::ofstream records(output / "monitor.csv");
    records << "capture,players,state,team_a_area,team_b_area\n";
    const OpeningPresentation& look = presentation.opening();
    require(look.bezel_shades.size() == 6, "monitor review requires the six authored body shades");
    const Canvas picture_canvas(app.render_resources(), Colour(0.2f, 0.55f, 0.4f));
    const Silhouette silhouette(&look, app.render_resources());
    const RectangleF reference(0, 0, 1280, 720);
    MonitorPersona persona;
    persona.mug = true;
    persona.coffee_ring = { 0.77f, 0.9f };
    persona.stickers = { { { 0.27f, 0.92f }, 0, 0 }, { { 0.65f, 0.91f }, 3, 0 } };
    persona.crack = { { 0.005f, 0.65f }, { 0.03f, 0.63f }, { 0.025f, 0.66f }, { 0.06f, 0.68f } };
    const Monitor geometry(&look, app.render_resources(), persona);
    const RectangleF picture = geometry.picture(reference);
    // A manually chosen sticky note reaches across the picture boundary. A
    // normal random roll is not guaranteed to expose the foreground-order bug.
    persona.sticky_notes = { { { picture.x / reference.width - 0.015f,
        picture.y / reference.height + 0.09f }, 0, 0.1f }, { { 0.72f, 0.005f }, 1, -0.1f } };

    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    std::vector<std::vector<unsigned char>> shades;
    for (size_t shade = 0; shade < look.bezel_shades.size(); ++shade)
    {
        persona.shade = shade;
        const Monitor monitor(&look, app.render_resources(), persona);
        const Frame frame = monitor_fixture(app, monitor, reference, canvas, picture_canvas,
            MonitorCapture::plain, true, &silhouette);
        for (const auto& previous : shades)
        {
            require(previous != frame.pixels, "two authored monitor body shades rendered identically");
        }
        shades.push_back(frame.pixels);
        save(frame, output / ("monitor-adult-mug-shade" + std::to_string(shade + 1) + "-1280.ppm"));
    }
    persona.child = true;
    persona.mug = false;
    const Monitor child(&look, app.render_resources(), persona);
    save(monitor_fixture(app, child, reference, canvas, picture_canvas,
        MonitorCapture::plain, true, &silhouette), output / "monitor-child-stickers-1280.ppm");
    persona.child = false;
    persona.mug = true;

    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        const RectangleF window(0, 0, static_cast<float>(size.x), static_cast<float>(size.y));
        const Monitor monitor(&look, app.render_resources(), persona);
        const auto persona_before = monitor_persona_observation(monitor.persona());
        for (const bool pushed : { false, true })
        {
            if (pushed && size.x != 1280) { continue; }
            const RectangleF glass = monitor.glass(window);
            const RectangleF screen = pushed ? monitor.screen_for_glass(RectangleF(
                glass.x * 0.5f, glass.y * 0.5f,
                (glass.width + window.width) * 0.5f, (glass.height + window.height) * 0.5f)) : window;
            const std::string prefix = "monitor-" + std::string(pushed ? "pushed-" : "native-") +
                std::to_string(size.x);
            const Frame behind = monitor_fixture(app, monitor, screen, canvas, picture_canvas,
                MonitorCapture::plain, false);
            const Frame front = monitor_fixture(app, monitor, screen, canvas, picture_canvas, MonitorCapture::plain);
            const Frame preview = monitor_fixture(app, monitor, screen, canvas, picture_canvas, MonitorCapture::preview);
            const Frame saved = monitor_fixture(app, monitor, screen, canvas, picture_canvas, MonitorCapture::saved);
            const Frame closed = monitor_fixture(app, monitor, screen, canvas, picture_canvas, MonitorCapture::closed);
            const RectangleF area = monitor.picture(screen);
            size_t overlapping = 0;
            size_t untinted = 0;
            for (int y = std::max(0, static_cast<int>(area.y)); y < std::min(size.y, static_cast<int>(area.bottom())); ++y)
            {
                for (int x = std::max(0, static_cast<int>(area.x)); x < std::min(size.x, static_cast<int>(area.right())); ++x)
                {
                    const size_t offset = (static_cast<size_t>(y) * static_cast<size_t>(size.x) + static_cast<size_t>(x)) * 4;
                    if (!same_pixel(behind, front, offset))
                    {
                        ++overlapping;
                        if (same_pixel(front, preview, offset)) { ++untinted; }
                    }
                }
            }
            require(overlapping > 100 && untinted > overlapping / 2,
                "picture or preview covered foreground monitor notes");
            require(monitor_pixel_equal(behind, front, area.center()),
                "monitor decoration shading changed the picture interior");
            const Vector2F slot = monitor.empty_slot(screen).center();
            require(monitor_pixel_equal(behind, front, slot) && monitor_pixel_equal(front, preview, slot),
                "monitor details or preview changed the empty palette slot");
            require(!monitor_pixel_equal(front, preview, area.center()), "preview did not tint the picture");
            require(!monitor_pixel_equal(front, saved, slot) && monitor_pixel_equal(front, saved, area.center()),
                "saving did not fill only the intended palette slot");
            require(!monitor_pixel_equal(front, closed, area.center()), "closing did not replace the picture with the desktop");
            std::srand(9983);
            const int expected_random = std::rand();
            std::srand(9983);
            for (int repeat = 0; repeat < 3; ++repeat)
            {
                require(front.pixels == monitor_fixture(app, monitor, screen, canvas, picture_canvas,
                    MonitorCapture::plain).pixels, "frozen monitor fixture changed pixels");
            }
            require(std::rand() == expected_random, "monitor drawing consumed random numbers");
            require(persona_before == monitor_persona_observation(monitor.persona()), "monitor drawing changed its persona");
            save(front, output / (prefix + ".ppm"));
            save(preview, output / (prefix + "-preview.ppm"));
            save(saved, output / (prefix + "-saved.ppm"));
            save(closed, output / (prefix + "-closed.ppm"));
        }
    }

    const OpeningTuning& opening = tuning.match().opening;
    const RevealTuning& reveal = tuning.match().reveal;
    for (int count : { 2, 4 })
    {
        auto level = monitor_story(app, builder, count);
        resize(app, *level, { 1280, 720 });
        auto inputs = neutral_inputs();
        const std::string prefix = "monitor-" + std::to_string(count) + "p-story-";
        const auto snap = [&](const char* beat) { monitor_frozen(app, *level, output, prefix + beat, records); };
        const auto advance = [&](float seconds)
        {
            const int ticks = static_cast<int>(std::ceil(seconds / dt));
            for (int tick = 0; tick < ticks; ++tick) { level->update(inputs, dt); }
        };
        snap("opening");
        advance(opening.settle + opening.preview * 0.65f);
        snap("preview-a");
        advance(opening.preview);
        snap("preview-b");
        for (int tick = 0; tick < 1200 && level->state() == LevelState::opening; ++tick) { level->update(inputs, dt); }
        require(level->state() == LevelState::start_countdown, "monitor story missed its countdown");
        snap("countdown");
        advance(std::max(0.0f, tuning.match().countdown - opening.push * 0.5f));
        require(level->state() == LevelState::start_countdown, "monitor story missed its glass push");
        snap("push");
        for (int tick = 0; tick < 600 && level->state() != LevelState::active; ++tick) { level->update(inputs, dt); }
        require(level->state() == LevelState::active, "monitor story did not reach active play");
        if (count == 2)
        {
            require(level->place_player(0, { 1200, 1275 }), "could not place painting reveal fixture");
            inputs[3].primary_shoot = true;
            inputs[3].x_movement = 1;
        }
        for (int tick = 0; tick < 600 && level->state() == LevelState::active; ++tick) { level->update(inputs, dt); }
        require(level->state() == LevelState::zoom_out, "monitor story missed its reveal pullback");
        advance(tuning.match().zoom_out * 0.5f);
        snap("pullback");
        for (int tick = 0; tick < 600 && level->state() == LevelState::zoom_out; ++tick) { level->update(inputs, dt); }
        require(level->state() == LevelState::overview, "monitor story did not reach its monitor overview");
        const LevelEndInfo info = level->level_end_info();
        require(count == 2 ? info.team_a_area > 0 && info.winning_team() == PlayerTeam::a :
            info.team_a_area == 0 && info.team_b_area == 0, "monitor reveal fixture painted an unexpected result");
        advance(reveal.walk_in + reveal.hover * 0.65f);
        snap(count == 2 ? "reveal-preview" : "closing-hesitate");
        advance(reveal.hover * 1.35f + reveal.decide + 0.1f);
        snap(count == 2 ? "reveal-saved" : "closing-desktop");
    }
    // GameLevel::start_match supplies OpeningMode::off for solo practice;
    // use the same policy here rather than bypassing the shell's decision.
    auto solo = build(app, builder, 1, TeamColourPair::blue_orange);
    resize(app, *solo, { 1280, 720 });
    require(!solo->revealed() && !solo->intro_playing(), "solo practice unexpectedly acquired a monitor");
    monitor_frozen(app, *solo, output, "monitor-solo-practice-no-monitor", records);
    require(records.good(), "monitor capture record write failed");
    std::cout << "PASS monitor: six shades, adult/mug and child/stickers, picture-overlap notes before preview, "
        "untinted picture/palette interiors, saved slot and closed desktop at 720p/1080p/QHD and bounded push; "
        "real 2P/4P opening/preview/countdown/push/pullback/winner reveal/no-paint close, "
        "solo practice without monitor, frozen redraw/persona/state/fingerprint and unchanged draw RNG\n";
}
