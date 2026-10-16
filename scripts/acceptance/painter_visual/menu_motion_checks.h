// Real shared-cursor pages, driven through public local keyboard messages.
// These probes sample moving and resting production rasters separately; no
// test changes a widget position or advances cosmetic time from drawing.
void menu_motion_matrix(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    struct MotionProfile
    {
        std::wstring previous;
        explicit MotionProfile(const std::filesystem::path& root)
        {
            wchar_t* raw = nullptr;
            size_t length = 0;
            require(_wdupenv_s(&raw, &length, L"LOCALAPPDATA") == 0,
                "Cannot read the isolated painter profile");
            if (raw != nullptr) { this->previous = raw; std::free(raw); }
            std::filesystem::create_directories(root);
            require(_wputenv_s(L"LOCALAPPDATA", root.c_str()) == 0,
                "Cannot isolate the menu-motion preferences");
        }
        ~MotionProfile() { _wputenv_s(L"LOCALAPPDATA", this->previous.c_str()); }
    } isolated(output / "menu-motion-profile");
    Save preferences;
    PlaySettings initial;
    initial.music_volume = initial.sfx_volume = 0;
    initial.reduce_effects = false;
    require(preferences.set_play_settings_and_save(initial),
        "Cannot seed the isolated menu-motion preferences");
    auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    GameData data;
    data.set_application(&app); data.set_save(&preferences); data.set_tuning(&tuning);
    data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings);
    StateContext& stack = app;
    struct ClearMotionStates
    {
        StateContext& stack;
        ~ClearMotionStates() { this->stack.clear(); }
    } clear{ stack };
    const MenuStyle& style = presentation.menu();
    const MenuFocusMotionPresentation motion = style.motion;
    require(motion.arrow_travel > 0.0f && motion.press_fraction > 0.0f,
        "The shipped motion matrix requires visible focus and press responses");
    const float settle_time = std::max(motion.focus_duration, motion.press_duration) + 0.01f;
    const auto tick = [&](float elapsed = 0.0f)
    {
        app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed);
    };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick();
    };
    const auto open = [&](std::unique_ptr<State> page)
    {
        stack.clear(); stack.notify_activation(true);
        app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::up, Key::down, Key::left, Key::right,
            Key::enter, Key::space, Key::escape }) { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll();
        input.reset_actions(); stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        stack.push(std::move(page)); tick(); tick();
        require(stack.depth() == 2, "The menu-motion production page did not open");
    };
    const auto resize_page = [&](Vector2I size)
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
    };
    const auto project = [&](RectangleF bounds)
    {
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        camera.calculate_view_rectangle(bounds);
        return countdown_crop_bounds(bounds);
    };
    const auto measured = [&](const std::string& font, const std::wstring& text, Vector2F position)
    {
        return RectangleF(position, app.render_resources()->measure_text(
            app.render_resources()->resolve_sprite_font(font), text));
    };
    const auto arrow_bounds = [](RectangleF row, const MenuPaperArtPresentation& art)
    {
        row.inflate(art.focus_padding);
        const float pen = art.keyline_width;
        // Includes the full permitted inward travel and a native-pixel guard.
        return RectangleF(row.x - pen * 8.0f - 2.0f, row.center().y - pen * 4.0f - 2.0f,
            pen * 10.0f + 4.0f, pen * 8.0f + 4.0f);
    };
    const auto frozen = [&](const Frame& frame)
    {
        std::srand(4673); const int expected = std::rand(); std::srand(4673);
        require(frame.pixels == main_menu_frame(app).pixels &&
            frame.pixels == main_menu_frame(app).pixels && std::rand() == expected,
            "Frozen menu-motion drawing changed pixels or consumed RNG");
    };
    std::ofstream records(output / "menu-motion.csv");
    records << "case,width,height,delta_x,delta_y,delta_width,delta_height\n";
    const auto changed = [&](const char* name, const Frame& moving, const Frame& resting,
        const RectangleF& envelope)
    {
        const RectangleI bounds = foreground_delta_bounds(moving, resting);
        const RectangleI allowed = project(envelope);
        if (bounds.width <= 0 || bounds.height <= 0 || !allowed.contains(bounds))
        {
            save(moving, output / (std::string("menu-motion-") + name + "-failure-moving.ppm"));
            save(resting, output / (std::string("menu-motion-") + name + "-failure-rest.ppm"));
            std::cerr << "MOTION " << name << " delta=" << bounds.x << ',' << bounds.y << ',' <<
                bounds.width << ',' << bounds.height << " allowed=" << allowed.x << ',' << allowed.y <<
                ',' << allowed.width << ',' << allowed.height << '\n';
        }
        require(bounds.width > 0 && bounds.height > 0 && allowed.contains(bounds),
            "Menu motion disappeared or changed pixels outside its ornament envelope");
        records << name << ',' << moving.width << ',' << moving.height << ',' << bounds.x << ',' <<
            bounds.y << ',' << bounds.width << ',' << bounds.height << '\n';
    };
    const auto capture = [&](const char* name, const Frame& frame, RectangleF detail)
    {
        const std::string stem = std::string("menu-motion-") + name + '-' +
            std::to_string(frame.width) + 'x' + std::to_string(frame.height);
        save(frame, output / (stem + ".ppm"));
        save(foreground_crop(frame, project(detail)), output / (stem + "-detail.ppm"));
    };

    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
    {
        resize_page(size);
        require(preferences.set_play_settings_and_save(initial), "Cannot reset menu-motion preferences");
        open(std::make_unique<MainMenuHome>(&context));
        const Frame first = main_menu_frame(app); tick(settle_time);
        require(first.pixels == main_menu_frame(app).pixels,
            "A newly opened menu animated its initial focus");
        const auto& home = presentation.main_menu();
        const auto home_row = [&](MenuItem item, int row)
        {
            return measured(style.item_font, presentation.menu_text().item(item),
                home.home_position + Vector2F(0.0f, home.home_spacing.y * static_cast<float>(row + 2)));
        };
        const RectangleF options_arrow = arrow_bounds(home_row(MenuItem::options, 1), home.art);
        tap(Key::down);
        const Frame arrival = main_menu_frame(app); frozen(arrival);
        tick(0.0f); tick(-0.5f);
        require(arrival.pixels == main_menu_frame(app).pixels,
            "Nonpositive menu time advanced the focus ornament");
        tick(motion.focus_duration * 0.4f); const Frame middle = main_menu_frame(app); frozen(middle);
        tick(settle_time); const Frame rest = main_menu_frame(app);
        changed("home-focus-initial", arrival, rest, options_arrow);
        changed("home-focus-middle", middle, rest, options_arrow);
        require(arrival.pixels != middle.pixels, "Focus ornament did not progress with positive time");
        // Applied preferences are borrowed during draw, so suppression does
        // not need another simulation update and drawing cannot consume age.
        tap(Key::up); tap(Key::down); const Frame live = main_menu_frame(app);
        play.reduce_effects = true;
        require(rest.pixels == main_menu_frame(app).pixels,
            "Applied Reduce effects did not suppress focus motion immediately");
        play.reduce_effects = false;
        require(live.pixels == main_menu_frame(app).pixels,
            "Suppressed drawing mutated the focus-motion clock");
        stack.notify_activation(false);
        require(rest.pixels == main_menu_frame(app).pixels,
            "Deactivation did not clear the current focus motion");
        stack.notify_activation(true); tick();
        require(rest.pixels == main_menu_frame(app).pixels,
            "Activation restarted interrupted focus motion");
        // Several changes before any positive time may restart one response,
        // but only the last selected row may retain moving pixels.
        tap(Key::down); tap(Key::down); tap(Key::up);
        const Frame rapid = main_menu_frame(app); frozen(rapid); tick(settle_time);
        changed("home-rapid-focus", rapid, main_menu_frame(app),
            arrow_bounds(home_row(MenuItem::credits, 2), home.art));
        capture("home-arrival", arrival, options_arrow);
        capture("home-middle", middle, options_arrow);
        capture("home-rest", rest, options_arrow);

        open(std::make_unique<ControlsSettingsMenu>(&context));
        const auto& controls = presentation.controls_settings();
        const auto& art = controls.art;
        tap(Key::down); tick(settle_time);
        tap(Key::right);
        const Frame pressed = main_menu_frame(app); frozen(pressed);
        tick(motion.press_duration * 0.4f); const Frame release = main_menu_frame(app); frozen(release);
        tick(settle_time); const Frame released = main_menu_frame(app);
        RectangleF music_row(controls.layout.position + Vector2F(0.0f, controls.layout.spacing.y),
            Vector2F(controls.controls_position.x - controls.layout.position.x -
                art.focus_padding.x * 2.0f - art.shadow_offset, controls.layout.spacing.y));
        music_row.inflate(art.focus_padding + Vector2F(2.0f));
        changed("controls-value-press", pressed, released, music_row);
        changed("controls-value-release", release, released, music_row);
        require(pressed.pixels != release.pixels,
            "Selection paper did not release with positive time");
        RectangleF label = measured(controls.item_font,
            presentation.menu_text().item(MenuItem::music_volume), controls.layout.position +
                Vector2F(0.0f, controls.layout.spacing.y));
        RectangleF value = measured(controls.item_font, L"10%", controls.layout.position +
            Vector2F(controls.layout.spacing.x, controls.layout.spacing.y));
        const float text_height = controls.layout.spacing.y - art.focus_padding.y * 2.0f;
        label = RectangleF(label.position(), label.size() * std::min({ 1.0f,
            (controls.layout.spacing.x - art.focus_padding.x) / label.width, text_height / label.height }));
        value = RectangleF(value.position(), value.size() * std::min({ 1.0f,
            (controls.controls_position.x - art.focus_padding.x * 2.0f - art.shadow_offset -
                value.x) / value.width, text_height / value.height }));
        const auto fixed_lettering = [&](const RectangleF& text, const std::wstring& words,
            const Colour& ink)
        {
            UiText expected("motion-fixed-text", words, controls.item_font, text.position(),
                app.render_resources(), ink);
            expected.set_scale(text.height / measured(controls.item_font, words, text.position()).height);
            const auto fixture = [&](const Colour& background)
            {
                Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
                DrawList list = renderer.view(0);
                const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
                const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
                list.set_viewport(viewport); list.set_camera(camera); list.set_filter(TextureFilter::point);
                const Canvas paper(app.render_resources(), background);
                paper.draw(list, camera.visible_rectangle(viewport));
                list.set_filter(TextureFilter::linear); expected.draw(list);
                return read(renderer);
            };
            const Frame black = fixture(Colour::black), white = fixture(Colour::white);
            const RectangleI pixels = project(text);
            size_t solid_ink = 0;
            for (int y = pixels.y; y < pixels.bottom(); ++y)
            {
                for (int x = pixels.x; x < pixels.right(); ++x)
                {
                    const size_t offset = hud_pixel_offset(pressed, x, y);
                    // Equal independent lettering over black/white is fully
                    // opaque. Antialiased edges blend with the paper shade;
                    // opaque glyph interiors retain their exact coordinates.
                    if (!same_pixel(black, white, offset)) { continue; }
                    ++solid_ink;
                    require(same_pixel(pressed, released, offset) && same_pixel(pressed, black, offset),
                        "Pressed paper moved its opaque label/value lettering");
                }
            }
            require(solid_ink > 20, "Menu-motion text probe found too little solid lettering");
        };
        fixed_lettering(label, presentation.menu_text().item(MenuItem::music_volume), art.focus_ink);
        fixed_lettering(value, L"10%", art.ink);
        capture("controls-pressed", pressed, music_row);
        capture("controls-releasing", release, music_row);
        capture("controls-rest", released, music_row);
        resize_page({ 1920, 1080 }); std::ignore = main_menu_frame(app); resize_page(size);
        require(released.pixels == main_menu_frame(app).pixels,
            "Settled selection paper changed after a resize round-trip");
        tap(Key::left); const Frame repeat_press = main_menu_frame(app);
        play.reduce_effects = true; const Frame reduced_press = main_menu_frame(app);
        play.reduce_effects = false;
        require(repeat_press.pixels == main_menu_frame(app).pixels,
            "Suppressed selection drawing consumed the paper press");
        tick(settle_time);
        require(reduced_press.pixels == main_menu_frame(app).pixels,
            "Applied Reduce effects did not suppress selection paper immediately");
        tap(Key::right); stack.notify_activation(false);
        const Frame interrupted_press = main_menu_frame(app); stack.notify_activation(true); tick(settle_time);
        require(interrupted_press.pixels == main_menu_frame(app).pixels,
            "Deactivation retained or restarted a selection paper press");

        // The real child pushes over this parent, then pops back to the same
        // focus. Its successful activation must leave no pressed parent card.
        tap(Key::up); tick(settle_time); const Frame parent = main_menu_frame(app);
        tap(Key::enter); require(stack.depth() == 3, "Pad sticks did not push its actual child");
        tap(Key::escape); require(stack.depth() == 2 && parent.pixels == main_menu_frame(app).pixels,
            "Suspended/resumed Controls retained its successful child-activation press");
        tick(); // Arm fresh parent navigation only after returning to neutral.

        for (int row = 0; row < 6; ++row) { tap(Key::down); }
        tick(settle_time); tap(Key::enter);
        const Frame applied_press = main_menu_frame(app); frozen(applied_press); tick(settle_time);
        const Frame applied_rest = main_menu_frame(app);
        RectangleF apply_row = measured(controls.item_font, presentation.menu_text().item(MenuItem::apply),
            controls.layout.position + Vector2F(0.0f, controls.layout.spacing.y * 6.0f));
        apply_row.inflate(art.focus_padding + Vector2F(2.0f));
        changed("controls-apply-press", applied_press, applied_rest, apply_row);
        capture("apply-pressed", applied_press, apply_row);
        capture("apply-rest", applied_rest, apply_row);

        // Stage the reduction choice through the actual OptionRow, then
        // deliberately Apply it. The applied page is already completely still.
        tap(Key::up);
        tap(Key::right); require(!play.reduce_effects, "An un-applied reduction choice leaked into preferences");
        tap(Key::down); tap(Key::enter);
        require(play.reduce_effects, "Actual Controls Apply did not commit Reduce effects");
        Save reloaded; reloaded.load_save_file();
        require(reloaded.play_settings().reduce_effects, "Applied Reduce effects did not survive save reload");
        const Frame reduced_apply = main_menu_frame(app); tick(settle_time);
        require(reduced_apply.pixels == main_menu_frame(app).pixels,
            "Applying Reduce effects left an animated selection paper");
        tap(Key::up); const Frame reduced_focus = main_menu_frame(app); tick(settle_time);
        require(reduced_focus.pixels == main_menu_frame(app).pixels,
            "Applied Reduce effects allowed a subsequent focus animation");
        tap(Key::left); const Frame pending_off = main_menu_frame(app); tick(settle_time);
        require(play.reduce_effects && pending_off.pixels == main_menu_frame(app).pixels,
            "An unapplied reduction reversal restarted menu motion");
        tap(Key::down); tap(Key::enter); require(!play.reduce_effects,
            "Actual Controls Apply did not restore ordinary menu motion");
        for (int row = 0; row < 5; ++row) { tap(Key::up); }
        tick(settle_time); tap(Key::right);
        const Frame restored_press = main_menu_frame(app); frozen(restored_press); tick(settle_time);
        const Frame restored_rest = main_menu_frame(app);
        changed("controls-motion-restored", restored_press, restored_rest, music_row);
        capture("reduced-applied", reduced_apply, apply_row);
        capture("restored-value-pressed", restored_press, music_row);
    }
    require(records.good(), "Menu-motion evidence write failed");
    require(IsWindowVisible(static_cast<HWND>(app.window())) == FALSE &&
        GetForegroundWindow() != static_cast<HWND>(app.window()),
        "Menu-motion probes exposed or activated their hidden window");
    std::cout << "PASS actual menu motion: initial focus stays still; changed-focus arrow and accepted "
        "OptionRow paper initial/middle/rest and Apply pressed/rest rasters at 720p/1080p/QHD; local ornament bounds, "
        "fixed lettering, nonpositive time, rapid focus, immediate applied reduction, actual reduction "
        "Apply/reload/reversal, deactivation and nested child return; frozen redraw/RNG and resize\n";
}
