// Navigation is unscaled constructor-owned detail text. Hints retain their
// constructor text/font/position/ink, but refresh their width fit and offset.
void stage_cue_text_checks(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    struct Cues { std::wstring random_navigation; std::wstring locked; std::wstring browse; };
    const Cues legacy{ L"<   SURPRISE ME   >", L"Confirm again to start.\nBack to change your choice.",
        L"Left/Right to browse.\nConfirm to lock your choice." };
    const std::wstring close_navigation = L"<   ARENA 2 / 2   >", course_navigation = L"<   ARENA 1 / 2   >";
    const std::wstring random_hint = L"Random: 1 available arenas.\nConfirm to lock your choice.";
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
    struct RestoreCuePads
    {
        Application& app; std::array<GamepadState, Gamepads::max_count> previous;
        ~RestoreCuePads()
        {
            painter_scripted_pads::states = this->previous;
            this->app.gamepads()->poll(); this->app.gamepads()->poll();
        }
    } restore_pads{ app, painter_scripted_pads::states };
#endif
    struct RestoreCues
    {
        StateContext& stack; MenuContext& context; Presentation& presentation; Presentation original;
        ~RestoreCues()
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
        return a.player_count() == b.player_count() && a.stage() == b.stage() && a.duration() == b.duration() &&
            a.best_of() == b.best_of() && a.screen_layout() == b.screen_layout() &&
            a.player_settings()[0].num == b.player_settings()[0].num &&
            a.player_team(0) == b.player_team(0) && a.player_weapon(0) == b.player_weapon(0);
    };
    const auto open = [&]
    {
        stack.clear(); context.cancel_music_exit(); context.set_music_update({});
        stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::enter, Key::escape, Key::up, Key::down, Key::left, Key::right })
            { app.keyboard()->on_key_up(key); }
#if defined(COLOURWARS_PAINTER_SCRIPTED_PADS)
        painter_scripted_pads::states = {};
