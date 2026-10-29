// The sparse roster deliberately separates row, device and paint-owner indices.
// Complete row/neighbor literals and raw badge cells bypass their production
// text composition, credit selection and UiPlayerBadge/PlayerBadge helpers.
void contribution_team_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    const Presentation original = presentation;
    const MatchTuning original_match = tuning.match();
    using RowWords = std::array<const wchar_t*, 4>;
    const RowWords default_words{ L"P1 - Keyboard/mouse | Team A | 18.7%", L"P2 - Pad 3 | Team B | 25.0%",
        L"P3 - Waiting to join | Team A | 31.2%", L"P4 - Pad 1 | Team B | 25.0%" };
    const RowWords old_words{ L"P1 - Keyboard/mouse | Old~ A {A} | 18.7%", L"P2 - Pad 3 | Old~ B {B} | 25.0%",
        L"P3 - Waiting to join | Old~ A {A} | 31.2%", L"P4 - Pad 1 | Old~ B {B} | 25.0%" };
    const RowWords next_words{ L"P1 - Keyboard/mouse | {A} team | 18.7%", L"P2 - Pad 3 | {B} team | 25.0%",
        L"P3 - Waiting to join | {A} team | 31.2%", L"P4 - Pad 1 | {B} team | 25.0%" };
    // A mistaken player_areas[row] would show these four different values.
    const RowWords wrong_owner_words{ L"P1 - Keyboard/mouse | {A} team | 25.0%", L"P2 - Pad 3 | {B} team | 0.0%",
        L"P3 - Waiting to join | {A} team | 25.0%", L"P4 - Pad 1 | {B} team | 0.0%" };
    const RowWords percentages{ L"18.7%", L"25.0%", L"31.2%", L"25.0%" };
    const std::array<int, 4> slots{ input_slots::keyboard_and_mouse, 2, -1, 0 };
    const std::array<int, 4> owners{ 4, 2, 7, 0 };
    const std::array<PlayerTeam, 4> teams{ PlayerTeam::a, PlayerTeam::b, PlayerTeam::a, PlayerTeam::b };
    const std::array<float, paint_owners::count> areas{ 250.0f, 0.0f, 250.0f, 0.0f, 187.5f, 0.0f, 0.0f, 312.5f, 0.0f };
    LevelEndInfo info; info.total_area = 1000.0f; info.team_a_area = info.team_b_area = 500.0f;
    info.team_colours = TeamColourTools::team_colours(TeamColourPair::blue_orange, original.team_palette());
    info.player_areas = areas;
    Session session(3); session.finish_round(info);
    const auto ledger = [](const Session& value)
    {
        return std::vector<int>{ value.is_series() ? 1 : 0, value.complete() ? 1 : 0,
            value.round(), value.team_a_wins(), value.team_b_wins() };
    };
    const std::vector<int> frozen_ledger = ledger(session);
    const NestedSaveProfile profile(output / "contribution-team-profile");
    Save preferences; PlaySettings initial; initial.music_volume = initial.sfx_volume = 0;
    initial.aim_sensitivity = 4; initial.view_shake = false; initial.reduce_effects = true;
    require(preferences.set_play_settings_and_save(initial), "Cannot seed contribution team's private preferences");
    const auto saved_bytes = [&]
    {
        std::ifstream file(output / "contribution-team-profile/ColourWars/save_data.json", std::ios::binary);
        require(file.good(), "Contribution team probe lost its private save");
        return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    };
    const std::string saved = saved_bytes();
    GameData data; data.set_application(&app); data.set_save(&preferences);
    data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MenuLevelSettings settings;
    settings.set_player_count(4); settings.set_best_of(3); settings.set_duration(73.0f);
    settings.set_stage("movement_course"); settings.set_screen_layout(ScreenLayout::four_player);
    settings.begin_input_claims(input_slots::keyboard_and_mouse);
    require(settings.claim_input_slot(2) && settings.claim_input_slot(1) && settings.claim_input_slot(0) &&
        settings.release_input_slot(1), "Cannot assemble contribution team's sparse public input claims");
    for (size_t row = 0; row < slots.size(); ++row)
    {
        settings.set_player_team(static_cast<int>(row), teams[row]);
        settings.set_player_weapon(static_cast<int>(row), row % 2 == 0 ? WeaponType::sprayer : WeaponType::roller);
    }
    MenuContext context(&data, &input, &settings); StateContext stack;
    struct RestoreContributionTeam
    {
        StateContext& stack; MenuContext& context; Presentation& presentation; Presentation original;
        ~RestoreContributionTeam()
        {
            this->context.cancel_music_exit(); this->context.set_music_update({});
            this->stack.clear(); this->presentation = this->original;
        }
    } restore{ stack, context, presentation, original };
    size_t updates = 0, readbacks = 0, rows = 0, crops = 0, openings = 0, pops = 0;
    size_t font_controls = 0, owner_controls = 0;
    const auto unchanged = [&]
    {
        require(ledger(session) == frozen_ledger && session.score_string(original.menu_text().results) ==
            L"Wins: A 0 - B 0  |  First to 2" && session.result_string(original.menu_text().results) == L"Draw - no win awarded" &&
            info.winning_team_string(original.menu_text().results) == L"Draw!" && info.total_area == 1000.0f &&
            info.team_a_area == 500.0f && info.team_b_area == 500.0f && info.team_a_played && info.team_b_played &&
            info.player_areas == areas, "Contribution team changed the frozen draw ledger, areas or Session copy");
        const auto players = settings.player_settings(); const auto& play = preferences.play_settings();
        require(settings.player_count() == 4 && settings.best_of() == 3 && settings.duration() == 73.0f &&
            settings.stage() == "movement_course" && settings.screen_layout() == ScreenLayout::four_player &&
            players.size() == 4 && !settings.input_roster_complete() && !context.music_exit_pending(),
            "Contribution team changed setup, incomplete claims or pending music state");
        for (size_t row = 0; row < slots.size(); ++row)
        {
            require(players[row].num == slots[row] && players[row].team == teams[row] &&
                settings.player_weapon(static_cast<int>(row)) == (row % 2 == 0 ? WeaponType::sprayer : WeaponType::roller) &&
                paint_owners::index(players[row].num, static_cast<int>(row)) == owners[row] &&
                info.player_percentage_string(owners[row]) == percentages[row],
                "Contribution team changed a participant claim, team, weapon or owner-specific truncated credit");
        }
        require(play.music_volume == 0 && play.sfx_volume == 0 && play.aim_sensitivity == 4 && !play.view_shake &&
            play.aim_marker && play.reduce_effects && preferences.resolution() == ScreenResolution::s_1920_1080 &&
            !preferences.fullscreen() && preferences.team_palette() == TeamColourPair::random &&
            preferences.records().stages().empty() && !preferences.last_match_setup() && saved_bytes() == saved &&
            tuning.match().duration == original_match.duration && tuning.match().duration_choices == original_match.duration_choices,
            "Contribution team changed active preferences, records, private bytes or numeric tuning");
        for (const auto& slot : play.sticks)
            { require(slot.movement_deadzone == -1 && slot.aim_deadzone == -1, "Contribution team changed a pad profile"); }
    };
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    const auto resize = [&](Vector2I size)
    {
        const size_t before = updates; app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        require(updates == before, "Contribution team resize updated the state stack");
    };
    resize({ 1280, 720 });
    const auto tick = [&](float elapsed = 0.0f)
    {
        require(updates < 45, "Contribution team exceeded its fixed update budget");
        ++updates; app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed);
    };
    const auto tap = [&](Key key)
        { app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick(); unchanged(); };
    const auto open = [&](bool revealed)
    {
        stack.clear(); stack.notify_activation(true); context.cancel_music_exit();
        app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::up, Key::down, Key::left, Key::right,
            Key::enter, Key::escape, Key::space, Key::backspace }) { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        stack.push(std::make_unique<ResultsMenuProbeRoot>(app, canvas));
        stack.push(std::make_unique<ResultsMenuInitial>(&context, info, session, revealed));
        ++openings; tick();
        require(stack.depth() == 2, "Contribution team consumed entry input or lost its actual page"); unchanged();
    };
    const auto proceed = [&]
    {
        require(stack.depth() == 2, "Contribution team page closed before fresh continuation input");
        tap(Key::enter); require(stack.depth() == 1, "Fresh Enter failed to pop the actual contribution team page");
        ++pops; unchanged();
    };
    const auto actual_frame = [&]
    {
        require(readbacks < 56, "Contribution team exceeded its fixed readback budget");
        app.renderer()->begin_frame(); stack.draw(*app.renderer()); ++readbacks; return read(*app.renderer());
    };
    // All actual24-font rows measure38 high (Lato Black 24's whole-pixel line
    // spacing), so each literal54x42 badge centres 2 above its row: the four
    // occupy 414,678 through468,857; the unchanged heading ends673, prompt
    // starts880. The band starts on 675, a whole pixel at 720p.
    const RectangleF contribution_region(408.0f, 675.0f, 756.0f, 183.0f);
    const RectangleF legend_region(450.0f, 633.0f, 900.0f, 45.0f);
    const RectangleF heading_region(360.0f, 117.0f, 1200.0f, 129.0f);
    const RectangleF session_region(420.0f, 339.0f, 1080.0f, 135.0f);
    const auto project = [&](RectangleF bounds)
    {
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        camera.calculate_view_rectangle(bounds); return countdown_crop_bounds(bounds);
    };
    const SpriteSheet* badge_sheet = app.render_resources()->sprite_sheet(
        app.render_resources()->resolve_sprite_sheet("sprite_sheet_1"));
    const SpriteFrame& badge_pixel = badge_sheet->sprite_frame(badge_sheet->resolve_sprite_frame("pixel"));
    struct ContributionMetrics { std::array<RectangleF, 4> text; std::array<RectangleF, 4> badge; } metrics;
    const auto reference = [&](const Presentation& snapshot, const RowWords& words, bool details_visible, bool wrong_font = false)
    {
        const auto& style = snapshot.menu(); const auto& look = snapshot.results_menu();
        const std::string& font = wrong_font ? original.results_menu().contribution_font : look.contribution_font;
        colourwars::UiTextDropShadow heading("independent-contribution-title", L"Round 1", style.heading_font, Vector2F::ZERO,
            app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO);
        heading.set_position({ (menu_consts::DESIGN_RESOLUTION.x - heading.bounds().width) / 2.0f, look.heading_y });
        colourwars::UiTextDropShadow score("independent-contribution-score", L"Wins: A 0 - B 0  |  First to 2", style.detail_font, Vector2F::ZERO,
            app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO, !details_visible);
        score.set_position({ (menu_consts::DESIGN_RESOLUTION.x - score.bounds().width) / 2.0f, look.session_score_y });
        colourwars::UiTextDropShadow outcome("independent-contribution-outcome", L"Draw - no win awarded", style.detail_font, Vector2F::ZERO,
            app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO, !details_visible);
        outcome.set_position({ (menu_consts::DESIGN_RESOLUTION.x - outcome.bounds().width) / 2.0f, look.session_result_y });
        colourwars::UiTextDropShadow legend("independent-contribution-legend", L"Surface held (% of arena)", look.contribution_font,
            Vector2F(480.0f, 635.0f), app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO, !details_visible);
        std::array<std::unique_ptr<colourwars::UiTextDropShadow>, 4> labels;
        const RectangleF panel((menu_consts::DESIGN_RESOLUTION - look.box_size) / 2.0f, look.box_size);
        require(heading_region.contains(heading.bounds()) && legend_region.contains(legend.bounds()) &&
            session_region.contains(score.bounds()) && session_region.contains(outcome.bounds()) &&
            score.bounds().bottom() < outcome.bounds().y,
            "Independent contribution neighbors escaped their measured unscaled zero-offset comparison regions");
        for (size_t row = 0; row < labels.size(); ++row)
        {
            const float y = 680.0f + 45.0f * static_cast<float>(row);
            labels[row] = std::make_unique<colourwars::UiTextDropShadow>("independent-contribution-row", words[row], font,
                Vector2F(480.0f, y), app.render_resources(), look.art.ink, Colour(), Vector2F::ZERO, !details_visible);
            metrics.text[row] = labels[row]->bounds();
            metrics.badge[row] = RectangleF(414.0f, y - 2.0f, 54.0f, 42.0f);
            const RectangleF full = RectangleF::union_of(metrics.text[row], metrics.badge[row]);
            require(metrics.text[row].width > 0.0f && metrics.text[row].height == 38.0f && panel.contains(full) &&
                contribution_region.contains(full) && metrics.badge[row].right() + 12.0f == metrics.text[row].x &&
                (row == 0 ? legend.bounds().bottom() < metrics.badge[row].y :
                    metrics.badge[row - 1].bottom() < metrics.badge[row].y) && full.bottom() < look.proceed_y,
                "Independent contribution row/badge escaped the measured full-geometry band or changed spacing");
        }
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
        list.set_viewport(viewport); list.set_camera(camera); list.set_filter(TextureFilter::point);
        canvas.draw(list, camera.visible_rectangle(viewport)); FocusGroup empty_focus;
        const MenuPaperArtwork paper(panel, app.render_resources(), &style, &look.art, &empty_focus); paper.draw(list);
        // Independent fixed identity artwork: raw source, explicit rectangles
        // and literal 3x5 P/digit masks. No production badge draw/before helper.
        const auto fill = [&](const RectangleF& rectangle, const Colour& colour)
        {
            list.draw_sprite(badge_sheet->texture(), badge_pixel.source_rectangle(), rectangle, colour,
                0.0f, badge_pixel.origin(), SpriteFlip::none, 0.0f);
        };
        const std::array<unsigned int, 5> letter_p{ 7u, 5u, 7u, 4u, 4u };
        const std::array<std::array<unsigned int, 5>, 4> digits{ {
            { 2u, 6u, 2u, 2u, 7u }, { 7u, 1u, 7u, 4u, 7u },
            { 7u, 1u, 7u, 1u, 7u }, { 5u, 5u, 7u, 1u, 1u } } };
        if (details_visible)
        {
            for (size_t row = 0; row < labels.size(); ++row)
            {
                const RectangleF rectangle = metrics.badge[row];
                const float cell = 38.0f / 7.0f;
                const Vector2F origin = rectangle.center() - Vector2F(cell * 7.0f, cell * 5.0f) / 2.0f;
                fill(rectangle, Colour::white);
                fill(RectangleF(rectangle.x, rectangle.y, 54.0f, 2.0f), Colour::black);
                fill(RectangleF(rectangle.x, rectangle.bottom() - 2.0f, 54.0f, 2.0f), Colour::black);
                fill(RectangleF(rectangle.x, rectangle.y + 2.0f, 2.0f, 38.0f), Colour::black);
                fill(RectangleF(rectangle.right() - 2.0f, rectangle.y + 2.0f, 2.0f, 38.0f), Colour::black);
                for (size_t pencil_row = 0; pencil_row < letter_p.size(); ++pencil_row)
                {
                    for (int column = 0; column < 3; ++column)
                    {
                        const unsigned int mask = 1u << (2 - column);
                        const Vector2F offset(static_cast<float>(column) * cell, static_cast<float>(pencil_row) * cell);
                        if ((letter_p[pencil_row] & mask) != 0u)
                            { fill(RectangleF(origin + offset, Vector2F(cell)), Colour::black); }
                        if ((digits[row][pencil_row] & mask) != 0u)
                            { fill(RectangleF(origin + offset + Vector2F(cell * 4.0f, 0.0f), Vector2F(cell)), Colour::black); }
                    }
                }
            }
        }
        list.set_filter(TextureFilter::linear);
        heading.draw(list); score.draw(list); outcome.draw(list); legend.draw(list);
        for (const auto& label : labels) { label->draw(list); }
        require(readbacks < 56, "Contribution team reference exceeded its fixed readback budget");
        ++readbacks; return read(renderer);
    };
    std::ofstream records(output / "contribution-team.csv");
    records << "case,row,width,height,device_slot,paint_owner,team,revealed,details_visible,proceed_visible,expected_value,font,"
        "raw_width,raw_height,x,y,badge_x,badge_y,badge_width,badge_height,scale,shadow_x,shadow_y,percentage_literal,"
        "heading_literal,contribution_heading_literal,score_literal,outcome_literal,row_match,badge_match,heading_match,"
        "neighbor_match,frozen_session_setup_save_rng\n";
    const auto compare = [&](const char* name, const Presentation& snapshot, const RowWords& words,
        bool revealed, bool details_visible, bool proceed_visible, const char* crop = "", bool negative_controls = false)
    {
        unchanged(); std::srand(9377); const int next_random = std::rand(); std::srand(9377);
        const Frame actual = actual_frame();
        require(actual.pixels == actual_frame().pixels && std::rand() == next_random,
            "Frozen contribution team drawing changed pixels or consumed gameplay RNG");
        const Frame wanted = reference(snapshot, words, details_visible);
        const ContributionMetrics measured = metrics;
        const RectangleI area = project(contribution_region);
        const Frame observed = foreground_crop(actual, area, 0), expected = foreground_crop(wanted, area, 0);
        if (observed.pixels != expected.pixels)
        {
            const RectangleI delta = foreground_delta_bounds(observed, expected);
            std::cerr << "Contribution team case=" << name << " area=" << area.x << ',' << area.y << ',' << area.width << ',' <<
                area.height << " local_delta=" << delta.x << ',' << delta.y << ',' << delta.width << ',' << delta.height << '\n';
            throw std::runtime_error("Actual contribution rows/badges differ from independent literal text and raw badge cells");
        }
        for (const RectangleF& bounds : { heading_region, legend_region, session_region })
        {
            const RectangleI neighbor = project(bounds);
            require(foreground_crop(actual, neighbor, 0).pixels == foreground_crop(wanted, neighbor, 0).pixels,
                "Contribution team extraction changed the unmodified heading, legend, score or outcome");
        }
        for (const RectangleF& badge : measured.badge)
        {
            RectangleF complete = badge; complete.inflate(2.0f); const RectangleI badge_area = project(complete);
            require(foreground_crop(actual, badge_area, 0).pixels == foreground_crop(wanted, badge_area, 0).pixels,
                "Contribution team changed a complete independent roster badge including its quantized edge");
        }
        if (negative_controls)
        {
            require(details_visible, "Contribution team negative controls require revealed rows");
            const Frame wrong_font = reference(snapshot, words, true, true);
            const Frame wrong_owner = reference(snapshot, wrong_owner_words, true);
            for (const RectangleF& row : measured.text)
            {
                RectangleF tight = row; tight.inflate(2.0f); const RectangleI tight_area = project(tight);
                require(foreground_crop(actual, tight_area, 0).pixels != foreground_crop(wrong_font, tight_area, 0).pixels,
                    "Contribution team brace font control cannot distinguish an individual reopened row font"); ++font_controls;
                require(foreground_crop(actual, tight_area, 0).pixels != foreground_crop(wrong_owner, tight_area, 0).pixels,
                    "Contribution team paint-owner control cannot distinguish an individual mistaken view-index credit"); ++owner_controls;
            }
        }
        if (*crop != '\0')
        {
            require((actual.width == 1280 && actual.height == 720 && observed.width == 504 && observed.height == 122) ||
                (actual.width == 1920 && actual.height == 1080 && observed.width == 756 && observed.height == 183),
                "Contribution team compact crops drifted from their fixed native pixel dimensions");
            save(observed, output / crop); ++crops;
        }
        for (size_t row = 0; row < words.size(); ++row)
        {
            records << name << ',' << row << ',' << actual.width << ',' << actual.height << ',' << slots[row] << ',' << owners[row] << ',' <<
                (teams[row] == PlayerTeam::a ? "A" : "B") << ',' << (revealed ? "true" : "false") << ',' <<
                (details_visible ? "true" : "false") << ',' << (proceed_visible ? "true" : "false") << ',';
            const auto ascii = [&](const wchar_t* text) { for (; *text != L'\0'; ++text) { records << static_cast<char>(*text); } };
            ascii(words[row]); const RectangleF& text = measured.text[row]; const RectangleF& badge = measured.badge[row];
            records << ',' << snapshot.results_menu().contribution_font << ',' << text.width << ',' << text.height << ',' << text.x << ',' << text.y << ',' <<
                badge.x << ',' << badge.y << ',' << badge.width << ',' << badge.height << ",1,0,0,";
            ascii(percentages[row]);
            records << ",Round 1,Surface held (% of arena),Wins: A 0 - B 0  |  First to 2,Draw - no win awarded,true,true,true,true,true\n";
            ++rows;
        }
        unchanged(); return actual;
    };
    const auto& look = original.results_menu();
    const auto finish = [&](bool revealed)
    {
        if (!revealed) { tick(look.team_fill_delay + 0.01f); tick(look.team_fill_time + 0.01f); tick(); }
        tick(look.show_results_delay + 0.01f); tick();
    };
    Presentation old = original, next = original;
    auto& old_text = const_cast<MenuText&>(old.menu_text()).results;
    old_text.contribution_team_a = L" | Old~ A {A} | "; old_text.contribution_team_b = L" | Old~ B {B} | ";
    auto& next_text = const_cast<MenuText&>(next.menu_text()).results;
    next_text.contribution_team_a = L" | {A} team | "; next_text.contribution_team_b = L" | {B} team | ";
    const Font* source = app.render_resources()->font(app.render_resources()->resolve_sprite_font(original.results_menu().contribution_font));
    std::vector<Glyph> glyphs;
    for (char32_t character = U' '; character < U'~'; ++character)
    {
        const Glyph* source_glyph = source->find(character);
        require(source_glyph != nullptr, "Contribution team font fixture lacks an original printable glyph");
        Glyph glyph = *source_glyph;
        if (character == U'{' || character == U'}') { glyph.x_offset += 1.25f; glyph.x_advance += 0.75f; }
        glyphs.push_back(glyph);
    }
    const std::string next_font = "contribution_team_braces";
    app.render_resources()->add_font(next_font,
        std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
    require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(next_font), L"~"),
        "Contribution team replacement font still renders retained old copy's tilde");
    const_cast<ResultsMenuPresentation&>(next.results_menu()).contribution_font = next_font;
    old.validate_resources(*app.render_resources(), *app.audio_resources());
    next.validate_resources(*app.render_resources(), *app.audio_resources());
    presentation = original; open(false);
    tap(Key::enter); require(stack.depth() == 2, "Premature Enter bypassed contribution team's fill delay");
    compare("legacy-initial", original, default_words, false, false, false);
    tick(look.team_fill_delay + 0.01f); tick(look.team_fill_time + 0.01f);
    compare("legacy-fill-crossed", original, default_words, false, false, false);
    tick(); compare("legacy-details", original, default_words, false, true, false);
    tap(Key::enter); require(stack.depth() == 2, "Premature Enter bypassed contribution team's post-reveal delay");
    tick(look.show_results_delay + 0.01f);
    compare("legacy-prompt-crossed", original, default_words, false, true, false);
    tick(); const Frame default_frame = compare("legacy-ready", original, default_words, false, true, true,
        "contribution-team-default-720.ppm");
    resize({ 1920, 1080 });
    compare("default-1080", original, default_words, false, true, true, "contribution-team-default-1080.ppm");
    resize({ 1280, 720 });
    require(compare("default-return720", original, default_words, false, true, true).pixels == default_frame.pixels,
        "Default contribution team resize changed frozen row/badge pixels");
    tap(Key::escape); require(stack.depth() == 2, "Escape bypassed contribution team continuation"); proceed();
    presentation = old; open(false); finish(false);
    const Frame old_frame = compare("old-primary", old, old_words, false, true, true, "contribution-team-old-720.ppm");
    presentation = next;
    require(compare("retained-old", old, old_words, false, true, true).pixels == old_frame.pixels,
        "Source assignment rebased constructed contribution team copy, credit or font");
    resize({ 1920, 1080 });
    compare("old-1080", old, old_words, false, true, true, "contribution-team-old-1080.ppm");
    resize({ 1280, 720 });
    require(compare("old-return720", old, old_words, false, true, true).pixels == old_frame.pixels,
        "Old contribution team resize changed captured copy or font"); proceed();
    open(false); finish(false);
    const Frame next_frame = compare("reopened-new", next, next_words, false, true, true,
        "contribution-team-new-720.ppm", true);
    presentation = original;
    require(compare("retained-new", next, next_words, false, true, true).pixels == next_frame.pixels,
        "Source reversal rebased reopened contribution team copy, credit or font");
    resize({ 1920, 1080 });
    compare("new-1080", next, next_words, false, true, true, "contribution-team-new-1080.ppm");
    resize({ 1280, 720 });
    require(compare("new-return720", next, next_words, false, true, true).pixels == next_frame.pixels,
        "New contribution team resize changed captured copy or font"); proceed();
    open(false); finish(false);
    require(compare("restored-default", original, default_words, false, true, true).pixels == default_frame.pixels,
        "Restored default contribution team row/badge pixels differ from the original default page"); proceed();
    open(true); compare("revealed-initial", original, default_words, true, true, false);
    tap(Key::enter); require(stack.depth() == 2, "Already-revealed contribution team skipped its remaining prompt delay");
    finish(true); compare("revealed-ready", original, default_words, true, true, true); proceed();
    require(updates == 45 && readbacks == 56 && rows == 72 && crops == 6 && openings == 5 && pops == 5 &&
        font_controls == 4 && owner_controls == 4 && records.good(),
        "Contribution team drifted from its fixed update/readback/CSV/crop/action budget");
    std::ofstream summary(output / "contribution-team-summary.json");
    summary << "{\"outcome\":\"passed\",\"scope\":\"contribution-team-only\",\"updates\":45,\"readbacks\":56,"
        "\"csv_rows\":72,\"compact_crops\":6,\"page_openings\":5,\"proceed_pops\":5,\"font_negative_controls\":4,"
        "\"owner_negative_controls\":4,\"resize_updates\":0,\"save_calls\":1,\"crop_pixels\":599508}\n";
    require(summary.good(), "Cannot retain contribution team summary");
    std::cout << "PASS Contribution team leaf: four independent complete sparse-roster rows and raw P1-P4 badge cells; "
        "device/view/paint-owner indices4/2/-1/0,0/1/2/3,4/2/7/0 retain18.7/25.0/31.2/25.0 credit; "
        "literal default/custom separators and captured contribution font through assignment/reopen/reversal, "
        "four per-row brace font and wrong-view credit controls, unchanged title/legend/score/outcome neighbors; "
        "normal/already-revealed input gates, five fresh pops, frozen Session/setup/private Save/RNG, "
        "zero-update 720/1080/720 resizing,45 updates,56 readbacks,72 CSV rows and six useful compact crops "
        "(599508 pixels /1798614 PPM bytes), one private seed Save. Both presets use the normal reader; "
        "no match/global-input/physical-pad/listening/human/performance acceptance.\n";
}
