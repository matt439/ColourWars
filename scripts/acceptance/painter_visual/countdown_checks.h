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
    int count, OpeningMode opening, const std::string& stage = "movement_course")
{
    MatchSetup settings;
    settings.set_player_count(count);
    settings.set_stage(stage);
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

bool stage_entry_empty(const RectangleF& bounds)
{
    return bounds.x == 0.0f && bounds.y == 0.0f && bounds.width == 0.0f && bounds.height == 0.0f;
}

void stage_entry_hidden(const Level& level)
{
    require(stage_entry_empty(level.stage_entry_bounds()) &&
        stage_entry_empty(level.stage_entry_text_bounds()),
        "Stage entry survived outside the frozen start countdown");
}

void stage_entry_layout(const Level& level, const Vector2F& resolution,
    std::wstring_view expected_name)
{
    require(level.state() == LevelState::start_countdown &&
        level.stage_entry_name() == expected_name,
        "Stage entry did not use the selected stage's authored display name");
    const RectangleF card = level.stage_entry_bounds();
    const RectangleF text = level.stage_entry_text_bounds();
    require(card.width > 0.0f && card.height > 0.0f && card.x >= 0.0f && card.y >= 0.0f &&
        card.right() <= resolution.x && card.bottom() <= resolution.y,
        "Stage entry card escaped the display");
    require(text.width > 0.0f && text.height > 0.0f && text.x > card.x && text.y > card.y &&
        text.right() < card.right() && text.bottom() < card.bottom(),
        "Stage entry text or its shadow escaped the paper card");
    const auto cards = level.opening_cards();
    require(!cards.empty() && cards.back() == card,
        "Stage entry was absent from the developer panel's occupied-card bounds");
    for (size_t index = 0; index + 1 < cards.size(); ++index)
    {
        const RectangleF& other = cards[index];
        require(card.right() <= other.x || other.right() <= card.x ||
            card.bottom() <= other.y || other.bottom() <= card.y,
            "Stage entry overlapped the countdown or device reminders");
    }
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
    const RectangleF entry = level.stage_entry_bounds();
    if (entry.width > 0.0f)
    {
        save(foreground_crop(first, countdown_crop_bounds(entry)), output / (stem + "-stage-entry-detail.ppm"));
    }
    if (reduced_pair)
    {
        const RestoreFlag restore{ preferences.reduce_effects, preferences.reduce_effects };
        preferences.reduce_effects = true;
        const Frame reduced = render(level, *app.renderer());
        save(foreground_crop(reduced, countdown_crop_bounds(card)), output / (stem + "-reduced-detail.ppm"));
        if (entry.width > 0.0f)
        {
            const RectangleI text = countdown_crop_bounds(level.stage_entry_text_bounds());
            require(foreground_crop(first, text, 0).pixels == foreground_crop(reduced, text, 0).pixels,
                "Reduce effects changed the static stage entry's information");
            save(foreground_crop(reduced, countdown_crop_bounds(entry)),
                output / (stem + "-stage-entry-reduced-detail.ppm"));
        }
    }
    require(before == observation(level), "Countdown drawing changed public simulation or cosmetic state");
#if COLOURWARS_DEV_TOOLS
    require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
        "Countdown drawing changed the replay fingerprint");
#endif
    records << stem << ',' << level.player_count() << ',' << static_cast<int>(level.state()) << ','
        << first.width << ',' << first.height << ',' << card.x << ',' << card.y << ','
        << card.width << ',' << card.height << ',' << entry.x << ',' << entry.y << ','
        << entry.width << ',' << entry.height << '\n';
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

void stage_entry_stage_matrix(Application& app, LevelBuilder& builder,
    const GameTuning& tuning, PlaySettings& preferences,
    const std::filesystem::path& output, std::ofstream& records)
{
    struct NamedStage
    {
        const char* stage;
        std::wstring_view name;
    };
    const NamedStage stages[] = {
        { "movement_course", L"Movement Course" },
        { "close_quarters", L"Close Quarters" },
        { "king_of_the_hill", L"King of the Hill" },
        { "turbulence", L"Turbulence" }
    };
    const Vector2I native(1280, 720);
    const auto inputs = neutral_inputs();
    for (const NamedStage& stage : stages)
    {
        // Three players use the same four-pane layout, but a distinct roster.
        const int count = std::string_view(stage.stage) == "close_quarters" ? 3 : 4;
        auto level = countdown_level(app, builder, count, OpeningMode::off, stage.stage);
        resize(app, *level, native);
        stage_entry_layout(*level, app.resolution_manager()->resolution_vec(), stage.name);
        const RectangleF card = level->opening_cards().front();
        const std::string stem = "countdown-stage-" + std::string(stage.stage);
        countdown_capture(app, *level, preferences, card, output, stem, records, true);
        const Frame initial = render(*level, *app.renderer());
        std::vector<Camera> initial_cameras;
        for (int view = 0; view < level->player_count(); ++view) { initial_cameras.push_back(level->player(view).camera()); }
        for (const Vector2I size : { Vector2I(1024, 768), Vector2I(960, 1080), Vector2I(2560, 720) })
        {
            resize(app, *level, size);
            stage_entry_layout(*level, app.resolution_manager()->resolution_vec(), stage.name);
            save(foreground_crop(render(*level, *app.renderer()),
                countdown_crop_bounds(level->stage_entry_bounds())), output /
                (stem + '-' + std::to_string(size.x) + 'x' + std::to_string(size.y) + "-entry-detail.ppm"));
        }
        resize(app, *level, native);
        // The follow camera keeps its deadzone history across a resize, and a
        // narrow pane's minimum world width changes its vertical extent too, so
        // a round trip may leave the world framed elsewhere (polish-crossfeed:
        // no forced recenter). Its scale is a function of the layout alone; put
        // back only the history-dependent translation, then demand the frame.
        for (int view = 0; view < level->player_count(); ++view)
        {
            Player& player = const_cast<Player&>(level->player(view));
            Camera camera = player.camera();
            require(camera.scale == initial_cameras[static_cast<size_t>(view)].scale,
                "Player camera scale did not return with the native layout");
            camera.translation = initial_cameras[static_cast<size_t>(view)].translation;
            player.set_camera(camera);
        }
        level->refresh_layout();
        require(initial.pixels == render(*level, *app.renderer()).pixels,
            "Stage entry retained stale fitting after an aspect-ratio round-trip");
        const int countdown_ticks = static_cast<int>(std::ceil(tuning.match().countdown / dt)) + 2;
        for (int tick = 0; tick < countdown_ticks && !level->match_started(); ++tick)
        {
            level->update(acceptance::slot_seats(*level, inputs), dt);
        }
        require(level->match_started(), "Stage entry delayed the established gameplay start");
        stage_entry_hidden(*level);
        require(level->stage_entry_name() == stage.name,
            "Hiding stage entry changed its authored name");
    }
}

void stage_entry_skip_matrix(Application& app, LevelBuilder& builder,
    const GameTuning& tuning, PlaySettings& preferences,
    const std::filesystem::path& output, std::ofstream& records)
{
    const auto inputs = neutral_inputs();
    for (const OpeningMode opening : { OpeningMode::quick, OpeningMode::full })
    {
        for (const bool skip_countdown : { false, true })
        {
            auto level = countdown_level(app, builder, 2, opening, "close_quarters");
            resize(app, *level, Vector2I(1280, 720));
            require(level->state() == LevelState::opening && level->opening_cards().empty(),
                "Story fixture did not begin before its opening cards");
            stage_entry_hidden(*level);
            if (skip_countdown)
            {
                for (int tick = 0; tick < 1800 && level->state() == LevelState::opening; ++tick)
                {
                    level->update(acceptance::slot_seats(*level, inputs), dt);
                }
                stage_entry_layout(*level, app.resolution_manager()->resolution_vec(), L"Close Quarters");
                require(level->intro_playing(), "Story did not retain its monitor countdown before skip");
            }
            auto skip = acceptance::slot_inputs(inputs);
            skip[static_cast<size_t>(input_slots::keyboard_and_mouse)].skip_intro = true;
            acceptance::update_from_slots(*level, skip, dt);
            require(level->state() == LevelState::start_countdown && !level->intro_playing() &&
                !level->match_started(), "Story skip released controls or bypassed stage entry");
            stage_entry_layout(*level, app.resolution_manager()->resolution_vec(), L"Close Quarters");
            const std::string mode = opening == OpeningMode::quick ? "quick" : "full";
            countdown_capture(app, *level, preferences, level->opening_cards().front(), output,
                "countdown-skip-" + mode + (skip_countdown ? "-monitor" : "-opening"), records, true);
            const int countdown_ticks = static_cast<int>(std::ceil(tuning.match().countdown / dt)) + 2;
            for (int tick = 0; tick < countdown_ticks && !level->match_started(); ++tick)
            {
                level->update(acceptance::slot_seats(*level, inputs), dt);
            }
            require(level->match_started(), "Skipped stage entry delayed gameplay start");
            stage_entry_hidden(*level);
        }
    }
}

void countdown_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
    const GameTuning& tuning, const Presentation& presentation, PlaySettings& preferences,
    const std::filesystem::path& output)
{
    const RestoreFlag restore_reduced{ preferences.reduce_effects, preferences.reduce_effects };
    preferences.reduce_effects = false;
    countdown_glyph_matrix(app, canvas, presentation, output);
    std::ofstream records(output / "countdown.csv");
    records << "capture,players,state,width,height,card_x,card_y,card_width,card_height,"
        "stage_x,stage_y,stage_width,stage_height\n";
    stage_entry_stage_matrix(app, builder, tuning, preferences, output, records);
    stage_entry_skip_matrix(app, builder, tuning, preferences, output, records);
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
                if (level->state() == LevelState::opening)
                {
                    stage_entry_hidden(*level);
                }
                for (int tick = 0; tick < 1800 && level->state() == LevelState::opening; ++tick)
                {
                    level->update(acceptance::slot_seats(*level, inputs), dt);
                }
                require(level->state() == LevelState::start_countdown && !level->match_started(),
                    "Opening did not enter its frozen countdown");
                stage_entry_layout(*level, app.resolution_manager()->resolution_vec(), L"Movement Course");
                const auto cards = level->opening_cards();
                require(cards.size() == static_cast<size_t>(count == 1 ? 3 : 4),
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
                stage_entry_layout(*level, app.resolution_manager()->resolution_vec(), L"Movement Course");
                resize(app, *level, sizes[index]);
                require(native.pixels == render(*level, *app.renderer()).pixels,
                    "Countdown resize round-trip retained stale scale or placement");
                level->update(acceptance::slot_seats(*level, inputs), 0.0f);
                require(native.pixels == render(*level, *app.renderer()).pixels,
                    "Zero-time countdown update advanced its presentation");
                level->update(acceptance::slot_seats(*level, inputs), -dt);
                require(native.pixels == render(*level, *app.renderer()).pixels,
                    "Negative-time countdown update advanced its presentation");
                level->suspend();
                snap("paused");
                level->resume();
                level->update(acceptance::slot_seats(*level, inputs), 0.05f);
                snap("beat", true);
                level->update(acceptance::slot_seats(*level, inputs), 0.45f);
                snap("settled", true);
                const float remaining = tuning.match().countdown - 0.5f;
                require(remaining > 0.001f, "Countdown fixture needs at least half a second");
                level->update(acceptance::slot_seats(*level, inputs), remaining - 0.001f);
                require(!level->match_started(), "Countdown released controls before GO");
                snap("before-go");
                level->update(acceptance::slot_seats(*level, inputs), 0.002f);
                require(level->match_started() && level->state() == LevelState::active &&
                    level->opening_cards().empty(), "GO did not release controls and clear reminder cards");
                stage_entry_hidden(*level);
                snap("go", true);
                countdown_live_signal(app, *level, presentation, card, true);
                for (int tick = 0; tick < 54; ++tick) { level->update(acceptance::slot_seats(*level, inputs), dt); }
                snap("go-tail");
                countdown_live_signal(app, *level, presentation, card, true);
                for (int tick = 0; tick < 7; ++tick) { level->update(acceptance::slot_seats(*level, inputs), dt); }
                snap("expired");
                stage_entry_hidden(*level);
                countdown_live_signal(app, *level, presentation, card, false);
            }
        }
    }
    require(records.good(), "Countdown capture records could not be written");
    std::cout << "PASS countdown: distinct 0-9/GO glyphs, local beat/reduced/expired rasters at native "
        "720p/1080p/QHD; real solo and 2P/4P full/quick/off openings, device reminders, GO boundary, "
        "all authored stage names and bounded fitted titles, three-player roster, narrow/wide aspect ratios, "
        "story/monitor skips, pause, nonpositive updates, resize round-trip, frozen pixels/state/draw RNG "
        "and Debug fingerprint\n";
}
