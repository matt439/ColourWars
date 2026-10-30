// Actual Level path selection, independent from the fixture setters used by the
// material/Scene comparisons. First inspect production flags before any test
// setter runs. The only counterfactual deliberately selects the wrong art.
void miniature_path_checks(Application& app, Registry<LevelLoadedInfo>& levels,
    const GameTuning& tuning, const Presentation& presentation, PlaySettings& preferences,
    const std::filesystem::path& output, std::ofstream& records)
{
    const NestedSaveProfile isolated(output / "miniature-path-profile");
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    size_t heavy_contacts = 0;
    std::srand(314159);
    auto fixture = crossfeed_fixture(app, *levels.get("crossfeed"), tuning, presentation,
        preferences, 4, false, 0.75f, heavy_contacts, "crossfeed", std::nullopt, OpeningMode::full);
    require(heavy_contacts == 0, "Miniature path fixture unexpectedly painted its clean stage");
    auto& level = *fixture.level;
    const auto inputs = neutral_inputs();
    size_t mode_rows = 0, counter_rows = 0, readbacks = 0, positive_updates = 0, zero_refreshes = 0;
    const auto check_modes = [&](const char* beat, bool expected)
    {
        const auto modes = miniature_fixture_modes(*fixture.scene);
        require(!modes.empty() && modes.size() == fixture.structures.size(),
            "Miniature path fixture lost its stable authored Structure roles");
        const std::string message = std::string("Actual Level chose the wrong structure path at ") + beat;
        for (bool mode : modes) { require(mode == expected, message.c_str()); }
        const Vector2F size = app.resolution_manager()->resolution_vec();
        records << (expected ? "path-mode-miniature," : "path-mode-ordinary,") << "crossfeed-" << beat <<
            ",4," << size.x << ',' << size.y << ',' << static_cast<int>(level.state()) << ",0\n";
        require(++mode_rows <= 11, "Miniature path fixture exceeded its declared state rows");
    };
    const auto step = [&]()
    {
        require(++positive_updates <= 4800, "Miniature path fixture exceeded its source-bounded update budget");
        level.update(acceptance::slot_seats(level, inputs), dt);
    };
    const auto advance = [&](float seconds)
    { for (int tick = 0; tick < static_cast<int>(std::ceil(seconds / dt)); ++tick) { step(); } };
    require(level.state() == LevelState::opening, "Miniature path construction skipped its full opening");
    check_modes("constructed-opening", true);
    level.update(acceptance::slot_seats(level, inputs), 0.0f); ++zero_refreshes;
    check_modes("zero-opening", true);
    advance(tuning.match().opening.settle + tuning.match().opening.preview * 0.5f);
    require(level.state() == LevelState::opening, "Miniature path fixture missed its preview");
    check_modes("preview", true);
    for (int tick = 0; tick < 2400 && level.state() == LevelState::opening; ++tick) { step(); }
    require(level.state() == LevelState::start_countdown, "Miniature path fixture missed its countdown");
    check_modes("countdown", true);
    advance(std::max(0.0f, tuning.match().countdown - tuning.match().opening.push * 0.5f));
    require(level.state() == LevelState::start_countdown, "Miniature path fixture missed its glass push");
    check_modes("push", true);
    for (int tick = 0; tick < 1200 && level.state() != LevelState::active; ++tick) { step(); }
    require(level.state() == LevelState::active, "Miniature path fixture did not reach active play");
    check_modes("active", false);

    // Each actual four-player pane is 160x240. Follow selects 160/1280 = 1/8;
    // visible height 1920 centres the 500..1380 camera bounds at Y = -20.
    // This exposes the authored top-only floors' sealed bottom at 1680:
    // ordinary keyline 1676..1680 projects 212..212.5, both truncated to 212.
    // Miniature drawing keeps one black row 211 inside that same bottom 212.
    // Top primer and the HUD/centred GO art do not cover local (32,211).
    resize(app, level, { 320, 480 });
    check_modes("active-small", false);
    float minimum_scale = std::numeric_limits<float>::max(), maximum_scale = 0.0f;
    for (int view = 0; view < level.player_count(); ++view)
    {
        const Camera camera = level.view_camera(view);
        const float scale = camera.scale;
        require(scale == 0.125f && camera.translation.y == -20.0f &&
            (camera.translation.x == 0.0f || camera.translation.x == 2320.0f),
            "Miniature active counterexample changed its source-derived camera or floor witness");
        minimum_scale = std::min(minimum_scale, scale); maximum_scale = std::max(maximum_scale, scale);
    }
    const auto before = observation(level);
    const auto structures = crossfeed_structure_state(fixture);
#if COLOURWARS_DEV_TOOLS
    const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
    std::srand(61024); const int expected_random = std::rand(); std::srand(61024);
    const Frame ordinary = render(level, *app.renderer()); ++readbacks;
    require(ordinary.width == 320 && ordinary.height == 480,
        "Miniature active counterexample changed its declared native dimensions");
    require(miniature_fixture_modes(*fixture.scene) == std::vector<bool>(fixture.structures.size(), false),
        "Ordinary actual-Level draw mutated its path flags");
    configure_miniature_fixture(*fixture.scene, true);
    const Frame wrong = render(level, *app.renderer()); ++readbacks;
    require(miniature_fixture_modes(*fixture.scene) == std::vector<bool>(fixture.structures.size(), true),
        "Forced miniature counterfactual changed flags during drawing");
    const size_t changed = miniature_changed_pixels(ordinary, wrong);
    const RectangleI witness_pane = fixture.scene->view(0).viewport.pixel_rect();
    require(witness_pane.x == 0 && witness_pane.y == 0 &&
        witness_pane.width == 160 && witness_pane.height == 240,
        "Miniature floor-bottom witness changed its declared actual player pane");
    // Literal authored roles: metal_dark 6E7275 versus keyline 000000. This
    // geometry/colour witness is selected from source, not measured captures.
    require(monitor_rgb(ordinary, 32, 211) == 0x6E7275u &&
        monitor_rgb(wrong, 32, 211) == 0x000000u,
        "Wrong miniature mode did not change the exposed floor-bottom witness");
    require(changed > 0, "Low-scale active fixture cannot distinguish an incorrect scale-selected miniature rule");
    // Only ordinary update/refresh restores the flag; do not hand-reset it.
    level.update(acceptance::slot_seats(level, inputs), 0.0f); ++zero_refreshes;
    check_modes("zero-active-restored", false);
    const Frame restored = render(level, *app.renderer()); ++readbacks;
    require(ordinary.pixels == restored.pixels && before == observation(level) &&
        structures == crossfeed_structure_state(fixture) && std::rand() == expected_random,
        "Miniature active counterfactual did not restore exact pixels, state, ownership or RNG");
#if COLOURWARS_DEV_TOOLS
    require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
        "Miniature active counterfactual changed its Debug replay fingerprint");
