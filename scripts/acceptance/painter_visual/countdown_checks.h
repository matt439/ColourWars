// Included after monitor_checks.h inside painter_visual.cpp's private namespace.
// Observe the real drawing and match transitions, without reproducing its glyphs.

Frame countdown_fixture(Application& app, const Canvas& canvas, const CountdownArt* art,
    const RectangleF& bounds, std::wstring_view text, float age, bool reduced)
{
    Renderer& renderer = *app.renderer();
    renderer.begin_frame();
    renderer.set_view_count(1);
    DrawList list = renderer.view(0);
    list.set_viewport(app.viewport_manager()->fullscreen_viewport());
    list.set_camera(Camera::DEFAULT_CAMERA);
    list.set_filter(TextureFilter::point);
    canvas.draw(list, RectangleF(Vector2F::ZERO, app.resolution_manager()->resolution_vec()));
    if (art != nullptr) { art->draw(list, bounds, text, age, reduced); }
    return read(renderer);
}

RectangleI countdown_crop_bounds(const RectangleF& area)
{
    const int left = static_cast<int>(std::floor(area.x));
    const int top = static_cast<int>(std::floor(area.y));
    return RectangleI(left, top, static_cast<int>(std::ceil(area.right())) - left,
        static_cast<int>(std::ceil(area.bottom())) - top);
}

void countdown_glyph_matrix(Application& app, const Canvas& canvas,
    const Presentation& presentation, const std::filesystem::path& output)
{
    const MatchPresentation& look = presentation.match();
    const CountdownArt art(&look, &presentation.opening(), app.render_resources());
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        const float scale = static_cast<float>(size.y) / look.reference_size.y;
        const RectangleF box(Vector2F(static_cast<float>(size.x) / 2.0f - 130.0f * scale,
            static_cast<float>(size.y) / 2.0f - 100.0f * scale), Vector2F(260, 200) * scale);
        const Frame blank = countdown_fixture(app, canvas, nullptr, box, L"", 0, false);
        std::vector<std::vector<unsigned char>> rasters;
        for (int glyph = 0; glyph <= 10; ++glyph)
        {
            if (size.x != 1280 && glyph != 3 && glyph != 10) { continue; }
            const std::wstring text = glyph == 10 ? L"GO!" : std::to_wstring(glyph);
            const std::string stem = "countdown-glyph-" + std::to_string(glyph) + "-" + std::to_string(size.x);
            const auto draw = [&](float age, bool reduced)
            {
                return countdown_fixture(app, canvas, &art, box, text, age, reduced);
            };
            const Frame settled = draw(look.countdown_art.beat_duration, false);
            const Frame reduced = draw(look.countdown_art.beat_duration * 0.25f, true);
            require(settled.pixels == reduced.pixels && settled.pixels == draw(-1.0f, false).pixels,
                "Reduced or negative/expired countdown age did not retain the settled signal");
            require(foreground_local_delta(settled, blank, box) > 50,
                "Countdown glyph was absent or escaped its supplied rectangle");
            for (const auto& previous : rasters)
            {
                require(previous != settled.pixels, "Two countdown glyphs rendered identically");
            }
            rasters.push_back(settled.pixels);
            const Frame beat = draw(look.countdown_art.beat_duration * 0.25f, false);
            require(foreground_local_delta(beat, settled, box) > 0,
                "Countdown beat supplied no local accent or reduced-effects suppression");
            std::srand(39481);
            const int expected_random = std::rand();
            std::srand(39481);
            for (int repeat = 0; repeat < 3; ++repeat)
            {
                require(beat.pixels == draw(look.countdown_art.beat_duration * 0.25f, false).pixels,
                    "Frozen countdown glyph drawing changed pixels");
            }
            require(std::rand() == expected_random, "Countdown glyph drawing consumed random numbers");
            save(foreground_crop(settled, countdown_crop_bounds(box)), output / (stem + "-settled-detail.ppm"));
            save(foreground_crop(beat, countdown_crop_bounds(box)), output / (stem + "-beat-detail.ppm"));
        }
    }
}

