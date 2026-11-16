// Focused hidden actual Options -> Graphics route. No desktop input or real save.
void graphics_settings_matrix(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    const acceptance::IsolatedSave isolated(output / "graphics-profile");
    const auto path = acceptance::save_path_in(output / "graphics-profile");
    acceptance::ReplacementLock blocked(path);
    Save preferences; SettingsSession session(preferences);
    PlaySettings initial; initial.music_volume = initial.sfx_volume = 0;
    require(session.set_play_settings_and_save(initial), "Cannot seed isolated Graphics profile");
    require(preferences.set_display_settings_and_save(ScreenResolution::s_1280_720, false,
        TeamColourPair::blue_orange), "Cannot seed isolated Graphics display settings");
    const crt_trial::Runtime runtime(false, std::nullopt);
    const crt_trial::Runtime override_runtime(false, false);
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_settings_session(&session);
    data.set_tuning(&tuning); data.set_presentation(&presentation); data.set_crt_runtime(&runtime);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MatchSetup settings;
    MenuContext context(&data, &input, &settings); StateContext& stack = app;
    struct ClearGraphics { StateContext& stack; ~ClearGraphics() { stack.clear(); } } clear{stack};
    app.resolution_manager()->set_resolution_exactly({1280, 720});
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    stack.clear(); stack.notify_activation(true);
    app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
    for (Key key : {Key::enter, Key::escape, Key::up, Key::down, Key::left, Key::right}) app.keyboard()->on_key_up(key);
    app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); input.reset_actions();
    const auto tick = [&](float elapsed = 0.0f)
    { app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed); };
    const auto tap = [&](Key key)
    { app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick(); tick(0.51f); };
    const auto bytes = [&]
    { std::ifstream file(path, std::ios::binary); return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()); };
    const auto capture = [&](const char* stem)
    {
        const Frame first = stack_frame(app);
        require(first.pixels == stack_frame(app).pixels, "Graphics frozen draw changed pixels");
        save(first, output / (std::string("crt-graphics-") + stem + ".ppm"));
        return first;
    };
    stack.push(std::make_unique<CanvasProbeRoot>(app, canvas));
    stack.push(std::make_unique<MainMenuOptions>(&context)); tick(); tick(0.51f);
    for (int i=0; i<5; ++i) tap(Key::down);
    tap(Key::enter); require(stack.depth()==3, "Options did not open Graphics child");
    const auto unchanged = bytes(); capture("default-720p");
    tap(Key::right); require(!session.play_settings().crt_monitor && bytes()==unchanged,
        "Pending Graphics choice escaped before Apply");
    tap(Key::escape); require(stack.depth()==2 && bytes()==unchanged,
        "Graphics Back did not discard pending choice");
    tap(Key::enter); require(stack.depth()==3, "Graphics did not reopen from retained Options row");
    tap(Key::right); tap(Key::down); tap(Key::enter);
    require(session.play_settings().crt_monitor && !runtime.enabled(),
        "Graphics Apply did not save next launch separately from current runtime");
    Save reread; reread.load_save_file();
    require(reread.saved_play_settings().crt_monitor, "Graphics Apply was not durable");
    const Frame saved = capture("saved-restart-720p");
    tap(Key::escape); require(stack.depth()==2, "Graphics Back skipped Options");
    tap(Key::down); tap(Key::down); tap(Key::enter); // Options' own Apply.
    require(session.play_settings().crt_monitor && preferences.saved_play_settings().crt_monitor,
        "Parent Options Apply replaced child Graphics preference");
    tap(Key::up); tap(Key::up); tap(Key::enter); // Return to Graphics.
    require(stack.depth()==3, "Graphics did not reopen after parent Apply");
    tap(Key::left); tap(Key::down);
    const auto durable = bytes(); require(blocked.hold(), "Cannot lock isolated Graphics save");
    {
        acceptance::StderrCapture errors(output / "graphics-save-refusal.log");
        tap(Key::enter);
        errors.finish();
    }
    require(session.play_settings().crt_monitor && bytes()==durable && !runtime.enabled(),
        "Failed Graphics write changed saved/active choice");
    const Frame failed = capture("save-failed-720p");
    require(saved.pixels != failed.pixels, "Graphics save failure gave no visible change");
    require(blocked.release(), "Cannot release isolated Graphics save");
    tap(Key::enter); require(!session.play_settings().crt_monitor && !runtime.enabled(),
        "Graphics recovered save failed or changed current runtime");
    capture("off-saved-720p");
    tap(Key::escape); // Retained Options Graphics row.
    data.set_crt_runtime(&override_runtime);
    tap(Key::enter); tap(Key::right); tap(Key::down); tap(Key::enter);
    require(session.play_settings().crt_monitor && !override_runtime.enabled() && override_runtime.overridden(),
        "Override Graphics save changed launch behavior");
    capture("override-saved-720p");
    std::cout << "PASS Graphics child: pending Back, saved restart, failed-write rollback, parent preservation, frozen 720p captures\n";
}