#endif
    for (const auto& item : { std::pair{ "ordinary", &ordinary }, std::pair{ "wrong-miniature", &wrong },
        std::pair{ "restored", &restored } })
    {
        save(*item.second, output / (std::string("miniature-terrain-active-320x480-") + item.first + ".ppm"));
        records << "path-counterexample,crossfeed-" << item.first << ",4,320,480," << minimum_scale << ',' <<
            (std::string(item.first) == "wrong-miniature" ? changed : 0) << '\n';
        ++counter_rows;
    }
    resize(app, level, { 1280, 720 });
    for (int tick = 0; tick < 1200 && level.state() == LevelState::active; ++tick) { step(); }
    require(level.state() == LevelState::zoom_out, "Miniature path fixture missed its zoom-out transition");
    check_modes("zoom-out", true);
    advance(tuning.match().zoom_out * 0.5f);
    require(level.state() == LevelState::zoom_out, "Miniature path fixture missed its pullback");
    check_modes("pullback", true);
    for (int tick = 0; tick < 1200 && level.state() == LevelState::zoom_out; ++tick) { step(); }
    require(level.state() == LevelState::overview, "Miniature path fixture missed its overview");
    check_modes("overview", true);
    require(mode_rows == 11 && counter_rows == 3 && readbacks == 3 && zero_refreshes == 2,
        "Miniature path fixture's state/counterfactual/readback accounting changed");
    std::ofstream summary(output / "miniature-path-summary.json");
    summary << "{\"scope\":\"actual-Level-path-flags-and-low-scale-counterexample\",\"stage\":\"crossfeed\","
        "\"players\":4,\"path_rows\":" << mode_rows << ",\"counterexample_rows\":" << counter_rows <<
        ",\"readbacks\":" << readbacks << ",\"positive_time_updates\":" << positive_updates <<
        ",\"zero_dt_refreshes\":" << zero_refreshes << ",\"structures\":" << fixture.structures.size() <<
        ",\"width\":320,\"height\":480,\"minimum_gameplay_scale\":" << minimum_scale <<
        ",\"maximum_gameplay_scale\":" << maximum_scale << ",\"wrong_miniature_changed_pixels\":" << changed <<
        ",\"restored_pixels\":true,\"state_ownership_rng_restored\":true}\n";
    require(summary.good(), "Cannot write miniature path accounting");
    std::cout << "PASS miniature path selection:11 actual-Level state rows,3 native320x480 readbacks; "
        "four-player sub0.25 gameplay stays ordinary, wrong miniature differs and zero-dt update restores it. "
        "No route or human readability acceptance.\n";
}
