// Real pushed Home Exit child. Complete confirmation pixels are independently
// constructed; cancellation is compared with the retained Home Exit frame.
void home_exit_matrix(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    const Presentation original = presentation;
    const auto& style = original.menu(); const auto& look = original.pause_menu(); const auto& art = look.art;
    require(original.menu_text().confirmations.exit == L"Exit ColourWars?" &&
        original.menu_text().item(MenuItem::exit) == L"Exit" && original.menu_text().item(MenuItem::yes) == L"Yes" &&
        original.menu_text().item(MenuItem::no) == L"No", "Home Exit copy drifted from independent literals");
    Save preferences; auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0; play.reduce_effects = false;
    GameData data; data.set_application(&app); data.set_save(&preferences);
    data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings); StateContext& stack = app;
    struct ClearHomeExit
    {
        StateContext& stack; MenuContext& context; Presentation& presentation; Presentation original;
        ~ClearHomeExit() { this->context.cancel_music_exit(); this->context.set_music_update({}); this->stack.clear(); painter_scripted_pads::states = {}; this->presentation = this->original; }
    } clear{ stack, context, presentation, original };
    const HWND window = static_cast<HWND>(app.window()); const int keyboard = input_slots::keyboard_and_mouse;
    const float settled = std::max(style.motion.focus_duration, style.motion.press_duration) + 0.01f;
    int device = keyboard, begin_exits = 0; Frame retained_home; MainMenuExitConfirmation* active = nullptr;
    const auto tick = [&](float elapsed = 0.0f)
        { app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed); };
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
            const auto bit = static_cast<uint16_t>(1u << static_cast<unsigned int>(button));
            auto& buttons = painter_scripted_pads::states[static_cast<size_t>(slot)].buttons;
            buttons = down ? static_cast<uint16_t>(buttons | bit) : static_cast<uint16_t>(buttons & static_cast<uint16_t>(~bit));
        }
    };
    const auto tap = [&](GamepadButton button) { hold(device, button, true); tick(); hold(device, button, false); tick(); };
    const auto frame = [&] { return main_menu_frame(app); };
    const auto resize = [&](Vector2I size)
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
    };
    const auto live = [&]
    {
        require(IsWindow(window) != FALSE && settings.player_count() == 0 && settings.best_of() == 1 &&
            !settings.duration() && settings.stage().empty(), "Home Exit changed the empty match plan or prematurely closed its hidden window");
    };
    const auto home = [&]
    {
        live(); require(stack.depth() == 2 && !context.music_exit_pending() && begin_exits == 0 && frame().pixels == retained_home.pixels,
            "Home Exit cancellation did not resume the same settled Home Exit page");
        tick(); require(stack.depth() == 2 && frame().pixels == retained_home.pixels,
            "Home Exit cancellation reused a release to reopen the confirmation");
    };
    const auto open = [&](bool connect_pad = false)
    {
        stack.clear(); context.cancel_music_exit(); context.set_music_update([&](float elapsed) { if (elapsed == 0.0f) { ++begin_exits; } });
        begin_exits = 0; active = nullptr; stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (Key key : { Key::enter, Key::escape, Key::up, Key::down, Key::space }) { app.keyboard()->on_key_up(key); }
        painter_scripted_pads::states = {}; painter_scripted_pads::states[3].connected = connect_pad;
        app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll();
        input.reset_actions(); settings = MenuLevelSettings();
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas)); stack.push(std::make_unique<MainMenuHome>(&context)); tick(); tick();
        for (int row = 0; row < 3; ++row) { tap(GamepadButton::dpad_down); }
        tick(settled); retained_home = frame(); home();
    };
    const auto confirm = [&](bool direct = false)
    {
        if (direct)
        {
            auto page = std::make_unique<MainMenuExitConfirmation>(&context); active = page.get(); stack.push(std::move(page));
        }
        else { tap(GamepadButton::a); }
        tick(); tick(); live(); require(stack.depth() == 3 && !context.music_exit_pending() && begin_exits == 0,
            "Home Exit did not push one safe confirmation above retained Home");
    };
    const auto reference = [&](bool yes)
    {
        UiText heading("independent-exit-heading", L"Exit", style.heading_font, Vector2F(0.0f, look.heading_y), app.render_resources(), art.ink);
        heading.set_position(Vector2F((menu_consts::DESIGN_RESOLUTION.x - heading.bounds().width) / 2.0f, look.heading_y));
        UiText question("independent-exit-question", L"Exit ColourWars?", style.detail_font, Vector2F(0.0f, look.status_y), app.render_resources(), art.ink);
        question.set_position(Vector2F((menu_consts::DESIGN_RESOLUTION.x - question.bounds().width) / 2.0f, look.status_y));
        UiText accept("independent-exit-yes", L"Yes", style.item_font, look.layout.position, app.render_resources(), yes ? art.focus_ink : art.ink);
        UiText cancel("independent-exit-no", L"No", style.item_font, look.layout.position + look.layout.spacing, app.render_resources(), yes ? art.ink : art.focus_ink);
        const RectangleF paper((menu_consts::DESIGN_RESOLUTION - look.box_size) / 2.0f, look.box_size);
        require(paper.contains(heading.bounds()) && paper.contains(question.bounds()) &&
            heading.bounds().bottom() < question.bounds().y && question.bounds().bottom() < accept.bounds().y - art.focus_padding.y,
            "Independent Home Exit heading/question no longer fit the authored paper and action spacing");
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        const auto viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
        DrawList list = renderer.view(0); list.set_viewport(viewport); list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        canvas.draw(list, RectangleF(Vector2F::ZERO, app.resolution_manager()->resolution_vec())); list.set_camera(camera);
        const SpriteSheet* sheet = app.render_resources()->sprite_sheet(style.sprite_sheet); const auto pixel = sheet->resolve_sprite_frame(style.fill_frame);
        // Home and its pushed child each dim the extra aspect-ratio margins.
        // The child's opaque design background covers all underlying Home art.
        sheet->draw(list, pixel, camera.visible_rectangle(viewport), style.scrim_colour);
        sheet->draw(list, pixel, camera.visible_rectangle(viewport), style.scrim_colour);
        sheet->draw(list, pixel, RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), original.main_menu().home_background_colour);
        setup_paper_reference(list, *app.render_resources(), style, art, paper, yes ? accept.bounds() : cancel.bounds());
        sheet->draw(list, pixel, RectangleF(0.0f, style.footer_y, menu_consts::DESIGN_RESOLUTION.x,
            menu_consts::DESIGN_RESOLUTION.y - style.footer_y), style.scrim_colour);
        sheet->draw(list, pixel, RectangleF(style.brand_position.x, style.footer_y,
            menu_consts::DESIGN_RESOLUTION.x - style.brand_position.x * 2.0f, style.rule_height), style.panel_edge_colour);
        list.set_filter(TextureFilter::linear); heading.draw(list); question.draw(list); accept.draw(list); cancel.draw(list);
        const auto& footer = original.prompt_bar();
        const UiTextDropShadow prompt("independent-exit-prompt", L"A/Enter Select    B/Esc Back", footer.font,
            footer.position, app.render_resources(), footer.colour, footer.shadow_colour, footer.shadow_offset);
        prompt.draw(list); return read(renderer);
    };
    std::ofstream records(output / "home-exit.csv"); records << "case,width,height,input_slot,yes,pending,full_frame_matches\n";
    const auto compare = [&](const std::string& name, bool yes, bool capture = false)
    {
        live(); const Frame actual = frame(), expected = reference(yes);
        const std::string stem = "home-exit-" + name + '-' + (device == keyboard ? "keyboard-" : "pad3-") +
            std::to_string(actual.width) + 'x' + std::to_string(actual.height);
        if (actual.pixels != expected.pixels)
        {
            save(actual, output / (stem + "-failure-actual.ppm")); save(expected, output / (stem + "-failure-expected.ppm"));
            const auto delta = foreground_delta_bounds(actual, expected);
            std::cerr << "HOME EXIT " << name << " delta=" << delta.x << ',' << delta.y << ',' << delta.width << ',' << delta.height << '\n';
        }
        require(stack.depth() == 3 && actual.pixels == expected.pixels,
            "Actual Home Exit differs from independent full paper/question/answer/footer pixels");
        std::srand(7907); const int next = std::rand(); std::srand(7907);
        require(actual.pixels == frame().pixels && actual.pixels == frame().pixels && std::rand() == next,
            "Frozen Home Exit changed pixels or draw RNG");
        if (capture)
        {
            save(actual, output / (stem + ".ppm"));
            const RectangleF paper((menu_consts::DESIGN_RESOLUTION - look.box_size) / 2.0f, look.box_size);
            const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), app.viewport_manager()->fullscreen_viewport());
            save(foreground_crop(actual, countdown_crop_bounds(camera.calculate_view_rectangle(paper)), 0), output / (stem + "-detail.ppm"));
        }
        records << name << ',' << actual.width << ',' << actual.height << ',' << device << ',' << yes << ',' << context.music_exit_pending() << ",true\n";
        return actual;
    };
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440),
        Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        resize(size); device = keyboard; open(); confirm(); const Frame first = compare("safe-no", false, true);
        tick(settled * 0.4f); tick(settled); require(first.pixels == frame().pixels, "Home Exit entry played a phantom cursor animation");
        tap(GamepadButton::a); home(); confirm(); compare("reopened-no", false);
        tap(GamepadButton::b); home(); confirm(); tap(GamepadButton::dpad_up); tick(settled); compare("yes-before-back", true, true);
        tap(GamepadButton::b); home(); confirm(); compare("reopened-after-yes-back", false);
        tap(GamepadButton::a); home();
        std::cout << "PASS Home Exit safe routes " << size.x << 'x' << size.y << '\n';
    }
    resize({ 1280, 720 });
    for (int slot : { keyboard, 3 })
    {
        device = slot; open(true); confirm(); tap(GamepadButton::dpad_up); tick(settled); compare("armed-yes", true);
        hold(slot, GamepadButton::a, true); tick();
        if (slot == keyboard)
        {
            static_cast<WindowNotify&>(app).on_deactivated(); tick(); static_cast<WindowNotify&>(app).on_activated(); tick();
        }
        else
        {
            painter_scripted_pads::states[3].connected = false; tick();
            painter_scripted_pads::states[3].connected = true; tick(); tick();
        }
        hold(slot, GamepadButton::a, false); tick(); tick(); compare("interrupted-confirm", true, true);
        require(!context.music_exit_pending() && begin_exits == 0, "Interrupted/reconnected Home Exit accepted a stale Yes release");
        hold(slot, GamepadButton::a, true); hold(slot, GamepadButton::b, true); tick();
        hold(slot, GamepadButton::a, false); hold(slot, GamepadButton::b, false); tick(); tick(); compare("conflicting-actions", true);
        tap(GamepadButton::b); home(); confirm(true); tap(GamepadButton::dpad_up);
        const Frame moving = frame(); require(moving.pixels != reference(true).pixels, "Home Exit fixture did not start an ordinary arrow response");
        tick(0.0f); tick(-0.5f); require(moving.pixels == frame().pixels, "Nonpositive time advanced Home Exit motion");
        play.reduce_effects = true; compare("reduce-immediate", true); play.reduce_effects = false;
        require(moving.pixels == frame().pixels, "Reduce-effects drawing consumed Home Exit motion");
        stack.notify_activation(false); compare("deactivated", true); stack.notify_activation(true); tick();
        tap(GamepadButton::dpad_down); stack.push(std::make_unique<MainMenuCredits>(&context)); stack.pop(); compare("cover-return", false); tick();
        tap(GamepadButton::dpad_up); active->debug_reset_input(slot); compare("developer-reset", true);
        require(active->debug_dismiss(false), "Home Exit refused its public safe developer dismissal"); home();
        play.reduce_effects = true; confirm(); const Frame quiet = compare("reduced-entry", false); tick(settled);
        require(quiet.pixels == frame().pixels, "Reduced Home Exit entry changed pixels"); tap(GamepadButton::b); home(); play.reduce_effects = false;
        confirm(); const Frame before = compare("before-resize", false); resize({ 1600, 1200 }); std::ignore = frame(); resize({ 1280, 720 });
        require(before.pixels == frame().pixels, "Home Exit changed after a resize round-trip"); tap(GamepadButton::b); home();
    }
    device = keyboard; open(true);
    // The Home callback must clear ownership of another device held on entry.
    hold(keyboard, GamepadButton::a, true); tick(); hold(3, GamepadButton::a, true); tick(); hold(3, GamepadButton::a, false); tick(); tick();
    hold(keyboard, GamepadButton::a, false); tick(); tick(); compare("cross-device-held-entry", false); tap(GamepadButton::b); home();
    for (bool entering : { true, false })
    {
        if (!entering) { confirm(); }
        hold(3, GamepadButton::a, true); hold(keyboard, GamepadButton::a, true); tick();
        hold(3, GamepadButton::a, false); hold(keyboard, GamepadButton::a, false); tick(); tick();
        if (entering) { compare("simultaneous-entry", false); tap(GamepadButton::b); home(); }
        else { home(); }
    }
    confirm(); hold(keyboard, GamepadButton::a, true); tick();
    hold(3, GamepadButton::dpad_up, true); hold(keyboard, GamepadButton::a, false); tick();
    hold(3, GamepadButton::dpad_up, false); tick(); tick();
    // Navigation wins that update; a later fresh release is required for Yes.
    require(!context.music_exit_pending() && begin_exits == 0, "Simultaneous Home Exit navigation and confirm accepted Yes in one update");
    tick(settled); compare("simultaneous-move-confirm", true); tap(GamepadButton::b); home();
    confirm(); tap(GamepadButton::dpad_up); tick(settled);
    hold(3, GamepadButton::b, true); hold(keyboard, GamepadButton::a, true); tick();
    hold(3, GamepadButton::b, false); hold(keyboard, GamepadButton::a, false); tick(); tick(); home();
    confirm(); const_cast<MenuText&>(presentation.menu_text()).home_subtitle = L"New Home copy after the child opened.";
    tap(GamepadButton::b); home(); presentation = original;
    // Hold the original music-exit callback, proving a single accepted begin
    // while repeated inputs, interruptions and developer dismissal remain safe.
    confirm(true); tap(GamepadButton::dpad_up); tick(settled); tap(GamepadButton::a);
    require(context.music_exit_pending() && begin_exits == 1, "Deliberate Home Exit Yes failed to latch exactly one music-exit begin");
    const Frame pending = compare("pending-yes", true, true);
    tap(GamepadButton::a); tap(GamepadButton::b); tap(GamepadButton::dpad_down); tick(settled);
    static_cast<WindowNotify&>(app).on_deactivated(); tick(); static_cast<WindowNotify&>(app).on_activated(); tick();
    require(context.music_exit_pending() && begin_exits == 1 && pending.pixels == frame().pixels,
        "Pending Home Exit accepted repeated input, restarted music exit or changed outgoing pixels");
    compare("pending-interrupted", true); require(active->debug_dismiss(false), "Pending Home Exit developer dismissal failed");
    begin_exits = 0; home();
    require(records.good(), "Home Exit evidence write failed");
    // Last: the real no-music-update/muted path closes only this hidden window.
    confirm(); tap(GamepadButton::dpad_up); tick(settled); compare("final-deliberate-yes", true);
    context.set_music_update({}); tap(GamepadButton::a);
    require(IsWindow(window) == FALSE && !context.music_exit_pending() && begin_exits == 0,
        "Deliberate Home Exit Yes did not immediately close its own hidden window without a music clock");
    std::cout << "PASS Home Exit independent six-size confirmation paper/text; safe retained-Home return, repeat/rearm/reconnect/multiple-device safety, reduction/lifecycle/frozen RNG, one pending music begin and deliberate muted hidden-window quit\n";
}
