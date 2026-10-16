void menu_setup_text_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output);

// Independent point-filtered construction of the accepted resting paper.
// This fixture does not call MenuPaperArtwork or borrow production widgets.
void setup_paper_reference(DrawList& list, RenderResources& resources,
    const MenuStyle& style, const MenuPaperArtPresentation& art,
    const RectangleF& panel, const RectangleF& focused)
{
    const SpriteSheet* sheet = resources.sprite_sheet(style.sprite_sheet);
    const auto texel = sheet->resolve_sprite_frame(style.fill_frame);
    const auto fill = [&](const RectangleF& box, const Colour& colour)
    {
        sheet->draw(list, texel, box,
            Colour(colour.r * colour.a, colour.g * colour.a, colour.b * colour.a, colour.a));
    };
    const auto card = [&](const RectangleF& bounds, const Colour& colour)
    {
        const float pen = art.keyline_width, step = pen * 2.0f, shadow = art.shadow_offset;
        const RectangleF face(bounds.position(), bounds.size() - Vector2F(shadow));
        const auto stepped = [&](const RectangleF& box, const Colour& tint)
        {
            fill(RectangleF(box.x + step, box.y, box.width - step * 2.0f, box.height), tint);
            fill(RectangleF(box.x, box.y + step, box.width, box.height - step * 2.0f), tint);
        };
        stepped(RectangleF(face.position() + Vector2F(shadow), face.size()), art.shade);
        stepped(face, art.ink);
        const RectangleF inside(face.position() + Vector2F(pen), face.size() - Vector2F(pen * 2.0f));
        stepped(inside, colour);
        for (int pass = 3; pass > 0; --pass)
        {
            Colour shade = art.shade; shade.a *= 0.12f;
            const float width = pen * static_cast<float>(pass);
            fill(RectangleF(inside.x + step, inside.bottom() - width, inside.width - step * 2.0f, width), shade);
            fill(RectangleF(inside.right() - width, inside.y + step, width, inside.height - step * 2.0f), shade);
        }
    };
    list.set_filter(TextureFilter::point); card(panel, art.paper);
    RectangleF row = focused; row.inflate(art.focus_padding); card(row, art.focus_fill);
    const float pen = art.keyline_width;
    const Vector2F tip(row.x - pen * 3.0f, row.center().y);
    fill(RectangleF(tip.x - pen * 4.0f, tip.y - pen, pen * 4.0f, pen * 2.0f), art.ink);
    for (int step = 0; step < 3; ++step)
    {
        const float reach = static_cast<float>(step);
        fill(RectangleF(tip.x - reach * pen, tip.y - (reach + 1.0f) * pen,
            pen, (reach + 1.0f) * pen * 2.0f), art.ink);
    }
}

