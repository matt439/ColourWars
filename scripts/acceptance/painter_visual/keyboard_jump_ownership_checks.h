// Included after keyboard_jump_checks.h, only in the focused scripted-pad
// binary. All input is private snapshot data or a message to this probe's HWND.
#if defined(COLOURWARS_PAINTER_SCRIPTED_PADS)
void keyboard_jump_ownership_matrix(Application& app, Registry<LevelLoadedInfo>& levels,
    const Canvas& canvas, GameTuning& tuning, Presentation& presentation,
    const std::filesystem::path& content, const std::filesystem::path& output)
{
    const auto profile = output / "keyboard-jump-ownership-profile";
    const NestedSaveProfile isolated(profile);
    Save preferences; PlaySettings quiet; quiet.music_volume = quiet.sfx_volume = 0;
    InputBindings initial; initial.keyboard_jump = Key::j;
    require(preferences.set_play_settings_and_save(quiet) && preferences.set_input_bindings_and_save(initial),
        "Cannot seed private Jump ownership preferences");
    const auto bytes = [&]
    {
        std::ifstream file(profile / "ColourWars/save_data.json", std::ios::binary);
        require(file.good(), "Private Jump ownership save disappeared");
        return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    };
    const std::string saved = bytes();
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false } };
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_tuning(&tuning);
    data.set_presentation(&presentation); data.set_level_infos(&levels); data.set_stages(&stages);
    DebugDirector director(&tuning, &presentation); data.set_debug(&director);
#if COLOURWARS_DEV_TOOLS
    director.impl()->set_content_directories(content, content);
    director.impl()->set_user_directory(profile / "debug");
#else
    static_cast<void>(content);
#endif
    director.attach(&app, &data);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MatchSetup settings;
    settings.set_player_count(2); settings.begin_input_claims(0);
    require(settings.claim_input_slot(1), "Cannot build Jump ownership participant roster");
    MenuContext context(&data, &input, &settings); StateContext& stack = app;
    struct RestoreJumpOwnership
    {
        StateContext& stack; Application& app;
        ~RestoreJumpOwnership()
        {
            this->stack.clear(); painter_scripted_pads::states = {};
            this->app.gamepads()->poll(); this->app.gamepads()->poll();
            this->app.keyboard()->set_focused(true);
            for (unsigned int key = 1; key < static_cast<unsigned int>(Key::count); ++key)
                { this->app.keyboard()->on_key_up(static_cast<Key>(key)); }
            this->app.keyboard()->poll(); this->app.keyboard()->poll();
        }
    } restore{ stack, app };
    int updates = 0, cases = 0; KeyboardJumpMenu* page = nullptr;
    std::ofstream records(output / "keyboard-jump-ownership.csv");
    records << "case,capture_active,pending,active,stack_depth,updates,save_unchanged\n";
    const auto tick = [&](float elapsed = 0.0f)
    {
        require(++updates <= 240, "Jump ownership exceeded its bounded update budget");
        app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed);
    };
    const auto tap_key = [&](Key key)
        { app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick(); };
    const auto mask = [](GamepadButton button) { return static_cast<uint16_t>(1u << static_cast<unsigned int>(button)); };
    const auto tap_pad = [&](int slot, GamepadButton button)
    {
        auto& pad = painter_scripted_pads::states[static_cast<size_t>(slot)];
        pad.buttons = mask(button); tick(); pad.buttons = 0; tick();
    };
    const auto record = [&](const char* name, bool capturing, Key pending = Key::j)
    {
        const std::string message = std::string("Actual Jump ownership mismatch: ") + name;
        require(stack.depth() == 2 && page->capture_active() == capturing && page->pending_key() == pending &&
            preferences.input_bindings().keyboard_jump == Key::j && bytes() == saved,
            message.c_str());
        records << name << ',' << capturing << ',' << input_key_name(pending) << ",j," << stack.depth() << ',' <<
            updates << ",true\n"; ++cases;
    };
    const auto open = [&](int owner)
    {
        stack.clear(); stack.notify_activation(true); painter_scripted_pads::states = {};
        for (size_t slot = 0; slot < 3; ++slot) { painter_scripted_pads::states[slot].connected = true; }
        app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (unsigned int key = 1; key < static_cast<unsigned int>(Key::count); ++key)
            { app.keyboard()->on_key_up(static_cast<Key>(key)); }
        app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll();
        input.reset_actions(); stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        auto owned = std::make_unique<KeyboardJumpMenu>(&context, owner); page = owned.get(); stack.push(std::move(owned));
        tick();
        // Jump is the Keyboard Keys page's third row, after Move left and Move right.
        for (int row = 0; row < 2; ++row)
        { if (owner >= 0) { tap_pad(owner, GamepadButton::dpad_down); } else { tap_key(Key::down); } }
        require(page->capture_action() == KeyboardAction::jump || !page->capture_active(),
            "Ownership fixture could not reach the Jump row");
        if (owner >= 0) { tap_pad(owner, GamepadButton::a); } else { tap_key(Key::enter); }
        tick(); require(page->capture_active() && page->capture_action() == KeyboardAction::jump,
            "Actual Keyboard Keys menu did not enter Jump capture");
    };

    open(0); app.keyboard()->on_key_down(Key::k); tick();
    tap_pad(1, GamepadButton::b); record("nonowner-back-ignored", true);
    tap_pad(0, GamepadButton::b); record("owner-back-drains-held-key", true);
    app.keyboard()->on_key_up(Key::k); tick(); record("owner-back-cancelled", false);
    open(-1); tap_key(Key::escape); record("keyboard-cancel", false);
    tap_key(Key::space); tick(); record("fresh-space-reopens-capture", true);
    tap_key(Key::space); tick(); record("fresh-space-captured-pending-only", false, Key::space);

    open(0); painter_scripted_pads::states[0].connected = false; tick();
    tap_pad(2, GamepadButton::b); record("spare-blocked-by-participant", true);
    tap_pad(1, GamepadButton::b); record("disconnected-owner-participant-fallback", false);
    open(0); painter_scripted_pads::states[0].connected = false; tick();
    painter_scripted_pads::states[0].connected = true;
    painter_scripted_pads::states[0].buttons = mask(GamepadButton::b); tick();
    painter_scripted_pads::states[0].buttons = 0; tick(); record("reconnected-held-back-not-owned", true);
    tap_pad(0, GamepadButton::b); record("reconnected-fresh-back-cancels", false);
    open(0); painter_scripted_pads::states[0].connected = painter_scripted_pads::states[1].connected = false; tick();
    tap_pad(2, GamepadButton::b); record("all-participants-missing-spare-fallback", false);

