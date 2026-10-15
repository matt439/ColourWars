// Actual EndMenuInitial pages, driven through StateContext and MenuInput. The
// fixture uses memory-only preferences and never opens the player's save.
class EndMenuProbeRoot final : public State
{
public:
    EndMenuProbeRoot(Application& app, const Canvas& canvas) : app_(app), canvas_(canvas) {}
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

Frame end_menu_frame(Application& app)
{
    app.renderer()->begin_frame();
    static_cast<StateContext&>(app).draw(*app.renderer());
    return read(*app.renderer());
}

struct EndMenuProbeCase
{
    Session session;
    const char* name;
    const wchar_t* continuation;
};

void end_menu_matrix(Application& app, const Canvas& canvas, GameTuning& tuning,
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
    settings.set_player_count(2);
    settings.begin_input_claims(input_slots::keyboard_and_mouse);
    MenuContext context(&data, &input, &settings);
    StateContext& stack = app;
    struct ClearEndStates
    {
        StateContext& stack;
        ~ClearEndStates() { this->stack.clear(); }
    } clear{ stack };

    LevelEndInfo victory;
    victory.total_area = 100.0f;
    victory.team_a_area = 60.0f;
    victory.team_b_area = 40.0f;
    LevelEndInfo draw = victory;
    draw.team_a_area = draw.team_b_area = 50.0f;
    Session single;
    single.finish_round(victory);
    Session next(3);
    next.finish_round(victory);
    Session replay(3);
    replay.finish_round(draw);
    Session complete(3);
    complete.finish_round(victory);
    complete.next_round();
    complete.finish_round(victory);
    const std::array<EndMenuProbeCase, 4> cases{ {
        { single, "single", L"Rematch" },
        { next, "series-next", L"Next Round" },
        { replay, "series-draw", L"Replay Draw" },
        { complete, "series-complete", L"Play Series Again" } } };

    const MenuStyle& style = presentation.menu();
    const EndMenuPresentation& look = presentation.end_menu();
    auto& art = const_cast<EndMenuArtPresentation&>(look.art);
    struct RestoreEndArt
    {
        EndMenuArtPresentation& art;
        EndMenuArtPresentation original;
        ~RestoreEndArt() { this->art = this->original; }
    } restore_art{ art, art };
    const RectangleF panel((menu_consts::DESIGN_RESOLUTION - look.box_size) / 2.0f,
        look.box_size);
    const auto font = app.render_resources()->resolve_sprite_font(style.item_font);

    app.keyboard()->set_focused(true);
    app.mouse()->set_focused(true);
    for (const Key key : { Key::up, Key::down, Key::left, Key::right,
        Key::enter, Key::space, Key::escape })
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
    std::optional<EndMenuAction> choice;
    const auto open = [&](const Session& session)
    {
        stack.clear();
        stack.notify_activation(true);
        input.reset_actions();
        choice.reset();
        stack.push(std::make_unique<EndMenuProbeRoot>(app, canvas));
        stack.push<EndMenuAction>(std::make_unique<EndMenuInitial>(&context, session),
            [&](const EndMenuAction& result) { choice = result; });
        tick();
        require(stack.depth() == 2 && !choice.has_value(),
            "Actual end page did not remain open on neutral input");
    };
    std::ofstream records(output / "end-menu.csv");
    records << "session,width,height,visible_choices,paper_pixels,primary_pixels,primary_text\n";
    std::vector<unsigned char> next_round_pixels;
    for (const EndMenuProbeCase& fixture : cases)
    {
        require(fixture.session.continue_string() == fixture.continuation,
            "End-menu continuation label did not match its single/series state");
        const bool ongoing = fixture.session.is_series() && !fixture.session.complete();
        const std::vector<EndMenuAction> actions = ongoing ?
            std::vector<EndMenuAction>{ EndMenuAction::restart, EndMenuAction::exit } :
            std::vector<EndMenuAction>{ EndMenuAction::restart, EndMenuAction::change_teams,
                EndMenuAction::change_weapons, EndMenuAction::change_level, EndMenuAction::exit };
        for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080),
            Vector2I(2560, 1440), Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
        {
            app.resolution_manager()->set_resolution_exactly(size);
            std::ignore = app.renderer()->window_size_changed(size.x, size.y);
            open(fixture.session);
            const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO,
                menu_consts::DESIGN_RESOLUTION), app.viewport_manager()->fullscreen_viewport());
            RectangleF frame_bounds = panel;
            frame_bounds.inflate(Vector2F(art.keyline_width + art.shadow_offset + 4.0f));
            camera.calculate_view_rectangle(frame_bounds);
            const RectangleI pixels = countdown_crop_bounds(frame_bounds);
            require(pixels.x >= 0 && pixels.y >= 0 && pixels.right() <= size.x && pixels.bottom() <= size.y,
                "End-menu paper frame escaped a supported display/aspect ratio");

