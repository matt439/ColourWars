// Independent badge cells preserve the existing roster identity on new paper.
void team_paper_badge(DrawList& list, RenderResources& resources, const PlayerIdentityPresentation& art,
    const RectangleF& bounds, int player)
{
    if (!art.enabled) { return; }
    const std::array<unsigned int, 5> p{ 7, 5, 7, 4, 4 };
    const std::array<std::array<unsigned int, 5>, 4> digits{{ { 2, 6, 2, 2, 7 }, { 7, 1, 7, 4, 7 }, { 7, 1, 7, 1, 7 }, { 5, 5, 7, 1, 1 } }};
    const SpriteSheet* sheet = resources.sprite_sheet("sprite_sheet_1"); const auto pixel = sheet->resolve_sprite_frame("pixel");
    const auto fill = [&](RectangleF box, const Colour& ink) { sheet->draw(list, pixel, box, Colour(ink.r * ink.a, ink.g * ink.a, ink.b * ink.a, ink.a)); };
    const float border = art.border, cell = std::min((bounds.width - border * 2.0f) / 9.0f, (bounds.height - border * 2.0f) / 7.0f);
    const Vector2F origin = bounds.center() - Vector2F(cell * 7.0f, cell * 5.0f) / 2.0f;
    list.set_filter(TextureFilter::point); fill(bounds, art.background_colour);
    fill(RectangleF(bounds.x, bounds.y, bounds.width, border), art.ink_colour);
    fill(RectangleF(bounds.x, bounds.bottom() - border, bounds.width, border), art.ink_colour);
    fill(RectangleF(bounds.x, bounds.y, border, bounds.height), art.ink_colour);
    fill(RectangleF(bounds.right() - border, bounds.y, border, bounds.height), art.ink_colour);
    for (size_t row = 0; row < p.size(); ++row)
    {
        for (int column = 0; column < 3; ++column)
        {
            const unsigned int mask = 1u << (2 - column); const Vector2F offset(static_cast<float>(column) * cell, static_cast<float>(row) * cell);
            if ((p[row] & mask) != 0) { fill(RectangleF(origin + offset, Vector2F(cell)), art.ink_colour); }
            if ((digits[static_cast<size_t>(player)][row] & mask) != 0) { fill(RectangleF(origin + offset + Vector2F(cell * 4.0f, 0.0f), Vector2F(cell)), art.ink_colour); }
        }
    }
}