#if COLOURWARS_DEV_TOOLS
    open(-1);
    DebugScriptRunner::Frame script; script.name = "ownership-cancelled.cfg";
    script.statements.push_back({ script.name, 1, "stage movement_course" });
    require(director.impl()->execute("stage movement_course").ok && director.impl()->scripts().push(std::move(script)).ok,
        "Cannot queue private developer work during actual capture");
    tick(); record("queued-developer-work-quarantined", true);
    require(director.impl()->request().empty() && !director.impl()->scripts().running(),
        "Actual capture left developer work queued for later");
    tap_key(Key::grave); tap_key(Key::f11); record("reserved-shortcuts-owned", true);
    tap_key(Key::escape); tick(); record("no-deferred-shortcut-or-stage", false);
    require(!director.impl()->console_open() && !director.impl()->debug_menu_open(),
        "A captured developer shortcut opened an overlay after release");

    open(-1); painter_scripted_pads::states[0].buttons = static_cast<uint16_t>(
        mask(GamepadButton::back) | mask(GamepadButton::start));
    tick(0.6f); record("debug-pad-chord-owned", true);
    tap_key(Key::escape); tick(0.6f); record("held-debug-chord-not-deferred", false);
    painter_scripted_pads::states[0].buttons = mask(GamepadButton::start); tick(0.6f);
    record("partial-debug-chord-release-owned", false);
    painter_scripted_pads::states[0].buttons = 0; tick(0.6f); tick(); record("debug-chord-final-release-owned", false);

    // F10 is an ordinary Keyboard key in every build; held through capture it
    // must neither choose a binding nor open the debug menu once released.
    open(-1); app.keyboard()->on_key_down(Key::f10); tick(); tick();
    tap_key(Key::k); record("held-f10-blocks-key-gesture", true);
    tap_key(Key::escape); record("held-f10-cancel-drains", true);
    app.keyboard()->on_key_up(Key::f10); tick(); tick(); record("f10-release-not-deferred", false);
    require(!director.impl()->debug_menu_open(), "F10 held through capture opened the debug menu after release");
    open(-1); app.keyboard()->on_key_down(Key::f10); tick();
    app.keyboard()->set_focused(false); stack.notify_activation(false); tick();
    require(!app.keyboard()->state().is_down(Key::f10), "Focus loss retained held F10");
    app.keyboard()->set_focused(true); stack.notify_activation(true); tick(); tick();
    tap_key(Key::k); record("f10-focus-rearms-fresh-key", false, Key::k);
#endif
    require(cases == (COLOURWARS_DEV_TOOLS ? 22 : 11) && records.good(), "Jump ownership receipt is incomplete");
    std::cout << "PASS Keyboard Jump ownership: " << cases << " actual-menu observations over " << updates <<
        " scripted input updates; owner/fallback/reconnect, cancel drain and fresh Space; "
        "Debug additionally covers queued work, reserved shortcuts/chord and held F10/focus. "
        "No global input or physical-device acceptance.\n";
}
#endif
