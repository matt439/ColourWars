// Included after countdown_checks.h, using the same real renderer and Level
// observation helpers. These checks do not recreate the card's drawing.

Frame reveal_fixture(Application& app, const Canvas& canvas, const RevealArt* art,
    const RectangleF& bounds, std::wstring_view text, const Colour& colour,
    float age, bool reduced)
{
    Renderer& renderer = *app.renderer();
    renderer.begin_frame();
    renderer.set_view_count(1);
    DrawList list = renderer.view(0);
    list.set_viewport(app.viewport_manager()->fullscreen_viewport());
    list.set_camera(Camera::DEFAULT_CAMERA);
    list.set_filter(TextureFilter::point);
    canvas.draw(list, RectangleF(Vector2F::ZERO, app.resolution_manager()->resolution_vec()));
    if (art != nullptr) { art->draw(list, bounds, text, colour, age, reduced); }
    return read(renderer);
}

void reveal_card_matrix(Application& app, const Canvas& canvas,
    const Presentation& presentation, const std::filesystem::path& output)
{
    const OpeningPresentation& look = presentation.opening();
    const RevealArt art(&look, app.render_resources());
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        const float scale = static_cast<float>(size.y) / 720.0f;
        const RectangleF box(Vector2F(480.0f, 260.0f) * scale, Vector2F(260.0f, 96.0f) * scale);
        const Frame blank = reveal_fixture(app, canvas, nullptr, box, L"", Colour::white, 0, false);
        for (const TeamColourPair palette : { TeamColourPair::blue_orange,
            TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            if (size.x != 1280 && palette != TeamColourPair::blue_orange) { continue; }
            const TeamColour colours = TeamColourTools::team_colours(palette, presentation.team_palette());
            std::vector<unsigned char> previous;
            for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
            {
                const std::wstring text = team == PlayerTeam::a ? L"A 100.0%" : L"B 0.0%";
                const Colour& colour = colours.team_colour(team);
                const auto draw = [&](float age, bool reduced)
                {
                    return reveal_fixture(app, canvas, &art, box, text, colour, age, reduced);
                };
                const Frame settled = draw(look.reveal_art.beat_duration, false);
                const float beat_age = look.reveal_art.beat_duration * 0.25f;
                require(settled.pixels == draw(beat_age, true).pixels &&
                    settled.pixels == draw(-1.0f, false).pixels &&
                    settled.pixels == draw(look.reveal_art.beat_duration + 1.0f, false).pixels,
                    "Reduced, negative or expired reveal card did not retain its settled result");
                require(foreground_local_delta(settled, blank, box) > 100,
                    "Reveal card was absent or escaped its supplied bounds");
                require(previous.empty() || previous != settled.pixels, "A/B reveal labels rendered identically");
                previous = settled.pixels;
                const Frame beat = draw(beat_age, false);
                require(foreground_local_delta(beat, settled, box) > 0,
                    "Reveal click supplied no bounded accent or reduced-effects suppression");
                std::srand(39481);
                const int expected_random = std::rand();
                std::srand(39481);
                for (int repeat = 0; repeat < 3; ++repeat)
                {
                    require(beat.pixels == draw(beat_age, false).pixels, "Frozen reveal card changed pixels");
                }
                require(std::rand() == expected_random, "Reveal card drawing consumed random numbers");
                const std::string stem = std::string("closing-card-") + TeamColourTools::palette_key(palette) +
                    (team == PlayerTeam::a ? "-a-" : "-b-") + std::to_string(size.x);
                save(foreground_crop(settled, countdown_crop_bounds(box)), output / (stem + "-settled-detail.ppm"));
                save(foreground_crop(beat, countdown_crop_bounds(box)), output / (stem + "-beat-detail.ppm"));
            }
        }
        if (size.x == 1280)
        {
            OpeningPresentation extreme = look;
            extreme.reveal_art.keyline_width = 0.06f;
            extreme.reveal_art.padding = 0.08f;
            const RevealArt thick(&extreme, app.render_resources());
            const Frame thick_frame = reveal_fixture(app, canvas, &thick, box,
                L"A 100.0%", Colour::yellow, extreme.reveal_art.beat_duration, false);
            require(foreground_local_delta(thick_frame, blank, box) > 100,
                "Widest accepted reveal keyline and minimum padding escaped the card bounds");
            save(foreground_crop(thick_frame, countdown_crop_bounds(box)),
                output / "closing-card-wide-keyline-min-padding-detail.ppm");
        }
    }
}

