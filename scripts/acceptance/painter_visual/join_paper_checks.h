// Only the reader is scripted. Expected words, roster slots, connection state,
// text/shadow unions and paper/gutter raster are independent of the Join page.
void join_paper_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    const Presentation original = presentation;
    Save preferences; auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0; play.reduce_effects = false;
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false } };
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_tuning(&tuning);
    data.set_presentation(&presentation); data.set_stages(&stages); data.set_level_infos(&levels);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings); StateContext& stack = app;
    MenuPage* active = nullptr;
    struct ClearJoinPaper
    {
        StateContext& stack; Presentation& presentation; Presentation original;
        ~ClearJoinPaper() { this->stack.clear(); painter_scripted_pads::states = {}; this->presentation = this->original; }
    } clear{ stack, presentation, original };
    std::array<int, 4> slots{};
    std::vector<int> expected_slots;
    int expected_host_row = 1;
    std::array<bool, input_slots::count> connected{};
    std::vector<RectangleF> initial_rows;
    const auto bit = [](GamepadButton button) { return static_cast<uint16_t>(1u << static_cast<unsigned int>(button)); };
    const auto tick = [&](float elapsed = 0.0f)
        { app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed); };
    const auto hold = [&](int slot, GamepadButton button, bool down)
    {
        if (slot == input_slots::keyboard_and_mouse)
        {
            const Key key = button == GamepadButton::a ? Key::enter : Key::escape;
            if (down) { app.keyboard()->on_key_down(key); } else { app.keyboard()->on_key_up(key); }
        }
        else
        {
            auto& buttons = painter_scripted_pads::states[static_cast<size_t>(slot)].buttons;
            buttons = down ? static_cast<uint16_t>(buttons | bit(button)) : static_cast<uint16_t>(buttons & static_cast<uint16_t>(~bit(button)));
        }
    };
    const auto tap = [&](int slot, GamepadButton button)
        { hold(slot, button, true); tick(); hold(slot, button, false); tick(); };
    const auto frame = [&] { return main_menu_frame(app); };
    const auto set_connected = [&](int slot, bool value)
    {
        require(slot >= 0 && slot < input_slots::pad_count, "Join paper fixture tried to disconnect keyboard input");
        connected[static_cast<size_t>(slot)] = value;
        painter_scripted_pads::states[static_cast<size_t>(slot)].connected = value; tick();
    };
    const auto label = [&](int player)
    {
        const std::array<std::wstring, 4> names{ L"P1", L"P2", L"P3", L"P4" };
        const std::array<std::wstring, 5> devices{ L"Pad 1", L"Pad 2", L"Pad 3", L"Pad 4", L"Keyboard/mouse" };
        const int slot = expected_slots[static_cast<size_t>(player)];
        std::wstring words = names[static_cast<size_t>(player)] + L" - " +
            (slot < 0 ? L"Waiting to join" : devices[static_cast<size_t>(slot)]);
        if (slot >= 0 && !connected[static_cast<size_t>(slot)]) { words += L" - Reconnect"; }
        return words;
    };
    const auto position = [](const Presentation& snapshot, int row)
    {
        return snapshot.menu().layout.position + Vector2F(0.0f,
            snapshot.main_menu().player_count_spacing.y * static_cast<float>(row));
    };
    const auto text_bounds = [&](const Presentation& snapshot, int player)
    {
        const auto& style = snapshot.menu();
        const colourwars::UiTextDropShadow text("independent-join-seed", label(player), style.item_font,
            position(snapshot, player + 2), app.render_resources(), snapshot.main_menu().art.ink,
            Colour{}, style.item_shadow_offset);
        return text.bounds();
    };
    const auto open = [&](int count, bool pad_first)
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (Key key : { Key::enter, Key::escape }) { app.keyboard()->on_key_up(key); }
        painter_scripted_pads::states = {}; connected = {};
        for (int slot : { 0, 2, 3 }) { connected[static_cast<size_t>(slot)] = true; painter_scripted_pads::states[static_cast<size_t>(slot)].connected = true; }
        connected[input_slots::keyboard_and_mouse] = true;
        slots = pad_first ? std::array<int, 4>{ 3, input_slots::keyboard_and_mouse, 0, 2 } :
            std::array<int, 4>{ input_slots::keyboard_and_mouse, 3, 0, 2 };
        app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll();
        app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        settings = MenuLevelSettings(); settings.set_player_count(count); settings.set_best_of(1);
        settings.set_stage("movement_course"); settings.set_duration(73.0f);
        settings.set_screen_layout(count == 1 ? ScreenLayout::one_player : count == 2 ? ScreenLayout::two_player :
            count == 3 ? ScreenLayout::three_player : ScreenLayout::four_player);
        settings.begin_input_claims(slots[0]); expected_slots.assign(static_cast<size_t>(count), -1); expected_slots[0] = slots[0];
        expected_host_row = 1;
        for (int player = 0; player < count; ++player) { settings.set_player_team(player, player % 2 == 0 ? PlayerTeam::a : PlayerTeam::b); }
        initial_rows.clear(); for (int player = 0; player < count; ++player) { initial_rows.push_back(text_bounds(presentation, player)); }
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        auto page = std::make_unique<MainMenuJoin>(&context); active = page.get(); stack.push(std::move(page)); tick(); tick();
        require(stack.depth() == 2, "Join paper did not remain on its neutral initial page");
    };
    const auto assert_plan = [&]
    {
        const auto roster = settings.player_settings(); require(roster.size() == expected_slots.size(), "Join paper changed its roster size");
        for (size_t player = 0; player < roster.size(); ++player)
            { require(roster[player].num == expected_slots[player], "Join paper moved a stable device assignment"); }
        require(settings.best_of() == 1 && settings.stage() == "movement_course" && settings.duration() == std::optional<float>(73.0f),
            "Join paper changed the assembled match plan");
    };
    const auto frozen = [&](const Frame& image)
    {
        std::srand(8317); const int next = std::rand(); std::srand(8317);
        require(image.pixels == frame().pixels && image.pixels == frame().pixels && std::rand() == next,
            "Frozen Join paper drawing changed pixels or consumed RNG"); assert_plan();
    };
    std::ofstream records(output / "join-paper.csv");
    records << "case,width,height,players,p1_slot,acknowledgement_fraction,full_frame_matches\n";
    const auto compare = [&](const std::string& name, const Presentation& snapshot, const std::wstring& heading,
        int responding = -1, float fraction = 0.0f, bool capture = false)
    {
        const Frame actual = frame(); const auto& style = snapshot.menu(); const auto& look = snapshot.main_menu();
        auto art = look.art; art.focus_padding = look.setup_focus_padding; art.shadow_offset = look.setup_shadow_offset;
        std::vector<std::unique_ptr<colourwars::UiTextDropShadow>> text;
        const auto add = [&](const std::wstring& words, const std::string& font, int row, const Colour& ink, Vector2F shadow)
        {
            auto widget = std::make_unique<colourwars::UiTextDropShadow>("independent-join-text", words, font,
                position(snapshot, row), app.render_resources(), ink, Colour{}, shadow);
            text.push_back(std::move(widget));
        };
        add(heading, style.heading_font, 0, art.ink, style.heading_shadow_offset);
        bool ready = true;
        for (size_t player = 0; player < expected_slots.size(); ++player)
        {
            const int slot = expected_slots[player]; const bool available = slot >= 0 && connected[static_cast<size_t>(slot)];
            ready = ready && available;
            add(label(static_cast<int>(player)), style.item_font, static_cast<int>(player) + 2,
                available ? art.focus_ink : art.disabled_ink, style.item_shadow_offset);
        }
        add(ready ? L"Everyone joined. Player 1 can continue." :
            L"Press A / Enter on each device to join. Reconnect reserved pads.", style.detail_font, 7, art.ink, style.detail_shadow_offset);
        const RectangleF panel((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - look.setup_box_size) / 2.0f, look.setup_box_size);
        const float edge = art.keyline_width * 3.0f;
        const RectangleF inside(panel.position() + Vector2F(edge), panel.size() - Vector2F(edge * 2.0f + art.shadow_offset));
        for (const auto& widget : text) { require(inside.contains(widget->bounds()), "Independent unscaled Join text/shadow escapes paper interior"); }
        require(text.front()->bounds().bottom() <= text[1]->bounds().y &&
            text[text.size() - 2]->bounds().bottom() <= text.back()->bounds().y,
            "Independent Join heading, final roster row and status overlap");
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
        DrawList list = renderer.view(0); list.set_viewport(viewport); list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        canvas.draw(list, RectangleF(Vector2F::ZERO, app.resolution_manager()->resolution_vec())); list.set_camera(camera);
        const SpriteSheet* sheet = app.render_resources()->sprite_sheet(style.sprite_sheet); const auto fill = sheet->resolve_sprite_frame(style.fill_frame);
        sheet->draw(list, fill, camera.visible_rectangle(viewport), style.scrim_colour);
        const UiTexture background("independent-join-surround", style.sprite_sheet, style.fill_frame,
            RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), app.render_resources(), look.home_background_colour);
        background.draw(list); setup_paper_reference(list, *app.render_resources(), style, art, panel, RectangleF());
        if (responding >= 0 && fraction > 0.0f)
        {
            const RectangleF seed = initial_rows[static_cast<size_t>(responding)];
            const float width = std::min(style.accent_width, style.focus_padding.x);
            const RectangleF mark(seed.x - style.focus_padding.x + (style.focus_padding.x - width) * fraction,
                seed.center().y - seed.height * fraction / 2.0f, width, seed.height * fraction);
            require(inside.contains(mark), "Independent Join acknowledgement escapes the paper interior");
            sheet->draw(list, fill, mark, art.focus_ink);
        }
        // Keep the host's selected row when an earlier human leaves. A claim
        // that takes that selected row advances it to the first editable row.
        const auto editable = [&](int seat)
        {
            return seat > 0 && static_cast<size_t>(seat) < expected_slots.size() &&
                expected_slots[static_cast<size_t>(seat)] < 0;
        };
        if (!editable(expected_host_row))
        {
            expected_host_row = 0;
            for (size_t seat = 1; seat < expected_slots.size(); ++seat)
            {
                if (expected_slots[seat] < 0) { expected_host_row = static_cast<int>(seat); break; }
            }
        }
        if (editable(expected_host_row))
        {
            const RectangleF row = text[static_cast<size_t>(expected_host_row) + 1]->bounds();
            const RectangleF cursor(row.x, row.bottom() + style.rule_height, row.width, style.rule_height);
            require(inside.contains(cursor), "Independent host row underline escapes Join paper");
            sheet->draw(list, fill, cursor, art.focus_ink);
        }
        sheet->draw(list, fill, RectangleF(0.0f, style.footer_y, menu_consts::DESIGN_RESOLUTION.x,
            menu_consts::DESIGN_RESOLUTION.y - style.footer_y), style.scrim_colour);
        sheet->draw(list, fill, RectangleF(style.brand_position.x, style.footer_y,
            menu_consts::DESIGN_RESOLUTION.x - style.brand_position.x * 2.0f, style.rule_height), style.panel_edge_colour);
        list.set_filter(TextureFilter::linear); for (const auto& widget : text) { widget->draw(list); }
        const auto& footer = snapshot.prompt_bar();
        const colourwars::UiTextDropShadow prompt("independent-join-prompt", L"A/Enter Join / P1 continue    B/Esc Leave / back    X/R Profile    P1: Up/Down Row, Left/Right Human / Bot",
            footer.font, footer.position, app.render_resources(), footer.colour, footer.shadow_colour, footer.shadow_offset);
        prompt.draw(list); const Frame expected = read(renderer);
        const std::string stem = "join-paper-" + name + '-' + std::to_string(settings.player_count()) + "p-" +
            std::to_string(slots[0]) + '-' + std::to_string(actual.width) + 'x' + std::to_string(actual.height);
        if (actual.pixels != expected.pixels)
        {
            save(actual, output / (stem + "-failure-actual.ppm")); save(expected, output / (stem + "-failure-expected.ppm"));
            const RectangleI delta = foreground_delta_bounds(actual, expected);
            std::cerr << "JOIN PAPER " << name << " delta=" << delta.x << ',' << delta.y << ',' << delta.width << ',' << delta.height << '\n';
        }
        require(actual.pixels == expected.pixels, "Actual Join paper differs from its independent full text/paper/gutter fixture");
        for (const auto& widget : text)
        {
            RectangleF bounds = widget->bounds(); camera.calculate_view_rectangle(bounds); const RectangleI pixels = countdown_crop_bounds(bounds);
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
                "Actual Join text lost dark ink, pale paper or native raster contrast");
        }
        if (capture)
        {
            save(actual, output / (stem + ".ppm"));
            save(foreground_crop(actual, countdown_crop_bounds(camera.calculate_view_rectangle(panel)), 0), output / (stem + "-detail.ppm"));
        }
        frozen(actual); records << name << ',' << actual.width << ',' << actual.height << ',' << settings.player_count() << ',' <<
            slots[0] << ',' << fraction << ",true\n"; return actual;
    };
    const auto destination = [&](int kind)
    {
        MenuLevelSettings plan = settings; MenuInput reference_input(app.gamepads(), app.keyboard(), &tuning);
        MenuContext reference_context(&data, &reference_input, &plan); StateContext reference_stack;
        reference_stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        if (kind == 0) { reference_stack.push(std::make_unique<MainMenuPlayerCount>(&reference_context)); }
        else if (kind == 1) { reference_stack.push(std::make_unique<MainMenuWeaponSelect>(&reference_context)); }
        else { reference_stack.push(std::make_unique<MainMenuTeamSelect>(&reference_context)); }
        app.renderer()->begin_frame(); reference_stack.draw(*app.renderer()); return read(*app.renderer());
    };
    const float settled = std::max(original.menu().motion.focus_duration, original.menu().motion.press_duration) + 0.01f;
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440),
        Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        for (int count = 1; count <= 4; ++count)
        {
            for (bool pad_first : { false, true })
            {
                open(count, pad_first); const bool capture = count == 4 || size.x == 1280;
                const Frame neutral = compare("neutral", original, L"Join Players", -1, 0.0f, capture);
                tick(settled); require(neutral.pixels == frame().pixels, "New Join paper animated without an accepted claim");
                for (int player = 1; player < count; ++player)
                    { tap(slots[static_cast<size_t>(player)], GamepadButton::a); expected_slots[static_cast<size_t>(player)] = slots[static_cast<size_t>(player)]; }
                tick(settled); compare("ready", original, L"Join Players", -1, 0.0f, capture);
                const int missing_slot = pad_first ? slots[0] : count > 1 ? slots[static_cast<size_t>(count - 1)] : -1;
                if (missing_slot >= 0 && missing_slot != input_slots::keyboard_and_mouse)
                {
                    set_connected(missing_slot, false); compare("reconnect-reserved", original, L"Join Players", -1, 0.0f, capture);
                    set_connected(missing_slot, true); tick(); compare("reconnected", original, L"Join Players", -1, 0.0f, capture);
                }
                tap(slots[0], GamepadButton::a);
                require(stack.depth() == 2 && frame().pixels == destination(count == 1 ? 1 : 2).pixels,
                    "Complete Join paper did not immediately reach its ordinary setup destination");
            }
        }
        std::cout << "PASS static Join paper " << size.x << 'x' << size.y << '\n';
    }
    const auto motion = original.menu().motion;
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
    {
        app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        open(4, false); tap(slots[1], GamepadButton::a); expected_slots[1] = slots[1];
        const Frame moving = compare("claim-initial", original, L"Join Players", 1, motion.press_fraction, true);
        tick(0.0f); tick(-0.5f); require(moving.pixels == frame().pixels, "Nonpositive time advanced Join paper acknowledgement");
        play.reduce_effects = true; compare("reduced-immediate", original, L"Join Players"); play.reduce_effects = false;
        require(moving.pixels == frame().pixels, "Reduced Join paper drawing consumed its acknowledgement clock");
        tick(motion.press_duration * 0.4f);
        const Frame middle = compare("claim-middle", original, L"Join Players", 1, motion.press_fraction * 0.6f, true);
        tick(settled); const Frame rest = compare("claim-rest", original, L"Join Players", -1, 0.0f, true);
        require(moving.pixels != middle.pixels && middle.pixels != rest.pixels, "Join paper acknowledgement did not progress and expire");
        tap(slots[1], GamepadButton::b); expected_slots[1] = -1; stack.notify_activation(false);
        compare("deactivated", original, L"Join Players"); stack.notify_activation(true); tick(settled);
        compare("reactivated", original, L"Join Players");
        tap(slots[1], GamepadButton::a); expected_slots[1] = slots[1];
        stack.push(std::make_unique<MainMenuCredits>(&context)); stack.pop(); compare("cover-return", original, L"Join Players"); tick();
        tap(slots[1], GamepadButton::b); expected_slots[1] = -1; active->debug_reset_input(slots[0]);
        compare("developer-reset", original, L"Join Players");
        play.reduce_effects = true; tap(slots[1], GamepadButton::a); expected_slots[1] = slots[1];
        const Frame quiet = compare("reduced-input", original, L"Join Players"); tick(settled);
        require(quiet.pixels == frame().pixels, "Reduced Join paper input restarted acknowledgement"); play.reduce_effects = false;
        app.resolution_manager()->set_resolution_exactly({ 1600, 1200 }); std::ignore = app.renderer()->window_size_changed(1600, 1200);
        std::ignore = frame(); app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        require(quiet.pixels == frame().pixels, "Join paper changed after a resize round-trip");
    }
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 }); std::ignore = app.renderer()->window_size_changed(1280, 720);
    Presentation old = original, next = original;
    const_cast<MenuText&>(old.menu_text()).headings[static_cast<size_t>(MenuHeading::join_players)] = L"Join~ Players";
    const_cast<MenuText&>(next.menu_text()).headings[static_cast<size_t>(MenuHeading::join_players)] = L"Gather Players";
    auto& next_style = const_cast<MenuStyle&>(next.menu());
    next_style.heading_font = "lato_black_48"; next_style.item_font = "lato_black_36"; next_style.detail_font = "lato_black_24";
    auto& next_art = const_cast<MainMenuPresentation&>(next.main_menu()).art;
    next_art.paper = Colour(0.90f, 0.95f, 0.98f); next_art.ink = Colour(0.04f, 0.09f, 0.17f);
    next_art.focus_ink = Colour(0.02f, 0.04f, 0.09f); next_art.disabled_ink = Colour(0.30f, 0.32f, 0.36f);
    old.validate_resources(*app.render_resources(), *app.audio_resources()); next.validate_resources(*app.render_resources(), *app.audio_resources());
    presentation = old; open(3, false); compare("old-copy-font-art", old, L"Join~ Players");
    presentation = next; tap(slots[1], GamepadButton::a); expected_slots[1] = slots[1]; tick(settled);
    compare("old-refresh-after-replacement", old, L"Join~ Players");
    set_connected(slots[1], false); compare("old-reconnect-after-replacement", old, L"Join~ Players");
    set_connected(slots[1], true); tick(); compare("old-restored-after-replacement", old, L"Join~ Players");
    open(3, false); const Frame new_page = compare("reopened-new-copy-font-art", next, L"Gather Players", -1, 0.0f, true);
    presentation = original; tick(); require(new_page.pixels == frame().pixels, "Source reversal rebased an existing Join paper page");
    tap(slots[1], GamepadButton::a); expected_slots[1] = slots[1]; tick(settled);
    compare("new-refresh-after-reversal", next, L"Gather Players");
    set_connected(slots[1], false); compare("new-reconnect-after-reversal", next, L"Gather Players");
    open(2, true); tap(slots[1], GamepadButton::a); expected_slots[1] = slots[1]; tick(settled);
    set_connected(slots[0], false); compare("missing-p1-recovery", original, L"Join Players", -1, 0.0f, true);
    tap(input_slots::keyboard_and_mouse, GamepadButton::b);
    require(frame().pixels == destination(0).pixels, "Missing-P1 Join rescue did not reach Player Count");
    open(1, false); hold(slots[0], GamepadButton::a, true); tick(); static_cast<WindowNotify&>(app).on_deactivated(); tick();
    static_cast<WindowNotify&>(app).on_activated(); tick(); hold(slots[0], GamepadButton::a, false); tick(); tick();
    compare("interrupted-owner-confirm", original, L"Join Players");
    tap(slots[0], GamepadButton::b); require(frame().pixels == destination(0).pixels, "Join owner Back did not reach Player Count");
    {
        auto& live_style = const_cast<MenuStyle&>(presentation.menu());
        struct RestoreJoinDetailFont
        {
            std::string& font; std::string original;
            ~RestoreJoinDetailFont() { this->font = this->original; }
        } restore_font{ live_style.detail_font, live_style.detail_font };
        live_style.detail_font = "lato_black_144";
        MenuLevelSettings invalid_settings; invalid_settings.set_player_count(1); invalid_settings.begin_input_claims(input_slots::keyboard_and_mouse);
        MenuInput invalid_input(app.gamepads(), app.keyboard(), &tuning);
        MenuContext invalid_context(&data, &invalid_input, &invalid_settings); StateContext invalid_stack;
        bool rejected = false;
        try { invalid_stack.push(std::make_unique<MainMenuJoin>(&invalid_context)); }
        catch (const std::runtime_error& error) { rejected = std::string(error.what()).find("Join status must fit the paper") != std::string::npos; }
        require(rejected, "Actual Join accepted overflowing status-font bounds or reported the wrong diagnostic");
    }
    require(records.good(), "Join paper evidence write failed");
    require(IsWindowVisible(static_cast<HWND>(app.window())) == FALSE && GetForegroundWindow() != static_cast<HWND>(app.window()),
        "Join paper probes exposed or activated their hidden window");
    std::cout << "PASS actual Join paper: independent full paper/text/gutter rasters, one-to-four seats with keyboard/pad P1 "
        "at six sizes, connected/reserved/reconnect status, unscaled bounds/contrast, captured copy/font/art through refresh/reopen/reversal, "
        "acknowledgement progression, reduction/lifecycle/frozen RNG, recovery routes and resize; scripted reader only\n";
}
