// Options restores only its staged display fields. Actual writes live in a
// nested profile, leaving the runner's immutable save and desktop untouched.
void options_defaults_matrix(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    struct DefaultsProfile
    {
        std::wstring previous;
        explicit DefaultsProfile(const std::filesystem::path& root)
        {
            wchar_t* raw = nullptr; size_t length = 0;
            require(_wdupenv_s(&raw, &length, L"LOCALAPPDATA") == 0, "Cannot read isolated defaults profile");
            if (raw != nullptr) { this->previous = raw; std::free(raw); }
            std::filesystem::create_directories(root);
            require(_wputenv_s(L"LOCALAPPDATA", root.c_str()) == 0, "Cannot isolate Options defaults save");
        }
        ~DefaultsProfile() { _wputenv_s(L"LOCALAPPDATA", this->previous.c_str()); }
    } isolated(output / "options-defaults-profile");
    const auto save_path = output / "options-defaults-profile" / "ColourWars" / "save_data.json";
    const std::filesystem::path blocked(save_path.wstring() + L".tmp");
    Save preferences; PlaySettings expected;
    expected.aim_sensitivity = 5; expected.music_volume = expected.sfx_volume = 0;
    expected.view_shake = expected.aim_marker = false; expected.reduce_effects = false;
    for (size_t slot = 0; slot < expected.sticks.size(); ++slot)
    {
        expected.sticks[slot].movement_deadzone = 9 + static_cast<int>(slot) * 11;
        expected.sticks[slot].aim_deadzone = 21 + static_cast<int>(slot) * 13;
    }
    const auto preserved = [&](const PlaySettings& actual)
    {
        require(actual.aim_sensitivity == expected.aim_sensitivity && actual.music_volume == expected.music_volume &&
            actual.sfx_volume == expected.sfx_volume && actual.view_shake == expected.view_shake &&
            actual.aim_marker == expected.aim_marker && actual.reduce_effects == expected.reduce_effects,
            "Options defaults replaced unrelated shared play preferences");
        for (size_t slot = 0; slot < expected.sticks.size(); ++slot)
        {
            require(actual.sticks[slot].movement_deadzone == expected.sticks[slot].movement_deadzone &&
                actual.sticks[slot].aim_deadzone == expected.sticks[slot].aim_deadzone,
                "Options defaults replaced an input-slot deadzone profile");
        }
    };
    const auto bytes = [&]
    {
        std::ifstream stream(save_path, std::ios::binary); require(stream.good(), "Options defaults lost its isolated save");
        return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    };
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings); StateContext& stack = app; MenuPage* active = nullptr;
    struct ClearDefaults
    {
        StateContext& stack; std::filesystem::path blocked;
        ~ClearDefaults() { this->stack.clear(); painter_scripted_pads::states = {}; std::error_code ignored; std::filesystem::remove(this->blocked, ignored); }
    } clear{ stack, blocked };
    const auto& style = presentation.menu(); const auto& look = presentation.main_menu();
    auto art = look.art; art.focus_padding = look.setup_focus_padding; art.shadow_offset = look.setup_shadow_offset;
    const std::array<std::wstring, 7> labels{ L"Resolution", L"Fullscreen", L"Team palette", L"Controls / audio", L"Restore defaults", L"Apply", L"Back" };
    require(presentation.menu_text().item(MenuItem::restore_defaults) == labels[4] &&
        SaveData{}.resolution == ScreenResolution::s_1920_1080 && !SaveData{}.fullscreen && SaveData{}.team_palette == TeamColourPair::random,
        "Options defaults independent copy or canonical display defaults drifted");
    const int keyboard = input_slots::keyboard_and_mouse; int device = keyboard, row = 0, status = 0;
    bool staged = false, saved_fullscreen = false; std::string before_bytes;
    const float settled = std::max(style.motion.focus_duration, style.motion.press_duration) + 0.01f;
    const auto tick = [&](float elapsed = 0.0f) { app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed); };
    const auto hold = [&](GamepadButton button, bool down)
    {
        if (device == keyboard)
        {
            const Key key = button == GamepadButton::a ? Key::enter : button == GamepadButton::b ? Key::escape :
                button == GamepadButton::dpad_up ? Key::up : button == GamepadButton::dpad_down ? Key::down : Key::right;
            if (down) { app.keyboard()->on_key_down(key); } else { app.keyboard()->on_key_up(key); }
        }
        else
        {
            const auto bit = static_cast<uint16_t>(1u << static_cast<unsigned int>(button)); auto& buttons = painter_scripted_pads::states[3].buttons;
            buttons = down ? static_cast<uint16_t>(buttons | bit) : static_cast<uint16_t>(buttons & static_cast<uint16_t>(~bit));
        }
    };
    const auto tap = [&](GamepadButton button) { hold(button, true); tick(); hold(button, false); tick(); tick(settled); };
    const auto resize = [&](Vector2I size) { app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y); };
    const auto frame = [&] { return main_menu_frame(app); };
    const auto seed = [&](bool fullscreen = false)
    {
        stack.clear(); const_cast<PlaySettings&>(preferences.play_settings()) = expected;
        require(preferences.set_display_settings_and_save(ScreenResolution::s_1280_720, fullscreen, TeamColourPair::blue_yellow), "Cannot seed Options defaults preferences");
        saved_fullscreen = fullscreen; before_bytes = bytes(); staged = false; status = 0;
    };
    const auto open = [&]
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (Key key : { Key::up, Key::down, Key::right, Key::enter, Key::escape, Key::space }) { app.keyboard()->on_key_up(key); }
        painter_scripted_pads::states = {}; painter_scripted_pads::states[3].connected = device == 3;
        app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas)); auto page = std::make_unique<MainMenuOptions>(&context); active = page.get(); stack.push(std::move(page));
        row = saved_fullscreen ? 1 : 0; status = 0; tick(); tick();
        require(stack.depth() == 2 && !app.fullscreen() && IsWindowVisible(static_cast<HWND>(app.window())) == FALSE, "Options defaults exposed its hidden window or failed to open");
    };
    const auto go = [&](int wanted)
    {
        while (row != wanted) { const bool down = row < wanted; tap(down ? GamepadButton::dpad_down : GamepadButton::dpad_up); row += down ? 1 : -1; }
    };
    const auto unchanged = [&](Vector2I resolution)
    {
        preserved(preferences.play_settings()); require(preferences.resolution() == ScreenResolution::s_1280_720 &&
            preferences.fullscreen() == saved_fullscreen && preferences.team_palette() == TeamColourPair::blue_yellow && bytes() == before_bytes &&
            app.resolution_manager()->resolution_ivec() == resolution && !app.fullscreen() && IsWindowVisible(static_cast<HWND>(app.window())) == FALSE &&
            GetForegroundWindow() != static_cast<HWND>(app.window()), "Staged Options defaults changed active display, window, file or play preferences before Apply");
    };
    const auto restore = [&]
    {
        go(4); const Vector2I size = app.resolution_manager()->resolution_ivec(); tap(GamepadButton::a); staged = true; status = 1; unchanged(size);
    };
    const auto applied = [&]
    {
        preserved(preferences.play_settings()); require(preferences.resolution() == ScreenResolution::s_1920_1080 && !preferences.fullscreen() &&
            preferences.team_palette() == TeamColourPair::random && !app.fullscreen(), "Options defaults Apply lost canonical display choices");
    };
    const auto reload = [&](bool defaults)
    {
        Save loaded; loaded.load_save_file(); preserved(loaded.play_settings()); require(loaded.resolution() == (defaults ? ScreenResolution::s_1920_1080 : ScreenResolution::s_1280_720) &&
            loaded.team_palette() == (defaults ? TeamColourPair::random : TeamColourPair::blue_yellow) && loaded.fullscreen() == (defaults ? false : saved_fullscreen), "Options defaults persisted the wrong display choices");
    };
    const auto apply = [&](Vector2I size, bool success)
    {
        go(5); const HWND window = static_cast<HWND>(app.window()); const SettingsProbeActivationGuard guard(window);
        require(SetWindowPos(window, nullptr, -32000, -32000, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE) != FALSE, "Cannot keep defaults Apply offscreen");
        const auto before = bytes(); const auto resolution = preferences.resolution();
        const bool fullscreen = preferences.fullscreen(); const auto palette = preferences.team_palette();
        const bool display_trial = resolution != ScreenResolution::s_1920_1080 || fullscreen || app.fullscreen();
        tap(GamepadButton::a); ShowWindow(window, SW_HIDE); app.keyboard()->set_focused(true); app.mouse()->set_focused(true); stack.notify_activation(true); tick();
        require(stack.depth() == (display_trial ? 3 : 2), "Options defaults Apply did not use the expected trial or direct-save route");
        if (stack.depth() == 3)
        {
            require(bytes() == before && preferences.resolution() == resolution && preferences.fullscreen() == fullscreen &&
                preferences.team_palette() == palette, "Display defaults trial committed preferences before Keep");
            tap(GamepadButton::dpad_up); tap(GamepadButton::a);
            ShowWindow(window, SW_HIDE); app.keyboard()->set_focused(true); app.mouse()->set_focused(true); stack.notify_activation(true); tick();
            require(stack.depth() == 2, "Display defaults Keep did not return to the existing Options page");
        }
        require(IsWindowVisible(window) == FALSE && GetForegroundWindow() != window && !app.fullscreen(), "Options defaults Apply showed, activated or fullscreened its private window");
        applied(); saved_fullscreen = false; status = success ? 2 : 3; resize(size);
    };
    std::ofstream records(output / "options-defaults.csv"); records << "case,width,height,input_slot,row,staged,active_fullscreen,status,full_frame_matches\n";
    const auto compare = [&](const std::string& name, bool capture = false)
    {
        preserved(preferences.play_settings()); const Frame actual = frame(); const RectangleF design(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION);
        const RectangleF paper((Vector2F(1920.0f, style.footer_y) - look.options_box_size) / 2.0f, look.options_box_size);
        const float height = look.options_spacing.y - art.focus_padding.y * 2.0f;
        const std::array<std::wstring, 3> values{ saved_fullscreen ? std::to_wstring(actual.width) + L"x" + std::to_wstring(actual.height) : staged ? L"1920x1080" : L"1280x720",
            staged ? L"Off" : saved_fullscreen ? L"On" : L"Off", staged ? L"Random" : L"A: Blue / B: Yellow" };
        std::vector<std::unique_ptr<UiText>> text; std::array<RectangleF, 7> bounds;
        const auto label = [&](const std::wstring& value, const std::string& font, Vector2F position, Colour ink)
        {
            auto item = std::make_unique<UiText>("independent-options-defaults", value, font, position, app.render_resources(), ink); auto* raw = item.get(); text.push_back(std::move(item)); return raw;
        };
        label(L"Options", style.heading_font, look.options_heading_position, art.ink);
        for (int index = 0; index < 7; ++index)
        {
            const Vector2F position = look.options_position + Vector2F(0.0f, look.options_spacing.y * static_cast<float>(index));
            auto* item = label(labels[static_cast<size_t>(index)], style.item_font, position,
                index == 0 && saved_fullscreen ? art.disabled_ink : index == row ? art.focus_ink : art.ink);
            if (index < 3)
            {
                item->set_scale(std::min({ 1.0f, (look.options_value_x - position.x - art.focus_padding.x) / item->bounds().width, height / item->bounds().height }));
                auto* value = label(values[static_cast<size_t>(index)], style.item_font, Vector2F(look.options_value_x, position.y), art.ink);
                value->set_scale(std::min({ 1.0f, (paper.right() - art.shadow_offset - art.focus_padding.x - look.options_value_x) / value->bounds().width, height / value->bounds().height }));
                bounds[static_cast<size_t>(index)] = RectangleF::union_of(item->bounds(), value->bounds());
            }
            else { bounds[static_cast<size_t>(index)] = item->bounds(); }
            RectangleF padded = bounds[static_cast<size_t>(index)]; padded.inflate(art.focus_padding);
            require(paper.contains(padded) && (index == 0 || padded.y >= bounds[static_cast<size_t>(index - 1)].bottom() + art.focus_padding.y), "Independent Options defaults row/card escapes paper or overlaps its predecessor");
        }
        const std::array<std::wstring, 4> messages{ L"", L"Apply to save changes.", L"Settings applied and saved.", L"Settings active for this run. Could not save for next time." };
        auto* message = label(messages[static_cast<size_t>(status)], presentation.prompt_bar().font, look.options_status_position, status == 3 ? look.options_warning_colour : art.focus_ink);
        const float available = paper.right() - art.shadow_offset - art.focus_padding.x - look.options_status_position.x;
        if (message->bounds().width > available) { message->set_scale(available / message->bounds().width); }
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1); const auto viewport = app.viewport_manager()->fullscreen_viewport(); const Camera camera = Camera::frame(design, viewport);
        DrawList list = renderer.view(0); list.set_viewport(viewport); list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        canvas.draw(list, RectangleF(Vector2F::ZERO, app.resolution_manager()->resolution_vec())); list.set_camera(camera);
        const SpriteSheet* sheet = app.render_resources()->sprite_sheet(style.sprite_sheet); const auto pixel = sheet->resolve_sprite_frame(style.fill_frame);
        sheet->draw(list, pixel, camera.visible_rectangle(viewport), style.scrim_colour); sheet->draw(list, pixel, design, look.home_background_colour);
        setup_paper_reference(list, *app.render_resources(), style, art, paper, bounds[static_cast<size_t>(row)]);
        sheet->draw(list, pixel, RectangleF(0.0f, style.footer_y, 1920.0f, 1080.0f - style.footer_y), style.scrim_colour);
        sheet->draw(list, pixel, RectangleF(style.brand_position.x, style.footer_y, 1920.0f - style.brand_position.x * 2.0f, style.rule_height), style.panel_edge_colour);
        list.set_filter(TextureFilter::linear); for (const auto& item : text) { item->draw(list); }
        const auto& footer = presentation.prompt_bar(); const UiTextDropShadow prompt("independent-options-defaults-prompt", L"Left/Right Change    A/Enter Select    B/Esc Back", footer.font, footer.position, app.render_resources(), footer.colour, footer.shadow_colour, footer.shadow_offset); prompt.draw(list);
        const Frame expected_frame = read(renderer); const std::string stem = "options-defaults-" + name + (device == keyboard ? "-keyboard-" : "-pad3-") + std::to_string(actual.width) + 'x' + std::to_string(actual.height);
        if (actual.pixels != expected_frame.pixels) { save(actual, output / (stem + "-failure-actual.ppm")); save(expected_frame, output / (stem + "-failure-expected.ppm")); const auto delta = foreground_delta_bounds(actual, expected_frame); std::cerr << "OPTIONS DEFAULTS " << name << " row=" << row << " delta=" << delta.x << ',' << delta.y << ',' << delta.width << ',' << delta.height << '\n'; }
        require(stack.depth() == 2 && actual.pixels == expected_frame.pixels, "Actual Options defaults differs from independent complete seven-row paper/text/value/status reference");
        std::srand(7567); const int next = std::rand(); std::srand(7567); require(actual.pixels == frame().pixels && actual.pixels == frame().pixels && std::rand() == next, "Frozen Options defaults changed pixels or draw RNG");
        if (capture) { save(actual, output / (stem + ".ppm")); save(foreground_crop(actual, countdown_crop_bounds(camera.calculate_view_rectangle(paper)), 0), output / (stem + "-detail.ppm")); }
        records << name << ',' << actual.width << ',' << actual.height << ',' << device << ',' << row << ',' << staged << ',' << saved_fullscreen << ',' << status << ",true\n"; return actual;
    };
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440), Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        device = keyboard; resize(size); seed(); open();
        for (int focus = 0; focus < 7; ++focus) { go(focus); compare("initial-row" + std::to_string(focus), focus == 4); }
        restore(); compare("restored-pending", true); restore(); compare("repeat-restore");
        apply(size, true); compare("saved", true); reload(true); const auto once = bytes(); apply(size, true); require(bytes() == once, "Repeated defaults Apply changed serialized preferences"); compare("repeat-apply");
        seed(); open(); restore(); const auto prior = bytes(); require(std::filesystem::create_directory(blocked), "Cannot block isolated defaults temporary save");
        apply(size, false); require(bytes() == prior, "Failed defaults save replaced the previous JSON"); compare("failed-save", true);
        Save old; old.load_save_file(); preserved(old.play_settings()); require(old.resolution() == ScreenResolution::s_1280_720 && old.team_palette() == TeamColourPair::blue_yellow, "Failed defaults write lost prior durable preferences");
        require(std::filesystem::remove(blocked), "Cannot remove isolated defaults write refusal"); apply(size, true); reload(true); compare("recovered-save");
        std::cout << "PASS Options defaults complete states " << size.x << 'x' << size.y << '\n';
    }
    const Vector2I size(1280, 720); resize(size);
    for (int slot : { keyboard, 3 })
    {
        device = slot; seed(); stack.clear(); stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        stack.push(std::make_unique<MainMenuHome>(&context, MainMenuHome::InitialFocus::options)); tick(); tick(); const Frame home_destination = frame();
        open(); go(4); hold(GamepadButton::a, true); tick();
        if (slot == keyboard) { static_cast<WindowNotify&>(app).on_deactivated(); tick(); static_cast<WindowNotify&>(app).on_activated(); tick(); }
        else { painter_scripted_pads::states[3].connected = false; tick(); painter_scripted_pads::states[3].connected = true; tick(); tick(); }
        hold(GamepadButton::a, false); tick(); tick(); tick(settled); unchanged(size); compare("interrupted-restore");
        restore(); compare("fresh-restore", true); go(3); const Frame parent = compare("before-controls"); tap(GamepadButton::a); tick(); require(stack.depth() == 3, "Defaults Controls row did not push child");
        for (int move = 0; move < 3; ++move) { tap(GamepadButton::dpad_down); } tap(GamepadButton::dpad_right);
        tap(GamepadButton::b); tick(); require(parent.pixels == frame().pixels && stack.depth() == 2, "Controls return lost pending defaults or parent focus"); unchanged(size); compare("after-controls");
        go(6); tap(GamepadButton::a); require(stack.depth() == 2 && frame().pixels == home_destination.pixels, "Defaults Back did not return actual Home Options focus"); unchanged(size); staged = false; open(); compare("back-discard");
        restore(); tap(GamepadButton::b); require(stack.depth() == 2 && frame().pixels == home_destination.pixels, "Defaults Escape did not return actual Home Options focus"); unchanged(size); staged = false; open(); compare("escape-discard");
        restore(); stack.notify_activation(false); compare("deactivated-restore"); stack.notify_activation(true); tick(); active->debug_reset_input(slot); compare("developer-reset");
        expected.reduce_effects = true; const_cast<PlaySettings&>(preferences.play_settings()).reduce_effects = true; restore(); const Frame quiet = compare("reduced-restore"); tick(0.0f); tick(-0.5f); tick(settled); require(quiet.pixels == frame().pixels, "Reduced defaults response moved or nonpositive time changed it");
        expected.reduce_effects = false; const_cast<PlaySettings&>(preferences.play_settings()).reduce_effects = false;
        seed(true); open(); compare("fullscreen-metadata", true); restore(); compare("fullscreen-staged-off", true); apply(size, true); reload(true); compare("fullscreen-applied-off");
    }
    // A real Controls Apply may update shared preferences while display defaults
    // remain pending. Parent Apply must preserve that independently saved edit.
    device = keyboard; seed(); open(); restore(); go(3); tap(GamepadButton::a); tick();
    for (int move = 0; move < 3; ++move) { tap(GamepadButton::dpad_down); } tap(GamepadButton::dpad_right);
    for (int move = 0; move < 3; ++move) { tap(GamepadButton::dpad_down); } tap(GamepadButton::a); expected.view_shake = true; preserved(preferences.play_settings());
    tap(GamepadButton::b); tick(); compare("controls-saved-parent-pending"); apply(size, true); reload(true); compare("controls-merge-applied");
    require(records.good(), "Options defaults evidence write failed");
    std::cout << "PASS Options defaults independent complete seven rows; staged/no-write restore, repeat/discard/Controls merge, hidden windowed Apply/reload/failure/recovery, all preserved play/pad fields and keyboard/sparse-pad rearming\n";
}