void team_paper_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    const Presentation original = presentation;
    Save preferences; auto& play = const_cast<PlaySettings&>(preferences.play_settings()); play.music_volume = play.sfx_volume = 0; play.reduce_effects = false;
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false } };
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_level_infos(&levels); data.set_stages(&stages); data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MenuLevelSettings settings, initial;
    MenuContext context(&data, &input, &settings); StateContext& stack = app; MenuPage* active = nullptr;
    struct ClearTeamPaper { StateContext& stack; Presentation& presentation; Presentation original; ~ClearTeamPaper() { this->stack.clear(); painter_scripted_pads::states = {}; this->presentation = this->original; } } clear{ stack, presentation, original };
    const std::array<int, 4> slots{ input_slots::keyboard_and_mouse, 3, 0, 2 };
    const std::array<WeaponType, 4> weapons{ WeaponType::sprayer, WeaponType::sniper, WeaponType::roller, WeaponType::bucket };
    std::vector<PlayerTeam> teams; std::vector<bool> ready; bool warning = false, missing = false;
    const auto tick = [&](float elapsed = 0.0f) { app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed); };
    const auto bit = [](GamepadButton button) { return static_cast<uint16_t>(1u << static_cast<unsigned int>(button)); };
    const auto hold = [&](int player, GamepadButton button, bool down)
    {
        const int slot = slots[static_cast<size_t>(player)];
        if (slot == input_slots::keyboard_and_mouse)
        {
            const Key key = button == GamepadButton::a ? Key::enter : button == GamepadButton::b ? Key::escape : button == GamepadButton::x ? Key::r : button == GamepadButton::dpad_left ? Key::left : Key::right;
            if (down) { app.keyboard()->on_key_down(key); } else { app.keyboard()->on_key_up(key); }
        }
        else { auto& buttons = painter_scripted_pads::states[static_cast<size_t>(slot)].buttons; buttons = down ? static_cast<uint16_t>(buttons | bit(button)) : static_cast<uint16_t>(buttons & static_cast<uint16_t>(~bit(button))); }
    };
    const auto tap = [&](int player, GamepadButton button) { hold(player, button, true); tick(); hold(player, button, false); tick(); };
    const auto frame = [&] { return main_menu_frame(app); };
    const float settled = std::max(original.menu().motion.focus_duration, original.menu().motion.press_duration) + 0.01f;
    const auto open = [&](int count)
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (Key key : { Key::enter, Key::escape, Key::r, Key::left, Key::right }) { app.keyboard()->on_key_up(key); }
        painter_scripted_pads::states = {}; for (int player = 1; player < count; ++player) { painter_scripted_pads::states[static_cast<size_t>(slots[static_cast<size_t>(player)])].connected = true; }
        app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        settings = MenuLevelSettings(); settings.set_player_count(count); settings.set_best_of(3); settings.set_duration(73.0f); settings.set_stage("movement_course");
        settings.set_screen_layout(count == 1 ? ScreenLayout::one_player : count == 2 ? ScreenLayout::two_player : count == 3 ? ScreenLayout::three_player : ScreenLayout::four_player);
        settings.begin_input_claims(slots[0]); for (int player = 1; player < count; ++player) { require(settings.claim_input_slot(slots[static_cast<size_t>(player)]), "Team paper fixture claim failed"); }
        teams.clear(); ready.assign(static_cast<size_t>(count), false); warning = missing = false;
        for (int player = 0; player < count; ++player) { teams.push_back(player % 2 == 0 ? PlayerTeam::a : PlayerTeam::b); settings.set_player_team(player, teams.back()); settings.set_player_weapon(player, weapons[static_cast<size_t>(player)]); }
        initial = settings; stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        auto page = std::make_unique<MainMenuTeamSelect>(&context); active = page.get(); stack.push(std::move(page)); tick(); tick(); require(stack.depth() == 2, "Team paper did not retain neutral entry");
    };
    const auto plan = [&](bool committed = false)
    {
        require(settings.player_count() == initial.player_count() && settings.duration() == initial.duration() && settings.best_of() == initial.best_of() && settings.stage() == initial.stage() && settings.screen_layout() == initial.screen_layout(), "Team paper changed match settings");
        const auto rows = settings.player_settings(), before = initial.player_settings();
        for (size_t i = 0; i < rows.size(); ++i) { require(rows[i].num == before[i].num && rows[i].weapon == before[i].weapon && rows[i].team == (committed ? teams[i] : before[i].team), "Team paper committed provisional choices or changed slots/tools"); }
    };
    std::ofstream records(output / "team-paper.csv"); records << "case,width,height,players,warning,missing,moving_rows,full_frame_matches\n";
    const auto compare = [&](const std::string& name, const Presentation& snapshot, const std::wstring& heading,
        const std::vector<float>& insets = std::vector<float>{}, bool capture = false)
    {
        const Frame actual = frame(); const auto& style = snapshot.menu(); const auto& look = snapshot.main_menu();
        auto art = look.art; art.focus_padding = look.setup_focus_padding; art.shadow_offset = look.setup_shadow_offset; auto row_art = art; row_art.paper = art.focus_fill;
        const RectangleF paper((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - look.team_box_size) / 2.0f, look.team_box_size);
        const float edge = art.keyline_width * 3.0f; const RectangleF inside(paper.position() + Vector2F(edge), paper.size() - Vector2F(edge * 2.0f + art.shadow_offset));
        const auto position = [&](int column, int row) { return style.layout.position + Vector2F(look.team_select.spacing.x * static_cast<float>(column), look.team_select.spacing.y * static_cast<float>(row)); };
        const UiTextDropShadow title("independent-team-title", heading, style.heading_font, position(0, 0), app.render_resources(), art.ink, Colour{}, style.heading_shadow_offset);
        const UiTextDropShadow message("independent-team-warning", L"Both teams need a player", style.detail_font, look.team_warning_position, app.render_resources(), art.ink, Colour{}, style.detail_shadow_offset);
        std::vector<std::unique_ptr<UiTextDropShadow>> labels; std::vector<RectangleF> rows, badges;
        const std::array<std::wstring, 4> devices{ L"Keyboard/mouse", L"Pad 4", L"Pad 1", L"Pad 3" };
        for (size_t i = 0; i < teams.size(); ++i)
        {
            auto text = std::make_unique<UiTextDropShadow>("independent-team-label", L"P" + std::to_wstring(i + 1) + L" - " + devices[i], style.detail_font, position(0, static_cast<int>(i) + 2), app.render_resources(), art.ink, Colour{}, style.item_shadow_offset);
            const RectangleF bounds = text->bounds(); const auto& identity = snapshot.player_identity();
            const RectangleF badge(bounds.x - identity.ui_size.x - identity.ui_gap, bounds.y + (bounds.height - identity.ui_size.y) / 2.0f, identity.ui_size.x, identity.ui_size.y);
            RectangleF row = RectangleF::union_of(bounds, badge); row = RectangleF::union_of(row,
                RectangleF(position(2, static_cast<int>(i) + 2), Vector2F(look.team_select.size.x * 3.0f, look.team_select.size.y)));
            row.inflate(art.focus_padding); require(inside.contains(row), "Independent Team row/card escapes paper interior");
            rows.push_back(row); badges.push_back(badge); labels.push_back(std::move(text));
        }
        require(inside.contains(title.bounds()) && inside.contains(message.bounds()) && title.bounds().bottom() <= rows.front().y && rows.back().bottom() <= message.bounds().y,
            "Independent Team title/rows/warning overlap or escape paper");
        for (size_t i = 1; i < rows.size(); ++i) { require(rows[i - 1].bottom() <= rows[i].y, "Independent Team cards overlap"); }
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1); const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport); DrawList list = renderer.view(0); list.set_viewport(viewport); list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        canvas.draw(list, RectangleF(Vector2F::ZERO, app.resolution_manager()->resolution_vec())); list.set_camera(camera);
        const SpriteSheet* fill_sheet = app.render_resources()->sprite_sheet(style.sprite_sheet); const auto fill = fill_sheet->resolve_sprite_frame(style.fill_frame);
        fill_sheet->draw(list, fill, camera.visible_rectangle(viewport), style.scrim_colour);
        const UiTexture surround("independent-team-surround", style.sprite_sheet, style.fill_frame, RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), app.render_resources(), look.home_background_colour); surround.draw(list);
        setup_paper_reference(list, *app.render_resources(), style, art, paper, RectangleF());
        for (size_t i = 0; i < teams.size(); ++i)
        {
            setup_paper_reference(list, *app.render_resources(), style, row_art, rows[i], RectangleF());
            const int selected = teams[i] == PlayerTeam::a ? 0 : teams[i] == PlayerTeam::none ? 1 : 2;
            for (int choice = 0; choice < 3; ++choice)
            {
                RectangleF box(position(2, static_cast<int>(i) + 2) + Vector2F(look.team_select.size.x * static_cast<float>(choice), 0.0f), look.team_select.size);
                if (choice == selected && i < insets.size()) { box.inflate(-std::min(box.width, box.height) * insets[i]); }
                const auto& frames = choice == selected ? look.team_select_selected_frames : look.team_select_frames;
                const auto& live = presentation.main_menu().team_select; const Colour tint = choice == selected && ready[i] ? live.selected : live.unselected;
                const UiTexture icon("independent-team-icon", look.team_select_sheet, frames[static_cast<size_t>(choice)], box, app.render_resources(), tint); icon.draw(list);
            }
            team_paper_badge(list, *app.render_resources(), snapshot.player_identity(), badges[i], static_cast<int>(i));
        }
        fill_sheet->draw(list, fill, RectangleF(0.0f, style.footer_y, menu_consts::DESIGN_RESOLUTION.x, menu_consts::DESIGN_RESOLUTION.y - style.footer_y), style.scrim_colour);
        fill_sheet->draw(list, fill, RectangleF(style.brand_position.x, style.footer_y, menu_consts::DESIGN_RESOLUTION.x - style.brand_position.x * 2.0f, style.rule_height), style.panel_edge_colour);
        list.set_filter(TextureFilter::linear); title.draw(list); if (warning) { message.draw(list); } for (const auto& label : labels) { label->draw(list); }
        const auto& footer = snapshot.prompt_bar(); const std::wstring prompt = missing ? L" Reconnect controller    B/Esc Change players" : teams.size() < 2 ?
            L"Left/Right Pick a team    A/Enter Confirm    B/Esc Cancel" : L"Left/Right Pick a team    A/Enter Confirm    B/Esc Cancel    X/R Random teams";
        const UiTextDropShadow legend("independent-team-prompt", prompt, footer.font, footer.position, app.render_resources(), footer.colour, footer.shadow_colour, footer.shadow_offset); legend.draw(list);
        const Frame expected = read(renderer); const std::string stem = "team-paper-" + name + '-' + std::to_string(teams.size()) + "p-" + std::to_string(actual.width) + 'x' + std::to_string(actual.height);
        if (actual.pixels != expected.pixels) { save(actual, output / (stem + "-failure-actual.ppm")); save(expected, output / (stem + "-failure-expected.ppm")); const RectangleI delta = foreground_delta_bounds(actual, expected); std::cerr << "TEAM PAPER " << name << " delta=" << delta.x << ',' << delta.y << ',' << delta.width << ',' << delta.height << '\n'; }
        require(actual.pixels == expected.pixels, "Actual Team paper differs from independent paper/card/text/icon/badge fixture");
        const auto contrast = [&](RectangleF bounds)
        {
            camera.calculate_view_rectangle(bounds); const RectangleI box = countdown_crop_bounds(bounds); float dark = 1.0f, light = 0.0f; size_t ink = 0, pale = 0;
            for (int y = box.y; y < box.bottom(); ++y) { for (int x = box.x; x < box.right(); ++x) { const float value = hud_luminance(actual, hud_pixel_offset(actual, x, y)); dark = std::min(dark, value); light = std::max(light, value); if (value < 0.1f) { ++ink; } if (value > 0.5f) { ++pale; } } }
            require(ink > 2 && pale > 2 && (light + 0.05f) / (dark + 0.05f) >= 7.0f, "Team paper lost native dark lettering/pale-paper contrast");
        };
        contrast(title.bounds()); for (const auto& label : labels) { contrast(label->bounds()); } if (warning) { contrast(message.bounds()); }
        if (capture) { save(actual, output / (stem + ".ppm")); save(foreground_crop(actual, countdown_crop_bounds(camera.calculate_view_rectangle(paper)), 0), output / (stem + "-detail.ppm")); }
        std::srand(7919); const int next = std::rand(); std::srand(7919); require(actual.pixels == frame().pixels && actual.pixels == frame().pixels && std::rand() == next, "Frozen Team paper changed pixels or RNG"); plan();
        records << name << ',' << actual.width << ',' << actual.height << ',' << teams.size() << ',' << warning << ',' << missing << ',' << std::count_if(insets.begin(), insets.end(), [](float value) { return value > 0.0f; }) << ",true\n"; return actual;
    };
    const auto destination = [&](bool weapon)
    {
        MenuLevelSettings copy = settings; MenuInput other_input(app.gamepads(), app.keyboard(), &tuning); MenuContext other_context(&data, &other_input, &copy); StateContext other;
        other.push(std::make_unique<PauseMenuProbeRoot>(app, canvas)); if (weapon) { other.push(std::make_unique<MainMenuWeaponSelect>(&other_context)); } else { other.push(std::make_unique<MainMenuPlayerCount>(&other_context)); }
        app.renderer()->begin_frame(); other.draw(*app.renderer()); return read(*app.renderer());
    };
    const auto shuffle = [&](unsigned int seed)
    {
        // Pin the existing CRT draw budget and its ordered-seat contract without
        // calling the production shuffle helper to obtain the expected roster.
        std::array<int, 4> order{ 0, 1, 2, 3 }; const int count = static_cast<int>(teams.size()); std::srand(seed);
        for (int last = count - 1; last > 0; --last) { const int chosen = std::rand() % (last + 1); std::swap(order[static_cast<size_t>(last)], order[static_cast<size_t>(chosen)]); }
        const int a = count / 2 + (count % 2 ? std::rand() % 2 : 0); const int next = std::rand(); std::srand(seed); tap(count - 1, GamepadButton::x);
        require(std::rand() == next, "Team paper shuffle changed its seeded RNG draw budget");
        for (int i = 0; i < count; ++i) { teams[static_cast<size_t>(order[static_cast<size_t>(i)])] = i < a ? PlayerTeam::a : PlayerTeam::b; }
        ready.assign(teams.size(), false); warning = false;
    };
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440), Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        for (int count = 1; count <= 4; ++count)
        {
            const bool capture = count == 4 || size.x == 1280; open(count); compare("initial", original, L"Team Select", {}, capture);
            tap(0, GamepadButton::dpad_right); teams[0] = PlayerTeam::none; tick(settled); compare("unassigned", original, L"Team Select", {}, capture);
            tap(0, GamepadButton::a); compare("no-team-confirm-refused", original, L"Team Select");
            tap(0, GamepadButton::dpad_right); teams[0] = PlayerTeam::b; tick(settled); compare("team-b", original, L"Team Select");
            tap(0, GamepadButton::dpad_left); tap(0, GamepadButton::dpad_left); teams[0] = PlayerTeam::a; tick(settled);
            if (count == 1) { const Frame solo = compare("solo-restored", original, L"Team Select"); std::srand(437); const int next = std::rand(); std::srand(437); tap(0, GamepadButton::x); require(solo.pixels == frame().pixels && std::rand() == next, "Solo Team paper accepted shuffle or consumed RNG"); tap(0, GamepadButton::a); plan(true); require(frame().pixels == destination(true).pixels, "Solo Team paper failed its Weapon route"); continue; }
            tap(0, GamepadButton::a); ready[0] = true; tick(settled); compare("partly-ready", original, L"Team Select", {}, capture);
            tap(0, GamepadButton::b); ready[0] = false; tick(settled); compare("unready", original, L"Team Select");
            shuffle(17u + static_cast<unsigned int>(count)); tick(settled); compare("shuffled", original, L"Team Select", {}, capture);
            open(count);
            for (int player = 1; player < count; player += 2) { tap(player, GamepadButton::dpad_left); tap(player, GamepadButton::dpad_left); teams[static_cast<size_t>(player)] = PlayerTeam::a; }
            for (int player = 0; player < count - 1; ++player) { tap(player, GamepadButton::a); ready[static_cast<size_t>(player)] = true; }
            tick(settled); tap(count - 1, GamepadButton::a); warning = true; tick(settled); compare("both-teams-warning", original, L"Team Select", {}, capture);
            tap(count - 1, GamepadButton::dpad_right); tap(count - 1, GamepadButton::dpad_right); teams.back() = PlayerTeam::b; warning = false; tick(settled); compare("warning-cleared", original, L"Team Select");
            tap(count - 1, GamepadButton::a); plan(true); require(frame().pixels == destination(true).pixels, "Complete Team paper failed its committed Weapon route");
            open(count); const int slot = slots[static_cast<size_t>(count - 1)]; painter_scripted_pads::states[static_cast<size_t>(slot)].connected = false; tick(); missing = true; compare("missing-player", original, L"Team Select", {}, capture);
            painter_scripted_pads::states[static_cast<size_t>(slot)].connected = true; hold(count - 1, GamepadButton::a, true); tick(); tick(); hold(count - 1, GamepadButton::a, false); tick(); tick(); missing = false;
            compare("held-reconnect", original, L"Team Select");
            tap(0, GamepadButton::b); require(frame().pixels == destination(false).pixels, "Team paper Back failed its Count route");
        }
        std::cout << "PASS static Team paper " << size.x << 'x' << size.y << '\n';
    }
    const auto motion = original.menu().motion;
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
    {
        app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y); open(4);
        hold(0, GamepadButton::dpad_right, true); hold(1, GamepadButton::dpad_left, true); tick(); hold(0, GamepadButton::dpad_right, false); hold(1, GamepadButton::dpad_left, false); tick(); teams[0] = teams[1] = PlayerTeam::none;
        const Frame moving = compare("simultaneous-choice", original, L"Team Select", { motion.selection_inset_fraction, motion.selection_inset_fraction, 0, 0 }, true);
        tick(0.0f); tick(-0.5f); require(moving.pixels == frame().pixels, "Nonpositive time advanced Team paper motion");
        play.reduce_effects = true; compare("reduced-immediate", original, L"Team Select"); play.reduce_effects = false; require(moving.pixels == frame().pixels, "Reduced drawing consumed Team choice motion");
        tick(motion.focus_duration * 0.4f); compare("choice-middle", original, L"Team Select", { motion.selection_inset_fraction * 0.36f, motion.selection_inset_fraction * 0.36f, 0, 0 }, true); tick(settled); compare("choice-rest", original, L"Team Select", {}, true);
        tap(0, GamepadButton::dpad_left); teams[0] = PlayerTeam::a; tick(settled); tap(0, GamepadButton::a); ready[0] = true;
        compare("ready-press", original, L"Team Select", { motion.selection_inset_fraction * motion.press_fraction, 0, 0, 0 }, true); tick(settled);
        tap(0, GamepadButton::b); ready[0] = false; compare("unready-press", original, L"Team Select", { motion.selection_inset_fraction * motion.press_fraction, 0, 0, 0 }); stack.notify_activation(false); compare("deactivated", original, L"Team Select"); stack.notify_activation(true); tick();
        tap(0, GamepadButton::dpad_right); teams[0] = PlayerTeam::none; stack.push(std::make_unique<MainMenuCredits>(&context)); stack.pop(); compare("cover-return", original, L"Team Select"); tick();
        tap(0, GamepadButton::dpad_left); teams[0] = PlayerTeam::a; active->debug_reset_input(slots[0]); compare("developer-reset", original, L"Team Select");
        play.reduce_effects = true; tap(0, GamepadButton::dpad_right); teams[0] = PlayerTeam::none; const Frame quiet = compare("reduced-input", original, L"Team Select"); tick(settled); require(quiet.pixels == frame().pixels, "Reduced Team input restarted motion"); play.reduce_effects = false;
        const auto before_shuffle = teams; shuffle(67); std::vector<float> shuffled_insets;
        for (size_t player = 0; player < teams.size(); ++player) { shuffled_insets.push_back(motion.selection_inset_fraction * (teams[player] != before_shuffle[player] ? 1.0f : motion.press_fraction)); }
        compare("shuffle-press", original, L"Team Select", shuffled_insets, true); tick(settled); const Frame rest = compare("shuffle-rest", original, L"Team Select");
        app.resolution_manager()->set_resolution_exactly({ 1600, 1200 }); std::ignore = app.renderer()->window_size_changed(1600, 1200); std::ignore = frame(); app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y); require(rest.pixels == frame().pixels, "Team paper changed after resize round-trip");
    }
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 }); std::ignore = app.renderer()->window_size_changed(1280, 720);
    open(4); painter_scripted_pads::states[3].connected = false; tick(); missing = true; compare("missing-recovery", original, L"Team Select");
    tap(0, GamepadButton::b); require(frame().pixels == destination(false).pixels, "Missing Team player rescue failed its Count route");
    Presentation old = original, next = original; const_cast<MenuText&>(old.menu_text()).headings[static_cast<size_t>(MenuHeading::teams)] = L"Teams~"; const_cast<MenuText&>(next.menu_text()).headings[static_cast<size_t>(MenuHeading::teams)] = L"Pick Teams";
    auto& next_style = const_cast<MenuStyle&>(next.menu()); next_style.heading_font = "lato_black_48"; next_style.detail_font = "lato_black_24";
    auto& next_look = const_cast<MainMenuPresentation&>(next.main_menu()); next_look.art.paper = Colour(0.90f, 0.95f, 0.98f); next_look.art.focus_fill = Colour(0.80f, 0.85f, 0.90f); next_look.art.ink = Colour(0.04f, 0.09f, 0.17f);
    next_look.team_select.unselected = Colour(0.70f, 0.80f, 1.0f); next_look.team_select.selected = Colour(1.0f, 0.30f, 0.20f);
    next_look.team_select_sheet = "menu_paint"; next_look.team_select_frames.fill("backdrop"); next_look.team_select_selected_frames.fill("backdrop"); next_look.team_warning_position += Vector2F(4.0f, 0.0f);
    old.validate_resources(*app.render_resources(), *app.audio_resources()); next.validate_resources(*app.render_resources(), *app.audio_resources());
    presentation = old; open(3); compare("old-art-font-atlas", old, L"Teams~"); presentation = next;
    tap(0, GamepadButton::dpad_right); teams[0] = PlayerTeam::none; tick(settled); compare("old-choice-after-replacement", old, L"Teams~"); shuffle(101); tick(settled); compare("old-shuffle-after-replacement", old, L"Teams~");
    open(3); compare("reopened-art-font-atlas", next, L"Pick Teams", {}, true); presentation = original; tick(); compare("captured-art-live-tint-after-reversal", next, L"Pick Teams");
    tap(0, GamepadButton::dpad_right); teams[0] = PlayerTeam::none; tick(settled); compare("new-choice-after-reversal", next, L"Pick Teams"); shuffle(103); tick(settled); compare("new-shuffle-after-reversal", next, L"Pick Teams");
    open(4); compare("restored-default", original, L"Team Select");
    {
        auto& live = const_cast<MainMenuPresentation&>(presentation.main_menu());
        struct RestoreTeamWarning { Vector2F& position; Vector2F original; ~RestoreTeamWarning() { this->position = this->original; } } restore_warning{ live.team_warning_position, live.team_warning_position };
        live.team_warning_position.y = 930.0f;
        MenuLevelSettings invalid_settings = settings; MenuInput invalid_input(app.gamepads(), app.keyboard(), &tuning);
        MenuContext invalid_context(&data, &invalid_input, &invalid_settings); StateContext invalid_stack; bool rejected = false;
        try { invalid_stack.push(std::make_unique<MainMenuTeamSelect>(&invalid_context)); }
        catch (const std::runtime_error& error) { rejected = std::string(error.what()).find("Team Select warning must fit the paper") != std::string::npos; }
        require(rejected, "Actual Team paper accepted an overflowing warning or reported the wrong diagnostic");
    }
    require(records.good(), "Team paper evidence write failed"); require(IsWindowVisible(static_cast<HWND>(app.window())) == FALSE && GetForegroundWindow() != static_cast<HWND>(app.window()), "Team paper probe exposed or activated its hidden window");
    std::cout << "PASS Team paper independent full paper/cards/text/icons/badges at six sizes; solo and sparse2/3/4-roster choices/readiness/warning, seeded shuffle/RNG, recovery/routes, captured art/font/atlas, local motion/reduction/lifecycle/frozen redraw and resize; scripted reader only\n";
}
