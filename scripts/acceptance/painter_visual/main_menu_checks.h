// Real title/home pages: keyboard input reaches the production StateContext,
// and colour deltas observe the actual paper/card raster rather than redrawing it.
Frame main_menu_frame(Application& app)
{
    app.renderer()->begin_frame();
    static_cast<StateContext&>(app).draw(*app.renderer());
    return read(*app.renderer());
}

// Construct the production base with real audio/render resources. A CPU-only
// fixture cannot reach this constructor through MenuContext's service bag.
void menu_widget_geometry_checks(MenuContext& context, Presentation& presentation,
    const std::filesystem::path& output)
{
    class MenuGeometryProbe final : public MenuPage
    {
    public:
        explicit MenuGeometryProbe(MenuContext* menu_context) : MenuPage(menu_context) {}
        void init() override {}
        void update_page(float /*dt*/) override {}
        using MenuPage::widget_position;
        using MenuPage::widget_size;
        using MenuPage::widget_spacing;
        using MenuPage::calculate_widget_position;
        using MenuPage::set_widget_position;
        using MenuPage::set_widget_size;
        using MenuPage::set_widget_spacing;
    };

    WidgetLayout& layout = const_cast<MenuStyle&>(presentation.menu()).layout;
    const WidgetLayout original = layout;
    require(original.position == Vector2F(150.0f, 150.0f) &&
        original.size == Vector2F(300.0f, 75.0f) &&
        original.spacing == Vector2F(250.0f, 150.0f),
        "Shared menu widget defaults changed from their shipped geometry");
    std::ofstream records(output / "menu-widget-geometry.csv");
    records << "case,position_x,position_y,size_x,size_y,spacing_x,spacing_y,column_2_x,row_3_y\n";
    const auto check = [&](const char* name, const MenuGeometryProbe& page,
        const WidgetLayout& expected)
    {
        require(page.widget_position() == expected.position && page.widget_size() == expected.size &&
            page.widget_spacing() == expected.spacing,
            "Actual MenuPage geometry differs from its constructor snapshot or page override");
        require(page.calculate_widget_position(0, 0) == expected.position &&
            page.calculate_widget_position(2, 3) == expected.position +
                Vector2F(expected.spacing.x * 2.0f, expected.spacing.y * 3.0f) &&
            page.calculate_widget_position(-1, -1) == expected.position - expected.spacing,
            "Actual MenuPage row/column placement changed its design-space calculation");
        const Vector2F placed = page.calculate_widget_position(2, 3);
        records << name << ',' << page.widget_position().x << ',' << page.widget_position().y << ',' <<
            page.widget_size().x << ',' << page.widget_size().y << ',' <<
            page.widget_spacing().x << ',' << page.widget_spacing().y << ',' <<
            placed.x << ',' << placed.y << '\n';
    };

    {
        struct RestoreWidgetLayout
        {
            WidgetLayout& layout;
            WidgetLayout original;
            ~RestoreWidgetLayout() { this->layout = this->original; }
        } restore{ layout, original };
        MenuGeometryProbe shipped(&context);
        check("shipped", shipped, original);
        const WidgetLayout custom{ Vector2F(129.5f, 84.25f), Vector2F(410.0f, 92.0f),
            Vector2F(330.0f, 111.0f) };
        layout = custom;
        MenuGeometryProbe authored(&context);
        check("custom", authored, custom);
        check("shipped-snapshot-after-custom", shipped, original);

        const WidgetLayout next{ Vector2F(200.0f, 210.0f), Vector2F(315.0f, 80.0f),
            Vector2F(275.0f, 160.0f) };
        layout = next;
        check("custom-snapshot-after-edit", authored, custom);
        MenuGeometryProbe reopened(&context);
        check("reopened-after-edit", reopened, next);

        const WidgetLayout overridden{ Vector2F(41.5f, 63.25f), Vector2F(145.0f, 67.0f),
            Vector2F(33.5f, 22.25f) };
        shipped.set_widget_position(overridden.position);
        shipped.set_widget_size(overridden.size);
        shipped.set_widget_spacing(overridden.spacing);
        check("page-overrides", shipped, overridden);
        check("other-page-after-overrides", reopened, next);
        require(layout.position == next.position && layout.size == next.size &&
            layout.spacing == next.spacing,
            "Per-page widget setters changed the shared presentation geometry");
    }
    MenuGeometryProbe restored(&context);
    check("restored-baseline", restored, original);
    require(records.good(), "Menu widget geometry evidence write failed");
}

