// Fixed briefing copy is constructed once. The optional scripted reader is a
// test-only link seam, allowing a connected second seat without native input.
void stage_briefing_text_checks(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    struct Words
    {
        std::wstring label;
        std::wstring goal;
        std::wstring solo;
        std::wstring versus;
        std::wstring solo_mode;
        std::wstring single_match_mode;
    };
    const Words legacy{ L"MATCH SETUP", L"PAINT TO WIN",
        L"Make the arena your canvas.\nTry weapons. Cover more ground.",
        L"Cover the arena in your colour.\nMost painted surface wins.", L"SOLO PRACTICE", L"SINGLE MATCH" };
    struct BriefingCase
    {
        int players;
        int best_of;
        const char* name;
        const wchar_t* composed_mode;
        const wchar_t* roster;
        std::wstring Words::* objective;
        std::wstring Words::* fixed_mode;
    };
    const std::array<BriefingCase, 4> cases{ {
        { 1, 1, "solo", nullptr, L"1 PLAYER", &Words::solo, &Words::solo_mode },
        { 2, 1, "versus-single", nullptr, L"2 PLAYERS / 2 TEAMS", &Words::versus, &Words::single_match_mode },
        { 2, 3, "versus-best-three", L"BEST OF 3", L"2 PLAYERS / 2 TEAMS", &Words::versus, nullptr },
        { 2, 5, "versus-best-five", L"BEST OF 5", L"2 PLAYERS / 2 TEAMS", &Words::versus, nullptr } } };
    const Presentation original = presentation;
    Save preferences; auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0;
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false },
        { "close_quarters", "Close Quarters", "stage_close_quarters", true } };
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_level_infos(&levels);
    data.set_stages(&stages); data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings); StateContext& stack = app;
#if defined(COLOURWARS_PAINTER_SCRIPTED_PADS)
    struct RestoreBriefingPads
    {
        Application& app;
        std::array<GamepadState, Gamepads::max_count> previous;
        ~RestoreBriefingPads()
        {
            painter_scripted_pads::states = this->previous;
            this->app.gamepads()->poll(); this->app.gamepads()->poll();
        }
    } restore_pads{ app, painter_scripted_pads::states };
    constexpr size_t case_count = cases.size();
#else
    constexpr size_t case_count = 1;
    std::cout << "SKIP 2P briefing contexts: this normal-reader binary runs solo only; focused MenuTextOnly uses "
        "the scripted reader for connected versus seats\n";
#endif
    struct RestoreBriefing
    {
        StateContext& stack; MenuContext& context; Presentation& presentation; Presentation original;
        ~RestoreBriefing()
        {
            this->context.cancel_music_exit(); this->context.set_music_update({});
            this->stack.clear(); this->presentation = this->original;
        }
    } restore{ stack, context, presentation, original };
    const auto tick = [&](float elapsed = 0.0f)
        { app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed); };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick();
        // Static text fixtures observe the settled briefing ornament.
        tick(0.51f);
    };
    const auto frame = [&]
        { app.renderer()->begin_frame(); stack.draw(*app.renderer()); return read(*app.renderer()); };
    const auto same_plan = [](const MenuLevelSettings& a, const MenuLevelSettings& b)
    {
        if (a.player_count() != b.player_count() || a.stage() != b.stage() || a.duration() != b.duration() ||
            a.best_of() != b.best_of() || a.screen_layout() != b.screen_layout()) { return false; }
        const auto first = a.player_settings(), second = b.player_settings();
        for (size_t index = 0; index < first.size(); ++index)
        {
            if (first[index].num != second[index].num || first[index].team != second[index].team ||
                first[index].weapon != second[index].weapon) { return false; }
        }
        return true;
    };
    const auto open = [&](const BriefingCase& fixture)
    {
        stack.clear(); context.cancel_music_exit(); context.set_music_update({});
        stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::enter, Key::escape, Key::up, Key::down, Key::left, Key::right })
            { app.keyboard()->on_key_up(key); }
#if defined(COLOURWARS_PAINTER_SCRIPTED_PADS)
        painter_scripted_pads::states = {};
        if (fixture.players == 2) { painter_scripted_pads::states[0].connected = true; }