std::unique_ptr<Level> countdown_level(Application& app, LevelBuilder& builder,
    int count, OpeningMode opening)
{
    MenuLevelSettings settings;
    settings.set_player_count(count);
    settings.set_stage("movement_course");
    settings.set_duration(10.0f);
    settings.set_screen_layout(count == 1 ? ScreenLayout::one_player :
        (count == 2 ? ScreenLayout::two_player : ScreenLayout::four_player));
    constexpr int slots[] = { input_slots::keyboard_and_mouse, 3, 1, 0 };
    settings.begin_input_claims(slots[0]);
    for (int view = 1; view < count; ++view)
    {
        require(settings.claim_input_slot(slots[view]), "Countdown input claim failed");
    }
    for (int view = 0; view < count; ++view)
    {
        settings.set_player_team(view, view % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
        settings.set_player_weapon(view, WeaponType::roller);
    }
    app.viewport_manager()->set_layout(settings.screen_layout());
    std::srand(9409 + count);
    return builder.build_level(settings, TeamColourPair::blue_orange, opening);
}

void countdown_capture(Application& app, Level& level, PlaySettings& preferences,
    const RectangleF& card, const std::filesystem::path& output, const std::string& stem,
    std::ofstream& records, bool reduced_pair = false)
{
    const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
    const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
    std::srand(39481);
    const int expected_random = std::rand();
    std::srand(39481);
    const Frame first = render(level, *app.renderer());
    for (int repeat = 0; repeat < 3; ++repeat)
    {
        require(first.pixels == render(level, *app.renderer()).pixels,
            "Frozen countdown scene drawing changed pixels");
    }
    require(std::rand() == expected_random, "Countdown scene drawing consumed random numbers");
    save(first, output / (stem + "-context.ppm"));
    save(foreground_crop(first, countdown_crop_bounds(card)), output / (stem + "-detail.ppm"));
    if (reduced_pair)
    {
        const RestoreFlag restore{ preferences.reduce_effects, preferences.reduce_effects };
        preferences.reduce_effects = true;
        const Frame reduced = render(level, *app.renderer());
        save(foreground_crop(reduced, countdown_crop_bounds(card)), output / (stem + "-reduced-detail.ppm"));
    }
    require(before == observation(level), "Countdown drawing changed public simulation or cosmetic state");
#if COLOURWARS_DEV_TOOLS
    require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
        "Countdown drawing changed the replay fingerprint");
#endif
    records << stem << ',' << level.player_count() << ',' << static_cast<int>(level.state()) << ','
        << first.width << ',' << first.height << ',' << card.x << ',' << card.y << ','
        << card.width << ',' << card.height << '\n';
}

void countdown_live_signal(Application& app, Level& level, const Presentation& presentation,
    const RectangleF& area, bool visible)
{
    auto& art = const_cast<CountdownArtPresentation&>(presentation.match().countdown_art);
    const CountdownArtPresentation original = art;
    const Frame normal = render(level, *app.renderer());
    art.ink.a = 0.0f;
    art.shade.a = 0.0f;
    art.go.a = 0.0f;
    art.number.a = 0.0f;
    const Frame hidden = render(level, *app.renderer());
    art = original;
    const size_t changed = foreground_local_delta(normal, hidden, area);
    require(visible ? changed > 0 : changed == 0,
        "GO signal was missing, expired early or survived its one-second beat");
}

