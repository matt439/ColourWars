RectangleF stage_paper_surface_reference(DrawList& list, RenderResources& resources,
    const Presentation& captured, const Presentation& fitted, const MenuLevelSettings& settings,
    bool random, bool locked, size_t eligible_count, float accent_offset);

// Compare the real paper-page labels with separately rendered legacy/custom
// text, including the stepped paper and focus-card bevel beneath each crop.
void menu_item_page_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    constexpr std::array<const wchar_t*, 10> legacy{ L"Play", L"Options", L"Credits", L"Exit",
        L"Resolution", L"Fullscreen", L"Team palette", L"Controls / audio", L"Apply", L"Back" };
    static_assert(legacy.size() == static_cast<size_t>(MenuItem::single_match));
    constexpr std::array<MenuItem, 7> options_items{ MenuItem::resolution, MenuItem::fullscreen,
        MenuItem::team_palette, MenuItem::controls_audio, MenuItem::restore_defaults, MenuItem::apply, MenuItem::back };
    using ItemLabels = std::array<std::wstring, static_cast<size_t>(MenuItem::count)>;
    const Presentation original = presentation;
    ItemLabels defaults = original.menu_text().items;
    for (size_t index = 0; index < legacy.size(); ++index) { defaults[index] = legacy[index]; }
    defaults[static_cast<size_t>(MenuItem::restore_defaults)] = L"Restore defaults";
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
    struct RestoreMenuItems
    {
        StateContext& stack;
        Presentation& presentation;
        Presentation original;
        ~RestoreMenuItems() { this->stack.clear(); this->presentation = this->original; }
    } restore{ stack, presentation, original };
    const auto tick = [&] { app.keyboard()->poll(); app.mouse()->poll(); stack.update(dt); };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick();
        // This static paper/text snapshot fixture observes resting ornaments.
        app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.51f);
    };
    const auto frame = [&]
    {
        app.renderer()->begin_frame(); stack.draw(*app.renderer()); return read(*app.renderer());
    };
    const auto open = [&](int page)
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true);
        app.mouse()->set_focused(true);
        for (const Key key : { Key::enter, Key::escape, Key::up, Key::down, Key::left, Key::right })
            { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll();
        input.reset_actions(); stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        if (page == 0) { stack.push(std::make_unique<MainMenuHome>(&context)); }
        else if (page == 1) { stack.push(std::make_unique<MainMenuOptions>(&context)); }
        else { stack.push(std::make_unique<MainMenuCredits>(&context)); }
        tick(); tick();
        require(stack.depth() == 2, "Actual static-item page did not remain open on neutral input");
    };
    std::ofstream records(output / "menu-items.csv");
    records << "case,page,item,width,height,label_scale,label_right,value_left,fixture_matches\n";
    const auto compare = [&](const std::string& name, int page, const Presentation& snapshot,
        const ItemLabels& labels, bool require_fitting = false, int focused_home_row = 0)
    {
        const auto& style = snapshot.menu(); const auto& look = snapshot.main_menu();
        MenuPaperArtPresentation art = look.art;
        if (page == 1)
        {
            art.focus_padding = look.setup_focus_padding;
            art.shadow_offset = look.setup_shadow_offset;
        }
        const Frame actual = frame();
        const int rows = page == 0 ? 4 : page == 1 ? static_cast<int>(options_items.size()) : 1;
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        for (int row = 0; row < rows; ++row)
        {
            const size_t item = page == 0 ? static_cast<size_t>(row) :
                page == 1 ? static_cast<size_t>(options_items[static_cast<size_t>(row)]) : static_cast<size_t>(MenuItem::back);
            const Vector2F position = page == 0 ? look.home_position +
                Vector2F(0.0f, look.home_spacing.y * static_cast<float>(row + 2)) :
                page == 1 ? look.options_position + Vector2F(0.0f, look.options_spacing.y * static_cast<float>(row)) :
                look.credits_back_position;
            const bool focused = row == (page == 0 ? focused_home_row : 0);
            UiText expected("reference-item", labels[item], style.item_font, position, app.render_resources(),
                focused ? art.focus_ink : art.ink);
            float scale = 1.0f;
            if (page == 1 && row < 3)
            {
                const RectangleF bounds = expected.bounds();
                const float width = look.options_value_x - position.x - art.focus_padding.x;
                const float height = look.options_spacing.y - art.focus_padding.y * 2.0f;
                scale = std::min({ 1.0f, width / bounds.width, height / bounds.height });
                expected.set_scale(scale);
                require(expected.bounds().right() <= look.options_value_x - art.focus_padding.x + 0.01f &&
                    expected.bounds().height <= height + 0.01f,
                    "Independent fitted Options label crosses its value column or row height");
                require(!require_fitting || scale < 1.0f,
                    "Long custom Options label did not exercise real column fitting");
            }
            Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
            DrawList list = renderer.view(0); list.set_viewport(app.viewport_manager()->fullscreen_viewport());
            list.set_camera(camera); list.set_filter(TextureFilter::point);
            const Canvas background(app.render_resources(), look.home_background_colour);
            background.draw(list, camera.visible_rectangle(app.viewport_manager()->fullscreen_viewport()));
            const SpriteSheet* sheet = app.render_resources()->sprite_sheet(style.sprite_sheet);
            const auto solid_frame = sheet->resolve_sprite_frame(style.fill_frame);
            const auto fill = [&](const RectangleF& bounds, const Colour& colour)
            {
                sheet->draw(list, solid_frame, bounds,
                    Colour(colour.r * colour.a, colour.g * colour.a, colour.b * colour.a, colour.a));
            };
            const auto card = [&](const RectangleF& bounds, const Colour& colour)
            {
                const float pen = art.keyline_width, step = pen * 2.0f, shadow = art.shadow_offset;
                const RectangleF face(bounds.x, bounds.y, bounds.width - shadow, bounds.height - shadow);
                const auto stepped = [&](const RectangleF& box, const Colour& tint)
                {
                    fill(RectangleF(box.x + step, box.y, box.width - step * 2.0f, box.height), tint);
                    fill(RectangleF(box.x, box.y + step, box.width, box.height - step * 2.0f), tint);
                };
                stepped(RectangleF(face.position() + Vector2F(shadow), face.size()), art.shade);
                stepped(face, art.ink);
                const RectangleF inside(face.x + pen, face.y + pen,
                    face.width - pen * 2.0f, face.height - pen * 2.0f);
                stepped(inside, colour);
                for (int pass = 3; pass > 0; --pass)
                {
                    Colour shade = art.shade; shade.a *= 0.12f;
                    const float width = pen * static_cast<float>(pass);
                    fill(RectangleF(inside.x + step, inside.bottom() - width,
                        inside.width - step * 2.0f, width), shade);
                    fill(RectangleF(inside.right() - width, inside.y + step,
                        width, inside.height - step * 2.0f), shade);
                }
            };
            const Vector2F panel_size = page == 0 ? look.home_box_size :
                page == 1 ? look.options_box_size : look.credits_box_size;
            const RectangleF panel((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) -
                panel_size) / 2.0f, panel_size);
            card(panel, art.paper);
            if (focused || (page == 0 && row == 0))
            {
                RectangleF focus_bounds = expected.bounds();
                if (page == 1)
                {
                    // The real compound row owns its label AND value. A long
                    // label can shrink below the unchanged value's height.
                    UiText value("reference-resolution-value", L"1920x1080", style.item_font,
                        Vector2F(look.options_value_x, position.y), app.render_resources(), art.ink);
                    const RectangleF bounds = value.bounds();
                    const float width = panel.right() - art.shadow_offset - art.focus_padding.x -
                        look.options_value_x;
                    const float height = look.options_spacing.y - art.focus_padding.y * 2.0f;
                    value.set_scale(std::min({ 1.0f, width / bounds.width, height / bounds.height }));
                    focus_bounds = RectangleF::union_of(focus_bounds, value.bounds());
                }
                focus_bounds.inflate(art.focus_padding);
                card(focus_bounds, art.focus_fill);
            }
            list.set_filter(TextureFilter::linear); expected.draw(list);
            const Frame reference = read(renderer);
            RectangleF crop = expected.bounds(); crop.inflate(Vector2F(2.0f));
            camera.calculate_view_rectangle(crop);
            const RectangleI pixels = countdown_crop_bounds(crop);
            const Frame observed = foreground_crop(actual, pixels, 0);
            const Frame wanted = foreground_crop(reference, pixels, 0);
            const std::string stem = "menu-text-items-" + name + '-' + std::to_string(page) + '-' +
                std::to_string(item) + '-' + std::to_string(actual.width) + 'x' + std::to_string(actual.height);
            if (observed.pixels != wanted.pixels)
            {
                save(observed, output / (stem + "-failure-actual.ppm"));
                save(wanted, output / (stem + "-failure-expected.ppm"));
                const RectangleI delta = foreground_delta_bounds(observed, wanted);
                std::cerr << "Menu item case=" << name << " page=" << page << " item=" << item <<
                    " scale=" << scale << " crop=(" << pixels.x << ',' << pixels.y << ',' << pixels.width << ',' <<
                    pixels.height << ") local_delta=(" << delta.x << ',' << delta.y << ',' << delta.width << ',' <<
                    delta.height << ")\n";
                throw std::runtime_error("Actual Home/Options/Credits label differs from its independent item fixture");
            }
            save(observed, output / (stem + "-detail.ppm"));
            records << name << ',' << page << ',' << item << ',' << actual.width << ',' << actual.height << ',' <<
                scale << ',' << expected.bounds().right() << ',' << (page == 1 && row < 3 ? look.options_value_x : 0.0f) <<
                ",true\n";
        }
        std::srand(6287); const int next_random = std::rand(); std::srand(6287);
        require(actual.pixels == frame().pixels && std::rand() == next_random,
            "Static menu item drawing changed pixels or consumed random numbers");
        return actual;
    };
    std::array<Frame, 3> baseline{};
    Frame credits_home;
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        for (int page = 0; page < 3; ++page)
        {
            open(page); const Frame actual = compare("default", page, original, defaults);
            if (size.x == 1280) { baseline[static_cast<size_t>(page)] = actual; }
            if (page == 0 && size.x == 1280)
            {
                tap(Key::down); tap(Key::down);
                credits_home = compare("default-home-credits", 0, original, defaults, false, 2);
            }
        }
    }
    const std::array<const wchar_t*, 10> old_prefix{ L"Play~", L"Options~", L"Credits~", L"Exit~",
        L"Detailed~ output resolution for every studio display and presentation surface",
        L"Choose~ whether this studio display occupies its whole available desktop",
        L"Choose~ the two team colours used throughout this paint shooting session",
        L"Controls~", L"Apply~", L"Back~" };
    ItemLabels old_labels = defaults;
    for (size_t index = 0; index < old_prefix.size(); ++index) { old_labels[index] = old_prefix[index]; }
    old_labels[static_cast<size_t>(MenuItem::restore_defaults)] = L"Restore~ defaults";
    Presentation old = original;
    const_cast<MenuText&>(old.menu_text()).items = old_labels;
    old.validate_resources(*app.render_resources(), *app.audio_resources());
    const Font* source = app.render_resources()->font(
        app.render_resources()->resolve_sprite_font(original.menu().item_font));
    std::vector<Glyph> glyphs;
    for (char32_t character = U' '; character < U'~'; ++character)
    {
        const Glyph* glyph = source->find(character);
        require(glyph != nullptr, "Static-item font fixture lacks a baseline printable glyph");
        glyphs.push_back(*glyph);
    }
    const std::string narrow_name = "acceptance_menu_items_without_tilde";
    app.render_resources()->add_font(narrow_name,
        std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
    require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(narrow_name), L"~"),
        "Static-item replacement font still contains the old labels' glyph");
    const std::array<const wchar_t*, 10> new_prefix{ L"Start", L"Settings", L"Thanks", L"Leave", L"Display size",
        L"Fill display", L"Team colours", L"Audio and controls", L"Commit", L"Return" };
    ItemLabels new_labels = defaults;
    for (size_t index = 0; index < new_prefix.size(); ++index) { new_labels[index] = new_prefix[index]; }
    new_labels[static_cast<size_t>(MenuItem::restore_defaults)] = L"Reset choices";
    Presentation next = original;
    const_cast<MenuText&>(next.menu_text()).items = new_labels;
    const_cast<MenuStyle&>(next.menu()).item_font = narrow_name;
    next.validate_resources(*app.render_resources(), *app.audio_resources());
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    for (int page = 0; page < 3; ++page)
    {
        presentation = old; open(page); const Frame old_frame = compare("custom-long", page, old, old_labels, true);
        presentation = next; tick();
        require(frame().pixels == old_frame.pixels, "Live item/font replacement rebased an existing static page");
        if (page < 2) { tap(Key::down); tap(Key::up); }
        require(frame().pixels == old_frame.pixels, "Focus movement rebased old labels into the replacement item font");
        compare("old-page-after-replacement", page, old, old_labels, true);
        open(page); const Frame new_frame = compare("reopened-new-font", page, next, new_labels);
        require(new_frame.pixels != old_frame.pixels, "Reopened static page did not adopt new item labels and font");
        presentation = original; tick();
        require(frame().pixels == new_frame.pixels, "Item/font reversal changed an already reopened static page");
        if (page < 2) { tap(Key::down); tap(Key::up); }
        require(frame().pixels == new_frame.pixels, "Focus movement after reversal changed cached new labels/font");
        compare("new-page-after-reversal", page, next, new_labels);
        open(page); const Frame restored = compare("restored-default", page, original, defaults);
        require(restored.pixels == baseline[static_cast<size_t>(page)].pixels,
            "Reopened static page did not restore the exact default raster");
        if (page == 2)
        {
            tap(Key::enter); compare("credits-select-home", 0, original, defaults, false, 2);
            require(frame().pixels == credits_home.pixels,
                "Credits Select did not restore the exact Home Credits row");
        }
    }
    require(records.good(), "Static menu item evidence write failed");
    std::cout << "PASS actual Home/Options/Credits: eleven independent item roles at 720p/1080p/QHD, "
        "long fitted Options labels, custom item/font reopen and exact retained/reversed snapshots, focus, "
        "Credits Select return and frozen draw RNG\n";
}

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
        tap(Key::escape); compare("count-back-mode-" + std::to_string(mode), false, 0, original, legacy);
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
        tap(Key::escape); const Frame returned = compare("join-back-count-" + std::to_string(count), true, 0, original, legacy);
        tick(); require(frame().pixels == returned.pixels, "Neutral Join return changed initial Count focus");
    }
    for (int move = 0; move < 4; ++move) { tap(Key::down); }
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

