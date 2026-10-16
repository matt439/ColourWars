// Confirmation copy is plain, unscaled detail text. Keep the reference words
// independent of MenuText, and include the complete stepped paper below them.
void pause_question_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    using Questions = decltype(MenuText{}.confirmations);
    const Questions legacy{ L"Are you sure you want to restart?", L"Are you sure you want to quit?",
        L"Exit ColourWars?", L"Keep these display settings?" };
    const Presentation original = presentation;
    Save preferences;
    auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0;
    GameData data; data.set_application(&app); data.set_save(&preferences);
    data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings); StateContext& stack = app;
    struct RestoreQuestions
    {
        StateContext& stack; Presentation& presentation; Presentation original;
        ~RestoreQuestions() { this->stack.clear(); this->presentation = this->original; }
    } restore{ stack, presentation, original };
    const auto tick = [&] { app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.0f); };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick();
        // This historical raster fixture samples the settled ornament pose.
        if (key == Key::up || key == Key::down || key == Key::left || key == Key::right)
        { app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.51f); }
    };
    std::optional<PauseMenuAction> choice;
    const auto open = [&]
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::up, Key::down, Key::left, Key::right, Key::enter, Key::space, Key::escape })
            { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        choice.reset(); settings.set_player_count(1); settings.begin_input_claims(input_slots::keyboard_and_mouse);
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        stack.push<PauseMenuAction>(std::make_unique<PauseMenuInitial>(&context, input_slots::keyboard_and_mouse),
            [&](const PauseMenuAction& result) { choice = result; });
        tick(); tick(); require(stack.depth() == 2 && !choice, "Question fixture consumed neutral Pause entry");
    };
    const auto confirmation = [&](bool quit)
    {
        for (int row = 0; row < (quit ? 3 : 1); ++row) { tap(Key::down); }
        tap(Key::enter); tick();
        require(stack.depth() == 2 && !choice, "Question fixture lost the typed confirmation frame");
    };
    std::ofstream records(output / "pause-menu-questions.csv");
    records << "case,page,width,height,font,raw_width,raw_height,scale,fixture_matches\n";
    const auto compare = [&](const std::string& name, bool quit, const Presentation& snapshot,
        const Questions& questions)
    {
        const auto& style = snapshot.menu(); const auto& look = snapshot.pause_menu(); const auto& art = look.art;
        const std::wstring& question = quit ? questions.quit : questions.restart;
        const auto font = app.render_resources()->resolve_sprite_font(style.detail_font);
        const Vector2F raw = app.render_resources()->measure_text(font, question);
        UiText expected("reference-pause-question", question, style.detail_font,
            Vector2F((menu_consts::DESIGN_RESOLUTION.x - raw.x) / 2.0f, look.status_y),
            app.render_resources(), art.ink);
        require(expected.bounds().size() == raw &&
            expected.bounds().x == (menu_consts::DESIGN_RESOLUTION.x - raw.x) / 2.0f,
            "Question reference changed its unscaled full-paper centering contract");
        const RectangleF panel((menu_consts::DESIGN_RESOLUTION - look.box_size) / 2.0f, look.box_size);
        require(panel.contains(expected.bounds()) && expected.bounds().bottom() <=
            look.layout.position.y - art.focus_padding.y,
            "Question reference escapes its paper or reaches the first padded action");
        const Frame actual = pause_menu_frame(app);
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app.viewport_manager()->fullscreen_viewport());
        list.set_camera(camera); list.set_filter(TextureFilter::point);
        canvas.draw(list, camera.visible_rectangle(app.viewport_manager()->fullscreen_viewport()));
        const SpriteSheet* sheet = app.render_resources()->sprite_sheet(style.sprite_sheet);
        const auto solid = sheet->resolve_sprite_frame(style.fill_frame);
        const auto fill = [&](const RectangleF& bounds, const Colour& colour)
        {
            sheet->draw(list, solid, bounds,
                Colour(colour.r * colour.a, colour.g * colour.a, colour.b * colour.a, colour.a));
        };
        const float pen = art.keyline_width, step = pen * 2.0f, shadow = art.shadow_offset;
        const auto stepped = [&](const RectangleF& bounds, const Colour& colour)
        {
            fill(RectangleF(bounds.x + step, bounds.y, bounds.width - step * 2.0f, bounds.height), colour);
            fill(RectangleF(bounds.x, bounds.y + step, bounds.width, bounds.height - step * 2.0f), colour);
        };
        const RectangleF face(panel.x, panel.y, panel.width - shadow, panel.height - shadow);
        stepped(RectangleF(face.position() + Vector2F(shadow), face.size()), art.shade);
        stepped(face, art.ink);
        const RectangleF inside(face.x + pen, face.y + pen, face.width - pen * 2.0f, face.height - pen * 2.0f);
        stepped(inside, art.paper);
        for (int pass = 3; pass > 0; --pass)
        {
            Colour shade = art.shade; shade.a *= 0.12f; const float width = pen * static_cast<float>(pass);
            fill(RectangleF(inside.x + step, inside.bottom() - width, inside.width - step * 2.0f, width), shade);
            fill(RectangleF(inside.right() - width, inside.y + step, width, inside.height - step * 2.0f), shade);
        }
        // Production Pause puts MenuPaperArtwork first in its text container;
        // that paper leaves point filtering active for the following question.
        expected.draw(list);
        const Frame reference = read(renderer);
        RectangleF bounds = expected.bounds(); bounds.inflate(Vector2F(2.0f)); camera.calculate_view_rectangle(bounds);
        const RectangleI pixels = countdown_crop_bounds(bounds);
        const Frame observed = foreground_crop(actual, pixels, 0), wanted = foreground_crop(reference, pixels, 0);
        const std::string stem = "pause-menu-question-" + name + (quit ? "-quit-" : "-restart-") +
            std::to_string(actual.width) + 'x' + std::to_string(actual.height);
        if (observed.pixels != wanted.pixels)
        {
            save(observed, output / (stem + "-failure-actual.ppm"));
            save(wanted, output / (stem + "-failure-expected.ppm"));
            const RectangleI delta = foreground_delta_bounds(observed, wanted);
            std::cerr << "Pause question case=" << name << " quit=" << quit << " crop=(" << pixels.x << ',' <<
                pixels.y << ',' << pixels.width << ',' << pixels.height << ") delta=(" << delta.x << ',' << delta.y <<
                ',' << delta.width << ',' << delta.height << ")\n";
            throw std::runtime_error("Actual Pause confirmation question differs from its independent complete-bounds fixture");
        }
        save(observed, output / (stem + "-detail.ppm"));
        records << name << ',' << (quit ? "quit" : "restart") << ',' << actual.width << ',' << actual.height << ',' <<
            style.detail_font << ',' << raw.x << ',' << raw.y << ",1,true\n";
        std::srand(6493); const int next_random = std::rand(); std::srand(6493);
        require(actual.pixels == pause_menu_frame(app).pixels && std::rand() == next_random,
            "Question drawing changed frozen pixels or consumed random numbers");
        return actual;
    };
    std::array<Frame, 2> baseline{};
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440),
        Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        for (const bool quit : { false, true })
        {
            open(); confirmation(quit); const Frame actual = compare("default", quit, original, legacy);
            if (size.x == 1280) { baseline[quit ? 1 : 0] = actual; }
            tap(Key::enter); tick();
            require(stack.depth() == 2 && !choice, "Default No on a confirmation completed a Pause action");
        }
    }
    const Questions old_words{ L"Reset this match~?", L"Leave the match~?", L"Exit ColourWars?", L"Keep these display settings?" };
    const Questions new_words{ L"Restart the current paint round?", L"Return to the menu?", L"Exit ColourWars?", L"Keep these display settings?" };
    const auto with_words = [&](const Questions& words)
    {
        Presentation result = original; const_cast<MenuText&>(result.menu_text()).confirmations = words;
        return result;
    };
    Presentation old = with_words(old_words); old.validate_resources(*app.render_resources(), *app.audio_resources());
    const Font* source = app.render_resources()->font(
        app.render_resources()->resolve_sprite_font(original.menu().detail_font));
    std::vector<Glyph> glyphs;
    for (char32_t character = U' '; character < U'~'; ++character)
    {
        const Glyph* glyph = source->find(character);
        require(glyph != nullptr, "Question replacement detail font lacks a baseline printable glyph");
        glyphs.push_back(*glyph);
    }
    const std::string font_name = "acceptance_pause_questions_without_tilde";
    app.render_resources()->add_font(font_name,
        std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
    require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(font_name), L"~"),
        "Question replacement detail font still has the old copy's glyph");
    Presentation next = with_words(new_words); const_cast<MenuStyle&>(next.menu()).detail_font = font_name;
    next.validate_resources(*app.render_resources(), *app.audio_resources());
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    for (const bool quit : { false, true })
    {
        presentation = old; open(); confirmation(quit);
        const Frame retained = compare("custom-old", quit, old, old_words);
        presentation = next; tick();
        require(retained.pixels == pause_menu_frame(app).pixels,
            "Live question/detail font replacement changed an existing confirmation");
        tap(Key::up); compare("old-yes-after-replacement", quit, old, old_words); tap(Key::down);
        require(retained.pixels == pause_menu_frame(app).pixels,
            "Confirmation focus movement rebased its captured question or detail font");
        compare("retained-old", quit, old, old_words);
        tap(Key::enter); tick(); require(!choice && stack.depth() == 2, "Custom safe No completed a Pause action");
        confirmation(quit); const Frame adopted = compare("reopened-new-font", quit, next, new_words);
        require(adopted.pixels != retained.pixels, "Reopened confirmation retained the old question");
        presentation = original; tick(); tap(Key::up); tap(Key::down);
        require(adopted.pixels == pause_menu_frame(app).pixels,
            "Question/detail font reversal changed the reopened confirmation");
        compare("retained-new-after-reversal", quit, next, new_words);
        tap(Key::escape); tick(); require(!choice && stack.depth() == 2, "Custom confirmation Back completed an action");
        confirmation(quit);
        require(compare("restored-default", quit, original, legacy).pixels == baseline[quit ? 1 : 0].pixels,
            "Confirmation Back failed to adopt the restored default question");
        tap(Key::up); tap(Key::enter);
        require(choice == (quit ? PauseMenuAction::quit : PauseMenuAction::restart) && stack.depth() == 1,
            "Custom/reversed confirmation changed deliberate Yes behavior");

        presentation = old; open(); confirmation(quit);
        const Frame before_focus = compare("focus-before", quit, old, old_words);
        app.keyboard()->on_key_down(Key::enter); tick();
        static_cast<WindowNotify&>(app).on_deactivated(); tick(); presentation = next;
        static_cast<WindowNotify&>(app).on_activated(); tick();
        app.keyboard()->on_key_up(Key::enter); tick(); tick();
        require(!choice && stack.depth() == 2 && before_focus.pixels == pause_menu_frame(app).pixels,
            "Interrupted Enter release selected an answer or rebased the captured question");
        compare("focus-return-held-release", quit, old, old_words);
        tap(Key::enter); tick(); require(!choice && stack.depth() == 2, "Fresh safe No after focus loss completed an action");
        confirmation(quit); compare("focus-return-new-confirmation", quit, next, new_words);
        tap(Key::up); tap(Key::enter);
        require(choice == (quit ? PauseMenuAction::quit : PauseMenuAction::restart) && stack.depth() == 1,
            "Fresh Yes after focus interruption lost its intended typed action");
    }
    presentation = old; open(); confirmation(false);
    compare("missing-seat-before", false, old, old_words);
    app.keyboard()->on_key_down(Key::enter); tick();
    require(!app.gamepads()->connected(0), "Question missing-seat fixture requires absent pad slot zero");
    settings.set_player_count(2); settings.begin_input_claims(input_slots::keyboard_and_mouse);
    require(settings.claim_input_slot(0), "Cannot assign the question fixture's missing pad seat");
    tick(); presentation = next; tick(); app.keyboard()->on_key_up(Key::enter); tick(); tick();
    require(!choice && stack.depth() == 2, "A missing seat or interrupted release completed Restart");
    settings.set_player_count(1); tick(); tick();
    require(!choice && stack.depth() == 2, "Clearing a synthetic missing seat resumed automatically");
    confirmation(false); compare("missing-seat-reopened-new", false, next, new_words);
    tap(Key::enter); tick(); require(!choice && stack.depth() == 2, "Missing-seat recovery changed safe No");
    tap(Key::enter); require(choice == PauseMenuAction::resume, "Missing-seat recovery rejected a fresh Resume");
    presentation = original; open(); confirmation(false);
    require(compare("final-default", false, original, legacy).pixels == baseline[0].pixels,
        "Question regression failed to restore the original complete confirmation raster");
    require(records.good(), "Pause question evidence write failed");
    std::cout << "PASS actual Pause confirmation questions: two independent legacy rasters across six display sizes, "
        "unscaled full-paper centering, custom widths/detail font, retained/reopened/reversed snapshots, "
        "safe No/Back, deliberate typed Yes, focus-interrupted Enter, synthetic missing-seat Restart cancellation "
        "and frozen draw RNG; no physical reconnection claim\n";
}
