// Actual Results consumers plus three public-builder Level reveals. The latter
// compare only opaque card interiors, not the simulated picture or foreground.
void result_percentage_checks(Application& app, Registry<LevelLoadedInfo>& levels,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    const Presentation original = presentation;
    const MatchTuning original_match = tuning.match();
    const Canvas canvas(app.render_resources());
    using Words = std::array<const wchar_t*, 3>;
    struct ResultCase
    {
        LevelEndInfo info; Session session; const char* name; int players; int best_of;
        Words defaults; Words old; Words next; std::array<float, 2> fill;
        const wchar_t* heading; const wchar_t* score; const wchar_t* outcome;
    };
    LevelEndInfo solo_a; solo_a.total_area = 100.0f; solo_a.team_a_area = 72.0f; solo_a.team_b_played = false;
    solo_a.team_colours = TeamColourTools::team_colours(TeamColourPair::blue_orange, original.team_palette());
    solo_a.player_areas[paint_owners::index(input_slots::keyboard_and_mouse, 0)] = 72.0f;
    LevelEndInfo competitive = solo_a; competitive.team_a_area = 60.0f; competitive.team_b_area = 40.0f;
    competitive.team_b_played = true;
    competitive.player_areas[paint_owners::index(input_slots::keyboard_and_mouse, 0)] = 60.0f;
    competitive.player_areas[paint_owners::index(-1, 1)] = 40.0f;
    LevelEndInfo solo_b = solo_a; solo_b.team_a_area = 0.0f; solo_b.team_b_area = 72.0f;
    solo_b.team_a_played = false; solo_b.team_b_played = true;
    Session single_a, series(3), single_b;
    single_a.finish_round(solo_a); series.finish_round(competitive); single_b.finish_round(solo_b);
    const std::array<ResultCase, 3> cases{ {
        { solo_a, single_a, "solo-a", 1, 1,
            { L"72.0% Painted", L"A 72.0%", L"B 0.0%" },
            { L"Old~ 72.0%", L"A~ 72.0%", L"B~ 0.0%" },
            { L"^solo 72.0%", L"^72.0% A", L"^0.0% B" }, { 0.72f, 0.0f },
            L"Results", L"Wins: A 0 - B 0  |  First to 1", L"Team B wins the series!" },
        { competitive, series, "competitive", 2, 3,
            { L"Team A Wins!", L"A 60.0%", L"B 40.0%" },
            { L"Team A Wins!", L"A~ 60.0%", L"B~ 40.0%" },
            { L"Team A Wins!", L"^60.0% A", L"^40.0% B" }, { 0.6f, 0.4f },
            L"Round 1", L"Wins: A 1 - B 0  |  First to 2", L"Series continues" },
        { solo_b, single_b, "solo-b", 1, 1,
            { L"72.0% Painted", L"A 0.0%", L"B 72.0%" },
            { L"Old~ 72.0%", L"A~ 0.0%", L"B~ 72.0%" },
            { L"^solo 72.0%", L"^0.0% A", L"^72.0% B" }, { 0.0f, 0.72f },
            L"Results", L"Wins: A 0 - B 0  |  First to 1", L"Team B wins the series!" } } };
    const auto ledger = [](const Session& value)
    {
        return std::vector<int>{ value.is_series() ? 1 : 0, value.complete() ? 1 : 0,
            value.round(), value.team_a_wins(), value.team_b_wins() };
    };
    std::array<std::vector<int>, 3> frozen_ledgers;
    for (size_t index = 0; index < cases.size(); ++index) { frozen_ledgers[index] = ledger(cases[index].session); }
    const NestedSaveProfile profile(output / "result-percentage-profile");
    Save preferences; PlaySettings initial; initial.music_volume = initial.sfx_volume = 0;
    initial.aim_sensitivity = 4; initial.view_shake = false; initial.reduce_effects = true;
    require(preferences.set_play_settings_and_save(initial), "Cannot seed result percentage's private preferences");
    const auto saved_bytes = [&]
    {
        std::ifstream file(output / "result-percentage-profile/ColourWars/save_data.json", std::ios::binary);
        require(file.good(), "Result percentage probe lost its private save");
        return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    };
    const std::string saved = saved_bytes();
    GameData data; data.set_application(&app); data.set_save(&preferences);
    data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MenuLevelSettings settings;
    settings.set_duration(73.0f); settings.set_stage("movement_course");
    MenuContext context(&data, &input, &settings); StateContext stack;
    struct RestoreResultPercentage
    {
        StateContext& stack; MenuContext& context; Presentation& presentation; Presentation original;
        ~RestoreResultPercentage()
        {
            this->context.cancel_music_exit(); this->context.set_music_update({});
            this->stack.clear(); this->presentation = this->original;
        }
    } restore{ stack, context, presentation, original };
    size_t updates = 0, level_updates = 0, readbacks = 0, result_rows = 0, monitor_rows = 0;
    size_t openings = 0, pops = 0, font_controls = 0, crops = 0, level_builds = 0;
    const ResultCase* active = nullptr;
    const auto saved_unchanged = [&]
    {
        const auto& play = preferences.play_settings();
        require(play.music_volume == 0 && play.sfx_volume == 0 && play.aim_sensitivity == 4 && !play.view_shake &&
            play.aim_marker && play.reduce_effects && preferences.resolution() == ScreenResolution::s_1920_1080 &&
            !preferences.fullscreen() && preferences.team_palette() == TeamColourPair::random &&
            preferences.records().stages().empty() && !preferences.last_match_setup() && saved_bytes() == saved &&
            tuning.match().duration == original_match.duration && tuning.match().duration_choices == original_match.duration_choices,
            "Result percentage changed active preferences, records, private bytes or numeric duration tuning");
        for (const auto& slot : play.sticks)
            { require(slot.movement_deadzone == -1 && slot.aim_deadzone == -1, "Result percentage changed a pad profile"); }
    };
    const auto unchanged = [&]
    {
        saved_unchanged();
        for (size_t index = 0; index < cases.size(); ++index)
        {
            const auto& fixture = cases[index];
            require(ledger(fixture.session) == frozen_ledgers[index] &&
                fixture.session.score_string(original.menu_text().results) == fixture.score &&
                fixture.session.result_string(original.menu_text().results) == fixture.outcome &&
                fixture.info.winning_team_string(original.menu_text().results) == fixture.defaults[0] &&
                fixture.info.team_a_percentage_string(original.menu_text().results) == fixture.defaults[1] &&
                fixture.info.team_b_percentage_string(original.menu_text().results) == fixture.defaults[2] &&
                fixture.info.total_area == 100.0f && fixture.info.team_a_area == (index == 0 ? 72.0f : index == 1 ? 60.0f : 0.0f) &&
                fixture.info.team_b_area == (index == 0 ? 0.0f : index == 1 ? 40.0f : 72.0f) &&
                fixture.info.team_a_played == (index != 2) && fixture.info.team_b_played == (index != 0),
                "Result percentage changed a frozen Session, outcome or original floating percentage spelling");
        }
        require(active != nullptr && settings.player_count() == active->players && settings.best_of() == active->best_of &&
            settings.duration() == 73.0f && settings.stage() == "movement_course" && !context.music_exit_pending(),
            "Result percentage changed its frozen page setup or pending music state");
        const auto players = settings.player_settings();
        require(players.size() == static_cast<size_t>(active->players) && players[0].num == input_slots::keyboard_and_mouse &&
            players[0].team == (active->info.team_a_played ? PlayerTeam::a : PlayerTeam::b) &&
            players[0].weapon == WeaponType::sprayer &&
            settings.screen_layout() == (active->players == 1 ? ScreenLayout::one_player : ScreenLayout::two_player),
            "Result percentage changed first-seat participation, input, weapon or screen layout");
        if (active->players == 2)
            { require(players[1].num == -1 && players[1].team == PlayerTeam::b && players[1].weapon == WeaponType::roller,
                "Result percentage changed the device-less second seat"); }
    };
    const auto resize_menu = [&](Vector2I size)
    {
        const size_t before = updates; std::srand(12613); const int next_random = std::rand(); std::srand(12613);
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        require(updates == before && std::rand() == next_random, "Result percentage resize updated the page or consumed gameplay RNG");
    };
    app.viewport_manager()->set_layout(ScreenLayout::one_player); resize_menu({ 1280, 720 });
    const auto tick = [&](float elapsed = 0.0f)
    {
        require(updates < 60, "Result percentage exceeded its StateContext update budget");
        ++updates; app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed);
    };
    const auto tap = [&](Key key)
        { app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick(); unchanged(); };
    const auto open = [&](const ResultCase& fixture, bool revealed)
    {
        stack.clear(); stack.notify_activation(true); context.cancel_music_exit(); active = &fixture;
        settings.set_player_count(fixture.players); settings.set_best_of(fixture.best_of);
        settings.set_screen_layout(fixture.players == 1 ? ScreenLayout::one_player : ScreenLayout::two_player);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_player_team(0, fixture.info.team_a_played ? PlayerTeam::a : PlayerTeam::b);
        settings.set_player_weapon(0, WeaponType::sprayer);
        if (fixture.players == 2) { settings.set_player_team(1, PlayerTeam::b); settings.set_player_weapon(1, WeaponType::roller); }
        app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::up, Key::down, Key::left, Key::right, Key::enter, Key::escape, Key::space, Key::backspace })
            { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        stack.push(std::make_unique<ResultsMenuProbeRoot>(app, canvas));
        stack.push(std::make_unique<ResultsMenuInitial>(&context, fixture.info, fixture.session, revealed));
        ++openings; tick(); require(stack.depth() == 2, "Result percentage consumed entry input or lost its page"); unchanged();
    };
    const auto proceed = [&]
    {
        require(stack.depth() == 2, "Result percentage page closed before fresh continuation input");
        tap(Key::enter); require(stack.depth() == 1, "Fresh Enter did not pop the result percentage page"); ++pops; unchanged();
    };
    const auto read_frame = [&](Renderer& renderer)
    {
        require(readbacks < 120, "Result percentage exceeded its readback budget"); ++readbacks; return read(renderer);
    };
    const auto actual_result = [&]
        { app.renderer()->begin_frame(); stack.draw(*app.renderer()); return read_frame(*app.renderer()); };
    const auto project = [&](RectangleF bounds)
    {
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        camera.calculate_view_rectangle(bounds); return countdown_crop_bounds(bounds);
    };
    const RectangleF winner_region(600.0f, 246.0f, 720.0f, 93.0f);
    const RectangleF plate_region(588.0f, 480.0f, 744.0f, 96.0f);
    const RectangleF heading_region(360.0f, 117.0f, 1200.0f, 129.0f);
    const RectangleF session_region(420.0f, 339.0f, 1080.0f, 135.0f);
    struct TextMetric { Vector2F raw; Vector2F position; float scale = 1.0f; };
    std::array<TextMetric, 3> result_metrics;
    const auto result_reference = [&](const ResultCase& fixture, const Presentation& snapshot, const Words& words,
        bool revealed, bool details, bool wrong_font = false)
    {
        const auto& style = snapshot.menu(); const auto& look = snapshot.results_menu();
        const std::string& item_font = wrong_font ? original.menu().item_font : style.item_font;
        const std::string& detail_font = wrong_font ? original.menu().detail_font : style.detail_font;
        const std::array<bool, 3> visible{ details && !revealed, details && fixture.info.team_a_played,
            details && fixture.info.team_b_played };
        std::array<std::unique_ptr<UiTextDropShadow>, 3> labels;
        for (size_t role = 0; role < labels.size(); ++role)
        {
            labels[role] = std::make_unique<UiTextDropShadow>("independent-result-percentage", words[role],
                role == 0 ? item_font : detail_font, Vector2F::ZERO, app.render_resources(), look.art.ink,
                Colour(), Vector2F::ZERO, !visible[role]);
            const Vector2F raw = labels[role]->bounds().size();
            const Vector2F at = role == 0 ? Vector2F((1920.0f - raw.x) / 2.0f, 255.0f) :
                Vector2F(role == 1 ? 620.0f : 1300.0f - raw.x, 500.0f);
            labels[role]->set_position(at); result_metrics[role] = { raw, at, 1.0f };
        }
        UiTextDropShadow heading("independent-result-percentage-title", fixture.heading, style.heading_font,
            Vector2F::ZERO, app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO);
        heading.set_position({ (1920.0f - heading.bounds().width) / 2.0f, 125.0f });
        // The fixed A mark extends into this complete plate band even before
        // the percentages appear. Include both literal marks at their original
        // centers, with the same absent-team and already-revealed gates.
        UiTextDropShadow mark_a("independent-result-percentage-mark-a", L"A", item_font,
            Vector2F::ZERO, app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO,
            revealed || !fixture.info.team_a_played);
        UiTextDropShadow mark_b("independent-result-percentage-mark-b", L"B", item_font,
            Vector2F::ZERO, app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO,
            revealed || !fixture.info.team_b_played);
        mark_a.set_position(Vector2F(562.0f, 550.0f) - mark_a.bounds().center());
        mark_b.set_position(Vector2F(1358.0f, 550.0f) - mark_b.bounds().center());
        UiTextDropShadow score("independent-result-percentage-score", fixture.score, style.detail_font,
            Vector2F::ZERO, app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO, !details || !fixture.session.is_series());
        score.set_position({ (1920.0f - score.bounds().width) / 2.0f, 340.0f });
        UiTextDropShadow outcome("independent-result-percentage-outcome", fixture.outcome, style.detail_font,
            Vector2F::ZERO, app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO, !details || !fixture.session.is_series());
        outcome.set_position({ (1920.0f - outcome.bounds().width) / 2.0f, 405.0f });
        const RectangleF panel((menu_consts::DESIGN_RESOLUTION - look.box_size) / 2.0f, look.box_size);
        const RectangleF fill_box(610.0f, 490.0f, 700.0f, 120.0f);
        std::array<RectangleF, 2> plates;
        for (size_t side = 0; side < plates.size(); ++side)
        {
            plates[side] = labels[side + 1]->bounds(); plates[side].inflate(look.art.focus_padding);
            require(plate_region.contains(plates[side]) && panel.contains(plates[side]),
                "Independent complete percentage plate escaped the measured unscaled artifact band");
        }
        require(winner_region.contains(labels[0]->bounds()) && heading_region.contains(heading.bounds()) &&
            session_region.contains(score.bounds()) && session_region.contains(outcome.bounds()),
            "Independent result percentage or unchanged neighbor escaped the measured zero-offset band");
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
        list.set_viewport(viewport); list.set_camera(camera); list.set_filter(TextureFilter::point);
        canvas.draw(list, camera.visible_rectangle(viewport)); FocusGroup empty_focus;
        const MenuPaperArtwork paper(panel, app.render_resources(), &style, &look.art, &empty_focus); paper.draw(list);
        const auto fill = [&](const RectangleF& rectangle, const Colour& colour)
        {
            UiTexture block("independent-result-fill", style.sprite_sheet, style.fill_frame,
                rectangle, app.render_resources(), colour); block.draw(list);
        };
        if (!revealed)
        {
            fill(fill_box, look.art.ink);
            fill(RectangleF(614.0f, 494.0f, 692.0f, 112.0f), look.track_colour);
            if (details && fixture.info.team_a_played)
                { fill(RectangleF(620.0f, 500.0f, look.team_fill_size.x * fixture.fill[0], look.team_fill_size.y), fixture.info.team_colours.team_a); }
            if (details && fixture.info.team_b_played)
            {
                const float width = look.team_fill_size.x * fixture.fill[1];
                fill(RectangleF(1300.0f - width, 500.0f, width, look.team_fill_size.y), fixture.info.team_colours.team_b);
            }
        }
        for (size_t side = 0; side < plates.size(); ++side)
        {
            if (visible[side + 1])
                { const MenuPaperArtwork plate(plates[side], app.render_resources(), &style, &look.art, &empty_focus); plate.draw(list); }
        }
        list.set_filter(TextureFilter::linear);
        heading.draw(list); mark_a.draw(list); mark_b.draw(list); score.draw(list); outcome.draw(list);
        for (const auto& label : labels) { label->draw(list); }
        return read_frame(renderer);
    };
    const auto compact = [&](const Frame& frame, const std::vector<RectangleI>& regions, bool horizontal)
    {
        std::vector<Frame> parts; int width = 0, height = 0;
        for (const auto& region : regions)
        {
            parts.push_back(foreground_crop(frame, region, 0));
            width = horizontal ? width + parts.back().width : std::max(width, parts.back().width);
            height = horizontal ? std::max(height, parts.back().height) : height + parts.back().height;
        }
        Frame strip{ width, height, std::vector<unsigned char>(static_cast<size_t>(width) * static_cast<size_t>(height) * 4, 0) };
        for (size_t alpha = 3; alpha < strip.pixels.size(); alpha += 4) { strip.pixels[alpha] = 255; }
        int offset = 0;
        for (const Frame& part : parts)
        {
            const int x = horizontal ? offset : (width - part.width) / 2;
            const int y = horizontal ? (height - part.height) / 2 : offset;
            for (int row = 0; row < part.height; ++row)
            {
                const size_t source = static_cast<size_t>(row) * static_cast<size_t>(part.width) * 4;
                const size_t target = (static_cast<size_t>(y + row) * static_cast<size_t>(width) + static_cast<size_t>(x)) * 4;
                std::copy_n(part.pixels.begin() + source, static_cast<size_t>(part.width) * 4, strip.pixels.begin() + target);
            }
            offset += horizontal ? part.width : part.height;
        }
        return strip;
    };
    std::ofstream records(output / "result-percentage.csv");
    records << "consumer,case,fixture,role,width,height,expected_value,font,raw_width,raw_height,x,y,scale,visible,"
        "revealed,details_visible,proceed_visible,region_match,neighbor_match,frozen_state_setup_save_rng\n";
    const auto write_row = [&](const char* consumer, const char* name, const char* fixture, const char* role,
        const Frame& actual, const wchar_t* literal, const std::string& font, const TextMetric& metric,
        bool visible, bool revealed, bool details, bool prompt)
    {
        records << consumer << ',' << name << ',' << fixture << ',' << role << ',' << actual.width << ',' << actual.height << ',';
        for (; *literal != L'\0'; ++literal) { records << static_cast<char>(*literal); }
        records << ',' << font << ',' << metric.raw.x << ',' << metric.raw.y << ',' << metric.position.x << ',' << metric.position.y << ',' <<
            metric.scale << ',' << (visible ? "true" : "false") << ',' << (revealed ? "true" : "false") << ',' <<
            (details ? "true" : "false") << ',' << (prompt ? "true" : "false") << ",true,true,true\n";
    };
    const auto compare_results = [&](const char* name, const ResultCase& fixture, const Presentation& snapshot,
        const Words& words, bool revealed, bool details, bool prompt, const char* crop = "", bool negative_fonts = false)
    {
        unchanged(); std::srand(12473); const int next_random = std::rand(); std::srand(12473);
        const Frame actual = actual_result();
        require(actual.pixels == actual_result().pixels && std::rand() == next_random,
            "Frozen result percentage drawing changed pixels or consumed gameplay RNG");
        const Frame wanted = result_reference(fixture, snapshot, words, revealed, details);
        const auto measured = result_metrics;
        for (const auto& region : { winner_region, plate_region, heading_region, session_region })
        {
            const RectangleI area = project(region);
            if (foreground_crop(actual, area, 0).pixels != foreground_crop(wanted, area, 0).pixels)
            {
                std::cerr << "Result percentage case=" << name << " fixture=" << fixture.name << " region=" << region.x << ',' << region.y << '\n';
                throw std::runtime_error("Actual result percentage, complete plate/fill or unchanged neighbor differs from its literal reference");
            }
        }
        const std::array<bool, 3> visible{ details && !revealed, details && fixture.info.team_a_played, details && fixture.info.team_b_played };
        const std::array<const char*, 3> roles{ "winner", "team-a", "team-b" };
        if (negative_fonts)
        {
            const Frame wrong = result_reference(fixture, snapshot, words, revealed, details, true);
            for (size_t role = 0; role < visible.size(); ++role)
            {
                if (!visible[role]) { continue; }
                RectangleF tight(measured[role].position, measured[role].raw); tight.inflate(2.0f);
                const RectangleI area = project(tight);
                require(foreground_crop(actual, area, 0).pixels != foreground_crop(wrong, area, 0).pixels,
                    "Result percentage font control cannot distinguish one visible replacement role"); ++font_controls;
            }
        }
        for (size_t role = 0; role < roles.size(); ++role)
        {
            write_row("results", name, fixture.name, roles[role], actual, words[role],
                role == 0 ? snapshot.menu().item_font : snapshot.menu().detail_font, measured[role], visible[role], revealed, details, prompt);
            ++result_rows;
        }
        if (*crop != '\0')
        {
            const Frame strip = compact(actual, { project(winner_region), project(plate_region) }, false);
            require((actual.width == 1280 && strip.width == 496 && strip.height == 126) ||
                (actual.width == 1920 && strip.width == 744 && strip.height == 189), "Result percentage strip dimensions drifted");
            save(strip, output / crop); ++crops;
        }
        unchanged(); return actual;
    };
    Presentation old = original, next = original;
    auto& old_text = const_cast<MenuText&>(old.menu_text()).results;
    old_text.solo_coverage = L"Old~ {percentage}"; old_text.team_a_percentage = L"A~ {percentage}"; old_text.team_b_percentage = L"B~ {percentage}";
    auto& next_text = const_cast<MenuText&>(next.menu_text()).results;
    next_text.solo_coverage = L"^solo {percentage}"; next_text.team_a_percentage = L"^{percentage} A"; next_text.team_b_percentage = L"^{percentage} B";
    const auto add_caret_font = [&](const std::string& original_font, const std::string& name)
    {
        const Font* source = app.render_resources()->font(app.render_resources()->resolve_sprite_font(original_font));
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character < U'~'; ++character)
        {
            const Glyph* source_glyph = source->find(character);
            require(source_glyph != nullptr, "Result percentage font fixture lacks an original printable glyph");
            Glyph glyph = *source_glyph;
            if (character == U'^') { glyph.x_offset += 4.0f; glyph.x_advance += 2.0f; } glyphs.push_back(glyph);
        }
        app.render_resources()->add_font(name, std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
        require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(name), L"~"),
            "Result percentage replacement font still renders retained copy's tilde");
    };
    const std::string item_font = "result_percentage_item_caret", detail_font = "result_percentage_detail_caret";
    const std::string monitor_font = "result_percentage_monitor_caret";
    add_caret_font(original.menu().item_font, item_font); add_caret_font(original.menu().detail_font, detail_font);
    add_caret_font(original.opening().percent_font, monitor_font);
    const_cast<MenuStyle&>(next.menu()).item_font = item_font; const_cast<MenuStyle&>(next.menu()).detail_font = detail_font;
    const_cast<OpeningPresentation&>(next.opening()).percent_font = monitor_font;
    old.validate_resources(*app.render_resources(), *app.audio_resources()); next.validate_resources(*app.render_resources(), *app.audio_resources());
    const auto& look = original.results_menu();
    const auto finish = [&](bool revealed)
    {
        if (!revealed) { tick(look.team_fill_delay + 0.01f); tick(look.team_fill_time + 0.01f); tick(); }
        tick(look.show_results_delay + 0.01f); tick();
    };
    presentation = original; open(cases[0], false);
    tap(Key::enter); require(stack.depth() == 2, "Premature Enter bypassed percentage fill delay");
    compare_results("legacy-initial", cases[0], original, cases[0].defaults, false, false, false);
    tick(look.team_fill_delay + 0.01f); tick(look.team_fill_time + 0.01f);
    compare_results("legacy-fill-crossed", cases[0], original, cases[0].defaults, false, false, false);
    tick(); compare_results("legacy-details", cases[0], original, cases[0].defaults, false, true, false);
    tap(Key::enter); require(stack.depth() == 2, "Premature Enter bypassed percentage prompt delay");
    tick(look.show_results_delay + 0.01f);
    compare_results("legacy-prompt-crossed", cases[0], original, cases[0].defaults, false, true, false);
    tick(); const Frame default_result = compare_results("legacy-ready", cases[0], original, cases[0].defaults, false, true, true,
        "result-percentage-results-default-720.ppm");
    resize_menu({ 1920, 1080 }); compare_results("default-1080", cases[0], original, cases[0].defaults, false, true, true);
    resize_menu({ 1280, 720 });
    require(compare_results("default-return720", cases[0], original, cases[0].defaults, false, true, true).pixels == default_result.pixels,
        "Default percentage page resize changed frozen pixels");
    tap(Key::escape); require(stack.depth() == 2, "Escape bypassed percentage continuation"); proceed();
    open(cases[1], true); compare_results("competitive-initial", cases[1], original, cases[1].defaults, true, true, false);
    tap(Key::enter); require(stack.depth() == 2, "Already-revealed competitive percentage skipped prompt delay"); finish(true);
    compare_results("competitive-ready", cases[1], original, cases[1].defaults, true, true, true); proceed();
    open(cases[2], true); finish(true);
    compare_results("solo-b-ready", cases[2], original, cases[2].defaults, true, true, true); proceed();
    for (size_t index = 0; index < 2; ++index)
    {
        const auto& fixture = cases[index]; const bool revealed = index == 1;
        presentation = old; open(fixture, revealed); finish(revealed);
        const Frame old_frame = compare_results("old-primary", fixture, old, fixture.old, revealed, true, true,
            index == 0 ? "result-percentage-results-old-720.ppm" : "");
        presentation = next;
        require(compare_results("retained-old", fixture, old, fixture.old, revealed, true, true).pixels == old_frame.pixels,
            "Source assignment rebased captured percentage templates or Results fonts");
        resize_menu({ 1920, 1080 }); compare_results("old-1080", fixture, old, fixture.old, revealed, true, true);
        resize_menu({ 1280, 720 });
        require(compare_results("old-return720", fixture, old, fixture.old, revealed, true, true).pixels == old_frame.pixels,
            "Old percentage page resize changed its captured copy or font"); proceed();
        open(fixture, revealed); finish(revealed);
        const Frame next_frame = compare_results("reopened-new", fixture, next, fixture.next, revealed, true, true,
            index == 0 ? "result-percentage-results-new-720.ppm" : "", true);
        presentation = original;
        require(compare_results("retained-new", fixture, next, fixture.next, revealed, true, true).pixels == next_frame.pixels,
            "Source reversal rebased reopened percentage templates or Results fonts");
        resize_menu({ 1920, 1080 });
        compare_results("new-1080", fixture, next, fixture.next, revealed, true, true,
            index == 0 ? "result-percentage-results-new-1080.ppm" : "");
        resize_menu({ 1280, 720 });
        require(compare_results("new-return720", fixture, next, fixture.next, revealed, true, true).pixels == next_frame.pixels,
            "New percentage page resize changed its captured copy or font"); proceed();
    }
    open(cases[0], false); finish(false);
    require(compare_results("restored-default", cases[0], original, cases[0].defaults, false, true, true).pixels == default_result.pixels,
        "Restoring default templates did not recover the original percentage page"); proceed(); stack.clear();
    require(updates == 60 && result_rows == 81 && readbacks == 83 && openings == 8 && pops == 8 && font_controls == 4,
        "Result percentage menu sub-scope drifted from its fixed budget");

    // Simulation here is intentional fixture setup, independently counted from
    // StateContext. No GameLevel or save/record callback owns these three Levels.
    const PlaySettings level_play = preferences.play_settings();
    LevelBuilder builder(app.viewport_manager(), app.render_resources(), app.audio_resources(), &level_play,
        &levels, &tuning, &presentation, app.resolution_manager(), app.thread_pool(), app.partitioner(), app.mouse());
    std::array<std::unique_ptr<Level>, 3> monitor_levels;
    const auto level_tick = [&](Level& level, const std::vector<PlayerInputData>& inputs, float elapsed)
    {
        require(level_updates < 240, "Result percentage exceeded its separate Level-update budget");
        ++level_updates; level.update(inputs, elapsed);
    };
    const auto build_monitor = [&]
    {
        MenuLevelSettings setup; setup.set_player_count(2); setup.set_best_of(3); setup.set_duration(1.25f);
        setup.set_stage("movement_course"); setup.set_screen_layout(ScreenLayout::two_player); setup.begin_input_claims(3);
        require(setup.claim_input_slot(1), "Cannot claim the monitor's second roller seat");
        setup.set_player_team(0, PlayerTeam::a); setup.set_player_team(1, PlayerTeam::b);
        setup.set_player_weapon(0, WeaponType::roller); setup.set_player_weapon(1, WeaponType::roller);
        app.viewport_manager()->set_layout(ScreenLayout::two_player); std::srand(411);
        auto level = builder.build_level(setup, TeamColourPair::blue_orange, OpeningMode::full); ++level_builds;
        resize(app, *level, { 1280, 720 }); const size_t before = level_updates;
        constexpr float simulation_dt = 1.0f / 60.0f; auto inputs = neutral_inputs();
        inputs[3].skip_intro = true; level_tick(*level, inputs, simulation_dt); inputs[3].skip_intro = false;
        require(level->state() == LevelState::start_countdown, "Public monitor skip missed the countdown");
        level_tick(*level, inputs, tuning.match().countdown - simulation_dt * 0.5f);
        require(level->state() == LevelState::start_countdown, "Near-boundary monitor countdown advanced the world early");
        level_tick(*level, inputs, simulation_dt); require(level->state() == LevelState::active, "Monitor countdown did not begin live play");
        require(level->place_player(0, { 1200.0f, 1275.0f }), "Cannot place the existing monitor painting fixture");
        inputs[3].primary_shoot = true; inputs[3].x_movement = 1.0f;
        for (int tick_index = 0; tick_index < 75; ++tick_index)
        {
            require(level->state() == LevelState::active, "Monitor paint fixture ended before its fixed 76 active ticks");
            level_tick(*level, inputs, simulation_dt);
        }
        require(level->state() == LevelState::zoom_out, "Monitor paint fixture did not finish after its fixed active ticks");
        inputs = neutral_inputs(); level_tick(*level, inputs, tuning.match().zoom_out + 0.01f);
        require(level->state() == LevelState::overview, "Monitor public zoom transition missed overview");
        const auto& reveal = tuning.match().reveal;
        level_tick(*level, inputs, reveal.walk_in + reveal.hover + reveal.hover + reveal.decide +
            original.opening().reveal_art.beat_duration + 0.01f);
        const LevelEndInfo result = level->level_end_info();
        require(level_updates - before == 80 && level->state() == LevelState::overview && level->revealed() &&
            level->player_count() == 2 && level->player(0).player_num() == 3 && level->player(1).player_num() == 1 &&
            result.total_area > 0.0f && result.team_a_area > 0.0f && result.team_b_area == 0.0f &&
            result.team_a_played && result.team_b_played &&
            result.team_a_percentage_string(original.menu_text().results) == L"A 100.0%" &&
            result.team_b_percentage_string(original.menu_text().results) == L"B 0.0%",
            "Monitor fixture lost its public phase, participant claims or independent 100/0 percentage invariant");
        saved_unchanged(); return level;
    };
    const auto monitor_regions = [&]
    {
        const float scale = app.resolution_manager()->resolution_vec().y / 720.0f;
        return std::array<RectangleI, 2>{ countdown_crop_bounds(RectangleF(194.0f * scale, 166.0f * scale, 382.0f * scale, 88.0f * scale)),
            countdown_crop_bounds(RectangleF(698.0f * scale, 166.0f * scale, 382.0f * scale, 88.0f * scale)) };
    };
    std::array<TextMetric, 2> monitor_metrics;
    const auto monitor_reference = [&](const Presentation& snapshot, const std::array<const wchar_t*, 2>& words, bool wrong_font = false)
    {
        const auto& look = snapshot.opening(); const auto& art = look.reveal_art;
        const RectangleF window(Vector2F::ZERO, app.resolution_manager()->resolution_vec());
        const Monitor geometry(&look, app.render_resources(), {});
        const RectangleF screen = geometry.screen_for_glass(geometry.glass(window));
        const RectangleF picture = geometry.picture(screen); const auto regions = monitor_regions();
        const auto font = app.render_resources()->resolve_sprite_font(wrong_font ? original.opening().percent_font : look.percent_font);
        require(app.render_resources()->measure_text(font, L" ") == Vector2F::ZERO && art.ink.a == 1.0f && art.paper.a == 1.0f,
            "Monitor paper-only reference lost its zero-size SPACE or opaque face contract");
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app.viewport_manager()->fullscreen_viewport());
        list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point); canvas.draw(list, window);
        const RevealArt paper(&look, app.render_resources());
        const std::array<Colour, 2> colours{ solo_a.team_colours.team_a, solo_a.team_colours.team_b };
        const auto tint = [](const Colour& colour)
            { return Colour(colour.r * colour.a, colour.g * colour.a, colour.b * colour.a, colour.a); };
        for (size_t side = 0; side < words.size(); ++side)
        {
            const float height = std::min(picture.height * 0.45f, look.percent_height * window.height /
                (1.0f - art.padding * 2.0f - art.shadow_offset));
            const Vector2F centre(picture.x + picture.width * (side == 0 ? 0.25f : 0.75f), picture.y + picture.height * 0.32f);
            const RectangleF bounds(centre - Vector2F(picture.width * 0.46f, height) * 0.5f, Vector2F(picture.width * 0.46f, height));
            // SPACE draws the unchanged paper and returns before text/fit. The
            // visible literal, font, outline and both fit limits are independent.
            paper.draw(list, bounds, L" ", colours[side], -1.0f, true);
            const float pen = bounds.height * art.keyline_width, shade = bounds.height * art.shadow_offset;
            const float margin = bounds.height * art.tick_length + pen * 3.0f;
            const RectangleF face(bounds.x + margin, bounds.y, bounds.width - margin * 2.0f - shade, bounds.height - shade);
            const float padding = std::max(bounds.height * art.padding, pen * 3.6f);
            const Vector2F available = face.size() - Vector2F(padding * 2.0f);
            const Vector2F raw = app.render_resources()->measure_text(font, words[side]);
            const float scale = std::min(available.x / raw.x, available.y / raw.y);
            const Vector2F at = face.center() - raw * scale * 0.5f; const float outline = pen * 0.6f;
            RectangleF outlined(at, raw * scale); outlined.inflate(outline);
            const RectangleI roi = regions[side]; const RectangleF proof(static_cast<float>(roi.x), static_cast<float>(roi.y),
                static_cast<float>(roi.width), static_cast<float>(roi.height));
            const RectangleF opaque(face.x + pen * 2.0f, face.y, face.width - pen * 4.0f, face.height);
            require(raw.x > 0.0f && raw.y > 0.0f && std::isfinite(scale) && scale > 0.0f && opaque.contains(proof) && proof.contains(outlined),
                "Monitor inner-card reference clipped text/outline or escaped the proven opaque face");
            monitor_metrics[side] = { raw, at, scale }; list.set_filter(TextureFilter::linear);
            for (const Vector2F offset : { Vector2F(-outline, 0), Vector2F(outline, 0), Vector2F(0, -outline), Vector2F(0, outline),
                Vector2F(-outline, -outline), Vector2F(outline, -outline), Vector2F(-outline, outline), Vector2F(outline, outline) })
                { list.draw_text(font, words[side], at + offset, tint(art.ink), scale, 0.0f, Vector2F::ZERO, 0.0f); }
            list.draw_text(font, words[side], at, tint(colours[side]), scale, 0.0f, Vector2F::ZERO, 0.0f); list.set_filter(TextureFilter::point);
        }
        return read_frame(renderer);
    };
    const std::array<const wchar_t*, 2> monitor_default{ L"A 100.0%", L"B 0.0%" };
    const std::array<const wchar_t*, 2> monitor_old{ L"A~ 100.0%", L"B~ 0.0%" };
    const std::array<const wchar_t*, 2> monitor_next{ L"^100.0% A", L"^0.0% B" };
    const auto compare_monitor = [&](const char* name, Level& level, const Presentation& snapshot,
        const std::array<const wchar_t*, 2>& words, const char* crop = "", bool negative_font = false)
    {
        saved_unchanged(); const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
        const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
        require(level.state() == LevelState::overview && level.revealed(), "Frozen monitor left its clicked overview");
        const auto info_before = level.level_end_info();
        require(level.player_count() == 2 && level.player(0).player_num() == 3 && level.player(1).player_num() == 1 &&
            level.player(0).team() == PlayerTeam::a && level.player(1).team() == PlayerTeam::b &&
            level.player(0).primary_weapon().type() == WeaponType::roller && level.player(1).primary_weapon().type() == WeaponType::roller &&
            info_before.team_a_played && info_before.team_b_played && info_before.team_a_area > 0.0f && info_before.team_b_area == 0.0f,
            "Frozen monitor changed participant claims, teams, tools or the 100/0 coverage invariant");
        std::srand(12577); const int next_random = std::rand(); std::srand(12577);
        app.renderer()->begin_frame(); level.draw(*app.renderer()); const Frame actual = read_frame(*app.renderer());
        app.renderer()->begin_frame(); level.draw(*app.renderer()); const Frame repeated = read_frame(*app.renderer());
        require(actual.pixels == repeated.pixels && std::rand() == next_random, "Frozen monitor drawing changed pixels or consumed simulation RNG");
        const Frame wanted = monitor_reference(snapshot, words); const auto measured = monitor_metrics; const auto regions = monitor_regions();
        for (size_t side = 0; side < words.size(); ++side)
        {
            require(foreground_crop(actual, regions[side], 0).pixels == foreground_crop(wanted, regions[side], 0).pixels,
                "Actual monitor inner percentage card differs from independent literal font/outline/fit math");
            write_row("monitor", name, "2p-roller", side == 0 ? "team-a" : "team-b", actual, words[side],
                snapshot.opening().percent_font, measured[side], true, true, true, false); ++monitor_rows;
        }
        if (negative_font)
        {
            const Frame wrong = monitor_reference(snapshot, words, true);
            for (const auto& region : regions)
            {
                require(foreground_crop(actual, region, 0).pixels != foreground_crop(wrong, region, 0).pixels,
                    "Monitor caret control cannot distinguish one rebuilt percentage font"); ++font_controls;
            }
        }
        if (*crop != '\0')
        {
            const Frame strip = compact(actual, { regions[0], regions[1] }, true);
            require((actual.width == 1280 && strip.width == 764 && strip.height == 88) ||
                (actual.width == 1920 && strip.width == 1146 && strip.height == 132), "Monitor percentage strip dimensions drifted");
            save(strip, output / crop); ++crops;
        }
        require(before == observation(level), "Monitor inner-card drawing changed frozen public simulation state");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
            "Monitor inner-card drawing changed the Debug replay fingerprint");
