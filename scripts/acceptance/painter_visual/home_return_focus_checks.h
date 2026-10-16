// Focus is observed through complete independent Home rasters and fresh input
// into independently opened real destinations; no protected cursor inspection.
void home_return_focus_matrix(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    const auto& style = presentation.menu(); const auto& look = presentation.main_menu();
    const auto& art = look.art; const auto& words = presentation.menu_text();
    require(words.heading(MenuHeading::home) == L"Make your mark." &&
        words.home_subtitle == L"Paint the arena. Claim the win." &&
        words.item(MenuItem::play) == L"Play" && words.item(MenuItem::options) == L"Options" &&
        words.item(MenuItem::credits) == L"Credits" && words.item(MenuItem::exit) == L"Exit",
        "Home return-focus copy drifted from its independent literals");
    Save preferences; auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0; play.reduce_effects = false;
    GameData data; data.set_application(&app); data.set_save(&preferences);
    data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings); StateContext& stack = app;
    MenuPage* active_home = nullptr; bool pad = false;
    struct ClearHomeReturn
    {
        StateContext& stack;
        ~ClearHomeReturn() { this->stack.clear(); painter_scripted_pads::states = {}; }
    } clear{ stack };
    const float settled = std::max(style.motion.focus_duration, style.motion.press_duration) + 0.01f;
    const auto tick = [&](float elapsed = 0.0f)
        { app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed); };
    const auto hold = [&](GamepadButton button, bool down)
    {
        if (pad)
        {
            const auto bit = static_cast<uint16_t>(1u << static_cast<unsigned int>(button));
            auto& buttons = painter_scripted_pads::states[3].buttons;
            buttons = down ? static_cast<uint16_t>(buttons | bit) :
                static_cast<uint16_t>(buttons & static_cast<uint16_t>(~bit));
        }
        else
        {
            const Key key = button == GamepadButton::a ? Key::enter :
                button == GamepadButton::b ? Key::escape :
                button == GamepadButton::dpad_up ? Key::up : Key::down;
            if (down) { app.keyboard()->on_key_down(key); } else { app.keyboard()->on_key_up(key); }
        }
    };
    const auto tap = [&](GamepadButton button)
        { hold(button, true); tick(); hold(button, false); tick(); };
    const auto frame = [&] { return main_menu_frame(app); };
    const auto resize = [&](Vector2I size)
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
    };
    const auto open = [&](int page, int initial_focus = 0)
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (Key key : { Key::enter, Key::escape, Key::up, Key::down, Key::space }) { app.keyboard()->on_key_up(key); }
        painter_scripted_pads::states = {}; painter_scripted_pads::states[3].connected = pad;
        app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll();
        app.mouse()->poll(); app.mouse()->poll(); input.reset_actions(); settings = MenuLevelSettings();
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas)); active_home = nullptr;
        if (page == 0)
        {
            auto home = initial_focus == 0 ? std::make_unique<MainMenuHome>(&context) :
                std::make_unique<MainMenuHome>(&context, initial_focus == 1 ?
                    MainMenuHome::InitialFocus::options : MainMenuHome::InitialFocus::credits);
            active_home = home.get(); stack.push(std::move(home));
        }
        else if (page == 1) { stack.push(std::make_unique<MainMenuTitle>(&context)); }
        else if (page == 2) { stack.push(std::make_unique<MainMenuModeSelect>(&context)); }
        else if (page == 3) { stack.push(std::make_unique<MainMenuOptions>(&context)); }
        else { stack.push(std::make_unique<MainMenuCredits>(&context)); }
        tick(); tick(); require(stack.depth() == 2 && !context.music_exit_pending(), "Home return fixture did not open a neutral real page");
    };
    const auto reference = [&](int selected)
    {
        std::vector<std::unique_ptr<UiText>> text;
        const auto label = [&](const std::wstring& value, const std::string& font, Vector2F position, Colour ink)
        {
            auto item = std::make_unique<UiText>("independent-home-return", value, font, position, app.render_resources(), ink);
            auto* raw = item.get(); text.push_back(std::move(item)); return raw;
        };
        label(L"Make your mark.", style.heading_font, look.home_position, art.ink);
        label(L"Paint the arena. Claim the win.", style.detail_font, look.home_subtitle_position, art.ink);
        const std::array<std::wstring, 4> actions{ L"Play", L"Options", L"Credits", L"Exit" };
        std::array<RectangleF, 4> rows;
        for (int row = 0; row < 4; ++row)
        {
            rows[static_cast<size_t>(row)] = label(actions[static_cast<size_t>(row)], style.item_font,
                look.home_position + Vector2F(0.0f, look.home_spacing.y * static_cast<float>(row + 2)),
                row == selected ? art.focus_ink : art.ink)->bounds();
        }
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        const auto viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
        DrawList list = renderer.view(0); list.set_viewport(viewport); list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        canvas.draw(list, RectangleF(Vector2F::ZERO, app.resolution_manager()->resolution_vec())); list.set_camera(camera);
        const SpriteSheet* sheet = app.render_resources()->sprite_sheet(style.sprite_sheet); const auto pixel = sheet->resolve_sprite_frame(style.fill_frame);
        sheet->draw(list, pixel, camera.visible_rectangle(viewport), style.scrim_colour);
        sheet->draw(list, pixel, RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), look.home_background_colour);
        const RectangleF panel((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - look.home_box_size) / 2.0f, look.home_box_size);
        setup_paper_reference(list, *app.render_resources(), style, art, panel, RectangleF());
        auto card_art = art; card_art.paper = art.focus_fill;
        RectangleF primary = rows[0]; primary.inflate(art.focus_padding);
        setup_paper_reference(list, *app.render_resources(), style, card_art, primary, RectangleF());
        RectangleF focused = rows[static_cast<size_t>(selected)]; focused.inflate(art.focus_padding);
        if (selected != 0) { setup_paper_reference(list, *app.render_resources(), style, card_art, focused, RectangleF()); }
        const auto ink = [&](RectangleF bounds)
        {
            sheet->draw(list, pixel, bounds, Colour(art.ink.r * art.ink.a, art.ink.g * art.ink.a, art.ink.b * art.ink.a, art.ink.a));
        };
        const float pen = art.keyline_width; const Vector2F tip(focused.x - pen * 3.0f, focused.center().y);
        ink(RectangleF(tip.x - pen * 4.0f, tip.y - pen, pen * 4.0f, pen * 2.0f));
        for (int step = 0; step < 3; ++step)
        {
            const float reach = static_cast<float>(step);
            ink(RectangleF(tip.x - reach * pen, tip.y - (reach + 1.0f) * pen, pen, (reach + 1.0f) * pen * 2.0f));
        }
        sheet->draw(list, pixel, RectangleF(0.0f, style.footer_y, menu_consts::DESIGN_RESOLUTION.x,
            menu_consts::DESIGN_RESOLUTION.y - style.footer_y), style.scrim_colour);
        sheet->draw(list, pixel, RectangleF(style.brand_position.x, style.footer_y,
            menu_consts::DESIGN_RESOLUTION.x - style.brand_position.x * 2.0f, style.rule_height), style.panel_edge_colour);
        list.set_filter(TextureFilter::linear); for (const auto& item : text) { item->draw(list); }
        const auto& footer = presentation.prompt_bar();
        const UiTextDropShadow prompt("independent-home-return-prompt", L"A/Enter Select    B/Esc Back", footer.font,
            footer.position, app.render_resources(), footer.colour, footer.shadow_colour, footer.shadow_offset);
        prompt.draw(list); return read(renderer);
    };
    std::ofstream records(output / "home-return-focus.csv"); records << "case,width,height,input_slot,focus,full_frame_matches\n";
    const auto compare = [&](const std::string& name, int selected, bool capture = false)
    {
        const Frame actual = frame(), expected = reference(selected);
        const std::string stem = "home-return-focus-" + name + (pad ? "-pad3-" : "-keyboard-") +
            std::to_string(actual.width) + 'x' + std::to_string(actual.height);
        if (actual.pixels != expected.pixels)
        {
            save(actual, output / (stem + "-failure-actual.ppm")); save(expected, output / (stem + "-failure-expected.ppm"));
            const RectangleI delta = foreground_delta_bounds(actual, expected);
            std::cerr << "HOME RETURN " << name << " focus=" << selected << " delta=" << delta.x << ',' << delta.y << ',' << delta.width << ',' << delta.height << '\n';
        }
        require(stack.depth() == 2 && !context.music_exit_pending() && actual.pixels == expected.pixels,
            "Actual Home return differs from its independent selected-row paper/text/arrow raster");
        std::srand(1367); const int next = std::rand(); std::srand(1367);
        require(actual.pixels == frame().pixels && actual.pixels == frame().pixels && std::rand() == next &&
            settings.player_count() == 0 && settings.best_of() == 1 && !settings.duration().has_value() && settings.stage().empty(),
            "Frozen Home return changed pixels, plan or draw RNG");
        if (capture)
        {
            save(actual, output / (stem + ".ppm"));
            const RectangleF panel((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - look.home_box_size) / 2.0f, look.home_box_size);
            const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), app.viewport_manager()->fullscreen_viewport());
            save(foreground_crop(actual, countdown_crop_bounds(camera.calculate_view_rectangle(panel)), 0), output / (stem + "-detail.ppm"));
        }
        records << name << ',' << actual.width << ',' << actual.height << ',' << (pad ? 3 : input_slots::keyboard_and_mouse) << ',' << selected << ",true\n";
        return actual;
    };
    const auto still = [&](const std::string& name, int selected, bool capture = false)
    {
        const Frame immediate = compare(name, selected, capture); tick(0.0f); tick(-0.5f); tick(settled * 0.4f); tick(settled);
        require(immediate.pixels == frame().pixels, "Restored Home played a phantom focus/press entry animation");
    };
    const auto child_matches = [&](const Frame& expected)
    {
        require(stack.depth() == 2 && !context.music_exit_pending() && expected.pixels == frame().pixels,
            "Fresh Home confirmation opened the wrong real destination");
    };
    const auto focus = [&](int selected)
    {
        for (int move = 0; move < selected; ++move) { tap(GamepadButton::dpad_down); }
        tick(settled);
    };
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440),
        Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        resize(size); pad = false;
        std::array<Frame, 3> children;
        for (int child = 0; child < 3; ++child) { open(child + 2); children[static_cast<size_t>(child)] = frame(); }
        open(0); still("initial-default", 0, true);
        open(1); tap(GamepadButton::a); still("title-entry", 0);
        tap(GamepadButton::a); child_matches(children[0]); tap(GamepadButton::b); still("mode-back", 0);
        for (int selected : { 1, 2 })
        {
            open(0); focus(selected); compare("before-child-" + std::to_string(selected), selected);
            tap(GamepadButton::a); child_matches(children[static_cast<size_t>(selected)]);
            tap(GamepadButton::b); still("back-action-" + std::to_string(selected), selected, true);
            // No direction is sent: the newly restored cursor must reopen its origin.
            tap(GamepadButton::a); child_matches(children[static_cast<size_t>(selected)]); tick();
            if (selected == 1) { for (int row = 0; row < 5; ++row) { tap(GamepadButton::dpad_down); } }
            tap(GamepadButton::a); still("back-row-" + std::to_string(selected), selected, true);
            tap(GamepadButton::a); child_matches(children[static_cast<size_t>(selected)]);
            tap(GamepadButton::b); still("repeat-back-" + std::to_string(selected), selected);
        }
        std::cout << "PASS Home return routes " << size.x << 'x' << size.y << '\n';
    }
    resize({ 1280, 720 });
    for (bool use_pad : { false, true })
    {
        pad = use_pad; std::array<Frame, 3> children;
        for (int child = 0; child < 3; ++child) { open(child + 2); children[static_cast<size_t>(child)] = frame(); }
        for (int selected : { 0, 1, 2 })
        {
            open(0, selected); still("explicit-initial-" + std::to_string(selected), selected, pad);
            hold(GamepadButton::a, true); tick(); compare("held-confirm-" + std::to_string(selected), selected);
            if (pad) { active_home->debug_reset_input(3); }
            else
            {
                static_cast<WindowNotify&>(app).on_deactivated(); tick();
                static_cast<WindowNotify&>(app).on_activated(); tick();
            }
            hold(GamepadButton::a, false); tick(); tick(); still("interrupted-release-" + std::to_string(selected), selected);
            tap(GamepadButton::a); child_matches(children[static_cast<size_t>(selected)]);
            tap(GamepadButton::b); still("fresh-confirm-return-" + std::to_string(selected), selected, pad);
            // Exercise each on-screen Back route again with the sparse pad.
            if (selected != 0)
            {
                tap(GamepadButton::a); child_matches(children[static_cast<size_t>(selected)]); tick();
                if (selected == 1) { for (int row = 0; row < 5; ++row) { tap(GamepadButton::dpad_down); } }
                tap(GamepadButton::a); still("repeat-back-row-" + std::to_string(selected), selected);
            }
        }
        open(0, 1); tap(GamepadButton::dpad_down); const Frame moving = frame();
        require(moving.pixels != reference(2).pixels, "Home return fixture did not start an ordinary cursor response");
        tick(0.0f); tick(-0.5f); require(moving.pixels == frame().pixels, "Nonpositive time advanced Home focus motion");
        play.reduce_effects = true; compare("reduce-immediate", 2);
        play.reduce_effects = false; require(moving.pixels == frame().pixels, "Reduce-effects drawing consumed Home focus motion");
        stack.notify_activation(false); compare("deactivated-focus", 2); stack.notify_activation(true); tick();
        tap(GamepadButton::dpad_up); require(frame().pixels != reference(1).pixels, "Home cover fixture did not start cursor motion");
        stack.push(std::make_unique<MainMenuCredits>(&context)); stack.pop(); still("cover-return", 1); tick();
        tap(GamepadButton::dpad_down); active_home->debug_reset_input(pad ? 3 : input_slots::keyboard_and_mouse);
        still("developer-reset-focus", 2);
        play.reduce_effects = true; open(0, 1); still("reduced-initial", 1);
        tap(GamepadButton::a); child_matches(children[1]); tap(GamepadButton::b); still("reduced-child-return", 1);
        play.reduce_effects = false;
        const Frame before = compare("before-resize", 1); resize({ 1600, 1200 }); std::ignore = frame(); resize({ 1280, 720 });
        require(before.pixels == frame().pixels, "Home restored focus changed after a resize round-trip");
    }
    require(records.good(), "Home return-focus evidence write failed");
    std::cout << "PASS Home origin focus: independent complete rasters; default/Title/Mode Play, both Options/Credits returns, fresh reopen, keyboard/sparse-pad rearming, lifecycle/reduction/frozen RNG\n";
}
