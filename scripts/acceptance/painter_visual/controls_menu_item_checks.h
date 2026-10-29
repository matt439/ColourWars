// Independent historical labels on both production settings pages. Existing
// help/status and response-preview matrices below keep their separate scope.
void controls_menu_item_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output, const std::filesystem::path& save_path)
{
    using Labels = std::array<std::wstring, static_cast<size_t>(MenuItem::count)>;
    constexpr std::array<MenuItem, 11> controls_roles{ MenuItem::pad_sticks, MenuItem::keyboard_jump, MenuItem::pad_buttons, MenuItem::profiles,
        MenuItem::music_volume,
        MenuItem::sfx_volume, MenuItem::view_shake, MenuItem::aim_marker, MenuItem::reduce_effects,
        MenuItem::apply, MenuItem::back };
    constexpr std::array<MenuItem, 9> stick_roles{ MenuItem::pad, MenuItem::move_deadzone,
        MenuItem::aim_deadzone, MenuItem::rumble, MenuItem::view_shake, MenuItem::aim_marker,
        MenuItem::reduce_effects, MenuItem::apply, MenuItem::back };
    const std::array<const wchar_t*, 11> controls_legacy{ L"Pad sticks", L"Keyboard Keys", L"Pad Buttons", L"Profiles", L"Music volume", L"SFX volume",
        L"View shake", L"Aim marker", L"Reduce effects", L"Apply", L"Back" };
    const std::array<const wchar_t*, 9> stick_legacy{ L"Pad", L"Move deadzone", L"Aim deadzone", L"Rumble",
        L"View shake", L"Aim marker", L"Reduce effects", L"Apply", L"Back" };
    const Presentation original = presentation;
    Labels defaults = original.menu_text().items;
    for (size_t row = 0; row < controls_roles.size(); ++row)
        { defaults[static_cast<size_t>(controls_roles[row])] = controls_legacy[row]; }
    for (size_t row = 0; row < stick_roles.size(); ++row)
        { defaults[static_cast<size_t>(stick_roles[row])] = stick_legacy[row]; }
    Save preferences;
    auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    const auto reset_preferences = [&]
    {
        PlaySettings fresh;
        fresh.music_volume = fresh.sfx_volume = 0;
        require(preferences.set_play_settings_and_save(fresh), "Cannot reset the isolated settings-label save");
    };
    reset_preferences();
    const auto saved_bytes = [&]
    {
        std::ifstream stream(save_path, std::ios::binary);
        require(stream.good(), "Settings-label fixture lost its isolated save");
        return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    };
    GameData data;
    data.set_application(&app); data.set_save(&preferences); data.set_tuning(&tuning);
    data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MatchSetup settings;
    MenuContext context(&data, &input, &settings);
    StateContext& stack = app;
    struct RestoreControlsItems
    {
        StateContext& stack;
        Presentation& presentation;
        Presentation original;
        ~RestoreControlsItems() { this->stack.clear(); this->presentation = this->original; }
    } restore{ stack, presentation, original };
    const auto tick = [&] { app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.0f); };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick();
        // This historical raster fixture samples the settled ornament pose.
        app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.51f);
    };
    const auto open = [&](bool sticks)
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true);
        app.mouse()->set_focused(true);
        for (const Key key : { Key::up, Key::down, Key::left, Key::right, Key::enter, Key::space, Key::escape })
            { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll();
        input.reset_actions(); stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        stack.push(std::make_unique<ControlsSettingsMenu>(&context)); tick(); tick();
        if (sticks)
        {
            tap(Key::enter); tick();
            for (int move = 0; move < input_slots::pad_count; ++move) { tap(Key::left); }
        }
        require(stack.depth() == (sticks ? 3 : 2), "Settings-label fixture did not open its actual parent/child page");
    };
    const auto percent = [](float value)
        { return std::to_wstring(static_cast<int>(std::lround(value * 100.0f))) + L"%"; };
    using Values = std::vector<std::wstring>;
    const auto initial_values = [&](bool sticks) -> Values
    {
        const auto shared = [](bool value) { return value ? L"Shared (On)" : L"Shared (Off)"; };
        if (sticks) { return { L"Pad 1", percent(play.movement_deadzone(0, tuning.input().movement_deadzone)),
            percent(play.aim_deadzone(0)), play.rumble_enabled(0) ? L"On" : L"Off", shared(play.view_shake),
            shared(play.aim_marker), shared(play.reduce_effects), L"", L"" }; }
        return { L"", L"", L"", L"", std::to_wstring(play.music_volume) + L"%", std::to_wstring(play.sfx_volume) + L"%",
            play.view_shake ? L"On" : L"Off", play.aim_marker ? L"On" : L"Off",
            play.reduce_effects ? L"On" : L"Off", L"", L"" };
    };
    std::ofstream records(output / "controls-menu-items.csv");
    records << "case,page,focus,item,width,height,label_scale,label_right,value_left,fixture_matches\n";
    const auto compare = [&](const std::string& name, bool sticks, size_t focused,
        const Presentation& snapshot, const Labels& labels, const Values& values, bool require_fitting = false)
    {
        const auto& style = snapshot.menu(); const auto& look = snapshot.controls_settings(); const auto& art = look.art;
        const Frame actual = main_menu_frame(app);
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        const auto project = [&](RectangleF bounds)
            { camera.calculate_view_rectangle(bounds); return countdown_crop_bounds(bounds); };
        const size_t count = sticks ? stick_roles.size() : controls_roles.size();
        std::vector<std::unique_ptr<UiText>> text;
        std::vector<float> scales;
        std::vector<RectangleF> row_bounds;
        std::vector<RectangleF> label_bounds;
        std::vector<MenuItem> roles;
        const float label_width = look.layout.spacing.x - art.focus_padding.x;
        const float row_height = look.layout.spacing.y - art.focus_padding.y * 2.0f;
        const float value_width = look.controls_position.x - art.focus_padding.x * 2.0f - art.shadow_offset -
            look.layout.position.x - look.layout.spacing.x;
        for (size_t row = 0; row < count; ++row)
        {
            const MenuItem role = sticks ? stick_roles[row] : controls_roles[row]; roles.push_back(role);
            const Vector2F position = look.layout.position + Vector2F(0.0f, look.layout.spacing.y * static_cast<float>(row));
            auto label = std::make_unique<UiText>("reference-settings-item", labels[static_cast<size_t>(role)],
                look.item_font, position, app.render_resources(), row == focused ? art.focus_ink : art.ink);
            float scale = 1.0f;
            const bool fitted_action = role == MenuItem::keyboard_jump || role == MenuItem::pad_buttons ||
                role == MenuItem::profiles || role == MenuItem::apply;
            if (!values[row].empty() || fitted_action)
            {
                const RectangleF raw = label->bounds();
                const float available_width = fitted_action ? look.controls_position.x -
                    art.focus_padding.x * 2.0f - art.shadow_offset - position.x : label_width;
                scale = std::min({ 1.0f, available_width / raw.width, row_height / raw.height });
                label->set_scale(scale);
                require(label->bounds().right() <= position.x + available_width + 0.01f &&
                    label->bounds().height <= row_height + 0.01f,
                    "Independent Controls/Sticks fitted label crosses its value column or row height");
                require(!require_fitting || fitted_action || scale < 1.0f,
                    "Long Controls/Sticks option label did not exercise production column fitting");
            }
            scales.push_back(scale); label_bounds.push_back(label->bounds()); row_bounds.push_back(label->bounds());
            text.push_back(std::move(label));
            if (!values[row].empty())
            {
                auto value = std::make_unique<UiText>("reference-settings-value", values[row], look.item_font,
                    position + Vector2F(look.layout.spacing.x, 0.0f), app.render_resources(), art.ink);
                const RectangleF raw = value->bounds();
                value->set_scale(std::min({ 1.0f, value_width / raw.width, row_height / raw.height }));
                row_bounds.back() = RectangleF::union_of(row_bounds.back(), value->bounds());
                text.push_back(std::move(value));
            }
        }
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app.viewport_manager()->fullscreen_viewport());
        list.set_camera(camera); list.set_filter(TextureFilter::point);
        const Canvas background(app.render_resources(), snapshot.main_menu().home_background_colour);
        background.draw(list, camera.visible_rectangle(app.viewport_manager()->fullscreen_viewport()));
        UiTexture focus_widget("reference-settings-focus", style.sprite_sheet, style.fill_frame,
            row_bounds[focused], app.render_resources());
        FocusGroup focus; focus.add(&focus_widget);
        const RectangleF panel((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - look.box_size) / 2.0f,
            look.box_size);
        const MenuPaperArtwork artwork(panel, app.render_resources(), &style, &art, &focus);
        artwork.draw(list);
        // Paper is in the texture layer on these pages, so their following
        // text layer restores linear sampling even after point-sampled cards.
        list.set_filter(TextureFilter::linear);
        for (const auto& widget : text) { widget->draw(list); }
        const Frame reference = read(renderer);
        const auto compare_bounds = [&](RectangleF bounds, const std::string& suffix)
        {
            bounds.inflate(Vector2F(2.0f)); const RectangleI pixels = project(bounds);
            const Frame observed = foreground_crop(actual, pixels, 0);
            const Frame wanted = foreground_crop(reference, pixels, 0);
            const std::string stem = "controls-menu-items-" + name + '-' + (sticks ? "sticks-" : "controls-") +
                std::to_string(focused) + '-' + suffix + '-' + std::to_string(actual.width) + 'x' +
                std::to_string(actual.height);
            if (observed.pixels != wanted.pixels)
            {
                save(observed, output / (stem + "-failure-actual.ppm"));
                save(wanted, output / (stem + "-failure-expected.ppm"));
                const RectangleI delta = foreground_delta_bounds(observed, wanted);
                std::cerr << "Controls item case=" << name << " sticks=" << sticks << " focus=" << focused <<
                    " item=" << suffix << " crop=(" << pixels.x << ',' << pixels.y << ',' << pixels.width << ',' <<
                    pixels.height << ") delta=(" << delta.x << ',' << delta.y << ',' << delta.width << ',' << delta.height << ")\n";
                throw std::runtime_error("Actual Controls/Sticks label differs from its independent fitted linear fixture");
            }
            save(observed, output / (stem + "-detail.ppm"));
        };
        for (size_t row = 0; row < count; ++row)
        {
            compare_bounds(row_bounds[row], std::to_string(static_cast<size_t>(roles[row])));
            records << name << ',' << (sticks ? "sticks" : "controls") << ',' << focused << ',' <<
                static_cast<size_t>(roles[row]) << ',' << actual.width << ',' << actual.height << ',' << scales[row] <<
                ',' << label_bounds[row].right() << ',' << (!values[row].empty() ? look.layout.position.x +
                    look.layout.spacing.x : 0.0f) << ",true\n";
        }
        RectangleF selected = row_bounds[focused]; selected.inflate(art.focus_padding);
        compare_bounds(selected, "complete-focus-card");
        std::srand(5099); const int next_random = std::rand(); std::srand(5099);
        require(actual.pixels == main_menu_frame(app).pixels && std::rand() == next_random,
            "Controls/Sticks label drawing changed frozen pixels or consumed random numbers");
        return actual;
    };
    std::array<Frame, 2> baseline{};
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440),
        Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        for (const bool sticks : { false, true })
        {
            open(sticks); const Frame actual = compare("default", sticks, 0, original, defaults, initial_values(sticks));
            if (size.x == 1280) { baseline[sticks ? 1 : 0] = actual; }
        }
    }
    Labels old_labels = defaults, new_labels = defaults;
    old_labels[static_cast<size_t>(MenuItem::pad_sticks)] = L"Sticks~";
    old_labels[static_cast<size_t>(MenuItem::apply)] = L"Apply~";
    old_labels[static_cast<size_t>(MenuItem::back)] = L"Back~";
    for (const MenuItem role : { MenuItem::music_volume, MenuItem::sfx_volume, MenuItem::view_shake,
        MenuItem::aim_marker, MenuItem::reduce_effects, MenuItem::pad, MenuItem::move_deadzone, MenuItem::aim_deadzone, MenuItem::rumble })
        { old_labels[static_cast<size_t>(role)] = L"Detailed~ shared setting for this paint session and every connected controller"; }
    const std::array<const wchar_t*, 11> next_controls{ L"Sticks", L"Jump key", L"Jump button", L"Name tags", L"Music gain", L"Effects gain", L"Camera shake",
        L"Aim cue", L"Less motion", L"Commit", L"Return" };
    const std::array<const wchar_t*, 9> next_sticks{ L"Slot", L"Movement cutoff", L"Aim cutoff", L"Haptics",
        L"Camera shake", L"Aim cue", L"Less motion", L"Commit", L"Return" };
    for (size_t row = 0; row < controls_roles.size(); ++row)
        { new_labels[static_cast<size_t>(controls_roles[row])] = next_controls[row]; }
    for (size_t row = 0; row < stick_roles.size(); ++row)
        { new_labels[static_cast<size_t>(stick_roles[row])] = next_sticks[row]; }
    Presentation old = original; const_cast<MenuText&>(old.menu_text()).items = old_labels;
    old.validate_resources(*app.render_resources(), *app.audio_resources());
    const Font* source = app.render_resources()->font(
        app.render_resources()->resolve_sprite_font(original.controls_settings().item_font));
    std::vector<Glyph> glyphs;
    for (char32_t character = U' '; character < U'~'; ++character)
    {
        const Glyph* glyph = source->find(character);
        require(glyph != nullptr, "Controls item replacement font lacks a baseline printable glyph"); glyphs.push_back(*glyph);
    }
    const std::string font_name = "acceptance_controls_items_without_tilde";
    app.render_resources()->add_font(font_name,
        std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
    require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(font_name), L"~"),
        "Controls item replacement font still contains the old labels' glyph");
    Presentation next = original; const_cast<MenuText&>(next.menu_text()).items = new_labels;
    const_cast<ControlsSettingsPresentation&>(next.controls_settings()).item_font = font_name;
    next.validate_resources(*app.render_resources(), *app.audio_resources());
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    for (const bool sticks : { false, true })
    {
        reset_preferences(); presentation = old; open(sticks);
        Values values = initial_values(sticks);
        const Frame retained = compare("custom-long", sticks, 0, old, old_labels, values, true);
        presentation = next; tick();
        require(retained.pixels == main_menu_frame(app).pixels,
            "Live Controls/Sticks item/font replacement rebased an existing page");
        tap(Key::down); tap(Key::up);
        require(retained.pixels == main_menu_frame(app).pixels,
            "Controls/Sticks focus movement rebased old labels into the replacement font");
        compare("retained-old", sticks, 0, old, old_labels, values, true);
        if (!sticks)
        {
            for (int repeat = 0; repeat < 2; ++repeat)
            {
                const std::string before_child = saved_bytes();
                tap(Key::enter); tick(); require(stack.depth() == 3, "Custom Controls did not reopen its actual Sticks child");
                for (int move = 0; move < input_slots::pad_count; ++move) { tap(Key::left); }
                compare("new-child-from-old-parent", true, 0, next, new_labels, initial_values(true));
                tap(Key::down); tap(Key::right); tap(Key::escape); tick();
                require(stack.depth() == 2 && saved_bytes() == before_child && retained.pixels == main_menu_frame(app).pixels,
                    "Custom Sticks discard/return changed the old Controls labels, font, focus or save");
            }
        }
        const size_t edited_row = sticks ? 1 : 4;
        if (!sticks) { tap(Key::down); tap(Key::down); tap(Key::down); }
        const std::string unapplied = saved_bytes();
        tap(Key::down); tap(Key::right);
        values[edited_row] = sticks ? percent(std::min(0.9f, play.movement_deadzone(0, tuning.input().movement_deadzone) + 0.05f)) : L"10%";
        compare("pending-old-font", sticks, edited_row, old, old_labels, values, true);
        const size_t apply_row = sticks ? 7 : 9;
        for (size_t row = edited_row; row < apply_row; ++row) { tap(Key::down); }
        const Frame before_apply = compare("old-apply-before-focus-loss", sticks, apply_row, old, old_labels, values, true);
        app.keyboard()->on_key_down(Key::enter); tick();
        static_cast<WindowNotify&>(app).on_deactivated(); tick();
        static_cast<WindowNotify&>(app).on_activated(); tick();
        app.keyboard()->on_key_up(Key::enter); tick(); tick();
        require(saved_bytes() == unapplied && before_apply.pixels == main_menu_frame(app).pixels,
            "Interrupted Controls/Sticks Enter applied pending settings or rebased cached labels");
        tap(Key::enter);
        require(sticks ? play.sticks[0].movement_deadzone == std::stoi(values[1]) : play.music_volume == 10,
            "Custom Controls/Sticks Apply label changed which pending settings were committed");
        Save reloaded; reloaded.load_save_file();
        require(sticks ? reloaded.play_settings().sticks[0].movement_deadzone == std::stoi(values[1]) :
            reloaded.play_settings().music_volume == 10, "Custom Controls/Sticks Apply did not survive save reload");
        compare("applied-retained-old", sticks, apply_row, old, old_labels, values, true);
        const std::string applied = saved_bytes(); tap(Key::enter);
        require(saved_bytes() == applied, "Repeated custom Controls/Sticks Apply changed the selected preferences");
        compare("repeated-apply-old", sticks, apply_row, old, old_labels, values, true);
        tap(Key::down); tap(Key::enter); tick();
        require(stack.depth() == (sticks ? 2 : 1), "Custom Controls/Sticks Back did not close only its own page");
        if (sticks)
        {
            compare("old-parent-after-child-apply", false, 0, old, old_labels, initial_values(false), true);
            for (size_t row = 0; row < 9; ++row) { tap(Key::down); }
            tap(Key::enter);
            require(play.sticks[0].movement_deadzone == std::stoi(values[1]),
                "Old Controls Apply replaced independently applied custom Sticks settings");
            tap(Key::escape); tick();
        }
        presentation = next; open(sticks); values = initial_values(sticks);
        const Frame adopted = compare("reopened-new-font", sticks, 0, next, new_labels, values);
        require(adopted.pixels != retained.pixels, "Reopened Controls/Sticks did not adopt authored labels and its own font");
        presentation = original; tick(); tap(Key::down); tap(Key::up);
        require(adopted.pixels == main_menu_frame(app).pixels,
            "Controls/Sticks font reversal or focus movement changed the reopened text snapshot");
        compare("retained-new-after-reversal", sticks, 0, next, new_labels, values);
        const std::string before_discard = saved_bytes();
        if (!sticks) { tap(Key::down); tap(Key::down); tap(Key::down); }
        tap(Key::down); tap(Key::right);
        values[edited_row] = sticks ? percent(std::min(0.9f, play.movement_deadzone(0, tuning.input().movement_deadzone) + 0.05f)) : L"20%";
        compare("pending-new-after-reversal", sticks, edited_row, next, new_labels, values);
        tap(Key::escape); tick();
        require(stack.depth() == (sticks ? 2 : 1) && saved_bytes() == before_discard,
            "Custom Controls/Sticks Escape persisted an unapplied edit or closed the wrong frame");
        reset_preferences(); presentation = original; open(sticks);
        require(compare("restored-default", sticks, 0, original, defaults, initial_values(sticks)).pixels == baseline[sticks ? 1 : 0].pixels,
            "Controls/Sticks source reversal and reopen did not restore the exact default raster");
    }
    require(records.good(), "Controls/Sticks authored item evidence write failed");
    std::cout << "PASS actual Controls/Sticks labels: 20 independent labels at six sizes/aspects; "
        "long OptionRow fitting, complete label/value focus cards and linear sampling; separate controls item font, "
        "retained/reopened/reversed snapshots, repeated child return/Apply, interrupted Enter, save reload, "
        "Back/Escape discard and parent Apply preserving child profiles; frozen draw RNG\n";
}
