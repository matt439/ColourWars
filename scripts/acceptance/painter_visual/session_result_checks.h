// Four actual Session branches on real Results pages. Expected outcomes, scores
// and Round headings are independent literals, never selector/formatter output.
void session_result_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    const Presentation original = presentation;
    const MatchTuning original_match = tuning.match();
    LevelEndInfo win_a; win_a.total_area = 100.0f; win_a.team_a_area = 60.0f; win_a.team_b_area = 40.0f;
    win_a.team_colours = TeamColourTools::team_colours(TeamColourPair::blue_orange, original.team_palette());
    win_a.player_areas[paint_owners::index(input_slots::keyboard_and_mouse, 0)] = 60.0f;
    win_a.player_areas[paint_owners::index(-1, 1)] = 40.0f;
    LevelEndInfo win_b = win_a; win_b.team_a_area = 40.0f; win_b.team_b_area = 60.0f;
    win_b.player_areas[paint_owners::index(input_slots::keyboard_and_mouse, 0)] = 40.0f;
    win_b.player_areas[paint_owners::index(-1, 1)] = 60.0f;
    LevelEndInfo draw = win_a; draw.team_a_area = draw.team_b_area = 50.0f;
    draw.player_areas[paint_owners::index(input_slots::keyboard_and_mouse, 0)] = 50.0f;
    draw.player_areas[paint_owners::index(-1, 1)] = 50.0f;
    Session complete_a(3); complete_a.finish_round(win_a); complete_a.next_round(); complete_a.finish_round(win_a);
    Session complete_b(5); complete_b.finish_round(win_b); complete_b.next_round(); complete_b.finish_round(win_b);
    complete_b.next_round(); complete_b.finish_round(win_b);
    Session drawn(3); drawn.finish_round(win_a); drawn.next_round(); drawn.finish_round(draw);
    Session ongoing(5); ongoing.finish_round(win_b);
    struct ResultCase
    {
        Session session; LevelEndInfo info; const char* name; int best_of;
        const wchar_t* heading; const wchar_t* score; const wchar_t* outcome;
    };
    const std::array<ResultCase, 4> cases{ {
        { complete_a, win_a, "series3-complete-a", 3, L"Round 2", L"Wins: A 2 - B 0  |  First to 2", L"Team A wins the series!" },
        { complete_b, win_b, "series5-complete-b", 5, L"Round 3", L"Wins: A 0 - B 3  |  First to 3", L"Team B wins the series!" },
        { drawn, draw, "series3-draw", 3, L"Round 2", L"Wins: A 1 - B 0  |  First to 2", L"Draw - no win awarded" },
        { ongoing, win_b, "series5-continues", 5, L"Round 1", L"Wins: A 0 - B 1  |  First to 3", L"Series continues" } } };
    const auto ledger = [](const Session& session)
    {
        return std::vector<int>{ session.is_series() ? 1 : 0, session.complete() ? 1 : 0,
            session.round(), session.team_a_wins(), session.team_b_wins() };
    };
    std::array<std::vector<int>, 4> ledgers;
    for (size_t index = 0; index < cases.size(); ++index) { ledgers[index] = ledger(cases[index].session); }
    const NestedSaveProfile profile(output / "session-result-profile");
    Save preferences; PlaySettings initial; initial.music_volume = initial.sfx_volume = 0;
    initial.aim_sensitivity = 4; initial.view_shake = false; initial.reduce_effects = true;
    require(preferences.set_play_settings_and_save(initial), "Cannot seed Session result's private preferences");
    const auto saved_bytes = [&]
    {
        std::ifstream file(output / "session-result-profile/ColourWars/save_data.json", std::ios::binary);
        require(file.good(), "Session result probe lost its private save");
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
    struct RestoreSessionResult
    {
        StateContext& stack; MenuContext& context; Presentation& presentation; Presentation original;
        ~RestoreSessionResult()
        {
            this->context.cancel_music_exit(); this->context.set_music_update({});
            this->stack.clear(); this->presentation = this->original;
        }
    } restore{ stack, context, presentation, original };
    size_t updates = 0, readbacks = 0, rows = 0, crops = 0, openings = 0, pops = 0, font_controls = 0;
    const auto unchanged = [&]
    {
        for (size_t index = 0; index < cases.size(); ++index)
        {
            const auto& fixture = cases[index];
            require(ledger(fixture.session) == ledgers[index] && fixture.session.score_string(original.menu_text().results) == fixture.score &&
                fixture.session.result_string(original.menu_text().results) == fixture.outcome &&
                fixture.info.total_area == 100.0f && fixture.info.team_a_played && fixture.info.team_b_played &&
                fixture.info.player_areas[paint_owners::index(input_slots::keyboard_and_mouse, 0)] == fixture.info.team_a_area &&
                fixture.info.player_areas[paint_owners::index(-1, 1)] == fixture.info.team_b_area &&
                fixture.info.team_a_area + fixture.info.team_b_area == 100.0f,
                "Session result changed a frozen branch, ledger, score or input-owner credit");
        }
        const auto players = settings.player_settings(); const auto& play = preferences.play_settings();
        require(settings.player_count() == 2 && settings.best_of() == 3 && settings.duration() == 73.0f &&
            settings.stage() == "movement_course" && settings.screen_layout() == ScreenLayout::two_player &&
            players.size() == 2 && players[0].num == input_slots::keyboard_and_mouse && players[1].num == -1 &&
            settings.player_team(0) == PlayerTeam::a && settings.player_team(1) == PlayerTeam::b &&
            settings.player_weapon(0) == WeaponType::sprayer && settings.player_weapon(1) == WeaponType::roller &&
            !context.music_exit_pending(), "Session result changed setup, input claims or pending music state");
        require(play.music_volume == 0 && play.sfx_volume == 0 && play.aim_sensitivity == 4 && !play.view_shake &&
            play.aim_marker && play.reduce_effects && preferences.resolution() == ScreenResolution::s_1920_1080 &&
            !preferences.fullscreen() && preferences.team_palette() == TeamColourPair::random &&
            preferences.records().stages().empty() && !preferences.last_match_setup() && saved_bytes() == saved &&
            tuning.match().duration == original_match.duration && tuning.match().duration_choices == original_match.duration_choices,
            "Session result changed active preferences, records, private bytes or numeric tuning");
        for (const auto& slot : play.sticks)
            { require(slot.movement_deadzone == -1 && slot.aim_deadzone == -1, "Session result changed a pad profile"); }
    };
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    const auto resize = [&](Vector2I size)
    {
        const size_t before = updates; app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        require(updates == before, "Session result resize updated the state stack");
    };
    resize({ 1280, 720 });
    const auto tick = [&](float elapsed = 0.0f)
    {
        require(updates < 76, "Session result exceeded its fixed update budget");
        ++updates; app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed);
    };
    const auto tap = [&](Key key)
        { app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick(); unchanged(); };
    const auto open = [&](const ResultCase& fixture, bool revealed)
    {
        stack.clear(); stack.notify_activation(true); context.cancel_music_exit();
        app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::up, Key::down, Key::left, Key::right,
            Key::enter, Key::escape, Key::space, Key::backspace }) { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        stack.push(std::make_unique<ResultsMenuProbeRoot>(app, canvas));
        stack.push(std::make_unique<ResultsMenuInitial>(&context, fixture.info, fixture.session, revealed));
        ++openings; tick();
        require(stack.depth() == 2, "Session result consumed entry input or lost its actual page"); unchanged();
    };
    const auto proceed = [&]
    {
        require(stack.depth() == 2, "Session result page closed before fresh continuation input");
        tap(Key::enter); require(stack.depth() == 1, "Fresh Enter failed to pop the actual Session result page");
        ++pops; unchanged();
    };
    const auto actual_frame = [&]
    {
        require(readbacks < 94, "Session result exceeded its fixed readback budget");
        app.renderer()->begin_frame(); stack.draw(*app.renderer()); ++readbacks; return read(*app.renderer());
    };
    // Preflight the real glyph extents: winner ends329.21875, score ends402,
    // outcomes end460.6640625 (default) or467 (custom g/braces), plates start482.
    // Multiples of three keep exact floor/ceil crops at both native sizes.
    const RectangleF row_region(420.0f, 333.0f, 1080.0f, 141.0f);
    const RectangleF heading_region(360.0f, 117.0f, 1200.0f, 129.0f);
    const auto project = [&](RectangleF bounds)
    {
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        camera.calculate_view_rectangle(bounds); return countdown_crop_bounds(bounds);
    };
    RectangleF result_bounds;
    const auto reference = [&](const ResultCase& fixture, const Presentation& snapshot, const std::wstring& literal,
        bool details_visible, bool wrong_font = false)
    {
        const auto& style = snapshot.menu(); const auto& look = snapshot.results_menu();
        UiTextDropShadow result("independent-session-result", literal,
            wrong_font ? original.menu().detail_font : style.detail_font, Vector2F::ZERO,
            app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO, !details_visible);
        result.set_position({ (menu_consts::DESIGN_RESOLUTION.x - result.bounds().width) / 2.0f, look.session_result_y });
        UiTextDropShadow score("independent-session-score", fixture.score, style.detail_font, Vector2F::ZERO,
            app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO, !details_visible);
        score.set_position({ (menu_consts::DESIGN_RESOLUTION.x - score.bounds().width) / 2.0f, look.session_score_y });
        UiTextDropShadow heading("independent-session-round", fixture.heading, style.heading_font, Vector2F::ZERO,
            app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO);
        heading.set_position({ (menu_consts::DESIGN_RESOLUTION.x - heading.bounds().width) / 2.0f, look.heading_y });
        const RectangleF panel((menu_consts::DESIGN_RESOLUTION - look.box_size) / 2.0f, look.box_size);
        result_bounds = result.bounds();
        require(result_bounds.width > 0.0f && result_bounds.height > 0.0f && panel.contains(result_bounds) &&
            row_region.contains(result_bounds) && row_region.contains(score.bounds()) &&
            heading_region.contains(heading.bounds()) && score.bounds().bottom() < result_bounds.y,
            "Independent Session outcome/unchanged neighbor bounds escaped their measured unscaled zero-offset comparison regions");
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
        list.set_viewport(viewport); list.set_camera(camera); list.set_filter(TextureFilter::point);
        canvas.draw(list, camera.visible_rectangle(viewport)); FocusGroup empty_focus;
        const MenuPaperArtwork paper(panel, app.render_resources(), &style, &look.art, &empty_focus);
        paper.draw(list); list.set_filter(TextureFilter::linear);
        heading.draw(list); score.draw(list); result.draw(list);
        require(readbacks < 94, "Session result reference exceeded its fixed readback budget");
        ++readbacks; return read(renderer);
    };
    std::ofstream records(output / "session-result.csv");
    records << "case,session,width,height,best_of,complete,round,a_wins,b_wins,revealed,details_visible,proceed_visible,"
        "expected_value,font,raw_width,raw_height,x,y,scale,shadow_x,shadow_y,visible,score_literal,heading_literal,"
        "row_neighbor_match,heading_match,frozen_session_setup_save_rng\n";
    const auto compare = [&](const char* name, const ResultCase& fixture, const Presentation& snapshot,
        const std::wstring& literal, bool revealed, bool details_visible, bool proceed_visible,
        const char* crop = "", bool negative_font = false)
    {
        unchanged(); std::srand(6637); const int next_random = std::rand(); std::srand(6637);
        const Frame actual = actual_frame();
        require(actual.pixels == actual_frame().pixels && std::rand() == next_random,
            "Frozen Session result drawing changed pixels or consumed gameplay RNG");
        const Frame wanted = reference(fixture, snapshot, literal, details_visible);
        const RectangleF measured = result_bounds; const RectangleI area = project(row_region);
        const Frame observed = foreground_crop(actual, area, 0), expected = foreground_crop(wanted, area, 0);
        if (observed.pixels != expected.pixels)
        {
            const RectangleI delta = foreground_delta_bounds(observed, expected);
            std::cerr << "Session result case=" << name << " session=" << fixture.name << " area=" << area.x << ',' <<
                area.y << ',' << area.width << ',' << area.height << " local_delta=" << delta.x << ',' <<
                delta.y << ',' << delta.width << ',' << delta.height << '\n';
            throw std::runtime_error("Actual Session outcome or unchanged score differs from the independent literal zero-offset reference");
        }
        const RectangleI heading_area = project(heading_region);
        require(foreground_crop(actual, heading_area, 0).pixels == foreground_crop(wanted, heading_area, 0).pixels,
            "Session outcome extraction changed the unmodified generated Round heading");
        if (negative_font)
        {
            const Frame wrong = reference(fixture, snapshot, literal, details_visible, true);
            RectangleF tight = measured; tight.inflate(2.0f); const RectangleI tight_area = project(tight);
            require(foreground_crop(actual, tight_area, 0).pixels != foreground_crop(wrong, tight_area, 0).pixels,
                "Session outcome brace font control cannot distinguish the reopened detail font"); ++font_controls;
        }
        if (*crop != '\0')
        {
            require((actual.width == 1280 && actual.height == 720 && observed.width == 720 && observed.height == 94) ||
                (actual.width == 1920 && actual.height == 1080 && observed.width == 1080 && observed.height == 141),
                "Session outcome compact crops drifted from their fixed native pixel dimensions");
            save(observed, output / crop); ++crops;
        }
        records << name << ',' << fixture.name << ',' << actual.width << ',' << actual.height << ',' << fixture.best_of << ',' <<
            (fixture.session.complete() ? "true" : "false") << ',' << fixture.session.round() << ',' <<
            fixture.session.team_a_wins() << ',' << fixture.session.team_b_wins() << ',' << (revealed ? "true" : "false") << ',' <<
            (details_visible ? "true" : "false") << ',' << (proceed_visible ? "true" : "false") << ',';
        const auto ascii = [&](const std::wstring& words)
            { for (const wchar_t character : words) { records << static_cast<char>(character); } };
        ascii(literal); records << ',' << snapshot.menu().detail_font << ',' << measured.width << ',' << measured.height << ',' <<
            measured.x << ',' << measured.y << ",1,0,0," << (details_visible ? "true" : "false") << ',';
        ascii(fixture.score); records << ','; ascii(fixture.heading); records << ",true,true,true\n";
        ++rows; unchanged(); return actual;
    };
    const auto& look = original.results_menu();
    const std::array<const char*, 4> default_crops{ "session-result-a3.ppm", "session-result-b5.ppm",
        "session-result-draw3.ppm", "session-result-continues5.ppm" };
    for (size_t index = 0; index < cases.size(); ++index)
    {
        const auto& fixture = cases[index]; const bool revealed = index != 0;
        open(fixture, revealed);
        if (index == 0)
        {
            tap(Key::enter); require(stack.depth() == 2, "Premature Enter bypassed the Session result fill delay");
            compare("legacy-initial", fixture, original, fixture.outcome, false, false, false);
            tick(look.team_fill_delay + 0.01f); tick(look.team_fill_time + 0.01f);
            compare("legacy-fill-crossed", fixture, original, fixture.outcome, false, false, false);
            tick(); compare("legacy-details", fixture, original, fixture.outcome, false, true, false);
            tap(Key::enter); require(stack.depth() == 2, "Premature Enter bypassed the Session result post-reveal delay");
            tick(look.show_results_delay + 0.01f);
            compare("legacy-prompt-crossed", fixture, original, fixture.outcome, false, true, false);
            tick();
        }
        else
        {
            if (index == 1)
            {
                compare("revealed-initial", fixture, original, fixture.outcome, true, true, false);
                tap(Key::enter); require(stack.depth() == 2, "Revealed Session result skipped its remaining prompt delay");
            }
            tick(look.show_results_delay + 0.01f); tick();
        }
        compare("legacy-ready", fixture, original, fixture.outcome, revealed, true, true, default_crops[index]);
        if (index == 0) { tap(Key::escape); require(stack.depth() == 2, "Escape bypassed Session result continuation"); }
        proceed();
    }
    const std::array<const wchar_t*, 4> old_words{ L"A wins~ {A}", L"B wins~ {B}", L"Draw~ {D}", L"More~ {N}" };
    const std::array<const wchar_t*, 4> next_words{ L"A again {A}", L"B again {B}", L"No win {D}", L"Continue {N}" };
    Presentation old = original, next = original;
    const auto assign = [](MenuResultsText& text, const std::array<const wchar_t*, 4>& words)
    {
        text.series_team_a_wins = words[0]; text.series_team_b_wins = words[1];
        text.draw_no_win = words[2]; text.series_continues = words[3];
    };
    assign(const_cast<MenuText&>(old.menu_text()).results, old_words);
    assign(const_cast<MenuText&>(next.menu_text()).results, next_words);
    const Font* source = app.render_resources()->font(app.render_resources()->resolve_sprite_font(original.menu().detail_font));
    std::vector<Glyph> glyphs;
    for (char32_t character = U' '; character < U'~'; ++character)
    {
        const Glyph* source_glyph = source->find(character);
        require(source_glyph != nullptr, "Session outcome font fixture lacks an original printable glyph");
        Glyph glyph = *source_glyph;
        if (character == U'{' || character == U'}') { glyph.x_offset += 1.25f; glyph.x_advance += 0.75f; }
        glyphs.push_back(glyph);
    }
    const std::string next_font = "session_result_braces";
    app.render_resources()->add_font(next_font,
        std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
    require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(next_font), L"~"),
        "Session outcome replacement font still renders retained old copy's tilde");
    const_cast<MenuStyle&>(next.menu()).detail_font = next_font;
    old.validate_resources(*app.render_resources(), *app.audio_resources());
    next.validate_resources(*app.render_resources(), *app.audio_resources());
    for (size_t index = 0; index < cases.size(); ++index)
    {
        const auto& fixture = cases[index];
        presentation = old; open(fixture, true); tick(look.show_results_delay + 0.01f); tick();
        const Frame old_frame = compare("old-primary", fixture, old, old_words[index], true, true, true,
            index == 0 ? "session-result-old-720.ppm" : "");
        presentation = next;
        require(compare("retained-old", fixture, old, old_words[index], true, true, true).pixels == old_frame.pixels,
            "Source assignment rebased a constructed Session outcome's text or font");
        if (index == 0)
        {
            resize({ 1920, 1080 });
            compare("old-1080", fixture, old, old_words[index], true, true, true, "session-result-old-1080.ppm");
            resize({ 1280, 720 });
            require(compare("old-return720", fixture, old, old_words[index], true, true, true).pixels == old_frame.pixels,
                "Session old outcome resize changed its captured text or font");
        }
        proceed(); open(fixture, true); tick(look.show_results_delay + 0.01f); tick();
        const Frame next_frame = compare("reopened-new", fixture, next, next_words[index], true, true, true,
            index == 0 ? "session-result-new-720.ppm" : "", true);
        presentation = original;
        require(compare("retained-new", fixture, next, next_words[index], true, true, true).pixels == next_frame.pixels,
            "Source reversal rebased a reopened Session outcome's text or detail font");
        if (index == 0)
        {
            resize({ 1920, 1080 });
            compare("new-1080", fixture, next, next_words[index], true, true, true, "session-result-new-1080.ppm");
            resize({ 1280, 720 });
            require(compare("new-return720", fixture, next, next_words[index], true, true, true).pixels == next_frame.pixels,
                "Session new outcome resize changed its captured text or font");
        }
        proceed();
    }
    presentation = original; open(cases[0], true); tick(look.show_results_delay + 0.01f); tick();
    compare("restored-default", cases[0], original, cases[0].outcome, true, true, true); proceed();
    require(updates == 76 && readbacks == 94 && rows == 30 && crops == 8 && openings == 13 && pops == 13 &&
        font_controls == 4 && records.good(), "Session result drifted from its fixed update/readback/CSV/crop/action budget");
    std::ofstream summary(output / "session-result-summary.json");
    summary << "{\"outcome\":\"passed\",\"scope\":\"session-result-only\",\"updates\":76,\"readbacks\":94,"
        "\"csv_rows\":30,\"compact_crops\":8,\"page_openings\":13,\"proceed_pops\":13,\"font_negative_controls\":4,"
        "\"resize_updates\":0,\"save_calls\":1,\"crop_pixels\":710640}\n";
    require(summary.good(), "Cannot retain Session result summary");
    std::cout << "PASS Session outcome leaf: four real best-of3/5 branches and independent plain literals including braces; "
        "actual centered unscaled zero-offset outcome, unchanged score and generated Round heading; captured copy/detailfont "
        "through assignment/reopen/reversal, real unrevealed/revealed delays, premature Enter/Escape refusal and13fresh pops; "
        "frozen ledger/setup/private Save/RNG, zero-update720/1080/720resize,76updates94readbacks30CSVrows4fontcontrols "
        "and8compactcrops710640pixels/2132036PPMbytes,oneprivate seedSave. Both presets normalreader; "
        "no match/global-input/physical-pad/listening/human/performance acceptance.\n";
}
