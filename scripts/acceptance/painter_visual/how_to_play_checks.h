// Real menu states and real D3D11 rasters, driven by owned synthetic inputs.
void how_to_play_matrix(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    const acceptance::IsolatedSave isolated(output / "learn-profile");
    Save preferences; SettingsSession session(preferences);
    PlaySettings quiet; quiet.music_volume = quiet.sfx_volume = 0; quiet.reduce_effects = true;
    require(session.set_play_settings_and_save(quiet), "Cannot seed isolated help profile");
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_settings_session(&session);
    data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MatchSetup setup;
    MenuContext context(&data, &input, &setup); StateContext& stack = app;
    struct ClearLearn { StateContext& stack; ~ClearLearn() { stack.clear(); painter_scripted_pads::states = {}; } } clear{stack};
    bool pad = false;
    const auto tick = [&] { poll_gamepads(app); app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.51f); };
    const auto tap = [&](Key key, GamepadButton button)
    {
        const auto bit = static_cast<uint16_t>(1u << static_cast<unsigned int>(button));
        if (pad) painter_scripted_pads::states[0].buttons = bit;
        else app.keyboard()->on_key_down(key);
        tick();
        painter_scripted_pads::states[0].buttons = 0; app.keyboard()->on_key_up(key); tick();
        // A transition resets directions: observe a neutral frame on its new page.
        tick();
    };
    const auto open = [&](bool help, bool focused = false)
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (Key key : {Key::enter, Key::escape, Key::left, Key::right, Key::up, Key::down}) app.keyboard()->on_key_up(key);
        painter_scripted_pads::states = {}; painter_scripted_pads::states[0].connected = pad;
        poll_gamepads(app); poll_gamepads(app); app.keyboard()->poll(); app.keyboard()->poll(); input.reset_actions();
        stack.push(std::make_unique<CanvasProbeRoot>(app, canvas));
        if (help) stack.push(std::make_unique<MainMenuHowToPlay>(&context));
        else stack.push(std::make_unique<MainMenuHome>(&context,
            focused ? MainMenuHome::InitialFocus::how_to_play : MainMenuHome::InitialFocus::play));
        tick();
    };
    const auto capture = [&](const std::string& name)
    {
        const Frame first = stack_frame(app);
        require(first.pixels == stack_frame(app).pixels, "Help frozen draw changed pixels");
        save(first, output / (name + ".ppm")); return first;
    };
    for (const Vector2I size : {Vector2I(1280,720), Vector2I(1920,1080)})
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x,size.y);
        for (bool use_pad : {false,true})
        {
            pad = use_pad;
            const std::string prefix = "learn-" + std::to_string(size.y) + (pad ? "-pad" : "-keys");
            open(true); const Frame expected = stack_frame(app);
            open(false,true); const Frame restored = stack_frame(app);
            open(false); capture(prefix + "-home");
            tap(Key::right, GamepadButton::dpad_right); tap(Key::enter, GamepadButton::a);
            require(expected.pixels == stack_frame(app).pixels, "Home did not open actual first guide page");
            const Frame first = capture(prefix + "-start");
            tap(Key::enter,GamepadButton::a); const Frame controls = capture(prefix + "-controls");
            tap(Key::enter,GamepadButton::a); const Frame finish = capture(prefix + "-finish");
            require(first.pixels != controls.pixels && controls.pixels != finish.pixels,
                "Help pages did not expose distinct content");
            tap(Key::enter,GamepadButton::a);
            require(first.pixels == stack_frame(app).pixels, "Guide did not cycle back to first page");
            tap(Key::escape,GamepadButton::b);
            require(restored.pixels == stack_frame(app).pixels, "Back did not restore Home help focus");
            tap(Key::enter,GamepadButton::a);
            require(first.pixels == stack_frame(app).pixels, "Restored Home did not reopen guide");
            tap(Key::right,GamepadButton::dpad_right); capture(prefix + "-back-focused"); tap(Key::enter,GamepadButton::a);
            capture(prefix + "-after-back");
            require(restored.pixels == stack_frame(app).pixels, "Visible Back did not return to Home");
            require(setup.player_count() == 0 && setup.stage().empty(), "Guide silently changed the match plan");
        }
    }
    pad = false; open(true); tap(Key::enter,GamepadButton::a);
    const Frame original = stack_frame(app);
    InputBindings changed = session.input_bindings(); changed.keyboard_jump = Key::j;
    require(session.set_input_bindings_and_save(changed), "Cannot save isolated remapped Jump");
    open(true); tap(Key::enter,GamepadButton::a);
    require(original.pixels != stack_frame(app).pixels, "Guide ignored saved keyboard binding");
    capture("learn-remapped-controls");
    const Frame keyboard_remapped = stack_frame(app);
    changed.pads[3][PadAction::jump].primary = PadControl::x;
    require(session.set_input_bindings_and_save(changed), "Cannot save isolated fourth-pad Jump");
    open(true); tap(Key::enter,GamepadButton::a);
    require(keyboard_remapped.pixels != stack_frame(app).pixels, "Guide ignored distinct fourth-pad binding");
    capture("learn-per-pad-controls");
    std::cout << "PASS optional Home guide: keyboard/synthetic pad, 720p/1080p, page cycle, both Back routes, retained focus and saved keyboard/fourth-pad Jump; no physical controller or full-match claim\n";
}
