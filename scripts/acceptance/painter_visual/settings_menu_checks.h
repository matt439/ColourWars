// Production Options/Credits pages: real navigation and save writes use a
// second isolated profile so the runner's immutable preference fixture survives.
void settings_menu_matrix(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    struct SettingsProfile
    {
        std::wstring previous;
        explicit SettingsProfile(const std::filesystem::path& root)
        {
            wchar_t* raw = nullptr;
            size_t length = 0;
            require(_wdupenv_s(&raw, &length, L"LOCALAPPDATA") == 0,
                "Could not read the runner's isolated profile");
            if (raw != nullptr) { this->previous = raw; std::free(raw); }
            std::filesystem::create_directories(root);
            require(_wputenv_s(L"LOCALAPPDATA", root.c_str()) == 0,
                "Could not isolate the interactive settings save");
        }
        ~SettingsProfile() { _wputenv_s(L"LOCALAPPDATA", this->previous.c_str()); }
    } isolated(output / "settings-profile");
    const std::filesystem::path save_path = output / "settings-profile" / "ColourWars" / "save_data.json";
    const auto saved_bytes = [&]
    {
        std::ifstream stream(save_path, std::ios::binary);
        require(stream.good(), "Interactive settings did not create its isolated save");
        return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    };
    Save preferences;
    auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0;
    require(preferences.set_display_settings_and_save(ScreenResolution::s_1920_1080,
        false, TeamColourPair::blue_yellow), "Could not seed isolated display settings");
    GameData data;
    data.set_application(&app);
    data.set_save(&preferences);
    data.set_tuning(&tuning);
    data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings);
    StateContext& stack = app;
    struct ClearSettingsStates
    {
        StateContext& stack;
        ~ClearSettingsStates() { this->stack.clear(); }
    } clear{ stack };
    const MainMenuPresentation& look = presentation.main_menu();
    const MenuStyle& style = presentation.menu();
    auto& art = const_cast<MenuPaperArtPresentation&>(look.art);
    struct RestoreSettingsArt
    {
        MenuPaperArtPresentation& art;
        MenuPaperArtPresentation original;
        ~RestoreSettingsArt() { this->art = this->original; }
    } restore_art{ art, art };
    const Vector2F area(menu_consts::DESIGN_RESOLUTION.x, style.footer_y);
    const RectangleF options_panel((area - look.options_box_size) / 2.0f, look.options_box_size);
    const RectangleF credits_panel((area - look.credits_box_size) / 2.0f, look.credits_box_size);
    const auto measure = [&](const std::string& font, const std::wstring& text)
    {
        return app.render_resources()->measure_text(app.render_resources()->resolve_sprite_font(font), text);
    };
    const auto text_bounds = [&](const std::string& font, const std::wstring& text, const Vector2F& position)
    {
        return RectangleF(position, measure(font, text));
    };
    const auto card = [&](RectangleF bounds)
    {
        bounds.inflate(art.focus_padding + Vector2F(art.keyline_width + 2.0f));
        return bounds;
    };
    const std::array<const wchar_t*, 6> labels{
        L"Resolution", L"Fullscreen", L"Team palette", L"Controls / audio", L"Apply", L"Back" };
    const auto row_label = [&](size_t index)
    {
        return text_bounds(style.item_font, labels[index], look.options_position +
            Vector2F(0.0f, static_cast<float>(index) * look.options_spacing.y));
    };
    const auto value_bounds = [&](size_t index, const std::wstring& value)
    {
        return text_bounds(style.item_font, value, Vector2F(look.options_value_x,
            look.options_position.y + static_cast<float>(index) * look.options_spacing.y));
    };
    const auto row_bounds = [&](size_t index, const std::wstring& value = L"")
    {
        RectangleF bounds = row_label(index);
        if (!value.empty())
        {
            const RectangleF second = value_bounds(index, value);
            bounds.width = std::max(bounds.right(), second.right()) - bounds.x;
            bounds.height = std::max(bounds.bottom(), second.bottom()) - bounds.y;
        }
        return bounds;
    };
    const RectangleF options_heading = text_bounds(style.heading_font, L"Options", look.options_heading_position);
    const RectangleF credits_heading = text_bounds(style.heading_font, L"Credits", look.credits_heading_position);
    const RectangleF credits_body = text_bounds(look.credits_font, look.credits_text, look.credits_position);
    const RectangleF credits_back = text_bounds(style.item_font, L"Back", look.credits_back_position);
    require(options_panel.bottom() + art.shadow_offset < style.footer_y &&
        credits_panel.bottom() + art.shadow_offset < style.footer_y,
        "Options/Credits paper overlaps the shared controls footer");
    std::cout << "OPTIONS heading=" << options_heading.x << ',' << options_heading.y << ',' <<
        options_heading.width << ',' << options_heading.height << " first-card-y=" << card(row_label(0)).y << '\n';
    for (const wchar_t* label : { L"Resolution", L"Fullscreen", L"Team palette", L"Controls / audio", L"Apply", L"Back", L"A: Blue / B: Yellow", L"3840x2160" })
    {
        const Vector2F size = measure(style.item_font, label);
        std::wcout << L"MEASURE " << label << L"=" << size.x << L',' << size.y << L'\n';
    }
    std::cout << "CREDITS body=" << credits_body.width << ',' << credits_body.height << " status=" <<
        measure(presentation.prompt_bar().font, L"Settings active for this run. Could not save for next time.").x << ',' <<
        measure(presentation.prompt_bar().font, L"Settings active for this run. Could not save for next time.").y << '\n';
    require(options_panel.contains(options_heading) && options_heading.bottom() < card(row_label(0)).y,
        "Measured Options heading escapes paper or overlaps the first action");
    for (size_t index = 0; index < labels.size(); ++index)
    {
        require(options_panel.contains(card(row_label(index))),
            "Measured Options action/card escapes its paper");
        require(look.options_spacing.y > row_label(index).height + art.focus_padding.y * 2.0f,
            "Options cards overlap at actual font height");
    }
    for (const std::wstring& value : { std::wstring(L"3840x2160"), std::wstring(L"Off"),
        presentation.team_palette().fixed_pair(TeamColourPair::blue_orange).label,
        presentation.team_palette().fixed_pair(TeamColourPair::pink_green).label,
        presentation.team_palette().fixed_pair(TeamColourPair::blue_yellow).label, std::wstring(L"Random") })
    {
        const size_t index = value.find(L'x') != std::wstring::npos ? 0 : value == L"Off" ? 1 : 2;
        require(row_label(index).right() + art.focus_padding.x < look.options_value_x &&
            options_panel.contains(card(row_bounds(index, value))),
            "Measured Options value overlaps its label or escapes its paper/card");
    }
    for (const wchar_t* status : { L"Apply to save changes.", L"Settings applied and saved.",
        L"Settings active for this run. Could not save for next time." })
    {
        const RectangleF bounds = text_bounds(presentation.prompt_bar().font, status, look.options_status_position);
        const RectangleF back = card(row_label(5));
        const bool separate = bounds.right() <= back.x || bounds.x >= back.right() ||
            bounds.bottom() <= back.y || bounds.y >= back.bottom();
        require(options_panel.contains(bounds) && separate,
            "Measured Options save status escapes its paper or overlaps Back");
    }
    require(credits_panel.contains(credits_heading) && credits_panel.contains(credits_body) &&
        credits_panel.contains(card(credits_back)) && credits_heading.bottom() < credits_body.y &&
        credits_body.bottom() < card(credits_back).y,
        "Measured Credits copy/Back overlap or escape their paper");

    app.keyboard()->set_focused(true);
    app.mouse()->set_focused(true);
    for (const Key key : { Key::up, Key::down, Key::left, Key::right, Key::enter, Key::space, Key::escape })
    {
        app.keyboard()->on_key_up(key);
    }
    app.keyboard()->poll(); app.keyboard()->poll();
    app.mouse()->poll(); app.mouse()->poll();
    const auto tick = [&](float elapsed = 0.0f)
    {
        app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed);
    };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick();
        app.keyboard()->on_key_up(key); tick();
    };
    const auto apply = [&]
    {
        // Win32 leave_fullscreen shows even an initially hidden window. Keep
        // that real Apply path off the desktop, then hide and rearm the probe.
        const HWND window = static_cast<HWND>(app.window());
        const SettingsProbeActivationGuard activation(window);
        require(SetWindowPos(window, nullptr, -32000, -32000, 0, 0,
            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE) != FALSE,
            "Could not keep the settings Apply probe off the desktop");
        tap(Key::enter);
        ShowWindow(window, SW_HIDE);
        app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        stack.notify_activation(true); tick();
        require(IsWindowVisible(window) == FALSE && !app.fullscreen() && GetForegroundWindow() != window,
            "Settings Apply left its probe visible, activated it or entered fullscreen");
    };
    const auto open = [&](std::unique_ptr<State> page)
    {
        stack.clear(); stack.notify_activation(true); input.reset_actions();
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        stack.push(std::move(page)); tick();
        require(stack.depth() == 2, "Actual settings page did not remain open after neutral input");
    };
    const auto resize_page = [&](Vector2I size)
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
    };
    const auto camera = [&]
    {
        return Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
    };
    const auto project = [&](const RectangleF& bounds)
    {
        return countdown_crop_bounds(camera().calculate_view_rectangle(bounds));
    };
    const auto contrast = [&](const Frame& frame, const RectangleF& bounds)
    {
        const RectangleI pixels = project(bounds);
        require(pixels.x >= 0 && pixels.y >= 0 && pixels.right() <= frame.width && pixels.bottom() <= frame.height,
            "Options/Credits text escaped its display");
        float darkest = 1.0f, lightest = 0.0f;
        size_t ink = 0, paper = 0;
        for (int y = pixels.y; y < pixels.bottom(); ++y)
        {
            for (int x = pixels.x; x < pixels.right(); ++x)
            {
                const float value = hud_luminance(frame, hud_pixel_offset(frame, x, y));
                darkest = std::min(darkest, value); lightest = std::max(lightest, value);
                if (value < 0.2f) { ++ink; }
                if (value > 0.5f) { ++paper; }
            }
        }
        require(ink > 2 && paper > 2 && (lightest + 0.05f) / (darkest + 0.05f) >= 4.5f,
            "Native Options/Credits text lost readable ink/paper contrast");
    };
    const auto linear_lettering = [&](const Frame& actual, const std::string& font,
        const std::wstring& text, const RectangleF& bounds,
        const std::optional<RectangleF>& focused = std::nullopt)
    {
        const UiText expected("expected-settings-text", text, font, bounds.position(),
            app.render_resources(), art.ink);
        const Canvas paper(app.render_resources(), art.paper);
        UiTexture focus_widget("expected-focus", "sprite_sheet_1", "pixel",
            focused.value_or(RectangleF::ZERO), app.render_resources());
        FocusGroup focus;
        if (focused.has_value()) { focus.add(&focus_widget); }
        const MenuPaperArtwork artwork(focused.has_value() ? options_panel : credits_panel,
            app.render_resources(), &style, &art, &focus);
        const auto fixture = [&](TextureFilter filter)
        {
            Renderer& renderer = *app.renderer();
            renderer.begin_frame(); renderer.set_view_count(1);
            DrawList list = renderer.view(0);
            const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
            list.set_viewport(viewport); list.set_camera(camera());
            list.set_filter(TextureFilter::point);
            paper.draw(list, camera().visible_rectangle(viewport));
            artwork.draw(list);
            list.set_filter(filter); expected.draw(list);
            return read(renderer);
        };
        const RectangleI pixels = project(bounds);
        const Frame observed = foreground_crop(actual, pixels, 0);
        const Frame linear = foreground_crop(fixture(TextureFilter::linear), pixels, 0);
        if (observed.pixels != linear.pixels)
        {
            std::wcout << L"LETTERING mismatch " << text << L" at " << actual.width << L'x' << actual.height << L'\n';
            save(observed, output / "settings-lettering-actual.ppm");
            save(linear, output / "settings-lettering-expected.ppm");
        }
        require(observed.pixels == linear.pixels,
            "Actual settings lettering lost complete authored text, natural size or linear sampling");
        if (actual.width == 1280 && actual.height == 720)
        {
            require(foreground_crop(fixture(TextureFilter::point), pixels, 0).pixels != linear.pixels,
                "720p settings lettering negative control failed to distinguish point sampling");
        }
    };
    const auto assert_focus = [&](const Frame& normal, const RectangleF& bounds)
    {
        const Colour original = art.focus_fill;
        art.focus_fill = Colour(0.0f, 1.0f, 1.0f);
        const Frame tinted = main_menu_frame(app);
        art.focus_fill = original;
        const RectangleI delta = foreground_delta_bounds(normal, tinted);
        const RectangleI expected = project(card(bounds));
        require(delta.width > 0 && delta.height > 0 && expected.contains(delta),
            "Actual Options/Credits focus did not follow its enabled row");
        const size_t fill_pixels = foreground_local_delta(normal, tinted,
            RectangleF(Vector2F(expected.position()), Vector2F(expected.size())));
        require(fill_pixels > 30, "Actual settings focus has no substantial paper fill");
        RectangleF padded = bounds; padded.inflate(art.focus_padding);
        const float pen = art.keyline_width;
        const RectangleI arrow = project(RectangleF(padded.x - pen * 8.0f,
            padded.center().y - pen * 4.0f, pen * 7.0f, pen * 8.0f));
        const Colour ink = art.ink;
        art.ink = Colour(1.0f, 0.0f, 1.0f);
        const Frame tinted_ink = main_menu_frame(app);
        art.ink = ink;
        const Frame arrow_before = foreground_crop(normal, arrow, 0);
        const Frame arrow_after = foreground_crop(tinted_ink, arrow, 0);
        size_t changed_arrow = 0;
        for (size_t offset = 0; offset < arrow_before.pixels.size(); offset += 4)
        {
            if (!same_pixel(arrow_before, arrow_after, offset)) { ++changed_arrow; }
        }
        require(changed_arrow > 5,
            "Actual settings focus lost its detached ink arrow");
        return fill_pixels;
    };
    const auto stable = [&](const Frame& normal)
    {
        std::srand(8191); const int expected = std::rand(); std::srand(8191);
        require(normal.pixels == main_menu_frame(app).pixels && normal.pixels == main_menu_frame(app).pixels &&
            std::rand() == expected, "Frozen Options/Credits draw changed pixels or consumed randomness");
        tick(0.75f);
        require(normal.pixels == main_menu_frame(app).pixels, "Neutral time moved static settings artwork");
        play.reduce_effects = true; tick(0.75f);
        require(normal.pixels == main_menu_frame(app).pixels, "Reduce effects changed static settings information");
        play.reduce_effects = false;
    };
    std::ofstream records(output / "settings-menu.csv");
    records << "page,focus,width,height,paper_pixels,focus_pixels\n";
    const auto capture = [&](const char* page, const char* focus, const RectangleF& panel,
        const RectangleF& focus_bounds)
    {
        const Frame normal = main_menu_frame(app);
        stable(normal);
        RectangleF expanded = panel; expanded.inflate(Vector2F(art.keyline_width + art.shadow_offset + 4.0f));
        const RectangleI pixels = project(expanded);
        require(pixels.x >= 0 && pixels.y >= 0 && pixels.right() <= normal.width && pixels.bottom() <= normal.height,
            "Options/Credits paper escaped a supported aspect ratio");
        const Colour paper = art.paper; art.paper = Colour(1.0f, 0.0f, 1.0f);
        const Frame tinted = main_menu_frame(app); art.paper = paper;
        const RectangleI delta = foreground_delta_bounds(normal, tinted);
        require(delta.width > 0 && delta.height > 0 && pixels.contains(delta),
            "Actual settings paper is absent or escaped its panel");
        const size_t paper_pixels = foreground_local_delta(normal, tinted,
            RectangleF(Vector2F(pixels.position()), Vector2F(pixels.size())));
        require(paper_pixels > 100, "Actual settings paper lost its visible fill");
        const size_t focus_pixels = assert_focus(normal, focus_bounds);
        contrast(normal, focus_bounds);
        const std::string stem = std::string("settings-menu-") + page + '-' + focus + '-' +
            std::to_string(normal.width) + 'x' + std::to_string(normal.height);
        save(normal, output / (stem + ".ppm"));
        save(foreground_crop(normal, pixels), output / (stem + "-detail.ppm"));
        records << page << ',' << focus << ',' << normal.width << ',' << normal.height << ',' <<
            paper_pixels << ',' << focus_pixels << '\n';
        resize_page({ 1920, 1080 }); std::ignore = main_menu_frame(app);
        resize_page({ normal.width, normal.height });
        require(normal.pixels == main_menu_frame(app).pixels,
            "Options/Credits resize round trip changed pixels or focus");
        return normal;
    };

    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440),
        Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        resize_page(size);
        open(std::make_unique<MainMenuOptions>(&context));
        for (size_t row = 0; row < labels.size(); ++row)
        {
            const std::wstring value = row == 0 ? L"1920x1080" : row == 1 ? L"Off" : row == 2 ?
                presentation.team_palette().fixed_pair(TeamColourPair::blue_yellow).label : L"";
            const Frame frame = capture("options", std::array<const char*, 6>{
                "resolution", "fullscreen", "palette", "controls", "apply", "back" }[row],
                options_panel, row_bounds(row, value));
            contrast(frame, options_heading);
            linear_lettering(frame, style.heading_font, L"Options", options_heading);
            for (size_t label = 0; label < labels.size(); ++label) { contrast(frame, row_label(label)); }
            if (!value.empty())
            {
                contrast(frame, value_bounds(row, value));
                linear_lettering(frame, style.item_font, value, value_bounds(row, value), row_bounds(row, value));
            }
            if (row + 1 < labels.size()) { tap(Key::down); }
        }
        tap(Key::up);
        std::ignore = assert_focus(main_menu_frame(app), row_label(4));
        open(std::make_unique<MainMenuCredits>(&context));
        const Frame credits = capture("credits", "back", credits_panel, credits_back);
        contrast(credits, credits_heading); contrast(credits, credits_body);
        linear_lettering(credits, style.heading_font, L"Credits", credits_heading);
        linear_lettering(credits, look.credits_font, look.credits_text, credits_body);
    }

    // Every palette is selected and rendered through the real value row,
    // including returning to the longest label after shorter choices.
    resize_page({ 1280, 720 });
    open(std::make_unique<MainMenuOptions>(&context));
    tap(Key::enter);
    require(preferences.team_palette() == TeamColourPair::blue_yellow,
        "Confirming a value row unexpectedly applied settings");
    tap(Key::down); tap(Key::down);
    for (const TeamColourPair palette : { TeamColourPair::random, TeamColourPair::blue_orange,
        TeamColourPair::pink_green, TeamColourPair::blue_yellow })
    {
        tap(Key::right);
        const std::wstring value = palette == TeamColourPair::random ? L"Random" :
            presentation.team_palette().fixed_pair(palette).label;
        const Frame frame = capture("palette", TeamColourTools::palette_key(palette),
            options_panel, row_bounds(2, value));
        linear_lettering(frame, style.item_font, value, value_bounds(2, value), row_bounds(2, value));
        require(preferences.team_palette() == TeamColourPair::blue_yellow,
            "Cycling a palette saved before Apply");
    }

    // Stage changes, visit Controls/audio, discard there and then discard the
    // underlying display edits with Back. No save write is allowed on either route.
    resize_page({ 1280, 720 });
    const std::string original = saved_bytes();
    open(std::make_unique<MainMenuOptions>(&context));
    tap(Key::left);
    const Frame pending = main_menu_frame(app);
    require(preferences.resolution() == ScreenResolution::s_1920_1080 && saved_bytes() == original,
        "Adjusting an Options row applied or saved before Apply");
    tap(Key::down); tap(Key::down); tap(Key::right); tap(Key::down);
    const Frame before_controls = main_menu_frame(app);
    tap(Key::enter); tick();
    require(stack.depth() == 3 && main_menu_frame(app).pixels != before_controls.pixels,
        "Controls/audio did not push above actual Options");
    tap(Key::right); tap(Key::escape); tick();
    require(stack.depth() == 2 && before_controls.pixels == main_menu_frame(app).pixels &&
        saved_bytes() == original && play.aim_sensitivity == PlaySettings{}.aim_sensitivity,
        "Controls/audio Back lost pending Options focus/values or saved its own changes");
    tap(Key::up); tap(Key::up); tap(Key::up);
    require(pending.pixels != main_menu_frame(app).pixels,
        "Palette adjustment had no visible settings effect");
    tap(Key::escape);
    require(saved_bytes() == original && preferences.resolution() == ScreenResolution::s_1920_1080 &&
        preferences.team_palette() == TeamColourPair::blue_yellow,
        "Options Back persisted unapplied display changes");

    // Apply through the real page, then load a fresh Save to check persistence.
    open(std::make_unique<MainMenuOptions>(&context));
    tap(Key::left); tap(Key::down); tap(Key::down); tap(Key::right);
    tap(Key::down); tap(Key::down); apply();
    resize_page({ 1280, 720 });
    require(preferences.resolution() == ScreenResolution::s_1280_720 &&
        preferences.team_palette() == TeamColourPair::random && !preferences.fullscreen() && !app.fullscreen(),
        "Actual Options Apply lost its selected display preferences");
    Save restored; restored.load_save_file();
    require(restored.resolution() == preferences.resolution() && restored.team_palette() == preferences.team_palette() &&
        restored.fullscreen() == preferences.fullscreen() && restored.play_settings().music_volume == 0,
        "Actual Options Apply failed to preserve the isolated settings after reload");
    const Frame applied = capture("options", "applied", options_panel, row_label(4));
    contrast(applied, text_bounds(presentation.prompt_bar().font,
        L"Settings applied and saved.", look.options_status_position));

    // A directory at the exact temporary-file location refuses this write while
    // preserving the previous JSON, allowing the production warning to render.
    const std::string last_saved = saved_bytes();
    const std::filesystem::path blocked = save_path.wstring() + L".tmp";
    require(std::filesystem::create_directory(blocked), "Cannot block the isolated save's temporary file");
    tap(Key::up); tap(Key::up); tap(Key::right); tap(Key::down); tap(Key::down); apply();
    require(preferences.team_palette() == TeamColourPair::blue_orange && saved_bytes() == last_saved,
        "Failed settings save lost its active choice or replaced the previous JSON");
    const Frame failed = capture("options", "save-failed", options_panel, row_label(4));
    contrast(failed, text_bounds(presentation.prompt_bar().font,
        L"Settings active for this run. Could not save for next time.", look.options_status_position));
    require(std::filesystem::remove(blocked), "Could not remove the isolated write refusal fixture");

    // Saved fullscreen metadata is enough to exercise the disabled row. Only
    // Off is applied using the isolated offscreen, windowed fixture above.
    require(preferences.set_display_settings_and_save(ScreenResolution::s_1920_1080,
        true, TeamColourPair::blue_yellow), "Could not seed the fullscreen Options fixture");
    resize_page({ 1280, 720 });
    open(std::make_unique<MainMenuOptions>(&context));
    const Frame fullscreen = capture("options", "disabled-resolution", options_panel, row_bounds(1, L"On"));
    contrast(fullscreen, row_label(0)); contrast(fullscreen, value_bounds(0, L"1280x720"));
    tap(Key::up);
    std::ignore = assert_focus(main_menu_frame(app), row_label(5));
    tap(Key::down);
    std::ignore = assert_focus(main_menu_frame(app), row_bounds(1, L"On"));
    tap(Key::right);
    std::ignore = assert_focus(main_menu_frame(app), row_bounds(1, L"Off"));
    tap(Key::down); tap(Key::down); tap(Key::down); apply();
    require(!preferences.fullscreen() && !app.fullscreen() &&
        preferences.resolution() == ScreenResolution::s_1920_1080,
        "Leaving fullscreen lost the retained windowed resolution preset");
    resize_page({ 1280, 720 });
    tap(Key::up); tap(Key::up); tap(Key::up); tap(Key::up);
    std::ignore = assert_focus(main_menu_frame(app), row_bounds(0, L"1920x1080"));
    tap(Key::escape);
    const Frame home = main_menu_frame(app);
    open(std::make_unique<MainMenuCredits>(&context)); tap(Key::enter);
    require(home.pixels == main_menu_frame(app).pixels, "Credits Select did not return to Home");
    open(std::make_unique<MainMenuCredits>(&context)); tap(Key::escape);
    require(home.pixels == main_menu_frame(app).pixels, "Credits Back did not return to Home");
    require(records.good(), "Settings menu capture record write failed");
    std::cout << "PASS Options/Credits: measured labels, all palettes, long save status and authored credits; "
        "six native sizes/aspects, paper/cards/arrows and contrast; actual row edits, delayed Apply, "
        "saved reload/failure, Controls/audio return and Back discard, disabled fullscreen resolution "
        "and windowed preset retention; frozen/reduced/random/resize and Credits Select/Back\n";
    save_status_checks(app, canvas, tuning, presentation, output, false);
}
