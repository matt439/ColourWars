// Small text-only native check. Device snapshots are scripted, while each page,
// input gate, text refresh and presentation snapshot is the actual game code.
void connection_text_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    using Words = std::array<std::wstring, 8>;
    const Words original_words{ L" - Reconnect", L"Everyone joined. Player 1 can continue.",
        L"Press A / Enter on each device to join. Reconnect reserved pads.", L"Pause Menu",
        L"Pause Menu P{player}", L"Controller lost", L"All players connected.", L"Both teams need a player" };
    const Words replacement_words{ L" < plug back >", L"All claimed. Start when ready.", L"Waiting for a device.",
        L"Paint paused", L"Painter {player} paused", L"Pad missing", L"Devices ready.", L"Choose both sides before confirming." };
    const std::array<std::wstring, 4> titles{ L"Pause Menu P1", L"Pause Menu P2", L"Pause Menu P3", L"Pause Menu P4" };
    const Presentation original = presentation;
    Presentation replacement = original;
    auto& text = const_cast<MenuText&>(replacement.menu_text()).connection;
    text.join_reconnect_suffix = replacement_words[0]; text.join_ready = replacement_words[1];
    text.join_waiting = replacement_words[2]; text.pause_title = replacement_words[3];
    text.pause_player_title = replacement_words[4]; text.controller_lost = replacement_words[5];
    text.all_connected = replacement_words[6]; text.teams_required = replacement_words[7];
    auto& replacement_style = const_cast<MenuStyle&>(replacement.menu());
    replacement_style.heading_font = "gill_sans_mt_bold_48";
    replacement_style.item_font = "gill_sans_mt_bold_24";
    replacement_style.detail_font = "courier_new_bold_16";
    replacement.validate_resources(*app.render_resources(), *app.audio_resources());
    Save preferences; auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0; play.reduce_effects = true;
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false } };
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_tuning(&tuning);
    data.set_presentation(&presentation); data.set_stages(&stages); data.set_level_infos(&levels);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MenuLevelSettings settings, expected;
    MenuContext context(&data, &input, &settings); StateContext& stack = app;
    struct RestoreConnection
    {
        StateContext& stack; Presentation& look; Presentation original;
        ~RestoreConnection() { this->stack.clear(); painter_scripted_pads::states = {}; this->look = this->original; }
    } restore{ stack, presentation, original };
    const std::array<int, 4> slots{ 3, input_slots::keyboard_and_mouse, 0, 2 };
    int ticks = 0, captures = 0, rows = 0;
    std::optional<PauseMenuAction> choice;
    std::ofstream records(output / "connection-text.csv");
    records << "case,role,width,height,font,x,y,text_width,text_height,visible,raster_matches\n";
    const auto tick = [&]
    {
        require(++ticks <= 300, "Connection text exceeded its bounded 300 update budget");
        app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.0f);
    };
    const auto bit = [](GamepadButton button) { return static_cast<uint16_t>(1u << static_cast<unsigned int>(button)); };
    const auto hold = [&](int slot, GamepadButton button, bool down)
    {
        if (slot == input_slots::keyboard_and_mouse)
        {
            const Key key = button == GamepadButton::a ? Key::enter : button == GamepadButton::b ? Key::escape :
                button == GamepadButton::dpad_up ? Key::up : button == GamepadButton::dpad_down ? Key::down :
                button == GamepadButton::dpad_left ? Key::left : Key::right;
            if (down) { app.keyboard()->on_key_down(key); } else { app.keyboard()->on_key_up(key); }
        }
        else
        {
            auto& buttons = painter_scripted_pads::states[static_cast<size_t>(slot)].buttons;
            buttons = down ? static_cast<uint16_t>(buttons | bit(button)) :
                static_cast<uint16_t>(buttons & static_cast<uint16_t>(~bit(button)));
        }
    };
    const auto tap = [&](int slot, GamepadButton button)
        { hold(slot, button, true); tick(); hold(slot, button, false); tick(); };
    const auto connected = [&](int slot, bool value)
        { painter_scripted_pads::states[static_cast<size_t>(slot)].connected = value; tick(); };
    const auto frame = [&] { return main_menu_frame(app); };
    const auto unchanged = [&]
    {
        require(settings.player_count() == expected.player_count() && settings.stage() == expected.stage() &&
            settings.best_of() == expected.best_of() && settings.duration() == expected.duration() &&
            settings.screen_layout() == expected.screen_layout(), "Connection copy changed the match choices");
        const auto actual = settings.player_settings(), wanted = expected.player_settings();
        for (size_t seat = 0; seat < wanted.size(); ++seat)
        {
            require(actual[seat].num == wanted[seat].num && actual[seat].team == wanted[seat].team &&
                actual[seat].weapon == wanted[seat].weapon, "Connection copy changed a roster choice or claim");
        }
    };
    // kind: Join=0, Pause=1, Team=2. The deliberately sparse/reordered slots
    // make player numbers observably different from device numbers.
    const auto open = [&](int kind, int owner = 3)
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::enter, Key::space, Key::escape, Key::up, Key::down, Key::left, Key::right })
            { app.keyboard()->on_key_up(key); }
        painter_scripted_pads::states = {};
        for (int slot : { 3, 0, 2 }) { painter_scripted_pads::states[static_cast<size_t>(slot)].connected = true; }
        app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll();
        app.mouse()->poll(); app.mouse()->poll(); input.reset_actions(); choice.reset();
        settings = MenuLevelSettings(); settings.set_player_count(kind == 0 ? 3 : kind == 1 ? 4 : 2);
        settings.begin_input_claims(3); require(settings.claim_input_slot(4), "Cannot create the connection text sparse roster");
        if (kind == 1)
        {
            require(settings.claim_input_slot(0) && settings.claim_input_slot(2), "Cannot create the four-player Pause mapping");
        }
        settings.set_stage("movement_course"); settings.set_duration(73.0f); settings.set_best_of(3);
        settings.set_screen_layout(kind == 0 ? ScreenLayout::three_player : kind == 1 ? ScreenLayout::four_player : ScreenLayout::two_player);
        for (int seat = 0; seat < settings.player_count(); ++seat)
        {
            settings.set_player_team(seat, kind == 2 || seat % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
            settings.set_player_weapon(seat, WeaponType::sprayer);
        }
        expected = settings;
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        if (kind == 0) { stack.push(std::make_unique<MainMenuJoin>(&context)); }
        else if (kind == 1)
        {
            stack.push<PauseMenuAction>(std::make_unique<PauseMenuInitial>(&context, owner),
                [&](const PauseMenuAction& result) { choice = result; });
        }
        else { stack.push(std::make_unique<MainMenuTeamSelect>(&context)); }
        tick(); tick(); require(stack.depth() == 2 && !choice, "Connection text page consumed neutral entry input");
    };
    // Each oracle draws literal copy in independently supplied bounds/fonts.
    // Only the paper background is shared artwork, outside this text scope.
    const auto compare = [&](const char* name, const char* role, int kind, const Presentation& snapshot,
        const std::wstring& copy, const std::string& font, Vector2F position, const Colour& ink,
        bool centred, bool shadowed, bool visible, bool capture = false)
    {
        unchanged(); const Frame actual = frame(); const MenuStyle& style = snapshot.menu();
        MenuPaperArtPresentation art = kind == 1 ? snapshot.pause_menu().art : snapshot.main_menu().art;
        RectangleF panel;
        if (kind == 1) { panel = RectangleF((menu_consts::DESIGN_RESOLUTION - snapshot.pause_menu().box_size) / 2.0f,
            snapshot.pause_menu().box_size); }
        else
        {
            art.focus_padding = snapshot.main_menu().setup_focus_padding; art.shadow_offset = snapshot.main_menu().setup_shadow_offset;
            const Vector2F size = kind == 0 ? snapshot.main_menu().setup_box_size : snapshot.main_menu().team_box_size;
            panel = RectangleF((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - size) / 2.0f, size);
        }
        const Vector2F size = app.render_resources()->measure_text(app.render_resources()->resolve_sprite_font(font), copy);
        if (centred) { position.x = (menu_consts::DESIGN_RESOLUTION.x - size.x) / 2.0f; }
        std::unique_ptr<UiWidget> text_reference;
        if (shadowed)
        {
            const Vector2F offset = std::string(role) == "join-row" ? style.item_shadow_offset : style.detail_shadow_offset;
            text_reference = std::make_unique<UiTextDropShadow>("literal-connection-copy", copy, font, position,
                app.render_resources(), ink, Colour{}, offset, !visible);
        }
        else { text_reference = std::make_unique<UiText>("literal-connection-copy", copy, font, position,
            app.render_resources(), ink, !visible); }
        const RectangleF text_bounds = text_reference->bounds();
        require(panel.contains(text_bounds) && text_bounds.width > 0.0f && text_bounds.height > 0.0f,
            "Connection text's complete unscaled bounds escape their paper");
        if (kind == 1 && (std::string(role) == "pause-title" || std::string(role) == "pause-status"))
        {
            require(text_bounds.bottom() < snapshot.pause_menu().layout.position.y - art.focus_padding.y,
                "Pause connection copy reaches the first action card");
        }
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); const auto viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
        list.set_viewport(viewport); list.set_camera(camera); list.set_filter(TextureFilter::point);
        canvas.draw(list, camera.visible_rectangle(viewport)); FocusGroup no_focus;
        const MenuPaperArtwork paper(panel, app.render_resources(), &style, &art, &no_focus); paper.draw(list);
        if (kind != 1) { list.set_filter(TextureFilter::linear); }
        text_reference->draw(list); const Frame wanted = read(renderer);
        RectangleF bounds = text_bounds; bounds.inflate(Vector2F(2.0f)); camera.calculate_view_rectangle(bounds);
        const RectangleI crop = countdown_crop_bounds(bounds);
        const Frame observed = foreground_crop(actual, crop, 0), reference = foreground_crop(wanted, crop, 0);
        const std::string stem = std::string("connection-text-") + name + '-' + role + '-' + std::to_string(actual.width);
        if (observed.pixels != reference.pixels)
        {
            if (captures <= 6) { save(observed, output / (stem + "-failure-actual.ppm"));
                save(reference, output / (stem + "-failure-expected.ppm")); captures += 2; }
            std::cerr << stem << " crop=" << crop.x << ',' << crop.y << ',' << crop.width << ',' << crop.height << '\n';
            throw std::runtime_error("Actual connection copy differs from the independent literal text raster");
        }
        records << name << ',' << role << ',' << actual.width << ',' << actual.height << ',' << font << ',' <<
            text_bounds.x << ',' << text_bounds.y << ',' << text_bounds.width << ',' << text_bounds.height << ',' <<
            (visible ? "true" : "false") << ",true\n"; ++rows;
        if (capture) { require(captures < 8, "Connection text exceeded eight compact captures");
            save(observed, output / (stem + "-detail.ppm")); ++captures; }
        std::srand(9241); const int next = std::rand(); std::srand(9241);
        require(actual.pixels == frame().pixels && std::rand() == next, "Connection text drawing changed frozen pixels or RNG");
    };
    const auto join = [&](const char* name, const Presentation& snapshot, const Words& words, bool missing,
        bool ready, bool capture = false)
    {
        const Vector2F base = snapshot.menu().layout.position, spacing = snapshot.main_menu().player_count_spacing;
        compare(name, "join-row", 0, snapshot, std::wstring(L"P1 - Pad 4") + (missing ? words[0] : L""),
            snapshot.menu().item_font, base + Vector2F(0.0f, spacing.y * 2.0f), missing ?
            snapshot.main_menu().art.disabled_ink : snapshot.main_menu().art.focus_ink, false, true, true, capture);
        compare(name, "join-status", 0, snapshot, words[ready ? 1 : 2], snapshot.menu().detail_font,
            base + Vector2F(0.0f, spacing.y * 7.0f), snapshot.main_menu().art.ink, false, true, true);
    };
    const auto pause = [&](const char* name, const Presentation& snapshot, const std::wstring& title,
        const std::wstring& status, bool capture = false)
    {
        compare(name, "pause-title", 1, snapshot, title, snapshot.menu().heading_font,
            Vector2F(0.0f, snapshot.pause_menu().heading_y), snapshot.pause_menu().art.ink, true, false, true, capture);
        compare(name, "pause-status", 1, snapshot, status, snapshot.menu().detail_font,
            Vector2F(0.0f, snapshot.pause_menu().status_y), snapshot.pause_menu().art.ink, true, false, true);
    };
    const auto warning = [&](const char* name, const Presentation& snapshot, const Words& words, bool visible, bool capture = false)
    {
        compare(name, "teams-required", 2, snapshot, words[7], snapshot.menu().detail_font,
            snapshot.main_menu().team_warning_position, snapshot.main_menu().art.ink, false, true, visible, capture);
    };
    const auto invalid_team = [&] { tap(3, GamepadButton::a); tap(4, GamepadButton::a); };
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080) })
    {
        presentation = original; app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        open(0); join("unclaimed", original, original_words, false, false);
        connected(3, false); join("join-missing", original, original_words, true, false, true);
        connected(3, true); tick(); join("returned-no-auto-join", original, original_words, false, false);
        require(settings.player_settings()[2].num == -1, "A connected neutral device autojoined the sparse roster");
        tap(0, GamepadButton::a); require(expected.claim_input_slot(0), "Cannot specify the accepted sparse claim");
        join("joined-ready", original, original_words, false, true);
        for (int player = -1; player < 4; ++player)
        {
            open(1, player < 0 ? -1 : slots[static_cast<size_t>(player)]);
            pause(player < 0 ? "fallback" : (std::string("number-") + std::to_string(player + 1)).c_str(),
                original, player < 0 ? original_words[3] : titles[static_cast<size_t>(player)], original_words[6]);
        }
        open(1); tap(3, GamepadButton::dpad_down); tap(3, GamepadButton::a);
        connected(0, false); require(stack.depth() == 2 && !choice, "Restart confirmation ignored a newly missing participant");
        pause("restart-loss", original, original_words[5], L"P3 - Pad 1", true);
        for (int row = 0; row < 2; ++row)
        {
            compare("missing-disabled", row == 0 ? "resume" : "restart", 1, original, row == 0 ? L"Resume" : L"Restart",
                original.menu().item_font, original.pause_menu().layout.position + Vector2F(0.0f,
                original.pause_menu().layout.spacing.y * static_cast<float>(row)), original.pause_menu().art.disabled_ink,
                false, false, true);
        }
        tap(3, GamepadButton::b); require(stack.depth() == 2 && !choice, "Resume accepted Back while a participant was missing");
        hold(3, GamepadButton::b, true); tick(); connected(0, true); tick();
        pause("returned-held", original, titles[0], original_words[6]);
        hold(3, GamepadButton::b, false); tick(); require(stack.depth() == 2 && !choice, "Reconnect consumed a held/released Resume action");
        tap(3, GamepadButton::b); require(choice == PauseMenuAction::resume && stack.depth() == 1,
            "A fresh Back after reconnect did not resume");
        open(2); warning("hidden-warning", original, original_words, false); invalid_team();
        warning("both-teams-required", original, original_words, true, true);
        tap(4, GamepadButton::dpad_right); tap(4, GamepadButton::dpad_right);
        warning("valid-provisional-teams", original, original_words, false);
    }
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    presentation = original; open(0); connected(3, false); const Frame old_join = frame();
    presentation = replacement; tick(); join("assigned-join-keeps-old", original, original_words, true, false);
    require(frame().pixels == old_join.pixels, "An open Join adopted assigned connection text or fonts");
    connected(3, true); tap(0, GamepadButton::a); require(expected.claim_input_slot(0), "Cannot specify captured Join claim");
    join("old-join-ready-refresh", original, original_words, false, true);
    open(0); connected(3, false); join("reopened-new-join", replacement, replacement_words, true, false);
    connected(3, true); tap(0, GamepadButton::a); require(expected.claim_input_slot(0), "Cannot specify replacement Join claim");
    join("new-join-ready", replacement, replacement_words, false, true);
    compare("new-join-ready", "join-status", 0, replacement, replacement_words[1], replacement.menu().detail_font,
        replacement.menu().layout.position + Vector2F(0.0f, replacement.main_menu().player_count_spacing.y * 7.0f),
        replacement.main_menu().art.ink, false, true, true, true);
    presentation = original; connected(3, false); join("restored-open-join-keeps-new", replacement, replacement_words, true, false);
    open(0); join("reopened-join-reversed", original, original_words, false, false);
    open(1, 2); presentation = replacement; connected(0, false);
    pause("assigned-pause-keeps-old", original, original_words[5], L"P3 - Pad 1");
    connected(0, true); pause("old-pause-return-refresh", original, titles[3], original_words[6]);
    open(1, 2); pause("reopened-new-pause", replacement, L"Painter 4 paused", replacement_words[6], true);
    connected(0, false); const Frame new_lost = frame(); presentation = original; tick();
    pause("restored-open-pause-keeps-new", replacement, replacement_words[5], L"P3 - Pad 1");
    require(frame().pixels == new_lost.pixels, "An open Pause adopted restored connection text or fonts");
    connected(0, true); pause("new-pause-return-refresh", replacement, L"Painter 4 paused", replacement_words[6]);
    open(1, 2); pause("reopened-pause-reversed", original, titles[3], original_words[6]);
    presentation = replacement; open(1, -1); pause("custom-fallback", replacement, replacement_words[3], replacement_words[6]);
    presentation = original; tick(); pause("custom-fallback-snapshot", replacement, replacement_words[3], replacement_words[6]);
    open(1, -1); pause("fallback-reversed", original, original_words[3], original_words[6]);
    open(2); invalid_team(); const Frame old_warning = frame(); presentation = replacement; tick();
    warning("assigned-warning-keeps-old", original, original_words, true);
    require(frame().pixels == old_warning.pixels, "An open Team warning adopted assigned copy or fonts");
    open(2); invalid_team(); warning("reopened-new-warning", replacement, replacement_words, true);
    const Frame new_warning = frame(); presentation = original; tick(); warning("restored-warning-keeps-new", replacement, replacement_words, true);
    require(frame().pixels == new_warning.pixels, "An open Team warning adopted restored copy or fonts");
    open(2); invalid_team(); warning("warning-reversed", original, original_words, true);
    require(captures == 8 && rows > 0 && records.good(), "Connection text evidence is incomplete or exceeded its bounded crop budget");
    std::cout << "PASS connection text: literal full-bounds Join/Pause/Team rasters at720/1080, sparse claims and all four player/device mappings, reconnect fresh-press Resume/Restart gates, custom copy/font captured refresh/reopen/reversal; " <<
        rows << " CSV checks, " << ticks << " page updates, eight compact crops; no physical-pad, foreground, listening or human readability acceptance\n";
}
