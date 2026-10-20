// Production Controls/audio, including both real entry routes. A separate
// profile owns every write; the runner's immutable save is never borrowed.
void controls_menu_matrix(Application& app, Registry<LevelLoadedInfo>& levels,
    const Canvas& canvas, GameTuning& tuning, Presentation& presentation,
    const std::filesystem::path& content, const std::filesystem::path& output)
{
    struct ControlsProfile
    {
        std::wstring previous;
        explicit ControlsProfile(const std::filesystem::path& root)
        {
            wchar_t* raw = nullptr;
            size_t length = 0;
            require(_wdupenv_s(&raw, &length, L"LOCALAPPDATA") == 0,
                "Cannot read the runner's isolated controls profile");
            if (raw != nullptr) { this->previous = raw; std::free(raw); }
            std::filesystem::create_directories(root);
            require(_wputenv_s(L"LOCALAPPDATA", root.c_str()) == 0,
                "Cannot isolate Controls/audio save writes");
        }
        ~ControlsProfile() { _wputenv_s(L"LOCALAPPDATA", this->previous.c_str()); }
    } isolated(output / "controls-profile");
    const std::filesystem::path save_path = output / "controls-profile" / "ColourWars" / "save_data.json";
    const auto saved_bytes = [&]
    {
        std::ifstream stream(save_path, std::ios::binary);
        require(stream.good(), "Controls/audio did not create its isolated save");
        return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    };
    Save preferences;
    auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0;
    require(preferences.set_display_settings_and_save(ScreenResolution::s_1920_1080,
        false, TeamColourPair::blue_yellow), "Cannot seed the isolated Controls/audio save");
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false } };
    GameData data;
    data.set_application(&app); data.set_save(&preferences);
    data.set_tuning(&tuning); data.set_presentation(&presentation);
    data.set_level_infos(&levels); data.set_stages(&stages);
    DebugDirector director(&tuning, &presentation);
    data.set_debug(&director);
#if COLOURWARS_DEV_TOOLS
    director.impl()->set_content_directories(content, content);
#else
    static_cast<void>(content);
#endif
    director.attach(&app, &data);
#if COLOURWARS_DEV_TOOLS
    director.run_launch_arguments({ L"+intro", L"off" });
