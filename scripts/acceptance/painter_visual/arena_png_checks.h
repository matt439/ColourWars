namespace arena_png_acceptance
{
    // The painter inspects the existing level through both draw modes.
    struct LevelMember
    {
        using Type = std::unique_ptr<Level> GameLevel::*;
        friend Type level_member(LevelMember);
    };
    template<LevelMember::Type Member> struct Access
    {
        friend LevelMember::Type level_member(LevelMember) { return Member; }
    };
    template struct Access<&GameLevel::level_>;
}

// The actual timed GameLevel opens Results and exports a PNG through its menu.
void arena_png_checks(Application& app, Registry<LevelLoadedInfo>& levels, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    const acceptance::IsolatedSave profile(L"arena-png");
    const std::filesystem::path screens = profile.save_path().parent_path() / L"screens";
    Save preferences;
    seed_play_settings(preferences, [](PlaySettings& play) { play.music_volume = play.sfx_volume = 0; });
    SettingsSession settings_session(preferences);
    const StageList stages{ acceptance::shipped_stage(levels, "crossfeed") };
    const StageCatalog catalog = acceptance::catalog_of(levels, stages, { "crossfeed" });
    GameData data;
    data.set_application(&app);
    data.set_save(&preferences);
    data.set_settings_session(&settings_session);
    data.set_stage_catalog(&catalog);
    data.set_tuning(&tuning);
    data.set_presentation(&presentation);

    StateContext& stack = app;
    struct RestoreInput
    {
        Application& app;
        bool active;
        bool focused;
        ~RestoreInput()
        {
            this->app.clear();
            this->app.keyboard()->on_key_up(Key::right);
            this->app.keyboard()->on_key_up(Key::enter);
            this->app.keyboard()->poll();
            this->app.keyboard()->poll();
            this->app.notify_activation(this->active);
            this->app.keyboard()->set_focused(this->focused);
        }
    } restore{ app, stack.active(), app.keyboard()->focused() };
    stack.clear();
    stack.notify_activation(true);
    app.keyboard()->set_focused(true);
    app.keyboard()->poll();
    app.keyboard()->poll();

    MatchSetup setup;
    setup.set_stage("crossfeed");
    setup.set_duration(3.0f);
    setup.set_player_count(2);
    setup.begin_input_claims(input_slots::keyboard_and_mouse);
    setup.set_player_bot(1, BotDifficulty::normal);
    setup.set_player_team(0, PlayerTeam::a);
    setup.set_player_team(1, PlayerTeam::b);
    setup.set_player_weapon(0, WeaponType::sprayer);
    setup.set_player_weapon(1, WeaponType::sprayer);
    setup.set_screen_layout(ScreenLayout::one_player);
    stack.push(std::make_unique<ResumeProbeRoot>());
    auto match = std::make_unique<GameLevel>(&data, setup, MatchStart::menu);
    GameLevel* const game = match.get();
    stack.push<EndMenuAction>(std::move(match), [](const EndMenuAction&) {});

    const auto tick = [&](float elapsed)
    {
        poll_gamepads(app);
        app.keyboard()->poll();
        app.mouse()->poll();
        stack.update(elapsed);
    };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key);
        tick(0.0f);
        app.keyboard()->on_key_up(key);
        tick(0.0f);
    };
    for (int step = 0; step < 2400 && stack.depth() == 2; ++step) { tick(dt); }
    require(stack.depth() == 3 && game->level().state() == LevelState::finished,
        "Timed round did not open its production Results page");
    for (const float wait : { presentation.results_menu().team_fill_delay,
        presentation.results_menu().team_fill_time, presentation.results_menu().show_results_delay })
    {
        tick(wait + 1.0f);
    }
    tick(0.0f);
    require(!std::filesystem::exists(screens), "The round wrote a PNG before choosing Save");
    app.renderer()->begin_frame();
    game->draw(*app.renderer());
    const Frame reveal = read(*app.renderer());
    Level& level = *(game->*level_member(arena_png_acceptance::LevelMember{}));
    level.set_arena_view(true);
    require(level.scene_view_count() == 1 && level.scene_view(0).viewport.pixel_rect() ==
        app.viewport_manager()->fullscreen_viewport().pixel_rect(), "Arena export is not framed fullscreen");
    const Frame arena = render(level, *app.renderer());
    level.set_arena_view(false);
    const Frame results = stack_frame(app);
    require(arena.pixels != reveal.pixels && arena.pixels != results.pixels,
        "The arena frame still contains the reveal room or Results overlay");
    save(results, output / "arena-export-results.ppm");

    // A regular file where the screens directory belongs makes the write fail.
    acceptance::write_text(screens, "blocked");
    tap(Key::right);
    save(stack_frame(app), output / "arena-export-save-focused.ppm");
    const std::filesystem::path refusal_log = output / "arena-export-refusal.log";
    acceptance::StderrCapture refused(refusal_log);
    tap(Key::enter);
    refused.finish();
    require(stack.depth() == 3 && acceptance::read_text(screens) == "blocked" &&
        acceptance::read_text(refusal_log).find("Arena capture: writing the file failed") != std::string::npos,
        "A failed save left Results, replaced the blocking file, or lacked a diagnostic");
    require(std::filesystem::remove(screens), "Could not remove private save-directory blocker");
    tap(Key::enter);
    require(stack.depth() == 3 && std::filesystem::is_directory(screens),
        "The Save retry did not keep Results open and create the screens directory");
    require(!level.arena_view(), "Saving left the Results background in export-only arena mode");
    const std::filesystem::directory_iterator png(screens);
    require(png != std::filesystem::directory_iterator{} && png->is_regular_file() &&
        png->path().extension() == L".png" &&
        png->path().filename().wstring().starts_with(L"colourwars-arena-"),
        "The action did not publish its arena PNG");
    const std::filesystem::path path = png->path();
    require(std::distance(std::filesystem::directory_iterator(screens),
        std::filesystem::directory_iterator()) == 1, "More than one file remained after Save");

    Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
    require(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(factory.GetAddressOf()))), "Could not initialize the PNG inspection factory");
    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    require(SUCCEEDED(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
        WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf())), "Saved arena could not be decoded");
    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> decoded;
    require(SUCCEEDED(decoder->GetFrame(0, decoded.GetAddressOf())), "Saved arena has no PNG frame");
    UINT width = 0, height = 0;
    WICPixelFormatGUID format{};
    require(SUCCEEDED(decoded->GetSize(&width, &height)) &&
        SUCCEEDED(decoded->GetPixelFormat(&format)) && format == GUID_WICPixelFormat24bppBGR &&
        width == static_cast<UINT>(arena.width) && height == static_cast<UINT>(arena.height),
        "Saved arena PNG does not match the back-buffer size or opaque RGB format");
    std::vector<unsigned char> bgr(static_cast<size_t>(width) * height * 3);
    require(SUCCEEDED(decoded->CopyPixels(nullptr, width * 3, static_cast<UINT>(bgr.size()), bgr.data())),
        "Could not read the saved arena PNG pixels");
    for (size_t pixel = 0; pixel < bgr.size() / 3; ++pixel)
    {
        if (bgr[pixel * 3] != arena.pixels[pixel * 4 + 2] ||
            bgr[pixel * 3 + 1] != arena.pixels[pixel * 4 + 1] ||
            bgr[pixel * 3 + 2] != arena.pixels[pixel * 4])
        {
            throw std::runtime_error("Saved arena pixel differs from the finished level at " +
                std::to_string(pixel));
        }
    }
    std::filesystem::copy_file(path, output / "arena-export.png",
        std::filesystem::copy_options::overwrite_existing);
    const Frame saved_results = stack_frame(app);
    tap(Key::enter);
    require(stack.depth() == 3 && stack_frame(app).pixels != saved_results.pixels &&
        std::distance(std::filesystem::directory_iterator(screens),
            std::filesystem::directory_iterator()) == 1,
        "Continue after saving did not advance from Results to End without a second PNG");
    std::cout << "PASS arena PNG: explicit save, failed write and retry, exact arena pixels, then Continue\n";
}
