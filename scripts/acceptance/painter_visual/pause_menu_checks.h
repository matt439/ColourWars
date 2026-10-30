// Real pause and confirmation pages, with public keyboard input through the
// state stack. Preferences remain in memory; settings close without Apply.
class PauseMenuProbeRoot final : public State
{
public:
    PauseMenuProbeRoot(Application& app, const Canvas& canvas) : app_(app), canvas_(canvas) {}
    void init() override {}
    void update(float) override {}
    void draw(Renderer& renderer) const override
    {
        renderer.set_view_count(1);
        DrawList list = renderer.view(0);
        list.set_viewport(this->app_.viewport_manager()->fullscreen_viewport());
        list.set_camera(Camera::DEFAULT_CAMERA);
        list.set_filter(TextureFilter::point);
        this->canvas_.draw(list, RectangleF(Vector2F::ZERO,
            this->app_.resolution_manager()->resolution_vec()));
    }
private:
    Application& app_;
    const Canvas& canvas_;
};

Frame pause_menu_frame(Application& app)
{
    app.renderer()->begin_frame();
    static_cast<StateContext&>(app).draw(*app.renderer());
    return read(*app.renderer());
}

void pause_menu_item_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output);
void pause_question_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output);

void pause_menu_matrix(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    Save preferences; SettingsSession preferences_session(preferences);
    auto& play = const_cast<PlaySettings&>(preferences_session.play_settings());
    play.music_volume = play.sfx_volume = 0;
    GameData data;
    data.set_application(&app);
    data.set_save(&preferences); data.set_settings_session(&preferences_session);
    data.set_tuning(&tuning);
    data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MatchSetup settings;
    MenuContext context(&data, &input, &settings);
    StateContext& stack = app;
    struct ClearPauseStates
    {
        StateContext& stack;
        ~ClearPauseStates() { this->stack.clear(); }
    } clear{ stack };
    const MenuStyle& style = presentation.menu();
    const PauseMenuPresentation& look = presentation.pause_menu();
    auto& art = const_cast<MenuPaperArtPresentation&>(look.art);
    struct RestorePauseArt
    {
        MenuPaperArtPresentation& art;
        MenuPaperArtPresentation original;
        ~RestorePauseArt() { this->art = this->original; }
    } restore_art{ art, art };
    const RectangleF panel((menu_consts::DESIGN_RESOLUTION - look.box_size) / 2.0f,
        look.box_size);
    const auto item_font = app.render_resources()->resolve_sprite_font(style.item_font);
    const auto heading_font = app.render_resources()->resolve_sprite_font(style.heading_font);
    const auto detail_font = app.render_resources()->resolve_sprite_font(style.detail_font);
    const auto row_bounds = [&](const wchar_t* label, int row)
    {
        RectangleF bounds(look.layout.position + Vector2F(0.0f,
            look.layout.spacing.y * static_cast<float>(row)),
            app.render_resources()->measure_text(item_font, label));
        bounds.inflate(art.focus_padding + Vector2F(art.keyline_width + 2.0f));
        return bounds;
    };
    for (const auto& row : std::array<std::pair<const wchar_t*, int>, 6>{ {
        { L"Resume", 0 }, { L"Restart", 1 }, { L"Options", 2 },
        { L"Quit", 3 }, { L"Yes", 0 }, { L"No", 1 } } })
    {
        require(panel.contains(row_bounds(row.first, row.second)),
            "Pause action card escapes its authored paper panel");
        const Vector2F measured = app.render_resources()->measure_text(item_font, row.first);
        require(look.layout.spacing.y > measured.y + art.focus_padding.y * 2.0f,
            "Pause action cards overlap at the actual font height");
    }
    for (const wchar_t* title : { L"Pause Menu P1", L"Controller lost" })
    {
        const Vector2F measured = app.render_resources()->measure_text(heading_font, title);
        const RectangleF heading((menu_consts::DESIGN_RESOLUTION.x - measured.x) / 2.0f,
            look.heading_y, measured.x, measured.y);
        if (!panel.contains(heading) || heading.bottom() >= look.status_y)
        {
            std::cerr << "Pause heading measured=" << measured.x << ',' << measured.y <<
                " bounds=" << heading.x << ',' << heading.y << ',' << heading.width << ',' << heading.height <<
                " panel=" << panel.x << ',' << panel.y << ',' << panel.width << ',' << panel.height <<
                " status_y=" << look.status_y << '\n';
        }
        require(panel.contains(heading) && heading.bottom() < look.status_y,
            "Pause heading escapes its paper or overlaps status text");
    }
    for (const wchar_t* question : { L"Are you sure you want to restart?",
        L"Are you sure you want to quit?" })
    {
        const Vector2F measured = app.render_resources()->measure_text(detail_font, question);
        const RectangleF bounds((menu_consts::DESIGN_RESOLUTION.x - measured.x) / 2.0f,
            look.status_y, measured.x, measured.y);
        require(panel.contains(bounds) && bounds.bottom() < look.layout.position.y - art.focus_padding.y,
            "Pause confirmation question escapes its paper or overlaps its first action");
    }

    app.keyboard()->set_focused(true);
    app.mouse()->set_focused(true);
    for (const Key key : { Key::up, Key::down, Key::left, Key::right,
        Key::enter, Key::space, Key::escape })
    {
        app.keyboard()->on_key_up(key);
    }
    app.keyboard()->poll(); app.keyboard()->poll();
    app.mouse()->poll(); app.mouse()->poll();
    const auto tick = [&](float elapsed = 0.0f)
    {
        app.keyboard()->poll();
        app.mouse()->poll();
        stack.update(elapsed);
    };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick();
        app.keyboard()->on_key_up(key); tick();
        // This historical raster fixture samples the settled ornament pose.
        if (key == Key::up || key == Key::down || key == Key::left || key == Key::right)
        { app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.51f); }
    };
    const auto resize_page = [&](Vector2I size)
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
    };
    std::optional<PauseMenuAction> choice;
    const auto open = [&](int missing_pads = 0, bool keyboard_participates = true)
    {
        stack.clear();
        stack.notify_activation(true);
        input.reset_actions();
        choice.reset();
        settings.set_player_count(missing_pads + (keyboard_participates ? 1 : 0));
        settings.begin_input_claims(keyboard_participates ? input_slots::keyboard_and_mouse : 0);
        for (int pad = keyboard_participates ? 0 : 1; pad < missing_pads; ++pad)
        {
            require(!app.gamepads()->connected(pad),
                "Missing-controller pause fixture requires its authored pad slots to be absent");
            require(settings.claim_input_slot(pad), "Cannot assign missing-controller fixture roster");
        }
        if (!keyboard_participates)
        {
            require(!app.gamepads()->connected(0),
                "All-disconnected pause fixture requires pad slot zero to be absent");
        }
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        stack.push<PauseMenuAction>(std::make_unique<PauseMenuInitial>(&context,
            keyboard_participates ? input_slots::keyboard_and_mouse : 0),
            [&](const PauseMenuAction& result) { choice = result; });
        tick();
        require(stack.depth() == 2 && !choice.has_value(),
            "Actual pause page did not remain open on neutral input");
    };
    const auto confirmation = [&](bool quit)
    {
        for (int move = 0; move < (quit ? 3 : 1); ++move) { tap(Key::down); }
        tap(Key::enter);
        tick();
        require(stack.depth() == 2 && !choice.has_value(),
            "Pause confirmation did not replace its initial page in the typed frame");
    };
    const auto camera = [&]
    {
        return Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
    };
    const auto assert_focus = [&](const Frame& baseline, const RectangleF& expected)
    {
        const Colour original = art.focus_fill;
        art.focus_fill = Colour(0.0f, 1.0f, 1.0f);
        const Frame tinted = pause_menu_frame(app);
        art.focus_fill = original;
        const RectangleI changed = foreground_delta_bounds(baseline, tinted);
        const RectangleI row = countdown_crop_bounds(camera().calculate_view_rectangle(expected));
        require(changed.width > 0 && changed.height > 0 && row.contains(changed),
            "Pause paper focus did not follow the actual enabled action row");
        const size_t fill_pixels = foreground_local_delta(baseline, tinted,
            RectangleF(Vector2F(row.position()), Vector2F(row.size())));
        // The detached arrow is a shape cue independent of the text colour.
        // Observe the real artwork's ink only to the left of this action,
        // away from both the card keyline and the page's outer frame.
        const float pen = art.keyline_width;
        const float left = expected.x + pen + 2.0f;
        const RectangleF arrow(left - pen * 8.0f, expected.center().y - pen * 4.0f,
            pen * 7.0f, pen * 8.0f);
        const Colour original_ink = art.ink;
        art.ink = Colour(1.0f, 0.0f, 1.0f);
        const Frame tinted_ink = pause_menu_frame(app);
        art.ink = original_ink;
        const RectangleI arrow_pixels = countdown_crop_bounds(camera().calculate_view_rectangle(arrow));
        const Frame arrow_before = foreground_crop(baseline, arrow_pixels, 0);
        const Frame arrow_after = foreground_crop(tinted_ink, arrow_pixels, 0);
        size_t arrow_changed = 0;
        for (size_t offset = 0; offset < arrow_before.pixels.size(); offset += 4)
        {
            if (!same_pixel(arrow_before, arrow_after, offset)) { ++arrow_changed; }
        }
        require(arrow_changed > 5,
            "Actual enabled pause focus has no detached ink arrow");
        return fill_pixels;
    };
    std::ofstream records(output / "pause-menu.csv");
    records << "page,width,height,paper_pixels,focus_pixels\n";
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080),
        Vector2I(2560, 1440), Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        resize_page(size);
        for (const int phase : { 0, 1, 2, 3 })
        {
            const char* name = phase == 0 ? "initial" : phase == 1 ? "restart" :
                phase == 2 ? "quit" : "missing-pad";
            open(phase == 3 ? 1 : 0);
            if (phase == 1 || phase == 2) { confirmation(phase == 2); }
            const Frame normal = pause_menu_frame(app);
            std::srand(2137);
            const int expected_random = std::rand();
            std::srand(2137);
            require(normal.pixels == pause_menu_frame(app).pixels &&
                normal.pixels == pause_menu_frame(app).pixels && std::rand() == expected_random,
                "Frozen pause drawing changed pixels or consumed random numbers");
            tick(0.75f);
            require(normal.pixels == pause_menu_frame(app).pixels && !choice.has_value(),
                "Neutral pause time moved artwork or selected an action");
            play.reduce_effects = true;
            tick(0.75f);
            require(normal.pixels == pause_menu_frame(app).pixels,
                "Reduce effects changed the static readable pause artwork");
            play.reduce_effects = false;
            RectangleF frame_bounds = panel;
            frame_bounds.inflate(Vector2F(art.keyline_width + art.shadow_offset + 4.0f));
            camera().calculate_view_rectangle(frame_bounds);
            const RectangleI pixels = countdown_crop_bounds(frame_bounds);
            require(pixels.x >= 0 && pixels.y >= 0 && pixels.right() <= size.x && pixels.bottom() <= size.y,
                "Pause paper escaped a supported display/aspect ratio");
            const Colour original_paper = art.paper;
            art.paper = Colour(1.0f, 0.0f, 1.0f);
            const Frame tinted_paper = pause_menu_frame(app);
            art.paper = original_paper;
            const RectangleI paper_delta = foreground_delta_bounds(normal, tinted_paper);
            require(paper_delta.width > 0 && paper_delta.height > 0 && pixels.contains(paper_delta),
                "Actual pause paper was absent or escaped its panel");
            const size_t paper_pixels = foreground_local_delta(normal, tinted_paper,
                RectangleF(Vector2F(pixels.position()), Vector2F(pixels.size())));
            require(paper_pixels > 100, "Pause paper had no substantial visible fill");
            size_t focus_pixels = 0;
            if (phase != 3)
            {
                focus_pixels = assert_focus(normal, phase == 0 ? row_bounds(L"Resume", 0) : row_bounds(L"No", 1));
                require(focus_pixels > 30, "Pause initial/safe answer focus had no visible card fill");
            }
            else
            {
                const Colour original_focus = art.focus_fill;
                art.focus_fill = Colour(0.0f, 1.0f, 1.0f);
                const Frame disabled = pause_menu_frame(app);
                art.focus_fill = original_focus;
                require(normal.pixels == disabled.pixels,
                    "Missing-controller pause still painted a focus card on disabled Resume");
            }
            const std::string stem = std::string("pause-menu-") + name + '-' +
                std::to_string(size.x) + 'x' + std::to_string(size.y);
            save(normal, output / (stem + ".ppm"));
            save(foreground_crop(normal, pixels), output / (stem + "-detail.ppm"));
            records << name << ',' << size.x << ',' << size.y << ',' << paper_pixels << ',' << focus_pixels << '\n';
            resize_page({ 1920, 1080 });
            std::ignore = pause_menu_frame(app);
            resize_page(size);
            require(normal.pixels == pause_menu_frame(app).pixels && !choice.has_value(),
                "Resizing the same pause page changed pixels, focus or action");
            tap(Key::enter);
            if (phase == 0)
            {
                require(choice == PauseMenuAction::resume && stack.depth() == 1,
                    "Initial Enter on actual pause did not Resume");
            }
            else if (phase == 1 || phase == 2)
            {
                require(!choice.has_value() && stack.depth() == 2,
                    "Initial Enter on a confirmation bypassed its safe No default");
                tap(Key::escape);
                require(choice == PauseMenuAction::resume && stack.depth() == 1,
                    "No did not return to the initial pause page");
            }
            else
            {
                require(!choice.has_value() && stack.depth() == 2,
                    "Missing-pad Enter resumed or restarted a blocked match");
            }
        }
    }
    resize_page({ 1280, 720 });
    for (const bool quit : { false, true })
    {
        open(); confirmation(quit);
        tap(Key::up);
        const Frame yes = pause_menu_frame(app);
        require(assert_focus(yes, row_bounds(L"Yes", 0)) > 30,
            "Confirmation Up did not visibly select Yes");
        save(yes, output / (quit ? "pause-menu-quit-yes-720p.ppm" : "pause-menu-restart-yes-720p.ppm"));
        tap(Key::enter);
        require(choice == (quit ? PauseMenuAction::quit : PauseMenuAction::restart) && stack.depth() == 1,
            "Deliberate confirmation Yes returned the wrong typed pause action");
        open(); confirmation(quit); tap(Key::escape);
        require(!choice.has_value() && stack.depth() == 2,
            "Escape from a pause confirmation did not return to its initial page");
        tap(Key::enter);
        require(choice == PauseMenuAction::resume,
            "Returning from a confirmation lost initial Resume focus");
    }
    open();
    tap(Key::down);
    const Frame restart = pause_menu_frame(app);
    require(assert_focus(restart, row_bounds(L"Restart", 1)) > 30,
        "Pause Down did not visibly focus Restart");
    tap(Key::down); tap(Key::enter); tick();
    require(stack.depth() == 3 && !choice.has_value(),
        "Pause Options did not push a covering settings page");
    for (int row = preferences.fullscreen() ? 1 : 0; row < 3; ++row) { tap(Key::down); }
    const Frame options = pause_menu_frame(app);
    tap(Key::enter); tick();
    require(stack.depth() == 4 && !choice.has_value(),
        "Paused Options did not push actual Controls/audio");
    tap(Key::escape); tick();
    require(stack.depth() == 3 && !choice.has_value() && options.pixels == pause_menu_frame(app).pixels,
        "Closing Controls/audio did not preserve retained Options focus");
    tap(Key::escape); tick();
    require(stack.depth() == 2 && !choice.has_value() &&
        assert_focus(pause_menu_frame(app), row_bounds(L"Options", 2)) > 30,
        "Closing Options did not preserve the pause cursor");
    tap(Key::escape);
    require(choice == PauseMenuAction::resume, "Escape from actual initial pause did not Resume");

    // No hardware controller is injected: these rosters intentionally own
    // absent slots. Keyboard is first a participant, then an outside rescue
    // device when all four participants have disappeared.
    for (const bool keyboard_participates : { true, false })
    {
        open(keyboard_participates ? 1 : 4, keyboard_participates);
        const Frame disconnected = pause_menu_frame(app);
        std::wstring missing;
        const int first_missing = keyboard_participates ? 1 : 0;
        const auto players = settings.player_settings();
        constexpr std::array<const wchar_t*, 5> devices{
            L"Pad 1", L"Pad 2", L"Pad 3", L"Pad 4", L"Keyboard/mouse" };
        for (int player = first_missing; player < settings.player_count(); ++player)
        {
            const int index = player - first_missing;
            if (index > 0) { missing += index % 2 == 0 ? L"\n" : L"   "; }
            const int slot = players.at(static_cast<size_t>(player)).num;
            const std::wstring device = slot < 0 ? L"Waiting to join" : devices.at(static_cast<size_t>(slot));
            missing += L"P" + std::to_wstring(player + 1) + L" - " + device;
        }
        const Vector2F missing_size = app.render_resources()->measure_text(detail_font, missing);
        const RectangleF missing_bounds((menu_consts::DESIGN_RESOLUTION.x - missing_size.x) / 2.0f,
            look.status_y, missing_size.x, missing_size.y);
        require(panel.contains(missing_bounds) && missing_bounds.bottom() <
            look.layout.position.y - art.focus_padding.y,
            "Missing-controller roster text escapes its paper or overlaps its actions");
        tap(Key::escape);
        require(!choice.has_value() && stack.depth() == 2,
            "Escape bypassed the missing-controller pause");
        tap(Key::down);
        const Frame options_focus = pause_menu_frame(app);
        require(assert_focus(options_focus, row_bounds(L"Options", 2)) > 30,
            "Missing-controller navigation did not skip disabled Resume/Restart");
        tap(Key::enter); tick();
        require(stack.depth() == 3 && !choice.has_value(),
            "Missing-controller pause could not reach Options");
        for (int row = preferences.fullscreen() ? 1 : 0; row < 3; ++row) { tap(Key::down); }
        const Frame missing_options = pause_menu_frame(app);
        tap(Key::enter); tick();
        require(stack.depth() == 4 && !choice.has_value(),
            "Missing-controller Options could not reach Controls/audio");
        tap(Key::escape); tick();
        require(stack.depth() == 3 && !choice.has_value() && missing_options.pixels == pause_menu_frame(app).pixels,
            "Missing-controller Controls return changed retained Options focus");
        tap(Key::escape); tick();
        require(stack.depth() == 2 && !choice.has_value() && options_focus.pixels == pause_menu_frame(app).pixels,
            "Missing-controller Options return changed retained Pause focus");
        tap(Key::down);
        const Frame quit = pause_menu_frame(app);
        require(assert_focus(quit, row_bounds(L"Quit", 3)) > 30,
            "Missing-controller pause could not visibly focus Quit");
        tap(Key::enter); tick();
        tap(Key::up); tap(Key::enter);
        require(choice == PauseMenuAction::quit && stack.depth() == 1,
            "Missing participants left no keyboard route through Quit confirmation");
        save(disconnected, output / (keyboard_participates ?
            "pause-menu-keyboard-missing-pad-720p.ppm" : "pause-menu-four-missing-pads-720p.ppm"));
    }
    require(records.good(), "Pause-menu evidence write failed");
    pause_menu_item_checks(app, canvas, tuning, presentation, output);
    pause_question_checks(app, canvas, tuning, presentation, output);
    std::cout << "PASS actual pause/confirmations: initial Resume, safe No, deliberate Restart/Quit, "
        "Controls return, missing-device recovery, enabled focus cards, frozen/reduced redraw, "
        "resize and 720p/1080p/QHD/4:3/16:10/ultrawide paper bounds\n";
}
