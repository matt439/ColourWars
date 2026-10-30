// Real Pad sticks page, reached through Controls/audio. This fixture owns its
// profile and drives keyboard navigation; a physical controller is unnecessary.
void stick_settings_matrix(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    struct StickProfile
    {
        std::wstring previous;
        explicit StickProfile(const std::filesystem::path& root)
        {
            wchar_t* raw = nullptr;
            size_t length = 0;
            require(_wdupenv_s(&raw, &length, L"LOCALAPPDATA") == 0,
                "Cannot read the runner's isolated stick profile");
            if (raw != nullptr) { this->previous = raw; std::free(raw); }
            std::filesystem::create_directories(root);
            require(_wputenv_s(L"LOCALAPPDATA", root.c_str()) == 0,
                "Cannot isolate Pad sticks save writes");
        }
        ~StickProfile() { _wputenv_s(L"LOCALAPPDATA", this->previous.c_str()); }
    } isolated(output / "stick-profile");
    const std::filesystem::path save_path = output / "stick-profile" / "ColourWars" / "save_data.json";
    const auto saved_bytes = [&]
    {
        std::ifstream stream(save_path, std::ios::binary);
        require(stream.good(), "Pad sticks did not create its isolated save");
        return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    };
    Save preferences; SettingsSession preferences_session(preferences);
    auto& play = const_cast<PlaySettings&>(preferences_session.play_settings());
    play.music_volume = play.sfx_volume = 0;
    require(preferences.set_display_settings_and_save(ScreenResolution::s_1920_1080,
        false, TeamColourPair::blue_yellow), "Cannot seed the isolated Pad sticks save");
    GameData data;
    data.set_application(&app); data.set_save(&preferences); data.set_settings_session(&preferences_session);
    data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MatchSetup settings;
    MenuContext context(&data, &input, &settings);
    StateContext& stack = app;
    struct ClearStickStates
    {
        StateContext& stack;
        ~ClearStickStates() { this->stack.clear(); }
    } clear{ stack };
    const ControlsSettingsPresentation& look = presentation.controls_settings();
    const MenuStyle& style = presentation.menu();
    auto& art = const_cast<MenuPaperArtPresentation&>(look.art);
    struct RestoreStickArt
    {
        MenuPaperArtPresentation& art;
        MenuPaperArtPresentation original;
        ~RestoreStickArt() { this->art = this->original; }
    } restore{ art, art };
    const RectangleF panel((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) -
        look.box_size) / 2.0f, look.box_size);
    const std::array<const wchar_t*, 9> labels{ L"Pad", L"Move deadzone", L"Aim deadzone", L"Rumble",
        L"View shake", L"Aim marker", L"Reduce effects", L"Apply", L"Back" };
    const std::array<const char*, 9> names{ "pad", "move", "aim", "rumble", "view-shake", "aim-marker",
        "reduce-effects", "apply", "back" };
    const std::wstring pending = L"Apply to save pad settings.";
    const auto measure = [&](const std::string& font, const std::wstring& text)
    {
        return app.render_resources()->measure_text(app.render_resources()->resolve_sprite_font(font), text);
    };
    const auto label_scale = [&](size_t row)
    {
        if (row != 7) { return 1.0f; }
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
    using Values = std::array<std::wstring, 7>;
    const auto row_bounds = [&](size_t row, const Values& values)
    {
        RectangleF result = row_label(row);
        if (row < values.size())
        {
            const RectangleF second = value_bounds(row, values[row]);
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
    const RectangleF heading(look.heading_position, measure(style.heading_font, L"Pad sticks"));
    require(panel.contains(heading) && heading.bottom() < card(row_label(0)).y,
        "Pad sticks heading overlaps its first row or escapes paper");
    for (size_t row = 0; row < labels.size(); ++row)
    {
        const float padded_height = row_label(row).height + art.focus_padding.y * 2.0f;
        // A smaller font may fit without scaling. Test the complete padded
        // row and shade rather than requiring an overflowing raw label.
        require(panel.contains(card(row_label(row))) &&
            padded_height + art.shadow_offset <= look.layout.spacing.y,
            "Pad sticks action card escapes paper or overlaps its neighbour");
    }
    require(label_scale(7) > 0.0f && label_scale(7) <= 1.0f &&
        row_label(7).height + art.focus_padding.y * 2.0f <= look.layout.spacing.y,
        "Pad sticks Apply label did not fit its available row");
    const Values widest{ L"Pad 4", L"90%", L"90%", L"Off", L"Shared (Off)", L"Shared (Off)", L"Shared (Off)" };
    for (size_t row = 0; row < widest.size(); ++row)
    {
        require(row_label(row).right() + art.focus_padding.x < value_bounds(row, widest[row]).x &&
            card(row_bounds(row, widest)).right() < look.controls_position.x &&
            panel.contains(card(row_bounds(row, widest))),
            "Pad sticks label/value card overlaps preview or escapes paper");
    }
    const StickPreviewPresentation& preview = look.stick_preview;
    for (RectangleF bounds : { preview.raw_movement, preview.movement_response, preview.aim })
    {
        bounds.inflate(Vector2F(preview.marker_size + preview.line_width));
        require(panel.contains(bounds), "Pad sticks preview escapes its paper panel");
    }

    app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
    for (const Key key : { Key::up, Key::down, Key::left, Key::right, Key::enter, Key::escape })
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
    const auto open_parent = [&]
    {
        stack.clear(); stack.notify_activation(true); input.reset_actions();
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        stack.push(std::make_unique<ControlsSettingsMenu>(&context)); tick();
        require(stack.depth() == 2, "Pad sticks parent did not remain open after neutral input");
    };
    const auto open_child = [&]
    {
        tap(Key::enter); tick();
        require(stack.depth() == 3, "Controls/audio did not push the actual Pad sticks page");
        for (int step = 0; step < 8; ++step) { tap(Key::left); }
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
    const auto status_bounds = [&](const std::wstring& status)
    {
        const Vector2F raw = measure(look.help_font, status);
        const float available = panel.right() - art.shadow_offset - art.focus_padding.x - look.status_position.x;
        return RectangleF(look.status_position, raw * std::min(1.0f, available / raw.x));
    };
    for (const wchar_t* status : { pending.c_str(), L"Pad settings applied and saved.",
        L"Active for this run. Could not save." })
    {
        require(panel.contains(status_bounds(status)) && card(row_label(8)).bottom() < status_bounds(status).y,
            "Pad sticks save status escapes paper or overlaps Back");
    }
    const auto contrast = [&](const Frame& frame, const RectangleF& bounds)
    {
        const RectangleI pixels = project(bounds);
        require(pixels.x >= 0 && pixels.y >= 0 && pixels.right() <= frame.width && pixels.bottom() <= frame.height,
            "Pad sticks copy escaped its native display");
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
            "Native Pad sticks copy lost readable ink/paper contrast");
    };
    const auto lettering = [&](const Frame& actual, size_t selected, const Values& values,
        const std::wstring& status, const Colour& status_colour)
    {
        std::vector<std::unique_ptr<UiText>> text;
        const auto add = [&](const std::wstring& copy, const std::string& font,
            const Vector2F& position, const Colour& colour, float scale = 1.0f)
        {
            text.push_back(std::make_unique<UiText>("expected-stick-copy", copy, font,
                position, app.render_resources(), colour));
            text.back()->set_scale(scale);
        };
        add(L"Pad sticks", style.heading_font, heading.position(), art.ink);
        for (size_t row = 0; row < labels.size(); ++row)
        {
            add(labels[row], look.item_font, row_label(row).position(), row == selected ? art.focus_ink : art.ink,
                label_scale(row));
            if (row < values.size())
            {
                add(values[row], look.item_font, value_bounds(row, values[row]).position(), art.ink);
            }
        }
        add(status, look.help_font, look.status_position, status_colour,
            status_bounds(status).width / measure(look.help_font, status).x);
        const int slot = static_cast<int>(values[0].back() - L'1');
        const GamepadState& state = app.gamepads()->state(slot);
        const Vector2F left = state.connected ? state.left_stick : Vector2F::ZERO;
        const Vector2F right = state.connected ? state.right_stick : Vector2F::ZERO;
        PlaySettings expected_play = play;
        expected_play.sticks[static_cast<size_t>(slot)] = { std::stoi(values[1]), std::stoi(values[2]) };
        const StickResponse response = stick_response(left, right, tuning.input().movement_deadzone, expected_play, slot);
        const auto percent = [](float value)
        {
            return std::to_wstring(static_cast<int>(std::lround(value * 100.0f))) + L"%";
        };
        const float label_height = measure(look.help_font, L"Move").y;
        add(values[0] + (state.connected ? L" - connected" : L" - disconnected"), look.help_font,
            look.controls_position, art.ink);
        add(L"Left stick raw X: " + percent(left.x), look.help_font, preview.raw_movement.position() -
            Vector2F(0.0f, label_height + art.focus_padding.y), art.ink);
        add(L"Move response: " + percent(response.movement), look.help_font, preview.movement_response.position() -
            Vector2F(0.0f, label_height + art.focus_padding.y), art.ink);
        add(L"Right stick: radial cutoff", look.help_font, preview.aim.position() -
            Vector2F(0.0f, label_height + art.focus_padding.y), art.ink);
        const Vector2F aim = response.aim.normalized();
        add(L"Raw X: " + percent(right.x) + L"\nRaw Y: " + percent(right.y) + L"\n" +
            (response.aim == Vector2F::ZERO ? std::wstring(L"No new aim") :
                L"Aim X: " + percent(aim.x) + L"\nAim Y: " + percent(aim.y)) + L"\nRaw: hollow\nAim: solid",
            look.help_font, preview.aim_readout_position, art.ink);
        const std::wstring association = L"Profiles follow Pad 1-4 slots,\nnot the player number or device.\n"
            L"After reconnecting, check the slot.\nKeyboard/mouse is unchanged.";
        const Vector2F raw_association = measure(look.help_font, association);
        const Vector2F available = look.controls_position + look.controls_size - preview.association_position;
        const float association_scale = std::min({ 1.0f, available.x / raw_association.x,
            available.y / raw_association.y });
        add(association, look.help_font, preview.association_position, art.ink, association_scale);
        UiTexture focus_widget("expected-stick-focus", "sprite_sheet_1", "pixel",
            row_bounds(selected, values), app.render_resources());
        FocusGroup focus; focus.add(&focus_widget);
        const MenuPaperArtwork artwork(panel, app.render_resources(), &style, &art, &focus);
        const Canvas paper(app.render_resources(), art.paper);
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0);
        const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
        list.set_viewport(viewport); list.set_camera(camera()); list.set_filter(TextureFilter::point);
        paper.draw(list, camera().visible_rectangle(viewport)); artwork.draw(list);
        list.set_filter(TextureFilter::linear);
        for (const auto& widget : text) { widget->draw(list); }
        const Frame expected = read(renderer);
        for (const auto& widget : text)
        {
            require(panel.contains(widget->bounds()), "Pad sticks copy or preview readout escaped its paper panel");
            const RectangleI pixels = project(widget->bounds());
            if (foreground_crop(actual, pixels, 0).pixels != foreground_crop(expected, pixels, 0).pixels)
            {
                save(actual, output / "stick-lettering-actual.ppm");
                save(expected, output / "stick-lettering-expected.ppm");
                throw std::runtime_error("Actual Pad sticks lettering lost authored copy, fitting or linear sampling");
            }
            contrast(actual, widget->bounds());
        }
    };
    std::ofstream records(output / "stick-settings.csv");
    records << "focus,width,height,paper_pixels,focus_pixels\n";
    const auto capture = [&](const char* name, size_t selected, const Values& values,
        const std::wstring& status, const Colour& status_colour)
    {
        const Frame normal = main_menu_frame(app);
        std::srand(8191); const int next_random = std::rand(); std::srand(8191);
        require(normal.pixels == main_menu_frame(app).pixels && normal.pixels == main_menu_frame(app).pixels &&
            std::rand() == next_random, "Frozen Pad sticks draw changed pixels or consumed randomness");
        tick(0.75f);
        require(normal.pixels == main_menu_frame(app).pixels, "Neutral time moved Pad sticks artwork");
        const bool reduce = play.reduce_effects; play.reduce_effects = !reduce; tick(0.75f);
        require(normal.pixels == main_menu_frame(app).pixels, "Reduce effects changed Pad sticks information");
        play.reduce_effects = reduce;
        RectangleF expanded = panel; expanded.inflate(Vector2F(art.keyline_width + art.shadow_offset + 4.0f));
        const RectangleI pixels = project(expanded);
        require(pixels.x >= 0 && pixels.y >= 0 && pixels.right() <= normal.width && pixels.bottom() <= normal.height,
            "Pad sticks paper escaped a supported display");
        const Colour paper = art.paper; art.paper = Colour(1.0f, 0.0f, 1.0f);
        const Frame tinted = main_menu_frame(app); art.paper = paper;
        require(pixels.contains(foreground_delta_bounds(normal, tinted)),
            "Pad sticks paper escaped its authored panel");
        const size_t paper_pixels = foreground_local_delta(normal, tinted,
            RectangleF(Vector2F(pixels.position()), Vector2F(pixels.size())));
        const Colour fill = art.focus_fill; art.focus_fill = Colour(0.0f, 1.0f, 1.0f);
        const Frame focused = main_menu_frame(app); art.focus_fill = fill;
        const RectangleI focused_pixels = project(card(row_bounds(selected, values)));
        // The same authored shade marks the raw deadzone. Exclude that known
        // preview region before checking that focus tint stays inside its row.
        Frame focus_only = focused;
        const RectangleI raw_pixels = project(preview.raw_movement);
        for (int y = raw_pixels.y; y < raw_pixels.bottom(); ++y)
        {
            const size_t offset = hud_pixel_offset(normal, raw_pixels.x, y);
            std::copy_n(normal.pixels.begin() + offset, static_cast<size_t>(raw_pixels.width) * 4,
                focus_only.pixels.begin() + offset);
        }
        const RectangleI focus_delta = foreground_delta_bounds(normal, focus_only);
        require(focus_delta.width > 0 && focus_delta.height > 0 && focused_pixels.contains(focus_delta),
            "Pad sticks focus fill escaped the complete label/value row");
        const size_t focus_pixels = foreground_local_delta(normal, focus_only,
            RectangleF(Vector2F(focused_pixels.position()), Vector2F(focused_pixels.size())));
        require(paper_pixels > 100 && focus_pixels > 30, "Pad sticks paper/focus card has no substantial fill");
        RectangleF padded = row_bounds(selected, values); padded.inflate(art.focus_padding);
        const float pen = art.keyline_width;
        const RectangleI arrow = project(RectangleF(padded.x - pen * 8.0f,
            padded.center().y - pen * 4.0f, pen * 7.0f, pen * 8.0f));
        const Colour ink = art.ink; art.ink = Colour(1.0f, 0.0f, 1.0f);
        const Frame ink_tinted = main_menu_frame(app); art.ink = ink;
        require(foreground_crop(normal, arrow, 0).pixels != foreground_crop(ink_tinted, arrow, 0).pixels,
            "Pad sticks focus lost its detached ink arrow");
        lettering(normal, selected, values, status, status_colour);
        const std::string stem = std::string("stick-settings-") + name + '-' +
            std::to_string(normal.width) + 'x' + std::to_string(normal.height);
        save(normal, output / (stem + ".ppm"));
        save(foreground_crop(normal, pixels), output / (stem + "-detail.ppm"));
        records << name << ',' << normal.width << ',' << normal.height << ',' << paper_pixels << ',' << focus_pixels << '\n';
        resize_page({ 1920, 1080 }); std::ignore = main_menu_frame(app);
        resize_page({ normal.width, normal.height });
        require(normal.pixels == main_menu_frame(app).pixels, "Pad sticks resize changed pixels or focus");
        return normal;
    };
    // A fresh page follows the shared Options switches: on, on, off.
    const Values initial{ L"Pad 1", L"50%", L"50%", L"On", L"Shared (On)", L"Shared (On)", L"Shared (Off)" };
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
    {
        resize_page(size); open_parent(); open_child();
        for (size_t row = 0; row < labels.size(); ++row)
        {
            capture(names[row], row, initial, pending, art.focus_ink);
            if (row + 1 < labels.size()) { tap(Key::down); }
        }
    }
    resize_page({ 1280, 720 }); open_parent(); tap(Key::enter); tick();
    int initial_slot = 0;
    for (int slot = 0; slot < input_slots::pad_count; ++slot)
    {
        if (app.gamepads()->connected(slot)) { initial_slot = slot; break; }
    }
    std::ignore = capture("initial-slot", 0, { input_slots::device_name(initial_slot), L"50%", L"50%", L"On",
        L"Shared (On)", L"Shared (On)", L"Shared (Off)" },
        pending, art.focus_ink);
    open_parent(); stack.push(std::make_unique<StickSettingsMenu>(&context, 3)); tick();
    std::ignore = capture("pausing-slot", 0, { L"Pad 4", L"50%", L"50%", L"On",
        L"Shared (On)", L"Shared (On)", L"Shared (Off)" }, pending, art.focus_ink);

    // The child saves only sticks into the current settings. The suspended
    // parent later saves its older pending music choice without reverting them.
    resize_page({ 1280, 720 }); open_parent();
    tap(Key::down); tap(Key::down); tap(Key::down); tap(Key::down); tap(Key::right);
    tap(Key::up); tap(Key::up); tap(Key::up); tap(Key::up); open_child();
    const std::string original = saved_bytes();
    Values values = initial;
    tap(Key::down);
    for (int step = 0; step < 30; ++step) { tap(Key::left); }
    values[1] = L"0%"; const Frame movement_min = capture("move-min", 1, values, pending, art.focus_ink);
    for (int step = 0; step < 30; ++step) { tap(Key::right); }
    values[1] = L"90%"; const Frame movement_max = capture("move-max", 1, values, pending, art.focus_ink);
    require(foreground_crop(movement_min, project(preview.raw_movement), 0).pixels !=
        foreground_crop(movement_max, project(preview.raw_movement), 0).pixels,
        "Pending movement deadzone did not change its preview cutoff");
    for (int step = 0; step < 12; ++step) { tap(Key::left); }
    values[1] = L"30%"; tap(Key::down);
    for (int step = 0; step < 30; ++step) { tap(Key::left); }
    values[2] = L"0%"; const Frame aim_min = capture("aim-min", 2, values, pending, art.focus_ink);
    for (int step = 0; step < 30; ++step) { tap(Key::right); }
    values[2] = L"90%"; const Frame aim_max = capture("aim-max", 2, values, pending, art.focus_ink);
    require(foreground_crop(aim_min, project(preview.aim), 0).pixels !=
        foreground_crop(aim_max, project(preview.aim), 0).pixels,
        "Pending aim deadzone did not change its radial preview cutoff");
    for (int step = 0; step < 4; ++step) { tap(Key::left); }
    values[2] = L"70%";
    require(saved_bytes() == original && play.sticks[0].movement_deadzone == -1 &&
        play.sticks[0].aim_deadzone == -1 && play.music_volume == 0,
        "Pad sticks changed active preferences or saved before Apply");
    for (int row = 2; row < 7; ++row) { tap(Key::down); }
    tap(Key::enter);
    require(play.sticks[0].movement_deadzone == 30 && play.sticks[0].aim_deadzone == 70 && play.music_volume == 0,
        "Pad sticks Apply lost its pending values or applied suspended parent edits");
    capture("applied", 7, values, L"Pad settings applied and saved.", art.focus_ink);
    const std::string child_saved = saved_bytes();
    Save reloaded; SettingsSession reloaded_session(reloaded); reloaded_session.load_save_file();
    require(reloaded_session.play_settings().sticks[0].movement_deadzone == 30 &&
        reloaded_session.play_settings().sticks[0].aim_deadzone == 70 && reloaded_session.play_settings().music_volume == 0 &&
        reloaded.resolution() == ScreenResolution::s_1920_1080 && reloaded.team_palette() == TeamColourPair::blue_yellow,
        "Pad sticks reload lost values or replaced other preferences");
    for (int row = 7; row > 0; --row) { tap(Key::up); }
    for (int step = 0; step < 8; ++step) { tap(Key::right); }
    values = { L"Pad 4", L"50%", L"50%", L"On", L"Shared (On)", L"Shared (On)", L"Shared (Off)" };
    capture("pad-max", 0, values, L"Pad settings applied and saved.", art.focus_ink);
    tap(Key::down); tap(Key::right); tap(Key::down); tap(Key::left);
    values[1] = L"55%"; values[2] = L"45%";
    capture("discard-pending", 2, values, pending, art.focus_ink);
    for (int row = 2; row < 8; ++row) { tap(Key::down); }
    tap(Key::enter);
    require(stack.depth() == 2 && saved_bytes() == child_saved &&
        play.sticks[3].movement_deadzone == -1 && play.sticks[3].aim_deadzone == -1,
        "Pad sticks Back saved an unapplied edit to another pad");
    tick();
    for (int row = 0; row < 9; ++row) { tap(Key::down); }
    tap(Key::enter);
    require(play.music_volume == 10 && play.sticks[0].movement_deadzone == 30 && play.sticks[0].aim_deadzone == 70 &&
        play.sticks[3].movement_deadzone == -1 && play.sticks[3].aim_deadzone == -1,
        "Suspended Controls/audio Apply reverted child stick settings");
    const std::string parent_saved = saved_bytes();
    reloaded_session.load_save_file();
    require(reloaded_session.play_settings().music_volume == 10 && reloaded_session.play_settings().sticks[0].movement_deadzone == 30 &&
        reloaded_session.play_settings().sticks[0].aim_deadzone == 70,
        "Parent Apply failed to persist both independent settings edits");
    for (int row = 0; row < 9; ++row) { tap(Key::up); }
    open_child(); values = { L"Pad 1", L"30%", L"70%", L"On", L"Shared (On)", L"Shared (On)", L"Shared (Off)" };
    capture("reopened", 0, values, pending, art.focus_ink);
    tap(Key::down); tap(Key::right); tap(Key::escape);
    require(stack.depth() == 2 && saved_bytes() == parent_saved && play.sticks[0].movement_deadzone == 30,
        "Pad sticks Escape saved an unapplied edit");
    tick();
    open_child(); tap(Key::down); tap(Key::right); values[1] = L"35%";
    const std::filesystem::path blocked = save_path.wstring() + L".tmp";
    require(std::filesystem::create_directory(blocked), "Cannot block isolated Pad sticks temporary save");
    for (int row = 1; row < 7; ++row) { tap(Key::down); }
    tap(Key::enter);
    require(play.sticks[0].movement_deadzone == 35 && saved_bytes() == parent_saved,
        "Failed Pad sticks save lost the active choice or replaced previous JSON");
    capture("save-failed", 7, values, L"Active for this run. Could not save.",
        presentation.main_menu().options_warning_colour);
    require(std::filesystem::remove(blocked), "Cannot remove isolated Pad sticks write refusal");
    require(records.good(), "Pad sticks capture record write failed");
    std::cout << "PASS Pad sticks: actual child navigation; nine bounded readable cards at 720p/1080p/QHD; "
        "pad/deadzone clamps, pending Apply, reload/failure and Back/Escape discard; parent Apply preserves "
        "child profiles; frozen/reduced/random/resize; no physical-controller comfort acceptance\n";
}
