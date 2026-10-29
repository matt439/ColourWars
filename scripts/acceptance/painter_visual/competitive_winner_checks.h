// Actual competitive Results winner rows and the unchanged one-team coverage
// branch. All target and neighboring words below are independent literals.
void competitive_winner_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
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
    LevelEndInfo solo = win_a; solo.team_a_area = 72.0f; solo.team_b_area = 0.0f; solo.team_b_played = false;
    solo.player_areas[paint_owners::index(input_slots::keyboard_and_mouse, 0)] = 72.0f;
    solo.player_areas[paint_owners::index(-1, 1)] = 0.0f;
    Session series_a(3), series_b(3), series_draw(3), single;
    series_a.finish_round(win_a); series_b.finish_round(win_b); series_draw.finish_round(draw); single.finish_round(solo);
    struct WinnerCase
    {
        Session session; LevelEndInfo info; const char* name; int players; int best_of; int target;
        const wchar_t* winner; const wchar_t* heading; const wchar_t* score; const wchar_t* outcome;
    };
    const std::array<WinnerCase, 4> cases{ {
        { series_a, win_a, "competitive-a", 2, 3, 2, L"Team A Wins!", L"Round 1",
            L"Wins: A 1 - B 0  |  First to 2", L"Series continues" },
        { series_b, win_b, "competitive-b", 2, 3, 2, L"Team B Wins!", L"Round 1",
            L"Wins: A 0 - B 1  |  First to 2", L"Series continues" },
        { series_draw, draw, "competitive-draw", 2, 3, 2, L"Draw!", L"Round 1",
            L"Wins: A 0 - B 0  |  First to 2", L"Draw - no win awarded" },
        { single, solo, "solo", 1, 1, 1, L"72.0% Painted", L"Results",
            L"Wins: A 0 - B 0  |  First to 1", L"Team B wins the series!" } } };
    const auto ledger = [](const Session& session)
    {
        return std::vector<int>{ session.is_series() ? 1 : 0, session.complete() ? 1 : 0,
            session.round(), session.team_a_wins(), session.team_b_wins() };
    };
    std::array<std::vector<int>, 4> ledgers;
    for (size_t index = 0; index < cases.size(); ++index) { ledgers[index] = ledger(cases[index].session); }
    const NestedSaveProfile profile(output / "competitive-winner-profile");
    Save preferences; PlaySettings initial; initial.music_volume = initial.sfx_volume = 0;
    initial.aim_sensitivity = 4; initial.view_shake = false; initial.reduce_effects = true;
    require(preferences.set_play_settings_and_save(initial), "Cannot seed competitive winner's private preferences");
    const auto saved_bytes = [&]
    {
        std::ifstream file(output / "competitive-winner-profile/ColourWars/save_data.json", std::ios::binary);
        require(file.good(), "Competitive winner probe lost its private save");
        return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    };
    const std::string saved = saved_bytes();
    GameData data; data.set_application(&app); data.set_save(&preferences);
    data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MatchSetup settings;
    settings.set_duration(73.0f); settings.set_stage("movement_course");
    MenuContext context(&data, &input, &settings); StateContext stack;
    struct RestoreCompetitiveWinner
    {
        StateContext& stack; MenuContext& context; Presentation& presentation; Presentation original;
        ~RestoreCompetitiveWinner()
        {
            this->context.cancel_music_exit(); this->context.set_music_update({});
            this->stack.clear(); this->presentation = this->original;
        }
    } restore{ stack, context, presentation, original };
    size_t updates = 0, readbacks = 0, rows = 0, crops = 0, openings = 0, pops = 0, font_controls = 0;
    int active_players = 0, active_best_of = 0;
    const auto unchanged = [&]
    {
        for (size_t index = 0; index < cases.size(); ++index)
        {
            const auto& fixture = cases[index];
            require(ledger(fixture.session) == ledgers[index] &&
                fixture.session.score_string(original.menu_text().results) == fixture.score &&
                fixture.session.result_string(original.menu_text().results) == fixture.outcome &&
                fixture.info.winning_team_string(original.menu_text().results) == fixture.winner &&
                fixture.info.total_area == 100.0f && fixture.info.team_a_played &&
                fixture.info.team_b_played == (fixture.players == 2) &&
                fixture.info.player_areas[paint_owners::index(input_slots::keyboard_and_mouse, 0)] == fixture.info.team_a_area &&
                fixture.info.player_areas[paint_owners::index(-1, 1)] == fixture.info.team_b_area &&
                fixture.info.team_a_area + fixture.info.team_b_area == (fixture.players == 1 ? 72.0f : 100.0f),
                "Competitive winner changed a frozen branch, ledger, solo coverage or input-owner credit");
        }
        const auto players = settings.player_settings(); const auto& play = preferences.play_settings();
        require(settings.player_count() == active_players && settings.best_of() == active_best_of &&
            settings.duration() == 73.0f && settings.stage() == "movement_course" &&
            settings.screen_layout() == (active_players == 1 ? ScreenLayout::one_player : ScreenLayout::two_player) &&
            players.size() == static_cast<size_t>(active_players) && players[0].num == input_slots::keyboard_and_mouse &&
            settings.player_team(0) == PlayerTeam::a && settings.player_weapon(0) == WeaponType::sprayer &&
            !context.music_exit_pending(), "Competitive winner changed setup, first input claim or pending music state");
        if (active_players == 2)
        {
            require(players[1].num == -1 && settings.player_team(1) == PlayerTeam::b &&
                settings.player_weapon(1) == WeaponType::roller, "Competitive winner changed the frozen second seat");
        }
        require(play.music_volume == 0 && play.sfx_volume == 0 && play.aim_sensitivity == 4 && !play.view_shake &&
            play.aim_marker && play.reduce_effects && preferences.resolution() == ScreenResolution::s_1920_1080 &&
            !preferences.fullscreen() && preferences.team_palette() == TeamColourPair::random &&
            preferences.records().stages().empty() && !preferences.last_match_setup() && saved_bytes() == saved &&
            tuning.match().duration == original_match.duration && tuning.match().duration_choices == original_match.duration_choices,
            "Competitive winner changed active preferences, records, private bytes or numeric tuning");
        for (const auto& slot : play.sticks)
            { require(slot.movement_deadzone == -1 && slot.aim_deadzone == -1, "Competitive winner changed a pad profile"); }
    };
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    const auto resize = [&](Vector2I size)
    {
        const size_t before = updates; app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        require(updates == before, "Competitive winner resize updated the state stack");
    };
    resize({ 1280, 720 });
    const auto tick = [&](float elapsed = 0.0f)
    {
        require(updates < 102, "Competitive winner exceeded its fixed update budget");
        ++updates; app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed);
    };
    const auto tap = [&](Key key)
        { app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick(); unchanged(); };
    const auto open = [&](const WinnerCase& fixture, bool revealed)
    {
        stack.clear(); stack.notify_activation(true); context.cancel_music_exit();
        active_players = fixture.players; active_best_of = fixture.best_of;
        settings.set_player_count(active_players); settings.set_best_of(active_best_of);
        settings.set_screen_layout(active_players == 1 ? ScreenLayout::one_player : ScreenLayout::two_player);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_player_team(0, PlayerTeam::a); settings.set_player_weapon(0, WeaponType::sprayer);
        if (active_players == 2)
            { settings.set_player_team(1, PlayerTeam::b); settings.set_player_weapon(1, WeaponType::roller); }
        app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::up, Key::down, Key::left, Key::right,
            Key::enter, Key::escape, Key::space, Key::backspace }) { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        stack.push(std::make_unique<ResultsMenuProbeRoot>(app, canvas));
        stack.push(std::make_unique<ResultsMenuInitial>(&context, fixture.info, fixture.session, revealed));
        ++openings; tick();
        require(stack.depth() == 2, "Competitive winner consumed entry input or lost its actual page"); unchanged();
    };
    const auto proceed = [&]
    {
        require(stack.depth() == 2, "Competitive winner page closed before fresh continuation input");
        tap(Key::enter); require(stack.depth() == 1, "Fresh Enter failed to pop the actual competitive winner page");
        ++pops; unchanged();
    };
    const auto actual_frame = [&]
    {
        require(readbacks < 99, "Competitive winner exceeded its fixed readback budget");
        app.renderer()->begin_frame(); stack.draw(*app.renderer()); ++readbacks; return read(*app.renderer());
    };
    // Shipped48-font winners end329.21875, authored braces end335; the next
    // score starts340. Actual72-font headings end236.328125 before winner255.
    // The unchanged score/outcome end402/460.6640625, before the plates482.
    const RectangleF winner_region(600.0f, 246.0f, 720.0f, 93.0f);
    const RectangleF row_region(420.0f, 339.0f, 1080.0f, 135.0f);
    const RectangleF heading_region(360.0f, 117.0f, 1200.0f, 129.0f);
    const auto project = [&](RectangleF bounds)
    {
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        camera.calculate_view_rectangle(bounds); return countdown_crop_bounds(bounds);
    };
    RectangleF winner_bounds;
    const auto reference = [&](const WinnerCase& fixture, const Presentation& snapshot, const std::wstring& literal,
        bool details_visible, bool winner_visible, bool wrong_font = false)
    {
        const auto& style = snapshot.menu(); const auto& look = snapshot.results_menu();
        const bool rows_visible = fixture.session.is_series() && details_visible;
        colourwars::UiTextDropShadow heading("independent-winner-heading", fixture.heading, style.heading_font, Vector2F::ZERO,
            app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO);
        heading.set_position({ (menu_consts::DESIGN_RESOLUTION.x - heading.bounds().width) / 2.0f, look.heading_y });
        colourwars::UiTextDropShadow winner("independent-competitive-winner", literal,
            wrong_font ? original.menu().item_font : style.item_font, Vector2F::ZERO,
            app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO, !winner_visible);
        winner.set_position({ (menu_consts::DESIGN_RESOLUTION.x - winner.bounds().width) / 2.0f, look.winner_y });
        colourwars::UiTextDropShadow score("independent-winner-score", fixture.score, style.detail_font, Vector2F::ZERO,
            app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO, !rows_visible);
        score.set_position({ (menu_consts::DESIGN_RESOLUTION.x - score.bounds().width) / 2.0f, look.session_score_y });
        colourwars::UiTextDropShadow outcome("independent-winner-outcome", fixture.outcome, style.detail_font, Vector2F::ZERO,
            app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO, !rows_visible);
        outcome.set_position({ (menu_consts::DESIGN_RESOLUTION.x - outcome.bounds().width) / 2.0f, look.session_result_y });
        const RectangleF panel((menu_consts::DESIGN_RESOLUTION - look.box_size) / 2.0f, look.box_size);
        winner_bounds = winner.bounds();
        require(winner_bounds.width > 0.0f && winner_bounds.height > 0.0f && panel.contains(winner_bounds) &&
            winner_region.contains(winner_bounds) && heading_region.contains(heading.bounds()) &&
            row_region.contains(score.bounds()) && row_region.contains(outcome.bounds()) &&
            score.bounds().bottom() < outcome.bounds().y,
            "Independent winner/unchanged neighbor bounds escaped their measured unscaled zero-offset comparison regions");
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
        list.set_viewport(viewport); list.set_camera(camera); list.set_filter(TextureFilter::point);
        canvas.draw(list, camera.visible_rectangle(viewport)); FocusGroup empty_focus;
        const MenuPaperArtwork paper(panel, app.render_resources(), &style, &look.art, &empty_focus);
        paper.draw(list); list.set_filter(TextureFilter::linear);
        heading.draw(list); winner.draw(list); score.draw(list); outcome.draw(list);
        require(readbacks < 99, "Competitive winner reference exceeded its fixed readback budget");
        ++readbacks; return read(renderer);
    };
    std::ofstream records(output / "competitive-winner.csv");
    records << "case,session,width,height,players,best_of,target,complete,round,a_wins,b_wins,revealed,details_visible,"
        "proceed_visible,winner_visible,hidden_policy_control,expected_value,font,raw_width,raw_height,x,y,scale,shadow_x,shadow_y,"
        "score_literal,outcome_literal,heading_literal,winner_match,row_neighbor_match,heading_match,frozen_session_setup_save_rng\n";
    const auto compare = [&](const char* name, const WinnerCase& fixture, const Presentation& snapshot,
        const std::wstring& literal, bool revealed, bool details_visible, bool proceed_visible, bool winner_visible,
        bool hidden_policy_control = false, const char* crop = "", bool negative_font = false)
    {
        unchanged(); std::srand(7319); const int next_random = std::rand(); std::srand(7319);
        const Frame actual = actual_frame();
        require(actual.pixels == actual_frame().pixels && std::rand() == next_random,
            "Frozen competitive winner drawing changed pixels or consumed gameplay RNG");
        const Frame wanted = reference(fixture, snapshot, literal, details_visible, winner_visible);
        const RectangleF measured = winner_bounds; const RectangleI area = project(winner_region);
        const Frame observed = foreground_crop(actual, area, 0), expected = foreground_crop(wanted, area, 0);
        if (observed.pixels != expected.pixels)
        {
            const RectangleI delta = foreground_delta_bounds(observed, expected);
            std::cerr << "Competitive winner case=" << name << " session=" << fixture.name << " area=" << area.x << ',' <<
                area.y << ',' << area.width << ',' << area.height << " local_delta=" << delta.x << ',' <<
                delta.y << ',' << delta.width << ',' << delta.height << '\n';
            throw std::runtime_error("Actual competitive winner differs from its independent literal unscaled zero-offset reference");
        }
        const RectangleI neighbor_area = project(row_region), title_area = project(heading_region);
        require(foreground_crop(actual, neighbor_area, 0).pixels == foreground_crop(wanted, neighbor_area, 0).pixels,
            "Competitive winner extraction changed the unmodified score/outcome or single-match row absence");
        require(foreground_crop(actual, title_area, 0).pixels == foreground_crop(wanted, title_area, 0).pixels,
            "Competitive winner extraction changed the unmodified Round or single-match heading");
        if (negative_font)
        {
            const Frame wrong = reference(fixture, snapshot, literal, details_visible, winner_visible, true);
            RectangleF tight = measured; tight.inflate(2.0f); const RectangleI tight_area = project(tight);
            require(winner_visible && foreground_crop(actual, tight_area, 0).pixels != foreground_crop(wrong, tight_area, 0).pixels,
                "Competitive winner brace font control cannot distinguish the reopened item font"); ++font_controls;
        }
        if (*crop != '\0')
        {
            require((actual.width == 1280 && actual.height == 720 && observed.width == 480 && observed.height == 62) ||
                (actual.width == 1920 && actual.height == 1080 && observed.width == 720 && observed.height == 93),
                "Competitive winner compact crops drifted from their fixed native pixel dimensions");
            save(observed, output / crop); ++crops;
        }
        records << name << ',' << fixture.name << ',' << actual.width << ',' << actual.height << ',' << fixture.players << ',' <<
            fixture.best_of << ',' << fixture.target << ',' << (fixture.session.complete() ? "true" : "false") << ',' <<
            fixture.session.round() << ',' << fixture.session.team_a_wins() << ',' << fixture.session.team_b_wins() << ',' <<
            (revealed ? "true" : "false") << ',' << (details_visible ? "true" : "false") << ',' <<
            (proceed_visible ? "true" : "false") << ',' << (winner_visible ? "true" : "false") << ',' <<
            (hidden_policy_control ? "true" : "false") << ',';
        const auto ascii = [&](const std::wstring& words)
            { for (const wchar_t character : words) { records << static_cast<char>(character); } };
        ascii(literal); records << ',' << snapshot.menu().item_font << ',' << measured.width << ',' << measured.height << ',' <<
            measured.x << ',' << measured.y << ",1,0,0,";
        ascii(fixture.score); records << ','; ascii(fixture.outcome); records << ','; ascii(fixture.heading);
        records << ",true,true,true,true\n"; ++rows; unchanged(); return actual;
    };
    const auto& look = original.results_menu();
    const auto finish = [&](bool revealed)
    {
        if (!revealed) { tick(look.team_fill_delay + 0.01f); tick(look.team_fill_time + 0.01f); tick(); }
        tick(look.show_results_delay + 0.01f); tick();
    };
    const std::array<const wchar_t*, 3> old_words{ L"A won~ {A}", L"B won~ {B}", L"Draw~ {D}" };
    const std::array<const wchar_t*, 3> next_words{ L"A wins {A}", L"B wins {B}", L"Even {D}" };
    Presentation old = original, next = original;
    auto& old_text = const_cast<MenuText&>(old.menu_text()).results;
    old_text.competitive_team_a_wins = old_words[0]; old_text.competitive_team_b_wins = old_words[1];
    old_text.competitive_draw = old_words[2];
    auto& next_text = const_cast<MenuText&>(next.menu_text()).results;
    next_text.competitive_team_a_wins = next_words[0]; next_text.competitive_team_b_wins = next_words[1];
    next_text.competitive_draw = next_words[2];
    const Font* source = app.render_resources()->font(app.render_resources()->resolve_sprite_font(original.menu().item_font));
    std::vector<Glyph> glyphs;
    for (char32_t character = U' '; character < U'~'; ++character)
    {
        const Glyph* source_glyph = source->find(character);
        require(source_glyph != nullptr, "Competitive winner font fixture lacks an original printable glyph");
        Glyph glyph = *source_glyph;
        if (character == U'{' || character == U'}') { glyph.x_offset += 1.25f; glyph.x_advance += 0.75f; }
        glyphs.push_back(glyph);
    }
    const std::string next_font = "competitive_winner_braces";
    app.render_resources()->add_font(next_font,
        std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
    require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(next_font), L"~"),
        "Competitive winner replacement font still renders retained old copy's tilde");
    const_cast<MenuStyle&>(next.menu()).item_font = next_font;
    old.validate_resources(*app.render_resources(), *app.audio_resources());
    next.validate_resources(*app.render_resources(), *app.audio_resources());
    const std::array<const char*, 4> default_crops{ "competitive-winner-a.ppm", "competitive-winner-b.ppm",
        "competitive-winner-draw.ppm", "competitive-winner-solo.ppm" };
    for (size_t index = 0; index < cases.size(); ++index)
    {
        const auto& fixture = cases[index]; const bool revealed = index == 2, solo_control = index == 3;
        const Presentation& snapshot = solo_control ? next : original;
        presentation = snapshot; open(fixture, revealed);
        if (index == 0)
        {
            tap(Key::enter); require(stack.depth() == 2, "Premature Enter bypassed the competitive winner fill delay");
            compare("legacy-initial", fixture, original, fixture.winner, false, false, false, false);
            tick(look.team_fill_delay + 0.01f); tick(look.team_fill_time + 0.01f);
            compare("legacy-fill-crossed", fixture, original, fixture.winner, false, false, false, false);
            tick(); compare("legacy-details", fixture, original, fixture.winner, false, true, false, true);
            tap(Key::enter); require(stack.depth() == 2, "Premature Enter bypassed the competitive winner post-reveal delay");
            tick(look.show_results_delay + 0.01f);
            compare("legacy-prompt-crossed", fixture, original, fixture.winner, false, true, false, true);
            tick();
        }
        else if (revealed)
        {
            compare("revealed-initial", fixture, original, fixture.winner, true, true, false, true);
            tap(Key::enter); require(stack.depth() == 2, "Revealed draw skipped its remaining prompt delay"); finish(true);
        }
        else { finish(false); }
        compare(solo_control ? "solo-control" : "legacy-ready", fixture, snapshot, fixture.winner,
            revealed, true, true, true, false, default_crops[index]);
        if (index == 0) { tap(Key::escape); require(stack.depth() == 2, "Escape bypassed competitive winner continuation"); }
        proceed();
    }
    for (size_t index = 0; index < old_words.size(); ++index)
    {
        const auto& fixture = cases[index]; const bool revealed = index == 2;
        presentation = old; open(fixture, revealed); finish(revealed);
        const Frame old_frame = compare("old-primary", fixture, old, old_words[index], revealed, true, true, true,
            false, index == 0 ? "competitive-winner-old-720.ppm" : "");
        presentation = next;
        require(compare("retained-old", fixture, old, old_words[index], revealed, true, true, true).pixels == old_frame.pixels,
            "Source assignment rebased a constructed competitive winner's text or item font");
        if (index == 0)
        {
            resize({ 1920, 1080 });
            compare("old-1080", fixture, old, old_words[index], revealed, true, true, true, false, "competitive-winner-old-1080.ppm");
            resize({ 1280, 720 });
            require(compare("old-return720", fixture, old, old_words[index], revealed, true, true, true).pixels == old_frame.pixels,
                "Old competitive winner resize changed its captured copy or item font");
        }
        proceed(); open(fixture, revealed); finish(revealed);
        const Frame next_frame = compare("reopened-new", fixture, next, next_words[index], revealed, true, true, true,
            false, index == 0 ? "competitive-winner-new-720.ppm" : "", true);
        presentation = original;
        require(compare("retained-new", fixture, next, next_words[index], revealed, true, true, true).pixels == next_frame.pixels,
            "Source reversal rebased a reopened competitive winner's text or item font");
        if (index == 0)
        {
            resize({ 1920, 1080 });
            compare("new-1080", fixture, next, next_words[index], revealed, true, true, true, false, "competitive-winner-new-1080.ppm");
            resize({ 1280, 720 });
            require(compare("new-return720", fixture, next, next_words[index], revealed, true, true, true).pixels == next_frame.pixels,
                "New competitive winner resize changed its captured copy or item font");
        }
        proceed();
    }
    presentation = original; open(cases[0], false); finish(false);
    compare("restored-default", cases[0], original, cases[0].winner, false, true, true, true); proceed();
    // Preserve the existing post-reveal policy: A/B and nonzero solo coverage
    // stay hidden when constructed already revealed; draw above stays visible.
    for (const size_t index : { size_t(0), size_t(1), size_t(3) })
    {
        const auto& fixture = cases[index]; const std::wstring literal = index == 3 ? fixture.winner : next_words[index];
        presentation = next; open(fixture, true);
        compare("already-revealed-initial", fixture, next, literal, true, true, false, false, true);
        finish(true); compare("already-revealed-ready", fixture, next, literal, true, true, true, false, true); proceed();
    }
    require(updates == 102 && readbacks == 99 && rows == 32 && crops == 8 && openings == 14 && pops == 14 &&
        font_controls == 3 && records.good(), "Competitive winner drifted from its fixed update/readback/CSV/crop/action budget");
    std::ofstream summary(output / "competitive-winner-summary.json");
    summary << "{\"outcome\":\"passed\",\"scope\":\"competitive-winner-only\",\"updates\":102,\"readbacks\":99,"
        "\"csv_rows\":32,\"compact_crops\":8,\"page_openings\":14,\"proceed_pops\":14,\"font_negative_controls\":3,"
        "\"resize_updates\":0,\"save_calls\":1,\"crop_pixels\":312480}\n";
    require(summary.good(), "Cannot retain competitive winner summary");
    std::cout << "PASS Competitive winner leaf: actual A/B/draw Results with independent literal winner rows; "
        "unchanged solo coverage under the new item font/copy, Round/score/outcome neighbors and single series-row absence; "
        "centered unscaled zero-offset text, captured copy/item font through assignment/reopen/reversal, three brace font controls; "
        "normal reveal/proceed gates and already-revealed A/B/solo hidden versus draw visible, 14 fresh pops, "
        "frozen ledger/setup/private Save/RNG, zero-update 720/1080/720 resize, "
        "102 updates, 99 readbacks, 32 CSV rows and eight compact crops (312480 pixels / 937552 PPM bytes), "
        "one private seed Save. Both presets use the normal reader; "
        "no match/global-input/physical-pad/listening/human/performance acceptance.\n";
}
