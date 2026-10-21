// Included by painter_visual.cpp directly after main_menu_checks.h, whose
// main_menu_frame it uses; called by main_menu_matrix and menu_text_checks.h.
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
        // This text-snapshot fixture observes the settled focus artwork.
        if (key == Key::up || key == Key::down || key == Key::left || key == Key::right)
            { app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.51f); }
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
