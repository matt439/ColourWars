// Actual Results pages and normal MenuInput. The three extracted strings are
// independent plain literals here; braces have no formatting meaning.
void results_text_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    const Presentation original = presentation;
    const MatchTuning original_match = tuning.match();
    LevelEndInfo victory; victory.total_area = 100.0f;
    victory.team_a_area = 60.0f; victory.team_b_area = 40.0f;
    victory.team_colours = TeamColourTools::team_colours(TeamColourPair::blue_orange,
        original.team_palette());
    victory.player_areas[paint_owners::index(input_slots::keyboard_and_mouse, 0)] = 35.0f;
    victory.player_areas[paint_owners::index(-1, 1)] = 25.0f;
    LevelEndInfo drawn = victory; drawn.team_a_area = drawn.team_b_area = 50.0f;
    Session single; single.finish_round(victory);
    Session series(3); series.finish_round(victory); series.next_round(); series.finish_round(drawn);
    const auto ledger = [](const Session& session)
    {
        return std::vector<int>{ session.is_series() ? 1 : 0, session.complete() ? 1 : 0,
            session.round(), session.team_a_wins(), session.team_b_wins() };
    };
    const auto single_ledger = ledger(single), series_ledger = ledger(series);
    RecordOutcome record; record.recorded = record.new_best = true; record.value = record.best = 68.4f;

    const NestedSaveProfile profile(output / "results-text-profile");
    Save preferences; PlaySettings initial; initial.music_volume = initial.sfx_volume = 0;
    initial.aim_sensitivity = 4; initial.view_shake = false; initial.reduce_effects = true;
    require(preferences.set_play_settings_and_save(initial), "Cannot seed Results text's private preferences");
    const auto saved_bytes = [&]
    {
        std::ifstream file(output / "results-text-profile/ColourWars/save_data.json", std::ios::binary);
        require(file.good(), "Results text probe lost its private save");
        return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    };
    const std::string saved = saved_bytes();
    GameData data; data.set_application(&app); data.set_save(&preferences);
    data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MenuLevelSettings settings;
    settings.set_player_count(2); settings.set_best_of(3); settings.set_duration(73.0f);
    settings.set_stage("movement_course"); settings.set_screen_layout(ScreenLayout::two_player);
    settings.begin_input_claims(input_slots::keyboard_and_mouse);
    settings.set_player_team(0, PlayerTeam::a); settings.set_player_team(1, PlayerTeam::b);
    settings.set_player_weapon(0, WeaponType::sprayer); settings.set_player_weapon(1, WeaponType::roller);
    MenuContext context(&data, &input, &settings); StateContext stack;
    struct RestoreResultsText
    {
        StateContext& stack; MenuContext& context; Presentation& presentation; Presentation original;
        ~RestoreResultsText()
        {
            this->context.cancel_music_exit(); this->context.set_music_update({});
            this->stack.clear(); this->presentation = this->original;
        }
    } restore{ stack, context, presentation, original };
    size_t updates = 0, readbacks = 0, rows = 0, crops = 0, openings = 0, pops = 0, font_controls = 0;
    const auto unchanged = [&]
    {
        const auto players = settings.player_settings(); const auto& play = preferences.play_settings();
        require(ledger(single) == single_ledger && ledger(series) == series_ledger &&
            single.score_string() == L"Wins: A 1 - B 0  |  First to 1" && single.result_string(original.menu_text().results) == L"Team A wins the series!" &&
            series.score_string() == L"Wins: A 1 - B 0  |  First to 2" && series.result_string(original.menu_text().results) == L"Draw - no win awarded" &&
            record.recorded && record.new_best && !record.coverage && record.value == 68.4f && record.best == 68.4f &&
            victory.total_area == 100.0f && victory.team_a_area == 60.0f && victory.team_b_area == 40.0f &&
            victory.player_areas[paint_owners::index(input_slots::keyboard_and_mouse, 0)] == 35.0f &&
            victory.player_areas[paint_owners::index(-1, 1)] == 25.0f,
            "Results text changed its frozen Session ledger, record or input-owner credits");
        require(settings.player_count() == 2 && settings.best_of() == 3 && settings.duration() == 73.0f &&
            settings.stage() == "movement_course" && settings.screen_layout() == ScreenLayout::two_player &&
            players.size() == 2 && players[0].num == input_slots::keyboard_and_mouse && players[1].num == -1 &&
            settings.player_team(0) == PlayerTeam::a && settings.player_team(1) == PlayerTeam::b &&
            settings.player_weapon(0) == WeaponType::sprayer && settings.player_weapon(1) == WeaponType::roller &&
            !context.music_exit_pending(), "Results text changed setup, input claims or pending music state");
        require(play.music_volume == 0 && play.sfx_volume == 0 && play.aim_sensitivity == 4 && !play.view_shake &&
            play.aim_marker && play.reduce_effects && preferences.resolution() == ScreenResolution::s_1920_1080 &&
            !preferences.fullscreen() && preferences.team_palette() == TeamColourPair::random &&
            preferences.records().stages().empty() && !preferences.last_match_setup() && saved_bytes() == saved &&
            tuning.match().duration == original_match.duration && tuning.match().duration_choices == original_match.duration_choices,
            "Results text changed active preferences, records, private bytes or numeric tuning");
        for (const auto& slot : play.sticks)
            { require(slot.movement_deadzone == -1 && slot.aim_deadzone == -1, "Results text changed a pad profile"); }
    };
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    const auto resize = [&](Vector2I size)
    {
        const size_t before = updates; app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        require(updates == before, "Results text resize updated the state stack");
    };
    resize({ 1280, 720 });
    const auto tick = [&](float elapsed = 0.0f)
    {
        require(updates < 43, "Results text exceeded its fixed update budget");
        ++updates; app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed);
    };
    const auto tap = [&](Key key)
        { app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick(); unchanged(); };
    const auto open = [&](const Session& session, bool revealed)
    {
        stack.clear(); stack.notify_activation(true); context.cancel_music_exit();
        app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::up, Key::down, Key::left, Key::right,
            Key::enter, Key::escape, Key::space, Key::backspace }) { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        stack.push(std::make_unique<ResultsMenuProbeRoot>(app, canvas));
        stack.push(std::make_unique<ResultsMenuInitial>(&context, victory, session, revealed, record));
        ++openings; tick();
        require(stack.depth() == 2, "Results text consumed entry input or lost its actual page"); unchanged();
    };
    const auto proceed = [&]
    {
        require(stack.depth() == 2, "Results text page closed before fresh continuation input");
        tap(Key::enter); require(stack.depth() == 1, "Fresh Enter failed to pop the actual Results text page");
        ++pops; unchanged();
    };
    const auto actual_frame = [&]
    {
        require(readbacks < 52, "Results text exceeded its fixed readback budget");
        app.renderer()->begin_frame(); stack.draw(*app.renderer()); ++readbacks; return read(*app.renderer());
    };
    // These clear bands isolate the three roles while retaining the actual
    // paper and the unmodified record on the contribution heading's row.
    const std::array<RectangleF, 3> regions{ RectangleF(360.0f, 117.0f, 1200.0f, 129.0f),
        RectangleF(450.0f, 633.0f, 900.0f, 45.0f), RectangleF(360.0f, 870.0f, 1200.0f, 81.0f) };
    const auto project = [&](RectangleF bounds)
    {
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        camera.calculate_view_rectangle(bounds); return countdown_crop_bounds(bounds);
    };
    struct ResultsTextMetrics { std::array<RectangleF, 3> bounds; } metrics;
    const auto reference = [&](const Presentation& snapshot, const std::array<const wchar_t*, 3>& words,
        bool details_visible, bool proceed_visible, bool wrong_fonts = false)
    {
        const auto& style = snapshot.menu(); const auto& look = snapshot.results_menu();
        const std::array<std::string, 3> fonts = wrong_fonts ?
            std::array<std::string, 3>{ original.menu().heading_font, original.results_menu().contribution_font,
                original.menu().detail_font } :
            std::array<std::string, 3>{ style.heading_font, look.contribution_font, style.detail_font };
        UiTextDropShadow heading("independent-results-heading", words[0], fonts[0], Vector2F::ZERO,
            app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO);
        heading.set_position({ (menu_consts::DESIGN_RESOLUTION.x - heading.bounds().width) / 2.0f, look.heading_y });
        UiTextDropShadow contribution("independent-results-contribution", words[1], fonts[1], look.contribution_position,
            app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO, !details_visible);
        UiTextDropShadow prompt("independent-results-proceed", words[2], fonts[2], Vector2F::ZERO,
            app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO, !proceed_visible);
        prompt.set_position({ (menu_consts::DESIGN_RESOLUTION.x - prompt.bounds().width) / 2.0f, look.proceed_y });
        UiTextDropShadow record_line("independent-results-record", L"NEW BEST 68.4%", fonts[1], Vector2F::ZERO,
            app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO, !details_visible);
        const float fill_right = (menu_consts::DESIGN_RESOLUTION.x + look.fill_box_size.x) / 2.0f;
        record_line.set_position({ fill_right - record_line.bounds().width, look.record_y });
        metrics.bounds = { heading.bounds(), contribution.bounds(), prompt.bounds() };
        const RectangleF panel((menu_consts::DESIGN_RESOLUTION - look.box_size) / 2.0f, look.box_size);
        for (size_t role = 0; role < regions.size(); ++role)
        {
            require(metrics.bounds[role].width > 0.0f && metrics.bounds[role].height > 0.0f &&
                panel.contains(metrics.bounds[role]) && regions[role].contains(metrics.bounds[role]),
                "Independent Results text's unscaled zero-offset bounds escaped the paper or strict comparison band");
        }
        require(record_line.bounds().x > contribution.bounds().right() &&
            regions[1].contains(record_line.bounds()) && contribution.bounds().height < look.contribution_spacing,
            "Authored Results contribution heading lost strict clearance from the unchanged record or next roster row");
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
        list.set_viewport(viewport); list.set_camera(camera); list.set_filter(TextureFilter::point);
        canvas.draw(list, camera.visible_rectangle(viewport)); FocusGroup empty_focus;
        const MenuPaperArtwork paper(panel, app.render_resources(), &style, &look.art, &empty_focus);
        paper.draw(list); list.set_filter(TextureFilter::linear);
        heading.draw(list); contribution.draw(list); record_line.draw(list); prompt.draw(list);
        require(readbacks < 52, "Results text reference exceeded its fixed readback budget");
        ++readbacks; return read(renderer);
    };
    const auto compact = [&](const Frame& actual)
    {
        std::array<Frame, 3> bands{ foreground_crop(actual, project(regions[0]), 0),
            foreground_crop(actual, project(regions[1]), 0), foreground_crop(actual, project(regions[2]), 0) };
        Frame strip{ bands[0].width, bands[0].height + bands[1].height + bands[2].height, {} };
        strip.pixels.assign(static_cast<size_t>(strip.width) * static_cast<size_t>(strip.height) * 4, 0);
        int top = 0;
        for (const Frame& band : bands)
        {
            for (int y = 0; y < band.height; ++y)
            {
                const size_t from = static_cast<size_t>(y) * static_cast<size_t>(band.width) * 4;
                const size_t to = static_cast<size_t>(top + y) * static_cast<size_t>(strip.width) * 4;
                std::copy_n(band.pixels.begin() + from, static_cast<size_t>(band.width) * 4, strip.pixels.begin() + to);
            }
            top += band.height;
        }
        require((actual.width == 1280 && actual.height == 720 && strip.width == 800 && strip.height == 170) ||
            (actual.width == 1920 && actual.height == 1080 && strip.width == 1200 && strip.height == 255),
            "Results text compact strips drifted from their fixed native pixel dimensions");
        return strip;
    };
    std::ofstream records(output / "results-text.csv");
    records << "case,session,role,width,height,revealed,details_visible,proceed_visible,series,round,a_wins,b_wins,"
        "expected_value,font,raw_width,raw_height,x,y,scale,shadow_x,shadow_y,visible,full_region_match,frozen_session_setup_save_rng\n";
    const auto compare = [&](const char* name, const Session& session, const Presentation& snapshot,
        const std::array<const wchar_t*, 3>& words, bool revealed, bool details_visible, bool proceed_visible,
        const char* crop = "", bool negative_fonts = false)
    {
        unchanged(); std::srand(5419); const int next_random = std::rand(); std::srand(5419);
        const Frame actual = actual_frame();
        require(actual.pixels == actual_frame().pixels && std::rand() == next_random,
            "Frozen Results text drawing changed pixels or consumed gameplay RNG");
        const Frame wanted = reference(snapshot, words, details_visible, proceed_visible);
        const ResultsTextMetrics measured = metrics;
        const std::array<const char*, 3> roles{ "heading", "contribution", "proceed" };
        const std::array<std::string, 3> fonts{ snapshot.menu().heading_font,
            snapshot.results_menu().contribution_font, snapshot.menu().detail_font };
        for (size_t role = 0; role < regions.size(); ++role)
        {
            const RectangleI area = project(regions[role]);
            const Frame observed = foreground_crop(actual, area, 0), expected = foreground_crop(wanted, area, 0);
            if (observed.pixels != expected.pixels)
            {
                const RectangleI delta = foreground_delta_bounds(observed, expected);
                std::cerr << "Results text case=" << name << " role=" << roles[role] << " area=" << area.x << ',' <<
                    area.y << ',' << area.width << ',' << area.height << " local_delta=" << delta.x << ',' <<
                    delta.y << ',' << delta.width << ',' << delta.height << '\n';
                throw std::runtime_error("Actual Results plain text, font, unscaled placement or reveal gate differs from its independent literal reference");
            }
            records << name << ',' << (session.is_series() ? "series" : "single") << ',' << roles[role] << ',' <<
                actual.width << ',' << actual.height << ',' << (revealed ? "true" : "false") << ',' <<
                (details_visible ? "true" : "false") << ',' << (proceed_visible ? "true" : "false") << ',' <<
                (session.is_series() ? "true" : "false") << ',' << session.round() << ',' << session.team_a_wins() << ',' <<
                session.team_b_wins() << ',';
            for (const wchar_t character : std::wstring(words[role])) { records << static_cast<char>(character); }
            const auto& bounds = measured.bounds[role];
            const bool visible = role == 0 || (role == 1 ? details_visible : proceed_visible);
            records << ',' << fonts[role] << ',' << bounds.width << ',' << bounds.height << ',' << bounds.x << ',' << bounds.y <<
                ",1,0,0," << (visible ? "true" : "false") << ",true,true\n"; ++rows;
        }
        if (negative_fonts)
        {
            const Frame wrong = reference(snapshot, words, details_visible, proceed_visible, true);
            for (const RectangleF& bounds : measured.bounds)
            {
                RectangleF tight = bounds; tight.inflate(2.0f); const RectangleI area = project(tight);
                require(foreground_crop(actual, area, 0).pixels != foreground_crop(wrong, area, 0).pixels,
                    "Results text font control cannot distinguish one reopened role's replacement font"); ++font_controls;
            }
        }
        if (*crop != '\0') { save(compact(actual), output / crop); ++crops; }
        unchanged(); return actual;
    };
    const std::array<const wchar_t*, 3> legacy{ L"Results", L"Surface held (% of arena)", L"Press A/Enter to proceed" };
    const std::array<const wchar_t*, 3> legacy_series{ L"Round 2", legacy[1], legacy[2] };
    const auto& look = original.results_menu();
    open(single, false);
    tap(Key::enter); require(stack.depth() == 2, "Premature Enter bypassed Results' fill delay");
    compare("plain-initial", single, original, legacy, false, false, false);
    tick(look.team_fill_delay + 0.01f); tick(look.team_fill_time + 0.01f);
    compare("plain-fill-crossed", single, original, legacy, false, false, false);
    tick(); compare("plain-details", single, original, legacy, false, true, false);
    tap(Key::enter); require(stack.depth() == 2, "Premature Enter bypassed Results' post-reveal prompt delay");
    tick(look.show_results_delay + 0.01f);
    compare("plain-prompt-crossed", single, original, legacy, false, true, false);
    tick(); compare("plain-ready", single, original, legacy, false, true, true, "results-text-default-single.ppm");
    tap(Key::escape); require(stack.depth() == 2, "Escape bypassed Results' continuation action"); proceed();
    open(series, true); compare("series-initial", series, original, legacy_series, true, true, false);
    tap(Key::enter); require(stack.depth() == 2, "Revealed Results skipped its remaining prompt delay");
    tick(look.show_results_delay + 0.01f); tick();
    compare("series-ready", series, original, legacy_series, true, true, true, "results-text-default-series.ppm");
    tap(Key::escape); require(stack.depth() == 2, "Escape bypassed revealed Results continuation"); proceed();

    Presentation old = original, next = original;
    const std::array<const wchar_t*, 3> old_words{ L"Results~ {1}", L"Held~ {area}", L"Continue~ {A}" };
    const std::array<const wchar_t*, 3> next_words{ L"Score {2}", L"Owned {area}", L"Proceed {B}" };
    const auto assign = [](MenuResultsText& text, const std::array<const wchar_t*, 3>& words)
    {
        text.single_match_heading = words[0]; text.contribution_heading = words[1]; text.proceed = words[2];
    };
    assign(const_cast<MenuText&>(old.menu_text()).results, old_words);
    assign(const_cast<MenuText&>(next.menu_text()).results, next_words);
    const std::array<std::string, 3> source_fonts{ original.menu().heading_font,
        original.results_menu().contribution_font, original.menu().detail_font };
    const std::array<std::string, 3> next_fonts{ "results_text_heading_letters",
        "results_text_contribution_letters", "results_text_detail_letters" };
    for (size_t role = 0; role < source_fonts.size(); ++role)
    {
        const Font* source = app.render_resources()->font(app.render_resources()->resolve_sprite_font(source_fonts[role]));
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character < U'~'; ++character)
        {
            const Glyph* source_glyph = source->find(character);
            require(source_glyph != nullptr, "Results font fixture lacks an original printable glyph");
            Glyph glyph = *source_glyph;
            if (character == U'{' || character == U'}') { glyph.x_offset += 1.25f; glyph.x_advance += 0.75f; }
            glyphs.push_back(glyph);
        }
        app.render_resources()->add_font(next_fonts[role],
            std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
        require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(next_fonts[role]), L"~"),
            "Results replacement font still renders its retained old role's tilde");
    }
    const_cast<MenuStyle&>(next.menu()).heading_font = next_fonts[0];
    const_cast<ResultsMenuPresentation&>(next.results_menu()).contribution_font = next_fonts[1];
    const_cast<MenuStyle&>(next.menu()).detail_font = next_fonts[2];
    old.validate_resources(*app.render_resources(), *app.audio_resources());
    next.validate_resources(*app.render_resources(), *app.audio_resources());
    presentation = old; open(single, true); tick(look.show_results_delay + 0.01f); tick();
    const Frame old_frame = compare("old-ready", single, old, old_words, true, true, true, "results-text-old-720.ppm");
    presentation = next;
    require(compare("retained-old", single, old, old_words, true, true, true).pixels == old_frame.pixels,
        "Results source assignment rebased constructed plain copy or one of its three fonts");
    resize({ 1920, 1080 });
    compare("old-1080", single, old, old_words, true, true, true, "results-text-old-1080.ppm");
    resize({ 1280, 720 });
    require(compare("old-return720", single, old, old_words, true, true, true).pixels == old_frame.pixels,
        "Results old page resize changed captured text or font"); proceed();
    open(single, true); tick(look.show_results_delay + 0.01f); tick();
    const Frame next_frame = compare("reopened-new", single, next, next_words, true, true, true,
        "results-text-new-720.ppm", true);
    presentation = original;
    require(compare("retained-new", single, next, next_words, true, true, true).pixels == next_frame.pixels,
        "Results source reversal rebased reopened plain copy or one of its fonts");
    resize({ 1920, 1080 });
    compare("new-1080", single, next, next_words, true, true, true, "results-text-new-1080.ppm");
    resize({ 1280, 720 });
    require(compare("new-return720", single, next, next_words, true, true, true).pixels == next_frame.pixels,
        "Results new page resize changed captured text or font"); proceed();
    presentation = next; open(series, true); tick(look.show_results_delay + 0.01f); tick();
    const std::array<const wchar_t*, 3> next_series{ L"Round 2", next_words[1], next_words[2] };
    compare("new-series", series, next, next_series, true, true, true, "results-text-new-series.ppm"); proceed();
    presentation = original; open(single, true); tick(look.show_results_delay + 0.01f); tick();
    compare("restored-default", single, original, legacy, true, true, true, "results-text-restored-default.ppm"); proceed();
    require(updates == 43 && readbacks == 52 && rows == 51 && crops == 8 && openings == 6 && pops == 6 &&
        font_controls == 3 && records.good(), "Results text drifted from its fixed update/readback/CSV/crop/action budget");
    std::ofstream summary(output / "results-text-summary.json");
    summary << "{\"outcome\":\"passed\",\"scope\":\"results-text-only\",\"updates\":43,\"readbacks\":52,"
        "\"csv_rows\":51,\"compact_crops\":8,\"page_openings\":6,\"proceed_pops\":6,\"font_negative_controls\":3,"
        "\"resize_updates\":0,\"save_calls\":1,\"crop_pixels\":1428000}\n";
    require(summary.good(), "Cannot retain Results text summary");
    std::cout << "PASS Results text leaf: three independent plain literals with braces, actual unscaled zero-offset "
        "heading/contribution/proceed roles and unchanged record clearance; captured copy and three distinct fonts "
        "through assignment/reopen/reversal, single/series Round2, unrevealed/revealed detail and prompt timing, "
        "premature Enter/Escape refusal and six fresh Enter pops; frozen Session/setup/private Save/RNG, "
        "43 updates,52 readbacks,51 CSV rows,eight compact strips,three font controls,one private seed Save. "
        "Both presets use the same normal reader; no match/global-input/physical-pad/listening/human/performance acceptance.\n";
}