// Independent fixed Title/Home copy, including the currently hidden keyboard
// connection hint. MainMenuOnly owns six-size parity; MenuTextOnly owns reloads.
void title_home_text_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output, bool reloads)
{
    using Words = std::array<std::wstring, 5>;
    const Words legacy{ L"COLOUR\nWARS", L"MAKE A MESS. MAKE IT YOURS.", L"LET'S PLAY",
        L"Connect a controller", L"Paint the arena. Claim the win." };
    const Presentation original = presentation;
    Save preferences;
    auto& play = const_cast<PlaySettings&>(preferences.play_settings()); play.music_volume = play.sfx_volume = 0;
    GameData data;
    data.set_application(&app); data.set_save(&preferences); data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings);
    StateContext& stack = app;
    struct RestoreTitleText
    {
        StateContext& stack;
        Presentation& presentation;
        Presentation original;
        ~RestoreTitleText() { this->stack.clear(); this->presentation = this->original; }
    } restore{ stack, presentation, original };
    const auto tick = [&] { app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.0f); };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick();
    };
    const auto open = [&](int page)
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::up, Key::down, Key::left, Key::right, Key::enter, Key::space, Key::escape })
            { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll();
        input.reset_actions(); stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        if (page == 0) { stack.push(std::make_unique<MainMenuTitle>(&context)); }
        else if (page == 1) { stack.push(std::make_unique<MainMenuHome>(&context)); }
        else { stack.push(std::make_unique<MainMenuModeSelect>(&context)); }
        tick(); tick(); require(stack.depth() == 2, "Title/Home text fixture consumed neutral entry input");
    };
    std::ofstream records(output / (reloads ? "title-home-text-reloads.csv" : "title-home-text.csv"));
    records << "case,page,role,width,height,visible,fixture_matches\n";
    const auto compare = [&](const std::string& name, bool home, const Presentation& snapshot,
        const Words& words, size_t focused = 0)
    {
        const auto& style = snapshot.menu(); const auto& look = snapshot.main_menu(); const auto& art = look.art;
        const Frame actual = main_menu_frame(app);
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        std::vector<std::unique_ptr<UiText>> text;
        const auto add = [&](const std::wstring& copy, const std::string& font, const Vector2F& position,
            const Colour& colour, bool centred = false, bool hidden = false)
        {
            auto widget = std::make_unique<UiText>("independent-title-home-text", copy, font, position,
                app.render_resources(), colour, hidden);
            if (centred) { widget->set_position(Vector2F((menu_consts::DESIGN_RESOLUTION.x -
                widget->bounds().width) / 2.0f, position.y)); }
            text.push_back(std::move(widget));
        };
        if (home)
        {
            add(L"Make your mark.", style.heading_font, look.home_position, art.ink);
            add(words[4], style.detail_font, look.home_subtitle_position, art.ink);
            constexpr std::array<const wchar_t*, 4> actions{ L"Play", L"Options", L"Credits", L"Exit" };
            for (size_t row = 0; row < actions.size(); ++row)
                { add(actions[row], style.item_font, look.home_position + Vector2F(0.0f,
                    look.home_spacing.y * static_cast<float>(row + 2)), art.ink); }
        }
        else
        {
            add(words[0], look.title_font, Vector2F(0.0f, look.title_y), art.ink, true);
            add(words[1], style.detail_font, Vector2F(0.0f, look.title_subtitle_y), art.ink, true);
            add(words[2], style.item_font, Vector2F(0.0f, look.title_start_y), art.focus_ink, true);
            // Keyboard input is always a connected menu slot. This checks the
            // complete absence raster at the independently measured hint bounds;
            // it does not simulate a visible controller-loss or physical device.
            add(words[3], style.item_font, Vector2F(0.0f, look.title_no_pad_y), art.ink, true, true);
            add(build_info::label, snapshot.prompt_bar().font, Vector2F(0.0f, look.build_id_y), art.ink, true);
        }
        FocusGroup focus; focus.set_style({ art.focus_ink, art.ink, art.disabled_ink });
        if (home)
        {
            for (size_t index = 2; index < text.size(); ++index) { focus.add(text[index].get()); }
            for (size_t move = 0; move < focused; ++move) { focus.move(0, Direction::down); }
        }
        else { focus.add(text[2].get()); }
        const Vector2F panel_size = home ? look.home_box_size : look.title_box_size;
        const RectangleF panel((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - panel_size) / 2.0f, panel_size);
        const MenuPaperArtwork artwork(panel, app.render_resources(), &style, &art, &focus, text[2].get());
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app.viewport_manager()->fullscreen_viewport());
        list.set_camera(camera); list.set_filter(TextureFilter::point);
        const Canvas background(app.render_resources(), home ? look.home_background_colour : look.title_background_colour);
        background.draw(list, camera.visible_rectangle(app.viewport_manager()->fullscreen_viewport())); artwork.draw(list);
        list.set_filter(TextureFilter::linear);
        for (const auto& widget : text) { widget->draw(list); }
        const Frame reference = read(renderer);
        const auto compare_bounds = [&](RectangleF bounds, const std::string& role, bool visible)
        {
            bounds.inflate(Vector2F(2.0f)); camera.calculate_view_rectangle(bounds);
            const RectangleI pixels = countdown_crop_bounds(bounds);
            const Frame observed = foreground_crop(actual, pixels, 0), wanted = foreground_crop(reference, pixels, 0);
            const std::string stem = "title-home-text-" + name + '-' + (home ? "home-" : "title-") + role + '-' +
                std::to_string(actual.width) + 'x' + std::to_string(actual.height);
            if (observed.pixels != wanted.pixels)
            {
                save(observed, output / (stem + "-failure-actual.ppm")); save(wanted, output / (stem + "-failure-expected.ppm"));
                const RectangleI delta = foreground_delta_bounds(observed, wanted);
                std::cerr << "Title/Home case=" << name << " home=" << home << " role=" << role <<
                    " crop=(" << pixels.x << ',' << pixels.y << ',' << pixels.width << ',' << pixels.height <<
                    ") delta=(" << delta.x << ',' << delta.y << ',' << delta.width << ',' << delta.height << ")\n";
                throw std::runtime_error("Actual Title/Home copy or Start-card bounds differ from the independent linear fixture");
            }
            save(observed, output / (stem + "-detail.ppm"));
            records << name << ',' << (home ? "home" : "title") << ',' << role << ',' << actual.width << ',' << actual.height <<
                ',' << (visible ? "true" : "false") << ",true\n";
        };
        if (home) { compare_bounds(text[1]->bounds(), "home-subtitle", true); }
        else
        {
            constexpr std::array<const char*, 4> names{ "wordmark", "subtitle", "start", "no-controller-hidden" };
            for (size_t index = 0; index < names.size(); ++index)
                { compare_bounds(text[index]->bounds(), names[index], index != 3); }
            RectangleF start = text[2]->bounds(); start.inflate(art.focus_padding);
            compare_bounds(start, "complete-start-card", true);
        }
        std::srand(6131); const int next_random = std::rand(); std::srand(6131);
        require(actual.pixels == main_menu_frame(app).pixels && std::rand() == next_random,
            "Title/Home fixed text drawing changed frozen pixels or consumed random numbers");
        return actual;
    };
    const std::array<Vector2I, 6> sizes{ Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440),
        Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) };
    std::array<Frame, 2> baseline{};
    for (size_t size_index = 0; size_index < (reloads ? 1 : sizes.size()); ++size_index)
    {
        const Vector2I size = sizes[size_index]; app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        for (int page = 0; page < 2; ++page)
        {
            open(page); const Frame actual = compare("default", page == 1, original, legacy);
            if (size_index == 0) { baseline[static_cast<size_t>(page)] = actual; }
        }
    }
    if (reloads)
    {
        const Words old_words{ L"PAINT~\nNOW", L"MAKE~ A MESS.", L"GO~ PAINT", L"Connect~ controls", L"Paint~ your arena." };
        const Words new_words{ L"PAINT\nON", L"MAKE YOUR MARK.", L"PLAY", L"Connect controls", L"Claim your arena." };
        const auto authored = [&](const Words& words)
        {
            Presentation result = original; auto& copy = const_cast<MenuText&>(result.menu_text());
            copy.title.wordmark = words[0]; copy.title.subtitle = words[1]; copy.title.start = words[2];
            copy.title.no_controller = words[3]; copy.home_subtitle = words[4]; return result;
        };
        Presentation old = authored(old_words), next = authored(new_words);
        old.validate_resources(*app.render_resources(), *app.audio_resources());
        const auto replacement_font = [&](const std::string& original_name, const std::string& name)
        {
            const Font* source = app.render_resources()->font(app.render_resources()->resolve_sprite_font(original_name));
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character < U'~'; ++character)
                { if (const Glyph* glyph = source->find(character)) { glyphs.push_back(*glyph); } }
            app.render_resources()->add_font(name,
                std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
            require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(name), L"~"),
                "Title/Home replacement font still contains the old fixed text's glyph");
            return name;
        };
        const_cast<MainMenuPresentation&>(next.main_menu()).title_font = replacement_font(original.main_menu().title_font,
            "acceptance_title_wordmark_without_tilde");
        auto& next_style = const_cast<MenuStyle&>(next.menu());
        next_style.item_font = replacement_font(original.menu().item_font, "acceptance_title_start_without_tilde");
        next_style.detail_font = replacement_font(original.menu().detail_font, "acceptance_title_detail_without_tilde");
        next.validate_resources(*app.render_resources(), *app.audio_resources());
        presentation = next; open(2); const Frame next_mode = main_menu_frame(app);
        const auto interrupted_enter = [&](const Frame& before)
        {
            app.keyboard()->on_key_down(Key::enter); tick();
            static_cast<WindowNotify&>(app).on_deactivated(); tick();
            static_cast<WindowNotify&>(app).on_activated(); tick();
            app.keyboard()->on_key_up(Key::enter); tick(); tick();
            require(stack.depth() == 2 && before.pixels == main_menu_frame(app).pixels,
                "Interrupted Title/Home Enter release selected a page or rebased cached fixed text");
        };
        for (int repeat = 0; repeat < 2; ++repeat)
        {
            const std::string suffix = '-' + std::to_string(repeat);
            presentation = old; open(0); const Frame old_title = compare("custom-old" + suffix, false, old, old_words);
            presentation = next; tick();
            require(old_title.pixels == main_menu_frame(app).pixels,
                "Live Title text/font assignment rebased its multiline wordmark or Start-card bounds");
            interrupted_enter(old_title); compare("retained-old-title" + suffix, false, old, old_words);
            tap(Key::enter); tick(); const Frame new_home = compare("new-home-from-old-title" + suffix, true, next, new_words);
            presentation = original; tick(); tap(Key::down); tap(Key::up);
            require(new_home.pixels == main_menu_frame(app).pixels,
                "Home subtitle/font reversal or focus movement changed its page snapshot");
            compare("retained-new-home" + suffix, true, next, new_words);
            tap(Key::escape); tick();
            require(compare("default-title-return" + suffix, false, original, legacy).pixels == baseline[0].pixels,
                "Home Back did not reopen the exact default Title text and Start card");
            presentation = old; tick(); tap(Key::enter); tick();
            const Frame old_home = compare("old-home-from-default-title" + suffix, true, old, old_words);
            presentation = next; tick(); interrupted_enter(old_home);
            require(old_home.pixels == main_menu_frame(app).pixels,
                "Home text/font assignment or interrupted selection rebased its old subtitle");
            compare("retained-old-home" + suffix, true, old, old_words);
            tap(Key::enter); tick();
            require(next_mode.pixels == main_menu_frame(app).pixels,
                "Fresh Home selection after focus return did not reach the actual Mode page");
            tap(Key::escape); tick(); compare("new-home-after-mode-back" + suffix, true, next, new_words);
            tap(Key::escape); tick(); const Frame new_title = compare("reopened-new-title" + suffix, false, next, new_words);
            require(new_title.pixels != old_title.pixels, "Reopened Title did not adopt new wordmark/detail/item fonts and text");
            presentation = original; tick();
            require(new_title.pixels == main_menu_frame(app).pixels,
                "Title source reversal changed its reopened wordmark, fixed copy or Start-card bounds");
            compare("retained-new-title-after-reversal" + suffix, false, next, new_words);
            tap(Key::enter); tick();
            require(compare("restored-default-home" + suffix, true, original, legacy).pixels == baseline[1].pixels,
                "Title Start did not reopen the exact default Home subtitle");
            tap(Key::escape); tick();
            require(compare("restored-default-title" + suffix, false, original, legacy).pixels == baseline[0].pixels,
                "Repeated Title/Home transitions did not restore the original Title raster");
        }
    }
    require(records.good(), "Title/Home fixed text evidence write failed");
    std::cout << "PASS Title/Home fixed text: independent multiline wordmark, subtitle, Start, hidden keyboard connection-hint "
        "extent and Home subtitle; complete Start-card bounds and linear sampling; " <<
        (reloads ? "separate title/item/detail fonts, retained/reopened/reversed snapshots, repeated Title/Home/Back, Mode return and interrupted Enter rearming" :
            "native 720p/1080p/QHD/4:3/16:10/ultrawide parity") << "; frozen draw RNG\n";
}