void countdown_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
    const GameTuning& tuning, const Presentation& presentation, PlaySettings& preferences,
    const std::filesystem::path& output)
{
    const RestoreFlag restore_reduced{ preferences.reduce_effects, preferences.reduce_effects };
    preferences.reduce_effects = false;
    countdown_glyph_matrix(app, canvas, presentation, output);
    std::ofstream records(output / "countdown.csv");
    records << "capture,players,state,width,height,card_x,card_y,card_width,card_height\n";
    const auto inputs = neutral_inputs();
    for (int count : { 1, 2, 4 })
    {
        for (const OpeningMode opening : { OpeningMode::off, OpeningMode::quick, OpeningMode::full })
        {
            // Solo practice supplies off. Cover every multiplayer opening at
            // a different native display size, plus all three solo sizes.
            if (count == 1 && opening != OpeningMode::off) { continue; }
            const int variants = count == 1 ? 3 : 1;
            for (int variant = 0; variant < variants; ++variant)
            {
                const int index = count == 1 ? variant :
                    (opening == OpeningMode::off ? 0 : (opening == OpeningMode::quick ? 1 : 2));
                const Vector2I sizes[] = { { 1280, 720 }, { 1920, 1080 }, { 2560, 1440 } };
                auto level = countdown_level(app, builder, count, opening);
                resize(app, *level, sizes[index]);
                for (int tick = 0; tick < 1800 && level->state() == LevelState::opening; ++tick)
                {
                    level->update(inputs, dt);
                }
                require(level->state() == LevelState::start_countdown && !level->match_started(),
                    "Opening did not enter its frozen countdown");
                const auto cards = level->opening_cards();
                require(cards.size() == static_cast<size_t>(count == 1 ? 2 : 3),
                    "Countdown lost its keyboard/pad-specific reminder cards");
                for (const RectangleF& area : cards)
                {
                    require(area.x >= 0 && area.y >= 0 && area.right() <= static_cast<float>(sizes[index].x) &&
                        area.bottom() <= static_cast<float>(sizes[index].y), "Countdown card escaped the display");
                }
                const RectangleF card = cards.front();
                const char* mode = opening == OpeningMode::off ? "off" :
                    (opening == OpeningMode::quick ? "quick" : "full");
                const std::string prefix = "countdown-" + std::to_string(count) + "p-" + mode + "-" +
                    std::to_string(sizes[index].x) + '-';
                const auto snap = [&](const char* beat, bool reduced = false)
                {
                    countdown_capture(app, *level, preferences, card, output, prefix + beat, records, reduced);
                };
                snap("initial");
                // Returning to the native size while frozen must restore all
                // fitted geometry and the exact image, without an update.
                const Frame native = render(*level, *app.renderer());
                resize(app, *level, sizes[(index + 1) % 3]);
                resize(app, *level, sizes[index]);
                require(native.pixels == render(*level, *app.renderer()).pixels,
                    "Countdown resize round-trip retained stale scale or placement");
                level->update(inputs, 0.0f);
                require(native.pixels == render(*level, *app.renderer()).pixels,
                    "Zero-time countdown update advanced its presentation");
                level->suspend();
                snap("paused");
                level->resume();
                level->update(inputs, 0.05f);
                snap("beat", true);
                level->update(inputs, 0.45f);
                snap("settled", true);
                const float remaining = tuning.match().countdown - 0.5f;
                require(remaining > 0.001f, "Countdown fixture needs at least half a second");
                level->update(inputs, remaining - 0.001f);
                require(!level->match_started(), "Countdown released controls before GO");
                snap("before-go");
                level->update(inputs, 0.002f);
                require(level->match_started() && level->state() == LevelState::active &&
                    level->opening_cards().empty(), "GO did not release controls and clear reminder cards");
                snap("go", true);
                countdown_live_signal(app, *level, presentation, card, true);
                for (int tick = 0; tick < 54; ++tick) { level->update(inputs, dt); }
                snap("go-tail");
                countdown_live_signal(app, *level, presentation, card, true);
                for (int tick = 0; tick < 7; ++tick) { level->update(inputs, dt); }
                snap("expired");
                countdown_live_signal(app, *level, presentation, card, false);
            }
        }
    }
    require(records.good(), "Countdown capture records could not be written");
    std::cout << "PASS countdown: distinct 0-9/GO glyphs, local beat/reduced/expired rasters at native "
        "720p/1080p/QHD; real solo and 2P/4P full/quick/off openings, device reminders, GO boundary, "
        "pause, zero-time updates, resize round-trip, frozen pixels/state/draw RNG and Debug fingerprint\n";
}
