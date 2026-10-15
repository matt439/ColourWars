// Real pause and confirmation pages, with public keyboard input through the
// state stack. Preferences remain in memory; Controls is closed without Apply.
class PauseMenuProbeRoot final : public State
{
public:
    PauseMenuProbeRoot(Application& app, const Canvas& canvas) : app_(app), canvas_(canvas) {}
    void init() override {}
    void update(float) override {}
    void draw(Renderer& renderer) const override
    {
        renderer.set_view_count(1);
        DrawList list = renderer.view(0);
        list.set_viewport(this->app_.viewport_manager()->fullscreen_viewport());
        list.set_camera(Camera::DEFAULT_CAMERA);
        list.set_filter(TextureFilter::point);
        this->canvas_.draw(list, RectangleF(Vector2F::ZERO,
            this->app_.resolution_manager()->resolution_vec()));
    }
private:
    Application& app_;
    const Canvas& canvas_;
};

Frame pause_menu_frame(Application& app)
{
    app.renderer()->begin_frame();
    static_cast<StateContext&>(app).draw(*app.renderer());
    return read(*app.renderer());
}

// Keep the wording reference independent of MenuText and the actual widgets.
// The complete text bounds include the stepped card's lower bevel.
void pause_menu_item_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    using Labels = std::array<std::wstring, 6>;
    const Labels legacy{ L"Resume", L"Restart", L"Controls / audio", L"Quit", L"Yes", L"No" };
    constexpr std::array<MenuItem, 6> roles{ MenuItem::resume, MenuItem::restart,
        MenuItem::controls_audio, MenuItem::quit, MenuItem::yes, MenuItem::no };
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
    const Labels old_labels{ L"Resume~", L"Restart~", L"Audio~", L"Quit~", L"Yes~", L"No~" };
    const Labels new_labels{ L"Continue", L"Reset", L"Audio", L"Leave", L"OK", L"Return" };
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
            const Frame controls = compare("old-controls", 0, 2, old, old_labels);
            tap(Key::enter); tick();
            require(stack.depth() == 3 && !choice, "Custom Pause Controls did not open a child page");
            tap(Key::escape); tick();
            require(stack.depth() == 2 && !choice && controls.pixels == pause_menu_frame(app).pixels,
                "Controls return changed cached Pause labels, font or focus");
            compare("old-controls-return", 0, 2, old, old_labels);
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
        "custom labels/font, retained and reversed snapshots, Controls return, No/Back, deliberate actions, "
        "focus interruption, missing-seat gate clearing without automatic Resume and frozen draw RNG\n";
}