#endif
        saved_unchanged(); return actual;
    };
    const auto resize_monitor = [&](Level& level, Vector2I size)
    {
        const size_t before = level_updates; const auto state = observation(level);
        std::srand(12647); const int next_random = std::rand(); std::srand(12647); resize(app, level, size);
        require(level_updates == before && updates == 60 && observation(level) == state && std::rand() == next_random,
            "Monitor percentage resize advanced simulation, page input or gameplay RNG");
    };
    presentation = original; monitor_levels[0] = build_monitor();
    const auto default_state = observation(*monitor_levels[0]);
    const Frame default_monitor = compare_monitor("default-primary", *monitor_levels[0], original, monitor_default,
        "result-percentage-monitor-default-720.ppm");
    resize_monitor(*monitor_levels[0], { 1920, 1080 }); compare_monitor("default-1080", *monitor_levels[0], original, monitor_default);
    resize_monitor(*monitor_levels[0], { 1280, 720 });
    require(compare_monitor("default-return720", *monitor_levels[0], original, monitor_default).pixels == default_monitor.pixels &&
        observation(*monitor_levels[0]) == default_state, "Default monitor resize did not restore frozen pixels/state");
    presentation = old; monitor_levels[1] = build_monitor(); const auto old_state = observation(*monitor_levels[1]);
    const Frame old_monitor = compare_monitor("old-primary", *monitor_levels[1], old, monitor_old, "result-percentage-monitor-old-720.ppm");
    presentation = next;
    require(compare_monitor("retained-old", *monitor_levels[1], old, monitor_old).pixels == old_monitor.pixels,
        "Presentation assignment rebased a preconstructed Level's cached percentage wrappers/font");
    resize_monitor(*monitor_levels[1], { 1920, 1080 }); compare_monitor("old-1080", *monitor_levels[1], old, monitor_old);
    resize_monitor(*monitor_levels[1], { 1280, 720 });
    require(compare_monitor("old-return720", *monitor_levels[1], old, monitor_old).pixels == old_monitor.pixels &&
        observation(*monitor_levels[1]) == old_state, "Old monitor resize changed captured text/font or public state");
    monitor_levels[2] = build_monitor(); const auto next_state = observation(*monitor_levels[2]);
    const Frame next_monitor = compare_monitor("new-primary", *monitor_levels[2], next, monitor_next,
        "result-percentage-monitor-new-720.ppm", true);
    presentation = original;
    require(compare_monitor("retained-new", *monitor_levels[2], next, monitor_next).pixels == next_monitor.pixels,
        "Presentation reversal rebased the rebuilt Level's percentage wrappers/font epoch");
    resize_monitor(*monitor_levels[2], { 1920, 1080 });
    compare_monitor("new-1080", *monitor_levels[2], next, monitor_next, "result-percentage-monitor-new-1080.ppm");
    resize_monitor(*monitor_levels[2], { 1280, 720 });
    require(compare_monitor("new-return720", *monitor_levels[2], next, monitor_next).pixels == next_monitor.pixels &&
        observation(*monitor_levels[2]) == next_state, "New monitor resize changed captured text/font or public state");
    resize_monitor(*monitor_levels[0], { 1280, 720 });
    require(compare_monitor("restored-retained-default", *monitor_levels[0], original, monitor_default).pixels == default_monitor.pixels &&
        observation(*monitor_levels[0]) == default_state, "Restored source changed the retained default Level's percentage pixels/state");
    require(updates == 60 && level_updates == 240 && readbacks == 120 && result_rows == 81 && monitor_rows == 24 &&
        openings == 8 && pops == 8 && font_controls == 6 && crops == 8 && level_builds == 3 && records.good(),
        "Result percentage drifted from its separately counted menu/Level/readback/CSV/crop/action budgets");
    std::ofstream summary(output / "result-percentage-summary.json");
    summary << "{\"outcome\":\"passed\",\"scope\":\"result-percentage-only\",\"updates\":60,\"level_updates\":240,\"readbacks\":120,"
        "\"csv_rows\":105,\"results_rows\":81,\"monitor_rows\":24,\"compact_crops\":8,\"page_openings\":8,\"proceed_pops\":8,"
        "\"font_negative_controls\":6,\"resize_updates\":0,\"save_calls\":1,\"level_builds\":3,\"crop_pixels\":681072}\n";
    require(summary.good(), "Cannot retain result percentage summary");
    std::cout << "PASS Result percentage leaf: independent solo coverage and A/B plate literals with original participation, hidden-winner and input gates; "
        "captured templates/three caret-only font aliases through retained pages, reopening, seeded Level rebuilds, reversal and zero-update resizing. "
        "Three actual public-builder two-player roller Levels intentionally advance80 updates each to clicked overview; "
        "only opaque monitor inner-card interiors compare independent literal font/outline/width-height fitting over unchanged paper. "
        "Frozen drawing preserves public state, draw RNG and Debug replay fingerprint when available; private preferences/bytes remain unchanged. "
        "60 StateContext updates plus240 separate Level updates,120 readbacks,105 CSV rows,8 page openings/pops,6 font controls, "
        "8 compact crops (681072 pixels /2043334 PPM bytes),one private seed Save. Both presets use the normal reader. "
        "No whole-picture/foreground, global-input, physical-pad, listening, human readability or performance acceptance.\n";
}
