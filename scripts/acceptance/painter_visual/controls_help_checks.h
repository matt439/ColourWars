// Help has three different placement contracts. The existing production
// matrices pin the complete historical text at their supported sizes; this
// fixture independently exercises authored copy and retained/reopened fonts.
void controls_menu_help_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output, const std::filesystem::path& save_path)
{
    using Help = decltype(MenuText{}.help);
    const Help legacy{
        L"CONTROLLER\nMove / drop: left stick\nAim: right stick    Shoot: RT\nJump: A / LT (hold for height)\n"
        L"Pause: Start    Debug: Back / View\nKEYBOARD + MOUSE\nMove: A / D    Drop: S / Down\n"
        L"Aim: mouse cursor    Shoot: left click\nJump: {jump} (hold for height)\nPause: Esc    Debug: F1\n"
        L"MENUS\nNavigate: stick / D-pad / arrows\nSelect: A / Enter    Back: B / Esc\nAIM / READINESS\n"
        L"Direction only; roller aim is fixed.\nReady: chevron / Cooldown: bars\nNo paint: cross\n"
        L"Pad sticks: movement / aim deadzones.\nREDUCE EFFECTS\nLess shake, flashes and idle detail.",
        L"Profiles follow Pad 1-4 slots,\nnot the player number or device.\n"
        L"After reconnecting, check the slot.\nKeyboard/mouse is unchanged.",
        L"Right stick: radial cutoff"
    };
    const Presentation original = presentation;
    Save preferences;
    const auto reset_preferences = [&]
    {
        PlaySettings fresh;
        fresh.music_volume = fresh.sfx_volume = 0;
        require(preferences.set_play_settings_and_save(fresh), "Cannot reset the isolated static-help save");
        require(preferences.set_input_bindings_and_save(InputBindings{}), "Cannot reset the isolated help Jump preference");
    };
    const auto saved_bytes = [&]
    {
        std::ifstream stream(save_path, std::ios::binary);
        require(stream.good(), "Static-help fixture lost its isolated save");
        return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    };
    reset_preferences();
    const auto expected_help = [&](const std::wstring& authored)
    {
        // Independent literal labels for the actual choices driven below.
        // Do not use the formatter under test to construct the raster oracle.
        const Key key = preferences.input_bindings().keyboard_jump;
        require(key == Key::space || key == Key::j, "Help fixture encountered an undeclared effective Jump key");
        const std::wstring label = key == Key::space ? L"Space" : L"J";
        const size_t token = authored.find(L"{jump}");
        require(token != std::wstring::npos && authored.find(L"{jump}", token + 6) == std::wstring::npos,
            "Independent help fixture lost its single authored Jump token");
        return authored.substr(0, token) + label + authored.substr(token + 6);
    };
    GameData data;
    data.set_application(&app); data.set_save(&preferences); data.set_tuning(&tuning);
    data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings);
    StateContext& stack = app;
    struct RestoreControlsHelp
    {
        StateContext& stack;
        Presentation& presentation;
        Presentation original;
        ~RestoreControlsHelp() { this->stack.clear(); this->presentation = this->original; }
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
            for (int slot = 0; slot < input_slots::pad_count; ++slot) { tap(Key::left); }
        }
        require(stack.depth() == (sticks ? 3 : 2), "Static-help fixture failed to open its production page");
    };
    std::ofstream records(output / "controls-menu-help.csv");
    records << "case,page,help,font,lines,raw_width,raw_height,scale,fitted_width,fitted_height,fixture_matches,jump_key\n";
    const auto compare = [&](const std::string& name, bool sticks, const Presentation& snapshot,
        const Help& words, bool require_controls_height = false, bool require_association_width = false,
        bool require_controls_width = false, bool require_association_height = false)
    {
        const auto& look = snapshot.controls_settings(); const auto& art = look.art;
        const Frame actual = main_menu_frame(app);
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        std::vector<std::unique_ptr<UiText>> text;
        std::vector<RectangleF> regions;
        std::vector<float> scales;
        std::vector<Vector2F> raw_sizes;
        std::vector<std::string> roles;
        const auto add = [&](const char* role, const std::wstring& copy, const Vector2F& position,
            const Vector2F& available, bool fitted, bool require_width, bool require_height)
        {
            auto widget = std::make_unique<UiText>("reference-static-help", copy, look.help_font,
                position, app.render_resources(), art.ink);
            const Vector2F raw = widget->bounds().size();
            const float width_scale = available.x / raw.x, height_scale = available.y / raw.y;
            const float scale = fitted ? std::min({ 1.0f, width_scale, height_scale }) : 1.0f;
            require(!require_width || (width_scale < 1.0f && width_scale < height_scale),
                "Authored static help failed to exercise width fitting");
            require(!require_height || (height_scale < 1.0f && height_scale < width_scale),
                "Authored static help failed to exercise multiline height fitting");
            widget->set_scale(scale);
            const RectangleF region(position, available);
            require(region.contains(widget->bounds()), "Independent static help escaped its authored region");
            if (!fitted)
            {
                require(widget->bounds().width > look.stick_preview.aim.width,
                    "Aim-caption fixture did not exercise the full help column beyond its diagram width");
            }
            // Keep the caption's full horizontal help column, but stop at its
            // measured line height: the next raster row belongs to the diagram.
            regions.push_back(fitted ? region : RectangleF(position, Vector2F(available.x, raw.y)));
            scales.push_back(scale); raw_sizes.push_back(raw); roles.push_back(role);
            text.push_back(std::move(widget));
        };
        if (sticks)
        {
            const auto& preview = look.stick_preview;
            const float label_height = app.render_resources()->measure_text(
                app.render_resources()->resolve_sprite_font(look.help_font), L"Move").y;
            const Vector2F caption = preview.aim.position() - Vector2F(0.0f, label_height + art.focus_padding.y);
            add("aim_cutoff", words.aim_cutoff, caption,
                Vector2F(look.controls_position.x + look.controls_size.x - caption.x,
                    preview.aim.y - caption.y), false, false, false);
            add("pad_association", words.pad_association, preview.association_position,
                look.controls_position + look.controls_size - preview.association_position,
                true, require_association_width, require_association_height);
        }
        else
        {
            add("controls", expected_help(words.controls), look.controls_position, look.controls_size,
                true, require_controls_width, require_controls_height);
        }
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app.viewport_manager()->fullscreen_viewport());
        list.set_camera(camera); list.set_filter(TextureFilter::point);
        const Canvas paper(app.render_resources(), art.paper);
        paper.draw(list, camera.visible_rectangle(app.viewport_manager()->fullscreen_viewport()));
        list.set_filter(TextureFilter::linear);
        for (const auto& widget : text) { widget->draw(list); }
        const Frame reference = read(renderer);
        for (size_t index = 0; index < text.size(); ++index)
        {
            RectangleF projected = regions[index]; camera.calculate_view_rectangle(projected);
            const RectangleI pixels = countdown_crop_bounds(projected);
            const Frame observed = foreground_crop(actual, pixels, 0);
            const Frame wanted = foreground_crop(reference, pixels, 0);
            const std::string stem = "controls-menu-help-" + name + '-' + roles[index] + '-' +
                std::to_string(actual.width) + 'x' + std::to_string(actual.height);
            if (observed.pixels != wanted.pixels)
            {
                save(observed, output / (stem + "-failure-actual.ppm"));
                save(wanted, output / (stem + "-failure-expected.ppm"));
                throw std::runtime_error("Actual Controls/Sticks static help differs from its full-region linear fixture");
            }
            save(observed, output / (stem + "-detail.ppm"));
            const std::wstring& copy = text[index]->text();
            records << name << ',' << (sticks ? "sticks" : "controls") << ',' << roles[index] << ',' <<
                look.help_font << ',' << std::count(copy.begin(), copy.end(), L'\n') + 1 << ',' <<
                raw_sizes[index].x << ',' << raw_sizes[index].y << ',' << scales[index] << ',' <<
                text[index]->bounds().width << ',' << text[index]->bounds().height << ",true," <<
                input_key_name(preferences.input_bindings().keyboard_jump) << '\n';
        }
        std::srand(6151); const int next_random = std::rand(); std::srand(6151);
        require(actual.pixels == main_menu_frame(app).pixels && std::rand() == next_random,
            "Static-help redraw changed frozen pixels or consumed random numbers");
        return actual;
    };
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    std::array<Frame, 2> baseline{};
    for (const bool sticks : { false, true })
    {
        presentation = original; open(sticks);
        baseline[sticks ? 1 : 0] = compare("legacy", sticks, original, legacy);
    }
    Help old_words = legacy, new_words = legacy;
    old_words.controls.clear();
    for (int line = 0; line < 40; ++line)
    {
        if (line != 0) { old_words.controls += L'\n'; }
        old_words.controls += line == 0 ? L"Controller~ jump {jump} movement and paint" : L"Controller~ movement and paint";
    }
    old_words.pad_association = L"Profiles~ follow every controller slot and keep the movement cutoff for that slot; check the association after reconnecting.\n"
        L"Keyboard and mouse keep their current controls.\nPad slots remain independent.\nApply saves the chosen cutoff.";
    old_words.aim_cutoff = L"Right stick~: radial cutoff";
    new_words.controls = L"CONTROLS: movement, aiming, paint and pause remain available with every current input device; use the menu to change shared options.\n"
        L"Jump: {jump}. Apply saves shared preferences. Back discards pending changes.";
    new_words.pad_association = L"Profiles follow slots.\nCheck the pad slot.\nMove cutoff is saved.\nAim cutoff is saved.\n"
        L"Apply commits edits.\nBack discards edits.\nReconnect then check.\nMouse is unchanged.";
    new_words.aim_cutoff = L"Aim stick: circular cutoff";
    Presentation old = original; const_cast<MenuText&>(old.menu_text()).help = old_words;
    old.validate_resources(*app.render_resources(), *app.audio_resources());
    const Font* source = app.render_resources()->font(
        app.render_resources()->resolve_sprite_font(original.controls_settings().help_font));
    std::vector<Glyph> glyphs;
    for (char32_t character = U' '; character < U'~'; ++character)
    {
        const Glyph* glyph = source->find(character);
        require(glyph != nullptr, "Replacement static-help font lacks a baseline printable glyph"); glyphs.push_back(*glyph);
    }
    const std::string font_name = "acceptance_controls_help_without_tilde";
    app.render_resources()->add_font(font_name,
        std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
    require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(font_name), L"~"),
        "Replacement static-help font still contains the retained page's old glyph");
    Presentation next = original; const_cast<MenuText&>(next.menu_text()).help = new_words;
    const_cast<ControlsSettingsPresentation&>(next.controls_settings()).help_font = font_name;
    const_cast<ControlsSettingsPresentation&>(next.controls_settings()).controls_size += Vector2F(10.0f, 10.0f);
    next.validate_resources(*app.render_resources(), *app.audio_resources());

    reset_preferences(); presentation = old; open(false);
    tap(Key::down); tap(Key::down); tap(Key::down); tap(Key::right); tap(Key::up); tap(Key::up); tap(Key::up);
    const Frame old_parent = compare("custom-old-parent", false, old, old_words, true);
    presentation = next; tick();
    require(old_parent.pixels == main_menu_frame(app).pixels, "Static-help reload changed the suspended Controls snapshot");
    for (int repeat = 0; repeat < 2; ++repeat)
    {
        const std::string before = saved_bytes();
        tap(Key::enter); tick();
        require(stack.depth() == 3, "Static-help parent did not open its new Sticks child");
        for (int slot = 0; slot < input_slots::pad_count; ++slot) { tap(Key::left); }
        compare("new-child-from-old-parent-" + std::to_string(repeat), true, next, new_words, false, false, false, true);
        tap(Key::right); compare("new-child-slot-refresh-" + std::to_string(repeat), true, next, new_words,
            false, false, false, true); tap(Key::left);
        tap(Key::down); tap(Key::right);
        compare("new-child-pending-refresh-" + std::to_string(repeat), true, next, new_words, false, false, false, true);
        if (repeat == 0)
        {
            tap(Key::down); tap(Key::down); tap(Key::down); // Aim, Rumble, Apply.
            const Frame before_interruption = main_menu_frame(app);
            app.keyboard()->on_key_down(Key::enter); tick();
            static_cast<WindowNotify&>(app).on_deactivated(); tick();
            static_cast<WindowNotify&>(app).on_activated(); tick();
            app.keyboard()->on_key_up(Key::enter); tick(); tick();
            require(saved_bytes() == before && before_interruption.pixels == main_menu_frame(app).pixels,
                "Interrupted child Apply changed its save or static-help snapshot");
            tap(Key::enter); const std::string applied = saved_bytes();
            require(applied != before, "Static-help child Apply did not persist its staged cutoff");
            tap(Key::enter); require(saved_bytes() == applied, "Repeated child Apply changed its saved cutoff");
            compare("new-child-applied", true, next, new_words, false, false, false, true);
            tap(Key::down); tap(Key::enter); tick();
        }
        else
        {
            tap(Key::escape); tick(); require(saved_bytes() == before, "Static-help child discard persisted an edit");
        }
        require(stack.depth() == 2 && old_parent.pixels == main_menu_frame(app).pixels,
            "Child return changed the old Controls help/font, pending choice or focus");
        compare("old-parent-after-child-" + std::to_string(repeat), false, old, old_words, true);
    }
    // A real sibling child applies a new effective key while the old Controls
    // page retains its original authored template, font and fitting bounds.
    tap(Key::down);
    const Frame old_before_jump = compare("old-parent-before-jump", false, old, old_words, true);
    tap(Key::enter); tick(); require(stack.depth() == 3, "Old Controls did not open its actual Keyboard Jump child");
    tap(Key::enter); tick(); // Capture observes a fresh neutral frame after entry.
    tap(Key::j); tick();
    require(preferences.input_bindings().keyboard_jump == Key::space,
        "Capturing a pending Jump key applied it without confirmation");
    tap(Key::down); tap(Key::down); tap(Key::enter); tick();
    require(preferences.input_bindings().keyboard_jump == Key::j,
        "Actual Keyboard Jump child did not apply its captured key");
    tap(Key::escape); tick(); require(stack.depth() == 2, "Keyboard Jump did not return to its retained Controls parent");
    const Frame old_after_jump = compare("old-parent-refreshed-jump", false, old, old_words, true);
    const Camera help_camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
        app.viewport_manager()->fullscreen_viewport());
    const RectangleF retained_help(old.controls_settings().controls_position, old.controls_settings().controls_size);
    const RectangleI permitted_delta = countdown_crop_bounds(help_camera.calculate_view_rectangle(retained_help));
    const RectangleI help_delta = foreground_delta_bounds(old_before_jump, old_after_jump);
    require(help_delta.width > 0 && help_delta.height > 0 && permitted_delta.contains(help_delta),
        "Returning with a new Jump key changed pixels outside the old parent's captured help bounds");
    presentation = old; tick();
    require(old_after_jump.pixels == main_menu_frame(app).pixels,
        "Source reversal changed the retained parent after its effective Jump refresh");
    presentation = next; tick();
    require(old_after_jump.pixels == main_menu_frame(app).pixels,
        "Live replacement changed the retained parent's refreshed template, font or fitting bounds");

    const int applied_cutoff = preferences.play_settings().sticks[0].movement_deadzone;
    for (int row = 1; row < 8; ++row) { tap(Key::down); }
    tap(Key::enter);
    Save reloaded; reloaded.load_save_file();
    require(reloaded.play_settings().music_volume == 10 &&
        reloaded.play_settings().sticks[0].movement_deadzone == applied_cutoff,
        "Old Controls Apply lost its pending choice or its newer child's applied cutoff");
    require(reloaded.input_bindings().keyboard_jump == Key::j,
        "Old Controls Apply overwrote its child's independent applied Jump preference");
    compare("old-parent-applied-after-child", false, old, old_words, true);

    for (const bool sticks : { false, true })
    {
        reset_preferences(); presentation = old; open(sticks);
        const Frame retained = compare("custom-old", sticks, old, old_words, !sticks, sticks);
        presentation = next; tick();
        tap(Key::down); if (!sticks) { tap(Key::down); tap(Key::down); }
        tap(Key::right); tap(Key::left); tap(Key::up); if (!sticks) { tap(Key::up); tap(Key::up); }
        require(retained.pixels == main_menu_frame(app).pixels,
            "Dynamic settings refresh rebased an old page's static help or font");
        compare("retained-old-after-refresh", sticks, old, old_words, !sticks, sticks);
        presentation = next; open(sticks);
        const Frame adopted = compare("reopened-new", sticks, next, new_words, false, false, !sticks, sticks);
        require(adopted.pixels != retained.pixels, "Reopened settings did not adopt new static help and help font");
        presentation = old; tick();
        tap(Key::down); if (!sticks) { tap(Key::down); tap(Key::down); }
        tap(Key::right); tap(Key::left); tap(Key::up); if (!sticks) { tap(Key::up); tap(Key::up); }
        require(adopted.pixels == main_menu_frame(app).pixels,
            "Source reversal or dynamic settings refresh rebased the reopened static-help snapshot");
        compare("retained-new-after-reversal", sticks, next, new_words, false, false, !sticks, sticks);
        const std::string before_discard = saved_bytes(); tap(Key::down); if (!sticks) { tap(Key::down); tap(Key::down); }
        tap(Key::right); tap(Key::escape); tick();
        require(stack.depth() == (sticks ? 2 : 1) && saved_bytes() == before_discard,
            "Static-help reload changed Escape discard or closed the wrong page");
        reset_preferences(); presentation = original; open(sticks);
        require(compare("restored-legacy", sticks, original, legacy).pixels == baseline[sticks ? 1 : 0].pixels,
            "Static-help reversal and reopen did not restore the exact historical raster");
    }
    require(records.good(), "Static-help native evidence write failed");
    std::cout << "PASS actual Controls/Sticks static help: independent complete historical copy; fitted Controls and "
        "association blocks exercise both width and multiline height limits, unscaled aim caption uses Move-based "
        "placement and the full help column; replacement help font lacks an old glyph, retained/reopened/reversed "
        "snapshots survive value/preview refresh, repeated child return/Apply, interrupted Enter and save/discard; "
        "actual Keyboard Jump return refreshes only the effective key in captured help/font/bounds and parent Apply preserves it; frozen draw RNG\n";
}
