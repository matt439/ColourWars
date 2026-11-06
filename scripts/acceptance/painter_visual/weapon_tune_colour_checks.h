#if COLOURWARS_DEV_TOOLS
// Two readiness card tints only. The existing independent icon helper pins
// premultiplied paper separately from the opaque body/paint/supply canvas.
void weapon_tune_colour_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& content,
    const std::filesystem::path& output)
{
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    StageList stages{ acceptance::shipped_stage(levels, "movement_course") };
    TuneColourSession session(app, tuning, presentation, content, output,
        { .scratch = "weapon-tune-scratch", .transcript = "weapon-tune-commands.log",
        .seed = [](Save& save, SettingsSession& settings)
        { PlaySettings play = settings.play_settings(); play.music_volume = play.sfx_volume = 0; play.reduce_effects = true;
          require(settings.set_play_settings_and_save(play) && save.set_display_settings_and_save(
              ScreenResolution::s_1280_720, false, TeamColourPair::blue_orange), "Cannot seed the private Weapon tint profile/palette"); },
        .catalog = [&] { return acceptance::catalog_of(levels, stages, { "movement_course" }); } });
    const std::filesystem::path& source = session.source();
    const std::filesystem::path& staged = session.staged();
    const auto& files = tune_colour_files;
    const auto& real_bytes = session.real_bytes();
    const Presentation& original = session.original_presentation();
    Save& preferences = session.preferences();
    const auto save_path = session.save_path();
    GameData& data = session.data();
    DebugDirectorImpl& impl = session.impl();
    const std::string saved = session.seeded_save();
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MatchSetup settings;
    settings.set_player_count(2); settings.begin_input_claims(3); require(settings.claim_input_slot(4), "Cannot claim Weapon tint keyboard seat");
    settings.set_stage("movement_course"); settings.set_duration(73.0f); settings.set_best_of(3); settings.set_screen_layout(ScreenLayout::two_player);
    settings.set_player_team(0, PlayerTeam::a); settings.set_player_team(1, PlayerTeam::b);
    settings.set_player_weapon(0, WeaponType::sprayer); settings.set_player_weapon(1, WeaponType::mister);
    const MatchSetup before = settings; MenuContext context(&data, &input, &settings); StateContext& stack = app;
    struct ClearWeaponTuneStack
    {
        StateContext& stack;
        ~ClearWeaponTuneStack() { this->stack.clear(); painter_scripted_pads::states = {}; }
    } clear{ stack };
    stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
    for (const Key key : { Key::enter, Key::escape, Key::left, Key::right }) { app.keyboard()->on_key_up(key); }
    painter_scripted_pads::states = {}; painter_scripted_pads::states[3].connected = true;
    app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll();
    input.reset_actions(); int ticks = 0; bool pad_connected = true;
    const auto tick = [&](float elapsed = 0.0f)
    {
        require(++ticks <= 16, "Weapon tint probe exceeded sixteen bounded page updates");
        app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed);
    };
    stack.push(std::make_unique<CanvasProbeRoot>(app, canvas)); stack.push(std::make_unique<MainMenuWeaponSelect>(&context)); tick(); tick();
    painter_scripted_pads::states[3].buttons = static_cast<uint16_t>(1u << static_cast<unsigned int>(GamepadButton::a)); tick();
    painter_scripted_pads::states[3].buttons = 0; tick(); tick(0.51f);
    const auto unchanged = [&]
    {
        require(stack.depth() == 2 && settings.player_count() == before.player_count() && settings.stage() == before.stage() &&
            settings.duration() == before.duration() && settings.best_of() == before.best_of() && settings.screen_layout() == before.screen_layout(),
            "Weapon tint commands changed the page stack or match settings");
        const auto actual = settings.player_settings(), wanted = before.player_settings();
        for (size_t seat = 0; seat < wanted.size(); ++seat)
        { require(actual[seat].num == wanted[seat].num && actual[seat].team == wanted[seat].team && actual[seat].weapon == wanted[seat].weapon,
            "Weapon tint refresh changed a sparse claim, team or selected tool"); }
        for (size_t slot = 0; slot < painter_scripted_pads::states.size(); ++slot)
        {
            const auto& pad = painter_scripted_pads::states[slot];
            require(pad.connected == (slot == 3 && pad_connected) && pad.buttons == 0 && pad.left_stick == Vector2F::zero &&
                pad.right_stick == Vector2F::zero && pad.left_trigger == 0.0f && pad.right_trigger == 0.0f,
                "Weapon tint commands changed neutral scripted input");
        }
        require(preferences.team_palette() == TeamColourPair::blue_orange && tune_colour_bytes(save_path) == saved,
            "Weapon tint commands changed the palette or private save");
    };
    const auto draw = [&] { unchanged(); return stack_frame(app); };
    const Frame baseline = tune_colour_frozen(draw); const auto& style = original.menu(); const auto& look = original.main_menu();
    const TeamColour paint = original.team_palette().fixed_pair(TeamColourPair::blue_orange).colours;
    const std::array<WeaponChoicePresentation, 2> choices{{ { L"Sprayer", "sprayer", "sprayer_paint", "sprayer_supply" },
        { L"Mister", "mister", "mister_paint", "mister_supply" } }};
    std::array<RectangleF, 2> boxes, envelopes; std::array<RectangleI, 2> card_pixels, canvas_pixels;
    const auto viewport = app.viewport_manager()->fullscreen_viewport();
    const Camera camera = Camera::frame(RectangleF(Vector2F::zero, menu_consts::design_resolution), viewport);
    for (size_t seat = 0; seat < boxes.size(); ++seat)
    {
        boxes[seat] = RectangleF(style.layout.position + Vector2F(look.weapon_select.spacing.x * 2.0f,
            look.weapon_select.spacing.y * static_cast<float>(seat + 2)), look.weapon_select.size);
        envelopes[seat] = boxes[seat]; envelopes[seat].inflate(style.icon_padding + Vector2F(style.rule_height));
        RectangleF projected = envelopes[seat]; camera.calculate_view_rectangle(projected); card_pixels[seat] = pixel_bounds(projected);
        projected = boxes[seat]; camera.calculate_view_rectangle(projected); canvas_pixels[seat] = pixel_bounds(projected);
    }
    const auto check_cards = [&](const Frame& actual, const Colour& selected, const Colour& unselected)
    {
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(viewport); list.set_camera(camera); list.set_filter(TextureFilter::point);
        canvas.draw(list, camera.visible_rectangle(viewport)); auto art = look.art; art.shadow_offset = look.setup_shadow_offset;
        const RectangleF panel((Vector2F(menu_consts::design_resolution.x, style.footer_y) - look.weapon_box_size) / 2.0f, look.weapon_box_size);
        setup_paper_reference(list, *app.render_resources(), style, art, panel, RectangleF());
        for (size_t seat = 0; seat < boxes.size(); ++seat)
        { weapon_paper_icon_reference(list, *app.render_resources(), style, look, look.weapon_select_art, choices[seat], boxes[seat],
            seat == 0 ? selected : unselected, seat == 0 ? paint.team_a : paint.team_b); }
        const Frame expected = read(renderer);
        for (size_t seat = 0; seat < boxes.size(); ++seat)
        {
            const Frame observed = foreground_crop(actual, card_pixels[seat], 0), wanted = foreground_crop(expected, card_pixels[seat], 0);
            require(observed.pixels == wanted.pixels, "Weapon readiness card/body/paint/supply differs from the independent premultiplied oracle");
            require(foreground_crop(actual, canvas_pixels[seat], 0).pixels == foreground_crop(baseline, canvas_pixels[seat], 0).pixels,
                "Weapon tint recoloured or replaced its captured opaque body/paint/supply canvas");
        }
    };
    check_cards(baseline, look.weapon_select.selected, look.weapon_select.unselected);
    std::ofstream records(output / "weapon-tune-colours.csv"); records << "check,adoption,selected,unselected,readiness_roster_art_preserved,pixels_match\n";
    const auto command = [&](const std::string& line)
    {
        impl.log().write(DebugSeverity::echo, "] " + line); const DebugStatus status = impl.execute(line);
        if (!status.ok) { std::cerr << line << ": " << status.message << '\n'; }
        require(status.ok, "Actual Weapon tint command was rejected");
    };
    const std::string selected_path = "presentation.main_menu.weapon_select.selected", unselected_path = "presentation.main_menu.weapon_select.unselected";
    // Explicit channel fractions are independent of the command's colour parser.
    // The leaf oracle premultiplies these straight-alpha values itself.
    const Colour selected(34.0f / 255.0f, 102.0f / 255.0f, 204.0f / 255.0f, 128.0f / 255.0f);
    const Colour unselected(217.0f / 255.0f, 69.0f / 255.0f, 24.0f / 255.0f, 192.0f / 255.0f);
    stack.notify_activation(false); command("tune " + selected_path + " 2266CC80"); command("tune " + unselected_path + " D94518C0");
    require(presentation.main_menu().weapon_select.selected == selected && presentation.main_menu().weapon_select.unselected == unselected &&
        impl.tune_document("presentation")->edits().size() == 2, "Weapon commands did not update exactly both audited RGBA leaves");
    require(tune_colour_frozen(draw).pixels == baseline.pixels, "Frozen Weapon drawing adopted new tints before visual refresh");
    records << "commands-before-update,retained,2266CC80,D94518C0,true,true\n";
    tick(); require(tune_colour_frozen(draw).pixels == baseline.pixels, "Inactive Weapon update adopted pending live tints");
    records << "inactive-update,retained,2266CC80,D94518C0,true,true\n";
    pad_connected = false; painter_scripted_pads::states[3].connected = false; stack.notify_activation(true); tick();
    const Frame missing = tune_colour_frozen(draw); check_cards(missing, look.weapon_select.selected, look.weapon_select.unselected);
    tick(); require(tune_colour_frozen(draw).pixels == missing.pixels, "Missing-player Weapon update changed retained tints or readiness");
    records << "missing-player-updates,retained,2266CC80,D94518C0,true,true\n";
    pad_connected = true; painter_scripted_pads::states[3].connected = true; tick();
    const Frame tuned = tune_colour_frozen(draw); check_cards(tuned, selected, unselected);
    require(tuned.pixels != baseline.pixels, "Neutral connected Weapon update did not adopt both card tints");
    for (int y = 0; y < baseline.height; ++y) { for (int x = 0; x < baseline.width; ++x)
    {
        const auto inside = [&](const RectangleI& box) { return x >= box.x && x < box.right() && y >= box.y && y < box.bottom(); };
        if (inside(card_pixels[0]) || inside(card_pixels[1])) { continue; }
        const size_t pixel = (static_cast<size_t>(y) * static_cast<size_t>(baseline.width) + static_cast<size_t>(x)) * 4;
        for (size_t channel = 0; channel < 4; ++channel) { require(baseline.pixels[pixel + channel] == tuned.pixels[pixel + channel],
            "Weapon tune changed names/text/fonts/catalog/paper or other pixels outside its two separate cards"); }
    } }
    records << "neutral-reconnect-refresh,live,2266CC80,D94518C0,true,true\n";
    command("tune.reset " + selected_path); command("tune.reset " + unselected_path);
    require(impl.tune_document("presentation")->edits().empty() && tune_colour_frozen(draw).pixels == tuned.pixels,
        "Weapon reset changed frozen widgets before refresh or retained edits");
    records << "reset-before-update,retained,baseline,baseline,true,true\n";
    tick(); require(tune_colour_frozen(draw).pixels == baseline.pixels, "Weapon tune reset failed exact same-page baseline reversal"); unchanged();
    records << "reset-refresh,restored,baseline,baseline,true,true\n";
    RectangleF evidence = RectangleF::union_of(envelopes[0], envelopes[1]); camera.calculate_view_rectangle(evidence);
    const RectangleI crop = pixel_bounds(evidence);
    save(foreground_crop(baseline, crop, 0), output / "weapon-tune-colours-original-detail.ppm");
    save(foreground_crop(tuned, crop, 0), output / "weapon-tune-colours-tuned-detail.ppm");
    for (size_t index = 0; index < files.size(); ++index)
    { require(tune_colour_bytes(content / files[index]) == real_bytes[index] && tune_colour_bytes(source / files[index]) == real_bytes[index] &&
        tune_colour_bytes(staged / files[index]) == real_bytes[index], "Weapon tune changed actual content or its scratch source/staged files"); }
    require(records.good(), "Weapon tune CSV write failed");
    std::cout << "PASS Weapon tune colours: four validated RGBA tune/reset commands, actual sparse 2P P1-ready/P2-unready, frozen/inactive/missing retention then neutral reconnect adoption, independent premultiplied cards, unchanged opaque body/paint/supply and all outside-card pixels, captured names/fonts/catalog/palette/roster/settings/save/input/RNG; " <<
        ticks << " bounded updates, six CSV checks and two compact crops; no broad matrix, full match or performance acceptance\n";
}
#endif