#endif
        app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll();
        app.mouse()->poll(); app.mouse()->poll();
        settings = MenuLevelSettings(); settings.set_player_count(fixture.players);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        if (fixture.players == 2)
        {
            require(app.gamepads()->connected(0) && settings.claim_input_slot(0),
                "Briefing versus fixture must claim a connected scripted second seat");
        }
        settings.set_stage("close_quarters"); settings.set_best_of(fixture.best_of); settings.set_duration(81.0f);
        settings.set_screen_layout(fixture.players == 1 ? ScreenLayout::one_player : ScreenLayout::two_player);
        for (int player = 0; player < fixture.players; ++player)
        {
            settings.set_player_team(player, player == 0 ? PlayerTeam::a : PlayerTeam::b);
            settings.set_player_weapon(player, player == 0 ? WeaponType::bucket : WeaponType::sniper);
        }
        input.reset_actions(); stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        stack.push(std::make_unique<MainMenuStageSelect>(&context)); tick(); tick();
        require(stack.depth() == 2 && settings.input_roster_complete() && !context.music_exit_pending(),
            "Briefing fixture did not remain on its connected neutral stage page");
    };
    std::ofstream records(output / "menu-text-stage-briefing.csv");
    records << "case,session,role,width,height,font,x,y,raw_width,raw_height,shadow_x,shadow_y,scale,fixture_matches\n";
    const auto compare = [&](const std::string& name, const BriefingCase& fixture,
        const Presentation& snapshot, const Words& words, const Presentation* fitted = nullptr)
    {
        const auto& style = snapshot.menu(); const auto& built = snapshot.main_menu();
        const auto& geometry = built.stage_select;
        const Vector2F info = geometry.briefing.position + geometry.briefing_padding;
        const Vector2F row = geometry.briefing.spacing;
        const RectangleF panel(geometry.briefing.position, geometry.briefing.size);
        struct ReferenceRow
        {
            const char* name;
            std::wstring text;
            std::string font;
            float row;
            float following;
            Colour ink;
        };
        const std::wstring mode = fixture.fixed_mode != nullptr ? words.*fixture.fixed_mode :
            std::wstring(fixture.composed_mode);
        const std::array<ReferenceRow, 5> rows{ {
            { "briefing-label", words.label, style.brand_font, 0.0f, 1.0f, built.art.focus_ink },
            { "mode", mode, style.detail_font, 1.0f, 2.0f, built.art.ink },
            { "roster", fixture.roster, style.brand_font, 2.0f, 3.0f, built.art.ink },
            { "goal", words.goal, style.brand_font, 3.0f, 4.0f, built.art.focus_ink },
            { "objective", words.*fixture.objective, style.brand_font, 4.0f, 6.0f, built.art.ink } } };
        std::array<std::unique_ptr<UiTextDropShadow>, 5> expected{};
        for (size_t index = 0; index < rows.size(); ++index)
        {
            const auto& reference = rows[index];
            expected[index] = std::make_unique<UiTextDropShadow>("reference-briefing", reference.text, reference.font,
                info + row * reference.row, app.render_resources(), reference.ink, Colour(0.0f, 0.0f, 0.0f, 0.0f),
                style.detail_shadow_offset);
            const RectangleF bounds = expected[index]->bounds();
            require(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION).contains(bounds) &&
                bounds.y >= panel.y && bounds.bottom() <= panel.bottom() &&
                bounds.bottom() <= (info + row * reference.following).y,
                "Independent unscaled briefing copy escapes the design or reaches its following row");
        }
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
        stage_paper_surface_reference(list, *app.render_resources(), snapshot,
            fitted == nullptr ? snapshot : *fitted, settings, false, false);
        list.set_filter(TextureFilter::linear);
        for (const auto& text : expected) { text->draw(list); }
        const Frame reference = read(renderer);
        for (size_t index = 0; index < rows.size(); ++index)
        {
            RectangleF bounds = expected[index]->bounds(); const RectangleF design_bounds = bounds;
            bounds.inflate(Vector2F(2.0f)); camera.calculate_view_rectangle(bounds);
            const RectangleI pixels = countdown_crop_bounds(bounds);
            const Frame observed = foreground_crop(actual, pixels, 0), wanted = foreground_crop(reference, pixels, 0);
            const std::string stem = "menu-text-stage-briefing-" + name + '-' + fixture.name + '-' + rows[index].name + '-' +
                std::to_string(actual.width) + 'x' + std::to_string(actual.height);
            if (observed.pixels != wanted.pixels)
            {
                save(observed, output / (stem + "-failure-actual.ppm"));
                save(wanted, output / (stem + "-failure-expected.ppm"));
                const RectangleI delta = foreground_delta_bounds(observed, wanted);
                std::cerr << "Briefing case=" << name << " session=" << fixture.name << " role=" << rows[index].name <<
                    " crop=(" << pixels.x << ',' << pixels.y << ',' << pixels.width << ',' << pixels.height <<
                    ") delta=(" << delta.x << ',' << delta.y << ',' << delta.width << ',' << delta.height << ")\n";
                throw std::runtime_error("Actual fixed briefing copy differs from its independent full-shadow fixture");
            }
            save(observed, output / (stem + "-detail.ppm"));
            records << name << ',' << fixture.name << ',' << rows[index].name << ',' << actual.width << ',' <<
                actual.height << ',' << rows[index].font << ',' << design_bounds.x << ',' << design_bounds.y << ',' <<
                design_bounds.width << ',' << design_bounds.height << ',' << style.detail_shadow_offset.x << ',' <<
                style.detail_shadow_offset.y << ",1,true\n";
        }
        std::srand(8131); const int next_random = std::rand(); std::srand(8131);
        require(actual.pixels == frame().pixels && std::rand() == next_random,
            "Frozen briefing drawing changed pixels or consumed RNG");
    };
    const auto cycle_and_lock = [&](const BriefingCase& fixture, const Presentation& snapshot,
        const Words& words, const std::string& name)
    {
        const MenuLevelSettings before = settings;
        std::srand(3169); const int next_random = std::rand(); std::srand(3169);
        tap(Key::right); tap(Key::enter); tap(Key::escape); tap(Key::left);
        require(std::rand() == next_random && same_plan(settings, before) && stack.depth() == 2 &&
            !context.music_exit_pending(), "Briefing refresh fixture consumed RNG or committed the provisional match");
        compare(name, fixture, snapshot, words, &presentation);
    };
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440),
        Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        for (size_t index = 0; index < case_count; ++index)
        {
            presentation = original; open(cases[index]); compare("default", cases[index], original, legacy);
        }
    }
    const Words old_words{ L"PLAN~", L"COVER~", L"Try~ tools.\nMark the floor.",
        L"Paint~ the arena.\nMost surface wins.", L"SOLO~", L"MATCH~" };
    const Words new_words{ L"ROUND PLAN", L"PAINT THE FLOOR", L"Try the tools.\nCover the arena.",
        L"Cover the arena.\nMore surface wins.", L"PRACTICE", L"ONE MATCH" };
    const auto with_words = [&](const Words& words)
    {
        Presentation result = original; auto& text = const_cast<MenuText&>(result.menu_text()).stage;
        text.briefing_label = words.label; text.goal = words.goal;
        text.solo_objective = words.solo; text.versus_objective = words.versus;
        text.solo_mode = words.solo_mode; text.single_match_mode = words.single_match_mode; return result;
    };
    Presentation old = with_words(old_words); old.validate_resources(*app.render_resources(), *app.audio_resources());
    const Font* source = app.render_resources()->font(
        app.render_resources()->resolve_sprite_font(original.menu().brand_font));
    std::vector<Glyph> glyphs;
    for (char32_t character = U' '; character < U'~'; ++character)
    {
        const Glyph* glyph = source->find(character);
        require(glyph != nullptr, "Briefing replacement brand font lacks a baseline printable glyph"); glyphs.push_back(*glyph);
    }
    const std::string font_name = "acceptance_stage_briefing_without_tilde";
    app.render_resources()->add_font(font_name,
        std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
    require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(font_name), L"~"),
        "Briefing replacement brand font still contains the old copy glyph");
    const Font* mode_source = app.render_resources()->font(
        app.render_resources()->resolve_sprite_font(original.menu().detail_font));
    std::vector<Glyph> mode_glyphs;
    for (char32_t character = U' '; character < U'~'; ++character)
    {
        const Glyph* glyph = mode_source->find(character);
        require(glyph != nullptr, "Briefing replacement detail font lacks a baseline printable glyph");
        mode_glyphs.push_back(*glyph);
    }
    const std::string mode_font = "acceptance_stage_briefing_mode_without_tilde";
    app.render_resources()->add_font(mode_font, std::make_unique<Font>(mode_source->atlas(),
        std::move(mode_glyphs), mode_source->line_spacing()));
    require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(mode_font), L"~"),
        "Briefing replacement detail font still contains the old mode glyph");
    Presentation next = with_words(new_words); auto& next_style = const_cast<MenuStyle&>(next.menu());
    next_style.brand_font = font_name; next_style.detail_font = mode_font;
    next_style.detail_shadow_offset = Vector2F(4.0f, 3.0f);
    next_style.shadow_colour = Colour(0.65f, 0.15f, 0.8f); next_style.highlight = Colour::cyan;
    next_style.unhighlight = Colour::yellow; next_style.heading_colour = Colour::magenta;
    const_cast<MainMenuPresentation&>(next.main_menu()).stage_select.briefing_padding += Vector2F(8.0f, 4.0f);
    next.validate_resources(*app.render_resources(), *app.audio_resources());
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    for (size_t index = 0; index < case_count; ++index)
    {
        const auto& fixture = cases[index]; presentation = old; open(fixture);
        compare("custom-old", fixture, old, old_words);
        presentation = next; tick(); compare("retained-old-before-refresh", fixture, old, old_words);
        cycle_and_lock(fixture, old, old_words, "retained-old-after-refresh");
        open(fixture); compare("reopened-new-font", fixture, next, new_words);
        presentation = original; tick(); compare("retained-new-before-refresh", fixture, next, new_words);
        cycle_and_lock(fixture, next, new_words, "retained-new-after-reversal");
        open(fixture); compare("restored-default", fixture, original, legacy);
        require(settings.player_count() == fixture.players && settings.best_of() == fixture.best_of &&
            settings.stage() == "close_quarters" && settings.player_settings()[0].num == input_slots::keyboard_and_mouse,
            "Fixed briefing text checks changed their match context");
    }
    require(records.good(), "Stage briefing evidence write failed");
    std::cout << "PASS actual fixed stage briefing: six independent legacy strings, unchanged best-of mode and roster, "
        "solo and connected scripted versus single/best-three/best-five when enabled, six display sizes, "
        "unscaled complete text/shadow bounds, captured text/brand and detail fonts/position/shadow/ink through source "
        "replacement/reopen/reversal and cycling/lock/unlock, unchanged plan and RNG; no physical pad claim\n";
}
