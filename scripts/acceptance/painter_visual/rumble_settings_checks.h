// Actual per-slot settings navigation, using the focused painter input backend.
// The painter factory is always fake: this page never auditions a motor pulse.
#if defined(COLOURWARS_PAINTER_SCRIPTED_PADS)
void rumble_settings_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    const auto profile = output / "rumble-settings-profile";
    const NestedSaveProfile isolated(profile);
    const auto save_path = profile / "ColourWars/save_data.json";
    const auto blocked = std::filesystem::path(save_path.wstring() + L".tmp");
    Save preferences;
    PlaySettings initial;
    initial.music_volume = initial.sfx_volume = 0;
    initial.sticks[1] = { 25, 35 };
    initial.sticks[3] = { 30, 40 };
    require(preferences.set_play_settings_and_save(initial), "Cannot seed private rumble settings");
    const InputBindings bindings = preferences.input_bindings();
    GameData data;
    data.set_application(&app); data.set_save(&preferences);
    data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings);
    StateContext& stack = app;
    struct ClearRumbleSettings
    {
        StateContext& stack;
        Application& app;
        std::filesystem::path blocked;
        ~ClearRumbleSettings()
        {
            this->stack.clear(); painter_scripted_pads::states = {};
            this->app.gamepads()->poll(); this->app.gamepads()->poll();
            std::error_code ignored; std::filesystem::remove(this->blocked, ignored);
        }
    } clear{ stack, app, blocked };
    const auto bytes = [&]
    {
        std::ifstream stream(save_path, std::ios::binary);
        require(stream.good(), "Rumble settings private save disappeared");
        return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    };
    const auto tick = [&](float elapsed = 0.0f)
    {
        app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed);
    };
    const auto pad = [&](int slot, GamepadButton button)
    {
        painter_scripted_pads::states[static_cast<size_t>(slot)].buttons =
            static_cast<std::uint16_t>(1u << static_cast<unsigned int>(button));
        tick(); painter_scripted_pads::states[static_cast<size_t>(slot)].buttons = 0; tick(); tick(0.51f);
    };
    const auto move = [&](GamepadButton button, int count)
    {
        for (int step = 0; step < count; ++step) { pad(3, button); }
    };
    const auto keyboard = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick(); tick(0.51f);
    };
    using Slots = std::array<bool, input_slots::pad_count>;
    std::ofstream records(output / "rumble-settings.csv");
    records << "case,selected_slot,active_pad1,active_pad2,active_pad3,active_pad4,music,save_equal\n";
    int cases = 0;
    using Comfort = std::array<SeatComfort, input_slots::pad_count>;
    Comfort comfort = initial.pad_comfort;
    const auto verify = [&](const char* name, const Slots& active, const Slots& disk, int active_music, int disk_music)
    {
        const PlaySettings& play = preferences.play_settings();
        Save restored; restored.load_save_file();
        require(play.pad_comfort == comfort && restored.play_settings().pad_comfort == comfort,
            "Pad sticks lost or leaked a per-pad comfort override between slots, memory and disk");
        require(play.pad_rumble == active && restored.play_settings().pad_rumble == disk &&
            play.music_volume == active_music && restored.play_settings().music_volume == disk_music,
            "Rumble page lost pending/active/disk or physical-slot association");
        require(play.sfx_volume == 0 && play.aim_sensitivity == initial.aim_sensitivity &&
            play.view_shake == initial.view_shake && play.aim_marker == initial.aim_marker &&
            play.reduce_effects == initial.reduce_effects &&
            play.sticks[1].movement_deadzone == 25 && play.sticks[1].aim_deadzone == 35 &&
            play.sticks[3].movement_deadzone == 30 && play.sticks[3].aim_deadzone == 40 &&
            preferences.input_bindings().keyboard_jump == bindings.keyboard_jump &&
            preferences.input_bindings().pads == bindings.pads,
            "Rumble edit replaced unrelated audio, comfort, sticks or Jump preferences");
        records << name << ",3," << active[0] << ',' << active[1] << ',' << active[2] << ',' << active[3] << ',' <<
            active_music << ',' << (active == disk && active_music == disk_music) << '\n';
        ++cases;
    };

    stack.clear(); stack.notify_activation(true);
    app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
    for (const Key key : { Key::up, Key::down, Key::left, Key::right, Key::enter, Key::escape })
    { app.keyboard()->on_key_up(key); }
    painter_scripted_pads::states = {};
    painter_scripted_pads::states[1].connected = true;
    painter_scripted_pads::states[3].connected = true;
    tick(); tick(); input.reset_actions();
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
    stack.push(std::make_unique<ControlsSettingsMenu>(&context, 3)); tick(); tick();
    // Parent audio remains pending while its physical owner opens the child.
    move(GamepadButton::dpad_down, 3); pad(3, GamepadButton::dpad_right);
    move(GamepadButton::dpad_up, 3); pad(3, GamepadButton::a);
    require(stack.depth() == 3, "Controls did not open actual Pad sticks for its sparse physical owner");
    move(GamepadButton::dpad_down, 3);
    const Frame on = main_menu_frame(app);
    const std::string initial_bytes = bytes();
    pad(1, GamepadButton::dpad_left); keyboard(Key::left);
    require(main_menu_frame(app).pixels == on.pixels && bytes() == initial_bytes,
        "Another pad or keyboard changed a connected owner's rumble selection");
    pad(3, GamepadButton::dpad_left);
    const Frame off = main_menu_frame(app);
    require(bytes() == initial_bytes, "Pending rumble Off wrote the save before Apply");
    verify("pending-off", { true, true, true, true }, { true, true, true, true }, 0, 0);
    const auto& look = presentation.controls_settings();
    RectangleF changed(look.layout.position.x - look.art.focus_padding.x - look.art.keyline_width * 10.0f,
        look.layout.position.y + look.layout.spacing.y * 3.0f - 24.0f,
        look.controls_position.x - look.layout.position.x + look.art.focus_padding.x + look.art.keyline_width * 10.0f,
        look.layout.spacing.y + 48.0f);
    const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
        app.viewport_manager()->fullscreen_viewport());
    camera.calculate_view_rectangle(changed);
    const RectangleI delta = foreground_delta_bounds(on, off);
    require(delta.width > 0 && delta.height > 0 && countdown_crop_bounds(changed).contains(delta),
        "Actual rumble On/Off lettering did not change within its complete row");
    require(off.pixels == main_menu_frame(app).pixels, "Frozen rumble settings draw changed pixels");
    save(on, output / "rumble-settings-on.ppm"); save(off, output / "rumble-settings-off.ppm");
    move(GamepadButton::dpad_down, 4); pad(3, GamepadButton::a);
    verify("applied-off", { true, true, true, false }, { true, true, true, false }, 0, 0);
    const std::string applied_bytes = bytes();
    pad(3, GamepadButton::a);
    require(bytes() == applied_bytes, "Repeated rumble Apply changed canonical save bytes");
    move(GamepadButton::dpad_up, 4); pad(3, GamepadButton::dpad_right);
    move(GamepadButton::dpad_down, 5); pad(3, GamepadButton::a);
    require(stack.depth() == 2 && bytes() == applied_bytes, "Rumble Back saved its pending On choice");
    move(GamepadButton::dpad_down, 8); pad(3, GamepadButton::a);
    verify("parent-merge", { true, true, true, false }, { true, true, true, false }, 10, 10);

    move(GamepadButton::dpad_up, 8); pad(3, GamepadButton::a);
    move(GamepadButton::dpad_down, 3); pad(3, GamepadButton::dpad_right);
    require(std::filesystem::create_directory(blocked), "Cannot isolate one rumble save failure");
    move(GamepadButton::dpad_down, 4); pad(3, GamepadButton::a);
    verify("save-failed", { true, true, true, true }, { true, true, true, false }, 10, 10);
    require(std::filesystem::remove(blocked), "Cannot release the private rumble save blocker");
    pad(3, GamepadButton::a);
    verify("save-recovered", { true, true, true, true }, { true, true, true, true }, 10, 10);
    const std::string recovered_bytes = bytes();
    move(GamepadButton::dpad_up, 4); pad(3, GamepadButton::dpad_left); pad(3, GamepadButton::b);
    require(stack.depth() == 2 && bytes() == recovered_bytes, "Rumble Back action saved its pending Off choice");
    verify("discard", { true, true, true, true }, { true, true, true, true }, 10, 10);

    pad(3, GamepadButton::a);
    PlaySettings independent = preferences.play_settings(); independent.pad_rumble[1] = false;
    require(preferences.set_play_settings_and_save(independent), "Cannot prepare an independent pad-slot edit");
    move(GamepadButton::dpad_down, 3); pad(3, GamepadButton::dpad_left);
    move(GamepadButton::dpad_down, 4); pad(3, GamepadButton::a);
    verify("edited-slot-merge", { true, false, true, false }, { true, false, true, false }, 10, 10);
    const std::string merged_bytes = bytes();
    move(GamepadButton::dpad_up, 4); pad(3, GamepadButton::dpad_right);
    painter_scripted_pads::states[3].connected = false; tick();
    keyboard(Key::escape);
    require(stack.depth() == 2 && bytes() == merged_bytes,
        "Keyboard Escape recovery after owner disconnect saved a pending rumble choice");
    verify("escape-discard", { true, false, true, false }, { true, false, true, false }, 10, 10);

    painter_scripted_pads::states[3].connected = true; tick(); tick();
    pad(3, GamepadButton::a);
    require(stack.depth() == 3, "Controls did not reopen Pad sticks for its reconnected owner");
    move(GamepadButton::dpad_down, 4);
    const std::string comfort_before = bytes();
    pad(3, GamepadButton::dpad_left);
    require(bytes() == comfort_before, "Pending View shake Off wrote the save before Apply");
    verify("comfort-pending", { true, false, true, false }, { true, false, true, false }, 10, 10);
    move(GamepadButton::dpad_down, 3); pad(3, GamepadButton::a);
    comfort[3].view_shake = SeatSwitch::off;
    verify("comfort-applied", { true, false, true, false }, { true, false, true, false }, 10, 10);
    const PlaySettings& resolved = preferences.play_settings();
    require(!resolved.view_shake_enabled(3) && resolved.view_shake_enabled(1) && resolved.view_shake &&
        resolved.aim_marker_enabled(3) && !resolved.reduce_effects_for(3),
        "Pad 4's View shake Off did not resolve for its own slot only");
    pad(3, GamepadButton::dpad_down); pad(3, GamepadButton::a);
    require(stack.depth() == 2, "Pad sticks Back after a comfort Apply did not return to Controls");
    require(cases == 10 && records.good(), "Rumble settings evidence is incomplete");
    std::cout << "PASS rumble settings: actual Controls/Pad sticks, sparse owner3 with earlier connected slot1, "
        "pending On/Off, Apply/reload/repeat, Back/Escape discard, one failed save/recovery, unrelated settings and "
        "parent/edited-slot merge; one pad's View shake Off pending then applied to that slot only; two 720p captures, no native motor call or physical comfort claim\n";
}
#endif