void main_menu_matrix(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    Save preferences;
    auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0;
    GameData data;
    data.set_application(&app);
    data.set_save(&preferences);
    data.set_tuning(&tuning);
    data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings);
    menu_widget_geometry_checks(context, presentation, output);
    StateContext& stack = app;
    struct ClearMainStates
    {
        StateContext& stack;
        ~ClearMainStates() { this->stack.clear(); }
    } clear{ stack };
    const MainMenuPresentation& look = presentation.main_menu();
    const MenuStyle& style = presentation.menu();
    auto& art = const_cast<MenuPaperArtPresentation&>(look.art);
    struct RestoreMainArt
    {
        MenuPaperArtPresentation& art;
        MenuPaperArtPresentation original;
        ~RestoreMainArt() { this->art = this->original; }
    } restore_art{ art, art };
    const Vector2F paper_area(menu_consts::DESIGN_RESOLUTION.x, style.footer_y);
    const RectangleF title_panel((paper_area - look.title_box_size) / 2.0f,
        look.title_box_size);
    const RectangleF home_panel((paper_area - look.home_box_size) / 2.0f,
        look.home_box_size);
    require(title_panel.bottom() + art.shadow_offset < style.footer_y &&
        home_panel.bottom() + art.shadow_offset < style.footer_y,
        "Title/home paper and shadow overlap the shared control footer");
    const auto measure = [&](const std::string& font, const std::wstring& text)
    {
        return app.render_resources()->measure_text(
            app.render_resources()->resolve_sprite_font(font), text);
    };
    const auto centered = [&](const std::string& font, const std::wstring& text, float y)
    {
        const Vector2F size = measure(font, text);
        return RectangleF((menu_consts::DESIGN_RESOLUTION.x - size.x) / 2.0f, y, size.x, size.y);
    };
    const auto card = [&](RectangleF text)
    {
        text.inflate(art.focus_padding + Vector2F(art.keyline_width + 2.0f));
        return text;
    };
    const RectangleF title = centered(look.title_font, L"COLOUR\nWARS", look.title_y);
    const RectangleF subtitle = centered(style.detail_font, L"MAKE A MESS. MAKE IT YOURS.",
        look.title_subtitle_y);
    const RectangleF start = centered(style.item_font, L"LET'S PLAY", look.title_start_y);
    const RectangleF no_pad = centered(style.item_font, L"Connect a controller", look.title_no_pad_y);
    const RectangleF build = centered(presentation.prompt_bar().font, build_info::label,
        look.build_id_y);
    const RectangleF heading(look.home_position, measure(style.heading_font, L"Make your mark."));
    const RectangleF home_subtitle(look.home_subtitle_position,
        measure(style.detail_font, L"Paint the arena. Claim the win."));
    constexpr std::array<const wchar_t*, 4> labels{ L"Play", L"Options", L"Credits", L"Exit" };
    std::array<RectangleF, 4> home_rows;
    for (size_t row = 0; row < labels.size(); ++row)
    {
        home_rows[row] = RectangleF(look.home_position + Vector2F(0.0f,
            look.home_spacing.y * static_cast<float>(row + 2)), measure(style.item_font, labels[row]));
        require(home_panel.contains(card(home_rows[row])),
            "Home action text/card escapes its authored paper panel");
        require(look.home_spacing.y > home_rows[row].height + art.focus_padding.y * 2.0f,
            "Home action cards overlap at the actual font height");
    }
    for (const RectangleF& text : { title, subtitle, start, no_pad, build })
    {
        require(title_panel.contains(text), "Measured title text escapes its paper panel");
    }
    const RectangleF start_card = card(start);
    if (!title_panel.contains(start_card) || title.bottom() >= subtitle.y ||
        subtitle.bottom() >= start_card.y || start_card.bottom() >= no_pad.y || no_pad.bottom() >= build.y)
    {
        const auto describe = [&](const char* label, const RectangleF& bounds)
        {
            std::cerr << label << "=(" << bounds.x << ',' << bounds.y << ',' << bounds.width << ',' <<
                bounds.height << ") bottom=" << bounds.bottom() << '\n';
        };
        describe("title-panel", title_panel);
        describe("title", title);
        describe("subtitle", subtitle);
        describe("start-card", start_card);
        describe("no-controller", no_pad);
        describe("build", build);
    }
    require(title_panel.contains(start_card), "Measured title start card escapes its paper panel");
    require(title.bottom() < subtitle.y, "Measured title lettering overlaps its subtitle");
    require(subtitle.bottom() < start_card.y, "Measured title subtitle overlaps its start card");
    require(start_card.bottom() < no_pad.y, "Measured title start card overlaps its connection hint");
    require(no_pad.bottom() < build.y, "Measured title connection hint overlaps its build line");
    require(home_panel.contains(heading) && home_panel.contains(home_subtitle) &&
        heading.bottom() < home_subtitle.y && home_subtitle.bottom() < card(home_rows[0]).y,
        "Home heading/subtitle overlap an action card or escape their paper panel");

    app.keyboard()->set_focused(true);
    app.mouse()->set_focused(true);
    for (const Key key : { Key::up, Key::down, Key::left, Key::right,
        Key::enter, Key::space, Key::escape })
    {
        app.keyboard()->on_key_up(key);
    }
    app.keyboard()->poll(); app.keyboard()->poll();
    app.mouse()->poll(); app.mouse()->poll();
    const auto tick = [&](float elapsed = 0.0f)
    {
        app.keyboard()->poll();
        app.mouse()->poll();
        stack.update(elapsed);
    };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick();
        app.keyboard()->on_key_up(key); tick();
    };
    const auto open = [&](std::unique_ptr<State> page)
    {
        stack.clear();
        stack.notify_activation(true);
        input.reset_actions();
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        stack.push(std::move(page));
        tick();
        require(stack.depth() == 2, "Actual main-menu page did not remain open on neutral input");
    };
    const auto open_title = [&] { open(std::make_unique<MainMenuTitle>(&context)); };
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
    const auto project = [&](const RectangleF& box) { return camera().calculate_view_rectangle(box); };
    const auto text_contrast = [&](const Frame& frame, const RectangleF& text)
    {
        const RectangleI pixels = countdown_crop_bounds(project(text));
        require(pixels.x >= 0 && pixels.y >= 0 && pixels.right() <= frame.width && pixels.bottom() <= frame.height,
            "Main-menu text escaped the display");
        float darkest = 1.0f, lightest = 0.0f;
        size_t ink = 0, paper = 0;
        for (int y = pixels.y; y < pixels.bottom(); ++y)
        {
            for (int x = pixels.x; x < pixels.right(); ++x)
            {
                const float value = hud_luminance(frame, hud_pixel_offset(frame, x, y));
                darkest = std::min(darkest, value);
                lightest = std::max(lightest, value);
                if (value < 0.1f) { ++ink; }
                if (value > 0.5f) { ++paper; }
            }
        }
        // The slightly darker focused paper is still comfortably above 7:1.
        require(ink > 2 && paper > 2 && (lightest + 0.05f) / (darkest + 0.05f) >= 7.0f,
            "Native main-menu text lost dark ink, light paper or raster contrast");
    };
    const auto linear_lettering = [&](const Frame& actual, const std::string& font,
        const std::wstring& text, const RectangleF& bounds)
    {
        const UiText expected("linear-text", text, font, bounds.position(),
            app.render_resources(), art.ink);
        const Canvas paper(app.render_resources(), art.paper);
        const auto fixture = [&](TextureFilter filter)
        {
            Renderer& renderer = *app.renderer();
            renderer.begin_frame();
            renderer.set_view_count(1);
            DrawList list = renderer.view(0);
            const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
            list.set_viewport(viewport);
            list.set_camera(camera());
            list.set_filter(TextureFilter::point);
            paper.draw(list, camera().visible_rectangle(viewport));
            list.set_filter(filter);
            expected.draw(list);
            return read(renderer);
        };
        const RectangleI pixels = countdown_crop_bounds(project(bounds));
        const Frame linear = foreground_crop(fixture(TextureFilter::linear), pixels, 0);
        const Frame observed = foreground_crop(actual, pixels, 0);
        require(observed.pixels == linear.pixels,
            "Actual title/home lettering differs from centered plain text with linear sampling");
        if (actual.width == 1280 && actual.height == 720)
        {
            const Frame point = foreground_crop(fixture(TextureFilter::point), pixels, 0);
            require(point.pixels != linear.pixels,
                "Native 720p title/home sampler comparison did not distinguish point and linear text");
        }
    };
    const auto assert_focus = [&](const Frame& normal, const RectangleF& text, bool home = false)
    {
        const Colour original = art.focus_fill;
        art.focus_fill = Colour(0.0f, 1.0f, 1.0f);
        const Frame tinted = main_menu_frame(app);
        art.focus_fill = original;
        const RectangleI changed = foreground_delta_bounds(normal, tinted);
        const RectangleI expected = countdown_crop_bounds(project(card(text)));
        const RectangleI primary = countdown_crop_bounds(project(card(home_rows[0])));
        require(changed.width > 0 && changed.height > 0,
            "Actual title/home focus card disappeared");
        size_t fill_pixels = 0;
        for (int y = changed.y; y < changed.bottom(); ++y)
        {
            for (int x = changed.x; x < changed.right(); ++x)
            {
                if (same_pixel(normal, tinted, hud_pixel_offset(normal, x, y))) { continue; }
                const bool current = x >= expected.x && x < expected.right() &&
                    y >= expected.y && y < expected.bottom();
                const bool pinned = home && x >= primary.x && x < primary.right() &&
                    y >= primary.y && y < primary.bottom();
                require(current || pinned,
                    "Actual title/home focus fill escaped its action and pinned Play card");
                if (current) { ++fill_pixels; }
            }
        }
        require(fill_pixels > 30, "Title/home focus card had no visible paper fill");

        // Observe the detached shape cue to the left of the actual focus.
        RectangleF row = text;
        row.inflate(art.focus_padding);
        const float pen = art.keyline_width;
        const RectangleF arrow(row.x - pen * 8.0f, row.center().y - pen * 4.0f,
            pen * 7.0f, pen * 8.0f);
        const Colour ink = art.ink;
        art.ink = Colour(1.0f, 0.0f, 1.0f);
        const Frame tinted_ink = main_menu_frame(app);
        art.ink = ink;
        const RectangleI arrow_pixels = countdown_crop_bounds(project(arrow));
        const Frame before = foreground_crop(normal, arrow_pixels, 0);
        const Frame after = foreground_crop(tinted_ink, arrow_pixels, 0);
        size_t arrow_changed = 0;
        for (size_t offset = 0; offset < before.pixels.size(); offset += 4)
        {
            if (!same_pixel(before, after, offset)) { ++arrow_changed; }
        }
        require(arrow_changed > 5, "Actual title/home focus lost its detached ink arrow");
        return fill_pixels;
    };
    const auto stable = [&](const Frame& normal)
    {
        std::srand(4931);
        const int expected_random = std::rand();
        std::srand(4931);
        require(normal.pixels == main_menu_frame(app).pixels &&
            normal.pixels == main_menu_frame(app).pixels && std::rand() == expected_random,
            "Frozen title/home drawing changed pixels or consumed random numbers");
        tick(0.75f);
        require(normal.pixels == main_menu_frame(app).pixels,
            "Neutral title/home time moved its static artwork");
        play.reduce_effects = true;
        tick(0.75f);
        require(normal.pixels == main_menu_frame(app).pixels,
            "Reduce effects changed the static title/home artwork");
        play.reduce_effects = false;
    };
    // Compare routed destinations with directly opened production pages.
    // StateContext deliberately has no API that exposes its private top state.
    resize_page({ 1280, 720 });
    for (int destination = 0; destination < 3; ++destination)
    {
        if (destination == 0) { open(std::make_unique<MainMenuModeSelect>(&context)); }
        else if (destination == 1) { open(std::make_unique<MainMenuOptions>(&context)); }
        else { open(std::make_unique<MainMenuCredits>(&context)); }
        const Frame expected = main_menu_frame(app);
        open_title(); tap(Key::enter);
        // The freshly constructed Home waits for neutral direction input so
        // a key held across a transition cannot move its new cursor.
        tick();
        for (int move = 0; move < destination; ++move) { tap(Key::down); }
        const Frame home = main_menu_frame(app);
        try
        {
            require(assert_focus(home, home_rows[static_cast<size_t>(destination)], true) > 30,
                "Home route fixture did not focus its requested destination");
        }
        catch (...)
        {
            save(home, output / ("main-menu-route-" + std::to_string(destination) + "-home-focus-failure.ppm"));
            std::cerr << "Home route pre-activation focus failed for destination=" << destination << '\n';
            throw;
        }
        tap(Key::enter);
        const Frame actual = main_menu_frame(app);
        if (stack.depth() != 2 || expected.pixels != actual.pixels)
        {
            const std::string stem = std::string("main-menu-route-") +
                std::array<const char*, 3>{ "play", "options", "credits" }[static_cast<size_t>(destination)];
            save(expected, output / (stem + "-expected.ppm"));
            save(actual, output / (stem + "-actual.ppm"));
            const RectangleI delta = foreground_delta_bounds(expected, actual);
            size_t changed = 0;
            for (size_t offset = 0; offset < expected.pixels.size(); offset += 4)
            {
                if (!same_pixel(expected, actual, offset)) { ++changed; }
            }
            std::cerr << "Home route destination=" << destination << " (" << stem << ") depth=" <<
                stack.depth() << " delta=(" << delta.x << ',' << delta.y << ',' << delta.width << ',' <<
                delta.height << ") changed_pixels=" << changed << '\n';
        }
        require(stack.depth() == 2 && expected.pixels == actual.pixels,
            "Home Play/Options/Credits reached the wrong actual page");
        tap(Key::escape);
        require(assert_focus(main_menu_frame(app), home_rows[0], true) > 30,
            "Returning from Play/Options/Credits did not restore Home Play focus");
    }

    std::ofstream records(output / "main-menu.csv");
    records << "page,focus,width,height,paper_pixels,focus_pixels\n";
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080),
        Vector2I(2560, 1440), Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        resize_page(size);
        open_title();
        const Frame title_frame = main_menu_frame(app);
        const auto capture = [&](const char* page, const char* focused, const Frame& normal,
            const RectangleF& panel, const RectangleF& focus_text)
        {
            stable(normal);
            RectangleF frame_bounds = panel;
            frame_bounds.inflate(Vector2F(art.keyline_width + art.shadow_offset + 4.0f));
            const RectangleI pixels = countdown_crop_bounds(project(frame_bounds));
            require(pixels.x >= 0 && pixels.y >= 0 && pixels.right() <= size.x && pixels.bottom() <= size.y,
                "Title/home paper escaped a supported display/aspect ratio");
            const Colour original_paper = art.paper;
            art.paper = Colour(1.0f, 0.0f, 1.0f);
            const Frame tinted = main_menu_frame(app);
            art.paper = original_paper;
            const RectangleI changed = foreground_delta_bounds(normal, tinted);
            require(changed.width > 0 && changed.height > 0 && pixels.contains(changed),
                "Actual title/home paper was absent or escaped its panel");
            const size_t paper_pixels = foreground_local_delta(normal, tinted,
                RectangleF(Vector2F(pixels.position()), Vector2F(pixels.size())));
            require(paper_pixels > 100, "Title/home paper has no substantial visible fill");
            const size_t focus_pixels = assert_focus(normal, focus_text, std::string(page) == "home");
            text_contrast(normal, focus_text);
            const std::string stem = std::string("main-menu-") + page + '-' + focused + '-' +
                std::to_string(size.x) + 'x' + std::to_string(size.y);
            save(normal, output / (stem + ".ppm"));
            save(foreground_crop(normal, pixels), output / (stem + "-detail.ppm"));
            records << page << ',' << focused << ',' << size.x << ',' << size.y << ',' <<
                paper_pixels << ',' << focus_pixels << '\n';
            resize_page({ 1920, 1080 });
            std::ignore = main_menu_frame(app);
            resize_page(size);
            require(normal.pixels == main_menu_frame(app).pixels,
                "Resizing the same title/home page changed pixels or focus");
        };
        capture("title", "start", title_frame, title_panel, start);
        text_contrast(title_frame, title);
        text_contrast(title_frame, subtitle);
        text_contrast(title_frame, build);
        linear_lettering(title_frame, look.title_font, L"COLOUR\nWARS", title);
        tap(Key::enter);
        const Frame home = main_menu_frame(app);
        require(home.pixels != title_frame.pixels, "Title Enter did not leave its page");
        text_contrast(home, heading);
        text_contrast(home, home_subtitle);
        for (const RectangleF& row : home_rows) { text_contrast(home, row); }
        linear_lettering(home, style.heading_font, L"Make your mark.", heading);
        for (size_t row = 0; row < home_rows.size(); ++row)
        {
            const Frame current = main_menu_frame(app);
            capture("home", std::array<const char*, 4>{ "play", "options", "credits", "exit" }[row],
                current, home_panel, home_rows[row]);
            if (row + 1 < home_rows.size()) { tap(Key::down); }
        }
        tap(Key::up);
        require(assert_focus(main_menu_frame(app), home_rows[2], true) > 30,
            "Home Up did not return focus to Credits");
        tap(Key::escape);
        require(title_frame.pixels == main_menu_frame(app).pixels,
            "Home Back did not restore the actual title page");
    }

    require(records.good(), "Main-menu evidence write failed");
    title_home_text_checks(app, canvas, tuning, presentation, output, false);
    // Last: Exit destroys this probe's own hidden window, never a user session.
    open_title(); tap(Key::enter); tick();
    for (int move = 0; move < 3; ++move) { tap(Key::down); }
    const HWND window = static_cast<HWND>(app.window());
    require(IsWindow(window) != FALSE, "Main-menu probe lost its hidden window before Exit");
    tap(Key::enter);
    require(IsWindow(window) == FALSE, "Actual Home Exit did not close its isolated window");
    std::cout << "PASS actual menu widget geometry: shipped/custom constructor snapshots, reopened edits, "
        "page overrides and row/column placement; title/home: centered measured lettering, native ink/paper contrast, "
        "all focus cards/arrows, Start/Back/Play/Options/Credits/Exit, frozen/reduced redraw, draw RNG, "
        "resize and 720p/1080p/QHD/4:3/16:10/ultrawide paper bounds\n";
}
