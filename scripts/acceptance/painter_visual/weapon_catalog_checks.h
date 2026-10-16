// Independent paper tile and fixed artwork layers. No MenuIcon draw helper is
// used, so the authored canvas, optional masks and final supply frame are pinned.
void weapon_paper_icon_reference(DrawList& list, RenderResources& resources,
    const MenuStyle& style, const MainMenuPresentation& look,
    const WeaponSelectArtworkPresentation& catalog, const WeaponChoicePresentation& choice,
    const RectangleF& box, const Colour& readiness, const Colour& paint, float response = 0.0f)
{
    RectangleF tile = box; tile.inflate(style.icon_padding + Vector2F(style.rule_height));
    const float inset = std::clamp(std::min(box.width, box.height) * response,
        0.0f, std::max(0.0f, std::min(style.icon_padding.x, style.icon_padding.y)));
    tile.inflate(-inset);
    auto art = look.art; art.shadow_offset = look.setup_shadow_offset;
    art.keyline_width = style.rule_height; art.paper = readiness;
    auto tile_style = style; tile_style.sprite_sheet = catalog.sheet; tile_style.fill_frame = catalog.fill_frame;
    setup_paper_reference(list, resources, tile_style, art, tile, RectangleF());
    const SpriteSheet* sheet = resources.sprite_sheet(catalog.sheet);
    // The bounded padding guard also covers the native raster's truncated
    // rectangle edge. Tool/paint/supply artwork still uses the original box.
    RectangleF canvas = box; canvas.inflate(std::min(style.rule_height, std::min(style.icon_padding.x, style.icon_padding.y)));
    sheet->draw(list, sheet->resolve_sprite_frame(catalog.fill_frame), canvas, style.heading_colour);
    const auto frame = sheet->resolve_sprite_frame(choice.frame);
    const RectangleI source = sheet->sprite_frame(frame).source_rectangle();
    const Vector2F source_size(static_cast<float>(source.width), static_cast<float>(source.height));
    const Vector2F fitted = source_size * std::min(box.width / source_size.x, box.height / source_size.y);
    const RectangleF artwork(box.center() - fitted / 2.0f, fitted);
    list.set_filter(TextureFilter::point); sheet->draw(list, frame, artwork, Colour::white);
    if (!choice.paint_frame.empty()) { sheet->draw(list, sheet->resolve_sprite_frame(choice.paint_frame), artwork, paint); }
    if (!choice.supply_strip.empty())
    {
        const auto& strip = sheet->animation_strip(sheet->resolve_animation_strip(choice.supply_strip));
        sheet->draw(list, strip.frame_rect(strip.frame_count() - 1), artwork, paint);
    }
}