// The complete static fixture also checks every focus/card, disabled solo,
// authored copy/font snapshots, duration choices and all ordinary routes.
void setup_paper_matrix(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    menu_setup_text_checks(app, canvas, tuning, presentation, output);
    Save preferences; auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0; play.reduce_effects = false;
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_tuning(&tuning);
    data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings); StateContext& stack = app;
    MenuPage* active = nullptr;
    struct ClearSetupPaper { StateContext& stack; ~ClearSetupPaper() { this->stack.clear(); } } clear{ stack };
    const auto& style = presentation.menu(); const auto& look = presentation.main_menu();
    auto art = look.art; art.focus_padding = look.setup_focus_padding; art.shadow_offset = look.setup_shadow_offset;
    const auto motion = style.motion;
    const float settled = std::max(motion.focus_duration, motion.press_duration) + 0.01f;
    const auto tick = [&](float elapsed = 0.0f)
        { app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed); };
    const auto tap = [&](Key key)
        { app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick(); };
    const auto frame = [&] { return main_menu_frame(app); };
    const auto project = [&](RectangleF box)
    {
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        camera.calculate_view_rectangle(box); return countdown_crop_bounds(box);
    };
    const auto row = [&](bool count, MenuItem item, int index, int column = 0)
    {
        const Vector2F spacing = count ? look.player_count_spacing : look.mode_select_spacing;
        const Vector2F position = style.layout.position +
            Vector2F(spacing.x * static_cast<float>(column), spacing.y * static_cast<float>(index));
        const UiTextDropShadow text("independent-paper-row", presentation.menu_text().item(item), style.item_font,
            position, app.render_resources(), art.ink, Colour(0.0f, 0.0f, 0.0f, 0.0f), style.item_shadow_offset);
        return text.bounds();
    };
    const auto arrow = [&](RectangleF bounds)
    {
        bounds.inflate(art.focus_padding); const float pen = art.keyline_width;
        return RectangleF(bounds.x - pen * 8.0f - 2.0f, bounds.center().y - pen * 4.0f - 2.0f,
            pen * 10.0f + 4.0f, pen * 8.0f + 4.0f);
    };
    const auto open = [&](bool count)
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (Key key : { Key::enter, Key::escape, Key::up, Key::down, Key::left, Key::right }) { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll();
        input.reset_actions(); settings = MenuLevelSettings(); settings.set_best_of(1);
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        std::unique_ptr<MenuPage> page;
        if (count) { page = std::make_unique<MainMenuPlayerCount>(&context); }
        else { page = std::make_unique<MainMenuModeSelect>(&context); }
        active = page.get(); stack.push(std::move(page)); tick(); tick();
        const Frame first = frame(); tick(settled);
        require(first.pixels == frame().pixels && stack.depth() == 2, "Initial setup paper did not remain settled");
    };
    const auto frozen = [&](const Frame& image)
    {
        const int best_of = settings.best_of(); const auto duration = settings.duration();
        std::srand(9769); const int next = std::rand(); std::srand(9769);
        require(image.pixels == frame().pixels && image.pixels == frame().pixels && std::rand() == next &&
            settings.best_of() == best_of && settings.duration() == duration,
            "Frozen setup paper changed pixels, settings or RNG");
    };
    std::ofstream records(output / "setup-paper-motion.csv");
    records << "case,width,height,delta_x,delta_y,delta_width,delta_height\n";
    const auto changed = [&](const char* name, const Frame& moving, const Frame& rest, const RectangleF& envelope)
    {
        const RectangleI delta = foreground_delta_bounds(moving, rest), allowed = project(envelope);
        if (delta.width <= 0 || delta.height <= 0 || !allowed.contains(delta))
        {
            save(moving, output / (std::string("setup-paper-") + name + "-failure-moving.ppm"));
            save(rest, output / (std::string("setup-paper-") + name + "-failure-rest.ppm"));
            std::cerr << "SETUP PAPER " << name << " delta=" << delta.x << ',' << delta.y << ',' << delta.width << ',' <<
                delta.height << " allowed=" << allowed.x << ',' << allowed.y << ',' << allowed.width << ',' << allowed.height << '\n';
        }
        require(delta.width > 0 && delta.height > 0 && allowed.contains(delta),
            "Setup paper motion was absent or escaped its fixed ornament envelope");
        records << name << ',' << moving.width << ',' << moving.height << ',' << delta.x << ',' << delta.y << ',' <<
            delta.width << ',' << delta.height << '\n';
    };
    const auto capture = [&](const char* name, const Frame& image, RectangleF detail)
    {
        const std::string stem = std::string("setup-paper-") + name + '-' + std::to_string(image.width) + 'x' + std::to_string(image.height);
        save(image, output / (stem + ".ppm")); save(foreground_crop(image, project(detail)), output / (stem + "-detail.ppm"));
    };
    const auto lettering = [&](const Frame& moving, const Frame& rest, const std::wstring& words,
        Vector2F position, const Colour& ink)
    {
        const UiText text("independent-setup-paper-ink", words, style.item_font, position, app.render_resources(), ink);
        const auto fixture = [&](const Colour& background)
        {
            Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
            DrawList list = renderer.view(0); const auto viewport = app.viewport_manager()->fullscreen_viewport();
            const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
            list.set_viewport(viewport); list.set_camera(camera); list.set_filter(TextureFilter::point);
            const Canvas paper(app.render_resources(), background); paper.draw(list, camera.visible_rectangle(viewport));
            list.set_filter(TextureFilter::linear); text.draw(list); return read(renderer);
        };
        const Frame black = fixture(Colour::black), white = fixture(Colour::white);
        const RectangleI bounds = project(text.bounds()); size_t solid = 0;
        for (int y = bounds.y; y < bounds.bottom(); ++y)
        {
            for (int x = bounds.x; x < bounds.right(); ++x)
            {
                const size_t pixel = hud_pixel_offset(moving, x, y);
                if (!same_pixel(black, white, pixel)) { continue; }
                ++solid; require(same_pixel(moving, rest, pixel) && same_pixel(moving, black, pixel),
                    "Pressed setup paper moved its opaque duration label/value lettering");
            }
        }
        require(solid > 20, "Setup paper lettering fixture found too few opaque glyph pixels");
    };
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
    {
        app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        for (bool count : { false, true })
        {
            open(count); tap(Key::down); const Frame arrival = frame(); frozen(arrival);
            tick(0.0f); tick(-0.5f); require(arrival.pixels == frame().pixels, "Nonpositive time advanced setup paper focus");
            tick(motion.focus_duration * 0.4f); const Frame middle = frame(); tick(settled); const Frame rest = frame();
            const RectangleF envelope = arrow(row(count, count ? MenuItem::two_players : MenuItem::best_of_three, 3));
            changed(count ? "count-arrival" : "mode-arrival", arrival, rest, envelope);
            changed(count ? "count-middle" : "mode-middle", middle, rest, envelope);
            require(arrival.pixels != middle.pixels, "Setup paper focus did not progress with positive time");
            tap(Key::down); tap(Key::up); const Frame rapid = frame(); tick(settled);
            changed(count ? "count-rapid" : "mode-rapid", rapid, frame(), envelope);
            capture(count ? "count-arrival" : "mode-arrival", arrival, envelope);
            capture(count ? "count-middle" : "mode-middle", middle, envelope);
            capture(count ? "count-rest" : "mode-rest", rest, envelope);
            tap(Key::down); play.reduce_effects = true; const Frame reduced = frame(); play.reduce_effects = false;
            const Frame live = frame(); frozen(live); play.reduce_effects = true;
            require(reduced.pixels == frame().pixels, "Applied reduction did not immediately settle setup paper");
            play.reduce_effects = false; require(live.pixels == frame().pixels, "Suppressed setup drawing consumed its motion clock");
            tick(settled); require(reduced.pixels == frame().pixels, "Reduced setup paper did not equal ordinary rest");
            tap(Key::up); play.reduce_effects = true; const Frame clean = frame(); play.reduce_effects = false;
            stack.notify_activation(false); require(clean.pixels == frame().pixels, "Setup paper deactivation retained focus motion");
            stack.notify_activation(true); tick(settled); require(clean.pixels == frame().pixels, "Setup paper activation restarted focus motion");
            tap(Key::down); play.reduce_effects = true; const Frame covered = frame(); play.reduce_effects = false;
            stack.push(std::make_unique<MainMenuCredits>(&context)); stack.pop();
            require(covered.pixels == frame().pixels, "A covering-page return retained setup paper motion"); tick();
            tap(Key::up); play.reduce_effects = true; const Frame debug_rest = frame(); play.reduce_effects = false;
            active->debug_reset_input(input_slots::keyboard_and_mouse);
            require(debug_rest.pixels == frame().pixels, "Developer input reset retained setup paper motion");
            play.reduce_effects = true; tap(Key::down); const Frame quiet = frame(); tick(settled);
            require(quiet.pixels == frame().pixels, "Reduced-effects setup paper input animated"); play.reduce_effects = false;
        }
        open(false); for (int move = 0; move < 3; ++move) { tap(Key::down); } tick(settled);
        tap(Key::right); const Frame pressed = frame(); frozen(pressed);
        tick(motion.press_duration * 0.4f); const Frame releasing = frame(); tick(settled); const Frame rest = frame();
        std::wostringstream value; value << *settings.duration() / 60.0f << L" min";
        const Vector2F label_position = style.layout.position + Vector2F(0.0f, look.mode_select_spacing.y * 5.0f);
        const Vector2F value_position = label_position + Vector2F(look.mode_select_spacing.x * 2.0f, 0.0f);
        const UiTextDropShadow value_bounds("independent-duration-value", value.str(), style.item_font, value_position,
            app.render_resources(), art.ink, Colour(0.0f, 0.0f, 0.0f, 0.0f), style.item_shadow_offset);
        RectangleF duration = RectangleF::union_of(row(false, MenuItem::match_length, 5), value_bounds.bounds());
        duration.inflate(art.focus_padding + Vector2F(2.0f));
        changed("duration-pressed", pressed, rest, duration); changed("duration-releasing", releasing, rest, duration);
        require(pressed.pixels != releasing.pixels, "Setup duration paper did not release with positive time");
        lettering(pressed, rest, presentation.menu_text().item(MenuItem::match_length), label_position, art.focus_ink);
        lettering(pressed, rest, value.str(), value_position, art.ink);
        capture("duration-pressed", pressed, duration); capture("duration-releasing", releasing, duration); capture("duration-rest", rest, duration);
        app.resolution_manager()->set_resolution_exactly({ 1600, 1200 }); std::ignore = app.renderer()->window_size_changed(1600, 1200);
        std::ignore = frame(); app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        require(rest.pixels == frame().pixels, "Setup duration paper changed after a resize round-trip");
        std::cout << "PASS setup paper motion " << size.x << 'x' << size.y << '\n';
    }
    // Effective art is captured with each page, including the compact setup
    // padding/shadow overrides. Later source assignments apply on reopening.
    auto& source = const_cast<MainMenuPresentation&>(presentation.main_menu());
    struct RestoreSetupPaperArt
    {
        MainMenuPresentation& source; MenuPaperArtPresentation art; Vector2F padding; float shadow;
        ~RestoreSetupPaperArt()
            { this->source.art = this->art; this->source.setup_focus_padding = this->padding; this->source.setup_shadow_offset = this->shadow; }
    } restore_art{ source, source.art, source.setup_focus_padding, source.setup_shadow_offset };
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 }); std::ignore = app.renderer()->window_size_changed(1280, 720);
    for (bool count : { false, true })
    {
        source.art = restore_art.art; source.setup_focus_padding = restore_art.padding; source.setup_shadow_offset = restore_art.shadow;
        open(count); tap(Key::down); tick(settled); const Frame old_art = frame();
        source.art.paper = Colour(0.90f, 0.95f, 0.98f); source.art.focus_fill = Colour(0.80f, 0.85f, 0.90f);
        source.art.ink = Colour(0.04f, 0.09f, 0.17f); source.art.focus_ink = Colour(0.02f, 0.04f, 0.09f);
        source.setup_focus_padding += Vector2F(1.0f, 0.0f); source.setup_shadow_offset -= 1.0f;
        tick(); require(old_art.pixels == frame().pixels, "Live setup art/padding/shadow changed an existing page");
        tap(Key::up); tap(Key::down); tick(settled);
        require(old_art.pixels == frame().pixels, "Focus callbacks rebased a setup page's captured paper art");
        open(count); tap(Key::down); tick(settled); const Frame new_art = frame();
        require(old_art.pixels != new_art.pixels, "Reopening did not adopt authored setup art/padding/shadow");
        source.art = restore_art.art; source.setup_focus_padding = restore_art.padding; source.setup_shadow_offset = restore_art.shadow;
        tick(); require(new_art.pixels == frame().pixels, "Source reversal rebased a reopened setup paper page");
        tap(Key::up); tap(Key::down); tick(settled);
        require(new_art.pixels == frame().pixels, "Focus callbacks lost a reopened setup page's captured paper art");
        open(count); tap(Key::down); tick(settled);
        require(old_art.pixels == frame().pixels, "Reopening did not restore exact default setup paper art");
    }
    {
        struct RestoreDurationColumn
        {
            float& value; float original;
            ~RestoreDurationColumn() { this->value = this->original; }
        } restore_column{ source.mode_select_spacing.x, source.mode_select_spacing.x };
        // The value anchor still has positive interior width; its actual text
        // does not. This reaches the compound-row runtime guard deliberately.
        source.mode_select_spacing.x = 800.0f;
        MenuLevelSettings invalid_settings; MenuInput invalid_input(app.gamepads(), app.keyboard(), &tuning);
        MenuContext invalid_context(&data, &invalid_input, &invalid_settings); StateContext invalid_stack;
        bool rejected = false;
        try { invalid_stack.push(std::make_unique<MainMenuModeSelect>(&invalid_context)); }
        catch (const std::runtime_error& error)
        {
            rejected = std::string(error.what()).find("the complete duration row must fit main_menu.setup_box_size") != std::string::npos;
        }
        require(rejected, "Actual Mode accepted an overflowing compound duration value or reported the wrong diagnostic");
    }
    require(records.good(), "Setup paper motion evidence write failed");
    require(IsWindowVisible(static_cast<HWND>(app.window())) == FALSE && GetForegroundWindow() != static_cast<HWND>(app.window()),
        "Setup paper probes exposed or activated their hidden window");
    std::cout << "PASS actual setup paper: six-size independent full-page geometry/copy/card/arrow fixtures and contrast; "
        "native Mode/Count arrow and duration press initial/middle/rest, fixed lettering, reduction, lifecycle resets, "
        "frozen draw/RNG and resize; normal reader, keyboard-driven only\n";
}
