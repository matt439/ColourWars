// Actual all-draw Sessions reach rounds 1/10/100 without a Level. Heading,
// unchanged score and outcome expectations are independent literal strings.
void round_heading_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    const Presentation original = presentation;
    const MatchTuning original_match = tuning.match();
    LevelEndInfo win_a; win_a.total_area = 100.0f; win_a.team_a_area = 60.0f; win_a.team_b_area = 40.0f;
    win_a.team_colours = TeamColourTools::team_colours(TeamColourPair::blue_orange, original.team_palette());
    win_a.player_areas[paint_owners::index(input_slots::keyboard_and_mouse, 0)] = 60.0f;
    win_a.player_areas[paint_owners::index(-1, 1)] = 40.0f;
    LevelEndInfo draw = win_a; draw.team_a_area = draw.team_b_area = 50.0f;
    draw.player_areas[paint_owners::index(input_slots::keyboard_and_mouse, 0)] = 50.0f;
    draw.player_areas[paint_owners::index(-1, 1)] = 50.0f;
    const auto drawn_session = [&](int rounds)
    {
        Session session(3);
        for (int round = 1; round <= rounds; ++round)
        {
            session.finish_round(draw);
            if (round < rounds) { session.next_round(); }
        }
        return session;
    };
    // Fixture setup is 111 draw finishes and 108 next_round calls, not state
    // updates. These unbounded drawn series award no win and remain unfinished.
    const Session round1 = drawn_session(1), round10 = drawn_session(10), round100 = drawn_session(100);
    Session single; single.finish_round(win_a);
    struct RoundCase
    {
        Session session; LevelEndInfo info; const char* name; int best_of; int target;
        const wchar_t* heading; const wchar_t* score; const wchar_t* outcome;
    };
    const std::array<RoundCase, 4> cases{ {
        { round1, draw, "series-round1", 3, 2, L"Round 1", L"Wins: A 0 - B 0  |  First to 2", L"Draw - no win awarded" },
        { round10, draw, "series-round10", 3, 2, L"Round 10", L"Wins: A 0 - B 0  |  First to 2", L"Draw - no win awarded" },
        { round100, draw, "series-round100", 3, 2, L"Round 100", L"Wins: A 0 - B 0  |  First to 2", L"Draw - no win awarded" },
        { single, win_a, "single", 1, 1, L"Results", L"Wins: A 1 - B 0  |  First to 1", L"Team A wins the series!" } } };
    const auto ledger = [](const Session& session)
    {
        return std::vector<int>{ session.is_series() ? 1 : 0, session.complete() ? 1 : 0,
            session.round(), session.team_a_wins(), session.team_b_wins() };
    };
    std::array<std::vector<int>, 4> ledgers;
    for (size_t index = 0; index < cases.size(); ++index) { ledgers[index] = ledger(cases[index].session); }
    const NestedSaveProfile profile(output / "round-heading-profile");
    Save preferences; PlaySettings initial; initial.music_volume = initial.sfx_volume = 0;
    initial.aim_sensitivity = 4; initial.view_shake = false; initial.reduce_effects = true;
    require(preferences.set_play_settings_and_save(initial), "Cannot seed Round heading's private preferences");
    const auto saved_bytes = [&]
    {
        std::ifstream file(output / "round-heading-profile/ColourWars/save_data.json", std::ios::binary);
        require(file.good(), "Round heading probe lost its private save");
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
    struct RestoreRoundHeading
    {
        StateContext& stack; MenuContext& context; Presentation& presentation; Presentation original;
        ~RestoreRoundHeading()
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
            require(ledger(fixture.session) == ledgers[index] &&
                fixture.session.score_string(original.menu_text().results) == fixture.score &&
                fixture.session.result_string(original.menu_text().results) == fixture.outcome &&
                fixture.info.total_area == 100.0f && fixture.info.team_a_played && fixture.info.team_b_played &&
                fixture.info.player_areas[paint_owners::index(input_slots::keyboard_and_mouse, 0)] == fixture.info.team_a_area &&
                fixture.info.player_areas[paint_owners::index(-1, 1)] == fixture.info.team_b_area &&
                fixture.info.team_a_area + fixture.info.team_b_area == 100.0f,
                "Round heading changed a frozen branch, ledger, score or input-owner credit");
        }
        const auto players = settings.player_settings(); const auto& play = preferences.play_settings();
        require(settings.player_count() == 2 && settings.best_of() == 3 && settings.duration() == 73.0f &&
            settings.stage() == "movement_course" && settings.screen_layout() == ScreenLayout::two_player &&
            players.size() == 2 && players[0].num == input_slots::keyboard_and_mouse && players[1].num == -1 &&
            settings.player_team(0) == PlayerTeam::a && settings.player_team(1) == PlayerTeam::b &&
            settings.player_weapon(0) == WeaponType::sprayer && settings.player_weapon(1) == WeaponType::roller &&
            !context.music_exit_pending(), "Round heading changed setup, input claims or pending music state");
        require(play.music_volume == 0 && play.sfx_volume == 0 && play.aim_sensitivity == 4 && !play.view_shake &&
            play.aim_marker && play.reduce_effects && preferences.resolution() == ScreenResolution::s_1920_1080 &&
            !preferences.fullscreen() && preferences.team_palette() == TeamColourPair::random &&
            preferences.records().stages().empty() && !preferences.last_match_setup() && saved_bytes() == saved &&
            tuning.match().duration == original_match.duration && tuning.match().duration_choices == original_match.duration_choices,
            "Round heading changed active preferences, records, private bytes or numeric tuning");
        for (const auto& slot : play.sticks)
            { require(slot.movement_deadzone == -1 && slot.aim_deadzone == -1, "Round heading changed a pad profile"); }
    };
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    const auto resize = [&](Vector2I size)
    {
        const size_t before = updates; app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        require(updates == before, "Round heading resize updated the state stack");
    };
    resize({ 1280, 720 });
    const auto tick = [&](float elapsed = 0.0f)
    {
        require(updates < 66, "Round heading exceeded its fixed update budget");
        ++updates; app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed);
    };
    const auto tap = [&](Key key)
        { app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick(); unchanged(); };
    const auto open = [&](const RoundCase& fixture, bool revealed)
    {
        stack.clear(); stack.notify_activation(true); context.cancel_music_exit();
        app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::up, Key::down, Key::left, Key::right,
            Key::enter, Key::escape, Key::space, Key::backspace }) { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        stack.push(std::make_unique<ResultsMenuProbeRoot>(app, canvas));
        stack.push(std::make_unique<ResultsMenuInitial>(&context, fixture.info, fixture.session, revealed));
        ++openings; tick();
        require(stack.depth() == 2, "Round heading consumed entry input or lost its actual page"); unchanged();
    };
    const auto proceed = [&]
    {
        require(stack.depth() == 2, "Round heading page closed before fresh continuation input");
        tap(Key::enter); require(stack.depth() == 1, "Fresh Enter failed to pop the actual Round heading page");
        ++pops; unchanged();
    };
    const auto actual_frame = [&]
    {
        require(readbacks < 81, "Round heading exceeded its fixed readback budget");
        app.renderer()->begin_frame(); stack.draw(*app.renderer()); ++readbacks; return read(*app.renderer());
    };
    // Actual72-font headings, prefix/suffix templates and Results all end at
    //236.328125; winner begins255. Unchanged score/outcome end402/460.6640625.
    // Multiples of three keep exact floor/ceil crops at both native sizes.
    const RectangleF row_region(420.0f, 333.0f, 1080.0f, 141.0f);
    const RectangleF heading_region(360.0f, 117.0f, 1200.0f, 129.0f);
    const auto project = [&](RectangleF bounds)
    {
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        camera.calculate_view_rectangle(bounds); return countdown_crop_bounds(bounds);
    };
    RectangleF heading_bounds;
    const auto reference = [&](const RoundCase& fixture, const Presentation& snapshot, const std::wstring& literal,
        bool details_visible, bool wrong_font = false)
    {
        const auto& style = snapshot.menu(); const auto& look = snapshot.results_menu();
        const bool rows_visible = fixture.session.is_series() && details_visible;
        colourwars::UiTextDropShadow heading("independent-round-heading", literal,
            wrong_font ? original.menu().heading_font : style.heading_font, Vector2F::ZERO,
            app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO);
        heading.set_position({ (menu_consts::DESIGN_RESOLUTION.x - heading.bounds().width) / 2.0f, look.heading_y });
        colourwars::UiTextDropShadow score("independent-session-score", fixture.score, style.detail_font, Vector2F::ZERO,
            app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO, !rows_visible);
        score.set_position({ (menu_consts::DESIGN_RESOLUTION.x - score.bounds().width) / 2.0f, look.session_score_y });
        colourwars::UiTextDropShadow outcome("independent-session-outcome", fixture.outcome, style.detail_font, Vector2F::ZERO,
            app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO, !rows_visible);
        outcome.set_position({ (menu_consts::DESIGN_RESOLUTION.x - outcome.bounds().width) / 2.0f, look.session_result_y });
        const RectangleF panel((menu_consts::DESIGN_RESOLUTION - look.box_size) / 2.0f, look.box_size);
        heading_bounds = heading.bounds();
        require(heading_bounds.width > 0.0f && heading_bounds.height > 0.0f && panel.contains(heading_bounds) &&
            row_region.contains(score.bounds()) && row_region.contains(outcome.bounds()) &&
            heading_region.contains(heading_bounds) && score.bounds().bottom() < outcome.bounds().y,
            "Independent round heading/unchanged neighbor bounds escaped their measured unscaled zero-offset comparison regions");
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
        list.set_viewport(viewport); list.set_camera(camera); list.set_filter(TextureFilter::point);
        canvas.draw(list, camera.visible_rectangle(viewport)); FocusGroup empty_focus;
        const MenuPaperArtwork paper(panel, app.render_resources(), &style, &look.art, &empty_focus);
        paper.draw(list); list.set_filter(TextureFilter::linear);
        heading.draw(list); score.draw(list); outcome.draw(list);
        require(readbacks < 81, "Round heading reference exceeded its fixed readback budget");
        ++readbacks; return read(renderer);
    };
    std::ofstream records(output / "round-heading.csv");
    records << "case,session,width,height,best_of,target,complete,round,a_wins,b_wins,revealed,details_visible,proceed_visible,"
        "expected_value,font,raw_width,raw_height,x,y,scale,shadow_x,shadow_y,visible,score_literal,outcome_literal,"
        "row_neighbor_match,heading_match,frozen_session_setup_save_rng\n";
    const auto compare = [&](const char* name, const RoundCase& fixture, const Presentation& snapshot,
        const std::wstring& literal, bool revealed, bool details_visible, bool proceed_visible,
        const char* crop = "", bool negative_font = false)
    {
        unchanged(); std::srand(6637); const int next_random = std::rand(); std::srand(6637);
        const Frame actual = actual_frame();
        require(actual.pixels == actual_frame().pixels && std::rand() == next_random,
            "Frozen Round heading drawing changed pixels or consumed gameplay RNG");
        const Frame wanted = reference(fixture, snapshot, literal, details_visible);
        const RectangleF measured = heading_bounds; const RectangleI area = project(heading_region);
        const Frame observed = foreground_crop(actual, area, 0), expected = foreground_crop(wanted, area, 0);
        if (observed.pixels != expected.pixels)
        {
            const RectangleI delta = foreground_delta_bounds(observed, expected);
            std::cerr << "Round heading case=" << name << " session=" << fixture.name << " area=" << area.x << ',' <<
                area.y << ',' << area.width << ',' << area.height << " local_delta=" << delta.x << ',' <<
                delta.y << ',' << delta.width << ',' << delta.height << '\n';
            throw std::runtime_error("Actual round heading differs from its independent literal unscaled zero-offset reference");
        }
        const RectangleI neighbor_area = project(row_region);
        require(foreground_crop(actual, neighbor_area, 0).pixels == foreground_crop(wanted, neighbor_area, 0).pixels,
            "Round heading extraction changed the unmodified score/outcome or the single-match row absence");
        if (negative_font)
        {
            const Frame wrong = reference(fixture, snapshot, literal, details_visible, true);
            RectangleF tight = measured; tight.inflate(2.0f); const RectangleI tight_area = project(tight);
            require(foreground_crop(actual, tight_area, 0).pixels != foreground_crop(wrong, tight_area, 0).pixels,
                "Round heading caret font control cannot distinguish the reopened heading font"); ++font_controls;
        }
        if (*crop != '\0')
        {
            require((actual.width == 1280 && actual.height == 720 && observed.width == 800 && observed.height == 86) ||
                (actual.width == 1920 && actual.height == 1080 && observed.width == 1200 && observed.height == 129),
                "Round heading compact crops drifted from their fixed native pixel dimensions");
            save(observed, output / crop); ++crops;
        }
        records << name << ',' << fixture.name << ',' << actual.width << ',' << actual.height << ',' << fixture.best_of << ',' << fixture.target << ',' <<
            (fixture.session.complete() ? "true" : "false") << ',' << fixture.session.round() << ',' <<
            fixture.session.team_a_wins() << ',' << fixture.session.team_b_wins() << ',' << (revealed ? "true" : "false") << ',' <<
            (details_visible ? "true" : "false") << ',' << (proceed_visible ? "true" : "false") << ',';
        const auto ascii = [&](const std::wstring& words)
            { for (const wchar_t character : words) { records << static_cast<char>(character); } };
        ascii(literal); records << ',' << snapshot.menu().heading_font << ',' << measured.width << ',' << measured.height << ',' <<
            measured.x << ',' << measured.y << ",1,0,0,true,";
        ascii(fixture.score); records << ','; ascii(fixture.outcome); records << ",true,true,true\n";
        ++rows; unchanged(); return actual;
    };
    const auto& look = original.results_menu();
    const std::array<const wchar_t*, 3> old_words{ L"Old~ 1", L"Old~ 10", L"Old~ 100" };
    const std::array<const wchar_t*, 3> next_words{ L"1 ^ Round", L"10 ^ Round", L"100 ^ Round" };
    Presentation old = original, next = original;
    const_cast<MenuText&>(old.menu_text()).results.round_heading = L"Old~ {round}";
    const_cast<MenuText&>(next.menu_text()).results.round_heading = L"{round} ^ Round";
    const Font* source = app.render_resources()->font(app.render_resources()->resolve_sprite_font(original.menu().heading_font));
    std::vector<Glyph> glyphs;
    for (char32_t character = U' '; character < U'~'; ++character)
    {
        const Glyph* source_glyph = source->find(character);
        require(source_glyph != nullptr, "Round heading font fixture lacks an original printable glyph");
        Glyph glyph = *source_glyph;
        if (character == U'^') { glyph.x_offset += 1.25f; glyph.x_advance += 0.75f; }
        glyphs.push_back(glyph);
    }
    const std::string next_font = "round_heading_caret";
    app.render_resources()->add_font(next_font,
        std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
    require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(next_font), L"~"),
        "Round heading replacement font still renders retained old copy's tilde");
    const_cast<MenuStyle&>(next.menu()).heading_font = next_font;
    old.validate_resources(*app.render_resources(), *app.audio_resources());
    next.validate_resources(*app.render_resources(), *app.audio_resources());
    const std::array<const char*, 4> default_crops{ "round-heading-round1.ppm", "round-heading-round10.ppm",
        "round-heading-round100.ppm", "round-heading-single.ppm" };
    for (size_t index = 0; index < cases.size(); ++index)
    {
        const auto& fixture = cases[index]; const bool revealed = index != 0, single_control = index == 3;
        const Presentation& snapshot = single_control ? next : original;
        presentation = snapshot; open(fixture, revealed);
        if (index == 0)
        {
            tap(Key::enter); require(stack.depth() == 2, "Premature Enter bypassed the Round heading fill delay");
            compare("legacy-initial", fixture, original, fixture.heading, false, false, false);
            tick(look.team_fill_delay + 0.01f); tick(look.team_fill_time + 0.01f);
            compare("legacy-fill-crossed", fixture, original, fixture.heading, false, false, false);
            tick(); compare("legacy-details", fixture, original, fixture.heading, false, true, false);
            tap(Key::enter); require(stack.depth() == 2, "Premature Enter bypassed the Round heading post-reveal delay");
            tick(look.show_results_delay + 0.01f);
            compare("legacy-prompt-crossed", fixture, original, fixture.heading, false, true, false);
            tick();
        }
        else
        {
            if (index == 1)
            {
                compare("revealed-initial", fixture, original, fixture.heading, true, true, false);
                tap(Key::enter); require(stack.depth() == 2, "Revealed Round heading skipped its remaining prompt delay");
            }
            tick(look.show_results_delay + 0.01f); tick();
        }
        compare(single_control ? "single-control" : "legacy-ready", fixture, snapshot, fixture.heading,
            revealed, true, true, default_crops[index]);
        if (index == 0) { tap(Key::escape); require(stack.depth() == 2, "Escape bypassed Round heading continuation"); }
        proceed();
    }
    for (size_t index = 0; index < old_words.size(); ++index)
    {
        const auto& fixture = cases[index];
        presentation = old; open(fixture, true); tick(look.show_results_delay + 0.01f); tick();
        const Frame old_frame = compare("old-primary", fixture, old, old_words[index], true, true, true,
            index == 0 ? "round-heading-old-720.ppm" : "");
        presentation = next;
        require(compare("retained-old", fixture, old, old_words[index], true, true, true).pixels == old_frame.pixels,
            "Source assignment rebased a constructed Round heading's text or font");
        if (index == 0)
        {
            resize({ 1920, 1080 });
            compare("old-1080", fixture, old, old_words[index], true, true, true, "round-heading-old-1080.ppm");
            resize({ 1280, 720 });
            require(compare("old-return720", fixture, old, old_words[index], true, true, true).pixels == old_frame.pixels,
                "Old round heading resize changed its captured template or font");
        }
        proceed(); open(fixture, true); tick(look.show_results_delay + 0.01f); tick();
        const Frame next_frame = compare("reopened-new", fixture, next, next_words[index], true, true, true,
            index == 0 ? "round-heading-new-720.ppm" : "", true);
        presentation = original;
        require(compare("retained-new", fixture, next, next_words[index], true, true, true).pixels == next_frame.pixels,
            "Source reversal rebased a reopened Round heading's text or heading font");
        if (index == 0)
        {
            resize({ 1920, 1080 });
            compare("new-1080", fixture, next, next_words[index], true, true, true, "round-heading-new-1080.ppm");
            resize({ 1280, 720 });
            require(compare("new-return720", fixture, next, next_words[index], true, true, true).pixels == next_frame.pixels,
                "New round heading resize changed its captured template or font");
        }
        proceed();
    }
    presentation = original; open(cases[0], true); tick(look.show_results_delay + 0.01f); tick();
    compare("restored-default", cases[0], original, cases[0].heading, true, true, true); proceed();
    require(updates == 66 && readbacks == 81 && rows == 26 && crops == 8 && openings == 11 && pops == 11 &&
        font_controls == 3 && records.good(), "Round heading drifted from its fixed update/readback/CSV/crop/action budget");
    std::ofstream summary(output / "round-heading-summary.json");
    summary << "{\"outcome\":\"passed\",\"scope\":\"round-heading-only\",\"updates\":66,\"readbacks\":81,"
        "\"csv_rows\":26,\"compact_crops\":8,\"page_openings\":11,\"proceed_pops\":11,\"font_negative_controls\":3,"
        "\"resize_updates\":0,\"save_calls\":1,\"crop_pixels\":722400}\n";
    require(summary.good(), "Cannot retain Round heading summary");
    std::cout << "PASS Round heading leaf: actual drawn Session rounds 1/10/100 with independent literal headings; "
        "single Results under the new template/font with absent series rows, unchanged score/outcome neighbors, "
        "centered unscaled zero-offset text and captured templates/heading font through assignment/reopen/reversal; "
        "actual reveal/proceed gates and 11 fresh pops, frozen ledger/setup/private Save/RNG, zero-update 720/1080/720 resize, "
        "66 updates, 81 readbacks, 26 CSV rows, three font controls and eight compact crops "
        "(722400 pixels / 2167316 PPM bytes), one private seed Save. "
        "Both presets use the normal reader; no match/global-input/physical-pad/listening/human/performance acceptance.\n";
}