// Real Weapon Select input and raster coverage. Expected names and bindings
// below are independent of the authored catalog so shipped-default drift fails.
void weapon_catalog_matrix(Application& app, Registry<LevelLoadedInfo>& levels,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    const std::array<WeaponChoicePresentation, 6> historical{
        WeaponChoicePresentation{ L"Sprayer", "sprayer", "sprayer_paint", "sprayer_supply" },
        WeaponChoicePresentation{ L"Sniper", "sniper", "sniper_paint", "" },
        WeaponChoicePresentation{ L"Roller", "roller", "roller_paint", "" },
        WeaponChoicePresentation{ L"Mister", "mister", "mister_paint", "mister_supply" },
        WeaponChoicePresentation{ L"Bucket", "bucket", "bucket_paint", "" },
        WeaponChoicePresentation{ L"Random", "random", "", "" }
    };
    Save preferences;
    auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0;
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false } };
    GameData data;
    data.set_application(&app); data.set_save(&preferences); data.set_level_infos(&levels);
    data.set_stages(&stages); data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings);
    StateContext& stack = app;
    auto& catalog = const_cast<MainMenuPresentation&>(presentation.main_menu()).weapon_select_art;
    const WeaponSelectArtworkPresentation original = catalog;
    struct RestoreWeaponCatalog
    {
        StateContext& stack;
        WeaponSelectArtworkPresentation& catalog;
        WeaponSelectArtworkPresentation original;
        ~RestoreWeaponCatalog() { this->stack.clear(); this->catalog = this->original; }
    } restore{ stack, catalog, original };
    const auto tick = [&]
    {
        app.keyboard()->poll(); app.mouse()->poll(); stack.update(dt);
    };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick();
        // Catalog fixtures compare the authored resting artwork and tile.
        app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.51f);
    };
    const auto frame = [&]
    {
        app.renderer()->begin_frame(); stack.draw(*app.renderer()); return read(*app.renderer());
    };
    const auto resize_page = [&](const Vector2I& size)
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
    };
    const auto same_plan = [](const MenuLevelSettings& a, const MenuLevelSettings& b, bool weapon = true)
    {
        return a.player_count() == b.player_count() && a.stage() == b.stage() &&
            a.duration() == b.duration() && a.best_of() == b.best_of() &&
            a.screen_layout() == b.screen_layout() && a.player_team(0) == b.player_team(0) &&
            a.player_settings()[0].num == b.player_settings()[0].num &&
            (!weapon || a.player_weapon(0) == b.player_weapon(0));
    };
    const auto open = [&](WeaponType type = WeaponType::sprayer)
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true);
        app.mouse()->set_focused(true);
        for (const Key key : { Key::enter, Key::escape, Key::left, Key::right })
            { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll();
        settings = MenuLevelSettings(); settings.set_player_count(1);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_player_team(0, PlayerTeam::a); settings.set_player_weapon(0, type);
        settings.set_stage("movement_course"); settings.set_duration(123.0f); settings.set_best_of(5);
        settings.set_screen_layout(ScreenLayout::one_player);
        input.reset_actions(); stack.push(std::make_unique<ResumeProbeRoot>());
        stack.push(std::make_unique<MainMenuWeaponSelect>(&context)); tick(); tick();
        require(stack.depth() == 2, "Actual Weapon Select did not remain open on neutral input");
    };
    const auto reference_page = [&](bool stage)
    {
        MenuLevelSettings copy = settings;
        MenuInput reference_input(app.gamepads(), app.keyboard(), &tuning);
        MenuContext reference_context(&data, &reference_input, &copy);
        StateContext reference_stack;
        reference_stack.notify_activation(true);
        if (stage) { reference_stack.push(std::make_unique<MainMenuStageSelect>(&reference_context)); }
        else { reference_stack.push(std::make_unique<MainMenuModeSelect>(&reference_context)); }
        app.renderer()->begin_frame(); reference_stack.draw(*app.renderer()); return read(*app.renderer());
    };
    std::ofstream records(output / "weapon-catalog.csv");
    records << "phase,width,height,choice,atlas,frame,paint_frame,supply_strip,fixture_matches\n";
    const auto compare = [&](const char* phase, size_t choice, const WeaponChoicePresentation& expected,
        const std::string& sheet = "weapon_paint", const std::string& fill = "pixel")
    {
        const auto& style = presentation.menu();
        const auto& look = presentation.main_menu();
        const Vector2F position = style.layout.position +
            Vector2F(look.weapon_select.spacing.x * 2.0f, look.weapon_select.spacing.y * 2.0f);
        const RectangleF icon_box(position, look.weapon_select.size);
        const Vector2F name_position = position + Vector2F(0.0f, look.weapon_select.size.y +
            style.icon_padding.y + style.rule_height);
        WeaponSelectArtworkPresentation expected_catalog;
        expected_catalog.sheet = sheet; expected_catalog.fill_frame = fill;
        const UiTextDropShadow name("expected-weapon-name", expected.name, look.weapon_description_font,
            name_position, app.render_resources(), look.art.ink,
            Colour{}, style.detail_shadow_offset);
        const UiTexture background("expected-weapon-background", style.sprite_sheet, style.fill_frame,
            RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), app.render_resources(),
            look.home_background_colour);
        RectangleF icon_bounds = icon_box;
        icon_bounds.inflate(style.icon_padding + Vector2F(style.rule_height + 3.0f));
        RectangleF name_bounds = name.bounds(); name_bounds.inflate(Vector2F(3.0f));
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        const auto pixels = [&](const RectangleF& box)
            { return countdown_crop_bounds(camera.calculate_view_rectangle(box)); };
        const Frame actual = frame();
        app.renderer()->begin_frame(); app.renderer()->set_view_count(1);
        DrawList list = app.renderer()->view(0);
        list.set_viewport(app.viewport_manager()->fullscreen_viewport()); list.set_camera(camera);
        list.set_filter(TextureFilter::point); background.draw(list);
        auto art = look.art; art.shadow_offset = look.setup_shadow_offset;
        const RectangleF paper((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - look.weapon_box_size) / 2.0f, look.weapon_box_size);
        setup_paper_reference(list, *app.render_resources(), style, art, paper, RectangleF());
        weapon_paper_icon_reference(list, *app.render_resources(), style, look, expected_catalog,
            expected, icon_box, look.weapon_select.unselected, Colour::white);
        list.set_filter(TextureFilter::linear); name.draw(list);
        const Frame expected_frame = read(*app.renderer());
        require(foreground_crop(actual, pixels(icon_bounds), 0).pixels ==
            foreground_crop(expected_frame, pixels(icon_bounds), 0).pixels,
            "Actual Weapon Select icon differs from the independent catalog fixture");
        require(foreground_crop(actual, pixels(name_bounds), 0).pixels ==
            foreground_crop(expected_frame, pixels(name_bounds), 0).pixels,
            "Actual Weapon Select name differs from the independent catalog fixture");
        std::srand(3179); const int next_random = std::rand(); std::srand(3179);
        require(actual.pixels == frame().pixels && std::rand() == next_random,
            "Frozen Weapon Select drawing changed pixels or consumed random numbers");
        const std::string stem = std::string("weapon-catalog-") + phase + '-' + std::to_string(choice) + '-' +
            std::to_string(actual.width) + 'x' + std::to_string(actual.height);
        save(foreground_crop(actual, pixels(RectangleF::union_of(icon_bounds, name_bounds)), 0),
            output / (stem + "-detail.ppm"));
        records << phase << ',' << actual.width << ',' << actual.height << ',' << choice << ',' <<
            sheet << ',' << expected.frame << ',' << expected.paint_frame << ',' << expected.supply_strip << ",true\n";
        return actual;
    };

    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
    {
        resize_page(size); open(); const MenuLevelSettings initial = settings;
        const Frame first = frame();
        for (size_t choice = 0; choice < historical.size(); ++choice)
        {
            compare("default", choice, historical[choice]); tap(Key::right);
            require(same_plan(settings, initial), "Browsing weapon choices committed settings before confirmation");
        }
        require(first.pixels == frame().pixels, "Weapon Select right wrap did not return to Sprayer");
        tap(Key::left); compare("left-wrap", 5, historical[5]); tap(Key::right);
        require(first.pixels == frame().pixels, "Weapon Select Random-to-Sprayer wrap changed the default page");
        save(first, output / ("weapon-catalog-default-" + std::to_string(size.x) + "x" +
            std::to_string(size.y) + "-context.ppm"));
    }

    resize_page({ 1280, 720 }); open(); const Frame baseline = frame();
    catalog.choices[0] = { L"Studio Hose", "mister", "mister_paint", "mister_supply" };
    catalog.choices[1] = { L"Studio Pail", "bucket", "bucket_paint", "" };
    presentation.validate_resources(*app.render_resources(), *app.audio_resources());
    tick(); require(frame().pixels == baseline.pixels, "Live catalog edit changed an existing page snapshot");
    tap(Key::right); compare("old-page-after-edit", 1, historical[1]); tap(Key::left);
    require(frame().pixels == baseline.pixels, "Cycling an existing page read new catalog metadata");
    const WeaponChoicePresentation custom_hose = catalog.choices[0], custom_pail = catalog.choices[1];
    open(); const Frame custom = compare("custom", 0, custom_hose);
    require(custom.pixels != baseline.pixels, "Reopened Weapon Select did not adopt edited names and artwork");
    tap(Key::right); compare("custom-no-supply", 1, custom_pail); tap(Key::left);
    require(frame().pixels == custom.pixels, "Switching edited artwork retained or lost a supply overlay");
    catalog = original; tick();
    require(frame().pixels == custom.pixels, "Reversing catalog tuning changed an already opened custom page");
    tap(Key::right); compare("custom-page-after-restore", 1, custom_pail);

    // This deliberately sparse atlas has no weapon/paint/strip names. Empty
    // optional layers must permit a completely different artwork vocabulary.
    catalog.sheet = "menu_paint"; catalog.fill_frame = "backdrop";
    for (auto& choice : catalog.choices) { choice = { L"Sparse Study", "backdrop", "", "" }; }
    presentation.validate_resources(*app.render_resources(), *app.audio_resources());
    open(); const Frame sparse = compare("sparse-atlas", 0,
        WeaponChoicePresentation{ L"Sparse Study", "backdrop", "", "" }, "menu_paint", "backdrop");
    require(sparse.pixels != baseline.pixels, "Reopened Weapon Select ignored the replacement atlas/fill");
    catalog = original; tick();
    require(frame().pixels == sparse.pixels, "Existing page replaced its cached sparse atlas on live restore");
    tap(Key::right); compare("sparse-page-after-restore", 1,
        WeaponChoicePresentation{ L"Sparse Study", "backdrop", "", "" }, "menu_paint", "backdrop");
    open(); require(frame().pixels == baseline.pixels, "Reopened default catalog did not restore the exact baseline raster");

    for (size_t choice = 0; choice < historical.size(); ++choice)
    {
        open(static_cast<WeaponType>(choice)); const MenuLevelSettings before = settings;
        const Frame selecting = frame();
        constexpr unsigned int seed = 8763;
        std::srand(seed); const WeaponType random = static_cast<WeaponType>(std::rand() % 5); std::srand(seed);
        app.keyboard()->on_key_down(Key::enter); tick();
        require(frame().pixels == selecting.pixels && same_plan(settings, before),
            "Weapon Select confirmed before a fresh Enter release");
        app.keyboard()->on_key_up(Key::enter); tick();
        const WeaponType expected = choice == 5 ? random : static_cast<WeaponType>(choice);
        require(settings.player_weapon(0) == expected && same_plan(settings, before, false) &&
            stack.depth() == 2 && frame().pixels == reference_page(true).pixels,
            "Confirm did not commit the selected primary and enter actual Stage Select");
        const MenuLevelSettings committed = settings;
        tap(Key::escape); tick(); compare("stage-back", static_cast<size_t>(expected),
            historical[static_cast<size_t>(expected)]);
        require(same_plan(settings, committed), "Stage Back changed the committed roster or match settings");
        tap(Key::escape);
        require(stack.depth() == 2 && frame().pixels == reference_page(false).pixels &&
            same_plan(settings, committed), "Unconfirmed Weapon Back did not return to actual Mode Select");
    }
    require(records.good(), "Weapon catalog evidence write failed");
    std::cout << "PASS actual Weapon Select: six default labels/artwork at 720p/1080p/QHD, both wraps, "
        "provisional browsing, fresh confirmation, seeded Random primary, Stage/Mode Back, "
        "custom layers and sparse atlas/fill, reopened adoption and retained page snapshots, frozen draw RNG\n";
}