#endif
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings);
    StateContext& stack = app;
    struct ClearControlsStates
    {
        StateContext& stack;
        ~ClearControlsStates() { this->stack.clear(); }
    } clear{ stack };
    const ControlsSettingsPresentation& look = presentation.controls_settings();
    const MenuStyle& style = presentation.menu();
    auto& art = const_cast<MenuPaperArtPresentation&>(look.art);
    // Preview audio is outside this raster/navigation evidence. Changing the
    // authored preview gain keeps numeric clamp navigation silent.
    auto& preview_volume = const_cast<float&>(look.volume_preview_sound.volume);
    struct RestoreControlsLook
    {
        MenuPaperArtPresentation& art;
        MenuPaperArtPresentation original;
        float& preview;
        float original_preview;
        ~RestoreControlsLook() { this->art = this->original; this->preview = this->original_preview; }
    } restore{ art, art, preview_volume, preview_volume };
    preview_volume = 0.0f;
    const RectangleF panel((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) -
        look.box_size) / 2.0f, look.box_size);
    const std::array<const wchar_t*, 9> labels{ L"Pad sticks", L"Keyboard Jump", L"Music volume", L"SFX volume",
        L"View shake", L"Aim marker", L"Reduce effects", L"Apply", L"Back" };
    const std::array<const char*, 9> names{ "pad-sticks", "keyboard-jump", "music", "sfx", "shake", "marker", "effects", "apply", "back" };
    const std::wstring help = L"CONTROLLER\nMove / drop: left stick\nAim: right stick    Shoot: RT\nJump: A / LT (hold for height)\nPause: Start    Debug: Back / View\nKEYBOARD + MOUSE\nMove: A / D    Drop: S / Down\nAim: mouse cursor    Shoot: left click\nJump: Space (hold for height)\nPause: Esc    Debug: F1\nMENUS\nNavigate: stick / D-pad / arrows\nSelect: A / Enter    Back: B / Esc\nAIM / READINESS\nDirection only; roller aim is fixed.\nReady: chevron / Cooldown: bars\nNo paint: cross\nPad sticks: movement / aim deadzones.\nREDUCE EFFECTS\nLess shake, flashes and idle detail.";
    const std::wstring pending_status = L"Apply to save changes for all players.";
    const auto measure = [&](const std::string& font, const std::wstring& text)
    {
        return app.render_resources()->measure_text(app.render_resources()->resolve_sprite_font(font), text);
    };
    const auto label_scale = [&](size_t row)
    {
        if (row != 1 && row != 7) { return 1.0f; }
        const Vector2F raw = measure(look.item_font, labels[row]);
        return std::min({ 1.0f,
            (look.controls_position.x - art.focus_padding.x * 2.0f - art.shadow_offset - look.layout.position.x) / raw.x,
            (look.layout.spacing.y - art.focus_padding.y * 2.0f) / raw.y });
    };
    const auto row_label = [&](size_t row)
    {
        return RectangleF(look.layout.position + Vector2F(0.0f,
            static_cast<float>(row) * look.layout.spacing.y), measure(look.item_font, labels[row]) * label_scale(row));
    };
    const auto value_bounds = [&](size_t row, const std::wstring& value)
    {
        return RectangleF(row_label(row).position() + Vector2F(look.layout.spacing.x, 0.0f),
            measure(look.item_font, value));
    };
    const auto row_bounds = [&](size_t row, const std::wstring& value = L"")
    {
        RectangleF result = row_label(row);
        if (!value.empty())
        {
            const RectangleF second = value_bounds(row, value);
            result.width = std::max(result.right(), second.right()) - result.x;
            result.height = std::max(result.bottom(), second.bottom()) - result.y;
        }
        return result;
    };
    const auto card = [&](RectangleF bounds)
    {
        bounds.inflate(art.focus_padding + Vector2F(art.keyline_width + 2.0f));
        return bounds;
    };
    const Vector2F raw_help = measure(look.help_font, help);
    const float help_scale = std::min({ 1.0f, look.controls_size.x / raw_help.x, look.controls_size.y / raw_help.y });
    const RectangleF help_bounds(look.controls_position, raw_help * help_scale);
    const RectangleF heading(look.heading_position, measure(style.heading_font, L"Controls / audio"));
    require(panel.bottom() + art.shadow_offset < style.footer_y && panel.contains(heading) &&
        heading.bottom() < card(row_label(0)).y && panel.contains(help_bounds) &&
        RectangleF(look.controls_position, look.controls_size).contains(help_bounds),
        "Controls/audio heading/help/paper overlap or escape the footer-safe panel");
    for (size_t row = 0; row < labels.size(); ++row)
    {
        const float padded_height = row_label(row).height + art.focus_padding.y * 2.0f;
        require(panel.contains(card(row_label(row))) &&
            ((row == 1 || row == 7) ? padded_height <= look.layout.spacing.y + 0.01f :
                padded_height < look.layout.spacing.y),
            "Controls/audio action card escapes paper or overlaps another row");
    }
    for (const size_t row : { size_t(1), size_t(7) })
    {
        require(label_scale(row) < 1.0f && measure(look.item_font, labels[row]).y +
            art.focus_padding.y * 2.0f > look.layout.spacing.y,
            "Controls/audio fitted Keyboard Jump/Apply fixture did not distinguish the overlapping raw label");
    }
    for (const auto& entry : std::array<std::pair<size_t, const wchar_t*>, 5>{ {
        { 2, L"100%" }, { 3, L"0%" }, { 4, L"Off" }, { 5, L"On" }, { 6, L"Off" } } })
    {
        require(row_label(entry.first).right() + art.focus_padding.x < value_bounds(entry.first, entry.second).x &&
            panel.contains(card(row_bounds(entry.first, entry.second))) &&
            card(row_bounds(entry.first, entry.second)).right() < help_bounds.x,
            "Measured Controls/audio label/value/card overlaps adjacent copy");
    }
    const auto status_bounds = [&](const std::wstring& text)
    {
        const Vector2F raw = measure(look.help_font, text);
        const float available = panel.right() - art.shadow_offset - art.focus_padding.x - look.status_position.x;
        return RectangleF(look.status_position, raw * std::min(1.0f, available / raw.x));
    };
    for (const wchar_t* text : { pending_status.c_str(), L"Applied and saved for all players.",
        L"Active for this run. Could not save." })
    {
        const RectangleF bounds = status_bounds(text);
        require(panel.contains(bounds) && card(row_label(8)).bottom() < bounds.y &&
            bounds.right() < help_bounds.x, "Controls/audio status overlaps Back/help or escapes paper");
    }
    std::cout << "CONTROLS help=" << raw_help.x << ',' << raw_help.y << " scale=" << help_scale <<
        " fitted=" << help_bounds.width << ',' << help_bounds.height << '\n';

    app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
    for (const Key key : { Key::up, Key::down, Key::left, Key::right, Key::enter, Key::space, Key::escape })
    {
        app.keyboard()->on_key_up(key);
    }
    app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll();
    const auto tick = [&](float elapsed = 0.0f)
    {
        app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed);
    };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick();
        // This historical raster fixture samples the settled ornament pose.
        app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.51f);
    };
    const auto resize_page = [&](Vector2I size)
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
    };
    const auto open = [&](std::unique_ptr<State> page)
    {
        stack.clear(); stack.notify_activation(true); input.reset_actions();
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        stack.push(std::move(page)); tick();
        require(stack.depth() == 2, "Controls/audio probe did not remain open after neutral input");
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
            "Controls/audio copy escaped its native display");
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
            "Native Controls/audio copy lost readable ink/paper contrast");
    };
    using Values = std::array<std::wstring, 7>;
    const Values initial{ L"", L"", L"0%", L"0%", L"On", L"On", L"Off" };
    // One reference frame contains all authored copy, independent of the real
    // page's widget tree. Cropping each measured extent catches truncation,
    // missing lines, misplaced text, unwanted shadows and wrong filtering.
    const auto lettering = [&](const Frame& actual, size_t selected, const Values& values,
        const std::wstring& status, const Colour& status_colour, bool negative_control = false)
    {
        std::vector<std::unique_ptr<UiText>> text;
        const auto add = [&](const std::wstring& copy, const std::string& font,
            const Vector2F& position, const Colour& colour, float scale = 1.0f)
        {
            text.push_back(std::make_unique<UiText>("expected-controls-copy", copy, font,
                position, app.render_resources(), colour));
            text.back()->set_scale(scale);
        };
        add(L"Controls / audio", style.heading_font, heading.position(), art.ink);
        add(help, look.help_font, help_bounds.position(), art.ink, help_scale);
        for (size_t row = 0; row < labels.size(); ++row)
        {
            add(labels[row], look.item_font, row_label(row).position(), row == selected ? art.focus_ink : art.ink,
                label_scale(row));
            if (row < values.size() && !values[row].empty())
            {
                add(values[row], look.item_font, value_bounds(row, values[row]).position(), art.ink);
            }
        }
        add(status, look.help_font, look.status_position, status_colour,
            status_bounds(status).width / measure(look.help_font, status).x);
        UiTexture focus_widget("expected-controls-focus", "sprite_sheet_1", "pixel",
            row_bounds(selected, selected < values.size() ? values[selected] : L""), app.render_resources());
        FocusGroup focus; focus.add(&focus_widget);
        const MenuPaperArtwork artwork(panel, app.render_resources(), &style, &art, &focus);
        const Canvas paper(app.render_resources(), art.paper);
        const auto fixture = [&](TextureFilter filter)
        {
            Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
            DrawList list = renderer.view(0);
            const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
            list.set_viewport(viewport); list.set_camera(camera()); list.set_filter(TextureFilter::point);
            paper.draw(list, camera().visible_rectangle(viewport)); artwork.draw(list);
            list.set_filter(filter);
            for (const auto& widget : text) { widget->draw(list); }
            return read(renderer);
        };
        const Frame linear = fixture(TextureFilter::linear);
        for (const auto& widget : text)
        {
            const RectangleI pixels = project(widget->bounds());
            if (foreground_crop(actual, pixels, 0).pixels != foreground_crop(linear, pixels, 0).pixels)
            {
                save(actual, output / "controls-lettering-actual.ppm");
                save(linear, output / "controls-lettering-expected.ppm");
                throw std::runtime_error("Actual Controls/audio lettering lost authored copy, fitting or linear sampling");
            }
            contrast(actual, widget->bounds());
        }
        if (negative_control)
        {
            require(foreground_crop(fixture(TextureFilter::point), project(help_bounds), 0).pixels !=
                foreground_crop(linear, project(help_bounds), 0).pixels,
                "720p controls help negative control did not distinguish point sampling");
        }
    };
    const auto assert_focus = [&](const Frame& normal, const RectangleF& bounds)
    {
        const Colour fill = art.focus_fill; art.focus_fill = Colour(0.0f, 1.0f, 1.0f);
        const Frame tinted = main_menu_frame(app); art.focus_fill = fill;
        const RectangleI delta = foreground_delta_bounds(normal, tinted);
        const RectangleI expected = project(card(bounds));
        require(delta.width > 0 && delta.height > 0 && expected.contains(delta),
            "Controls/audio focus card lost its complete label/value row");
        const size_t changed = foreground_local_delta(normal, tinted,
            RectangleF(Vector2F(expected.position()), Vector2F(expected.size())));
        require(changed > 30, "Controls/audio focus card has no substantial fill");
        RectangleF padded = bounds; padded.inflate(art.focus_padding);
        const float pen = art.keyline_width;
        const RectangleI arrow = project(RectangleF(padded.x - pen * 8.0f,
            padded.center().y - pen * 4.0f, pen * 7.0f, pen * 8.0f));
        const Colour ink = art.ink; art.ink = Colour(1.0f, 0.0f, 1.0f);
        const Frame ink_tinted = main_menu_frame(app); art.ink = ink;
        require(foreground_crop(normal, arrow, 0).pixels != foreground_crop(ink_tinted, arrow, 0).pixels,
            "Controls/audio focus lost its detached ink arrow");
        return changed;
    };
    std::ofstream records(output / "controls-menu.csv");
    records << "focus,width,height,paper_pixels,focus_pixels,help_scale\n";
    const auto capture = [&](const char* name, size_t selected, const Values& values,
        const std::wstring& status, const Colour& status_colour, bool negative_control = false)
    {
        const Frame normal = main_menu_frame(app);
        std::srand(8191); const int next_random = std::rand(); std::srand(8191);
        require(normal.pixels == main_menu_frame(app).pixels && normal.pixels == main_menu_frame(app).pixels &&
            std::rand() == next_random, "Frozen Controls/audio draw changed pixels or consumed randomness");
        tick(0.75f);
        require(normal.pixels == main_menu_frame(app).pixels, "Neutral time moved Controls/audio artwork");
        const bool reduce = play.reduce_effects; play.reduce_effects = !reduce; tick(0.75f);
        require(normal.pixels == main_menu_frame(app).pixels, "Reduce effects changed static Controls/audio information");
        play.reduce_effects = reduce;
        RectangleF expanded = panel; expanded.inflate(Vector2F(art.keyline_width + art.shadow_offset + 4.0f));
        const RectangleI pixels = project(expanded);
        require(pixels.x >= 0 && pixels.y >= 0 && pixels.right() <= normal.width && pixels.bottom() <= normal.height,
            "Controls/audio paper escaped a supported display aspect");
        const Colour paper = art.paper; art.paper = Colour(1.0f, 0.0f, 1.0f);
        const Frame tinted = main_menu_frame(app); art.paper = paper;
        const RectangleI delta = foreground_delta_bounds(normal, tinted);
        require(delta.width > 0 && delta.height > 0 && pixels.contains(delta),
            "Controls/audio paper is absent or escaped its authored panel");
        const size_t paper_pixels = foreground_local_delta(normal, tinted,
            RectangleF(Vector2F(pixels.position()), Vector2F(pixels.size())));
        require(paper_pixels > 100, "Controls/audio paper has no substantial fill");
        const size_t focused = assert_focus(normal, row_bounds(selected,
            selected < values.size() ? values[selected] : L""));
        lettering(normal, selected, values, status, status_colour, negative_control);
        const std::string stem = std::string("controls-menu-") + name + '-' +
            std::to_string(normal.width) + 'x' + std::to_string(normal.height);
        save(normal, output / (stem + ".ppm"));
        save(foreground_crop(normal, pixels), output / (stem + "-detail.ppm"));
        records << name << ',' << normal.width << ',' << normal.height << ',' << paper_pixels << ',' <<
            focused << ',' << help_scale << '\n';
        resize_page({ 1920, 1080 }); std::ignore = main_menu_frame(app);
        resize_page({ normal.width, normal.height });
        require(normal.pixels == main_menu_frame(app).pixels, "Controls/audio resize changed pixels or focus");
        return normal;
    };
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440),
        Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        resize_page(size); open(std::make_unique<ControlsSettingsMenu>(&context));
        for (size_t row = 0; row < labels.size(); ++row)
        {
            std::ignore = capture(names[row], row, initial, pending_status, art.focus_ink,
                size == Vector2I(1280, 720) && row == 0);
            if (row + 1 < labels.size()) { tap(Key::down); }
        }
        tap(Key::up); std::ignore = assert_focus(main_menu_frame(app), row_label(7));
    }

    // Real volume edits at both clamps; the first action opens Pad sticks.
    // Values remain pending until Apply; SFX previews are silenced above.
    resize_page({ 1280, 720 }); open(std::make_unique<ControlsSettingsMenu>(&context));
    const std::string original = saved_bytes();
    Values values = initial;
    tap(Key::down); // Skip the independent Keyboard Jump child.
    for (size_t row = 2; row <= 3; ++row)
    {
        tap(Key::down);
        for (int step = 0; step < 12; ++step) { tap(Key::right); }
        values[row] = L"100%";
        std::ignore = capture(row == 2 ? "music-max" : "sfx-max", row, values, pending_status, art.focus_ink);
        for (int step = 0; step < 12; ++step) { tap(Key::left); }
        values[row] = L"0%";
        lettering(main_menu_frame(app), row, values, pending_status, art.focus_ink);
    }
    tap(Key::down); tap(Key::left); values[4] = L"Off";
    tap(Key::down); tap(Key::left); values[5] = L"Off";
    tap(Key::down); tap(Key::right); values[6] = L"On";
    std::ignore = capture("pending", 6, values, pending_status, art.focus_ink);
    tap(Key::enter);
    require(saved_bytes() == original && play.aim_sensitivity == 3 && play.view_shake && play.aim_marker &&
        !play.reduce_effects && play.music_volume == 0 && play.sfx_volume == 0,
        "Controls/audio changed active settings or saved before Apply");
    tap(Key::down); tap(Key::enter);
    require(play.aim_sensitivity == 3 && !play.view_shake && !play.aim_marker && play.reduce_effects &&
        play.music_volume == 0 && play.sfx_volume == 0, "Actual Controls/audio Apply lost pending settings");
    Save reloaded; reloaded.load_save_file();
    require(reloaded.play_settings().aim_sensitivity == 3 && !reloaded.play_settings().view_shake &&
        !reloaded.play_settings().aim_marker && reloaded.play_settings().reduce_effects &&
        reloaded.play_settings().music_volume == 0 && reloaded.play_settings().sfx_volume == 0 &&
        reloaded.resolution() == ScreenResolution::s_1920_1080 &&
        reloaded.team_palette() == TeamColourPair::blue_yellow,
        "Controls/audio Apply failed reload or replaced display preferences");
    std::ignore = capture("applied", 7, values, L"Applied and saved for all players.", art.focus_ink);
    const std::string last_saved = saved_bytes();
    const std::filesystem::path blocked = save_path.wstring() + L".tmp";
    require(std::filesystem::create_directory(blocked), "Cannot block the isolated Controls/audio temporary save");
    tap(Key::up); tap(Key::left); values[6] = L"Off"; tap(Key::down); tap(Key::enter);
    require(!play.reduce_effects && saved_bytes() == last_saved,
        "Failed Controls/audio save lost its active choice or replaced previous JSON");
    std::ignore = capture("save-failed", 7, values, L"Active for this run. Could not save.",
        presentation.main_menu().options_warning_colour);
    require(std::filesystem::remove(blocked), "Cannot remove isolated Controls/audio write refusal");
    tap(Key::down); tap(Key::enter);
    require(stack.depth() == 1, "Controls/audio Back action did not pop its own page");
    open(std::make_unique<ControlsSettingsMenu>(&context)); tap(Key::down); tap(Key::down); tap(Key::right);
    for (int row = 2; row < 8; ++row) { tap(Key::down); }
    tap(Key::enter);
    require(stack.depth() == 1 && play.aim_sensitivity == 3 && saved_bytes() == last_saved,
        "Controls/audio Back action persisted an unapplied change");
    open(std::make_unique<ControlsSettingsMenu>(&context)); tap(Key::down); tap(Key::down); tap(Key::right); tap(Key::escape);
    require(stack.depth() == 1 && play.aim_sensitivity == 3 && saved_bytes() == last_saved,
        "Controls/audio Escape persisted an unapplied change");

    // The underlying Options instance keeps pending display values and cursor.
    open(std::make_unique<MainMenuOptions>(&context)); tap(Key::left);
    tap(Key::down); tap(Key::down); tap(Key::right); tap(Key::down);
    const Frame pending_options = main_menu_frame(app);
    tap(Key::enter); tick(); require(stack.depth() == 3, "Options did not push actual Controls/audio");
    const Frame controls_from_options = main_menu_frame(app);
    tap(Key::down); tap(Key::down); tap(Key::right);
    require(controls_from_options.pixels != main_menu_frame(app).pixels,
        "Controls/audio Options-return fixture did not stage its discardable edit");
    tap(Key::escape); tick(0.5f);
    require(stack.depth() == 2 && pending_options.pixels == main_menu_frame(app).pixels &&
        saved_bytes() == last_saved && play.aim_sensitivity == 3,
        "Controls/audio return lost pending Options values/focus or saved its own edits");

    // Actual GameLevel owns the Pause frame: returning through Controls/audio
    // and Options must keep simulation and the Resume count-in stopped.
    stack.clear(); stack.notify_activation(true); input.reset_actions();
    settings.set_stage("movement_course"); settings.set_duration(60.0f); settings.set_player_count(1);
    settings.begin_input_claims(input_slots::keyboard_and_mouse);
    settings.set_screen_layout(ScreenLayout::one_player);
    settings.set_player_team(0, PlayerTeam::a); settings.set_player_weapon(0, WeaponType::sprayer);
    stack.push(std::make_unique<ResumeProbeRoot>());
    auto owned = std::make_unique<GameLevel>(&data, settings);
    GameLevel* game = owned.get();
    stack.push<EndMenuAction>(std::move(owned), [](const EndMenuAction&)
    {
        throw std::runtime_error("Controls/audio return unexpectedly ended its paused match");
    });
    for (int step = 0; step < 2400 && game->level().state() != LevelState::active; ++step) { tick(dt); }
    require(stack.depth() == 2 && game->level().state() == LevelState::active,
        "Controls/audio pause-origin fixture did not enter live play");
    for (int step = 0; step < 70; ++step) { tick(dt); }
    tap(Key::escape); tick(); require(stack.depth() == 3, "Actual GameLevel did not open Pause");
    tap(Key::down); tap(Key::down);
    const auto frozen = resume_observation(game->level());
    const Frame paused = main_menu_frame(app);
    tap(Key::enter); tick(); require(stack.depth() == 4, "Pause did not push actual Options");
    for (int row = preferences.fullscreen() ? 1 : 0; row < 3; ++row) { tap(Key::down); }
    const Frame options_from_pause = main_menu_frame(app);
    tap(Key::enter); tick(); require(stack.depth() == 5, "Paused Options did not push actual Controls/audio");
    const Frame controls_from_pause = main_menu_frame(app);
    tap(Key::enter); tick(); require(stack.depth() == 6, "Paused Controls/audio did not push Pad sticks");
    tap(Key::down); tap(Key::right); tick(0.75f); tap(Key::escape); tick(0.75f);
    require(stack.depth() == 5 && game->resume_remaining() == 0.0f &&
        frozen == resume_observation(game->level()) && controls_from_pause.pixels == main_menu_frame(app).pixels &&
        saved_bytes() == last_saved,
        "Pad sticks return changed/resumed the paused match, lost parent focus or saved a pending edit");
    tap(Key::down); tap(Key::down); tap(Key::right);
    require(controls_from_pause.pixels != main_menu_frame(app).pixels,
        "Controls/audio Pause-return fixture did not stage its discardable edit");
    tick(0.75f); tap(Key::escape); tick(0.75f);
    require(stack.depth() == 4 && game->resume_remaining() == 0.0f &&
        frozen == resume_observation(game->level()) && options_from_pause.pixels == main_menu_frame(app).pixels &&
        saved_bytes() == last_saved && play.aim_sensitivity == 3,
        "Controls/audio return resumed/changed the paused match, lost Options focus or saved pending edits");
    tap(Key::escape); tick(0.75f);
    require(stack.depth() == 3 && game->resume_remaining() == 0.0f &&
        frozen == resume_observation(game->level()) && paused.pixels == main_menu_frame(app).pixels &&
        saved_bytes() == last_saved && play.aim_sensitivity == 3,
        "Options return resumed/changed the paused match, lost Pause focus or saved pending edits");
    require(records.good(), "Controls/audio capture record write failed");
    std::cout << "PASS Controls/audio: complete fitted help, labels/values/status in linear ink; all nine cards/arrows, "
        "six native sizes/aspects; clamps, delayed Apply, reload/failure and Back discard; pending Options "
        "values/focus and actual GameLevel Pause return remain intact; frozen/reduced/random/resize\n";
    controls_menu_item_checks(app, canvas, tuning, presentation, output, save_path);
    controls_menu_help_checks(app, canvas, tuning, presentation, output, save_path);
    save_status_checks(app, canvas, tuning, presentation, output, true);
    settings_value_checks(app, canvas, tuning, presentation, output, true);
}
