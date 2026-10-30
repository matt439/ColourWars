// Shared menu identities, checked against supplied literal strings rather
// than MenuText's new device/roster formatters or the roster label helper.
void menu_label_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    const Presentation original = presentation; Presentation alternate = original;
    auto& labels = const_cast<MenuText&>(alternate.menu_text()).labels;
    labels.pad = L"Port {pad}"; labels.keyboard_mouse = L"Keys"; labels.waiting = L"Join";
    labels.roster_row = L"{device} / {player}";
    auto& fonts = const_cast<MenuStyle&>(alternate.menu());
    fonts.item_font = "lato_black_36"; fonts.detail_font = "lato_black_24";
    auto& controls = const_cast<ControlsSettingsPresentation&>(alternate.controls_settings());
    controls.item_font = "lato_black_24"; controls.help_font = "courier_prime_bold_16";
    const_cast<ResultsMenuPresentation&>(alternate.results_menu()).contribution_font = "courier_prime_bold_16";
    alternate.validate_resources(*app.render_resources(), *app.audio_resources());
    const NestedSaveProfile isolated(output / "menu-label-profile"); Save preferences;
    PlaySettings play = preferences.play_settings(); play.music_volume = play.sfx_volume = 0; play.reduce_effects = true;
    require(preferences.set_play_settings_and_save(play), "Cannot seed the private menu-label profile");
    const auto saved_bytes = [&]
    {
        std::ifstream file(output / "menu-label-profile/ColourWars/save_data.json", std::ios::binary);
        require(file.good(), "Cannot read the private menu-label save");
        return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    };
    const std::string saved = saved_bytes();
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false } };
    GameData data; data.set_application(&app); data.set_save(&preferences);
    const StageCatalog stage_catalog = acceptance::catalog_of(levels, stages, { "movement_course" }); data.set_stage_catalog(&stage_catalog); data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MatchSetup settings, expected;
    MenuContext context(&data, &input, &settings); StateContext& stack = app;
    struct RestoreMenuLabels
    {
        StateContext& stack; Presentation& look; Presentation original;
        ~RestoreMenuLabels() { this->stack.clear(); painter_scripted_pads::states = {}; this->look = this->original; }
    } restore{ stack, presentation, original };
    constexpr std::array<int, 4> sparse{ 3, 4, 0, 2 }, pads{ 3, 0, 2, 1 };
    constexpr std::array<WeaponType, 4> tools{ WeaponType::sprayer, WeaponType::sniper, WeaponType::roller, WeaponType::bucket };
    LevelEndInfo result; result.total_area = 100.0f; result.team_a_area = 44.375f; result.team_b_area = 55.625f;
    result.team_colours = TeamColourTools::team_colours(TeamColourPair::blue_orange, original.team_palette());
    result.player_areas[3] = 12.375f; result.player_areas[4] = 21.625f; result.player_areas[0] = 32.0f; result.player_areas[2] = 34.0f;
    const LevelEndInfo result_before = result; Session single; single.finish_round(result);
    int ticks = 0, captures = 0, rows = 0;
    const auto tick = [&](float elapsed = 0.0f)
    {
        require(++ticks <= 120, "Menu labels exceeded the bounded120 update budget");
        app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed);
    };
    const auto key = [&](Key value)
        { app.keyboard()->on_key_down(value); tick(); app.keyboard()->on_key_up(value); tick(); };
    const auto claim = [&]
    {
        const auto a = static_cast<uint16_t>(1u << static_cast<unsigned int>(GamepadButton::a));
        painter_scripted_pads::states[0].buttons = a; tick(); painter_scripted_pads::states[0].buttons = 0; tick();
        require(settings.player_index_for_slot(0) == 2, "A fresh device did not explicitly claim the waiting third Join seat"); expected = settings;
    };
    const auto frame = [&] { return main_menu_frame(app); };
    const auto unchanged = [&]
    {
        require(stack.depth() == 2 && settings.player_count() == expected.player_count() && settings.stage() == expected.stage() &&
            settings.duration() == expected.duration() && settings.best_of() == expected.best_of() && settings.screen_layout() == expected.screen_layout(),
            "Menu label drawing/refresh changed the page stack or match settings");
        const auto actual = settings.player_settings(), wanted = expected.player_settings();
        for (size_t seat = 0; seat < wanted.size(); ++seat)
        { require(actual[seat].num == wanted[seat].num && actual[seat].team == wanted[seat].team && actual[seat].weapon == wanted[seat].weapon,
            "Menu label refresh changed a claim, team or tool"); }
        require(result.total_area == result_before.total_area && result.team_a_area == result_before.team_a_area &&
            result.team_b_area == result_before.team_b_area && result.player_areas == result_before.player_areas && saved_bytes() == saved,
            "Menu label coverage changed result numbers or persisted preferences");
    };
    // Page kind: Join0, Team1, Weapon2, Pause3, Results4, Stick5.
    const auto open = [&](int kind)
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key value : { Key::enter, Key::space, Key::escape, Key::up, Key::down, Key::left, Key::right })
            { app.keyboard()->on_key_up(value); }
        painter_scripted_pads::states = {};
        for (auto& state : painter_scripted_pads::states) { state.connected = kind != 3; }
        app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll();
        app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        settings = MatchSetup(); settings.set_player_count(kind == 0 ? 3 : 4);
        settings.begin_input_claims(3); const auto& slots = kind == 3 ? pads : sparse;
        for (int seat = 1; seat < settings.player_count(); ++seat)
        { if (kind != 0 || seat == 1) { require(settings.claim_input_slot(slots[static_cast<size_t>(seat)]), "Menu label fixture claim failed"); } }
        settings.set_stage("movement_course"); settings.set_duration(73.0f); settings.set_best_of(3);
        settings.set_screen_layout(kind == 0 ? ScreenLayout::three_player : ScreenLayout::four_player);
        for (int seat = 0; seat < settings.player_count(); ++seat)
        { settings.set_player_team(seat, seat % 2 == 0 ? PlayerTeam::a : PlayerTeam::b); settings.set_player_weapon(seat, tools[static_cast<size_t>(seat)]); }
        expected = settings; stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        if (kind == 0) { stack.push(std::make_unique<MainMenuJoin>(&context)); }
        else if (kind == 1) { stack.push(std::make_unique<MainMenuTeamSelect>(&context)); }
        else if (kind == 2) { stack.push(std::make_unique<MainMenuWeaponSelect>(&context)); }
        else if (kind == 3) { stack.push(std::make_unique<PauseMenuInitial>(&context, 3, false)); }
        else if (kind == 4) { stack.push(std::make_unique<ResultsMenuInitial>(&context, result, single, true)); }
        else { stack.push(std::make_unique<StickSettingsMenu>(&context)); }
        tick(); tick(); unchanged();
    };
    std::ofstream records(output / "menu-labels.csv");
    records << "case,role,width,height,font,x,y,text_width,text_height,raster_matches,settings_preserved\n";
    // Roles: Join0, Team1, Weapon2, Pause3, Results4, Stick option5, connection6.
    const auto compare = [&](const char* name, int role, const Presentation& snapshot, const std::wstring& literal,
        int seat = 0, bool connected = true, bool capture = false)
    {
        unchanged(); const Frame actual = frame(); const auto& style = snapshot.menu(); const auto& main = snapshot.main_menu();
        MenuPaperArtPresentation art = main.art; art.focus_padding = main.setup_focus_padding; art.shadow_offset = main.setup_shadow_offset;
        Vector2F size = role == 1 ? main.team_box_size : role == 2 ? main.weapon_box_size : main.setup_box_size;
        RectangleF panel((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - size) / 2.0f, size);
        Vector2F position, shadow = Vector2F::ZERO; std::string font; Colour ink = art.ink; bool plain = false;
        if (role <= 2)
        {
            const auto spacing = role == 0 ? main.player_count_spacing : role == 1 ? main.team_select.spacing : main.weapon_select.spacing;
            position = style.layout.position + Vector2F(0.0f, spacing.y * static_cast<float>(seat + 2));
            font = role == 0 ? style.item_font : style.detail_font;
            shadow = role == 2 ? Vector2F(2.0f) : style.item_shadow_offset;
            if (role == 0) { ink = connected ? art.focus_ink : art.disabled_ink; }
        }
        else if (role == 3)
        {
            const auto& pause = snapshot.pause_menu(); art = pause.art; panel = RectangleF((menu_consts::DESIGN_RESOLUTION - pause.box_size) / 2.0f, pause.box_size);
            font = style.detail_font; position.y = pause.status_y;
            position.x = (menu_consts::DESIGN_RESOLUTION.x - app.render_resources()->measure_text(
                app.render_resources()->resolve_sprite_font(font), literal).x) / 2.0f; ink = art.ink; plain = true;
        }
        else if (role == 4)
        {
            const auto& look = snapshot.results_menu(); art = look.art; panel = RectangleF((menu_consts::DESIGN_RESOLUTION - look.box_size) / 2.0f, look.box_size);
            font = look.contribution_font; position = look.contribution_position + Vector2F(0.0f, look.contribution_spacing * static_cast<float>(seat + 1)); ink = art.ink;
        }
        else
        {
            const auto& look = snapshot.controls_settings(); art = look.art;
            panel = RectangleF((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - look.box_size) / 2.0f, look.box_size);
            font = role == 5 ? look.item_font : look.help_font;
            position = role == 5 ? look.layout.position + Vector2F(look.layout.spacing.x, 0.0f) : look.controls_position;
            ink = art.ink; plain = true;
        }
        std::unique_ptr<UiWidget> text;
        if (plain)
        {
            auto value = std::make_unique<UiText>("independent-menu-label", literal, font, position, app.render_resources(), ink);
            if (role == 5)
            {
                const auto& look = snapshot.controls_settings(); const RectangleF raw = value->bounds();
                const Vector2F limit(look.controls_position.x - look.art.focus_padding.x * 2.0f - look.art.shadow_offset - position.x,
                    look.layout.spacing.y - look.art.focus_padding.y * 2.0f);
                float scale = raw.width > limit.x ? limit.x / raw.width : 1.0f;
                if (raw.height * scale > limit.y) { scale = limit.y / raw.height; } value->set_scale(scale);
            }
            text = std::move(value);
        }
        else { text = std::make_unique<colourwars::UiTextDropShadow>("independent-menu-label", literal, font, position, app.render_resources(), ink, Colour{}, shadow); }
        const RectangleF text_bounds = text->bounds();
        require(panel.contains(text_bounds) && text_bounds.width > 0.0f && text_bounds.height > 0.0f,
            "Composed menu label escapes its actual paper");
        if (role == 3) { require(text_bounds.bottom() <= snapshot.pause_menu().layout.position.y - art.focus_padding.y,
            "Packed missing-player labels overlap the Pause actions"); }
        if (role == 4) { require(text_bounds.height < snapshot.results_menu().contribution_spacing && text_bounds.bottom() < snapshot.results_menu().proceed_y,
            "Results label/unchanged percentage overlaps another row or prompt"); }
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        const auto viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
        DrawList list = renderer.view(0); list.set_viewport(viewport); list.set_camera(camera); list.set_filter(TextureFilter::point);
        canvas.draw(list, camera.visible_rectangle(viewport)); FocusGroup focus;
        std::unique_ptr<UiTexture> focus_bounds;
        if (role == 5)
        {
            const auto& look = snapshot.controls_settings(); const UiText label("independent-pad-label", L"Pad", look.item_font,
                look.layout.position, app.render_resources());
            focus_bounds = std::make_unique<UiTexture>("independent-pad-row", style.sprite_sheet, style.fill_frame,
                RectangleF::union_of(label.bounds(), text_bounds), app.render_resources()); focus.add(focus_bounds.get());
        }
        const MenuPaperArtwork paper(panel, app.render_resources(), &style, &art, &focus); paper.draw(list);
        if (role == 1)
        {
            auto row_art = art; row_art.paper = art.focus_fill; const auto& identity = snapshot.player_identity();
            const RectangleF badge(text_bounds.x - identity.ui_size.x - identity.ui_gap,
                text_bounds.y + (text_bounds.height - identity.ui_size.y) / 2.0f, identity.ui_size.x, identity.ui_size.y);
            RectangleF row = RectangleF::union_of(RectangleF::union_of(text_bounds, badge), RectangleF(
                style.layout.position + Vector2F(main.team_select.spacing.x * 2.0f, main.team_select.spacing.y * static_cast<float>(seat + 2)),
                Vector2F(main.team_select.size.x * 3.0f, main.team_select.size.y)));
            row.inflate(art.focus_padding); const MenuPaperArtwork row_paper(row, app.render_resources(), &style, &row_art, &focus); row_paper.draw(list);
        }
        if (role == 0)
        {
            // Join underlines the host's row: the first seat after P1 that is
            // waiting or a bot, as nothing here moves it. Independent rectangle,
            // one rule below the label and one rule high.
            const auto seats = expected.player_settings(); int host_row = -1;
            for (size_t index = 1; index < seats.size() && host_row < 0; ++index)
            {
                if (seats[index].controller == PlayerController::bot || seats[index].num < 0) { host_row = static_cast<int>(index); }
            }
            if (seat == host_row)
            {
                const SpriteSheet* sheet = app.render_resources()->sprite_sheet(style.sprite_sheet);
                const RectangleF underline(text_bounds.x, text_bounds.bottom() + style.rule_height, text_bounds.width, style.rule_height);
                require(panel.contains(underline), "Independent Join host underline escapes its paper");
                sheet->draw(list, sheet->resolve_sprite_frame(style.fill_frame), underline, art.focus_ink);
            }
        }
        if (role != 3) { list.set_filter(TextureFilter::linear); }
        text->draw(list); const Frame wanted = read(renderer);
        RectangleF bounds = text_bounds; bounds.inflate(Vector2F(2.0f)); camera.calculate_view_rectangle(bounds);
        const RectangleI crop = countdown_crop_bounds(bounds);
        const Frame observed = foreground_crop(actual, crop, 0), reference = foreground_crop(wanted, crop, 0);
        const std::string stem = std::string("menu-labels-") + name + '-' + std::to_string(role) + '-' + std::to_string(seat) + '-' + std::to_string(actual.width);
        if (observed.pixels != reference.pixels)
        {
            if (captures <= 6) { save(observed, output / (stem + "-failure-actual.ppm")); save(reference, output / (stem + "-failure-expected.ppm")); captures += 2; }
            std::cerr << stem << " crop=" << crop.x << ',' << crop.y << ',' << crop.width << ',' << crop.height << '\n';
            throw std::runtime_error("Actual menu label differs from independent literal copy/font/shadow/paper raster");
        }
        records << name << ',' << role << ',' << actual.width << ',' << actual.height << ',' << font << ',' << text_bounds.x << ',' << text_bounds.y << ',' <<
            text_bounds.width << ',' << text_bounds.height << ",true,true\n"; ++rows;
        if (capture) { require(captures < 8, "Menu labels exceeded eight small crops"); save(observed, output / (stem + "-detail.ppm")); ++captures; }
        std::srand(7913); const int next = std::rand(); std::srand(7913);
        require(actual.pixels == frame().pixels && std::rand() == next, "Menu label drawing changed frozen pixels or RNG"); unchanged();
    };
    const std::array<std::wstring, 4> old_rows{ L"P1 - Pad 4", L"P2 - Keyboard/mouse", L"P3 - Pad 1", L"P4 - Pad 3" };
    const std::array<std::wstring, 4> new_rows{ L"Port 4 / P1", L"Keys / P2", L"Port 1 / P3", L"Port 3 / P4" };
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 }); std::ignore = app.renderer()->window_size_changed(1280, 720);
    const auto join = [&](const char* name, const Presentation& look, bool next, bool claimed, bool missing, bool capture = false)
    {
        compare(name, 0, look, (next ? new_rows[0] : old_rows[0]) + (missing ? L" - Reconnect" : L""), 0, !missing, capture);
        compare(name, 0, look, next ? new_rows[1] : old_rows[1], 1);
        compare(name, 0, look, claimed ? (next ? new_rows[2] : old_rows[2]) : (next ? L"Join / P3" : L"P3 - Waiting to join"), 2, claimed);
    };
    open(0); join("join-original", original, false, false, false);
    compare("waiting-original", 0, original, L"P3 - Waiting to join", 2, false, true);
    const Frame old_join = frame(); presentation = alternate; tick(); join("join-assigned-captured", original, false, false, false);
    require(frame().pixels == old_join.pixels, "Open Join adopted new labels/fonts on assignment");
    painter_scripted_pads::states[3].connected = false; tick(); join("join-old-reconnect", original, false, false, true);
    open(0); join("join-reopened-alternate", alternate, true, false, false); claim(); join("join-explicit-claim", alternate, true, true, false);
    painter_scripted_pads::states[3].connected = false; tick(); join("join-custom-reconnect", alternate, true, true, true, true);
    presentation = original; tick(); join("join-reverse-still-captured", alternate, true, true, true);
    open(0); join("join-reopened-original", original, false, false, false);
    app.resolution_manager()->set_resolution_exactly({ 1920, 1080 }); std::ignore = app.renderer()->window_size_changed(1920, 1080);
    join("join-1080", original, false, false, false);
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 }); std::ignore = app.renderer()->window_size_changed(1280, 720);
    require(frame().pixels == old_join.pixels, "Join resize/reopen changed its initial claim state");
    for (const int kind : { 1, 2, 4 })
    {
        const auto all_rows = [&](const char* name, const Presentation& look, bool next, bool capture = false)
        {
            const auto& words = next ? new_rows : old_rows;
            constexpr std::array<const wchar_t*, 4> percentages{ L"12.3%", L"21.6%", L"32.0%", L"34.0%" };
            for (int seat = 0; seat < 4; ++seat) { compare(name, kind, look, words[static_cast<size_t>(seat)] +
                (kind == 4 ? (std::wstring(seat % 2 == 0 ? L" | Team A | " : L" | Team B | ") + percentages[static_cast<size_t>(seat)]) : L""),
                seat, true, capture && seat == 0); }
        };
        presentation = original; open(kind); all_rows("static-original", original, false); const Frame old = frame();
        presentation = alternate; tick(); all_rows("static-assignment-retained", original, false);
        require(frame().pixels == old.pixels, "Open Team/Weapon/Results adopted new labels/fonts");
        open(kind); all_rows("static-reopened-alternate", alternate, true, true);
        presentation = original; tick(); all_rows("static-reverse-retained", alternate, true);
        open(kind); all_rows("static-reopened-original", original, false);
        require(frame().pixels == old.pixels, "Reopened Team/Weapon/Results did not restore exact labels and result numbers");
    }
    app.resolution_manager()->set_resolution_exactly({ 1920, 1080 }); std::ignore = app.renderer()->window_size_changed(1920, 1080);
    open(3); compare("pause-four-missing-original", 3, original, L"P1 - Pad 4   P2 - Pad 1\nP3 - Pad 3   P4 - Pad 2", 0, true, true);
    presentation = alternate; tick(); compare("pause-assigned-captured", 3, original, L"P1 - Pad 4   P2 - Pad 1\nP3 - Pad 3   P4 - Pad 2");
    painter_scripted_pads::states[3].connected = true; tick(); compare("pause-old-repack", 3, original, L"P2 - Pad 1   P3 - Pad 3\nP4 - Pad 2");
    open(3); compare("pause-reopened-alternate", 3, alternate, L"Port 4 / P1   Port 1 / P2\nPort 3 / P3   Port 2 / P4");
    painter_scripted_pads::states[3].connected = true; tick(); compare("pause-custom-repack", 3, alternate, L"Port 1 / P2   Port 3 / P3\nPort 2 / P4");
    presentation = original; tick(); compare("pause-reverse-retained", 3, alternate, L"Port 1 / P2   Port 3 / P3\nPort 2 / P4");
    open(3); compare("pause-reopened-original", 3, original, L"P1 - Pad 4   P2 - Pad 1\nP3 - Pad 3   P4 - Pad 2");
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 }); std::ignore = app.renderer()->window_size_changed(1280, 720);
    open(5); compare("stick-original-value", 5, original, L"Pad 1"); compare("stick-original-connection", 6, original, L"Pad 1 - connected");
    presentation = alternate; key(Key::right); compare("stick-old-cycle-value", 5, original, L"Pad 2");
    compare("stick-old-cycle-connection", 6, original, L"Pad 2 - connected");
    open(5); compare("stick-reopened-custom-value", 5, alternate, L"Port 1", 0, true, true);
    compare("stick-reopened-custom-connection", 6, alternate, L"Port 1 - connected", 0, true, true);
    key(Key::right); compare("stick-custom-cycle-value", 5, alternate, L"Port 2");
    painter_scripted_pads::states[1].connected = false; tick(); compare("stick-custom-disconnected", 6, alternate, L"Port 2 - disconnected");
    presentation = original; tick(); compare("stick-reverse-value-retained", 5, alternate, L"Port 2");
    compare("stick-reverse-connection-retained", 6, alternate, L"Port 2 - disconnected");
    open(5); compare("stick-reopened-original-value", 5, original, L"Pad 1"); compare("stick-reopened-original-connection", 6, original, L"Pad 1 - connected");
    require(captures == 8 && rows > 0 && records.good(), "Menu label evidence is incomplete or exceeded eight crops");
    std::cout << "PASS menu labels actual Join/Team/Weapon/Pause/Results/Stick pages: independent literal devices/rows, sparse reordered claims and fresh Join action, preserved two-per-line missing-pad packing, contribution numbers, captured fonts/copy through assignment/refresh/reopen/reversal, OptionRow fit, exact save and frozen pixels/RNG; " <<
        ticks << " bounded updates, " << rows << " CSV checks, eight compact crops; no broad matrix, full match or physical/foreground acceptance\n";
}
