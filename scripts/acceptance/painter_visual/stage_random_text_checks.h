// Movement Course remains explicitly selectable but is excluded from Random.
// Include a real eligible arena so this fixture actually reaches Random.
void stage_random_text_checks(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    const Presentation original = presentation;
    const std::wstring legacy = L"Random";
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
        { app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick(); };
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
            app.render_resources(), locked ? live.selected : live.unselected, style.shadow_colour, shadow);
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
        const UiTexture background("reference-stage-background", built.backdrop_sheet, built.backdrop_frame,
            RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), app.render_resources(),
            built.play_background_colour);
        background.draw(list);
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
            style.heading_font, style.layout.position, app.render_resources(), style.heading_colour,
            style.shadow_colour, style.heading_shadow_offset);
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
    Frame baseline{};
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440),
        Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        open(); const MenuLevelSettings before = settings;
        compare("default-real", original, L"Close Quarters", false, false, 2);
        cycle(Key::right); const Frame random = compare("default-random", original, legacy, true, false, 2);
        if (size.x == 1280) { baseline = random; }
        cycle(Key::left); compare("default-real-return", original, L"Close Quarters", false, false, 2);
        cycle(Key::right); require(compare("default-random-return", original, legacy, true, false, 2).pixels == random.pixels,
            "Cycling real/Random stages failed to restore the exact Random raster");
        tap_without_random(Key::enter); compare("default-locked", original, legacy, true, true, 2);
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
    require(records.good(), "Stage Random text evidence write failed");
    std::cout << "PASS actual Random stage name: independent default/shadow rasters across six display sizes, "
        "eligible arena plus excluded Movement Course, real/Random cycles without RNG or plan commits, "
        "long width-only fit and short reset, retained/reopened/reversed text/item font, live fit geometry, "
        "lock/unlock, interrupted and deliberate two-release start, eligibility and pending music plan\n";
}
