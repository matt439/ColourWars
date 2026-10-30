// The fixed headings are independent of both MenuText and Session wording.
// Compare complete measured text bounds over a full stepped-paper reference.
void end_heading_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    using Headings = decltype(MenuText{}.end);
    const Headings legacy{ L"Match Complete", L"Round Complete", L"Series Complete" };
    const Presentation original = presentation;
    Save preferences; auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0;
    GameData data; data.set_application(&app); data.set_save(&preferences);
    data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MatchSetup settings;
    settings.set_player_count(2); settings.begin_input_claims(input_slots::keyboard_and_mouse);
    MenuContext context(&data, &input, &settings); StateContext& stack = app;
    struct RestoreEndHeadings
    {
        StateContext& stack; MenuContext& context; Presentation& presentation; Presentation original;
        ~RestoreEndHeadings()
        {
            this->context.cancel_music_exit(); script_menu_music(this->context, {});
            this->stack.clear(); this->presentation = this->original;
        }
    } restore{ stack, context, presentation, original };
    LevelEndInfo victory; victory.total_area = 100.0f; victory.team_a_area = 60.0f; victory.team_b_area = 40.0f;
    LevelEndInfo draw = victory; draw.team_a_area = draw.team_b_area = 50.0f;
    Session single; single.finish_round(victory);
    Session next_round(3); next_round.finish_round(victory);
    Session replay(3); replay.finish_round(draw);
    Session complete(3); complete.finish_round(victory); complete.next_round(); complete.finish_round(victory);
    struct HeadingProbeCase
    {
        Session session;
        const char* name;
        const wchar_t* continuation;
        std::wstring Headings::* heading_field;
    };
    const std::array<HeadingProbeCase, 4> cases{ {
        { single, "single", L"Rematch", &Headings::match_complete },
        { next_round, "series-next", L"Next Round", &Headings::round_complete },
        { replay, "series-draw", L"Replay Draw", &Headings::round_complete },
        { complete, "series-complete", L"Play Series Again", &Headings::series_complete } } };
    const auto tick = [&](float elapsed = 0.0f)
        { app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed); };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick();
        // This historical raster fixture samples the settled ornament pose.
        if (key == Key::up || key == Key::down || key == Key::left || key == Key::right)
        { app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.51f); }
    };
    std::optional<EndMenuAction> choice; int results = 0;
    const auto open = [&](const HeadingProbeCase& fixture)
    {
        stack.clear(); context.cancel_music_exit(); script_menu_music(context, {});
        stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::up, Key::down, Key::left, Key::right, Key::enter, Key::space, Key::escape })
            { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        choice.reset(); results = 0; stack.push(std::make_unique<EndMenuProbeRoot>(app, canvas));
        stack.push<EndMenuAction>(std::make_unique<EndMenuInitial>(&context, fixture.session),
            [&](const EndMenuAction& result) { choice = result; ++results; });
        tick(); tick();
        require(stack.depth() == 2 && !choice && !context.music_exit_pending(),
            "End heading page consumed neutral input or retained an earlier exit");
    };
    std::ofstream records(output / "end-menu-headings.csv");
    records << "case,session,width,height,font,raw_width,raw_height,scale,fixture_matches\n";
    const auto compare = [&](const std::string& name, const HeadingProbeCase& fixture,
        const Presentation& snapshot, const Headings& words)
    {
        const auto& style = snapshot.menu(); const auto& look = snapshot.end_menu(); const auto& art = look.art;
        const std::wstring& text = words.*fixture.heading_field;
        const Vector2F raw = app.render_resources()->measure_text(
            app.render_resources()->resolve_sprite_font(style.heading_font), text);
        UiText expected("reference-end-heading", text, style.heading_font,
            Vector2F((menu_consts::DESIGN_RESOLUTION.x - raw.x) / 2.0f, look.heading_y),
            app.render_resources(), art.ink);
        const RectangleF panel((menu_consts::DESIGN_RESOLUTION - look.box_size) / 2.0f, look.box_size);
        require(expected.bounds().size() == raw && panel.contains(expected.bounds()) &&
            expected.bounds().bottom() <= look.layout.position.y - art.focus_padding.y,
            "End heading reference changed its unscaled paper containment or reached its primary action");
        const Frame actual = end_menu_frame(app);
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app.viewport_manager()->fullscreen_viewport());
        list.set_camera(camera); list.set_filter(TextureFilter::point);
        canvas.draw(list, camera.visible_rectangle(app.viewport_manager()->fullscreen_viewport()));
        const SpriteSheet* sheet = app.render_resources()->sprite_sheet(style.sprite_sheet);
        const auto solid = sheet->resolve_sprite_frame(style.fill_frame);
        const auto fill = [&](const RectangleF& bounds, const Colour& colour)
        {
            sheet->draw(list, solid, bounds,
                Colour(colour.r * colour.a, colour.g * colour.a, colour.b * colour.a, colour.a));
        };
        const float pen = art.keyline_width, step = pen * 2.0f, shadow = art.shadow_offset;
        const auto stepped = [&](const RectangleF& bounds, const Colour& colour)
        {
            fill(RectangleF(bounds.x + step, bounds.y, bounds.width - step * 2.0f, bounds.height), colour);
            fill(RectangleF(bounds.x, bounds.y + step, bounds.width, bounds.height - step * 2.0f), colour);
        };
        const RectangleF face(panel.x, panel.y, panel.width - shadow, panel.height - shadow);
        stepped(RectangleF(face.position() + Vector2F(shadow), face.size()), art.shade); stepped(face, art.ink);
        const RectangleF inside(face.x + pen, face.y + pen, face.width - pen * 2.0f, face.height - pen * 2.0f);
        stepped(inside, art.paper);
        for (int pass = 3; pass > 0; --pass)
        {
            Colour shade = art.shade; shade.a *= 0.12f; const float width = pen * static_cast<float>(pass);
            fill(RectangleF(inside.x + step, inside.bottom() - width, inside.width - step * 2.0f, width), shade);
            fill(RectangleF(inside.right() - width, inside.y + step, width, inside.height - step * 2.0f), shade);
        }
        // The production paper is first in End's text container and leaves
        // point sampling active for the following heading and action labels.
        expected.draw(list);
        const Frame reference = read(renderer);
        RectangleF bounds = expected.bounds(); bounds.inflate(Vector2F(2.0f)); camera.calculate_view_rectangle(bounds);
        const RectangleI pixels = countdown_crop_bounds(bounds);
        const Frame observed = foreground_crop(actual, pixels, 0), wanted = foreground_crop(reference, pixels, 0);
        const std::string stem = "end-menu-heading-" + name + '-' + fixture.name + '-' +
            std::to_string(actual.width) + 'x' + std::to_string(actual.height);
        if (observed.pixels != wanted.pixels)
        {
            save(observed, output / (stem + "-failure-actual.ppm"));
            save(wanted, output / (stem + "-failure-expected.ppm"));
            const RectangleI delta = foreground_delta_bounds(observed, wanted);
            std::cerr << "End heading case=" << name << " session=" << fixture.name << " crop=(" << pixels.x << ',' <<
                pixels.y << ',' << pixels.width << ',' << pixels.height << ") delta=(" << delta.x << ',' << delta.y <<
                ',' << delta.width << ',' << delta.height << ")\n";
            throw std::runtime_error("Actual End heading differs from its independent complete-bounds paper fixture");
        }
        save(observed, output / (stem + "-detail.ppm"));
        records << name << ',' << fixture.name << ',' << actual.width << ',' << actual.height << ',' <<
            style.heading_font << ',' << raw.x << ',' << raw.y << ",1,true\n";
        std::srand(8231); const int next_random = std::rand(); std::srand(8231);
        require(actual.pixels == end_menu_frame(app).pixels && std::rand() == next_random,
            "End heading drawing changed frozen pixels or consumed random numbers");
        return actual;
    };
    std::array<Frame, 4> baseline{};
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440),
        Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        for (size_t index = 0; index < cases.size(); ++index)
        {
            open(cases[index]); const Frame actual = compare("default", cases[index], original, legacy);
            if (size.x == 1280) { baseline[index] = actual; }
            tap(Key::enter); require(choice == EndMenuAction::restart && results == 1 && stack.depth() == 1,
                "Default End heading changed fresh continuation behavior");
        }
    }
    const Headings old_words{ L"Match finished~", L"Round ended~", L"Series over~" };
    const Headings new_words{ L"Paint match done", L"Round is over", L"Series finished" };
    const auto with_words = [&](const Headings& words)
    {
        Presentation result = original;
        auto& text = const_cast<MenuText&>(result.menu_text()).end;
        text.match_complete = words.match_complete;
        text.round_complete = words.round_complete;
        text.series_complete = words.series_complete;
        return result;
    };
    Presentation old = with_words(old_words); old.validate_resources(*app.render_resources(), *app.audio_resources());
    const Font* source = app.render_resources()->font(
        app.render_resources()->resolve_sprite_font(original.menu().heading_font));
    std::vector<Glyph> glyphs;
    for (char32_t character = U' '; character < U'~'; ++character)
    {
        const Glyph* glyph = source->find(character);
        require(glyph != nullptr, "End heading replacement font lacks a baseline printable glyph"); glyphs.push_back(*glyph);
    }
    const std::string font_name = "acceptance_end_headings_without_tilde";
    app.render_resources()->add_font(font_name,
        std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
    require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(font_name), L"~"),
        "End heading replacement font still contains the old heading glyph");
    Presentation next = with_words(new_words); const_cast<MenuStyle&>(next.menu()).heading_font = font_name;
    next.validate_resources(*app.render_resources(), *app.audio_resources());
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    for (size_t index = 0; index < cases.size(); ++index)
    {
        const auto& fixture = cases[index]; const Session& session = fixture.session;
        const bool ongoing = session.is_series() && !session.complete();
        const std::wstring score = session.score_string(original.menu_text().results), result = session.result_string(original.menu_text().results);
        const int round = session.round(), team_a_wins = session.team_a_wins(), team_b_wins = session.team_b_wins();
        const auto measured_width = [&](const Headings& words)
        {
            return app.render_resources()->measure_text(app.render_resources()->resolve_sprite_font(
                original.menu().heading_font), words.*fixture.heading_field).x;
        };
        require(measured_width(old_words) != measured_width(new_words),
            "Custom End heading fixture must change its centered width");
        presentation = old; open(fixture); const Frame retained = compare("custom-old", fixture, old, old_words);
        presentation = next; tick(); tap(Key::down); tap(Key::up);
        require(retained.pixels == end_menu_frame(app).pixels,
            "Live End heading/font replacement or focus movement changed an existing page");
        compare("retained-old", fixture, old, old_words);
        int music_updates = 0; script_menu_music(context, [&](float) { ++music_updates; });
        tap(Key::enter);
        require(context.music_exit_pending() && !choice && results == 0 && stack.depth() == 2 && music_updates == 1,
            "End heading continuation bypassed its owner-controlled music exit");
        tap(Key::down); tap(Key::enter); tap(Key::escape); tick(0.1f);
        require(context.music_exit_pending() && !choice && retained.pixels == end_menu_frame(app).pixels,
            "Pending End exit accepted another action, changed focus or rebased the heading");
        presentation = original; app.keyboard()->on_key_down(Key::enter); tick();
        static_cast<WindowNotify&>(app).on_deactivated(); tick();
        static_cast<WindowNotify&>(app).on_activated(); tick();
        app.keyboard()->on_key_up(Key::enter); tick(); tick();
        require(context.music_exit_pending() && !choice && retained.pixels == end_menu_frame(app).pixels,
            "Pending End music exit or focus interruption rebased its captured heading");
        compare("pending-music-focus-return", fixture, old, old_words);
        context.complete_music_exit(); context.complete_music_exit();
        require(choice == EndMenuAction::restart && results == 1 && stack.depth() == 1 && !context.music_exit_pending(),
            "End heading pending music completion lost or duplicated its continuation action");

        presentation = next; open(fixture);
        for (int move = 0; move < (ongoing ? 3 : 4); ++move) { tap(Key::down); }
        const Frame adopted = compare("reopened-new-font-exit", fixture, next, new_words);
        presentation = old; tick(); tap(Key::up); tap(Key::down);
        require(adopted.pixels == end_menu_frame(app).pixels,
            "End heading/font reversal or focus movement changed an already reopened page");
        compare("retained-new-after-reversal", fixture, next, new_words);
        app.keyboard()->on_key_down(Key::enter); tick();
        static_cast<WindowNotify&>(app).on_deactivated(); tick();
        static_cast<WindowNotify&>(app).on_activated(); tick();
        app.keyboard()->on_key_up(Key::enter); tick(); tick();
        require(!choice && results == 0 && stack.depth() == 2 && !context.music_exit_pending() &&
            adopted.pixels == end_menu_frame(app).pixels,
            "Interrupted End Enter selected Exit or rebased its captured heading");
        compare("focus-return-held-release", fixture, next, new_words); tap(Key::enter);
        require(choice == EndMenuAction::exit && results == 1 && stack.depth() == 1,
            "Custom End heading or focus interruption changed a fresh Exit action");
        presentation = original; open(fixture);
        require(compare("restored-default", fixture, original, legacy).pixels == baseline[index].pixels,
            "End heading regression failed to restore the exact original session raster");
        tap(Key::enter); require(choice == EndMenuAction::restart && results == 1 && stack.depth() == 1,
            "Repeated restored End page changed fresh continuation");
        require(session.continue_string(original.menu_text().end) == fixture.continuation && session.score_string(original.menu_text().results) == score &&
            session.result_string(original.menu_text().results) == result && session.round() == round && session.team_a_wins() == team_a_wins &&
            session.team_b_wins() == team_b_wins,
            "Authored End headings or repeated typed actions mutated the Session ledger");
    }
    require(settings.player_count() == 2 && settings.player_settings()[0].num == input_slots::keyboard_and_mouse,
        "End heading fixture mutated the match setup");
    require(records.good(), "End heading evidence write failed");
    std::cout << "PASS actual End headings: three independent legacy strings across four Session contexts and six "
        "display sizes, unscaled full-paper centering, custom widths/heading font, retained/reopened/reversed "
        "snapshots, owner-controlled pending music exit, interrupted Enter and fresh repeated typed actions, "
        "unchanged Session ledger and frozen draw RNG\n";
}
