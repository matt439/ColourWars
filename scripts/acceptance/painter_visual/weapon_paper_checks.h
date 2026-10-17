// Actual sparse-seat Weapon Select with independent paper, artwork layers,
// unscaled lettering and badge cells. All input stays inside the hidden probe.
void weapon_paper_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    struct WeaponPaperProfile
    {
        std::wstring previous;
        explicit WeaponPaperProfile(const std::filesystem::path& root)
        {
            wchar_t* raw = nullptr; size_t length = 0;
            require(_wdupenv_s(&raw, &length, L"LOCALAPPDATA") == 0, "Cannot read the isolated Weapon paper profile");
            if (raw != nullptr) { this->previous = raw; std::free(raw); }
            std::filesystem::create_directories(root);
            require(_wputenv_s(L"LOCALAPPDATA", root.c_str()) == 0, "Cannot isolate Weapon paper palette preferences");
        }
        ~WeaponPaperProfile() { _wputenv_s(L"LOCALAPPDATA", this->previous.c_str()); }
    } isolated(output / "weapon-paper-profile");
    const Presentation original = presentation;
    const std::array<WeaponChoicePresentation, 6> shipped{{
        { L"Sprayer", "sprayer", "sprayer_paint", "sprayer_supply" },
        { L"Sniper", "sniper", "sniper_paint", "" }, { L"Roller", "roller", "roller_paint", "" },
        { L"Mister", "mister", "mister_paint", "mister_supply" }, { L"Bucket", "bucket", "bucket_paint", "" },
        { L"Random", "random", "", "" } }};
    const std::array<std::wstring, 6> descriptions{
        L"Fires a powerful stream of paint in an arc pattern.\nGood for beginners.",
        L"Slowly fires powerful, accurate shots.\nCan dispatch players quickly if you have skill.",
        L"Rolls paint in a wide area in front of you.\nCan be used to cover large floor areas quickly.",
        L"Produces a mist of slow-moving, diffusing paint particles.",
        L"Throws a bucket of paint in a short arc pattern.\nIdeal for defeating other players.",
        L"Randomly selects a weapon for you.\nIndecisiveness is nothing to be ashamed of." };
    for (size_t i = 0; i < shipped.size(); ++i)
    {
        const auto& actual = original.main_menu().weapon_select_art.choices[i]; const auto& expected = shipped[i];
        require(actual.name == expected.name && actual.frame == expected.frame && actual.paint_frame == expected.paint_frame && actual.supply_strip == expected.supply_strip &&
            original.main_menu().weapon_descriptions[i] == descriptions[i], "Shipped Weapon paper catalog or description drifted from its independent literals");
    }
    Save preferences; auto& play = const_cast<PlaySettings&>(preferences.play_settings()); play.music_volume = play.sfx_volume = 0; play.reduce_effects = false;
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false } };
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_level_infos(&levels); data.set_stages(&stages); data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MenuLevelSettings settings, initial;
    MenuContext context(&data, &input, &settings); StateContext& stack = app; MenuPage* active = nullptr;
    struct ClearWeaponPaper { StateContext& stack; Presentation& presentation; Presentation original; ~ClearWeaponPaper() { this->stack.clear(); painter_scripted_pads::states = {}; this->presentation = this->original; } } clear{ stack, presentation, original };
    const std::array<int, 4> slots{ input_slots::keyboard_and_mouse, 3, 0, 2 };
    std::vector<int> choices; std::vector<bool> ready; bool missing = false; TeamColourPair opened_palette = TeamColourPair::random;
    const auto tick = [&](float elapsed = 0.0f) { app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed); };
    const auto hold = [&](int player, GamepadButton button, bool down)
    {
        const int slot = slots[static_cast<size_t>(player)];
        if (slot == input_slots::keyboard_and_mouse)
        {
            const Key key = button == GamepadButton::a ? Key::enter : button == GamepadButton::b ? Key::escape : button == GamepadButton::dpad_left ? Key::left : Key::right;
            if (down) { app.keyboard()->on_key_down(key); } else { app.keyboard()->on_key_up(key); }
        }
        else { const auto bit = static_cast<uint16_t>(1u << static_cast<unsigned int>(button)); auto& buttons = painter_scripted_pads::states[static_cast<size_t>(slot)].buttons; buttons = down ? static_cast<uint16_t>(buttons | bit) : static_cast<uint16_t>(buttons & static_cast<uint16_t>(~bit)); }
    };
    const auto tap = [&](int player, GamepadButton button) { hold(player, button, true); tick(); hold(player, button, false); tick(); };
    const auto frame = [&] { return main_menu_frame(app); };
    const auto motion = original.menu().motion; const float settled = std::max(motion.focus_duration, motion.press_duration) + 0.01f;
    const auto open = [&](int count, int first = 0, TeamColourPair palette = TeamColourPair::random)
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (Key key : { Key::enter, Key::escape, Key::left, Key::right }) { app.keyboard()->on_key_up(key); }
        painter_scripted_pads::states = {}; for (int player = 1; player < count; ++player) { painter_scripted_pads::states[static_cast<size_t>(slots[static_cast<size_t>(player)])].connected = true; }
        app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        require(preferences.set_display_settings_and_save(preferences.resolution(), false, palette), "Isolated Weapon paper palette fixture could not save"); opened_palette = palette;
        settings = MenuLevelSettings(); settings.set_player_count(count); settings.set_best_of(3); settings.set_duration(73.0f); settings.set_stage("movement_course");
        settings.set_screen_layout(count == 1 ? ScreenLayout::one_player : count == 2 ? ScreenLayout::two_player : count == 3 ? ScreenLayout::three_player : ScreenLayout::four_player);
        settings.begin_input_claims(slots[0]); for (int player = 1; player < count; ++player) { require(settings.claim_input_slot(slots[static_cast<size_t>(player)]), "Weapon paper fixture claim failed"); }
        choices.clear(); ready.assign(static_cast<size_t>(count), false); missing = false;
        for (int player = 0; player < count; ++player) { choices.push_back((first + player) % 6); settings.set_player_team(player, player % 2 ? PlayerTeam::b : PlayerTeam::a); settings.set_player_weapon(player, static_cast<WeaponType>(choices.back())); }
        initial = settings; stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        std::srand(617); const int next_random = std::rand(); std::srand(617);
        auto page = std::make_unique<MainMenuWeaponSelect>(&context); active = page.get(); stack.push(std::move(page)); tick(); tick();
        require(stack.depth() == 2 && std::rand() == next_random, "Weapon paper neutral entry changed state or consumed palette RNG");
    };
    const auto plan = [&](bool committed = false)
    {
        require(settings.player_count() == initial.player_count() && settings.duration() == initial.duration() && settings.best_of() == initial.best_of() && settings.stage() == initial.stage() && settings.screen_layout() == initial.screen_layout(), "Weapon paper changed match settings");
        const auto rows = settings.player_settings(), before = initial.player_settings();
        for (size_t i = 0; i < rows.size(); ++i) { require(rows[i].num == before[i].num && rows[i].team == before[i].team && rows[i].weapon == (committed ? static_cast<WeaponType>(choices[i]) : before[i].weapon), "Weapon paper committed provisional choices or changed slots/teams"); }
    };
    std::ofstream records(output / "weapon-paper.csv"); records << "case,width,height,players,palette,missing,moving_rows,full_frame_matches\n";
    const auto compare = [&](const std::string& name, const Presentation& snapshot, const std::wstring& heading = L"Weapon Select",
        const std::vector<float>& insets = std::vector<float>{}, bool capture = false)
    {
        const Frame actual = frame(); const auto& style = snapshot.menu(); const auto& look = snapshot.main_menu(); auto art = look.art; art.shadow_offset = look.setup_shadow_offset;
        const RectangleF paper((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - look.weapon_box_size) / 2.0f, look.weapon_box_size);
        const float edge = art.keyline_width * 3.0f; const RectangleF inside(paper.position() + Vector2F(edge), paper.size() - Vector2F(edge * 2.0f + art.shadow_offset));
        const auto position = [&](int column, int row) { return style.layout.position + Vector2F(look.weapon_select.spacing.x * static_cast<float>(column), look.weapon_select.spacing.y * static_cast<float>(row)); };
        const UiTextDropShadow title("independent-weapon-title", heading, style.heading_font, position(0, 0), app.render_resources(), art.ink, Colour{}, style.heading_shadow_offset);
        std::vector<std::unique_ptr<UiTextDropShadow>> labels, names, details; std::vector<RectangleF> rows, badges, boxes;
        const std::array<std::wstring, 4> devices{ L"Keyboard/mouse", L"Pad 4", L"Pad 1", L"Pad 3" };
        for (size_t i = 0; i < choices.size(); ++i)
        {
            const auto& choice = look.weapon_select_art.choices[static_cast<size_t>(choices[i])];
            auto label = std::make_unique<UiTextDropShadow>("independent-weapon-label", L"P" + std::to_wstring(i + 1) + L" - " + devices[i], style.detail_font, position(0, static_cast<int>(i) + 2), app.render_resources(), art.ink, Colour{}, Vector2F(2.0f));
            const RectangleF box(position(2, static_cast<int>(i) + 2), look.weapon_select.size); boxes.push_back(box);
            auto tool_name = std::make_unique<UiTextDropShadow>("independent-weapon-name", choice.name, look.weapon_description_font,
                box.position() + Vector2F(0.0f, box.height + style.icon_padding.y + style.rule_height), app.render_resources(), ready[i] ? art.focus_ink : art.ink, Colour{}, style.detail_shadow_offset);
            auto detail = std::make_unique<UiTextDropShadow>("independent-weapon-description", look.weapon_descriptions[static_cast<size_t>(choices[i])], look.weapon_description_font,
                box.position() + Vector2F(look.weapon_description_x_offset, 0.0f), app.render_resources(), look.weapon_description_colour, Colour{}, look.weapon_description_shadow_offset);
            const RectangleF bounds = label->bounds(); const auto& identity = snapshot.player_identity();
            const RectangleF badge(bounds.x - identity.ui_size.x - identity.ui_gap, bounds.y + (bounds.height - identity.ui_size.y) / 2.0f, identity.ui_size.x, identity.ui_size.y);
            RectangleF tile = box; tile.inflate(style.icon_padding + Vector2F(style.rule_height));
            RectangleF row = RectangleF::union_of(RectangleF::union_of(bounds, badge), RectangleF::union_of(tile, RectangleF::union_of(tool_name->bounds(), detail->bounds())));
            require(inside.contains(row), "Independent Weapon row escapes paper interior"); rows.push_back(row); badges.push_back(badge);
            labels.push_back(std::move(label)); names.push_back(std::move(tool_name)); details.push_back(std::move(detail));
        }
        require(inside.contains(title.bounds()) && title.bounds().bottom() <= rows.front().y, "Independent Weapon heading escapes paper or overlaps rows");
        for (size_t i = 1; i < rows.size(); ++i) { require(rows[i - 1].bottom() <= rows[i].y, "Independent Weapon rows overlap"); }
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1); const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport); DrawList list = renderer.view(0); list.set_viewport(viewport); list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        canvas.draw(list, RectangleF(Vector2F::ZERO, app.resolution_manager()->resolution_vec())); list.set_camera(camera);
        const SpriteSheet* fill_sheet = app.render_resources()->sprite_sheet(style.sprite_sheet); const auto fill = fill_sheet->resolve_sprite_frame(style.fill_frame);
        fill_sheet->draw(list, fill, camera.visible_rectangle(viewport), style.scrim_colour);
        const UiTexture surround("independent-weapon-surround", style.sprite_sheet, style.fill_frame, RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), app.render_resources(), look.home_background_colour); surround.draw(list);
        setup_paper_reference(list, *app.render_resources(), style, art, paper, RectangleF());
        const TeamColour palette = opened_palette == TeamColourPair::random ? TeamColour(Colour::white, Colour::white) : snapshot.team_palette().fixed_pair(opened_palette).colours;
        for (size_t i = 0; i < choices.size(); ++i)
        {
            const auto& live = presentation.main_menu().weapon_select;
            weapon_paper_icon_reference(list, *app.render_resources(), style, look, look.weapon_select_art, look.weapon_select_art.choices[static_cast<size_t>(choices[i])], boxes[i], ready[i] ? live.selected : live.unselected,
                i % 2 ? palette.team_b : palette.team_a, i < insets.size() ? insets[i] : 0.0f);
            team_paper_badge(list, *app.render_resources(), snapshot.player_identity(), badges[i], static_cast<int>(i));
        }
        fill_sheet->draw(list, fill, RectangleF(0.0f, style.footer_y, menu_consts::DESIGN_RESOLUTION.x, menu_consts::DESIGN_RESOLUTION.y - style.footer_y), style.scrim_colour);
        fill_sheet->draw(list, fill, RectangleF(style.brand_position.x, style.footer_y, menu_consts::DESIGN_RESOLUTION.x - style.brand_position.x * 2.0f, style.rule_height), style.panel_edge_colour);
        list.set_filter(TextureFilter::linear); title.draw(list); for (size_t i = 0; i < choices.size(); ++i) { names[i]->draw(list); labels[i]->draw(list); details[i]->draw(list); }
        const auto& footer = snapshot.prompt_bar(); const std::wstring prompt = missing ? L" Reconnect controller    B/Esc Change players" : L"Left/Right Change weapon    A/Enter Confirm    B/Esc Cancel";
        const UiTextDropShadow legend("independent-weapon-prompt", prompt, footer.font, footer.position, app.render_resources(), footer.colour, footer.shadow_colour, footer.shadow_offset); legend.draw(list);
        const Frame expected = read(renderer); const std::string stem = "weapon-paper-" + name + '-' + std::to_string(choices.size()) + "p-" + TeamColourTools::palette_key(opened_palette) + '-' + std::to_string(actual.width) + 'x' + std::to_string(actual.height);
        if (actual.pixels != expected.pixels) { save(actual, output / (stem + "-failure-actual.ppm")); save(expected, output / (stem + "-failure-expected.ppm")); const RectangleI delta = foreground_delta_bounds(actual, expected); std::cerr << "WEAPON PAPER " << name << " delta=" << delta.x << ',' << delta.y << ',' << delta.width << ',' << delta.height << '\n'; }
        require(actual.pixels == expected.pixels, "Actual Weapon paper differs from independent paper/text/tool/mask/badge fixture");
        const auto contrast = [&](RectangleF bounds)
        {
            camera.calculate_view_rectangle(bounds); const RectangleI box = countdown_crop_bounds(bounds); float dark = 1.0f, light = 0.0f; size_t ink = 0, pale = 0;
            for (int y = box.y; y < box.bottom(); ++y) { for (int x = box.x; x < box.right(); ++x) { const float value = hud_luminance(actual, hud_pixel_offset(actual, x, y)); dark = std::min(dark, value); light = std::max(light, value); if (value < 0.1f) { ++ink; } if (value > 0.5f) { ++pale; } } }
            require(ink > 2 && pale > 2 && (light + 0.05f) / (dark + 0.05f) >= 7.0f, "Weapon paper lost native dark lettering/pale-paper contrast");
        };
        contrast(title.bounds()); for (size_t i = 0; i < choices.size(); ++i) { contrast(labels[i]->bounds()); contrast(names[i]->bounds()); contrast(details[i]->bounds()); }
        if (capture) { save(actual, output / (stem + ".ppm")); save(foreground_crop(actual, countdown_crop_bounds(camera.calculate_view_rectangle(paper)), 0), output / (stem + "-detail.ppm")); }
        std::srand(7919); const int next_random = std::rand(); std::srand(7919); require(actual.pixels == frame().pixels && actual.pixels == frame().pixels && std::rand() == next_random, "Frozen Weapon paper changed pixels or RNG"); plan();
        records << name << ',' << actual.width << ',' << actual.height << ',' << choices.size() << ',' << TeamColourTools::palette_key(opened_palette) << ',' << missing << ',' << std::count_if(insets.begin(), insets.end(), [](float value) { return value > 0.0f; }) << ",true\n"; return actual;
    };
    const auto destination = [&](int page)
    {
        MenuLevelSettings copy = settings; MenuInput other_input(app.gamepads(), app.keyboard(), &tuning); MenuContext other_context(&data, &other_input, &copy); StateContext other;
        other.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        if (page == 0) { other.push(std::make_unique<MainMenuModeSelect>(&other_context)); } else if (page == 1) { other.push(std::make_unique<MainMenuTeamSelect>(&other_context)); }
        else if (page == 2) { other.push(std::make_unique<MainMenuStageSelect>(&other_context)); } else { other.push(std::make_unique<MainMenuPlayerCount>(&other_context)); }
        app.renderer()->begin_frame(); other.draw(*app.renderer()); return read(*app.renderer());
    };
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440), Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        for (int count = 1; count <= 4; ++count)
        {
            open(count); const bool capture = count == 4 || size.x == 1280; const Frame first = compare("initial", original, L"Weapon Select", {}, capture);
            for (int step = 1; step <= 6; ++step)
            {
                for (int player = 0; player < count; ++player) { hold(player, GamepadButton::dpad_right, true); } tick();
                for (int player = 0; player < count; ++player) { hold(player, GamepadButton::dpad_right, false); choices[static_cast<size_t>(player)] = (choices[static_cast<size_t>(player)] + 1) % 6; } tick(); tick(settled);
                compare("cycle-" + std::to_string(step), original, L"Weapon Select", {}, capture && step == 5);
            }
            require(first.pixels == frame().pixels, "Weapon paper right wrap did not restore the original full frame");
            tap(0, GamepadButton::dpad_left); choices[0] = 5; tick(settled); compare("left-wrap", original); tap(0, GamepadButton::dpad_right); choices[0] = 0; tick(settled);
            if (count > 1)
            {
                tap(0, GamepadButton::a); ready[0] = true; tick(settled); const Frame confirmed = compare("partly-ready", original, L"Weapon Select", {}, capture);
                tap(0, GamepadButton::dpad_right); tap(0, GamepadButton::a); require(confirmed.pixels == frame().pixels, "Confirmed Weapon input changed the tool or restarted motion");
                tap(0, GamepadButton::b); ready[0] = false; tick(settled); compare("unready", original);
            }
            tap(0, GamepadButton::b); require(frame().pixels == destination(count == 1 ? 0 : 1).pixels, "Weapon paper Back failed its ordinary Mode/Team route"); plan();
            open(count, 5); for (int player = 1; player < count; ++player) { while (choices[static_cast<size_t>(player)] != 5) { tap(player, GamepadButton::dpad_right); choices[static_cast<size_t>(player)] = (choices[static_cast<size_t>(player)] + 1) % 6; } }
            tick(settled); compare("all-random", original, L"Weapon Select", {}, capture);
            for (int player = 0; player < count - 1; ++player) { tap(player, GamepadButton::a); ready[static_cast<size_t>(player)] = true; } tick(settled);
            std::srand(839); for (int& choice : choices) { choice = std::rand() % 5; } const int next_random = std::rand(); std::srand(839); tap(count - 1, GamepadButton::a);
            require(std::rand() == next_random, "Weapon paper Random commit changed its ordered per-seat RNG budget"); plan(true);
            require(frame().pixels == destination(2).pixels, "Weapon paper final confirmation failed its immediate Stage route");
            tap(0, GamepadButton::b); initial = settings; ready.assign(choices.size(), false); compare("stage-back", original);
            if (count > 1)
            {
                open(count); const int slot = slots[static_cast<size_t>(count - 1)]; painter_scripted_pads::states[static_cast<size_t>(slot)].connected = false; tick(); missing = true; compare("missing-player", original, L"Weapon Select", {}, capture);
                painter_scripted_pads::states[static_cast<size_t>(slot)].connected = true; hold(count - 1, GamepadButton::a, true); tick(); tick(); hold(count - 1, GamepadButton::a, false); tick(); tick(); missing = false; compare("held-reconnect", original);
                painter_scripted_pads::states[static_cast<size_t>(slot)].connected = false; tick(); tap(0, GamepadButton::b); require(frame().pixels == destination(3).pixels, "Missing Weapon player rescue failed its Count route");
            }
        }
        for (const TeamColourPair palette : { TeamColourPair::blue_orange, TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            open(4, 0, palette); const Frame first = compare("palette-supply", original, L"Weapon Select", {}, true);
            for (int step = 1; step <= 6; ++step)
            {
                for (int player = 0; player < 4; ++player) { hold(player, GamepadButton::dpad_right, true); } tick();
                for (int player = 0; player < 4; ++player) { hold(player, GamepadButton::dpad_right, false); choices[static_cast<size_t>(player)] = (choices[static_cast<size_t>(player)] + 1) % 6; } tick(); tick(settled);
                if (step < 6) { compare("palette-cycle-" + std::to_string(step), original); }
                else { require(first.pixels == frame().pixels, "Weapon paper palette cycle failed its exact wrap"); }
            }
            tap(3, GamepadButton::a); ready[3] = true; tick(settled); compare("palette-ready", original);
        }
        std::cout << "PASS static Weapon paper " << size.x << 'x' << size.y << '\n';
    }
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
    {
        app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y); open(4);
        hold(0, GamepadButton::dpad_right, true); hold(1, GamepadButton::dpad_right, true); tick(); hold(0, GamepadButton::dpad_right, false); hold(1, GamepadButton::dpad_right, false); tick(); ++choices[0]; ++choices[1];
        const Frame moving = compare("simultaneous-choice", original, L"Weapon Select", { motion.selection_inset_fraction, motion.selection_inset_fraction, 0, 0 }, true);
        tick(0.0f); tick(-0.5f); require(moving.pixels == frame().pixels, "Nonpositive time advanced Weapon paper motion");
        play.reduce_effects = true; compare("reduced-immediate", original); play.reduce_effects = false; require(moving.pixels == frame().pixels, "Reduced drawing consumed Weapon choice motion");
        tick(motion.focus_duration * 0.4f); compare("choice-middle", original, L"Weapon Select", { motion.selection_inset_fraction * 0.36f, motion.selection_inset_fraction * 0.36f, 0, 0 }, true); tick(settled); const Frame resting = compare("choice-rest", original, L"Weapon Select", {}, true);
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), app.viewport_manager()->fullscreen_viewport());
        for (int player = 0; player < 4; ++player)
        {
            RectangleF box(original.menu().layout.position + Vector2F(original.main_menu().weapon_select.spacing.x * 2.0f, original.main_menu().weapon_select.spacing.y * static_cast<float>(player + 2)), original.main_menu().weapon_select.size); box.inflate(-1.0f);
            camera.calculate_view_rectangle(box); const auto pixels = countdown_crop_bounds(box); require(foreground_crop(moving, pixels, 0).pixels == foreground_crop(resting, pixels, 0).pixels, "Weapon paper motion shifted its fixed tool, paint or supply pixels");
        }
        tap(0, GamepadButton::a); ready[0] = true; compare("ready-press", original, L"Weapon Select", { motion.selection_inset_fraction * motion.press_fraction, 0, 0, 0 }, true); tick(settled);
        tap(0, GamepadButton::b); ready[0] = false; compare("unready-press", original, L"Weapon Select", { motion.selection_inset_fraction * motion.press_fraction, 0, 0, 0 }); stack.notify_activation(false); compare("deactivated", original); stack.notify_activation(true); tick();
        tap(0, GamepadButton::dpad_right); ++choices[0]; stack.push(std::make_unique<MainMenuCredits>(&context)); stack.pop(); compare("cover-return", original); tick();
        tap(0, GamepadButton::dpad_left); --choices[0]; active->debug_reset_input(slots[0]); compare("developer-reset", original);
        play.reduce_effects = true; tap(0, GamepadButton::dpad_right); ++choices[0]; const Frame quiet = compare("reduced-input", original); tick(settled); require(quiet.pixels == frame().pixels, "Reduced Weapon input restarted motion"); play.reduce_effects = false;
        tap(0, GamepadButton::dpad_right); choices[0] = (choices[0] + 1) % 6; tap(0, GamepadButton::dpad_left); choices[0] = (choices[0] + 5) % 6;
        compare("rapid-choice", original, L"Weapon Select", { motion.selection_inset_fraction, 0, 0, 0 }); tick(settled); const Frame resize = compare("rapid-rest", original);
        app.resolution_manager()->set_resolution_exactly({ 1600, 1200 }); std::ignore = app.renderer()->window_size_changed(1600, 1200); std::ignore = frame(); app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y); require(resize.pixels == frame().pixels, "Weapon paper changed after resize round-trip");
    }
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 }); std::ignore = app.renderer()->window_size_changed(1280, 720);
    Presentation old = original, next = original; const_cast<MenuText&>(old.menu_text()).headings[static_cast<size_t>(MenuHeading::weapons)] = L"Tools~"; const_cast<MenuText&>(next.menu_text()).headings[static_cast<size_t>(MenuHeading::weapons)] = L"Pick Tools";
    auto& next_style = const_cast<MenuStyle&>(next.menu()); next_style.heading_font = "gill_sans_mt_bold_48"; next_style.detail_font = "gill_sans_mt_bold_24";
    auto& next_look = const_cast<MainMenuPresentation&>(next.main_menu()); next_look.weapon_description_font = "courier_new_bold_16"; next_look.art.paper = Colour(0.90f, 0.95f, 0.98f); next_look.art.ink = Colour(0.04f, 0.09f, 0.17f); next_look.art.focus_ink = Colour(0.02f, 0.05f, 0.10f);
    next_look.weapon_description_colour = next_look.art.ink; next_look.weapon_select.unselected = Colour(0.8f, 0.9f, 1.0f); next_look.weapon_select.selected = Colour(0.7f, 1.0f, 0.8f);
    next_look.weapon_select_art.sheet = "menu_paint"; next_look.weapon_select_art.fill_frame = "backdrop";
    for (auto& choice : next_look.weapon_select_art.choices) { choice = { L"Study", "backdrop", "", "" }; } next_look.weapon_descriptions.fill(L"Edited tool description");
    old.validate_resources(*app.render_resources(), *app.audio_resources()); next.validate_resources(*app.render_resources(), *app.audio_resources());
    presentation = old; open(3, 0, TeamColourPair::blue_orange); compare("old-art-font-catalog", old, L"Tools~"); presentation = next;
    require(preferences.set_display_settings_and_save(preferences.resolution(), false, TeamColourPair::pink_green), "Isolated Weapon palette replacement failed");
    tap(0, GamepadButton::dpad_right); ++choices[0]; tick(settled); compare("old-choice-after-replacement", old, L"Tools~"); tap(0, GamepadButton::a); ready[0] = true; tick(settled); compare("old-ready-after-replacement", old, L"Tools~");
    open(3, 0, TeamColourPair::pink_green); compare("reopened-art-font-catalog", next, L"Pick Tools", {}, true); presentation = original; tick(); compare("captured-art-live-fill-after-reversal", next, L"Pick Tools");
    tap(0, GamepadButton::dpad_right); ++choices[0]; tick(settled); compare("new-choice-after-reversal", next, L"Pick Tools"); tap(0, GamepadButton::a); ready[0] = true; tick(settled); compare("new-ready-after-reversal", next, L"Pick Tools");
    open(4); compare("restored-default", original);
    stack.clear();
    for (const bool description : { false, true })
    {
        Presentation malformed = original; auto& look = const_cast<MainMenuPresentation&>(malformed.main_menu());
        if (description) { look.weapon_descriptions[3] = std::wstring(100, L'W'); }
        else { look.weapon_select_art.choices[0].name = std::wstring(100, L'W'); }
        presentation = malformed; StateContext invalid_stack; bool rejected = false;
        try { invalid_stack.push(std::make_unique<MainMenuWeaponSelect>(&context)); }
        catch (const std::runtime_error& error) { rejected = std::string(error.what()).find(description ? "Weapon Select weapon description must fit the paper" : "Weapon Select weapon name must fit the paper") != std::string::npos; }
        require(rejected, "Actual Weapon paper accepted overflowing unscaled name/last-row description or reported the wrong diagnostic");
    }
    {
        // A description in words, too wide for the screen on one line from any tile
        // (it starts at least x_offset in), fits the paper in two lines once the page
        // wraps it to main_menu.weapon_description.max_width.
        Presentation wrapped = original; auto& look = const_cast<MainMenuPresentation&>(wrapped.main_menu());
        std::wstring words = L"paint"; for (int i = 1; i < 20; ++i) { words += L" paint"; }
        const FontHandle font = app.render_resources()->resolve_sprite_font(look.weapon_description_font);
        require(app.render_resources()->measure_text(font, words).x > 1920.0f - look.weapon_description_x_offset, "The wrapping fixture would fit the paper unwrapped");
        const std::wstring lines = text_wrap::wrap(*app.render_resources(), font, words, look.weapon_description_max_width);
        require(std::count(lines.begin(), lines.end(), L'\n') == 1, "The wrapping fixture no longer wraps to two lines");
        look.weapon_descriptions[3] = words; presentation = wrapped; StateContext wrap_stack;
        wrap_stack.push(std::make_unique<MainMenuWeaponSelect>(&context));
        wrap_stack.clear();
    }
    presentation = original;
    require(records.good(), "Weapon paper evidence write failed"); require(IsWindowVisible(static_cast<HWND>(app.window())) == FALSE && GetForegroundWindow() != static_cast<HWND>(app.window()), "Weapon paper probe exposed or activated its hidden window");
    std::cout << "PASS Weapon paper independent full paper/text/tool/masks/badges at six sizes; solo and sparse2/3/4 choices/readiness, palettes, ordered Random RNG/commit, recovery/routes, captured art/font/catalog, local motion/reduction/lifecycle/frozen redraw and resize; scripted reader only\n";
}
