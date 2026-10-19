// Actual normal-reader End pages. All continuation expectations are plain
// literals, including braces; neither Session's selector nor a formatter builds
// the reference. The local inert root only receives typed menu actions.
void end_continuation_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    const Presentation original = presentation;
    const MatchTuning original_match = tuning.match();
    LevelEndInfo win_a; win_a.total_area = 100.0f; win_a.team_a_area = 60.0f; win_a.team_b_area = 40.0f;
    LevelEndInfo win_b = win_a; win_b.team_a_area = 40.0f; win_b.team_b_area = 60.0f;
    LevelEndInfo draw = win_a; draw.team_a_area = draw.team_b_area = 50.0f;
    LevelEndInfo solo = win_a; solo.team_b_played = false; solo.team_a_area = 72.0f; solo.team_b_area = 0.0f;
    Session single_solo; single_solo.finish_round(solo);
    Session single_teams; single_teams.finish_round(win_b);
    Session complete_a(3); complete_a.finish_round(win_a); complete_a.next_round(); complete_a.finish_round(win_a);
    Session complete_b(5); complete_b.finish_round(win_b); complete_b.next_round(); complete_b.finish_round(win_b);
    complete_b.next_round(); complete_b.finish_round(win_b);
    Session replay(3); replay.finish_round(draw);
    Session next_round(5); next_round.finish_round(win_a);
    struct ContinuationCase { Session session; const char* name; const wchar_t* literal; };
    const std::array<ContinuationCase, 6> cases{ {
        { single_solo, "single-solo", L"Rematch" }, { single_teams, "single-two-team", L"Rematch" },
        { complete_a, "series3-complete-a", L"Play Series Again" },
        { complete_b, "series5-complete-b", L"Play Series Again" },
        { replay, "series3-draw", L"Replay Draw" }, { next_round, "series5-next-a", L"Next Round" } } };
    const auto session_numbers = [](const Session& session)
    {
        return std::vector<int>{ session.is_series() ? 1 : 0, session.complete() ? 1 : 0,
            session.round(), session.team_a_wins(), session.team_b_wins() };
    };
    std::array<std::vector<int>, 6> ledgers;
    std::array<std::wstring, 6> scores, results_text;
    for (size_t index = 0; index < cases.size(); ++index)
    {
        ledgers[index] = session_numbers(cases[index].session);
        scores[index] = cases[index].session.score_string(); results_text[index] = cases[index].session.result_string(original.menu_text().results);
    }
    const NestedSaveProfile profile(output / "end-continuation-profile");
    Save preferences; PlaySettings initial; initial.music_volume = initial.sfx_volume = 0;
    initial.aim_sensitivity = 4; initial.view_shake = false; initial.reduce_effects = true;
    require(preferences.set_play_settings_and_save(initial), "Cannot seed End continuation's private Play preferences");
    const auto saved_bytes = [&]
    {
        std::ifstream file(output / "end-continuation-profile/ColourWars/save_data.json", std::ios::binary);
        require(file.good(), "End continuation probe lost its private save");
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
    struct RestoreContinuation
    {
        StateContext& stack; MenuContext& context; Presentation& presentation; Presentation original;
        ~RestoreContinuation()
        {
            this->context.cancel_music_exit(); this->context.set_music_update({});
            this->stack.clear(); this->presentation = this->original;
        }
    } restore{ stack, context, presentation, original };
    size_t updates = 0, readbacks = 0, rows = 0, crops = 0, openings = 0, font_controls = 0;
    size_t actions = 0, restart_actions = 0, exit_actions = 0, deferred_actions = 0, music_starts = 0;
    std::optional<EndMenuAction> choice; int delivered = 0;
    const auto unchanged = [&]
    {
        for (size_t index = 0; index < cases.size(); ++index)
        {
            require(session_numbers(cases[index].session) == ledgers[index] &&
                cases[index].session.score_string() == scores[index] && cases[index].session.result_string(original.menu_text().results) == results_text[index],
                "End continuation presentation or actions changed the Session ledger");
        }
        const auto players = settings.player_settings(); const auto& play = preferences.play_settings();
        require(settings.player_count() == 2 && settings.best_of() == 3 && settings.duration() == 73.0f &&
            settings.stage() == "movement_course" && settings.screen_layout() == ScreenLayout::two_player &&
            players.size() == 2 && players[0].num == input_slots::keyboard_and_mouse && players[1].num == -1 &&
            settings.player_team(0) == PlayerTeam::a && settings.player_team(1) == PlayerTeam::b &&
            settings.player_weapon(0) == WeaponType::sprayer && settings.player_weapon(1) == WeaponType::roller,
            "End continuation changed the setup plan, roster or input claims");
        require(play.music_volume == 0 && play.sfx_volume == 0 && play.aim_sensitivity == 4 && !play.view_shake &&
            play.aim_marker && play.reduce_effects && preferences.resolution() == ScreenResolution::s_1920_1080 &&
            !preferences.fullscreen() && preferences.team_palette() == TeamColourPair::random &&
            preferences.records().stages().empty() && !preferences.last_match_setup() && saved_bytes() == saved &&
            tuning.match().duration == original_match.duration && tuning.match().duration_choices == original_match.duration_choices,
            "End continuation changed active preferences, records, private bytes or numeric tuning");
        for (const auto& slot : play.sticks)
            { require(slot.movement_deadzone == -1 && slot.aim_deadzone == -1, "End continuation changed a pad profile"); }
    };
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    const auto resize = [&](Vector2I size)
    {
        const size_t before = updates; app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        require(updates == before, "End continuation resizing updated the state stack");
    };
    resize({ 1280, 720 });
    const auto tick = [&]
    {
        require(updates < 68, "End continuation exceeded its fixed update budget");
        ++updates; app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.0f);
    };
    const auto tap = [&](Key key)
        { app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick(); unchanged(); };
    const auto open = [&](const ContinuationCase& fixture)
    {
        context.cancel_music_exit(); context.set_music_update({}); stack.clear();
        app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::up, Key::down, Key::left, Key::right, Key::enter, Key::escape, Key::space })
            { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        choice.reset(); delivered = 0; stack.push(std::make_unique<EndMenuProbeRoot>(app, canvas));
        stack.push<EndMenuAction>(std::make_unique<EndMenuInitial>(&context, fixture.session), [&](const EndMenuAction& result)
        {
            choice = result; ++delivered; ++actions;
            if (result == EndMenuAction::restart) { ++restart_actions; }
            else if (result == EndMenuAction::exit) { ++exit_actions; }
            else { throw std::runtime_error("End continuation delivered an unintended setup action"); }
        });
        ++openings; tick();
        require(stack.depth() == 2 && !choice && delivered == 0 && !context.music_exit_pending(),
            "End continuation consumed entry input or retained an earlier typed action"); unchanged();
    };
    const auto actual_frame = [&]
    {
        require(readbacks < 94, "End continuation exceeded its fixed readback budget");
        app.renderer()->begin_frame(); stack.draw(*app.renderer()); ++readbacks; return read(*app.renderer());
    };
    const RectangleF primary_region(630.0f, 300.0f, 810.0f, 144.0f);
    struct ContinuationMetrics { Vector2F raw; RectangleF primary; RectangleF selected; } metrics;
    const auto reference = [&](const ContinuationCase& fixture, const Presentation& snapshot,
        const std::wstring& literal, int focused, const std::string& font)
    {
        const auto& style = snapshot.menu(); const auto& look = snapshot.end_menu(); const auto& art = look.art;
        const RectangleF panel((menu_consts::DESIGN_RESOLUTION - look.box_size) / 2.0f, look.box_size);
        UiText primary("independent-end-continuation", literal, font, look.layout.position,
            app.render_resources(), focused == 0 ? art.focus_ink : art.ink);
        const bool ongoing = fixture.session.is_series() && !fixture.session.complete();
        UiText secondary("independent-end-secondary", ongoing ? L"Exit to Main Menu" : L"Change Teams", font,
            look.layout.position + Vector2F(0.0f, look.layout.spacing.y), app.render_resources(),
            focused == 1 ? art.focus_ink : art.ink);
        metrics.raw = primary.bounds().size();
        metrics.primary = primary.bounds(); metrics.primary.inflate(art.focus_padding);
        metrics.selected = focused == 0 ? metrics.primary : secondary.bounds();
        if (focused == 1) { metrics.selected.inflate(art.focus_padding); }
        const auto artifact = [&](const RectangleF& bounds)
        {
            const float pen = art.keyline_width;
            return RectangleF::union_of(bounds, RectangleF(bounds.x - pen * 7.0f,
                bounds.center().y - pen * 3.0f, pen * 5.0f, pen * 6.0f));
        };
        // Card bounds already include their shadow. This fixture does not add
        // another shadow offset or impose a new paper/arrow containment rule.
        require(panel.contains(metrics.primary) && metrics.raw.x > 0.0f && metrics.raw.y > 0.0f &&
            metrics.raw.y + art.focus_padding.y * 2.0f <= look.layout.spacing.y &&
            primary_region.contains(artifact(metrics.primary)),
            "Independent unscaled continuation or its full primary artifact escaped the shipped comparison crop");
        metrics.selected = artifact(metrics.selected);
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
        list.set_viewport(viewport); list.set_camera(camera); list.set_filter(TextureFilter::point);
        canvas.draw(list, camera.visible_rectangle(viewport));
        const SpriteSheet* sheet = app.render_resources()->sprite_sheet(style.sprite_sheet);
        const auto pixel = sheet->resolve_sprite_frame(style.fill_frame);
        const auto fill = [&](const RectangleF& bounds, const Colour& colour)
        {
            sheet->draw(list, pixel, bounds,
                Colour(colour.r * colour.a, colour.g * colour.a, colour.b * colour.a, colour.a));
        };
        const auto card = [&](const RectangleF& bounds, const Colour& colour)
        {
            const float pen = art.keyline_width, step = pen * 2.0f, shadow = art.shadow_offset;
            const RectangleF face(bounds.position(), bounds.size() - Vector2F(shadow));
            const auto stepped = [&](const RectangleF& box, const Colour& tint)
            {
                fill(RectangleF(box.x + step, box.y, box.width - step * 2.0f, box.height), tint);
                fill(RectangleF(box.x, box.y + step, box.width, box.height - step * 2.0f), tint);
            };
            stepped(RectangleF(face.position() + Vector2F(shadow), face.size()), art.shade); stepped(face, art.ink);
            const RectangleF inside(face.position() + Vector2F(pen), face.size() - Vector2F(pen * 2.0f));
            stepped(inside, colour);
            for (int pass = 3; pass > 0; --pass)
            {
                Colour shade = art.shade; shade.a *= 0.12f; const float width = pen * static_cast<float>(pass);
                fill(RectangleF(inside.x + step, inside.bottom() - width, inside.width - step * 2.0f, width), shade);
                fill(RectangleF(inside.right() - width, inside.y + step, width, inside.height - step * 2.0f), shade);
            }
        };
        card(panel, art.paper); card(metrics.primary, art.focus_fill);
        RectangleF selected = focused == 0 ? primary.bounds() : secondary.bounds(); selected.inflate(art.focus_padding);
        if (focused == 1) { card(selected, art.focus_fill); }
        const float pen = art.keyline_width; const Vector2F tip(selected.x - pen * 3.0f, selected.center().y);
        fill(RectangleF(tip.x - pen * 4.0f, tip.y - pen, pen * 4.0f, pen * 2.0f), art.ink);
        for (int step = 0; step < 3; ++step)
        {
            const float reach = static_cast<float>(step);
            fill(RectangleF(tip.x - reach * pen, tip.y - (reach + 1.0f) * pen,
                pen, (reach + 1.0f) * pen * 2.0f), art.ink);
        }
        // End's paper is in its text container, so its point sampler remains
        // active for the following plain labels. The whole resting paper and
        // row-one card are redrawn, including their context inside this crop.
        primary.draw(list); secondary.draw(list); ++readbacks; return read(renderer);
    };
    const auto project = [&](RectangleF bounds)
    {
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        camera.calculate_view_rectangle(bounds); return countdown_crop_bounds(bounds);
    };
    std::ofstream records(output / "end-continuation.csv");
    records << "case,session,width,height,focus,series,complete,round,a_wins,b_wins,music_pending,expected_value,font,raw_width,raw_height,scale,primary_persistent,full_ornament_match,frozen_session_setup_save_rng\n";
    const auto compare = [&](const char* name, const ContinuationCase& fixture, const Presentation& snapshot,
        const std::wstring& literal, int focused = 0, const char* crop = "", bool negative_font = false)
    {
        unchanged(); std::srand(3467); const int next_random = std::rand(); std::srand(3467);
        const Frame actual = actual_frame();
        require(actual.pixels == actual_frame().pixels && std::rand() == next_random,
            "End continuation drawing changed frozen pixels or consumed match RNG");
        const Frame wanted = reference(fixture, snapshot, literal, focused, snapshot.menu().item_font);
        const ContinuationMetrics measured = metrics; const RectangleI area = project(primary_region);
        const Frame observed = foreground_crop(actual, area, 0), expected = foreground_crop(wanted, area, 0);
        if (observed.pixels != expected.pixels)
        {
            const RectangleI delta = foreground_delta_bounds(observed, expected);
            std::cerr << "End continuation case=" << name << " session=" << fixture.name <<
                " crop=(" << area.x << ',' << area.y << ',' << area.width << ',' << area.height <<
                ") local_delta=(" << delta.x << ',' << delta.y << ',' << delta.width << ',' << delta.height << ")\n";
            throw std::runtime_error("Actual End continuation text, persistent primary card or full ornament differs from its independent plain fixture");
        }
        if (focused == 1)
        {
            RectangleF bounds = measured.selected; bounds.inflate(2.0f); const RectangleI selected_area = project(bounds);
            require(foreground_crop(actual, selected_area, 0).pixels == foreground_crop(wanted, selected_area, 0).pixels,
                "End secondary focus lost its complete unmodified card or detached arrow");
        }
        if (negative_font)
        {
            const Frame wrong = reference(fixture, snapshot, literal, focused, original.menu().item_font);
            require(observed.pixels != foreground_crop(wrong, area, 0).pixels,
                "End continuation numeric/letter font control failed to distinguish the captured actual font"); ++font_controls;
        }
        if (*crop != '\0')
        {
            require(actual.width == 1280 && actual.height == 720 && observed.width == 540 && observed.height == 96,
                "End continuation compact evidence drifted from its fixed 540x96 payload");
            save(observed, output / crop); ++crops;
        }
        const Session& session = fixture.session;
        records << name << ',' << fixture.name << ',' << actual.width << ',' << actual.height << ',' << focused << ',' <<
            (session.is_series() ? "true" : "false") << ',' << (session.complete() ? "true" : "false") << ',' <<
            session.round() << ',' << session.team_a_wins() << ',' << session.team_b_wins() << ',' <<
            (context.music_exit_pending() ? "true" : "false") << ',';
        for (const wchar_t character : literal) { records << static_cast<char>(character); }
        records << ',' << snapshot.menu().item_font << ',' << measured.raw.x << ',' << measured.raw.y <<
            ",1,true,true,true\n"; ++rows; unchanged(); return actual;
    };
    const auto delivered_once = [&](EndMenuAction expected)
    {
        require(choice == expected && delivered == 1 && stack.depth() == 1 && !context.music_exit_pending(),
            "End continuation lost, changed or duplicated its actual typed action"); unchanged();
    };
    for (size_t index = 0; index < cases.size(); ++index)
    {
        const auto& fixture = cases[index]; open(fixture);
        const char* crop = index == 0 ? "end-continuation-rematch.ppm" : index == 2 ? "end-continuation-series-again.ppm" :
            index == 4 ? "end-continuation-replay-draw.ppm" : index == 5 ? "end-continuation-next-round.ppm" : "";
        compare("legacy-primary", fixture, original, fixture.literal, 0, crop);
        tap(Key::down); compare("legacy-secondary", fixture, original, fixture.literal, 1);
        if (index == 5) { tap(Key::enter); delivered_once(EndMenuAction::exit); }
        else { tap(Key::up); tap(Key::enter); delivered_once(EndMenuAction::restart); }
    }
    const std::array<const wchar_t*, 4> old_words{ L"Rematch~ {1}", L"Again~ {3}", L"Draw~ {0}", L"Next~ {2}" };
    const std::array<const wchar_t*, 4> next_words{ L"Replay {1}", L"Series {3}", L"Draw {0}", L"Round {2}" };
    Presentation old = original, next = original;
    const auto assign = [](MenuEndText& text, const std::array<const wchar_t*, 4>& words)
    {
        text.rematch = words[0]; text.play_series_again = words[1]; text.replay_draw = words[2]; text.next_round = words[3];
    };
    assign(const_cast<MenuText&>(old.menu_text()).end, old_words);
    assign(const_cast<MenuText&>(next.menu_text()).end, next_words);
    const Font* source = app.render_resources()->font(app.render_resources()->resolve_sprite_font(original.menu().item_font));
    std::vector<Glyph> glyphs;
    for (char32_t character = U' '; character < U'~'; ++character)
    {
        const Glyph* source_glyph = source->find(character);
        require(source_glyph != nullptr, "End continuation font fixture lacks an original printable glyph");
        Glyph glyph = *source_glyph;
        if ((character >= U'0' && character <= U'9') || character == U'R' || character == U'S' || character == U'D' || character == U'N')
            { glyph.x_offset += 1.25f; glyph.x_advance += 0.75f; }
        glyphs.push_back(glyph);
    }
    const std::string next_font = "end_continuation_numeric_letters";
    app.render_resources()->add_font(next_font,
        std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
    require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(next_font), L"~"),
        "End continuation replacement font still draws the retained old copy's tilde");
    const_cast<MenuStyle&>(next.menu()).item_font = next_font;
    old.validate_resources(*app.render_resources(), *app.audio_resources());
    next.validate_resources(*app.render_resources(), *app.audio_resources());
    constexpr std::array<size_t, 4> selected_cases{ 1, 2, 4, 5 };
    for (size_t index = 0; index < selected_cases.size(); ++index)
    {
        const auto& fixture = cases[selected_cases[index]];
        presentation = old; open(fixture);
        const Frame retained = compare("old-primary", fixture, old, old_words[index], 0,
            index == 0 ? "end-continuation-old.ppm" : "");
        presentation = next;
        require(compare("retained-old", fixture, old, old_words[index]).pixels == retained.pixels,
            "End continuation assignment rebased a constructed page's plain copy or font");
        int music_updates = 0; context.set_music_update([&](float) { ++music_updates; });
        tap(Key::enter);
        require(context.music_exit_pending() && !choice && delivered == 0 && stack.depth() == 2 && music_updates == 1,
            "End continuation bypassed its owner-controlled music-exit seam");
        ++deferred_actions; music_starts += static_cast<size_t>(music_updates);
        if (index == 0)
        {
            tap(Key::down); tap(Key::enter);
            require(context.music_exit_pending() && !choice && delivered == 0 && music_updates == 1,
                "Pending continuation accepted a second action or restarted its music exit");
            require(compare("pending-music", fixture, old, old_words[index]).pixels == retained.pixels,
                "Pending continuation moved focus or changed its captured page");
        }
        context.complete_music_exit(); delivered_once(EndMenuAction::restart);
        context.complete_music_exit(); require(delivered == 1, "Repeated music completion duplicated an End continuation action");
        context.set_music_update({}); presentation = next; open(fixture);
        const Frame adopted = compare("reopened-new", fixture, next, next_words[index], 0,
            index == 0 ? "end-continuation-new.ppm" : "", index == 0);
        presentation = original;
        require(compare("retained-new", fixture, next, next_words[index]).pixels == adopted.pixels,
            "End continuation reversal rebased its reopened plain copy or actual font");
        if (index == selected_cases.size() - 1)
        {
            const size_t before = updates; resize({ 1920, 1080 });
            compare("resized-1080", fixture, next, next_words[index]); resize({ 1280, 720 });
            require(compare("resized-720", fixture, next, next_words[index]).pixels == adopted.pixels && updates == before,
                "End continuation resize round trip changed its cached plain text/font/card or updated state");
        }
        tap(Key::enter); delivered_once(EndMenuAction::restart);
    }
    require(updates == 68 && readbacks == 94 && rows == 31 && crops == 6 && openings == 14 && font_controls == 1 &&
        actions == 14 && restart_actions == 13 && exit_actions == 1 && deferred_actions == 4 && music_starts == 4 && records.good(),
        "End continuation drifted from its fixed update/readback/action/crop budget");
    std::ofstream summary(output / "end-continuation-summary.json");
    summary << "{\"outcome\":\"passed\",\"scope\":\"end-continuation-only\",\"updates\":68,\"readbacks\":94,"
        "\"csv_rows\":31,\"compact_crops\":6,\"page_openings\":14,\"font_negative_controls\":1,"
        "\"resize_updates\":0,\"save_calls\":1,\"crop_pixels\":311040,\"typed_actions\":14,"
        "\"restart_actions\":13,\"exit_actions\":1,\"deferred_actions\":4,\"music_starts\":4}\n";
    require(summary.good(), "Cannot retain End continuation summary");
    std::cout << "PASS End continuation leaf: six actual Session states/four independent plain literals, complete primary card/arrow "
        "and persistent primary off focus; literal-brace custom copy and numeric/letter font through assignment/reopen/reversal, "
        "point sampler and zero-update720/1080/720 resize; frozen Session/setup/private Save/RNG; "
        "14 actual typed callbacks (13 restart/one exit), four owner-controlled music deferrals/repeated completions; "
        "68 updates,94 readbacks,31 CSV rows,six compact crops,one private Play seed Save. "
        "No match launch/global-input/physical-pad/display/hardware/listening/human/performance acceptance.\n";
}
