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
