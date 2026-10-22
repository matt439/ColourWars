// Actual Options Apply owns the normal route. The independent child raster and
// private window/profile assertions keep temporary display changes reviewable.
void display_trial_matrix(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    const Presentation original = presentation;
    struct TrialProfile
    {
        std::wstring previous;
        explicit TrialProfile(const std::filesystem::path& path)
        {
            wchar_t* raw = nullptr; size_t length = 0;
            require(_wdupenv_s(&raw, &length, L"LOCALAPPDATA") == 0, "Cannot read isolated trial profile");
            if (raw != nullptr) { this->previous = raw; std::free(raw); }
            std::filesystem::create_directories(path);
            require(_wputenv_s(L"LOCALAPPDATA", path.c_str()) == 0, "Cannot isolate display trial saves");
        }
        ~TrialProfile() { _wputenv_s(L"LOCALAPPDATA", this->previous.c_str()); }
    } isolated(output / "display-trial-profile");
    const auto save_path = output / "display-trial-profile" / "ColourWars" / "save_data.json";
    const std::filesystem::path blocked(save_path.wstring() + L".tmp");
    const HWND window = static_cast<HWND>(app.window());
    // Trials resize the hidden window, which would activate it (see the guard).
    // Rollback may run from notification or destruction, so guard the whole
    // fixture, including explicit stack cleanup; this guard must never nest.
    const SettingsProbeActivationGuard activation(window);
    Save preferences; PlaySettings expected; expected.aim_sensitivity = 5;
    expected.music_volume = expected.sfx_volume = 0; expected.view_shake = expected.aim_marker = false;
    for (size_t slot = 0; slot < expected.sticks.size(); ++slot)
    {
        expected.sticks[slot].movement_deadzone = 9 + static_cast<int>(slot) * 11;
        expected.sticks[slot].aim_deadzone = 21 + static_cast<int>(slot) * 13;
    }
    const auto preserved = [&](const PlaySettings& actual)
    {
        require(actual.aim_sensitivity == expected.aim_sensitivity && actual.music_volume == expected.music_volume &&
            actual.sfx_volume == expected.sfx_volume && actual.view_shake == expected.view_shake && actual.aim_marker == expected.aim_marker &&
            actual.reduce_effects == expected.reduce_effects, "Display trial changed unrelated shared preferences");
        for (size_t slot = 0; slot < expected.sticks.size(); ++slot)
        {
            require(actual.sticks[slot].movement_deadzone == expected.sticks[slot].movement_deadzone &&
                actual.sticks[slot].aim_deadzone == expected.sticks[slot].aim_deadzone, "Display trial changed a pad profile");
        }
    };
    const auto bytes = [&]
    {
        std::ifstream stream(save_path, std::ios::binary); require(stream.good(), "Display trial lost its private save");
        return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    };
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings); StateContext& stack = app;
    struct ClearTrial
    {
        StateContext& stack; Presentation& presentation; Presentation original; std::filesystem::path blocked;
        ~ClearTrial() { this->stack.clear(); this->presentation = this->original; painter_scripted_pads::states = {}; std::error_code ignored; std::filesystem::remove(this->blocked, ignored); }
    } clear{ stack, presentation, original, blocked };
    const auto& style = original.menu(); const auto& look = original.pause_menu(); auto art = look.art;
    require(original.main_menu().setup_focus_padding == Vector2F(28.0f, 7.0f) && original.main_menu().setup_shadow_offset == 2.0f,
        "Display trial independent compact card geometry drifted");
    art.focus_padding = Vector2F(28.0f, 7.0f); art.shadow_offset = 2.0f;
    require(original.menu_text().confirmations.display == L"Keep these display settings?" &&
        original.menu_text().display_trial_countdown == L"Reverting in {seconds}s" &&
        original.menu_text().item(MenuItem::keep) == L"Keep" && original.menu_text().item(MenuItem::revert) == L"Revert" &&
        original.main_menu().display_trial_seconds == 15.0f, "Display trial copy or shipped timeout drifted from independent defaults");
    const int keyboard = input_slots::keyboard_and_mouse; int device = keyboard;
    bool in_trial = false, metadata_fullscreen = false; float age = 0.0f, timeout = 15.0f;
    std::string before_bytes; MainMenuDisplayConfirmation* direct_page = nullptr;
    std::optional<DisplayTrialResult> direct_result; int direct_results = 0;
    const float settled = std::max(style.motion.focus_duration, style.motion.press_duration) + 0.01f;
    const auto tick = [&](float elapsed = 0.0f)
    {
        if (in_trial && stack.depth() == 3 && std::isfinite(elapsed) && elapsed > 0.0f) { age += elapsed; }
        app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed);
        if (stack.depth() < 3) { in_trial = false; }
    };
    const auto hold = [&](int slot, GamepadButton button, bool down)
    {
        if (slot == keyboard)
        {
            const Key key = button == GamepadButton::a ? Key::enter : button == GamepadButton::b ? Key::escape :
                button == GamepadButton::dpad_up ? Key::up : Key::down;
            if (down) { app.keyboard()->on_key_down(key); } else { app.keyboard()->on_key_up(key); }
        }
        else
        {
            const auto bit = static_cast<uint16_t>(1u << static_cast<unsigned int>(button)); auto& buttons = painter_scripted_pads::states[static_cast<size_t>(slot)].buttons;
            buttons = down ? static_cast<uint16_t>(buttons | bit) : static_cast<uint16_t>(buttons & static_cast<uint16_t>(~bit));
        }
    };
    const auto tap = [&](GamepadButton button) { hold(device, button, true); tick(); hold(device, button, false); tick(); };
    const auto frame = [&] { return main_menu_frame(app); };
    const auto resize = [&](Vector2I size) { app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y); };
    const auto hide = [&]
    {
        require(IsWindowVisible(window) == FALSE && GetForegroundWindow() != window && !app.fullscreen(), "Display trial exposed, activated or fullscreened its private window");
    };
    const auto old = [&](bool rolled_back)
    {
        preserved(preferences.play_settings()); require(preferences.resolution() == ScreenResolution::s_1280_720 && preferences.fullscreen() == metadata_fullscreen &&
            preferences.team_palette() == TeamColourPair::blue_yellow && bytes() == before_bytes, "Unconfirmed display trial changed active or durable preferences");
        if (rolled_back) { require(app.resolution_manager()->resolution_ivec() == Vector2I(1280, 720) && !app.fullscreen(), "Display trial did not restore its configured preset and prior actual mode"); }
    };
    const auto kept = [&]
    {
        preserved(preferences.play_settings()); require(preferences.resolution() == ScreenResolution::s_1920_1080 && !preferences.fullscreen() && preferences.team_palette() == TeamColourPair::random &&
            app.resolution_manager()->resolution_ivec() == Vector2I(1920, 1080) && !app.fullscreen() && stack.depth() == 2,
            "Keep lost the selected private window or candidate preferences");
    };
    const auto seed = [&](bool saved_fullscreen = false)
    {
        stack.clear(); in_trial = false; direct_page = nullptr; direct_result.reset(); direct_results = 0;
        app.set_resolution(ScreenResolution::s_1280_720); app.set_fullscreen(false); hide();
        const_cast<PlaySettings&>(preferences.play_settings()) = expected;
        require(preferences.set_display_settings_and_save(ScreenResolution::s_1280_720, saved_fullscreen, TeamColourPair::blue_yellow), "Cannot seed private display trial preferences");
        metadata_fullscreen = saved_fullscreen; before_bytes = bytes();
        for (Key key : { Key::enter, Key::escape, Key::up, Key::down, Key::space }) { app.keyboard()->on_key_up(key); }
        painter_scripted_pads::states = {}; painter_scripted_pads::states[3].connected = true;
        app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas)); stack.push(std::make_unique<MainMenuOptions>(&context)); tick(); tick();
        // Restore supplies independently known 1920/windowed/Random choices.
        for (int move = saved_fullscreen ? 1 : 0; move < 4; ++move) { tap(GamepadButton::dpad_down); }
        tap(GamepadButton::a); tap(GamepadButton::dpad_down); tick(settled); old(true);
    };
    const auto begin = [&](bool direct = false)
    {
        if (direct)
        {
            auto page = std::make_unique<MainMenuDisplayConfirmation>(&context,
                DisplaySettingsChoice{ ScreenResolution::s_1920_1080, false, TeamColourPair::random }); direct_page = page.get();
            stack.push<DisplayTrialResult>(std::move(page), [&](const DisplayTrialResult& result) { direct_result = result; ++direct_results; });
        }
        else { tap(GamepadButton::a); }
        hide(); in_trial = true; age = 0.0f; timeout = presentation.main_menu().display_trial_seconds; tick(); tick();
        require(stack.depth() == 3 && app.resolution_manager()->resolution_ivec() == Vector2I(1920, 1080), "Changed-resolution Options Apply did not start its live trial child"); old(false);
        // Explicit finite simulation time leads capture/readback wall time.
        // The one-second fixture below instead isolates the real deadline.
        if (timeout == 60.0f) { tick(10.25f); }
        else if (timeout == 15.0f) { tick(3.25f); }
    };
    const auto reverted = [&](int depth = 2)
    {
        hide(); require(stack.depth() == depth, "Display trial rollback closed the wrong stack frame"); old(true);
        if (depth == 2) { tick(); require(stack.depth() == 2, "Rolled-back display trial reused an action to start again"); }
    };
    const auto reference = [&](bool keep, int seconds)
    {
        UiText heading("independent-display-heading", L"Options", style.heading_font, Vector2F(0.0f, look.heading_y), app.render_resources(), art.ink);
        heading.set_position(Vector2F((1920.0f - heading.bounds().width) / 2.0f, look.heading_y));
        UiText question("independent-display-question", L"Keep these display settings?", style.detail_font, Vector2F(0.0f, look.status_y), app.render_resources(), art.ink);
        question.set_position(Vector2F((1920.0f - question.bounds().width) / 2.0f, look.status_y));
        UiText accept("independent-display-keep", L"Keep", style.item_font, look.layout.position, app.render_resources(), keep ? art.focus_ink : art.ink);
        UiText cancel("independent-display-revert", L"Revert", style.item_font, look.layout.position + Vector2F(0.0f, look.layout.spacing.y), app.render_resources(), keep ? art.ink : art.focus_ink);
        const float countdown_y = look.layout.position.y + look.layout.spacing.y * 2.0f;
        UiText countdown("independent-display-countdown", L"Reverting in " + std::to_wstring(seconds) + L"s", style.detail_font, Vector2F(0.0f, countdown_y), app.render_resources(), art.ink);
        countdown.set_position(Vector2F((1920.0f - countdown.bounds().width) / 2.0f, countdown_y));
        const RectangleF design(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), paper((menu_consts::DESIGN_RESOLUTION - look.box_size) / 2.0f, look.box_size);
        require(paper.contains(heading.bounds()) && paper.contains(question.bounds()) && paper.contains(countdown.bounds()), "Independent display trial copy escaped its captured paper");
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1); const auto viewport = app.viewport_manager()->fullscreen_viewport(); const Camera camera = Camera::frame(design, viewport);
        DrawList list = renderer.view(0); list.set_viewport(viewport); list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        canvas.draw(list, RectangleF(Vector2F::ZERO, app.resolution_manager()->resolution_vec())); list.set_camera(camera);
        const SpriteSheet* sheet = app.render_resources()->sprite_sheet(style.sprite_sheet); const auto pixel = sheet->resolve_sprite_frame(style.fill_frame);
        sheet->draw(list, pixel, camera.visible_rectangle(viewport), style.scrim_colour); sheet->draw(list, pixel, camera.visible_rectangle(viewport), style.scrim_colour);
        sheet->draw(list, pixel, design, original.main_menu().home_background_colour);
        setup_paper_reference(list, *app.render_resources(), style, art, paper, keep ? accept.bounds() : cancel.bounds());
        sheet->draw(list, pixel, RectangleF(0.0f, style.footer_y, 1920.0f, 1080.0f - style.footer_y), style.scrim_colour);
        sheet->draw(list, pixel, RectangleF(style.brand_position.x, style.footer_y, 1920.0f - style.brand_position.x * 2.0f, style.rule_height), style.panel_edge_colour);
        list.set_filter(TextureFilter::linear); heading.draw(list); question.draw(list); accept.draw(list); cancel.draw(list); countdown.draw(list);
        const auto& footer = original.prompt_bar(); const UiTextDropShadow prompt("independent-display-prompt", L"A/Enter Select    B/Esc Back", footer.font, footer.position, app.render_resources(), footer.colour, footer.shadow_colour, footer.shadow_offset); prompt.draw(list);
        return read(renderer);
    };
    std::ofstream records(output / "display-trial.csv"); records << "case,width,height,input_slot,keep,seconds,full_frame_matches\n";
    const auto compare = [&](const std::string& name, bool keep, bool capture = false)
    {
        old(false); const int seconds = std::max(1, static_cast<int>(std::ceil(timeout - age))); const Frame actual = frame(), expected_frame = reference(keep, seconds);
        const std::string stem = "display-trial-" + name + (device == keyboard ? "-keyboard-" : "-pad3-") + std::to_string(actual.width) + 'x' + std::to_string(actual.height);
        if (actual.pixels != expected_frame.pixels) { save(actual, output / (stem + "-failure-actual.ppm")); save(expected_frame, output / (stem + "-failure-expected.ppm")); const auto delta = foreground_delta_bounds(actual, expected_frame); std::cerr << "DISPLAY TRIAL " << name << " seconds=" << seconds << " delta=" << delta.x << ',' << delta.y << ',' << delta.width << ',' << delta.height << '\n'; }
        require(stack.depth() == 3 && actual.pixels == expected_frame.pixels, "Actual display trial differs from independent complete paper/question/choices/countdown reference");
        std::srand(8231); const int next = std::rand(); std::srand(8231); require(actual.pixels == frame().pixels && actual.pixels == frame().pixels && std::rand() == next, "Frozen display trial changed pixels or draw RNG");
        if (capture) { save(actual, output / (stem + ".ppm")); const RectangleF paper((menu_consts::DESIGN_RESOLUTION - look.box_size) / 2.0f, look.box_size); const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), app.viewport_manager()->fullscreen_viewport()); save(foreground_crop(actual, countdown_crop_bounds(camera.calculate_view_rectangle(paper)), 0), output / (stem + "-detail.ppm")); }
        records << name << ',' << actual.width << ',' << actual.height << ',' << device << ',' << keep << ',' << seconds << ",true\n"; return actual;
    };
    const auto select_keep = [&] { tap(GamepadButton::dpad_up); tick(settled); };
    const_cast<MainMenuPresentation&>(presentation.main_menu()).display_trial_seconds = 60.0f;
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440), Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        device = keyboard; seed(); begin(); resize(size); compare("default-revert", false, true); select_keep(); compare("keep-before-back", true, true); tap(GamepadButton::b); reverted();
        seed(); begin(); resize(size); tick(23.0f); compare("countdown-middle", false, true); tick(26.74f); compare("before-timeout", false); tick(0.02f); reverted();
        std::cout << "PASS display trial independent native frames " << size.x << 'x' << size.y << '\n';
    }
    for (int slot : { keyboard, 3 })
    {
        device = slot; seed(); begin(); tap(GamepadButton::a); reverted();
        seed(); begin(); select_keep(); hold(slot, GamepadButton::a, true); hold(slot, GamepadButton::b, true); tick();
        hold(slot, GamepadButton::a, false); hold(slot, GamepadButton::b, false); tick(); tick(); compare("conflicting-actions", true);
        if (slot == 3)
        {
            hold(slot, GamepadButton::a, true); tick(); painter_scripted_pads::states[3].connected = false; tick(); painter_scripted_pads::states[3].connected = true; tick(); tick();
            hold(slot, GamepadButton::a, false); tick(); tick(); compare("held-reconnect", true);
            tap(GamepadButton::a); hide(); kept();
        }
        else { tap(GamepadButton::b); reverted(); }
        seed(); begin(); select_keep(); hold(slot, GamepadButton::a, true); tick(); static_cast<WindowNotify&>(app).on_deactivated(); old(true); tick();
        hold(slot, GamepadButton::a, false); static_cast<WindowNotify&>(app).on_activated(); tick(); tick(); reverted();
        seed(); begin(true); select_keep(); stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas)); old(true); require(stack.depth() == 4 && direct_results == 0, "Covered display trial popped its cover or delivered a premature result");
        stack.pop(); tick(); reverted(); require(direct_result == DisplayTrialResult::reverted && direct_results == 1, "Covered display trial failed to close once after its cover returned");
        seed(); begin(true); require(direct_page->debug_dismiss(false), "Display trial developer dismissal was refused"); reverted(); require(direct_results == 1 && direct_result == DisplayTrialResult::reverted, "Developer dismissal lost the typed rollback result");
        seed(); begin(true); direct_page->debug_reset_input(slot); old(true); tick(); reverted(); require(direct_results == 1, "Developer input reset failed to roll back once");
        seed(); begin(true); stack.clear(); old(true); hide(); require(stack.depth() == 0 && direct_results == 0, "Display trial clear delivered a callback or retained a frame");
        seed(); begin(); tap(GamepadButton::dpad_up); Frame moving = frame(); require(moving.pixels != reference(true, static_cast<int>(std::ceil(timeout - age))).pixels, "Display trial cursor response did not start");
        tick(0.0f); tick(-0.5f); const Frame after_nonpositive = frame(); const auto delta = foreground_delta_bounds(moving, after_nonpositive);
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), app.viewport_manager()->fullscreen_viewport());
        const float countdown_y = look.layout.position.y + look.layout.spacing.y * 2.0f;
        const auto line_size = app.render_resources()->measure_text(app.render_resources()->resolve_sprite_font(style.detail_font), L"Reverting in 60s");
        const RectangleF clock_band(0.0f, countdown_y - 2.0f, 1920.0f, line_size.y + 4.0f);
        require((delta.width <= 0 || countdown_crop_bounds(camera.calculate_view_rectangle(clock_band)).contains(delta)),
            "Nonpositive display trial update changed geometry beyond the live wall-clock countdown");
        moving = after_nonpositive; old(false);
        const_cast<PlaySettings&>(preferences.play_settings()).reduce_effects = true; expected.reduce_effects = true; compare("reduce-immediate", true, true);
        const_cast<PlaySettings&>(preferences.play_settings()).reduce_effects = false; expected.reduce_effects = false; require(moving.pixels == frame().pixels, "Reduced display trial drawing consumed focus motion"); tap(GamepadButton::b); reverted();
        seed(); begin(); select_keep(); compare("deliberate-keep", true); tap(GamepadButton::a); hide(); kept(); Save loaded; loaded.load_save_file(); preserved(loaded.play_settings()); require(loaded.resolution() == ScreenResolution::s_1920_1080 && !loaded.fullscreen() && loaded.team_palette() == TeamColourPair::random, "Keep failed to persist and reload its candidate");
        const auto once = bytes(); tap(GamepadButton::a); hide(); kept(); require(bytes() == once, "Repeated unchanged Apply reopened a trial or changed saved bytes");
    }
    device = keyboard; seed();
    // Another device opens Apply while Enter is held; init must disarm it.
    hold(keyboard, GamepadButton::a, true); tick(); hold(3, GamepadButton::a, true); tick(); hold(3, GamepadButton::a, false); tick();
    hide(); in_trial = true; age = 0; timeout = presentation.main_menu().display_trial_seconds; hold(keyboard, GamepadButton::a, false); tick(); tick(); tick(10.25f); compare("held-cross-device-entry", false); tap(GamepadButton::b); reverted();
    seed(); begin(); hold(keyboard, GamepadButton::a, true); tick(); hold(3, GamepadButton::dpad_up, true); hold(keyboard, GamepadButton::a, false); tick(); hold(3, GamepadButton::dpad_up, false); tick(); tick(settled); compare("simultaneous-move-confirm", true); tap(GamepadButton::b); reverted();
    seed(); begin(); select_keep(); hold(3, GamepadButton::a, true); hold(keyboard, GamepadButton::b, true); tick(); hold(3, GamepadButton::a, false); hold(keyboard, GamepadButton::b, false); tick(); reverted();
    seed(); begin(); select_keep(); require(std::filesystem::create_directory(blocked), "Cannot block the isolated trial Keep write"); tap(GamepadButton::a); hide(); kept(); require(bytes() == before_bytes, "Failed Keep replaced the prior durable save");
    Save durable; durable.load_save_file(); preserved(durable.play_settings()); require(durable.resolution() == ScreenResolution::s_1280_720 && durable.team_palette() == TeamColourPair::blue_yellow, "Failed Keep lost prior durable choices");
    require(std::filesystem::remove(blocked), "Cannot remove trial Keep write refusal"); tap(GamepadButton::a); hide(); kept(); Save recovered; recovered.load_save_file(); preserved(recovered.play_settings()); require(recovered.resolution() == ScreenResolution::s_1920_1080 && recovered.team_palette() == TeamColourPair::random, "Unchanged Apply did not recover failed Keep persistence");
    seed(true); begin(); select_keep(); tap(GamepadButton::a); hide(); kept();
    const_cast<MainMenuPresentation&>(presentation.main_menu()).display_trial_seconds = 15.0f;
    seed(); begin(); compare("shipped-fifteen-second-timeout", false, true); tap(GamepadButton::b); reverted();
    // Wall-clock expiry is independent of positive simulation updates.
    const_cast<MainMenuPresentation&>(presentation.main_menu()).display_trial_seconds = 1.0f; seed(); begin(); compare("wall-clock-start", false); std::this_thread::sleep_for(std::chrono::milliseconds(1100)); tick(); reverted();
    presentation = original; stack.clear(); hide(); require(records.good(), "Display trial evidence write failed");
    std::cout << "PASS actual changed-resolution Options trial: independent six-size child, safe defaults/timing/reduction, Keep and failure/recovery, rollback on every lifecycle, private window/profile and all play/pad preferences\n";
}