Frame closing_hud_fixture(Application& app, const Canvas& canvas,
    const InterfaceGameplay& hud, int count, float timer, bool reduced,
    bool clock_running = true)
{
    Renderer& renderer = *app.renderer();
    renderer.begin_frame();
    renderer.set_view_count(count);
    for (int view = 0; view < count; ++view)
    {
        const Viewport& viewport = app.viewport_manager()->player_viewport(view);
        DrawList list = renderer.view(view);
        list.set_viewport(viewport);
        list.set_camera(Camera::DEFAULT_CAMERA);
        list.set_filter(TextureFilter::point);
        canvas.draw(list, RectangleF(Vector2F::ZERO, viewport.size()));
        hud.draw_gameplay_interface(list, viewport.size(), view, view, PlayerTeam::a,
            0.75f, 0.5f, timer, Colour::blue, reduced, clock_running);
    }
    return read(renderer);
}

void closing_clock_matrix(Application& app, const Canvas& canvas,
    const Presentation& presentation, const std::filesystem::path& output)
{
    const InterfaceGameplay hud(app.render_resources(), &presentation.hud(), &presentation.player_identity());
    for (const int count : { 1, 2, 4 })
    {
        app.viewport_manager()->set_layout(count == 1 ? ScreenLayout::one_player :
            (count == 2 ? ScreenLayout::two_player : ScreenLayout::four_player));
        for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
        {
            if (count != 4 && size.x != 1280) { continue; }
            app.resolution_manager()->set_resolution_exactly(size);
            std::ignore = app.renderer()->window_size_changed(size.x, size.y);
            for (const float timer : { 11.0f, 10.0f, 9.95f, 9.5f, 1.0f, 0.95f, 0.5f, 0.0f })
            {
                const HudPresentation& look = presentation.hud();
                const auto font = app.render_resources()->resolve_sprite_font(look.timer_font);
                const std::wstring text = timer > 10.0f ? L"0:11" :
                    (timer > 1.0f ? L"0:10" : (timer > 0.0f ? L"0:01" : L"0:00"));
                std::vector<RectangleF> faces, bounds;
                std::vector<size_t> changed_in_view(static_cast<size_t>(count));
                std::vector<size_t> card_changed_in_view(static_cast<size_t>(count));
                for (int view = 0; view < count; ++view)
                {
                    const Viewport& viewport = app.viewport_manager()->player_viewport(view);
                    const RectangleI pane = viewport.pixel_rect();
                    const Vector2F size_in_pane = viewport.size();
                    const float scale = std::min(size_in_pane.x / look.player_identity_reference_width,
                        size_in_pane.y / look.reference_height);
                    const float timer_scale = scale * look.player_identity_reference_width / look.timer_reference_width;
                    const Vector2F measured = app.render_resources()->measure_text(font, text) * timer_scale;
                    const float pen = look.closing_clock.keyline_width * scale;
                    const float reach = look.closing_clock.tick_reach * scale + pen * 3.0f;
                    const float shadow = look.closing_clock.shadow_offset * scale;
                    const Vector2F padding(std::max(look.player_identity_padding.x * scale, pen * 3.0f),
                        std::max(look.player_identity_padding.y * scale, pen * 3.0f));
                    const Vector2F pos(static_cast<float>(pane.x) + (size_in_pane.x - measured.x) / 2.0f -
                        size_in_pane.x * look.timer_offset.x,
                        static_cast<float>(pane.y) + size_in_pane.y * look.timer_offset.y);
                    const RectangleF face(pos - padding, measured + padding * 2.0f);
                    faces.push_back(face);
                    bounds.emplace_back(face.x - reach, face.y,
                        face.width + reach * 2.0f + shadow, face.height + shadow);
                }
                std::srand(39481);
                const int expected_random = std::rand();
                std::srand(39481);
                const Frame normal = closing_hud_fixture(app, canvas, hud, count, timer, false);
                const Frame reduced = closing_hud_fixture(app, canvas, hud, count, timer, true);
                require(normal.pixels == closing_hud_fixture(app, canvas, hud, count, timer, false).pixels,
                    "Frozen closing clock drawing changed pixels");
                require(std::rand() == expected_random, "Closing clock drawing consumed random numbers");
                // The settled card, as well as its ticks, belongs only to
                // positive last-ten seconds. Hide its paper at the same state;
                // changing presentation must have no effect before or at zero.
                auto& art = const_cast<ClosingClockPresentation&>(look.closing_clock);
                const Colour paper = art.paper;
                art.paper.a = 0.0f;
                const Frame without_paper = closing_hud_fixture(app, canvas, hud, count, timer, true);
                art.paper = paper;
                const bool beat = timer == 10.0f || timer == 9.95f || timer == 1.0f || timer == 0.95f;
                size_t changed = 0;
                size_t card_changed = 0;
                for (int y = 0; y < size.y; ++y)
                {
                    for (int x = 0; x < size.x; ++x)
                    {
                        const size_t offset = (static_cast<size_t>(y) * static_cast<size_t>(size.x) +
                            static_cast<size_t>(x)) * 4;
                        const bool tick_pixel = !same_pixel(normal, reduced, offset);
                        const bool card_pixel = !same_pixel(reduced, without_paper, offset);
                        if (!tick_pixel && !card_pixel) { continue; }
                        bool local = false;
                        for (int view = 0; view < count; ++view)
                        {
                            const RectangleF& area = bounds[view];
                            const RectangleF& face = faces[view];
                            // Raster pixels cover a unit square. Fractional
                            // geometry, including addition of a pane origin,
                            // must be rounded out before testing pixel indices.
                            const RectangleI pixels = countdown_crop_bounds(area);
                            const float px = static_cast<float>(x);
                            if (x < pixels.x || x >= pixels.right() || y < pixels.y || y >= pixels.bottom()) { continue; }
                            require(!tick_pixel || px < face.x || px >= face.right(),
                                "Closing clock beat moved or altered its settled readable card");
                            local = true;
                            if (tick_pixel) { ++changed_in_view[view]; }
                            if (card_pixel) { ++card_changed_in_view[view]; }
                        }
                        if (!local)
                        {
                            std::cerr.precision(9);
                            std::cerr << "Clock locality: " << count << "P " << size.x << 'x' << size.y <<
                                " timer=" << timer << " pixel=" << x << ',' << y;
                            for (size_t view = 0; view < bounds.size(); ++view)
                            {
                                const auto& area = bounds[view];
                                const RectangleI pixels = countdown_crop_bounds(area);
                                std::cerr << " pane" << view << " bounds=" << area.x << ',' << area.y << ',' <<
                                    area.width << ',' << area.height << " pixels=" << pixels.x << ',' << pixels.y << ',' <<
                                    pixels.width << ',' << pixels.height;
                            }
                            std::cerr << '\n';
                            require(false, "Closing clock accent changed another HUD region or player pane");
                        }
                        if (tick_pixel) { ++changed; }
                        if (card_pixel) { ++card_changed; }
                    }
                }
                require(beat ? changed > 0 : changed == 0,
                    "Closing clock accent was missing, early, expired late or survived zero");
                for (const size_t pane_changed : changed_in_view)
                {
                    require(beat ? pane_changed > 0 : pane_changed == 0,
                        "Closing clock beat missed a player pane");
                }
                const bool closing = timer > 0.0f && timer <= 10.0f;
                require(closing ? card_changed > 0 : card_changed == 0,
                    "Settled closing clock card was missing, premature or survived zero");
                for (const size_t pane_changed : card_changed_in_view)
                {
                    require(closing ? pane_changed > 0 : pane_changed == 0,
                        "Settled closing clock card gate differed between player panes");
                }
                if (timer == 9.95f || timer == 9.5f || timer == 0.95f || timer == 0.0f)
                {
                    const RectangleI crop = countdown_crop_bounds(bounds.front());
                    const std::string stem = "closing-clock-" + std::to_string(count) + "p-" +
                        std::to_string(size.x) + '-' + std::to_string(static_cast<int>(timer * 100.0f));
                    save(foreground_crop(normal, crop), output / (stem + "-detail.ppm"));
                    if (timer == 9.95f)
                    {
                        save(foreground_crop(reduced, crop), output / (stem + "-reduced-detail.ppm"));
                    }
                }
            }
            for (const float timer : { 10.0f, 1.0f })
            {
                // A short match's clock exists behind its initial countdown,
                // but neither final-ten paper nor ticks start until live play.
                const Frame waiting = closing_hud_fixture(app, canvas, hud, count, timer, false, false);
                const Frame reduced = closing_hud_fixture(app, canvas, hud, count, timer, true, false);
                require(waiting.pixels == reduced.pixels,
                    "Short match displayed closing clock ticks before its live clock started");
                auto& art = const_cast<ClosingClockPresentation&>(presentation.hud().closing_clock);
                const Colour paper = art.paper;
                art.paper.a = 0.0f;
                const Frame without_paper = closing_hud_fixture(app, canvas, hud, count, timer, true, false);
                art.paper = paper;
                require(waiting.pixels == without_paper.pixels,
                    "Short match displayed a settled closing clock card before live play");
                require(waiting.pixels != closing_hud_fixture(app, canvas, hud, count, timer, true, true).pixels,
                    "Starting a short match clock did not reveal the settled closing card");
            }
        }
    }
}

