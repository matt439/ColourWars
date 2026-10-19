// Real results pages, driven through StateContext and MenuInput. Scores and
// preferences are memory-only; no player save or hardware pad is required.
class ResultsMenuProbeRoot final : public State
{
public:
    ResultsMenuProbeRoot(Application& app, const Canvas& canvas) : app_(app), canvas_(canvas) {}
    void init() override {}
    void update(float) override {}
    void draw(Renderer& renderer) const override
    {
        renderer.set_view_count(1);
        DrawList list = renderer.view(0);
        list.set_viewport(this->app_.viewport_manager()->fullscreen_viewport());
        list.set_camera(Camera::DEFAULT_CAMERA);
        list.set_filter(TextureFilter::point);
        this->canvas_.draw(list, RectangleF(Vector2F::ZERO,
            this->app_.resolution_manager()->resolution_vec()));
    }
private:
    Application& app_;
    const Canvas& canvas_;
};

Frame results_menu_frame(Application& app)
{
    app.renderer()->begin_frame();
    static_cast<StateContext&>(app).draw(*app.renderer());
    return read(*app.renderer());
}

struct ResultsMenuProbeCase
{
    LevelEndInfo info;
    Session session;
    const char* name;
    int players;
    bool revealed;
};

void results_menu_matrix(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    Save preferences;
    auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0;
    GameData data;
    data.set_application(&app);
    data.set_save(&preferences);
    data.set_tuning(&tuning);
    data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings);
    StateContext& stack = app;
    struct ClearResultsStates
    {
        StateContext& stack;
        ~ClearResultsStates() { this->stack.clear(); }
    } clear{ stack };
    const ResultsMenuPresentation& look = presentation.results_menu();
    auto& art = const_cast<MenuPaperArtPresentation&>(look.art);
    struct RestoreResultsArt
    {
        MenuPaperArtPresentation& art;
        MenuPaperArtPresentation original;
        ~RestoreResultsArt() { this->art = this->original; }
    } restore_art{ art, art };
    const MenuStyle& style = presentation.menu();
    const auto heading_font = app.render_resources()->resolve_sprite_font(style.heading_font);
    const auto item_font = app.render_resources()->resolve_sprite_font(style.item_font);
    const auto detail_font = app.render_resources()->resolve_sprite_font(style.detail_font);
    const auto contribution_font = app.render_resources()->resolve_sprite_font(look.contribution_font);
    const RectangleF panel((menu_consts::DESIGN_RESOLUTION - look.box_size) / 2.0f, look.box_size);
    const RectangleF fill_box(Vector2F((menu_consts::DESIGN_RESOLUTION.x - look.fill_box_size.x) / 2.0f,
        panel.y + look.fill_box_y_offset), look.fill_box_size);

    app.keyboard()->set_focused(true);
    app.mouse()->set_focused(true);
    for (const Key key : { Key::up, Key::down, Key::left, Key::right,
        Key::enter, Key::space, Key::escape, Key::backspace })
    {
        app.keyboard()->on_key_up(key);
    }
    app.keyboard()->poll(); app.keyboard()->poll();
    app.mouse()->poll(); app.mouse()->poll();
    const auto tick = [&](float elapsed = 0.0f)
    {
        app.keyboard()->poll();
        app.mouse()->poll();
        stack.update(elapsed);
    };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick();
        app.keyboard()->on_key_up(key); tick();
    };
    const auto resize_page = [&](Vector2I size)
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
    };
    const auto camera = [&]
    {
        return Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
    };
    const auto project = [&](RectangleF rectangle)
    {
        camera().calculate_view_rectangle(rectangle);
        return rectangle;
    };
    // Sparse/reordered devices deliberately differ from their roster positions.
    constexpr int devices[] = { input_slots::keyboard_and_mouse, 3, 0, 2 };
    const auto roster = [&](int count, bool solo_b = false)
    {
        settings.set_player_count(count);
        settings.begin_input_claims(devices[0]);
        for (int player = 1; player < count; ++player)
        {
            require(settings.claim_input_slot(devices[player]), "Cannot claim sparse results fixture roster");
        }
        for (int player = 0; player < count; ++player)
        {
            settings.set_player_team(player, solo_b || player % 2 != 0 ? PlayerTeam::b : PlayerTeam::a);
        }
    };
    const auto open = [&](const LevelEndInfo& info, const Session& session, bool revealed)
    {
        stack.clear();
        stack.notify_activation(true);
        input.reset_actions();
        stack.push(std::make_unique<ResultsMenuProbeRoot>(app, canvas));
        stack.push(std::make_unique<ResultsMenuInitial>(&context, info, session, revealed));
        tick();
        require(stack.depth() == 2, "Actual results page closed on neutral input");
    };
    const auto finish = [&]
    {
        // The real page advances phases on successive updates. Cross each
        // authored gate, then observe it on the following neutral update.
        tick(look.team_fill_delay + 0.01f);
        tick(look.team_fill_time + 0.01f);
        tick();
        tick(look.show_results_delay + 0.01f);
        tick();
        require(stack.depth() == 2, "Results timing selected an action without input");
    };
    const auto centered = [&](const auto& font, const std::wstring& text, float y)
    {
        const Vector2F measured = app.render_resources()->measure_text(font, text);
        return RectangleF((menu_consts::DESIGN_RESOLUTION.x - measured.x) / 2.0f,
            y, measured.x, measured.y);
    };
    const auto contribution = [&](const LevelEndInfo& info, int player)
    {
        const auto players = settings.player_settings();
        const MenuPlayerSettings& entry = players.at(static_cast<size_t>(player));
        constexpr std::array<const wchar_t*, 5> devices{
            L"Pad 1", L"Pad 2", L"Pad 3", L"Pad 4", L"Keyboard/mouse" };
        const std::wstring device = entry.num < 0 ? L"Waiting to join" : devices.at(static_cast<size_t>(entry.num));
        const std::wstring text = L"P" + std::to_wstring(player + 1) + L" - " + device +
            (entry.team == PlayerTeam::a ? L" | Team A | " : L" | Team B | ") +
            info.player_percentage_string(paint_owners::index(entry.num, player));
        return RectangleF(look.contribution_position + Vector2F(0.0f,
            look.contribution_spacing * static_cast<float>(player + 1)),
            app.render_resources()->measure_text(contribution_font, text));
    };
    const auto percentage = [&](const LevelEndInfo& info, bool team_b)
    {
        const std::wstring text = team_b ? info.team_b_percentage_string() : info.team_a_percentage_string();
        const Vector2F measured = app.render_resources()->measure_text(detail_font, text);
        return RectangleF(Vector2F(team_b ? fill_box.right() - look.team_fill_border.x - measured.x :
            fill_box.x + look.team_fill_border.x, fill_box.y + look.team_fill_border.y), measured);
    };

    LevelEndInfo victory;
    victory.total_area = 100.0f;
    victory.team_a_area = 60.0f;
    victory.team_b_area = 40.0f;
    victory.team_colours = TeamColourTools::team_colours(TeamColourPair::blue_orange,
        presentation.team_palette());
    victory.player_areas[devices[0]] = 11.0f;
    victory.player_areas[devices[1]] = 22.0f;
    victory.player_areas[devices[2]] = 33.0f;
    victory.player_areas[devices[3]] = 34.0f;
    LevelEndInfo draw = victory;
    draw.team_a_area = draw.team_b_area = 50.0f;
    LevelEndInfo solo_a = victory;
    solo_a.team_a_area = 37.0f;
    solo_a.team_b_area = 0.0f;
    solo_a.team_b_played = false;
    solo_a.player_areas[devices[0]] = 37.0f;
    LevelEndInfo solo_b = solo_a;
    solo_b.team_a_played = false;
    solo_b.team_b_played = true;
    solo_b.team_a_area = 0.0f;
    solo_b.team_b_area = 37.0f;
    Session single;
    single.finish_round(victory);
    Session next(3);
    next.finish_round(victory);
    Session complete(3);
    complete.finish_round(victory);
    complete.next_round();
    complete.finish_round(victory);
    const std::array<ResultsMenuProbeCase, 6> cases{ {
        { victory, single, "victory-revealed", 2, true },
        { draw, single, "draw-revealed", 2, true },
        { victory, next, "series-next-revealed", 4, true },
        { victory, complete, "series-complete-revealed", 4, true },
        { solo_a, single, "solo-a-plain", 1, false },
        { solo_b, single, "solo-b-plain", 1, false } } };

    std::ofstream records(output / "results-menu.csv");
    records << "case,width,height,players,paper_pixels\n";
    for (const ResultsMenuProbeCase& fixture : cases)
    {
        roster(fixture.players, !fixture.info.team_a_played);
        const RectangleF heading = centered(heading_font, fixture.session.is_series() ?
            L"Round " + std::to_wstring(fixture.session.round()) : L"Results", look.heading_y);
        if (!panel.contains(heading) || heading.bottom() >= look.winner_y)
        {
            std::cerr << "Results heading bounds=" << heading.x << ',' << heading.y << ',' <<
                heading.width << ',' << heading.height << " winner_y=" << look.winner_y << '\n';
        }
        require(panel.contains(heading) && heading.bottom() < look.winner_y,
            "Results heading escapes the paper or overlaps the winner line");
        const RectangleF winner = centered(item_font, fixture.info.winning_team_string(presentation.menu_text().results), look.winner_y);
        require(panel.contains(winner) && winner.bottom() < look.session_score_y,
            "Results winner/coverage line escapes the paper or overlaps the series ledger");
        if (fixture.session.is_series())
        {
            const RectangleF score = centered(detail_font, fixture.session.score_string(presentation.menu_text().results), look.session_score_y);
            const RectangleF result = centered(detail_font, fixture.session.result_string(presentation.menu_text().results), look.session_result_y);
            require(panel.contains(score) && panel.contains(result) && score.bottom() < result.y &&
                result.bottom() < fill_box.y - art.focus_padding.y,
                "Results series ledger overlaps itself, the scoring plates or the paper bounds");
        }
        const RectangleF proceed = centered(detail_font, L"Press A/Enter to proceed", look.proceed_y);
        require(panel.contains(proceed), "Results continuation prompt escapes the paper");
        for (const bool team_b : { false, true })
        {
            if (team_b ? !fixture.info.team_b_played : !fixture.info.team_a_played) { continue; }
            RectangleF plate = percentage(fixture.info, team_b);
            plate.inflate(art.focus_padding);
            require(panel.contains(plate) && plate.bottom() < look.contribution_position.y,
                "Results percentage paper escapes the panel or overlaps contributions");
        }
        const Vector2F label_size = app.render_resources()->measure_text(contribution_font,
            L"Surface held (% of arena)");
        require(panel.contains(RectangleF(look.contribution_position, label_size)) &&
            label_size.y < look.contribution_spacing,
            "Results contribution heading does not fit or overlaps its first row");
        for (int player = 0; player < fixture.players; ++player)
        {
            const RectangleF row = contribution(fixture.info, player);
            require(panel.contains(row) && row.height < look.contribution_spacing && row.bottom() < proceed.y,
                "Results contribution rows overlap, escape paper or collide with continuation");
            require(panel.contains(UiPlayerBadge::before(row, presentation.player_identity())),
                "Results roster badge escapes its paper");
        }
        for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080),
            Vector2I(2560, 1440), Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
        {
            resize_page(size);
            open(fixture.info, fixture.session, fixture.revealed);
            tap(Key::enter);
            require(stack.depth() == 2, "Results continued before its authored delay elapsed");
            finish();
            tap(Key::escape);
            require(stack.depth() == 2, "Escape bypassed actual results continuation");
            const Frame normal = results_menu_frame(app);
            std::srand(6197);
            const int expected_random = std::rand();
            std::srand(6197);
            require(normal.pixels == results_menu_frame(app).pixels &&
                normal.pixels == results_menu_frame(app).pixels && std::rand() == expected_random,
                "Frozen results drawing changed pixels or consumed gameplay randomness");
            tick(0.75f);
            require(normal.pixels == results_menu_frame(app).pixels,
                "Completed results artwork moved under neutral time");
            play.reduce_effects = true;
            tick(0.75f);
            require(normal.pixels == results_menu_frame(app).pixels,
                "Reduce effects changed the readable results artwork");
            play.reduce_effects = false;
            RectangleF bounds = panel;
            bounds.inflate(Vector2F(art.keyline_width + art.shadow_offset + 4.0f));
            const RectangleI pixels = countdown_crop_bounds(project(bounds));
            require(pixels.x >= 0 && pixels.y >= 0 && pixels.right() <= size.x && pixels.bottom() <= size.y,
                "Results paper escaped a supported display/aspect ratio");
            const Colour original = art.paper;
            art.paper = Colour(1.0f, 0.0f, 1.0f);
            const Frame tinted = results_menu_frame(app);
            art.paper = original;
            const RectangleI changed = foreground_delta_bounds(normal, tinted);
            require(changed.width > 0 && changed.height > 0 && pixels.contains(changed),
                "Actual results paper was absent or escaped its frame");
            const size_t paper_pixels = foreground_local_delta(normal, tinted,
                RectangleF(Vector2F(pixels.position()), Vector2F(pixels.size())));
            require(paper_pixels > 100, "Actual results paper has no substantial visible fill");
            hud_text_contrast(normal, project(heading));
            for (const bool team_b : { false, true })
            {
                if (team_b ? !fixture.info.team_b_played : !fixture.info.team_a_played) { continue; }
                hud_text_contrast(normal, project(percentage(fixture.info, team_b)));
            }
            for (int player = 0; player < fixture.players; ++player)
            {
                hud_text_contrast(normal, project(contribution(fixture.info, player)));
            }
            const std::string stem = std::string("results-menu-") + fixture.name + '-' +
                std::to_string(size.x) + 'x' + std::to_string(size.y);
            save(normal, output / (stem + ".ppm"));
            save(foreground_crop(normal, pixels), output / (stem + "-detail.ppm"));
            records << fixture.name << ',' << size.x << ',' << size.y << ',' << fixture.players << ',' <<
                paper_pixels << '\n';
            resize_page(size.x == 1920 && size.y == 1080 ? Vector2I(1280, 720) : Vector2I(1920, 1080));
            std::ignore = results_menu_frame(app);
            resize_page(size);
            require(normal.pixels == results_menu_frame(app).pixels && stack.depth() == 2,
                "Resizing the same results page changed its pixels or selected an action");
            tap(Key::enter);
            require(stack.depth() == 1, "Fresh Enter did not proceed from the actual results page");
        }
    }

    // Change only one input owner's credit. Its displayed row must change;
    // every other row and the team score must remain byte-identical.
    resize_page({ 1280, 720 });
    roster(2);
    open(victory, single, false);
    const Frame before_fill = results_menu_frame(app);
    tick(look.team_fill_delay + 0.01f);
    tick(look.team_fill_time * 0.5f);
    tick();
    const Frame half_fill = results_menu_frame(app);
    const RectangleF scoring_track(fill_box.position() + look.team_fill_border, look.team_fill_size);
    const RectangleI scoring_pixels = countdown_crop_bounds(project(scoring_track));
    require(foreground_local_delta(before_fill, half_fill,
        RectangleF(Vector2F(scoring_pixels.position()), Vector2F(scoring_pixels.size()))) > 100,
        "Plain results fill failed to advance independently of its hidden numbers/details");
    save(half_fill, output / "results-menu-half-fill-720p.ppm");
    LevelEndInfo unpainted = draw;
    unpainted.team_a_area = unpainted.team_b_area = 0.0f;
    unpainted.player_areas = {};
    open(unpainted, single, false); finish();
    const Frame unpainted_draw = results_menu_frame(app);
    LevelEndInfo even = unpainted;
    even.team_a_area = even.team_b_area = 50.0f;
    open(even, single, false); finish();
    require(unpainted_draw.pixels == results_menu_frame(app).pixels,
        "Unpainted competitive results lost their deliberate even draw presentation");

    roster(4);
    open(victory, next, true); finish();
    const Frame credited = results_menu_frame(app);
    for (int player = 0; player < 4; ++player)
    {
        LevelEndInfo changed = victory;
        changed.player_areas[devices[player]] = 9.0f;
        open(changed, next, true); finish();
        const Frame revised = results_menu_frame(app);
        RectangleF row = contribution(victory, player);
        const RectangleF shorter = contribution(changed, player);
        row.width = std::max(row.width, shorter.width);
        row.inflate(Vector2F(2.0f));
        const RectangleI delta = foreground_delta_bounds(credited, revised);
        const RectangleI row_pixels = countdown_crop_bounds(project(row));
        require(delta.width > 0 && delta.height > 0 && row_pixels.contains(delta),
            "Sparse input-owner credit altered pixels outside its own results row");
        require(foreground_local_delta(credited, revised, project(row)) > 5,
            "Sparse input-owner paint credit changed the wrong results row or was not rendered");
    }

    // Team-colour changes reveal only the bars. A revealed page has no second
    // fill animation, and an absent solo team has no bar even after time-up.
    for (const bool revealed : { false, true })
    {
        roster(2);
        for (const TeamColourPair palette : { TeamColourPair::blue_orange,
            TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            LevelEndInfo info = victory;
            info.team_colours = TeamColourTools::team_colours(palette, presentation.team_palette());
            open(info, single, revealed); finish();
            const Frame normal = results_menu_frame(app);
            LevelEndInfo changed = info;
            changed.team_colours = TeamColour(Colour(1.0f, 0.0f, 1.0f), Colour(0.0f, 1.0f, 1.0f));
            open(changed, single, revealed); finish();
            const Frame recoloured = results_menu_frame(app);
            if (revealed)
            {
                require(normal.pixels == recoloured.pixels,
                    "Already-revealed results rendered a second team-coloured score bar");
            }
            else
            {
                const RectangleF track(fill_box.position() + look.team_fill_border, look.team_fill_size);
                const RectangleI track_pixels = countdown_crop_bounds(project(track));
                require(foreground_local_delta(normal, recoloured,
                    RectangleF(Vector2F(track_pixels.position()), Vector2F(track_pixels.size()))) > 100,
                    "Plain results score bars disappeared or escaped the true scoring track");
                changed = info;
                changed.team_colours.team_a = Colour(1.0f, 0.0f, 1.0f);
                open(changed, single, false); finish();
                const RectangleI team_a_pixels = foreground_delta_bounds(normal, results_menu_frame(app));
                const RectangleF expected = project(RectangleF(track.position(),
                    Vector2F(track.width * info.team_a_ratio(), track.height)));
                require(team_a_pixels.width > 0 && std::abs(static_cast<float>(team_a_pixels.x) - expected.x) < 2.0f &&
                    std::abs(static_cast<float>(team_a_pixels.right()) - expected.right()) < 2.0f,
                    "Competitive results bar no longer shows its share of painted surface");
            }
            const std::string stem = std::string("results-menu-palette-") + TeamColourTools::palette_key(palette) +
                (revealed ? "-revealed" : "-plain");
            save(normal, output / (stem + "-720p.ppm"));
        }
    }
    for (const bool team_b : { false, true })
    {
        roster(1, team_b);
        const LevelEndInfo info = team_b ? solo_b : solo_a;
        open(info, single, false); finish();
        const Frame normal = results_menu_frame(app);
        const RectangleF track(fill_box.position() + look.team_fill_border, look.team_fill_size);
        // The unused side is neutral, including near its edge. Linear
        // sampling of the one-texel frame leaks adjacent atlas colours here.
        const Vector2F sample = camera().calculate_view_position(Vector2F(
            team_b ? track.x + 4.0f : track.right() - 4.0f, track.bottom() - 4.0f));
        const size_t offset = hud_pixel_offset(normal, static_cast<int>(sample.x), static_cast<int>(sample.y));
        const Colour expected = look.track_colour;
        require(std::abs(static_cast<float>(normal.pixels[offset]) - expected.r * 255.0f) <= 1.0f &&
            std::abs(static_cast<float>(normal.pixels[offset + 1]) - expected.g * 255.0f) <= 1.0f &&
            std::abs(static_cast<float>(normal.pixels[offset + 2]) - expected.b * 255.0f) <= 1.0f,
            "Solo unpainted track picked up neighbouring atlas colours");
        LevelEndInfo changed = info;
        if (team_b) { changed.team_colours.team_a = Colour(1.0f, 0.0f, 1.0f); }
        else { changed.team_colours.team_b = Colour(1.0f, 0.0f, 1.0f); }
        open(changed, single, false); finish();
        require(normal.pixels == results_menu_frame(app).pixels,
            "Solo results rendered an absent team's score bar");
        changed = info;
        if (team_b) { changed.team_colours.team_b = Colour(1.0f, 0.0f, 1.0f); }
        else { changed.team_colours.team_a = Colour(1.0f, 0.0f, 1.0f); }
        open(changed, single, false); finish();
        const RectangleI painted = foreground_delta_bounds(normal, results_menu_frame(app));
        const float expected_width = camera().scale * look.team_fill_size.x * info.coverage_ratio();
        require(painted.width > 0 && std::abs(static_cast<float>(painted.width) - expected_width) < 2.0f,
            "Solo results bar no longer shows actual arena coverage");
    }
    // The stage record: one contribution-font line right-aligned to the score
    // bar on the contribution heading's row, hidden until the details reveal.
    {
        const MenuRecordText& words = presentation.menu_text().records;
        RecordOutcome best; best.recorded = true; best.new_best = true; best.value = best.best = 60.0f;
        RecordOutcome held; held.recorded = true; held.value = 51.0f; held.best = 72.15f;
        RecordOutcome first_draw; first_draw.recorded = true;
        const auto shown = [&](const RecordOutcome& record, bool revealed_details)
        {
            stack.clear();
            stack.notify_activation(true);
            input.reset_actions();
            stack.push(std::make_unique<ResultsMenuProbeRoot>(app, canvas));
            stack.push(std::make_unique<ResultsMenuInitial>(&context, victory, single, false, record));
            tick();
            if (revealed_details) { finish(); }
            return results_menu_frame(app);
        };
        const RectangleF proceed = centered(detail_font, L"Press A/Enter to proceed", look.proceed_y);
        for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080) })
        {
            resize_page(size);
            roster(4);
            const Frame plain = shown(RecordOutcome(), true);
            const Frame hidden = shown(RecordOutcome(), false);
            const std::pair<const RecordOutcome*, std::wstring> lines[]{
                { &best, words.new_best + L" 60.0%" }, { &held, words.stage_best + L" 72.1%" } };
            for (const auto& [record, line] : lines)
            {
                const Vector2F measured = app.render_resources()->measure_text(contribution_font, line);
                const RectangleF box(Vector2F(fill_box.right() - measured.x, look.record_y), measured);
                require(panel.contains(box) && box.y >= fill_box.bottom() && box.bottom() < proceed.y,
                    "Results record line escapes the paper, the score bar or the continuation prompt");
                for (int player = 0; player < 4; ++player)
                {
                    require(!box.intersects(contribution(victory, player)) &&
                        !box.intersects(UiPlayerBadge::before(contribution(victory, player), presentation.player_identity())),
                        "Results record line overlaps a contribution row or its badge");
                }
                const Frame with = shown(*record, true);
                RectangleF area = box;
                area.inflate(Vector2F(2.0f));
                const RectangleI delta = foreground_delta_bounds(with, plain);
                require(delta.width > 0 && delta.height > 0 && countdown_crop_bounds(project(area)).contains(delta),
                    "Results record line was absent or drew outside its right-aligned row");
                hud_text_contrast(with, project(box));
                require(shown(*record, false).pixels == hidden.pixels,
                    "Results record line showed before the details were revealed");
                save(with, output / ("results-menu-record-" + std::string(record == &best ? "new-best" : "stage-best") + '-' +
                    std::to_string(size.x) + 'x' + std::to_string(size.y) + ".ppm"));
            }
            require(shown(first_draw, true).pixels == plain.pixels,
                "A recorded result with no stage best drew a record line");
        }
        stack.clear();
    }
    require(records.good(), "Results-menu evidence write failed");
    std::cout << "PASS actual results: winner/draw, series ledger, solo coverage, sparse input-owner credit, "
        "timed continuation, revealed/plain score bars, palettes, frozen/reduced redraw, resize and "
        "720p/1080p/QHD/4:3/16:10/ultrawide paper bounds\n";
}