// Shared text is selected by a fixed role. The independent literals below pin
// the shipped wording/spacing while actual page matrices cover their routing.
void stage_random_text_checks(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output);
void stage_briefing_text_checks(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output);
void stage_cue_text_checks(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output);

void menu_text_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    const std::wstring brand = L"C O L O U R W A R S   /   LOCAL PAINT BATTLES";
    constexpr std::array<const wchar_t*, 11> headings{ L"Make your mark.", L"Options", L"Credits",
        L"Controls / audio", L"Pad sticks", L"Number of Players", L"Join Players", L"Mode Select",
        L"Team Select", L"Weapon Select", L"Choose your arena" };
    constexpr std::array<const wchar_t*, 17> prompts{
        L"A/Enter Start", L"A/Enter Select    B/Esc Back",
        L"Left/Right Change    A/Enter Select    B/Esc Back",
        L"Left/Right Change    A/Enter Select    B/Esc Back (discard unapplied changes)",
        L"D-pad/arrows Navigate / change    A/Enter Select    B/Esc Back (discard unapplied changes)",
        L"A/Enter Select    Left/Right Match length    B/Esc Back",
        L"A/Enter Join / P1 continue    B/Esc Leave / P1 back",
        L"Left/Right Pick a team    A/Enter Confirm    B/Esc Cancel",
        L"Left/Right Pick a team    A/Enter Confirm    B/Esc Cancel    X/R Random teams",
        L"Left/Right Change weapon    A/Enter Confirm    B/Esc Cancel",
        L"Left/Right Browse arenas    A/Enter Lock arena    B/Esc Back",
        L"A/Enter Start match    B/Esc Unlock arena", L" Reconnect controller    B/Esc Change players",
        L"A/Enter Select    B/Start/Esc Resume", L" Reconnect pads to resume    A/Enter Select",
        L"A/Enter Select", L"A/Enter Back    B/Esc Back"
    };
    static_assert(headings.size() == static_cast<size_t>(MenuHeading::count));
    static_assert(prompts.size() == static_cast<size_t>(MenuPrompt::count));
    class MenuTextProbe final : public MenuPage
    {
    public:
        MenuTextProbe(MenuContext* context, MenuHeading heading) : MenuPage(context), heading_(heading) {}
        void init() override
        {
            this->texture_container_ = std::make_unique<UiContainer>("textures");
            this->text_container_ = std::make_unique<UiContainer>("text");
            const MenuStyle& style = this->menu_style();
            this->heading_text_ = std::make_unique<UiTextDropShadow>("heading",
                this->text().heading(this->heading_), style.heading_font, style.layout.position,
                this->render_resources(), style.heading_colour, style.shadow_colour,
                style.heading_shadow_offset);
            this->text_container_->add_child(this->heading_text_.get());
            this->set_prompts(MenuPrompt::select_back);
        }
        void update_page(float /*dt*/) override {}
        void rebuild(MenuPrompt prompt) { this->set_prompts(prompt); }
    private:
        MenuHeading heading_;
        std::unique_ptr<UiTextDropShadow> heading_text_;
    };

    const Presentation original = presentation;
    Save preferences;
    auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0;
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false } };
    GameData data;
    data.set_application(&app); data.set_save(&preferences); data.set_level_infos(&levels);
    data.set_stages(&stages); data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings);
    StateContext& stack = app;
    struct RestoreMenuText
    {
        StateContext& stack;
        Presentation& presentation;
        Presentation original;
        ~RestoreMenuText() { this->stack.clear(); this->presentation = this->original; }
    } restore{ stack, presentation, original };
    const auto tick = [&] { app.keyboard()->poll(); app.mouse()->poll(); stack.update(dt); };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick();
        // Static Stage text/chrome comparisons observe the settled accent.
        app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.51f);
    };
    const auto frame = [&]
    {
        app.renderer()->begin_frame(); stack.draw(*app.renderer()); return read(*app.renderer());
    };
    const auto prepare = [&]
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true);
        app.mouse()->set_focused(true);
        for (const Key key : { Key::enter, Key::escape, Key::left, Key::right })
            { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll();
        settings = MenuLevelSettings(); settings.set_player_count(1);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_player_team(0, PlayerTeam::a); settings.set_stage("movement_course");
        input.reset_actions(); stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
    };
    const auto open_probe = [&](MenuHeading heading)
    {
        prepare(); auto probe = std::make_unique<MenuTextProbe>(&context, heading);
        MenuTextProbe* result = probe.get(); stack.push(std::move(probe)); tick(); tick(); return result;
    };
    const auto open_stage = [&]
    {
        prepare(); stack.push(std::make_unique<MainMenuStageSelect>(&context)); tick(); tick();
    };
    struct TextReference
    {
        Frame frame;
        std::array<RectangleF, 3> bounds;
    };
    const auto reference = [&](const Presentation& snapshot, const std::wstring& expected_brand,
        const std::wstring& heading, const std::wstring& prompt, bool stage, bool locked)
    {
        const MenuStyle& style = snapshot.menu(); const auto& footer = snapshot.prompt_bar();
        const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
        const UiText brand_text("reference-brand", expected_brand, style.brand_font, style.brand_position,
            app.render_resources(), style.unhighlight);
        const UiTextDropShadow heading_text("reference-heading", heading, style.heading_font,
            style.layout.position, app.render_resources(), stage ? snapshot.main_menu().art.ink : style.heading_colour,
            stage ? Colour(0.0f, 0.0f, 0.0f, 0.0f) : style.shadow_colour,
            style.heading_shadow_offset);
        const UiTextDropShadow prompt_text("reference-prompt", prompt, footer.font, footer.position,
            app.render_resources(), footer.colour, footer.shadow_colour, footer.shadow_offset);
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(viewport);
        list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        canvas.draw(list, RectangleF(Vector2F::ZERO, app.resolution_manager()->resolution_vec()));
        list.set_camera(camera);
        const SpriteSheet* sheet = app.render_resources()->sprite_sheet(style.sprite_sheet);
        const auto fill = sheet->resolve_sprite_frame(style.fill_frame);
        sheet->draw(list, fill, camera.visible_rectangle(viewport), style.scrim_colour);
        if (stage)
        {
            stage_paper_surface_reference(list, *app.render_resources(), snapshot, snapshot, settings,
                false, locked, 1, 0.0f);
        }
        sheet->draw(list, fill, RectangleF(0.0f, style.footer_y, menu_consts::DESIGN_RESOLUTION.x,
            menu_consts::DESIGN_RESOLUTION.y - style.footer_y), style.scrim_colour);
        sheet->draw(list, fill, RectangleF(style.brand_position.x, style.footer_y,
            menu_consts::DESIGN_RESOLUTION.x - style.brand_position.x * 2.0f, style.rule_height),
            style.panel_edge_colour);
        list.set_filter(TextureFilter::linear);
        if (!stage) { brand_text.draw(list); }
        heading_text.draw(list);
        if (stage)
        {
            // The measured heading box reaches into the next row. Its crop
            // includes the beginning of the real stage name, drawn after it.
            const auto& look = snapshot.main_menu().stage_select;
            UiTextDropShadow stage_name("reference-stage-name", L"Movement Course", style.item_font,
                look.name_position, app.render_resources(), locked ? presentation.main_menu().stage_select.selected_ink :
                    presentation.main_menu().stage_select.unselected,
                Colour(0.0f, 0.0f, 0.0f, 0.0f), style.item_shadow_offset);
            const RectangleF bounds = RectangleF::union_of(stage_name.text_bounds_at(Vector2F::ZERO, 1.0f),
                stage_name.text_bounds_at(style.item_shadow_offset, 1.0f));
            const float scale = std::min(1.0f, look.preview_size.x / bounds.width);
            stage_name.set_scale(scale); stage_name.set_shadow_scale(scale);
            stage_name.set_shadow_offset(style.item_shadow_offset * scale); stage_name.draw(list);
        }
        prompt_text.draw(list);
        return TextReference{ read(renderer), { brand_text.bounds(), heading_text.bounds(), prompt_text.bounds() } };
    };
    std::ofstream records(output / "menu-text.csv");
    records << "case,width,height,full_frame,brand_heading_prompt_match\n";
    const auto compare = [&](const std::string& name, const Presentation& snapshot,
        const std::wstring& expected_brand, const std::wstring& heading, const std::wstring& prompt,
        bool stage = false, bool locked = false)
    {
        const Frame actual = frame();
        const TextReference expected = reference(snapshot, expected_brand, heading, prompt, stage, locked);
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        for (size_t index = 0; index < expected.bounds.size(); ++index)
        {
            RectangleF bounds = expected.bounds[index]; bounds.inflate(Vector2F(3.0f));
            camera.calculate_view_rectangle(bounds);
            const RectangleI pixels = countdown_crop_bounds(bounds);
            const Frame actual_crop = foreground_crop(actual, pixels, 0);
            const Frame expected_crop = foreground_crop(expected.frame, pixels, 0);
            if (actual_crop.pixels != expected_crop.pixels)
            {
                const char* component = std::array<const char*, 3>{ "brand", "heading", "prompt" }[index];
                const std::string stem = "menu-text-" + name + '-' + component + '-' +
                    std::to_string(actual.width) + 'x' + std::to_string(actual.height) + "-failure";
                save(actual_crop, output / (stem + "-actual.ppm"));
                save(expected_crop, output / (stem + "-expected.ppm"));
                const RectangleI delta = foreground_delta_bounds(actual_crop, expected_crop);
                std::cerr << "Menu text case=" << name << " component=" << component << " index=" << index <<
                    " stage=" << stage << " locked=" << locked << " crop=(" << pixels.x << ',' << pixels.y << ',' <<
                    pixels.width << ',' << pixels.height << ") local_delta=(" << delta.x << ',' << delta.y << ',' <<
                    delta.width << ',' << delta.height << ")\n";
                throw std::runtime_error("Actual menu brand, heading or formatted prompt differs from the independent text fixture");
            }
            save(foreground_crop(actual, pixels, 0), output / ("menu-text-" + name + '-' +
                std::array<const char*, 3>{ "brand", "heading", "prompt" }[index] + '-' +
                std::to_string(actual.width) + 'x' + std::to_string(actual.height) + "-detail.ppm"));
        }
        require(stage || actual.pixels == expected.frame.pixels,
            "Shared menu text probe differs outside its independent reference text/chrome");
        std::srand(5093); const int next_random = std::rand(); std::srand(5093);
        require(actual.pixels == frame().pixels && std::rand() == next_random,
            "Frozen menu text drawing changed pixels or consumed random numbers");
        records << name << ',' << actual.width << ',' << actual.height << ',' << (!stage ? "true" : "false") << ",true\n";
        return actual;
    };

    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        for (size_t index = 0; index < headings.size(); ++index)
        {
            open_probe(static_cast<MenuHeading>(index));
            compare("default-heading-" + std::to_string(index), original, brand, headings[index], prompts[1]);
        }
        MenuTextProbe* probe = open_probe(MenuHeading::arena);
        for (size_t index = 0; index < prompts.size(); ++index)
        {
            probe->rebuild(static_cast<MenuPrompt>(index));
            compare("default-prompt-" + std::to_string(index), original, brand, headings[10], prompts[index]);
        }
        open_stage(); const Frame browse = compare("stage-browse", original, brand, headings[10], prompts[10], true);
        tap(Key::enter); compare("stage-locked", original, brand, headings[10], prompts[11], true, true);
        tap(Key::escape); require(frame().pixels == browse.pixels,
            "Stage unlock did not restore the exact authored browse text and page");
    }

    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    Presentation old = original;
    auto& old_text = const_cast<MenuText&>(old.menu_text());
    old_text.brand = L"OLD~ BRAND";
    for (auto& heading : old_text.headings) { heading = L"OLD~ HEADING"; }
    for (auto& prompt : old_text.prompts) { prompt = L"OLD~ PROMPT"; }
    old_text.prompts[static_cast<size_t>(MenuPrompt::arena_browse)] = L"OLD~ BROWSE";
    old_text.prompts[static_cast<size_t>(MenuPrompt::arena_locked)] = L"OLD~ LOCKED";
    auto& old_footer = const_cast<PromptBarPresentation&>(old.prompt_bar());
    old_footer.position += Vector2F(36.0f, 8.0f); old_footer.colour = Colour::cyan;
    old.validate_resources(*app.render_resources(), *app.audio_resources());
    presentation = old; open_stage();
    const Frame old_browse = compare("custom-open", old, L"OLD~ BRAND", L"OLD~ HEADING", L"OLD~ BROWSE", true);

    // Borrow the loaded atlas through the public font seam, retaining its
    // original glyphs everywhere except '~'. No existing font or device changes.
    RenderResources& resources = *app.render_resources();
    const FontHandle old_font_handle = resources.resolve_sprite_font(old_footer.font);
    const Font* source = resources.font(old_font_handle);
    std::vector<Glyph> glyphs;
    for (char32_t character = U' '; character <= U'~'; ++character)
    {
        if (character == U'~') { continue; }
        const Glyph* glyph = source->find(character);
        require(glyph != nullptr, "Menu text font fixture lacks a baseline printable glyph");
        glyphs.push_back(*glyph);
    }
    const std::string narrow_name = "acceptance_menu_text_without_tilde";
    resources.add_font(narrow_name, std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
    const FontHandle narrow = resources.resolve_sprite_font(narrow_name);
    require(resources.can_render(old_font_handle, L"OLD~ PROMPT") && !resources.can_render(narrow, L"~"),
        "Menu text font-rebase fixture did not exclude the old text's glyph");
    Presentation next = original;
    auto& next_text = const_cast<MenuText&>(next.menu_text());
    next_text.brand = L"NEW BRAND";
    for (auto& heading : next_text.headings) { heading = L"NEW HEADING"; }
    for (auto& prompt : next_text.prompts) { prompt = L"NEW PROMPT"; }
    next_text.prompts[static_cast<size_t>(MenuPrompt::arena_browse)] = L"NEW BROWSE";
    next_text.prompts[static_cast<size_t>(MenuPrompt::arena_locked)] = L"NEW LOCKED";
    auto& next_footer = const_cast<PromptBarPresentation&>(next.prompt_bar());
    next_footer.font = narrow_name; next_footer.position += Vector2F(60.0f, 12.0f);
    next_footer.colour = Colour::yellow;
    const_cast<MenuStyle&>(next.menu()).brand_font = narrow_name;
    next.validate_resources(resources, *app.audio_resources());
    presentation = next; tick();
    require(frame().pixels == old_browse.pixels, "Presentation assignment rebased an already-open page's text or font");
    for (int repeat = 0; repeat < 2; ++repeat)
    {
        tap(Key::enter); compare("old-page-rebuild-locked-" + std::to_string(repeat), old,
            L"OLD~ BRAND", L"OLD~ HEADING", L"OLD~ LOCKED", true, true);
        tap(Key::escape); compare("old-page-rebuild-browse-" + std::to_string(repeat), old,
            L"OLD~ BRAND", L"OLD~ HEADING", L"OLD~ BROWSE", true);
        require(frame().pixels == old_browse.pixels,
            "Old page prompt rebuild used the replacement font, layout or text snapshot");
    }
    open_stage(); const Frame new_browse = compare("reopened-new-font", next,
        L"NEW BRAND", L"NEW HEADING", L"NEW BROWSE", true);
    require(new_browse.pixels != old_browse.pixels, "Reopened page did not adopt new authored text/font/layout");
    presentation = original; tick();
    require(frame().pixels == new_browse.pixels, "Presentation reversal changed the reopened page snapshot");
    tap(Key::enter); compare("new-page-after-reversal", next,
        L"NEW BRAND", L"NEW HEADING", L"NEW LOCKED", true, true);
    tap(Key::escape); require(frame().pixels == new_browse.pixels,
        "Prompt rebuild after reversal changed the new page's cached font or text");
    open_stage(); compare("restored-default", original, brand, headings[10], prompts[10], true);
    require(settings.stage() == "movement_course" && settings.player_count() == 1 &&
        settings.player_settings()[0].num == input_slots::keyboard_and_mouse &&
        settings.player_weapon(0) == WeaponType::sprayer && settings.player_team(0) == PlayerTeam::a,
        "Text/browse/lock/unlock fixture changed the match plan");
    menu_item_page_checks(app, canvas, tuning, presentation, output);
    menu_setup_text_checks(app, canvas, tuning, presentation, output);
    title_home_text_checks(app, canvas, tuning, presentation, output, true);
    stage_random_text_checks(app, levels, canvas, tuning, presentation, output);
    stage_briefing_text_checks(app, levels, canvas, tuning, presentation, output);
    stage_cue_text_checks(app, levels, canvas, tuning, presentation, output);
    require(records.good(), "Menu text evidence write failed");
    std::cout << "PASS shared menu text: independent brand, 11 headings and 17 formatted legends at 720p/1080p/QHD; "
        "actual arena lock/unlock; custom text/font/layout reopen and old-page snapshots through assignments, "
        "repeated prompt rebuilds and reversal with an old glyph absent from the new font; frozen draw RNG\n";
}