#endif
        app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll();
        app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        settings = MenuLevelSettings(); settings.set_player_count(1);
        settings.begin_input_claims(input_slots::keyboard_and_mouse); settings.set_player_team(0, PlayerTeam::a);
        settings.set_player_weapon(0, WeaponType::bucket); settings.set_stage("close_quarters");
        settings.set_duration(91.0f);
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        stack.push(std::make_unique<MainMenuStageSelect>(&context)); tick(); tick();
        require(stack.depth() == 2 && settings.input_roster_complete() && !context.music_exit_pending(),
            "Stage cue fixture consumed neutral entry or opened a missing-seat page");
    };
    const auto provisional = [&](Key key)
    {
        const MenuLevelSettings before = settings;
        std::srand(3581); const int next_random = std::rand(); std::srand(3581); tap(key);
        require(std::rand() == next_random && same_plan(settings, before) && stack.depth() == 2 &&
            !context.music_exit_pending(), "Stage cue browsing/lock/unlock changed the plan or consumed RNG");
    };
    std::ofstream records(output / "menu-text-stage-cues.csv");
    records << "case,role,width,height,font,raw_width,raw_height,scale,shadow_x,shadow_y,fixture_matches\n";
    const auto compare = [&](const std::string& name, const Presentation& snapshot,
        const Presentation& fitted, const std::wstring& navigation, const std::wstring& hint, int fit_kind = 0)
    {
        const auto& style = snapshot.menu(); const auto& built = snapshot.main_menu();
        const auto& geometry = built.stage_select; const auto& live = fitted.main_menu().stage_select;
        const Vector2F hint_position = geometry.briefing.position + geometry.briefing_padding +
            geometry.briefing.spacing * 7.0f;
        colourwars::UiTextDropShadow expected_navigation("reference-stage-navigation", navigation, style.detail_font,
            geometry.navigation_position, app.render_resources(), built.art.ink, Colour(0.0f, 0.0f, 0.0f, 0.0f),
            style.detail_shadow_offset);
        colourwars::UiTextDropShadow expected_hint("reference-stage-hint", hint, style.brand_font, hint_position,
            app.render_resources(), built.art.ink, Colour(0.0f, 0.0f, 0.0f, 0.0f), fitted.menu().detail_shadow_offset);
        const Vector2F shadow = fitted.menu().detail_shadow_offset;
        const RectangleF raw = RectangleF::union_of(expected_hint.text_bounds_at(Vector2F::ZERO, 1.0f),
            expected_hint.text_bounds_at(shadow, 1.0f));
        const float budget = live.briefing.size.x - live.briefing_padding.x * 2.0f;
        const float scale = std::min(1.0f, budget / raw.width);
        require(budget > 0.0f && scale > 0.0f && (fit_kind != 1 || scale < 1.0f) &&
            (fit_kind != 2 || scale == 1.0f), "Stage cue fixture failed to exercise width-only hint fit/reset");
        expected_hint.set_scale(scale); expected_hint.set_shadow_scale(scale); expected_hint.set_shadow_offset(shadow * scale);
        const RectangleF design(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION);
        require(design.contains(expected_navigation.bounds()) && design.contains(expected_hint.bounds()) &&
            expected_navigation.bounds().bottom() <= style.footer_y && expected_hint.bounds().bottom() <= style.footer_y &&
            expected_hint.bounds().width <= budget + 0.01f,
            "Independent navigation/hint full-shadow bounds escape the design/footer or width budget");
        const Frame actual = frame();
        const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(design, viewport);
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(viewport); list.set_camera(Camera::DEFAULT_CAMERA);
        list.set_filter(TextureFilter::point);
        canvas.draw(list, RectangleF(Vector2F::ZERO, app.resolution_manager()->resolution_vec()));
        list.set_camera(camera);
        const SpriteSheet* sheet = app.render_resources()->sprite_sheet(style.sprite_sheet);
        const auto solid = sheet->resolve_sprite_frame(style.fill_frame);
        sheet->draw(list, solid, camera.visible_rectangle(viewport), style.scrim_colour);
        stage_paper_surface_reference(list, *app.render_resources(), snapshot, fitted, settings,
            navigation == snapshot.menu_text().stage.random_navigation, hint == snapshot.menu_text().stage.locked_hint);
        list.set_filter(TextureFilter::linear); expected_navigation.draw(list); expected_hint.draw(list);
        const Frame reference = read(renderer);
        const std::array<const colourwars::UiTextDropShadow*, 2> texts{ &expected_navigation, &expected_hint };
        const std::array<const char*, 2> roles{ "navigation", "hint" };
        for (size_t index = 0; index < texts.size(); ++index)
        {
            RectangleF bounds = texts[index]->bounds(); bounds.inflate(Vector2F(2.0f)); camera.calculate_view_rectangle(bounds);
            const RectangleI pixels = countdown_crop_bounds(bounds);
            const Frame observed = foreground_crop(actual, pixels, 0), wanted = foreground_crop(reference, pixels, 0);
            const std::string stem = "menu-text-stage-cue-" + name + '-' + roles[index] + '-' +
                std::to_string(actual.width) + 'x' + std::to_string(actual.height);
            if (observed.pixels != wanted.pixels)
            {
                save(observed, output / (stem + "-failure-actual.ppm"));
                save(wanted, output / (stem + "-failure-expected.ppm"));
                const RectangleI delta = foreground_delta_bounds(observed, wanted);
                std::cerr << "Stage cue case=" << name << " role=" << roles[index] << " scale=" << scale <<
                    " crop=(" << pixels.x << ',' << pixels.y << ',' << pixels.width << ',' << pixels.height <<
                    ") delta=(" << delta.x << ',' << delta.y << ',' << delta.width << ',' << delta.height << ")\n";
                throw std::runtime_error("Actual stage navigation/hint differs from its independent full-shadow fixture");
            }
            save(observed, output / (stem + "-detail.ppm"));
            const RectangleF unscaled = index == 0 ? expected_navigation.bounds() : raw;
            records << name << ',' << roles[index] << ',' << actual.width << ',' << actual.height << ',' <<
                (index == 0 ? style.detail_font : style.brand_font) << ',' << unscaled.width << ',' << unscaled.height << ',' <<
                (index == 0 ? 1.0f : scale) << ',' <<
                (index == 0 ? style.detail_shadow_offset.x : shadow.x) << ',' <<
                (index == 0 ? style.detail_shadow_offset.y : shadow.y) << ",true\n";
        }
        std::srand(6329); const int next_random = std::rand(); std::srand(6329);
        require(actual.pixels == frame().pixels && std::rand() == next_random,
            "Frozen stage cue drawing changed pixels or consumed RNG");
    };
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440),
        Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        presentation = original; open(); compare("default-real-browse", original, original, close_navigation, legacy.browse, 2);
        provisional(Key::enter); compare("default-real-locked", original, original, close_navigation, legacy.locked, 2);
        const Frame real_locked = frame(); provisional(Key::left); provisional(Key::right);
        require(frame().pixels == real_locked.pixels, "Locked real stage accepted browsing arrows");
        provisional(Key::escape); provisional(Key::right);
        compare("default-random-browse-count", original, original, legacy.random_navigation, random_hint, 2);
        provisional(Key::enter); compare("default-random-locked", original, original, legacy.random_navigation, legacy.locked, 2);
        const Frame random_locked = frame(); provisional(Key::left); provisional(Key::right);
        require(frame().pixels == random_locked.pixels, "Locked Random stage accepted browsing arrows");
        provisional(Key::escape); provisional(Key::right);
        compare("default-numbered-course", original, original, course_navigation, legacy.browse, 2);
    }
    const Cues old_words{ L"<   CHANCE~   >", L"Locked~ " + std::wstring(45, L'W') + L"\nBack~ " + std::wstring(45, L'W'),
        L"Browse~ " + std::wstring(45, L'W') + L"\nConfirm~ " + std::wstring(45, L'W') };
    const Cues new_words{ L"<   LOTTO   >", L"Confirm to start.\nBack to unlock.", L"Browse the arenas.\nConfirm to lock." };
    const auto with_words = [&](const Cues& words)
    {
        Presentation result = original; auto& text = const_cast<MenuText&>(result.menu_text()).stage;
        text.random_navigation = words.random_navigation; text.locked_hint = words.locked; text.browse_hint = words.browse;
        return result;
    };
    Presentation old = with_words(old_words); old.validate_resources(*app.render_resources(), *app.audio_resources());
    const auto font_without_tilde = [&](const std::string& source_name, const std::string& name)
    {
        const Font* source = app.render_resources()->font(app.render_resources()->resolve_sprite_font(source_name));
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character < U'~'; ++character)
        {
            const Glyph* glyph = source->find(character);
            require(glyph != nullptr, "Stage cue replacement font lacks a baseline printable glyph"); glyphs.push_back(*glyph);
        }
        app.render_resources()->add_font(name,
            std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
        require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(name), L"~"),
            "Stage cue replacement font still contains the old cue glyph");
    };
    const std::string detail_font = "acceptance_stage_navigation_without_tilde";
    const std::string brand_font = "acceptance_stage_hint_without_tilde";
    font_without_tilde(original.menu().detail_font, detail_font);
    font_without_tilde(original.menu().brand_font, brand_font);
    Presentation next = with_words(new_words); auto& next_style = const_cast<MenuStyle&>(next.menu());
    next_style.detail_font = detail_font; next_style.brand_font = brand_font;
    next_style.detail_shadow_offset = Vector2F(-8.0f, -4.0f);
    next_style.shadow_colour = Colour(0.65f, 0.15f, 0.8f); next_style.highlight = Colour::cyan;
    next_style.unhighlight = Colour::yellow;
    auto& next_stage = const_cast<MainMenuPresentation&>(next.main_menu()).stage_select;
    next_stage.briefing.size.x = 620.0f; next_stage.briefing_padding += Vector2F(8.0f, 4.0f);
    next_stage.navigation_position += Vector2F(18.0f, 2.0f);
    next.validate_resources(*app.render_resources(), *app.audio_resources());
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    presentation = old; open(); compare("custom-old-long", old, old, close_navigation, old_words.browse, 1);
    presentation = next; tick(); compare("retained-old-cached-fit", old, old, close_navigation, old_words.browse, 1);
    provisional(Key::right);
    compare("old-navigation-new-live-fit-count", old, next, old_words.random_navigation, random_hint, 2);
    provisional(Key::enter);
    compare("old-locked-new-live-fit", old, next, old_words.random_navigation, old_words.locked, 1);
    provisional(Key::escape);
    compare("old-count-short-fit-reset", old, next, old_words.random_navigation, random_hint, 2);
    provisional(Key::left); compare("old-browse-long-fit-restored", old, next, close_navigation, old_words.browse, 1);
    provisional(Key::enter); compare("old-real-locked", old, next, close_navigation, old_words.locked, 1);
    provisional(Key::escape);
    open(); compare("reopened-new-short", next, next, close_navigation, new_words.browse, 2);
    provisional(Key::right); compare("reopened-new-navigation-count", next, next, new_words.random_navigation, random_hint, 2);
    provisional(Key::enter); compare("reopened-new-locked", next, next, new_words.random_navigation, new_words.locked, 2);
    presentation = original; tick();
    compare("new-after-reversal-cached-fit", next, next, new_words.random_navigation, new_words.locked, 2);
    provisional(Key::escape);
    compare("new-count-original-live-fit", next, original, new_words.random_navigation, random_hint, 2);
    provisional(Key::left); compare("new-browse-original-live-fit", next, original, close_navigation, new_words.browse, 2);
    provisional(Key::enter); compare("new-real-locked-original-live-fit", next, original, close_navigation, new_words.locked, 2);
    provisional(Key::escape);
    open(); compare("restored-default", original, original, close_navigation, legacy.browse, 2);
    require(records.good(), "Stage cue evidence write failed");
    std::cout << "PASS actual stage cues: independent fixed Random navigation/browse/locked hints plus numbered arenas "
        "and eligible Random count across six sizes, full glyph/shadow bounds, unscaled navigation and width-only "
        "hint fit/short reset, separate detail/brand font snapshots, retained/reopened/reversed copy and constructor "
        "geometry/ink with live hint budget/offset on refresh, locked arrows ignored, unchanged plan and RNG\n";
}
