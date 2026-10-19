// Setup pages retain their original shadow-union navigation bounds. Invisible
// shadows give paper lettering; the independent fixture pins complete artwork.
void menu_setup_text_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    struct Words
    {
        std::array<std::wstring, 8> items;
        std::wstring back;
        std::wstring hint;
    };
    const Words legacy{ { L"Single Match", L"Best of 3", L"Best of 5", L"Match length",
        L"1 Player (practice)", L"2 Players", L"3 Players", L"4 Players" }, L"Back",
        L"Solo practice: choose Single Match" };
    static_assert(static_cast<size_t>(MenuItem::resume) - static_cast<size_t>(MenuItem::single_match) == 8);
    const Presentation original = presentation;
    Save preferences;
    auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0;
    GameData data;
    data.set_application(&app); data.set_save(&preferences); data.set_tuning(&tuning);
    data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings);
    StateContext& stack = app;
    struct RestoreSetupText
    {
        StateContext& stack;
        Presentation& presentation;
        Presentation original;
        ~RestoreSetupText() { this->stack.clear(); this->presentation = this->original; }
    } restore{ stack, presentation, original };
    const auto tick = [&] { app.keyboard()->poll(); app.mouse()->poll(); stack.update(dt); };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick();
        // Mode/Count text fixtures compare settled focus accents.
        app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.51f);
    };
    const auto frame = [&]
    {
        app.renderer()->begin_frame(); stack.draw(*app.renderer()); return read(*app.renderer());
    };
    const auto open = [&](bool count, int best_of = 1)
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true);
        app.mouse()->set_focused(true);
        for (const Key key : { Key::enter, Key::escape, Key::up, Key::down, Key::left, Key::right })
            { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll();
        settings = MenuLevelSettings(); settings.set_best_of(best_of);
        input.reset_actions(); stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        if (count) { stack.push(std::make_unique<MainMenuPlayerCount>(&context)); }
        else { stack.push(std::make_unique<MainMenuModeSelect>(&context)); }
        tick(); tick();
        require(stack.depth() == 2, "Actual Mode/Player Count page did not remain open on neutral input");
    };
    const auto destination = [&](bool join)
    {
        MenuLevelSettings copy = settings;
        MenuInput reference_input(app.gamepads(), app.keyboard(), &tuning);
        MenuContext reference_context(&data, &reference_input, &copy);
        StateContext reference_stack;
        reference_stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        if (join) { reference_stack.push(std::make_unique<MainMenuJoin>(&reference_context)); }
        else { reference_stack.push(std::make_unique<MainMenuHome>(&reference_context)); }
        app.renderer()->begin_frame(); reference_stack.draw(*app.renderer()); return read(*app.renderer());
    };
    std::vector<RectangleF> text_bounds;
    const auto reference = [&](bool count, int focus, const Presentation& snapshot, const Words& words)
    {
        const auto& style = snapshot.menu(); const auto& look = snapshot.main_menu();
        auto art = look.art; art.focus_padding = look.setup_focus_padding; art.shadow_offset = look.setup_shadow_offset;
        const Vector2F spacing = count ? look.player_count_spacing : look.mode_select_spacing;
        const auto position = [&](int column, int row)
        {
            return style.layout.position + Vector2F(spacing.x * static_cast<float>(column),
                spacing.y * static_cast<float>(row));
        };
        std::vector<std::unique_ptr<UiTextDropShadow>> widgets;
        const auto add = [&](const std::wstring& text, const std::string& font,
            const Vector2F& where, const Colour& colour, const Vector2F& shadow)
        {
            auto widget = std::make_unique<UiTextDropShadow>("reference-setup-text", text, font, where,
                app.render_resources(), colour, Colour(0.0f, 0.0f, 0.0f, 0.0f), shadow);
            UiTextDropShadow* result = widget.get(); widgets.push_back(std::move(widget)); return result;
        };
        add(snapshot.menu_text().heading(count ? MenuHeading::players : MenuHeading::mode),
            style.heading_font, position(0, 0), art.ink, style.heading_shadow_offset);
        RectangleF focused;
        std::vector<RectangleF> actions;
        const int choices = count ? 4 : 3;
        for (int row = 0; row < choices; ++row)
        {
            const bool disabled = count && row == 0 && settings.best_of() > 1;
            const Colour colour = disabled ? art.disabled_ink : row == focus ? art.focus_ink : art.ink;
            const auto* label = add(words.items[static_cast<size_t>(row + (count ? 4 : 0))], style.item_font,
                position(0, row + 2), colour, style.item_shadow_offset);
            actions.push_back(label->bounds());
            if (row == focus) { focused = label->bounds(); }
        }
        if (!count)
        {
            const auto* label = add(words.items[3], style.item_font, position(0, 5),
                focus == 3 ? art.focus_ink : art.ink, style.item_shadow_offset);
            std::wostringstream duration;
            duration << settings.duration().value_or(tuning.match().duration) / 60.0f << L" min";
            const std::wstring value = settings.duration() ? duration.str() : L"Default (" + duration.str() + L")";
            const auto* time = add(value, style.item_font, position(2, 5), art.ink,
                style.item_shadow_offset);
            const RectangleF duration_bounds = RectangleF::union_of(label->bounds(), time->bounds());
            actions.push_back(duration_bounds);
            if (focus == 3) { focused = duration_bounds; }
            add(words.hint, style.detail_font, position(0, 7), art.ink,
                style.item_shadow_offset);
        }
        const auto* back = add(words.back, style.item_font, position(0, 6),
            focus == 4 ? art.focus_ink : art.ink, style.item_shadow_offset);
        actions.push_back(back->bounds());
        if (focus == 4) { focused = back->bounds(); }
        const auto& footer = snapshot.prompt_bar();
        const UiTextDropShadow prompt("reference-setup-prompt", snapshot.menu_text().prompt(
            count ? MenuPrompt::select_back : MenuPrompt::mode), footer.font, footer.position,
            app.render_resources(), footer.colour, footer.shadow_colour, footer.shadow_offset);
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
        DrawList list = renderer.view(0); list.set_viewport(viewport);
        list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        canvas.draw(list, RectangleF(Vector2F::ZERO, app.resolution_manager()->resolution_vec()));
        list.set_camera(camera);
        const SpriteSheet* sheet = app.render_resources()->sprite_sheet(style.sprite_sheet);
        const auto fill = sheet->resolve_sprite_frame(style.fill_frame);
        sheet->draw(list, fill, camera.visible_rectangle(viewport), style.scrim_colour);
        const UiTexture background("reference-setup-background", style.sprite_sheet, style.fill_frame,
            RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), app.render_resources(),
            look.home_background_colour);
        background.draw(list);
        const RectangleF paper((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - look.setup_box_size) / 2.0f,
            look.setup_box_size);
        for (RectangleF bounds : actions)
        {
            bounds.inflate(art.focus_padding);
            require(paper.contains(bounds), "Independent setup action/card escapes the paper rectangle");
        }
        for (const auto& widget : widgets)
            { require(paper.contains(widget->bounds()), "Independent setup text escapes its paper rectangle"); }
        setup_paper_reference(list, *app.render_resources(), style, art, paper, focused);
        sheet->draw(list, fill, RectangleF(0.0f, style.footer_y, menu_consts::DESIGN_RESOLUTION.x,
            menu_consts::DESIGN_RESOLUTION.y - style.footer_y), style.scrim_colour);
        sheet->draw(list, fill, RectangleF(style.brand_position.x, style.footer_y,
            menu_consts::DESIGN_RESOLUTION.x - style.brand_position.x * 2.0f, style.rule_height),
            style.panel_edge_colour);
        list.set_filter(TextureFilter::linear); text_bounds.clear();
        for (const auto& widget : widgets) { widget->draw(list); text_bounds.push_back(widget->bounds()); }
        prompt.draw(list); return read(renderer);
    };
    std::ofstream records(output / "menu-setup-text.csv");
    records << "case,page,width,height,focus,best_of,duration_override,full_frame_matches\n";
    const auto compare = [&](const std::string& name, bool count, int focus,
        const Presentation& snapshot, const Words& words)
    {
        const Frame actual = frame(); const Frame expected = reference(count, focus, snapshot, words);
        const std::string stem = "menu-text-setup-" + name + (count ? "-count-" : "-mode-") +
            std::to_string(actual.width) + 'x' + std::to_string(actual.height);
        if (actual.pixels != expected.pixels)
        {
            save(actual, output / (stem + "-failure-actual.ppm"));
            save(expected, output / (stem + "-failure-expected.ppm"));
            const RectangleI delta = foreground_delta_bounds(actual, expected);
            std::cerr << "Menu setup case=" << name << " count=" << count << " focus=" << focus <<
                " best_of=" << settings.best_of() << " delta=(" << delta.x << ',' << delta.y << ',' <<
                delta.width << ',' << delta.height << ")\n";
            throw std::runtime_error("Actual Mode/Player Count differs from the independent setup text fixture");
        }
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        for (RectangleF bounds : text_bounds)
        {
            camera.calculate_view_rectangle(bounds); const RectangleI pixels = countdown_crop_bounds(bounds);
            float darkest = 1.0f, lightest = 0.0f; size_t ink = 0, pale = 0;
            for (int y = pixels.y; y < pixels.bottom(); ++y)
            {
                for (int x = pixels.x; x < pixels.right(); ++x)
                {
                    const float luminance = hud_luminance(actual, hud_pixel_offset(actual, x, y));
                    darkest = std::min(darkest, luminance); lightest = std::max(lightest, luminance);
                    if (luminance < 0.2f) { ++ink; } if (luminance > 0.5f) { ++pale; }
                }
            }
            require(ink > 2 && pale > 2 && (lightest + 0.05f) / (darkest + 0.05f) >= 4.5f,
                "Actual setup text lost dark ink, pale paper or native raster contrast");
        }
        const auto& look = snapshot.main_menu();
        const RectangleF crop((Vector2F(menu_consts::DESIGN_RESOLUTION.x, snapshot.menu().footer_y) -
            look.setup_box_size) / 2.0f, look.setup_box_size);
        save(foreground_crop(actual, countdown_crop_bounds(camera.calculate_view_rectangle(crop)), 0),
            output / (stem + "-detail.ppm"));
        if (actual.width == 1280 && actual.height == 720)
        {
            if (!count && name == "default-focus-0") { save(actual, output / "setup-paper-mode-overview-1280x720.ppm"); }
            if (!count && name == "duration-focused") { save(actual, output / "setup-paper-duration-overview-1280x720.ppm"); }
            if (count && name == "series-solo-disabled-1") { save(actual, output / "setup-paper-series-count-overview-1280x720.ppm"); }
        }
        std::srand(7109); const int next_random = std::rand(); std::srand(7109);
        require(actual.pixels == frame().pixels && std::rand() == next_random,
            "Frozen setup-page drawing changed pixels or consumed random numbers");
        records << name << ',' << (count ? "count" : "mode") << ',' << actual.width << ',' << actual.height << ',' <<
            focus << ',' << settings.best_of() << ',' << settings.duration().value_or(-1.0f) << ",true\n";
        return actual;
    };
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440),
        Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        for (bool count : { false, true })
        {
            open(count);
            for (int focus = 0; focus < 5; ++focus)
            {
                if (focus > 0) { tap(Key::down); }
                compare("default-focus-" + std::to_string(focus), count, focus, original, legacy);
            }
        }
        open(true, 3);
        for (int focus = 1; focus < 5; ++focus)
        {
            if (focus > 1) { tap(Key::down); }
            compare("series-solo-disabled-" + std::to_string(focus), true, focus, original, legacy);
        }
    }
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    open(false);
    for (int move = 0; move < 3; ++move) { tap(Key::down); }
    compare("duration-focused", false, 3, original, legacy);
    const auto& choices = tuning.match().duration_choices;
    require(!choices.empty(), "Setup duration-cycle fixture requires authored choices");
    for (size_t index = 0; index < choices.size(); ++index)
    {
        tap(Key::right); require(settings.duration() == std::optional<float>(choices[index]),
            "Mode duration Right changed its existing authored cycle");
        compare("duration-" + std::to_string(index), false, 3, original, legacy);
    }
    tap(Key::right); require(!settings.duration(), "Mode duration Right did not wrap to the tuned default");
    tap(Key::left); require(settings.duration() == std::optional<float>(choices.back()),
        "Mode duration Left did not wrap to the last authored choice");
    tap(Key::enter); require(!settings.duration(), "Confirming Match length did not keep its ordinary right cycle");
    compare("duration-confirm-wrap", false, 3, original, legacy);
    for (int mode = 0; mode < 3; ++mode)
    {
        open(false); for (int move = 0; move < mode; ++move) { tap(Key::down); }
        tap(Key::enter); require(settings.best_of() == mode * 2 + 1, "Mode choice changed its series length");
        compare("mode-choice-" + std::to_string(mode), true, mode == 0 ? 0 : 1, original, legacy);
        if (mode != 0)
        {
            // The Count constructor disarms directions. Enter's release
            // transitioned into it, so let this page observe neutral first.
            const Frame entered = frame(); tick();
            require(frame().pixels == entered.pixels, "Neutral Count entry changed its initial series focus");
            tap(Key::up); compare("series-skips-solo-" + std::to_string(mode), true, 4, original, legacy);
            tap(Key::down); compare("series-wraps-to-two-" + std::to_string(mode), true, 1, original, legacy);
        }
        tap(Key::escape); compare("count-back-mode-" + std::to_string(mode), false, mode, original, legacy);
    }
    constexpr std::array<ScreenLayout, 4> layouts{ ScreenLayout::one_player, ScreenLayout::two_player,
        ScreenLayout::three_player, ScreenLayout::four_player };
    for (int count = 1; count <= 4; ++count)
    {
        open(true); for (int move = 1; move < count; ++move) { tap(Key::down); }
        tap(Key::enter);
        require(settings.player_count() == count && settings.screen_layout() == layouts[static_cast<size_t>(count - 1)] &&
            settings.player_settings()[0].num == input_slots::keyboard_and_mouse &&
            (count != 1 || settings.player_team(0) == PlayerTeam::a),
            "Player Count changed its roster, screen layout, keyboard claim or solo team");
        for (int seat = 1; seat < count; ++seat)
            { require(settings.player_settings()[static_cast<size_t>(seat)].num == -1, "Player Count preclaimed another device"); }
        require(frame().pixels == destination(true).pixels, "Player Count did not reach actual Join Players");
        tap(Key::escape); const Frame returned = compare("join-back-count-" + std::to_string(count), true, count - 1, original, legacy);
        tick(); require(frame().pixels == returned.pixels, "Neutral Join return changed initial Count focus");
    }
    // The last Join return retains Four Players, directly above Back.
    tap(Key::down);
    tap(Key::enter); const Frame returned_mode = compare("count-back-button", false, 0, original, legacy);
    tick(); require(frame().pixels == returned_mode.pixels, "Neutral Count Back changed initial Mode focus");
    for (int move = 0; move < 4; ++move) { tap(Key::down); }
    tap(Key::enter); require(frame().pixels == destination(false).pixels, "Mode Back button did not reach actual Home");
    open(false); tap(Key::escape);
    require(frame().pixels == destination(false).pixels, "Mode Escape did not reach actual Home");

    const Words old_words{ { L"Single~", L"Three~", L"Five~", L"Length~", L"Solo~",
        L"Pair~", L"Trio~", L"Four~" }, L"Back~", L"Solo~ choose the first mode" };
    const Words new_words{ { L"One match", L"Three rounds", L"Five rounds", L"Time", L"Practice",
        L"Pair", L"Trio", L"Four" }, L"Return", L"Practice uses the first mode" };
    const auto authored = [&](const Words& words)
    {
        Presentation result = original; auto& text = const_cast<MenuText&>(result.menu_text());
        for (size_t index = 0; index < words.items.size(); ++index)
            { text.items[static_cast<size_t>(MenuItem::single_match) + index] = words.items[index]; }
        text.items[static_cast<size_t>(MenuItem::back)] = words.back; text.solo_practice_hint = words.hint;
        return result;
    };
    Presentation old = authored(old_words), next = authored(new_words);
    const auto narrow_font = [&](const std::string& original_name, const std::string& name)
    {
        const Font* source = app.render_resources()->font(app.render_resources()->resolve_sprite_font(original_name));
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character < U'~'; ++character)
        {
            const Glyph* glyph = source->find(character);
            require(glyph != nullptr, "Setup font fixture lacks a baseline printable glyph"); glyphs.push_back(*glyph);
        }
        app.render_resources()->add_font(name,
            std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
        require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(name), L"~"),
            "Setup replacement font retained the old text's glyph");
    };
    narrow_font(original.menu().item_font, "acceptance_setup_item_without_tilde");
    narrow_font(original.menu().detail_font, "acceptance_setup_detail_without_tilde");
    auto& next_style = const_cast<MenuStyle&>(next.menu());
    next_style.item_font = "acceptance_setup_item_without_tilde";
    next_style.detail_font = "acceptance_setup_detail_without_tilde";
    old.validate_resources(*app.render_resources(), *app.audio_resources());
    next.validate_resources(*app.render_resources(), *app.audio_resources());
    presentation = old; open(false); const Frame old_mode = compare("custom-open", false, 0, old, old_words);
    presentation = next; tick(); require(frame().pixels == old_mode.pixels, "Live setup text/font replacement changed an old Mode page");
    for (int move = 0; move < 3; ++move) { tap(Key::down); }
    tap(Key::right); require(settings.duration() == std::optional<float>(choices[0]), "Old Mode duration behavior changed after text replacement");
    compare("old-mode-duration-after-replacement", false, 3, old, old_words);
    for (int move = 0; move < 3; ++move) { tap(Key::up); }
    tap(Key::enter); compare("reopened-new-count", true, 0, next, new_words);
    tap(Key::escape); const Frame new_mode = compare("returned-new-mode", false, 0, next, new_words);
    require(settings.duration() == std::optional<float>(choices[0]), "Mode child return lost the chosen duration");
    presentation = original; tick(); require(frame().pixels == new_mode.pixels, "Setup reversal changed the reopened Mode snapshot");
    for (int move = 0; move < 3; ++move) { tap(Key::down); }
    tap(Key::left); require(!settings.duration(), "Reopened Mode duration Left changed its existing cycle");
    compare("new-mode-duration-after-reversal", false, 3, next, new_words);
    presentation = old; open(true, 3); const Frame old_count = compare("custom-series-count", true, 1, old, old_words);
    presentation = next; tick(); require(frame().pixels == old_count.pixels, "Live setup replacement changed an old Count page");
    tap(Key::up); compare("old-count-skips-solo", true, 4, old, old_words); tap(Key::down);
    require(frame().pixels == old_count.pixels, "Count focus movement rebased its cached item font/text");
    tap(Key::enter); require(settings.player_count() == 2 && frame().pixels == destination(true).pixels,
        "Old series Count failed its ordinary two-player Join transition after replacement");
    tap(Key::escape); const Frame new_count = compare("returned-new-series-count", true, 1, next, new_words);
    presentation = original; tick(); require(frame().pixels == new_count.pixels, "Setup reversal changed the reopened Count snapshot");
    tap(Key::up); compare("new-count-after-reversal", true, 4, next, new_words); tap(Key::down);
    require(frame().pixels == new_count.pixels, "Reversal/focus changed cached Count item text or font");
    open(false); compare("restored-default", false, 0, original, legacy);
    open(true, 3); compare("restored-default-series", true, 1, original, legacy);
    require(records.good(), "Setup menu text evidence write failed");
    std::cout << "PASS actual Mode/Player Count: eight independent legacy items and solo hint across six sizes/aspects; "
        "duration cycles, three mode choices, solo-disabled series, all count/layout/claim and Back routes; "
        "custom item/detail fonts, retained/reopened/reversed snapshots and duration/child return, frozen draw RNG\n";
}
