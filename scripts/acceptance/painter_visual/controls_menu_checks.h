// Independent historical labels on both production settings pages. Existing
// help/status and response-preview matrices below keep their separate scope.
void controls_menu_item_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output, const std::filesystem::path& save_path)
{
    using Labels = std::array<std::wstring, static_cast<size_t>(MenuItem::count)>;
    constexpr std::array<MenuItem, 8> controls_roles{ MenuItem::pad_sticks, MenuItem::music_volume,
        MenuItem::sfx_volume, MenuItem::view_shake, MenuItem::aim_marker, MenuItem::reduce_effects,
        MenuItem::apply, MenuItem::back };
    constexpr std::array<MenuItem, 5> stick_roles{ MenuItem::pad, MenuItem::move_deadzone,
        MenuItem::aim_deadzone, MenuItem::apply, MenuItem::back };
    const std::array<const wchar_t*, 8> controls_legacy{ L"Pad sticks", L"Music volume", L"SFX volume",
        L"View shake", L"Aim marker", L"Reduce effects", L"Apply", L"Back" };
    const std::array<const wchar_t*, 5> stick_legacy{ L"Pad", L"Move deadzone", L"Aim deadzone", L"Apply", L"Back" };
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
    MenuLevelSettings settings;
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
        if (sticks) { return { L"Pad 1", percent(play.movement_deadzone(0, tuning.input().movement_deadzone)),
            percent(play.aim_deadzone(0)), L"", L"" }; }
        return { L"", std::to_wstring(play.music_volume) + L"%", std::to_wstring(play.sfx_volume) + L"%",
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
            if (!values[row].empty())
            {
                const RectangleF raw = label->bounds();
                scale = std::min({ 1.0f, label_width / raw.width, row_height / raw.height });
                label->set_scale(scale);
                require(label->bounds().right() <= position.x + label_width + 0.01f &&
                    label->bounds().height <= row_height + 0.01f,
                    "Independent Controls/Sticks fitted label crosses its value column or row height");
                require(!require_fitting || scale < 1.0f,
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
        MenuItem::aim_marker, MenuItem::reduce_effects, MenuItem::pad, MenuItem::move_deadzone, MenuItem::aim_deadzone })
        { old_labels[static_cast<size_t>(role)] = L"Detailed~ shared setting for this paint session and every connected controller"; }
    const std::array<const wchar_t*, 8> next_controls{ L"Sticks", L"Music gain", L"Effects gain", L"Camera shake",
        L"Aim cue", L"Less motion", L"Commit", L"Return" };
    const std::array<const wchar_t*, 5> next_sticks{ L"Slot", L"Movement cutoff", L"Aim cutoff", L"Commit", L"Return" };
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
        const std::string unapplied = saved_bytes();
        tap(Key::down); tap(Key::right);
        values[1] = sticks ? percent(std::min(0.9f, play.movement_deadzone(0, tuning.input().movement_deadzone) + 0.05f)) : L"10%";
        compare("pending-old-font", sticks, 1, old, old_labels, values, true);
        const size_t apply_row = sticks ? 3 : 6;
        for (size_t row = 1; row < apply_row; ++row) { tap(Key::down); }
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
            for (size_t row = 0; row < 6; ++row) { tap(Key::down); }
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
        const std::string before_discard = saved_bytes(); tap(Key::down); tap(Key::right);
        values[1] = sticks ? percent(std::min(0.9f, play.movement_deadzone(0, tuning.input().movement_deadzone) + 0.05f)) : L"20%";
        compare("pending-new-after-reversal", sticks, 1, next, new_labels, values);
        tap(Key::escape); tick();
        require(stack.depth() == (sticks ? 2 : 1) && saved_bytes() == before_discard,
            "Custom Controls/Sticks Escape persisted an unapplied edit or closed the wrong frame");
        reset_preferences(); presentation = original; open(sticks);
        require(compare("restored-default", sticks, 0, original, defaults, initial_values(sticks)).pixels == baseline[sticks ? 1 : 0].pixels,
            "Controls/Sticks source reversal and reopen did not restore the exact default raster");
    }
    require(records.good(), "Controls/Sticks authored item evidence write failed");
    std::cout << "PASS actual Controls/Sticks labels: 13 independent historical substitutions at six sizes/aspects; "
        "long OptionRow fitting, complete label/value focus cards and linear sampling; separate controls item font, "
        "retained/reopened/reversed snapshots, repeated child return/Apply, interrupted Enter, save reload, "
        "Back/Escape discard and parent Apply preserving child profiles; frozen draw RNG\n";
}

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
    const std::array<const wchar_t*, 8> labels{ L"Pad sticks", L"Music volume", L"SFX volume",
        L"View shake", L"Aim marker", L"Reduce effects", L"Apply", L"Back" };
    const std::array<const char*, 8> names{ "pad-sticks", "music", "sfx", "shake", "marker", "effects", "apply", "back" };
    const std::wstring help = L"CONTROLLER\nMove / drop: left stick\nAim: right stick    Shoot: RT\nJump: A / LT (hold for height)\nPause: Start    Debug: Back / View\nKEYBOARD + MOUSE\nMove: A / D    Drop: S / Down\nAim: mouse cursor    Shoot: left click\nJump: Space (hold for height)\nPause: Esc    Debug: F1\nMENUS\nNavigate: stick / D-pad / arrows\nSelect: A / Enter    Back: B / Esc\nAIM / READINESS\nDirection only; roller aim is fixed.\nReady: chevron / Cooldown: bars\nNo paint: cross\nPad sticks: movement / aim deadzones.\nREDUCE EFFECTS\nLess shake, flashes and idle detail.";
    const std::wstring pending_status = L"Apply to save changes for all players.";
    const auto measure = [&](const std::string& font, const std::wstring& text)
    {
        return app.render_resources()->measure_text(app.render_resources()->resolve_sprite_font(font), text);
    };
    const auto row_label = [&](size_t row)
    {
        return RectangleF(look.layout.position + Vector2F(0.0f,
            static_cast<float>(row) * look.layout.spacing.y), measure(look.item_font, labels[row]));
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
        require(panel.contains(card(row_label(row))) &&
            row_label(row).height + art.focus_padding.y * 2.0f < look.layout.spacing.y,
            "Controls/audio action card escapes paper or overlaps another row");
    }
    for (const auto& entry : std::array<std::pair<size_t, const wchar_t*>, 5>{ {
        { 1, L"100%" }, { 2, L"0%" }, { 3, L"Off" }, { 4, L"On" }, { 5, L"Off" } } })
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
        require(panel.contains(bounds) && card(row_label(7)).bottom() < bounds.y &&
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
    using Values = std::array<std::wstring, 6>;
    const Values initial{ L"", L"0%", L"0%", L"On", L"On", L"Off" };
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
            add(labels[row], look.item_font, row_label(row).position(), row == selected ? art.focus_ink : art.ink);
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
        tap(Key::up); std::ignore = assert_focus(main_menu_frame(app), row_label(6));
    }

    // Real volume edits at both clamps; the first action opens Pad sticks.
    // Values remain pending until Apply; SFX previews are silenced above.
    resize_page({ 1280, 720 }); open(std::make_unique<ControlsSettingsMenu>(&context));
    const std::string original = saved_bytes();
    Values values = initial;
    for (size_t row = 1; row <= 2; ++row)
    {
        tap(Key::down);
        for (int step = 0; step < 12; ++step) { tap(Key::right); }
        values[row] = L"100%";
        std::ignore = capture(row == 1 ? "music-max" : "sfx-max", row, values, pending_status, art.focus_ink);
        for (int step = 0; step < 12; ++step) { tap(Key::left); }
        values[row] = L"0%";
        lettering(main_menu_frame(app), row, values, pending_status, art.focus_ink);
    }
    tap(Key::down); tap(Key::left); values[3] = L"Off";
    tap(Key::down); tap(Key::left); values[4] = L"Off";
    tap(Key::down); tap(Key::right); values[5] = L"On";
    std::ignore = capture("pending", 5, values, pending_status, art.focus_ink);
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
    std::ignore = capture("applied", 6, values, L"Applied and saved for all players.", art.focus_ink);
    const std::string last_saved = saved_bytes();
    const std::filesystem::path blocked = save_path.wstring() + L".tmp";
    require(std::filesystem::create_directory(blocked), "Cannot block the isolated Controls/audio temporary save");
    tap(Key::up); tap(Key::left); values[5] = L"Off"; tap(Key::down); tap(Key::enter);
    require(!play.reduce_effects && saved_bytes() == last_saved,
        "Failed Controls/audio save lost its active choice or replaced previous JSON");
    std::ignore = capture("save-failed", 6, values, L"Active for this run. Could not save.",
        presentation.main_menu().options_warning_colour);
    require(std::filesystem::remove(blocked), "Cannot remove isolated Controls/audio write refusal");
    tap(Key::down); tap(Key::enter);
    require(stack.depth() == 1, "Controls/audio Back action did not pop its own page");
    open(std::make_unique<ControlsSettingsMenu>(&context)); tap(Key::down); tap(Key::right);
    for (int row = 1; row < 7; ++row) { tap(Key::down); }
    tap(Key::enter);
    require(stack.depth() == 1 && play.aim_sensitivity == 3 && saved_bytes() == last_saved,
        "Controls/audio Back action persisted an unapplied change");
    open(std::make_unique<ControlsSettingsMenu>(&context)); tap(Key::down); tap(Key::right); tap(Key::escape);
    require(stack.depth() == 1 && play.aim_sensitivity == 3 && saved_bytes() == last_saved,
        "Controls/audio Escape persisted an unapplied change");

    // The underlying Options instance keeps pending display values and cursor.
    open(std::make_unique<MainMenuOptions>(&context)); tap(Key::left);
    tap(Key::down); tap(Key::down); tap(Key::right); tap(Key::down);
    const Frame pending_options = main_menu_frame(app);
    tap(Key::enter); tick(); require(stack.depth() == 3, "Options did not push actual Controls/audio");
    const Frame controls_from_options = main_menu_frame(app);
    tap(Key::down); tap(Key::right);
    require(controls_from_options.pixels != main_menu_frame(app).pixels,
        "Controls/audio Options-return fixture did not stage its discardable edit");
    tap(Key::escape); tick(0.5f);
    require(stack.depth() == 2 && pending_options.pixels == main_menu_frame(app).pixels &&
        saved_bytes() == last_saved && play.aim_sensitivity == 3,
        "Controls/audio return lost pending Options values/focus or saved its own edits");

    // Actual GameLevel owns the Pause frame: returning from Controls/audio
    // must keep both the match simulation and the Resume count-in stopped.
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
    tap(Key::enter); tick(); require(stack.depth() == 4, "Pause did not push actual Controls/audio");
    const Frame controls_from_pause = main_menu_frame(app);
    tap(Key::enter); tick(); require(stack.depth() == 5, "Paused Controls/audio did not push Pad sticks");
    tap(Key::down); tap(Key::right); tick(0.75f); tap(Key::escape); tick(0.75f);
    require(stack.depth() == 4 && game->resume_remaining() == 0.0f &&
        frozen == resume_observation(game->level()) && controls_from_pause.pixels == main_menu_frame(app).pixels &&
        saved_bytes() == last_saved,
        "Pad sticks return changed/resumed the paused match, lost parent focus or saved a pending edit");
    tap(Key::down); tap(Key::right);
    require(controls_from_pause.pixels != main_menu_frame(app).pixels,
        "Controls/audio Pause-return fixture did not stage its discardable edit");
    tick(0.75f); tap(Key::escape); tick(0.75f);
    require(stack.depth() == 3 && game->resume_remaining() == 0.0f &&
        frozen == resume_observation(game->level()) && paused.pixels == main_menu_frame(app).pixels &&
        saved_bytes() == last_saved && play.aim_sensitivity == 3,
        "Controls/audio return resumed/changed the paused match, lost focus or saved pending edits");
    require(records.good(), "Controls/audio capture record write failed");
    std::cout << "PASS Controls/audio: complete fitted help, labels/values/status in linear ink; all eight cards/arrows, "
        "six native sizes/aspects; clamps, delayed Apply, reload/failure and Back discard; pending Options "
        "values/focus and actual GameLevel Pause return remain intact; frozen/reduced/random/resize\n";
    controls_menu_item_checks(app, canvas, tuning, presentation, output, save_path);
}
