// Independent construction of the Stage papers and moving briefing stripe.
// Text and preview renderers remain separate so every old crop keeps its scope.
RectangleF stage_paper_surface_reference(DrawList& list, RenderResources& resources,
    const Presentation& captured, const Presentation& fitted, const MenuLevelSettings& settings,
    bool random, bool locked, size_t eligible_count = 1, float accent_offset = 0.0f)
{
    const auto& style = captured.menu(); const auto& main = captured.main_menu();
    const auto& geometry = main.stage_select; const auto& words = captured.menu_text().stage;
    auto art = main.art; art.focus_padding = main.setup_focus_padding; art.shadow_offset = main.setup_shadow_offset;
    const Colour clear_shadow(0.0f, 0.0f, 0.0f, 0.0f);
    const RectangleF authored(geometry.briefing.position, geometry.briefing.size);
    RectangleF card = authored;
    const Vector2F info = authored.position() + geometry.briefing_padding;
    const Vector2F row = geometry.briefing.spacing;
    const float inset = art.keyline_width * 3.0f + art.shadow_offset;
    const auto include = [&](const UiTextDropShadow& text)
    {
        RectangleF bounds = text.bounds(); bounds.inflate(inset);
        card = RectangleF::union_of(card, bounds);
    };
    const bool solo = settings.player_count() == 1;
    const std::wstring mode = solo ? words.solo_mode : settings.best_of() == 1 ? words.single_match_mode :
        L"BEST OF " + std::to_wstring(settings.best_of());
    const std::wstring roster = std::to_wstring(settings.player_count()) +
        (solo ? L" PLAYER" : L" PLAYERS / 2 TEAMS");
    struct Row { std::wstring text; std::string font; float line; };
    const std::array<Row, 6> fixed{ {
        { words.briefing_label, style.brand_font, 0.0f }, { mode, style.detail_font, 1.0f },
        { roster, style.brand_font, 2.0f }, { words.goal, style.brand_font, 3.0f },
        { solo ? words.solo_objective : words.versus_objective, style.brand_font, 4.0f },
        { locked ? words.locked_status : words.browse_status, style.detail_font, 6.0f } } };
    for (const auto& reference : fixed)
    {
        const UiTextDropShadow text("independent-stage-paper-bound", reference.text, reference.font,
            info + row * reference.line, &resources, art.ink, clear_shadow, style.detail_shadow_offset);
        include(text);
    }
    const std::wstring hint = locked ? words.locked_hint : random ?
        L"Random: " + std::to_wstring(eligible_count) + L" available arenas.\nConfirm to lock your choice." :
        words.browse_hint;
    const Vector2F shadow = fitted.menu().detail_shadow_offset;
    UiTextDropShadow hint_text("independent-stage-paper-hint", hint, style.brand_font,
        info + row * 7.0f, &resources, art.ink, clear_shadow, shadow);
    const RectangleF raw = RectangleF::union_of(hint_text.text_bounds_at(Vector2F::ZERO, 1.0f),
        hint_text.text_bounds_at(shadow, 1.0f));
    const auto& live = fitted.main_menu().stage_select;
    const float scale = std::min(1.0f, (live.briefing.size.x - live.briefing_padding.x * 2.0f) / raw.width);
    hint_text.set_scale(scale); hint_text.set_shadow_scale(scale); hint_text.set_shadow_offset(shadow * scale);
    include(hint_text);

    list.set_filter(TextureFilter::point);
    const UiTexture background("independent-stage-paper-background", style.sprite_sheet, style.fill_frame,
        RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), &resources, main.home_background_colour);
    background.draw(list);
    const RectangleF panel((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - main.stage_box_size) / 2.0f,
        main.stage_box_size);
    setup_paper_reference(list, resources, style, art, panel, RectangleF());
    auto briefing_art = art; briefing_art.paper = art.focus_fill;
    setup_paper_reference(list, resources, style, briefing_art, card, RectangleF());
    const UiTexture accent("independent-stage-paper-accent", style.sprite_sheet, style.fill_frame,
        RectangleF(authored.position() + Vector2F(0.0f, accent_offset), Vector2F(authored.width, style.accent_width)),
        &resources, art.ink);
    accent.draw(list);
    return card;
}

