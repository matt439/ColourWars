// Fixed setting values retain their page text and font through later callbacks.
void settings_value_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output, bool controls)
{
    using Values = decltype(MenuText{}.values);
    const Presentation original = presentation;
    // Only On/Off/Random vary here. Every other value template (percentage,
    // durations, resolution) keeps its authored text, because the pages
    // format those through the same struct and reject a blank template.
    const auto words_of = [&original](std::wstring on, std::wstring off, std::wstring random)
    {
        Values words = original.menu_text().values;
        words.on = std::move(on); words.off = std::move(off); words.random = std::move(random);
        return words;
    };
    const Values legacy = words_of(L"On", L"Off", L"Random");
    const auto profile = output / (controls ? "values-controls-profile" : "values-options-profile");
    struct ValueProfile
    {
        std::wstring previous;
        explicit ValueProfile(const std::filesystem::path& root)
        {
            wchar_t* raw = nullptr; size_t length = 0;
            require(_wdupenv_s(&raw, &length, L"LOCALAPPDATA") == 0, "Cannot read isolated value profile");
            if (raw != nullptr) { this->previous = raw; std::free(raw); }
            std::filesystem::create_directories(root);
            require(_wputenv_s(L"LOCALAPPDATA", root.c_str()) == 0, "Cannot isolate setting-value saves");
        }
        ~ValueProfile() { _wputenv_s(L"LOCALAPPDATA", this->previous.c_str()); }
    } isolated(profile);
    const auto save_path = profile / "ColourWars" / "save_data.json";
    const auto blocked = std::filesystem::path(save_path.wstring() + L".tmp");
    Save preferences;
    const auto reset = [&]
    {
        auto& play = const_cast<PlaySettings&>(preferences.play_settings()); play = PlaySettings{};
        play.music_volume = play.sfx_volume = 0;
        require(preferences.set_display_settings_and_save(ScreenResolution::s_1280_720,
            false, TeamColourPair::blue_yellow), "Cannot seed the isolated value save");
    };
    const auto saved_bytes = [&]
    {
        std::ifstream stream(save_path, std::ios::binary); require(stream.good(), "Value fixture lost its isolated save");
        return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    };
    reset();
    GameData data; data.set_application(&app); data.set_save(&preferences);
    data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings); StateContext& stack = app;
    struct RestoreValues
    {
        StateContext& stack; Presentation& presentation; Presentation original; std::filesystem::path blocked;
        ~RestoreValues()
        {
            this->stack.clear(); this->presentation = this->original;
            std::error_code ignored; std::filesystem::remove(this->blocked, ignored);
        }
    } restore{ stack, presentation, original, blocked };
    const auto tick = [&] { app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.0f); };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick();
        // This historical raster fixture samples the settled ornament pose.
        app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.51f);
    };
    const auto resize = [&](Vector2I size)
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
    };
    const auto open = [&]
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (Key key : { Key::up, Key::down, Key::left, Key::right, Key::enter, Key::space, Key::escape })
            { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        if (controls) { stack.push(std::make_unique<ControlsSettingsMenu>(&context)); }
        else { stack.push(std::make_unique<MainMenuOptions>(&context)); }
        tick(); tick(); require(stack.depth() == 2, "Value fixture failed to open its production page");
    };
    const auto apply = [&]
    {
        if (controls) { tap(Key::enter); return; }
        // Fullscreen stays Off and the resolution is the saved one, so Apply
        // takes the direct-save route: no display trial and no set_resolution,
        // whose resize would activate the hidden probe (report 23).
        const HWND window = static_cast<HWND>(app.window());
        tap(Key::enter); tick();
        require(stack.depth() == 2, "Value Apply pushed a display trial; it must take the direct-save route");
        require(!app.fullscreen() && IsWindowVisible(window) == FALSE && GetForegroundWindow() != window,
            "Value Apply exposed or activated its probe, or applied pending fullscreen On");
    };
    std::ofstream records(output / (controls ? "values-controls.csv" : "values-options.csv"));
    records << "case,page,row,value,width,height,font,raw_width,raw_height,scale,width_scale,height_scale,fixture_matches\n";
    const auto compare = [&](const std::string& name, bool controls_page, int row, const std::wstring& value,
        const Presentation& snapshot, bool focused, int fit_kind = 0)
    {
        const auto& look = snapshot.main_menu(); const auto& control = snapshot.controls_settings();
        MenuPaperArtPresentation art = controls_page ? control.art : look.art;
        if (!controls_page)
        {
            art.focus_padding = look.setup_focus_padding;
            art.shadow_offset = look.setup_shadow_offset;
        }
        const auto& style = snapshot.menu();
        const auto& font = controls_page ? control.item_font : style.item_font;
        const Vector2F position = controls_page ? control.layout.position : look.options_position;
        const Vector2F spacing = controls_page ? control.layout.spacing : look.options_spacing;
        const Vector2F label_position = position + Vector2F(0.0f, spacing.y * static_cast<float>(row));
        const Vector2F value_position(controls_page ? position.x + spacing.x : look.options_value_x, label_position.y);
        const Vector2F paper_size = controls_page ? control.box_size : look.options_box_size;
        const RectangleF panel((Vector2F(1920.0f, style.footer_y) - paper_size) / 2.0f, paper_size);
        const Vector2F limit(controls_page ? control.controls_position.x - art.focus_padding.x * 2.0f -
            art.shadow_offset - value_position.x : panel.right() - art.shadow_offset - art.focus_padding.x - value_position.x,
            spacing.y - art.focus_padding.y * 2.0f);
        const wchar_t* label = controls_page ? (row == 5 ? L"View shake" : row == 6 ? L"Aim marker" : L"Reduce effects") :
            (row == 1 ? L"Fullscreen" : L"Team palette");
        UiText label_text("reference-value-label", label, font, label_position, app.render_resources(),
            focused ? art.focus_ink : art.ink);
        const RectangleF raw_label = label_text.bounds();
        label_text.set_scale(std::min({ 1.0f, (value_position.x - label_position.x - art.focus_padding.x) / raw_label.width,
            limit.y / raw_label.height }));
        UiText text("reference-setting-value", value, font, value_position, app.render_resources(), art.ink);
        const Vector2F raw = text.bounds().size(); const float width_scale = limit.x / raw.x, height_scale = limit.y / raw.y;
        const float scale = std::min({ 1.0f, width_scale, height_scale }); text.set_scale(scale);
        require(fit_kind != 1 || (width_scale < 1.0f && width_scale < height_scale), "Long value did not exercise width fitting");
        require(fit_kind != 2 || (height_scale < 1.0f && height_scale < width_scale), "Tall value did not exercise height fitting");
        require(fit_kind != 3 || scale == 1.0f, "Short value did not reset its previous fit scale");
        require(panel.contains(text.bounds()) && text.bounds().width <= limit.x + 0.01f && text.bounds().height <= limit.y + 0.01f,
            "Independent setting value escaped its available column or row");
        const RectangleF row_bounds = RectangleF::union_of(label_text.bounds(), text.bounds());
        const Frame actual = main_menu_frame(app);
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app.viewport_manager()->fullscreen_viewport());
        list.set_camera(camera); list.set_filter(TextureFilter::point);
        const Canvas background(app.render_resources(), look.home_background_colour);
        background.draw(list, camera.visible_rectangle(app.viewport_manager()->fullscreen_viewport()));
        UiTexture focus_widget("reference-value-focus", style.sprite_sheet, style.fill_frame, row_bounds, app.render_resources());
        FocusGroup focus; if (focused) { focus.add(&focus_widget); }
        const MenuPaperArtwork artwork(panel, app.render_resources(), &style, &art, &focus); artwork.draw(list);
        list.set_filter(TextureFilter::linear); label_text.draw(list); text.draw(list); const Frame reference = read(renderer);
        RectangleF region = focused ? row_bounds : text.bounds(); if (focused) { region.inflate(art.focus_padding); }
        RectangleF projected = region; camera.calculate_view_rectangle(projected); const RectangleI pixels = countdown_crop_bounds(projected);
        const Frame observed = foreground_crop(actual, pixels, 0), wanted = foreground_crop(reference, pixels, 0);
        const std::string stem = "setting-value-" + name + '-' + std::to_string(row) + '-' + std::to_string(actual.width) + 'x' +
            std::to_string(actual.height);
        if (observed.pixels != wanted.pixels)
        {
            save(observed, output / (stem + "-failure-actual.ppm")); save(wanted, output / (stem + "-failure-expected.ppm"));
            throw std::runtime_error("Actual setting value differs from its complete independent fitted linear fixture");
        }
        save(observed, output / (stem + "-detail.ppm"));
        records << name << ',' << (controls_page ? "controls" : "options") << ',' << row << ',' <<
            (value == snapshot.menu_text().values.on ? "on" : value == snapshot.menu_text().values.off ? "off" :
                value == snapshot.menu_text().values.random ? "random" : "fixed") << ',' <<
            actual.width << ',' << actual.height << ',' << font << ',' << raw.x << ',' << raw.y << ',' << scale << ',' <<
            width_scale << ',' << height_scale << ",true\n";
        std::srand(8819); const int next_random = std::rand(); std::srand(8819);
        require(actual.pixels == main_menu_frame(app).pixels && std::rand() == next_random,
            "Setting-value redraw changed frozen pixels or consumed random numbers");
    };
    // Leave focus on Apply, with windowed Off and Random selected in Options;
    // Controls stages View/Aim Off and Reduce effects On.
    const auto exercise = [&](const std::string& name, const Presentation& snapshot, const Values& words, bool custom, bool short_values)
    {
        const int on_fit = !custom ? 0 : short_values ? 3 : 2, off_fit = !custom ? 0 : short_values ? 3 : 1;
        if (controls)
        {
            for (int row = 0; row < 5; ++row) { tap(Key::down); }
            for (int row = 5; row <= 7; ++row)
            {
                const bool initial = row != 7;
                compare(name + "-initial", true, row, initial ? words.on : words.off, snapshot, true, initial ? on_fit : off_fit);
                tap(Key::left);
                compare(name + "-left", true, row, words.off, snapshot, true, off_fit);
                tap(Key::right);
                compare(name + "-right", true, row, words.on, snapshot, true, on_fit);
                if (row != 7) { tap(Key::left); }
                tap(Key::down);
            }
        }
        else
        {
            tap(Key::down); compare(name + "-fullscreen-off", false, 1, words.off, snapshot, true, off_fit);
            tap(Key::right); compare(name + "-fullscreen-on", false, 1, words.on, snapshot, true, on_fit);
            tap(Key::left); compare(name + "-fullscreen-off-reset", false, 1, words.off, snapshot, true, off_fit);
            tap(Key::down); tap(Key::right);
            compare(name + "-palette-random", false, 2, words.random, snapshot, true, custom ? (short_values ? 3 : 1) : 0);
            tap(Key::left);
            compare(name + "-palette-fixed-reset", false, 2,
                snapshot.team_palette().fixed_pair(TeamColourPair::blue_yellow).label, snapshot, true, custom ? 3 : 0);
            tap(Key::right);
            compare(name + "-palette-random-cycle", false, 2, words.random, snapshot, true, custom ? (short_values ? 3 : 1) : 0);
            tap(Key::down); tap(Key::down); tap(Key::down);
        }
    };
    const auto compare_pending = [&](const std::string& name, const Presentation& snapshot, const Values& words)
    {
        if (controls)
        {
            compare(name, true, 5, words.off, snapshot, false); compare(name, true, 6, words.off, snapshot, false);
            compare(name, true, 7, words.on, snapshot, false);
        }
        else
        {
            compare(name, false, 1, words.off, snapshot, false); compare(name, false, 2, words.random, snapshot, false);
        }
    };
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440),
        Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        presentation = original; reset(); resize(size); open(); const std::string before = saved_bytes();
        exercise("legacy", original, legacy, false, false);
        require(saved_bytes() == before, "Setting value toggle or palette cycle saved before Apply");
        apply(); resize(size); compare_pending("legacy-applied", original, legacy);
    }
    // Random is also a Stage caption. This short replacement stays inside
    // that independently validated line while exercising the Options value.
    const Values old_words = words_of(L"On~", L"Off~", L"Random~"), next_words = words_of(L"Enabled", L"Disabled", L"All colours");
    Values long_words = old_words;
    for (int repeat = 0; repeat < 36; ++repeat) { long_words.off += L" previous value"; long_words.random += L" previous palette"; }
    const std::string source_name = controls ? original.controls_settings().item_font : original.menu().item_font;
    const Font* source = app.render_resources()->font(app.render_resources()->resolve_sprite_font(source_name));
    std::vector<Glyph> tall, missing;
    for (char32_t character = U' '; character <= U'~'; ++character)
    {
        const Glyph* source_glyph = source->find(character); require(source_glyph != nullptr, "Value font lacks printable baseline glyph");
        Glyph glyph = *source_glyph;
        if (character == U'~') { glyph.y_offset += 320.0f; }
        else { missing.push_back(glyph); }
        tall.push_back(glyph);
    }
    const std::string tall_name = controls ? "acceptance_controls_values_tall_tilde" : "acceptance_options_values_tall_tilde";
    const std::string next_name = controls ? "acceptance_controls_values_no_tilde" : "acceptance_options_values_no_tilde";
    app.render_resources()->add_font(tall_name, std::make_unique<Font>(source->atlas(), std::move(tall), source->line_spacing()));
    app.render_resources()->add_font(next_name, std::make_unique<Font>(source->atlas(), std::move(missing), source->line_spacing()));
    require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(next_name), L"~"),
        "Replacement value font still renders the retained page's old glyph");
    Presentation old = original; const_cast<MenuText&>(old.menu_text()).values = long_words;
    Presentation next = original; const_cast<MenuText&>(next.menu_text()).values = next_words;
    if (controls)
    {
        const_cast<ControlsSettingsPresentation&>(old.controls_settings()).item_font = tall_name;
        const_cast<ControlsSettingsPresentation&>(next.controls_settings()).item_font = next_name;
    }
    else
    {
        const_cast<MenuStyle&>(old.menu()).item_font = tall_name;
        const_cast<MenuStyle&>(next.menu()).item_font = next_name;
    }
    old.validate_resources(*app.render_resources(), *app.audio_resources()); next.validate_resources(*app.render_resources(), *app.audio_resources());
    resize({ 1280, 720 });
    for (bool reopened : { false, true })
    {
        const Presentation& snapshot = reopened ? next : old; const Values& words = reopened ? next_words : long_words;
        presentation = snapshot; reset(); open(); presentation = reopened ? old : next; tick();
        exercise(reopened ? "reopened-new" : "retained-old", snapshot, words, true, reopened);
        const std::string before = saved_bytes(); const Frame pending = main_menu_frame(app);
        app.keyboard()->on_key_down(Key::enter); tick(); static_cast<WindowNotify&>(app).on_deactivated(); tick();
        static_cast<WindowNotify&>(app).on_activated(); tick(); app.keyboard()->on_key_up(Key::enter); tick(); tick();
        require(saved_bytes() == before && pending.pixels == main_menu_frame(app).pixels,
            "Interrupted value Apply saved or changed the retained pending page");
        compare_pending("after-interruption", snapshot, words); apply(); compare_pending("after-apply", snapshot, words);
        const auto committed = saved_bytes(); require(committed != before, "Fresh value Apply did not persist edits");
        apply(); require(saved_bytes() == committed, "Repeated value Apply changed persisted preferences");
        compare_pending("after-repeated-apply", snapshot, words);
        require(std::filesystem::create_directory(blocked), "Cannot refuse the isolated value save"); apply();
        require(saved_bytes() == committed, "Failed value Apply replaced the previous JSON");
        compare_pending("after-save-failure", snapshot, words);
        require(std::filesystem::remove(blocked), "Cannot release isolated value save refusal"); apply();
        compare_pending("after-save-recovery", snapshot, words);
        Save reloaded; reloaded.load_save_file();
        require(!reloaded.fullscreen() && reloaded.team_palette() == preferences.team_palette() &&
            reloaded.play_settings().view_shake == preferences.play_settings().view_shake &&
            reloaded.play_settings().aim_marker == preferences.play_settings().aim_marker &&
            reloaded.play_settings().reduce_effects == preferences.play_settings().reduce_effects,
            "Setting-value callbacks lost independently reloaded preferences");
        for (bool escape : { false, true })
        {
            presentation = snapshot; reset(); open(); const auto discard_before = saved_bytes();
            presentation = reopened ? old : next;
            exercise(escape ? "discard-escape" : "discard-back", snapshot, words, true, reopened);
            if (escape) { tap(Key::escape); } else { tap(Key::down); tap(Key::enter); }
            tick(); require(saved_bytes() == discard_before, "Authored setting values changed Back/Escape into a save");
        }
    }
    // Return to an old page after opening a child from the replacement source.
    presentation = old; reset(); open(); exercise("parent-before-child", old, long_words, true, false);
    if (controls)
    {
        for (int row = 0; row < 8; ++row) { tap(Key::up); }
        presentation = next; tap(Key::enter); tick(); require(stack.depth() == 3, "Value parent failed to open Pad sticks");
        tap(Key::down); tap(Key::right);
        for (int row = 1; row < 7; ++row) { tap(Key::down); }
        tap(Key::enter);
        const int cutoff = preferences.play_settings().sticks[0].movement_deadzone;
        tap(Key::down); tap(Key::enter); tick(); require(stack.depth() == 2, "Value child Back lost the Controls parent");
        compare_pending("old-parent-after-child", old, long_words);
        for (int row = 0; row < 8; ++row) { tap(Key::down); } apply();
        Save reloaded; reloaded.load_save_file();
        require(!reloaded.play_settings().view_shake && !reloaded.play_settings().aim_marker && reloaded.play_settings().reduce_effects &&
            reloaded.play_settings().sticks[0].movement_deadzone == cutoff, "Value parent Apply lost a newer child's committed cutoff");
    }
    else
    {
        tap(Key::up); tap(Key::up); presentation = next; tap(Key::enter); tick(); require(stack.depth() == 3, "Options value parent failed to open Controls");
        for (int row = 0; row < 5; ++row) { tap(Key::down); }
        compare("new-child-on", true, 5, next_words.on, next, true); tap(Key::left);
        compare("new-child-off", true, 5, next_words.off, next, true); tap(Key::escape); tick();
        require(stack.depth() == 2, "Controls child Back lost its Options value parent");
        compare_pending("old-parent-after-child", old, long_words); tap(Key::down); tap(Key::down); apply();
        Save reloaded; reloaded.load_save_file(); require(!reloaded.fullscreen() && reloaded.team_palette() == TeamColourPair::random,
            "Value parent Apply lost the retained windowed or Random choice");
    }
    compare_pending("old-parent-applied-after-child", old, long_words);
    require(records.good(), "Setting-value evidence write failed");
    std::cout << (controls ? "PASS Controls On/Off values" : "PASS Options On/Off/Random values") <<
        ": independent defaults across six sizes/aspects; width and tall-glyph height fits, complete focus unions, " <<
        (controls ? "refitting between width/height limits and reopened short copy, " : "Random-to-short-fixed-palette scale reset, ") <<
        "a missing-old-glyph item font, retained/reopened/reversed callbacks; " <<
        (controls ? "Left/Right boolean assignments, Pad sticks return/profile merge, interrupted/repeated Apply" :
            "both fullscreen toggle directions/Random cycles, Controls child On/Off pair/return, interrupted/repeated windowed-Off Apply") <<
        ", controlled failure/recovery, isolated reload/discard\n";
}