            // The longest continuation must fit as real font metrics, with
            // room for the authored paper/keyline rather than clipped text.
            const Vector2F measured = app.render_resources()->measure_text(font, fixture.continuation);
            require(look.layout.spacing.y > measured.y + art.focus_padding.y * 2.0f,
                "End-menu adjacent action cards overlap at the actual font height");
            const auto heading_font = app.render_resources()->resolve_sprite_font(style.heading_font);
            const Vector2F heading_size = app.render_resources()->measure_text(heading_font,
                fixture.session.is_series() ? (fixture.session.complete() ? L"Series Complete" : L"Round Complete") :
                    L"Match Complete");
            const RectangleF heading((menu_consts::DESIGN_RESOLUTION.x - heading_size.x) / 2.0f,
                look.heading_y, heading_size.x, heading_size.y);
            require(panel.contains(heading) && heading.bottom() < look.layout.position.y - art.focus_padding.y,
                "End-menu heading overlaps the first card or escapes its panel");
            const Vector2F exit_size = app.render_resources()->measure_text(font, L"Exit to Main Menu");
            RectangleF exit(look.layout.position + Vector2F(0.0f,
                look.layout.spacing.y * (ongoing ? 1.0f : 4.0f)), exit_size);
            exit.inflate(art.focus_padding);
            require(panel.contains(exit), "End-menu last action card escapes its panel");
            RectangleF primary(look.layout.position, measured);
            primary.inflate(art.focus_padding + Vector2F(art.keyline_width + 2.0f));
            require(panel.contains(primary), "End-menu continuation text/card does not fit its paper panel");

            std::srand(5693);
            const int expected_random = std::rand();
            std::srand(5693);
            const Frame normal = end_menu_frame(app);
            if (ongoing)
            {
                // Rows below the compact continuation/Exit pair must contain
                // paper, not merely unfocusable setup labels. Compare with
                // neighbouring paper on the same scanline so the authored
                // lower-edge airbrush shading remains valid too.
                const Vector2F absent_size = app.render_resources()->measure_text(font, L"Change Weapons");
                const RectangleF absent(look.layout.position + Vector2F(0.0f, look.layout.spacing.y * 2.0f),
                    Vector2F(absent_size.x, look.layout.spacing.y * 2.0f + absent_size.y));
                const RectangleI empty_rows = countdown_crop_bounds(camera.calculate_view_rectangle(absent));
                const int sample_x = static_cast<int>(camera.calculate_view_position(
                    Vector2F((panel.x + look.layout.position.x) * 0.5f, 0.0f)).x);
                for (int y = empty_rows.y; y < empty_rows.bottom(); ++y)
                {
                    const size_t sample = (static_cast<size_t>(y) * static_cast<size_t>(normal.width) +
                        static_cast<size_t>(sample_x)) * 4;
                    for (int x = empty_rows.x; x < empty_rows.right(); ++x)
                    {
                        const size_t offset = (static_cast<size_t>(y) * static_cast<size_t>(normal.width) +
                            static_cast<size_t>(x)) * 4;
                        const bool empty = normal.pixels[offset] == normal.pixels[sample] &&
                            normal.pixels[offset + 1] == normal.pixels[sample + 1] &&
                            normal.pixels[offset + 2] == normal.pixels[sample + 2];
                        if (!empty)
                        {
                            save(normal, output / "end-menu-hidden-rows-failure.ppm");
                            std::cerr << "Hidden-row raster mismatch at " << x << ',' << y << "; row bounds " <<
                                empty_rows.x << ',' << empty_rows.y << ',' << empty_rows.width << ',' << empty_rows.height << '\n';
                        }
                        require(empty, "Ongoing series still rendered setup choices below its compact two-row menu");
                    }
                }
            }
            require(normal.pixels == end_menu_frame(app).pixels &&
                normal.pixels == end_menu_frame(app).pixels,
                "Frozen actual end page changed pixels");
            require(std::rand() == expected_random, "End-page drawing consumed random numbers");
            tick(0.5f);
            require(normal.pixels == end_menu_frame(app).pixels,
                "Neutral end-page time moved the static continuation artwork");
            play.reduce_effects = true;
            tick(0.75f);
            require(normal.pixels == end_menu_frame(app).pixels,
                "Reduce effects changed or animated the readable end-page card");
            play.reduce_effects = false;

