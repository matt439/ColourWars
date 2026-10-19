// Keep the wording reference independent of MenuText and the actual widgets.
// The complete text bounds include the stepped card's lower bevel.
void pause_menu_item_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    using Labels = std::array<std::wstring, 6>;
    const Labels legacy{ L"Resume", L"Restart", L"Options", L"Quit", L"Yes", L"No" };
    constexpr std::array<MenuItem, 6> roles{ MenuItem::resume, MenuItem::restart,
        MenuItem::options, MenuItem::quit, MenuItem::yes, MenuItem::no };
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
    struct RestorePauseItems
    {
        StateContext& stack;
        Presentation& presentation;
        Presentation original;
        ~RestorePauseItems() { this->stack.clear(); this->presentation = this->original; }
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
    const auto open = [&](bool missing = false)
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true);
        app.mouse()->set_focused(true);
        for (const Key key : { Key::up, Key::down, Key::left, Key::right, Key::enter,
            Key::space, Key::escape }) { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll();
        input.reset_actions(); choice.reset(); settings.set_player_count(missing ? 2 : 1);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        if (missing)
        {
            require(!app.gamepads()->connected(0), "Pause item fixture requires absent pad slot zero");
            require(settings.claim_input_slot(0), "Cannot assign pause item missing-seat roster");
        }
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        stack.push<PauseMenuAction>(std::make_unique<PauseMenuInitial>(&context,
            input_slots::keyboard_and_mouse), [&](const PauseMenuAction& result) { choice = result; });
        tick(); tick();
        require(stack.depth() == 2 && !choice, "Pause item page consumed neutral entry input");
    };
    const auto confirmation = [&](int phase)
    {
        for (int row = 0; row < (phase == 1 ? 1 : 3); ++row) { tap(Key::down); }
        tap(Key::enter); tick();
        require(stack.depth() == 2 && !choice, "Pause item confirmation lost its typed page frame");
    };
    std::ofstream records(output / "pause-menu-items.csv");
    records << "case,page,focus,item,width,height,fixture_matches\n";
    const auto compare = [&](const std::string& name, int phase, int focus,
        const Presentation& snapshot, const Labels& labels)
    {
        const auto& style = snapshot.menu(); const auto& look = snapshot.pause_menu();
        const auto& art = look.art;
        const Frame actual = pause_menu_frame(app);
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        const int rows = phase == 1 || phase == 2 ? 2 : 4;
        for (int row = 0; row < rows; ++row)
        {
            const size_t index = static_cast<size_t>(rows == 2 ? row + 4 : row);
            const bool disabled = phase == 3 && row < 2;
            const bool focused = row == focus && !disabled;
            UiText expected("reference-pause-item", labels[index], style.item_font,
                look.layout.position + Vector2F(0.0f, look.layout.spacing.y * static_cast<float>(row)),
                app.render_resources(), disabled ? art.disabled_ink : focused ? art.focus_ink : art.ink);
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
            card(RectangleF((menu_consts::DESIGN_RESOLUTION - look.box_size) / 2.0f, look.box_size), art.paper);
            if (focused)
            {
                RectangleF bounds = expected.bounds(); bounds.inflate(art.focus_padding);
                card(bounds, art.focus_fill);
            }
            // Pause places its paper inside the text container. Its point
            // filter therefore also applies to the following real labels.
            expected.draw(list);
            const Frame reference = read(renderer);
            RectangleF bounds = expected.bounds(); bounds.inflate(Vector2F(2.0f));
            camera.calculate_view_rectangle(bounds);
            const RectangleI pixels = countdown_crop_bounds(bounds);
            const Frame observed = foreground_crop(actual, pixels, 0);
            const Frame wanted = foreground_crop(reference, pixels, 0);
            const std::string stem = "pause-menu-items-" + name + '-' + std::to_string(phase) + '-' +
                std::to_string(index) + '-' + std::to_string(actual.width) + 'x' + std::to_string(actual.height);
            if (observed.pixels != wanted.pixels)
            {
                save(observed, output / (stem + "-failure-actual.ppm"));
                save(wanted, output / (stem + "-failure-expected.ppm"));
                const RectangleI delta = foreground_delta_bounds(observed, wanted);
                std::cerr << "Pause item case=" << name << " page=" << phase << " focus=" << focus <<
                    " item=" << index << " crop=(" << pixels.x << ',' << pixels.y << ',' << pixels.width << ',' <<
                    pixels.height << ") local_delta=(" << delta.x << ',' << delta.y << ',' << delta.width << ',' <<
                    delta.height << ")\n";
                throw std::runtime_error("Actual Pause/confirmation label differs from its independent item fixture");
            }
            save(observed, output / (stem + "-detail.ppm"));
            records << name << ',' << phase << ',' << focus << ',' << index << ',' << actual.width << ',' <<
                actual.height << ",true\n";
        }
        std::srand(7523); const int next_random = std::rand(); std::srand(7523);
        require(actual.pixels == pause_menu_frame(app).pixels && std::rand() == next_random,
            "Pause label drawing changed frozen pixels or consumed random numbers");
        return actual;
    };
    std::array<Frame, 4> baseline{};
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440),
        Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        for (int phase = 0; phase < 4; ++phase)
        {
            open(phase == 3); if (phase == 1 || phase == 2) { confirmation(phase); }
            const Frame actual = compare("default", phase, phase == 1 || phase == 2 ? 1 : 0, original, legacy);
            if (size.x == 1280) { baseline[static_cast<size_t>(phase)] = actual; }
        }
    }
    const Labels old_labels{ L"Resume~", L"Restart~", L"Options~", L"Quit~", L"Yes~", L"No~" };
    const Labels new_labels{ L"Continue", L"Reset", L"Settings", L"Leave", L"OK", L"Return" };
    const auto with_labels = [&](const Labels& labels)
    {
        Presentation result = original;
        auto& text = const_cast<MenuText&>(result.menu_text());
        for (size_t index = 0; index < roles.size(); ++index)
            { text.items[static_cast<size_t>(roles[index])] = labels[index]; }
        return result;
    };
    Presentation old = with_labels(old_labels);
    old.validate_resources(*app.render_resources(), *app.audio_resources());
    const Font* source = app.render_resources()->font(
        app.render_resources()->resolve_sprite_font(original.menu().item_font));
    std::vector<Glyph> glyphs;
    for (char32_t character = U' '; character < U'~'; ++character)
    {
        const Glyph* glyph = source->find(character);
        require(glyph != nullptr, "Pause replacement font lacks a baseline printable glyph");
        glyphs.push_back(*glyph);
    }
    const std::string font_name = "acceptance_pause_items_without_tilde";
    app.render_resources()->add_font(font_name,
        std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
    require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(font_name), L"~"),
        "Pause replacement font still contains the old labels' glyph");
    Presentation next = with_labels(new_labels);
    const_cast<MenuStyle&>(next.menu()).item_font = font_name;
    next.validate_resources(*app.render_resources(), *app.audio_resources());
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    for (int phase = 0; phase < 3; ++phase)
    {
        presentation = old; open(); if (phase > 0) { confirmation(phase); }
        const int focus = phase == 0 ? 0 : 1;
        const Frame retained = compare("custom-old", phase, focus, old, old_labels);
        presentation = next; tick();
        require(retained.pixels == pause_menu_frame(app).pixels,
            "Live pause item/font replacement changed an existing page");
        if (phase == 0) { tap(Key::down); tap(Key::up); }
        else { tap(Key::up); compare("old-yes-after-replacement", phase, 0, old, old_labels); tap(Key::down); }
        require(retained.pixels == pause_menu_frame(app).pixels,
            "Pause focus movement rebased old labels into a font missing their glyph");
        compare("retained-old", phase, focus, old, old_labels);
        if (phase == 0)
        {
            tap(Key::down); tap(Key::down);
            const Frame options = compare("old-options", 0, 2, old, old_labels);
            tap(Key::enter); tick();
            require(stack.depth() == 3 && !choice, "Custom Pause Options did not open a child page");
            for (int row = preferences.fullscreen() ? 1 : 0; row < 3; ++row) { tap(Key::down); }
            const Frame controls_parent = pause_menu_frame(app);
            tap(Key::enter); tick();
            require(stack.depth() == 4 && !choice, "Paused Options did not open Controls/audio");
            tap(Key::escape); tick();
            require(stack.depth() == 3 && !choice && controls_parent.pixels == pause_menu_frame(app).pixels,
                "Controls return changed retained Options labels, font or focus");
            tap(Key::escape); tick();
            require(stack.depth() == 2 && !choice && options.pixels == pause_menu_frame(app).pixels,
                "Options return changed cached Pause labels, font or focus");
            compare("old-options-return", 0, 2, old, old_labels);
            tap(Key::down); tap(Key::enter); tick();
        }
        else
        {
            tap(Key::enter); tick();
            require(stack.depth() == 2 && !choice, "Custom safe No completed a pause action");
            compare("no-reopened-initial", 0, 0, next, new_labels);
            confirmation(phase);
        }
        const int reopened_phase = phase == 0 ? 2 : phase;
        const Frame adopted = compare("reopened-new-font", reopened_phase, 1, next, new_labels);
        presentation = original; tick();
        require(adopted.pixels == pause_menu_frame(app).pixels,
            "Pause item/font reversal changed an already reopened confirmation");
        tap(Key::up); tap(Key::down);
        require(adopted.pixels == pause_menu_frame(app).pixels,
            "Confirmation focus after reversal changed cached labels or font");
        compare("retained-new-after-reversal", reopened_phase, 1, next, new_labels);
        tap(Key::escape); tick();
        require(!choice && stack.depth() == 2, "Confirmation Back failed to return without a typed action");
        const Frame restored = compare("restored-default", 0, 0, original, legacy);
        require(restored.pixels == baseline[0].pixels, "Confirmation Back did not restore the exact default Pause");
        tap(Key::enter);
        require(choice == PauseMenuAction::resume && stack.depth() == 1,
            "Custom/reversed confirmation return changed fresh Resume behavior");
        presentation = next; open();
        const Frame new_initial = compare("new-initial", 0, 0, next, new_labels);
        require(new_initial.pixels != baseline[0].pixels, "New Pause did not adopt custom action labels");
        if (phase == 0)
        {
            presentation = original; tick();
            require(new_initial.pixels == pause_menu_frame(app).pixels,
                "Pause item/font reversal changed an already reopened initial page");
            tap(Key::down); tap(Key::up);
            require(new_initial.pixels == pause_menu_frame(app).pixels,
                "Initial focus after reversal changed cached custom labels or font");
            compare("new-initial-after-reversal", 0, 0, next, new_labels);
            tap(Key::enter);
            require(choice == PauseMenuAction::resume && stack.depth() == 1,
                "Custom Resume label changed its typed action");
        }
        else
        {
            confirmation(phase); tap(Key::up);
            compare("new-deliberate-yes", phase, 0, next, new_labels); tap(Key::enter);
            require(choice == (phase == 1 ? PauseMenuAction::restart : PauseMenuAction::quit) && stack.depth() == 1,
                "Custom confirmation labels changed deliberate Restart/Quit behavior");
        }
    }
    presentation = old; open();
    const Frame before_focus_loss = compare("focus-before", 0, 0, old, old_labels);
    app.keyboard()->on_key_down(Key::enter); tick();
    static_cast<WindowNotify&>(app).on_deactivated(); tick();
    presentation = next;
    static_cast<WindowNotify&>(app).on_activated(); tick();
    app.keyboard()->on_key_up(Key::enter); tick(); tick();
    require(!choice && stack.depth() == 2 && before_focus_loss.pixels == pause_menu_frame(app).pixels,
        "Focus return or an interrupted Enter release resumed Pause or rebased cached labels");
    compare("focus-return-held-release", 0, 0, old, old_labels);
    tap(Key::enter); require(choice == PauseMenuAction::resume, "Focus-return Pause rejected a fresh Resume");

    presentation = old; open(true);
    compare("missing-before", 3, 0, old, old_labels);
    presentation = next; tick();
    compare("missing-retained", 3, 0, old, old_labels);
    app.keyboard()->on_key_down(Key::enter); tick();
    // Remove only the fixture's missing seat. This exercises gate clearing,
    // without claiming a physical controller reconnection.
    settings.set_player_count(1); tick();
    app.keyboard()->on_key_up(Key::enter); tick(); tick();
    require(!choice && stack.depth() == 2, "Clearing a missing-seat gate consumed held Enter or resumed automatically");
    const Frame recovered = compare("missing-cleared-cached", 0, 0, old, old_labels);
    require(recovered.pixels == before_focus_loss.pixels,
        "Missing-seat gate clearing changed cached Pause labels or its restored focus");
    tap(Key::enter); require(choice == PauseMenuAction::resume, "Cleared missing-seat gate rejected a fresh Resume");
    presentation = original; open();
    require(compare("final-default", 0, 0, original, legacy).pixels == baseline[0].pixels,
        "Pause item regression did not restore the original exact raster");
    require(records.good(), "Pause item evidence write failed");
    std::cout << "PASS actual Pause/Restart/Quit labels: six independent legacy roles across six display sizes, "
        "custom labels/font, retained and reversed snapshots, Options/Controls return, No/Back, deliberate actions, "
        "focus interruption, missing-seat gate clearing without automatic Resume and frozen draw RNG\n";
}