void pause_menu_matrix(Application& app, const Canvas& canvas, GameTuning& tuning,
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
    StateContext& stack = app;
    struct ClearPauseStates
    {
        StateContext& stack;
        ~ClearPauseStates() { this->stack.clear(); }
    } clear{ stack };
    const MenuStyle& style = presentation.menu();
    const PauseMenuPresentation& look = presentation.pause_menu();
    auto& art = const_cast<MenuPaperArtPresentation&>(look.art);
    struct RestorePauseArt
    {
        MenuPaperArtPresentation& art;
        MenuPaperArtPresentation original;
        ~RestorePauseArt() { this->art = this->original; }
    } restore_art{ art, art };
    const RectangleF panel((menu_consts::DESIGN_RESOLUTION - look.box_size) / 2.0f,
        look.box_size);
    const auto item_font = app.render_resources()->resolve_sprite_font(style.item_font);
    const auto heading_font = app.render_resources()->resolve_sprite_font(style.heading_font);
    const auto detail_font = app.render_resources()->resolve_sprite_font(style.detail_font);
    const auto row_bounds = [&](const wchar_t* label, int row)
    {
        RectangleF bounds(look.layout.position + Vector2F(0.0f,
            look.layout.spacing.y * static_cast<float>(row)),
            app.render_resources()->measure_text(item_font, label));
        bounds.inflate(art.focus_padding + Vector2F(art.keyline_width + 2.0f));
        return bounds;
    };
    for (const auto& row : std::array<std::pair<const wchar_t*, int>, 6>{ {
        { L"Resume", 0 }, { L"Restart", 1 }, { L"Controls / audio", 2 },
        { L"Quit", 3 }, { L"Yes", 0 }, { L"No", 1 } } })
    {
        require(panel.contains(row_bounds(row.first, row.second)),
            "Pause action card escapes its authored paper panel");
        const Vector2F measured = app.render_resources()->measure_text(item_font, row.first);
        require(look.layout.spacing.y > measured.y + art.focus_padding.y * 2.0f,
            "Pause action cards overlap at the actual font height");
    }
    for (const wchar_t* title : { L"Pause Menu P1", L"Controller lost" })
    {
        const Vector2F measured = app.render_resources()->measure_text(heading_font, title);
        const RectangleF heading((menu_consts::DESIGN_RESOLUTION.x - measured.x) / 2.0f,
            look.heading_y, measured.x, measured.y);
        if (!panel.contains(heading) || heading.bottom() >= look.status_y)
        {
            std::cerr << "Pause heading measured=" << measured.x << ',' << measured.y <<
                " bounds=" << heading.x << ',' << heading.y << ',' << heading.width << ',' << heading.height <<
                " panel=" << panel.x << ',' << panel.y << ',' << panel.width << ',' << panel.height <<
                " status_y=" << look.status_y << '\n';
        }
        require(panel.contains(heading) && heading.bottom() < look.status_y,
            "Pause heading escapes its paper or overlaps status text");
    }
    for (const wchar_t* question : { L"Are you sure you want to restart?",
        L"Are you sure you want to quit?" })
    {
        const Vector2F measured = app.render_resources()->measure_text(detail_font, question);
        const RectangleF bounds((menu_consts::DESIGN_RESOLUTION.x - measured.x) / 2.0f,
            look.status_y, measured.x, measured.y);
        require(panel.contains(bounds) && bounds.bottom() < look.layout.position.y - art.focus_padding.y,
            "Pause confirmation question escapes its paper or overlaps its first action");
    }

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
    const auto resize_page = [&](Vector2I size)
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
    };
    std::optional<PauseMenuAction> choice;
    const auto open = [&](int missing_pads = 0, bool keyboard_participates = true)
    {
        stack.clear();
        stack.notify_activation(true);
        input.reset_actions();
        choice.reset();
        settings.set_player_count(missing_pads + (keyboard_participates ? 1 : 0));
        settings.begin_input_claims(keyboard_participates ? input_slots::keyboard_and_mouse : 0);
        for (int pad = keyboard_participates ? 0 : 1; pad < missing_pads; ++pad)
        {
            require(!app.gamepads()->connected(pad),
                "Missing-controller pause fixture requires its authored pad slots to be absent");
            require(settings.claim_input_slot(pad), "Cannot assign missing-controller fixture roster");
        }
        if (!keyboard_participates)
        {
            require(!app.gamepads()->connected(0),
                "All-disconnected pause fixture requires pad slot zero to be absent");
        }
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        stack.push<PauseMenuAction>(std::make_unique<PauseMenuInitial>(&context,
            keyboard_participates ? input_slots::keyboard_and_mouse : 0),
            [&](const PauseMenuAction& result) { choice = result; });
        tick();
        require(stack.depth() == 2 && !choice.has_value(),
            "Actual pause page did not remain open on neutral input");
    };
    const auto confirmation = [&](bool quit)
    {
        for (int move = 0; move < (quit ? 3 : 1); ++move) { tap(Key::down); }
        tap(Key::enter);
        tick();
        require(stack.depth() == 2 && !choice.has_value(),
            "Pause confirmation did not replace its initial page in the typed frame");
    };
    const auto camera = [&]
    {
        return Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
    };
    const auto assert_focus = [&](const Frame& baseline, const RectangleF& expected)
    {
        const Colour original = art.focus_fill;
        art.focus_fill = Colour(0.0f, 1.0f, 1.0f);
        const Frame tinted = pause_menu_frame(app);
        art.focus_fill = original;
        const RectangleI changed = foreground_delta_bounds(baseline, tinted);
        const RectangleI row = countdown_crop_bounds(camera().calculate_view_rectangle(expected));
        require(changed.width > 0 && changed.height > 0 && row.contains(changed),
            "Pause paper focus did not follow the actual enabled action row");
        const size_t fill_pixels = foreground_local_delta(baseline, tinted,
            RectangleF(Vector2F(row.position()), Vector2F(row.size())));
        // The detached arrow is a shape cue independent of the text colour.
        // Observe the real artwork's ink only to the left of this action,
        // away from both the card keyline and the page's outer frame.
        const float pen = art.keyline_width;
        const float left = expected.x + pen + 2.0f;
        const RectangleF arrow(left - pen * 8.0f, expected.center().y - pen * 4.0f,
            pen * 7.0f, pen * 8.0f);
        const Colour original_ink = art.ink;
        art.ink = Colour(1.0f, 0.0f, 1.0f);
        const Frame tinted_ink = pause_menu_frame(app);
        art.ink = original_ink;
        const RectangleI arrow_pixels = countdown_crop_bounds(camera().calculate_view_rectangle(arrow));
        const Frame arrow_before = foreground_crop(baseline, arrow_pixels, 0);
        const Frame arrow_after = foreground_crop(tinted_ink, arrow_pixels, 0);
        size_t arrow_changed = 0;
        for (size_t offset = 0; offset < arrow_before.pixels.size(); offset += 4)
        {
            if (!same_pixel(arrow_before, arrow_after, offset)) { ++arrow_changed; }
        }
        require(arrow_changed > 5,
            "Actual enabled pause focus has no detached ink arrow");
        return fill_pixels;
    };
    std::ofstream records(output / "pause-menu.csv");
    records << "page,width,height,paper_pixels,focus_pixels\n";
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080),
        Vector2I(2560, 1440), Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        resize_page(size);
        for (const int phase : { 0, 1, 2, 3 })
        {
            const char* name = phase == 0 ? "initial" : phase == 1 ? "restart" :
                phase == 2 ? "quit" : "missing-pad";
            open(phase == 3 ? 1 : 0);
            if (phase == 1 || phase == 2) { confirmation(phase == 2); }
            const Frame normal = pause_menu_frame(app);
            std::srand(2137);
            const int expected_random = std::rand();
            std::srand(2137);
            require(normal.pixels == pause_menu_frame(app).pixels &&
                normal.pixels == pause_menu_frame(app).pixels && std::rand() == expected_random,
                "Frozen pause drawing changed pixels or consumed random numbers");
            tick(0.75f);
            require(normal.pixels == pause_menu_frame(app).pixels && !choice.has_value(),
                "Neutral pause time moved artwork or selected an action");
            play.reduce_effects = true;
            tick(0.75f);
            require(normal.pixels == pause_menu_frame(app).pixels,
                "Reduce effects changed the static readable pause artwork");
            play.reduce_effects = false;
            RectangleF frame_bounds = panel;
            frame_bounds.inflate(Vector2F(art.keyline_width + art.shadow_offset + 4.0f));
            camera().calculate_view_rectangle(frame_bounds);
            const RectangleI pixels = countdown_crop_bounds(frame_bounds);
            require(pixels.x >= 0 && pixels.y >= 0 && pixels.right() <= size.x && pixels.bottom() <= size.y,
                "Pause paper escaped a supported display/aspect ratio");
            const Colour original_paper = art.paper;
            art.paper = Colour(1.0f, 0.0f, 1.0f);
            const Frame tinted_paper = pause_menu_frame(app);
            art.paper = original_paper;
            const RectangleI paper_delta = foreground_delta_bounds(normal, tinted_paper);
            require(paper_delta.width > 0 && paper_delta.height > 0 && pixels.contains(paper_delta),
                "Actual pause paper was absent or escaped its panel");
            const size_t paper_pixels = foreground_local_delta(normal, tinted_paper,
                RectangleF(Vector2F(pixels.position()), Vector2F(pixels.size())));
            require(paper_pixels > 100, "Pause paper had no substantial visible fill");
            size_t focus_pixels = 0;
            if (phase != 3)
            {
                focus_pixels = assert_focus(normal, phase == 0 ? row_bounds(L"Resume", 0) : row_bounds(L"No", 1));
                require(focus_pixels > 30, "Pause initial/safe answer focus had no visible card fill");
            }
            else
            {
                const Colour original_focus = art.focus_fill;
                art.focus_fill = Colour(0.0f, 1.0f, 1.0f);
                const Frame disabled = pause_menu_frame(app);
                art.focus_fill = original_focus;
                require(normal.pixels == disabled.pixels,
                    "Missing-controller pause still painted a focus card on disabled Resume");
            }
            const std::string stem = std::string("pause-menu-") + name + '-' +
                std::to_string(size.x) + 'x' + std::to_string(size.y);
            save(normal, output / (stem + ".ppm"));
            save(foreground_crop(normal, pixels), output / (stem + "-detail.ppm"));
            records << name << ',' << size.x << ',' << size.y << ',' << paper_pixels << ',' << focus_pixels << '\n';
            resize_page({ 1920, 1080 });
            std::ignore = pause_menu_frame(app);
            resize_page(size);
            require(normal.pixels == pause_menu_frame(app).pixels && !choice.has_value(),
                "Resizing the same pause page changed pixels, focus or action");
            tap(Key::enter);
            if (phase == 0)
            {
                require(choice == PauseMenuAction::resume && stack.depth() == 1,
                    "Initial Enter on actual pause did not Resume");
            }
            else if (phase == 1 || phase == 2)
            {
                require(!choice.has_value() && stack.depth() == 2,
                    "Initial Enter on a confirmation bypassed its safe No default");
                tap(Key::escape);
                require(choice == PauseMenuAction::resume && stack.depth() == 1,
                    "No did not return to the initial pause page");
            }
            else
            {
                require(!choice.has_value() && stack.depth() == 2,
                    "Missing-pad Enter resumed or restarted a blocked match");
            }
        }
    }
    resize_page({ 1280, 720 });
    for (const bool quit : { false, true })
    {
        open(); confirmation(quit);
        tap(Key::up);
        const Frame yes = pause_menu_frame(app);
        require(assert_focus(yes, row_bounds(L"Yes", 0)) > 30,
            "Confirmation Up did not visibly select Yes");
        save(yes, output / (quit ? "pause-menu-quit-yes-720p.ppm" : "pause-menu-restart-yes-720p.ppm"));
        tap(Key::enter);
        require(choice == (quit ? PauseMenuAction::quit : PauseMenuAction::restart) && stack.depth() == 1,
            "Deliberate confirmation Yes returned the wrong typed pause action");
        open(); confirmation(quit); tap(Key::escape);
        require(!choice.has_value() && stack.depth() == 2,
            "Escape from a pause confirmation did not return to its initial page");
        tap(Key::enter);
        require(choice == PauseMenuAction::resume,
            "Returning from a confirmation lost initial Resume focus");
    }
    open();
    tap(Key::down);
    const Frame restart = pause_menu_frame(app);
    require(assert_focus(restart, row_bounds(L"Restart", 1)) > 30,
        "Pause Down did not visibly focus Restart");
    tap(Key::down); tap(Key::enter); tick();
    require(stack.depth() == 3 && !choice.has_value(),
        "Pause Controls/audio did not push a covering settings page");
    tap(Key::escape); tick();
    require(stack.depth() == 2 && !choice.has_value() &&
        assert_focus(pause_menu_frame(app), row_bounds(L"Controls / audio", 2)) > 30,
        "Closing Controls/audio did not preserve the pause cursor");
    tap(Key::escape);
    require(choice == PauseMenuAction::resume, "Escape from actual initial pause did not Resume");

    // No hardware controller is injected: these rosters intentionally own
    // absent slots. Keyboard is first a participant, then an outside rescue
    // device when all four participants have disappeared.
    for (const bool keyboard_participates : { true, false })
    {
        open(keyboard_participates ? 1 : 4, keyboard_participates);
        const Frame disconnected = pause_menu_frame(app);
        std::wstring missing;
        const int first_missing = keyboard_participates ? 1 : 0;
        for (int player = first_missing; player < settings.player_count(); ++player)
        {
            const int index = player - first_missing;
            if (index > 0) { missing += index % 2 == 0 ? L"\n" : L"   "; }
            missing += settings.player_label(player);
        }
        const Vector2F missing_size = app.render_resources()->measure_text(detail_font, missing);
        const RectangleF missing_bounds((menu_consts::DESIGN_RESOLUTION.x - missing_size.x) / 2.0f,
            look.status_y, missing_size.x, missing_size.y);
        require(panel.contains(missing_bounds) && missing_bounds.bottom() <
            look.layout.position.y - art.focus_padding.y,
            "Missing-controller roster text escapes its paper or overlaps its actions");
        tap(Key::escape);
        require(!choice.has_value() && stack.depth() == 2,
            "Escape bypassed the missing-controller pause");
        tap(Key::down);
        const Frame controls = pause_menu_frame(app);
        require(assert_focus(controls, row_bounds(L"Controls / audio", 2)) > 30,
            "Missing-controller navigation did not skip disabled Resume/Restart");
        tap(Key::enter); tick();
        require(stack.depth() == 3 && !choice.has_value(),
            "Missing-controller pause could not reach Controls/audio");
        tap(Key::escape); tick();
        tap(Key::down);
        const Frame quit = pause_menu_frame(app);
        require(assert_focus(quit, row_bounds(L"Quit", 3)) > 30,
            "Missing-controller pause could not visibly focus Quit");
        tap(Key::enter); tick();
        tap(Key::up); tap(Key::enter);
        require(choice == PauseMenuAction::quit && stack.depth() == 1,
            "Missing participants left no keyboard route through Quit confirmation");
        save(disconnected, output / (keyboard_participates ?
            "pause-menu-keyboard-missing-pad-720p.ppm" : "pause-menu-four-missing-pads-720p.ppm"));
    }
    require(records.good(), "Pause-menu evidence write failed");
    pause_menu_item_checks(app, canvas, tuning, presentation, output);
    std::cout << "PASS actual pause/confirmations: initial Resume, safe No, deliberate Restart/Quit, "
        "Controls return, missing-device recovery, enabled focus cards, frozen/reduced redraw, "
        "resize and 720p/1080p/QHD/4:3/16:10/ultrawide paper bounds\n";
}