            // Tint the real page's presentation, then observe changed raster
            // bounds. No second implementation of the artwork is drawn here.
            const Colour paper = art.paper;
            art.paper = Colour(1.0f, 0.0f, 1.0f);
            const Frame tinted_paper = end_menu_frame(app);
            art.paper = paper;
            const RectangleI paper_delta = foreground_delta_bounds(normal, tinted_paper);
            require(paper_delta.width > 0 && paper_delta.height > 0 && pixels.contains(paper_delta),
                "Actual end-page paper was absent or escaped its authored frame");
            const size_t paper_pixels = foreground_local_delta(normal, tinted_paper,
                RectangleF(Vector2F(pixels.position()), Vector2F(pixels.size())));
            require(paper_pixels > 100, "Actual end-page paper had no substantial visible fill");

            const Colour focus_fill = art.focus_fill;
            art.focus_fill = Colour(0.0f, 1.0f, 1.0f);
            const Frame tinted_primary = end_menu_frame(app);
            art.focus_fill = focus_fill;
            const RectangleI primary_delta = foreground_delta_bounds(normal, tinted_primary);
            RectangleF projected_primary = primary;
            camera.calculate_view_rectangle(projected_primary);
            const RectangleI primary_pixels = countdown_crop_bounds(projected_primary);
            require(primary_delta.width > 0 && primary_delta.height > 0 && primary_pixels.contains(primary_delta),
                "Initial focus did not highlight the continuation's actual first-row paper card");
            const size_t primary_changed = foreground_local_delta(normal, tinted_primary,
                RectangleF(Vector2F(primary_pixels.position()), Vector2F(primary_pixels.size())));
            require(primary_changed > 30, "End-menu primary focus had no visible paper fill");

            const std::string stem = std::string("end-menu-") + fixture.name + '-' +
                std::to_string(size.x) + 'x' + std::to_string(size.y);
            save(normal, output / (stem + ".ppm"));
            save(foreground_crop(normal, pixels), output / (stem + "-detail.ppm"));
            records << fixture.name << ',' << size.x << ',' << size.y << ',' << actions.size() << ',' <<
                paper_pixels << ',' << primary_changed << ',';
            for (const wchar_t letter : std::wstring_view(fixture.continuation))
            {
                records << static_cast<char>(letter);
            }
            records << '\n';
            if (size.x == 1280 && size.y == 720)
            {
                // Keep this same live page and cursor through a resize. Its
                // geometry is design-space data, recomputed at draw time.
                app.resolution_manager()->set_resolution_exactly({ 1920, 1080 });
                std::ignore = app.renderer()->window_size_changed(1920, 1080);
                std::ignore = end_menu_frame(app);
                app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
                std::ignore = app.renderer()->window_size_changed(1280, 720);
                require(normal.pixels == end_menu_frame(app).pixels && !choice.has_value(),
                    "Resizing the same actual end page changed its pixels, focus or action");
                if (fixture.session.continue_string() == L"Next Round") { next_round_pixels = normal.pixels; }
                if (fixture.session.continue_string() == L"Replay Draw")
                {
                    require(!next_round_pixels.empty() && next_round_pixels != normal.pixels,
                        "Next Round and Replay Draw rendered the same continuation label");
                }
            }
            // Fresh Enter goes through the real typed result channel. This
            // must continue the session without a preceding navigation press.
            tap(Key::enter);
            require(choice == EndMenuAction::restart && stack.depth() == 1,
                "Initial Enter on the actual end page did not continue/rematch");
        }

        // Every remaining action is reached from a newly opened actual page.
        // Ongoing sessions therefore prove the three setup choices are absent
        // from navigation: one Down must go directly to Exit.
        app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
        std::ignore = app.renderer()->window_size_changed(1280, 720);
        for (size_t selected = 1; selected < actions.size(); ++selected)
        {
            open(fixture.session);
            const Frame initial = end_menu_frame(app);
            for (size_t move = 0; move < selected; ++move) { tap(Key::down); }
            const Frame moved = end_menu_frame(app);
            require(initial.pixels != moved.pixels,
                "Actual end-page navigation did not move the visible focus treatment");
            if (selected == 1)
            {
                save(moved, output / (std::string("end-menu-") + fixture.name + "-secondary-720p.ppm"));
                const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO,
                    menu_consts::DESIGN_RESOLUTION), app.viewport_manager()->fullscreen_viewport());
                save(foreground_crop(moved, countdown_crop_bounds(camera.calculate_view_rectangle(panel))),
                    output / (std::string("end-menu-") + fixture.name + "-secondary-720p-detail.ppm"));
            }
            tap(Key::enter);
            require(choice == actions[selected] && stack.depth() == 1,
                "Actual end-page navigation returned the wrong setup/exit action");
        }
        std::cout << "PASS actual end page " << fixture.name << " initial continuation, " << actions.size() <<
            " choices, frozen/reduced rendering, 720p/1080p/QHD and aspect ratios\n";
    }
    require(records.good(), "End-menu evidence write failed");
}