void reveal_live_card(Application& app, Level& level, Presentation& presentation,
    const RectangleF& picture, bool visible)
{
    const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
    const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
    auto& art = const_cast<RevealArtPresentation&>(presentation.opening().reveal_art);
    const Colour original = art.paper;
    const Frame normal = render(level, *app.renderer());
    art.paper.a = 0.0f;
    const Frame hidden = render(level, *app.renderer());
    art.paper = original;
    const size_t changed = foreground_local_delta(normal, hidden, picture);
    require(visible ? changed > 0 : changed == 0,
        "Real Level reveal cards were missing, premature or shown for a draw/solo match");
    require(before == observation(level), "Reveal card comparison changed observed match state");
#if COLOURWARS_DEV_TOOLS
    require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
        "Reveal card comparison changed the replay fingerprint");
#endif
}

void closing_story_matrix(Application& app, LevelBuilder& builder,
    const GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    std::ofstream records(output / "closing.csv");
    records << "capture,players,state,team_a_area,team_b_area\n";
    const RectangleF window(0, 0, 1280, 720);
    const Monitor geometry(&presentation.opening(), app.render_resources(), {});
    const RectangleF picture = geometry.picture(window);
    for (const int painter : { 0, 1, -1 })
    {
        auto level = monitor_story(app, builder, painter == -1 ? 4 : 2);
        resize(app, *level, { 1280, 720 });
        auto inputs = neutral_inputs();
        for (int tick = 0; tick < 1800 && level->state() != LevelState::active; ++tick)
        {
            level->update(acceptance::slot_seats(*level, inputs), dt);
        }
        require(level->state() == LevelState::active, "Closing story did not reach live play");
        if (painter >= 0)
        {
            require(level->place_player(painter, { 1200, 1275 }), "Could not place closing story painter");
            const int slot = level->player(painter).player_num();
            inputs[slot].primary_shoot = true;
            inputs[slot].x_movement = 1;
        }
        for (int tick = 0; tick < 1800 && level->state() != LevelState::overview; ++tick)
        {
            level->update(acceptance::slot_seats(*level, inputs), dt);
        }
        require(level->state() == LevelState::overview, "Closing story did not reach reveal");
        const LevelEndInfo info = level->level_end_info();
        require(painter < 0 ? info.team_a_area == 0 && info.team_b_area == 0 :
            info.winning_team() == (painter == 0 ? PlayerTeam::a : PlayerTeam::b),
            "Closing story painter produced the wrong result");
        const std::string prefix = painter == 0 ? "closing-story-a-" :
            (painter == 1 ? "closing-story-b-" : "closing-story-draw-");
        const RevealTuning& reveal = tuning.match().reveal;
        const float click_at = reveal.walk_in + reveal.hover * 2.0f + reveal.decide;
        float age = 0.0f;
        while (age + dt < click_at) { level->update(acceptance::slot_seats(*level, inputs), dt); age += dt; }
        reveal_live_card(app, *level, presentation, picture, false);
        monitor_frozen(app, *level, output, prefix + "before-click", records);
        level->update(acceptance::slot_seats(*level, inputs), dt);
        reveal_live_card(app, *level, presentation, picture, painter >= 0);
        monitor_frozen(app, *level, output, prefix + "click", records);
        for (int tick = 0; tick < 20; ++tick) { level->update(acceptance::slot_seats(*level, inputs), dt); }
        reveal_live_card(app, *level, presentation, picture, painter >= 0);
        monitor_frozen(app, *level, output, prefix + "settled", records);
    }
    auto solo = countdown_level(app, builder, 1, OpeningMode::off);
    resize(app, *solo, { 1280, 720 });
    auto inputs = neutral_inputs();
    for (int tick = 0; tick < 2400 && solo->state() != LevelState::overview; ++tick)
    {
        solo->update(acceptance::slot_seats(*solo, inputs), dt);
    }
    require(solo->state() == LevelState::overview && !solo->revealed(), "Solo closing fixture acquired a reveal");
    reveal_live_card(app, *solo, presentation, picture, false);
    monitor_frozen(app, *solo, output, "closing-story-solo-no-monitor", records);
    require(records.good(), "Closing capture records could not be written");
}

void closing_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
    const GameTuning& tuning, Presentation& presentation, PlaySettings& preferences,
    const std::filesystem::path& output)
{
    const RestoreFlag restore{ preferences.reduce_effects, preferences.reduce_effects };
    preferences.reduce_effects = false;
    closing_clock_matrix(app, canvas, presentation, output);
    reveal_card_matrix(app, canvas, presentation, output);
    closing_story_matrix(app, builder, tuning, presentation, output);
    std::cout << "PASS closing: bounded last-ten clock beats at 1P/2P/4P720 and 4P1080/QHD, "
        "settled/reduced/zero boundaries; native A/B result cards in every palette, local click ticks, "
        "negative/expired/reduced settled rasters and draw RNG; real A/B winner, no-paint draw and solo "
        "reveal gates, frozen scene pixels/state and Debug replay fingerprint\n";
}