// Movement Course remains explicitly selectable but is excluded from Random.
// Include a real eligible arena so this fixture actually reaches Random.
void stage_random_text_checks(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    const Presentation original = presentation;
    const std::wstring legacy = L"Random";
    const std::wstring legacy_browse_status = L"FIND YOUR ARENA", legacy_locked_status = L"ARENA LOCKED";
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false },
        { "close_quarters", "Close Quarters", "stage_close_quarters", true } };
    require(random_stage_indices(stages) == std::vector<size_t>{ 1 },
        "Random stage text fixture must exclude Movement Course and include the real arena");
    Save preferences; auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0;
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_level_infos(&levels);
    data.set_stages(&stages); data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings); StateContext& stack = app;
    struct RestoreStageRandom
    {
        StateContext& stack; MenuContext& context; Presentation& presentation; Presentation original;
        ~RestoreStageRandom()
        {
            this->context.cancel_music_exit(); this->context.set_music_update({});
            this->stack.clear(); this->presentation = this->original;
        }
    } restore{ stack, context, presentation, original };
    const auto tick = [&](float elapsed = 0.0f)
        { app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed); };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick();
        // Static text fixtures observe the settled briefing ornament.
        if (!context.music_exit_pending()) { tick(0.51f); }
    };
    const auto tap_without_random = [&](Key key)
    {
        std::srand(9341); const int next_random = std::rand(); std::srand(9341); tap(key);
        require(std::rand() == next_random, "Provisional stage navigation/lock/unlock consumed RNG");
    };
    const auto frame = [&]
        { app.renderer()->begin_frame(); stack.draw(*app.renderer()); return read(*app.renderer()); };
    const auto same_plan = [](const MenuLevelSettings& a, const MenuLevelSettings& b, bool stage = true)
    {
        return a.player_count() == b.player_count() && (!stage || a.stage() == b.stage()) &&
            a.duration() == b.duration() && a.best_of() == b.best_of() && a.screen_layout() == b.screen_layout() &&
            a.player_team(0) == b.player_team(0) && a.player_weapon(0) == b.player_weapon(0) &&
            a.player_settings()[0].num == b.player_settings()[0].num;
    };
    const auto open = [&](bool course = false)
    {
        stack.clear(); context.cancel_music_exit(); context.set_music_update({});
        stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::enter, Key::escape, Key::up, Key::down, Key::left, Key::right })
            { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        settings = MenuLevelSettings(); settings.set_player_count(1);
        settings.begin_input_claims(input_slots::keyboard_and_mouse); settings.set_player_team(0, PlayerTeam::a);
        settings.set_player_weapon(0, WeaponType::bucket);
        settings.set_stage(course ? "movement_course" : "close_quarters");
        settings.set_duration(73.0f);
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        stack.push(std::make_unique<MainMenuStageSelect>(&context)); tick(); tick();
        require(stack.depth() == 2 && !context.music_exit_pending(), "Random text fixture consumed neutral stage entry");
    };
    const auto cycle = [&](Key key)
    {
        const MenuLevelSettings before = settings;
        tap_without_random(key);
        require(same_plan(settings, before) && stack.depth() == 2,
            "Stage browsing consumed RNG, committed the provisional slot or changed the match plan");
    };
    std::ofstream records(output / "menu-text-stage-random.csv");
    records << "case,slot,locked,width,height,font,raw_union_width,raw_union_height,scale,shadow_x,shadow_y,fixture_matches\n";
    const auto compare = [&](const std::string& name, const Presentation& snapshot,
        const std::wstring& text, bool random, bool locked, int fit_kind = 0)
    {
        const auto& style = snapshot.menu(); const auto& built = snapshot.main_menu();
        const auto& geometry = built.stage_select; const auto& live = presentation.main_menu().stage_select;
        const Vector2F shadow = presentation.menu().item_shadow_offset;
        UiTextDropShadow expected("reference-stage-name", text, style.item_font, geometry.name_position,
            app.render_resources(), locked ? live.selected_ink : live.unselected,
            Colour(0.0f, 0.0f, 0.0f, 0.0f), shadow);
        const RectangleF raw = RectangleF::union_of(expected.text_bounds_at(Vector2F::ZERO, 1.0f),
            expected.text_bounds_at(shadow, 1.0f));
        const float scale = std::min(1.0f, live.preview_size.x / raw.width);
        require(scale > 0.0f && (fit_kind != 1 || scale < 1.0f) && (fit_kind != 2 || scale == 1.0f),
            "Stage name fixture did not exercise its expected width-only fit or no-upscale reset");
        expected.set_scale(scale); expected.set_shadow_scale(scale); expected.set_shadow_offset(shadow * scale);
        const RectangleF expected_bounds = expected.bounds();
        require(expected_bounds.width <= live.preview_size.x + 0.01f,
            "Independent stage name and shadow escape the preview's width budget");
        const Frame actual = frame();
        const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(viewport); list.set_camera(Camera::DEFAULT_CAMERA);
        list.set_filter(TextureFilter::point);
        canvas.draw(list, RectangleF(Vector2F::ZERO, app.resolution_manager()->resolution_vec()));
        list.set_camera(camera);
        const SpriteSheet* sheet = app.render_resources()->sprite_sheet(style.sprite_sheet);
        const auto solid = sheet->resolve_sprite_frame(style.fill_frame);
        sheet->draw(list, solid, camera.visible_rectangle(viewport), style.scrim_colour);
        stage_paper_surface_reference(list, *app.render_resources(), snapshot, presentation, settings, random, locked);
        const RectangleF preview(geometry.preview_position, geometry.preview_size);
        RectangleF edge = preview; edge.inflate(style.rule_height);
        UiTexture preview_edge("reference-preview-edge", style.sprite_sheet, style.fill_frame, edge,
            app.render_resources(), locked ? live.selected : style.panel_edge_colour);
        UiTexture preview_background("reference-preview-background", style.sprite_sheet, style.fill_frame, preview,
            app.render_resources(), snapshot.hud().player_identity_background_colour);
        preview_edge.draw(list); preview_background.draw(list);
        StagePreview stage_preview("reference-stage-preview", preview, &data);
        if (!random) { stage_preview.set_stage("close_quarters"); }
        stage_preview.draw(list);
        // The preceding heading reaches into the top of the complete name
        // crop. Render it too, rather than shortening the name/shadow bounds.
        UiTextDropShadow heading("reference-stage-heading", snapshot.menu_text().heading(MenuHeading::arena),
            style.heading_font, style.layout.position, app.render_resources(), built.art.ink,
            Colour(0.0f, 0.0f, 0.0f, 0.0f), style.heading_shadow_offset);
        list.set_filter(TextureFilter::linear); heading.draw(list); expected.draw(list);
        const Frame reference = read(renderer);
        RectangleF bounds = expected_bounds; bounds.inflate(Vector2F(2.0f)); camera.calculate_view_rectangle(bounds);
        const RectangleI pixels = countdown_crop_bounds(bounds);
        const Frame observed = foreground_crop(actual, pixels, 0), wanted = foreground_crop(reference, pixels, 0);
        const std::string stem = "menu-text-stage-random-" + name + '-' +
            std::to_string(actual.width) + 'x' + std::to_string(actual.height);
        if (observed.pixels != wanted.pixels)
        {
            save(observed, output / (stem + "-failure-actual.ppm"));
            save(wanted, output / (stem + "-failure-expected.ppm"));
            const RectangleI delta = foreground_delta_bounds(observed, wanted);
            std::cerr << "Stage Random case=" << name << " scale=" << scale << " crop=(" << pixels.x << ',' <<
                pixels.y << ',' << pixels.width << ',' << pixels.height << ") delta=(" << delta.x << ',' << delta.y <<
                ',' << delta.width << ',' << delta.height << ")\n";
            throw std::runtime_error("Actual Random/real stage name differs from its independent full-shadow fixture");
        }
        save(observed, output / (stem + "-detail.ppm"));
        records << name << ',' << (random ? "random" : "close_quarters") << ',' << locked << ',' << actual.width << ',' <<
            actual.height << ',' << style.item_font << ',' << raw.width << ',' << raw.height << ',' << scale << ',' <<
            shadow.x << ',' << shadow.y << ",true\n";
        std::srand(6571); const int next_random = std::rand(); std::srand(6571);
        require(actual.pixels == frame().pixels && std::rand() == next_random,
            "Frozen Random stage drawing changed pixels or consumed RNG");
        return actual;
    };
    std::ofstream status_records(output / "menu-text-stage-status.csv");
    status_records << "case,slot,locked,width,height,font,x,y,raw_width,raw_height,shadow_x,shadow_y,scale,fixture_matches\n";
    const auto compare_status = [&](const std::string& name, const Presentation& snapshot,
        const std::wstring& text, bool random, bool locked)
    {
        const auto& style = snapshot.menu(); const auto& built = snapshot.main_menu();
        const auto& geometry = built.stage_select;
        const Vector2F info = geometry.briefing.position + geometry.briefing_padding;
        const Vector2F position = info + geometry.briefing.spacing * 6.0f;
        const Vector2F shadow = style.detail_shadow_offset;
        UiTextDropShadow expected("reference-stage-status", text, style.detail_font, position, app.render_resources(),
            locked ? presentation.main_menu().stage_select.selected_ink :
                presentation.main_menu().stage_select.unselected, Colour(0.0f, 0.0f, 0.0f, 0.0f), shadow);
        const RectangleF bounds = expected.bounds();
        const RectangleF panel(geometry.briefing.position, geometry.briefing.size);
        require(panel.contains(bounds) && RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION).contains(bounds) &&
            bounds.bottom() <= (info + geometry.briefing.spacing * 7.0f).y,
            "Independent unscaled stage status/shadow escapes the full briefing or reaches its hint row");
        const Frame actual = frame();
        const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(viewport); list.set_camera(Camera::DEFAULT_CAMERA);
        list.set_filter(TextureFilter::point);
        canvas.draw(list, RectangleF(Vector2F::ZERO, app.resolution_manager()->resolution_vec()));
        list.set_camera(camera);
        const SpriteSheet* sheet = app.render_resources()->sprite_sheet(style.sprite_sheet);
        const auto solid = sheet->resolve_sprite_frame(style.fill_frame);
        sheet->draw(list, solid, camera.visible_rectangle(viewport), style.scrim_colour);
        stage_paper_surface_reference(list, *app.render_resources(), snapshot, presentation, settings, random, locked);
        list.set_filter(TextureFilter::linear); expected.draw(list);
        const Frame reference = read(renderer);
        RectangleF crop = bounds; crop.inflate(Vector2F(2.0f)); camera.calculate_view_rectangle(crop);
        const RectangleI pixels = countdown_crop_bounds(crop);
        const Frame observed = foreground_crop(actual, pixels, 0), wanted = foreground_crop(reference, pixels, 0);
        const std::string stem = "menu-text-stage-status-" + name + '-' + (random ? "random-" : "real-") +
            std::to_string(actual.width) + 'x' + std::to_string(actual.height);
        if (observed.pixels != wanted.pixels)
        {
            save(observed, output / (stem + "-failure-actual.ppm"));
            save(wanted, output / (stem + "-failure-expected.ppm"));
            const RectangleI delta = foreground_delta_bounds(observed, wanted);
            std::cerr << "Stage status case=" << name << " random=" << random << " locked=" << locked <<
                " crop=(" << pixels.x << ',' << pixels.y << ',' << pixels.width << ',' << pixels.height <<
                ") delta=(" << delta.x << ',' << delta.y << ',' << delta.width << ',' << delta.height << ")\n";
            throw std::runtime_error("Actual arena status differs from its independent unscaled full-shadow fixture");
        }
        save(observed, output / (stem + "-detail.ppm"));
        status_records << name << ',' << (random ? "random" : "close_quarters") << ',' << locked << ',' <<
            actual.width << ',' << actual.height << ',' << style.detail_font << ',' << position.x << ',' << position.y <<
            ',' << bounds.width << ',' << bounds.height << ',' << shadow.x << ',' << shadow.y << ",1,true\n";
        std::srand(5731); const int next_random = std::rand(); std::srand(5731);
        require(actual.pixels == frame().pixels && std::rand() == next_random,
            "Frozen stage status drawing changed pixels or consumed RNG");
        return observed;
    };
    Frame baseline{}, status_baseline{};
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440),
        Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        open(); const MenuLevelSettings before = settings;
        compare("default-real", original, L"Close Quarters", false, false, 2);
        const Frame browse_status = compare_status("default-browse", original, legacy_browse_status, false, false);
        if (size.x == 1280) { status_baseline = browse_status; }
        tap_without_random(Key::enter);
        compare_status("default-locked", original, legacy_locked_status, false, true);
        tap_without_random(Key::escape);
        cycle(Key::right); const Frame random = compare("default-random", original, legacy, true, false, 2);
        compare_status("default-browse", original, legacy_browse_status, true, false);
        if (size.x == 1280) { baseline = random; }
        cycle(Key::left); compare("default-real-return", original, L"Close Quarters", false, false, 2);
        cycle(Key::right); require(compare("default-random-return", original, legacy, true, false, 2).pixels == random.pixels,
            "Cycling real/Random stages failed to restore the exact Random raster");
        tap_without_random(Key::enter); compare("default-locked", original, legacy, true, true, 2);
        compare_status("default-locked", original, legacy_locked_status, true, true);
        require(same_plan(settings, before) && !context.music_exit_pending() && stack.depth() == 2,
            "The first Random confirmation committed or started its provisional match");
        tap_without_random(Key::escape);
        require(compare("default-unlocked", original, legacy, true, false, 2).pixels == random.pixels &&
            same_plan(settings, before), "Random unlock changed the displayed label or match plan");
    }
    const std::wstring old_word = L"Random~ " + std::wstring(70, L'W'), new_word = L"Lotto arena";
    Presentation old = original; const_cast<MenuText&>(old.menu_text()).values.random = old_word;
    old.validate_resources(*app.render_resources(), *app.audio_resources());
    const Font* source = app.render_resources()->font(
        app.render_resources()->resolve_sprite_font(original.menu().item_font));
    std::vector<Glyph> glyphs;
    for (char32_t character = U' '; character < U'~'; ++character)
    {
        const Glyph* glyph = source->find(character);
        require(glyph != nullptr, "Stage Random replacement item font lacks a baseline printable glyph");
        glyphs.push_back(*glyph);
    }
    const std::string font_name = "acceptance_stage_random_without_tilde";
    app.render_resources()->add_font(font_name,
        std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
    require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(font_name), L"~"),
        "Stage Random replacement item font still has the old copy's glyph");
    Presentation next = original; const_cast<MenuText&>(next.menu_text()).values.random = new_word;
    const_cast<MenuStyle&>(next.menu()).item_font = font_name;
    next.validate_resources(*app.render_resources(), *app.audio_resources());
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    presentation = old; open(); cycle(Key::right);
    const Frame retained = compare("custom-long-old", old, old_word, true, false, 1);
    presentation = next; tick(); require(frame().pixels == retained.pixels,
        "Live Random text/item font replacement changed an existing stage page");
    cycle(Key::left); compare("old-real-scale-reset", old, L"Close Quarters", false, false, 2);
    cycle(Key::right); require(compare("old-random-after-replacement", old, old_word, true, false, 1).pixels == retained.pixels,
        "Stage cycling rebased captured old Random copy or item font");
    tap(Key::enter); compare("old-locked-after-replacement", old, old_word, true, true, 1);
    tap(Key::escape); compare("old-unlocked-after-replacement", old, old_word, true, false, 1);
    open(); cycle(Key::right); const Frame adopted = compare("reopened-new-font", next, new_word, true, false, 2);
    presentation = old; tick(); require(frame().pixels == adopted.pixels,
        "Random source reversal changed an already reopened stage page");
    cycle(Key::left); compare("new-real-after-reversal", next, L"Close Quarters", false, false, 2);
    cycle(Key::right); require(compare("new-random-after-reversal", next, new_word, true, false, 2).pixels == adopted.pixels,
        "Stage cycling after reversal changed captured new Random copy or item font");

    // Fitting still reads the live width and offset when the selection changes.
    // Constructor-owned text/font/position/shadow colour stay with the old page.
    presentation = old; open(); cycle(Key::right);
    const Frame before_live_fit = frame();
    Presentation live = next;
    const_cast<MainMenuPresentation&>(live.main_menu()).stage_select.preview_size.x = 700.0f;
    const_cast<MenuStyle&>(live.menu()).item_shadow_offset = Vector2F(-18.0f, -10.0f);
    presentation = live; tick(); require(frame().pixels == before_live_fit.pixels,
        "Live fit geometry changed a cached stage name before a slot/state refresh");
    tap(Key::enter); compare("old-live-width-shadow-locked", old, old_word, true, true, 1);
    tap(Key::escape); compare("old-live-width-shadow-unlocked", old, old_word, true, false, 1);

    presentation = original; open(true); cycle(Key::left);
    const MenuLevelSettings before = settings;
    require(compare("restored-default", original, legacy, true, false, 2).pixels == baseline.pixels,
        "Reopened stage page failed to restore the original Random raster");
    const Frame before_lock = frame(); app.keyboard()->on_key_down(Key::enter); tick();
    require(frame().pixels == before_lock.pixels && same_plan(settings, before),
        "Random locked or committed on key-down before a deliberate release");
    app.keyboard()->on_key_up(Key::enter); tick(); tick();
    tick(0.51f);
    const Frame locked = compare("deliberate-first-release-locked", original, legacy, true, true, 2);
    require(same_plan(settings, before) && stack.depth() == 2 && !context.music_exit_pending(),
        "Random did not retain the provisional plan after its first release");
    app.keyboard()->on_key_down(Key::enter); tick(); static_cast<WindowNotify&>(app).on_deactivated(); tick();
    static_cast<WindowNotify&>(app).on_activated(); tick(); app.keyboard()->on_key_up(Key::enter); tick(); tick();
    require(frame().pixels == locked.pixels && same_plan(settings, before) && stack.depth() == 2 &&
        !context.music_exit_pending(), "Interrupted Enter started a locked Random selection");
    compare("interrupted-locked-release", original, legacy, true, true, 2);
    int music_updates = 0; context.set_music_update([&](float) { ++music_updates; });
    app.keyboard()->on_key_down(Key::enter); tick();
    require(same_plan(settings, before) && !context.music_exit_pending(), "Random started before its fresh second release");
    std::srand(4937); std::ignore = std::rand(); const int after_pick = std::rand(); std::srand(4937);
    app.keyboard()->on_key_up(Key::enter); tick();
    require(std::rand() == after_pick && context.music_exit_pending() && stack.depth() == 2 && music_updates == 1 &&
        settings.stage() == "close_quarters" && same_plan(settings, before, false),
        "Random start bypassed eligibility, changed other settings, or failed to consume exactly one choice");
    const MenuLevelSettings committed = settings;
    tap(Key::right); tap(Key::enter); tap(Key::escape); tick(0.1f);
    require(frame().pixels == locked.pixels && same_plan(settings, committed) && context.music_exit_pending() &&
        stack.depth() == 2, "Pending Random music exit accepted input, rebased its label or changed the committed plan");
    compare("pending-random-start", original, legacy, true, true, 2);
    context.complete_music_exit(); context.complete_music_exit();
    require(stack.depth() == 1 && !context.music_exit_pending() && same_plan(settings, committed),
        "Completing Random start duplicated its pop or changed the selected plan");

    // Status widgets keep constructor-owned font, position, shadow offset and
    // shadow colour. Slot/state refreshes replace captured copy and live ink.
    using StageStatus = decltype(MenuText{}.stage);
    StageStatus old_status = original.menu_text().stage, new_status = original.menu_text().stage;
    old_status.browse_status = L"BROWSE~"; old_status.locked_status = L"LOCKED~";
    new_status.browse_status = L"FIND A STAGE"; new_status.locked_status = L"READY TO START";
    Presentation status_old = original;
    const_cast<MenuText&>(status_old.menu_text()).stage = old_status;
    status_old.validate_resources(*app.render_resources(), *app.audio_resources());
    const Font* status_source = app.render_resources()->font(
        app.render_resources()->resolve_sprite_font(original.menu().detail_font));
    std::vector<Glyph> status_glyphs;
    for (char32_t character = U' '; character < U'~'; ++character)
    {
        const Glyph* glyph = status_source->find(character);
        require(glyph != nullptr, "Stage status replacement detail font lacks a baseline printable glyph");
        status_glyphs.push_back(*glyph);
    }
    const std::string status_font = "acceptance_stage_status_without_tilde";
    app.render_resources()->add_font(status_font, std::make_unique<Font>(status_source->atlas(),
        std::move(status_glyphs), status_source->line_spacing()));
    require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(status_font), L"~"),
        "Stage status replacement detail font still contains the old status glyph");
    Presentation status_next = original;
    const_cast<MenuText&>(status_next.menu_text()).stage = new_status;
    auto& next_style = const_cast<MenuStyle&>(status_next.menu());
    next_style.detail_font = status_font; next_style.detail_shadow_offset = Vector2F(4.0f, 3.0f);
    next_style.shadow_colour = Colour(0.65f, 0.15f, 0.8f); next_style.heading_colour = Colour::cyan;
    auto& next_stage = const_cast<MainMenuPresentation&>(status_next.main_menu()).stage_select;
    next_stage.briefing_padding += Vector2F(8.0f, 4.0f); next_stage.selected = Colour::yellow;
    next_stage.unselected = Colour::cyan; next_stage.selected_ink = Colour::yellow;
    status_next.validate_resources(*app.render_resources(), *app.audio_resources());
    presentation = status_old; open();
    compare_status("custom-old-browse", status_old, old_status.browse_status, false, false);
    const MenuLevelSettings status_plan = settings; const Frame status_before = frame();
    presentation = status_next; tick(); require(frame().pixels == status_before.pixels,
        "Live stage status text/font/geometry/shadow replacement changed an existing widget before refresh");
    tap_without_random(Key::enter);
    compare_status("old-locked-after-replacement", status_old, old_status.locked_status, false, true);
    tap_without_random(Key::escape);
    compare_status("old-browse-after-replacement", status_old, old_status.browse_status, false, false);
    cycle(Key::right);
    compare_status("old-random-browse-after-replacement", status_old, old_status.browse_status, true, false);
    tap_without_random(Key::enter);
    compare_status("old-random-locked-after-replacement", status_old, old_status.locked_status, true, true);
    tap_without_random(Key::escape);
    compare_status("old-random-unlocked-after-replacement", status_old, old_status.browse_status, true, false);
    require(same_plan(settings, status_plan) && !context.music_exit_pending(),
        "Captured stage status callbacks committed a provisional selection or changed the match plan");
    open();
    compare_status("reopened-new-browse", status_next, new_status.browse_status, false, false);
    cycle(Key::right);
    compare_status("reopened-new-random-browse", status_next, new_status.browse_status, true, false);
    tap_without_random(Key::enter);
    compare_status("reopened-new-random-locked", status_next, new_status.locked_status, true, true);
    const Frame status_adopted = frame(); presentation = original; tick();
    require(frame().pixels == status_adopted.pixels,
        "Reversing source stage status text/font/geometry/shadow changed an already reopened widget");
    tap_without_random(Key::escape);
    compare_status("new-random-unlocked-after-reversal", status_next, new_status.browse_status, true, false);
    cycle(Key::left);
    compare_status("new-real-browse-after-reversal", status_next, new_status.browse_status, false, false);
    tap_without_random(Key::enter);
    compare_status("new-real-locked-after-reversal", status_next, new_status.locked_status, false, true);
    tap_without_random(Key::escape);
    require(same_plan(settings, status_plan) && !context.music_exit_pending(),
        "Reopened/reversed stage status callbacks changed the provisional match plan");
    open(); require(compare_status("restored-default", original, legacy_browse_status, false, false).pixels ==
        status_baseline.pixels, "Reopened stage status did not restore the exact original text/shadow raster");
    require(status_records.good(), "Stage status text evidence write failed");
    require(records.good(), "Stage Random text evidence write failed");
    std::cout << "PASS actual Random stage name: independent default/shadow rasters across six display sizes, "
        "eligible arena plus excluded Movement Course, real/Random cycles without RNG or plan commits, "
        "long width-only fit and short reset, retained/reopened/reversed text/item font, live fit geometry, "
        "lock/unlock, interrupted and deliberate two-release start, eligibility and pending music plan\n";
    std::cout << "PASS actual arena status: independent FIND YOUR ARENA/ARENA LOCKED full-shadow rasters on real/Random "
        "browse/locked pages across six display sizes, captured text/detail font/position/shadow through forced "
        "cycles/lock/unlock and source replacement/reopen/reversal, live ink and unchanged provisional plan/RNG\n";
}
