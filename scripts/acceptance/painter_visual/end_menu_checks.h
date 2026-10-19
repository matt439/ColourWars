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

// Draw independent historical wording against the actual EndMenuInitial.
// The complete label crops retain the stepped cards' lower bevel, including
// the continuation card that remains present after moving the cursor.
void end_menu_item_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    using Labels = std::array<std::wstring, 4>;
    const Labels legacy{ L"Change Teams", L"Change Weapons", L"Change Level", L"Exit to Main Menu" };
    constexpr std::array<MenuItem, 4> roles{ MenuItem::change_teams, MenuItem::change_weapons,
        MenuItem::change_level, MenuItem::exit_to_main_menu };
    const Presentation original = presentation;
    Save preferences;
    auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0;
    GameData data;
    data.set_application(&app); data.set_save(&preferences); data.set_tuning(&tuning);
    data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MenuLevelSettings settings;
    settings.set_player_count(2); settings.begin_input_claims(input_slots::keyboard_and_mouse);
    MenuContext context(&data, &input, &settings);
    StateContext& stack = app;
    struct RestoreEndItems
    {
        StateContext& stack;
        Presentation& presentation;
        Presentation original;
        ~RestoreEndItems() { this->stack.clear(); this->presentation = this->original; }
    } restore{ stack, presentation, original };
    LevelEndInfo victory;
    victory.total_area = 100.0f; victory.team_a_area = 60.0f; victory.team_b_area = 40.0f;
    LevelEndInfo draw = victory; draw.team_a_area = draw.team_b_area = 50.0f;
    Session single; single.finish_round(victory);
    Session next_round(3); next_round.finish_round(victory);
    Session replay(3); replay.finish_round(draw);
    Session complete(3); complete.finish_round(victory); complete.next_round(); complete.finish_round(victory);
    const std::array<EndMenuProbeCase, 4> cases{ {
        { single, "single", L"Rematch" }, { next_round, "series-next", L"Next Round" },
        { replay, "series-draw", L"Replay Draw" }, { complete, "series-complete", L"Play Series Again" } } };
    const auto tick = [&](float elapsed = 0.0f)
    {
        app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed);
    };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick();
        // This historical raster fixture samples the settled ornament pose.
        if (key == Key::up || key == Key::down || key == Key::left || key == Key::right)
        { app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.51f); }
    };
    std::optional<EndMenuAction> choice;
    int results = 0;
    const auto open = [&](const EndMenuProbeCase& fixture)
    {
        stack.clear(); context.cancel_music_exit(); context.set_music_update({});
        stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::up, Key::down, Key::left, Key::right, Key::enter,
            Key::space, Key::escape }) { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll();
        input.reset_actions(); choice.reset(); results = 0;
        stack.push(std::make_unique<EndMenuProbeRoot>(app, canvas));
        stack.push<EndMenuAction>(std::make_unique<EndMenuInitial>(&context, fixture.session),
            [&](const EndMenuAction& result) { choice = result; ++results; });
        tick(); tick();
        require(stack.depth() == 2 && !choice && !context.music_exit_pending(),
            "End item page consumed neutral entry input or retained a previous exit");
    };
    std::ofstream records(output / "end-menu-items.csv");
    records << "case,session,focus,item,row,width,height,fixture_matches\n";
    const auto compare = [&](const std::string& name, const EndMenuProbeCase& fixture, int focus,
        const Presentation& snapshot, const Labels& labels)
    {
        const auto& style = snapshot.menu(); const auto& look = snapshot.end_menu(); const auto& art = look.art;
        const bool ongoing = fixture.session.is_series() && !fixture.session.complete();
        const Frame actual = end_menu_frame(app);
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        std::vector<std::unique_ptr<UiText>> rows;
        rows.push_back(std::make_unique<UiText>("reference-continuation", fixture.continuation, style.item_font,
            look.layout.position, app.render_resources(), focus == 0 ? art.focus_ink : art.ink));
        for (size_t index = 0; index < labels.size(); ++index)
        {
            if (ongoing && index != 3) { continue; }
            const int row = ongoing ? 1 : static_cast<int>(index) + 1;
            rows.push_back(std::make_unique<UiText>("reference-end-item", labels[index], style.item_font,
                look.layout.position + Vector2F(0.0f, look.layout.spacing.y * static_cast<float>(row)),
                app.render_resources(), row == focus ? art.focus_ink : art.ink));
        }
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
        const auto card = [&](const RectangleF& bounds, const Colour& colour)
        {
            const float pen = art.keyline_width, step = pen * 2.0f, shadow = art.shadow_offset;
            const RectangleF face(bounds.x, bounds.y, bounds.width - shadow, bounds.height - shadow);
            const auto stepped = [&](const RectangleF& box, const Colour& tint)
            {
                fill(RectangleF(box.x + step, box.y, box.width - step * 2.0f, box.height), tint);
                fill(RectangleF(box.x, box.y + step, box.width, box.height - step * 2.0f), tint);
            };
            stepped(RectangleF(face.position() + Vector2F(shadow), face.size()), art.shade);
            stepped(face, art.ink);
            const RectangleF inside(face.x + pen, face.y + pen, face.width - pen * 2.0f, face.height - pen * 2.0f);
            stepped(inside, colour);
            for (int pass = 3; pass > 0; --pass)
            {
                Colour shade = art.shade; shade.a *= 0.12f;
                const float width = pen * static_cast<float>(pass);
                fill(RectangleF(inside.x + step, inside.bottom() - width, inside.width - step * 2.0f, width), shade);
                fill(RectangleF(inside.right() - width, inside.y + step, width, inside.height - step * 2.0f), shade);
            }
        };
        const RectangleF panel((menu_consts::DESIGN_RESOLUTION - look.box_size) / 2.0f, look.box_size);
        card(panel, art.paper);
        RectangleF primary = rows[0]->bounds(); primary.inflate(art.focus_padding); card(primary, art.focus_fill);
        if (focus != 0)
        {
            RectangleF selected = rows[static_cast<size_t>(focus)]->bounds();
            selected.inflate(art.focus_padding); card(selected, art.focus_fill);
        }
        // End places its paper inside the text container. Keep the resulting
        // point sampler for the real labels, rather than the layer's initial filter.
        for (const auto& row : rows) { row->draw(list); }
        const Frame reference = read(renderer);
        const auto compare_bounds = [&](RectangleF bounds, int item, int row)
        {
            bounds.inflate(Vector2F(2.0f)); camera.calculate_view_rectangle(bounds);
            const RectangleI pixels = countdown_crop_bounds(bounds);
            const Frame observed = foreground_crop(actual, pixels, 0);
            const Frame wanted = foreground_crop(reference, pixels, 0);
            const std::string stem = "end-menu-items-" + name + '-' + fixture.name + '-' +
                std::to_string(focus) + '-' + std::to_string(item) + '-' + std::to_string(actual.width) + 'x' +
                std::to_string(actual.height);
            if (observed.pixels != wanted.pixels)
            {
                save(observed, output / (stem + "-failure-actual.ppm"));
                save(wanted, output / (stem + "-failure-expected.ppm"));
                const RectangleI delta = foreground_delta_bounds(observed, wanted);
                std::cerr << "End item case=" << name << " session=" << fixture.name << " focus=" << focus <<
                    " item=" << item << " row=" << row << " crop=(" << pixels.x << ',' << pixels.y << ',' <<
                    pixels.width << ',' << pixels.height << ") local_delta=(" << delta.x << ',' << delta.y << ',' <<
                    delta.width << ',' << delta.height << ")\n";
                throw std::runtime_error("Actual End label differs from its independent item/card fixture");
            }
            save(observed, output / (stem + "-detail.ppm"));
            records << name << ',' << fixture.name << ',' << focus << ',' << item << ',' << row << ',' <<
                actual.width << ',' << actual.height << ",true\n";
        };
        compare_bounds(rows[0]->bounds(), -1, 0);
        for (size_t row = 1; row < rows.size(); ++row)
        {
            compare_bounds(rows[row]->bounds(), ongoing ? 3 : static_cast<int>(row) - 1, static_cast<int>(row));
        }
        if (ongoing)
        {
            const float height = app.render_resources()->measure_text(
                app.render_resources()->resolve_sprite_font(style.item_font), L"Change Weapons").y;
            compare_bounds(RectangleF(look.layout.position.x, look.layout.position.y + look.layout.spacing.y * 2.0f,
                panel.right() - look.layout.position.x - art.keyline_width * 3.0f,
                look.layout.spacing.y * 2.0f + height), 4, 2);
        }
        std::srand(6089); const int next_random = std::rand(); std::srand(6089);
        require(actual.pixels == end_menu_frame(app).pixels && std::rand() == next_random,
            "End label drawing changed frozen pixels or consumed random numbers");
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
            open(cases[index]); const Frame actual = compare("default", cases[index], 0, original, legacy);
            if (size.x == 1280) { baseline[index] = actual; }
        }
    }
    const Labels old_labels{ L"Teams~", L"Weapons~", L"Arenas~", L"Exit~" };
    const Labels new_labels{ L"Squads", L"Tools", L"Arena", L"Leave" };
    const auto with_labels = [&](const Labels& labels)
    {
        Presentation result = original;
        auto& text = const_cast<MenuText&>(result.menu_text());
        for (size_t index = 0; index < roles.size(); ++index)
            { text.items[static_cast<size_t>(roles[index])] = labels[index]; }
        return result;
    };
    Presentation old = with_labels(old_labels);
    old.validate_resources(*app.render_resources(), *app.audio_resources());
    const Font* source = app.render_resources()->font(
        app.render_resources()->resolve_sprite_font(original.menu().item_font));
    std::vector<Glyph> glyphs;
    for (char32_t character = U' '; character < U'~'; ++character)
    {
        const Glyph* glyph = source->find(character);
        require(glyph != nullptr, "End replacement font lacks a baseline printable glyph"); glyphs.push_back(*glyph);
    }
    const std::string font_name = "acceptance_end_items_without_tilde";
    app.render_resources()->add_font(font_name,
        std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
    require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(font_name), L"~"),
        "End replacement font still contains the old labels' glyph");
    Presentation next = with_labels(new_labels);
    const_cast<MenuStyle&>(next.menu()).item_font = font_name;
    next.validate_resources(*app.render_resources(), *app.audio_resources());
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    for (size_t index = 0; index < cases.size(); ++index)
    {
        const auto& fixture = cases[index];
        const bool ongoing = fixture.session.is_series() && !fixture.session.complete();
        const std::vector<EndMenuAction> actions = ongoing ?
            std::vector<EndMenuAction>{ EndMenuAction::restart, EndMenuAction::exit } :
            std::vector<EndMenuAction>{ EndMenuAction::restart, EndMenuAction::change_teams,
                EndMenuAction::change_weapons, EndMenuAction::change_level, EndMenuAction::exit };
        for (size_t selected = 0; selected < actions.size(); ++selected)
        {
            presentation = old; open(fixture);
            for (size_t move = 0; move < selected; ++move) { tap(Key::down); }
            const int focus = static_cast<int>(selected);
            const Frame retained = compare("custom-old", fixture, focus, old, old_labels);
            presentation = next; tick();
            require(retained.pixels == end_menu_frame(app).pixels,
                "Live End label/font replacement changed an existing page");
            if (selected == 0) { tap(Key::down); tap(Key::up); }
            else { tap(Key::up); tap(Key::down); }
            require(retained.pixels == end_menu_frame(app).pixels,
                "End focus movement rebased old labels into a font missing their glyph");
            compare("retained-old", fixture, focus, old, old_labels);

            // Hold the owner's music exit callback deliberately. The actual
            // typed action must wait on that seam without accepting input or
            // rebasing text; this does not claim hardware/listening coverage.
            int music_updates = 0;
            context.set_music_update([&](float) { ++music_updates; });
            tap(Key::enter);
            require(context.music_exit_pending() && !choice && stack.depth() == 2 && music_updates == 1,
                "End action bypassed or failed to start its owner-controlled music exit");
            tap(Key::down); tap(Key::enter); tap(Key::escape); tick(0.1f);
            require(context.music_exit_pending() && !choice && music_updates > 1 &&
                retained.pixels == end_menu_frame(app).pixels,
                "Pending End music exit accepted another action, moved focus or rebased labels");
            app.keyboard()->on_key_down(Key::enter); tick();
            static_cast<WindowNotify&>(app).on_deactivated(); tick();
            static_cast<WindowNotify&>(app).on_activated(); tick();
            app.keyboard()->on_key_up(Key::enter); tick(); tick();
            require(context.music_exit_pending() && !choice && retained.pixels == end_menu_frame(app).pixels,
                "Focus interruption completed an End music exit or changed its cached page");
            compare("pending-music-focus-return", fixture, focus, old, old_labels);
            context.complete_music_exit();
            require(choice == actions[selected] && results == 1 && stack.depth() == 1 && !context.music_exit_pending(),
                "Completing End music exit lost or duplicated its original typed action");
            context.complete_music_exit();
            require(results == 1, "Repeated End music completion duplicated a typed action");

            presentation = next; open(fixture);
            for (size_t move = 0; move < selected; ++move) { tap(Key::down); }
            const Frame adopted = compare("reopened-new-font", fixture, focus, next, new_labels);
            require(adopted.pixels != retained.pixels, "Reopened End did not adopt custom labels and item font");
            presentation = original; tick();
            require(adopted.pixels == end_menu_frame(app).pixels,
                "End label/font reversal changed an already reopened page");
            if (selected == 0) { tap(Key::down); tap(Key::up); }
            else { tap(Key::up); tap(Key::down); }
            require(adopted.pixels == end_menu_frame(app).pixels,
                "End focus after reversal changed cached labels or font");
            compare("retained-new-after-reversal", fixture, focus, next, new_labels);
            // Interrupted Enter must return to neutral and require a new
            // deliberate release before taking this same custom-labelled action.
            app.keyboard()->on_key_down(Key::enter); tick();
            static_cast<WindowNotify&>(app).on_deactivated(); tick();
            static_cast<WindowNotify&>(app).on_activated(); tick();
            app.keyboard()->on_key_up(Key::enter); tick(); tick();
            require(!choice && stack.depth() == 2 && !context.music_exit_pending() &&
                adopted.pixels == end_menu_frame(app).pixels,
                "Interrupted End Enter release selected an action or rebased cached labels");
            compare("focus-return-held-release", fixture, focus, next, new_labels);
            tap(Key::enter);
            require(choice == actions[selected] && results == 1 && stack.depth() == 1,
                "Custom/reversed End label or focus return changed a fresh typed action");
        }
        presentation = original; open(fixture);
        require(compare("restored-default", fixture, 0, original, legacy).pixels == baseline[index].pixels,
            "End item regression did not restore the original exact session raster");
        require(fixture.session.continue_string(original.menu_text().end) == fixture.continuation,
            "Authored End labels changed their session continuation");
    }
    require(settings.player_count() == 2 &&
        settings.player_settings()[0].num == input_slots::keyboard_and_mouse,
        "End label and typed-action fixtures mutated the match setup");
    require(records.good(), "End item evidence write failed");
    std::cout << "PASS actual End labels: four independent legacy roles across single/ongoing/draw/completed series "
        "and six display sizes; both Exit rows and hidden setup labels; custom labels/font, retained/reopened/reversed "
        "snapshots through repeated typed actions, owner-controlled pending music exits, interrupted focus and held "
        "Enter releases; unchanged continuation and frozen draw RNG\n";
}

void end_heading_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output);

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
    auto& art = const_cast<MenuPaperArtPresentation&>(look.art);
    struct RestoreEndArt
    {
        MenuPaperArtPresentation& art;
        MenuPaperArtPresentation original;
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
        // This historical raster fixture samples the settled ornament pose.
        if (key == Key::up || key == Key::down || key == Key::left || key == Key::right)
        { app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.51f); }
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
        require(fixture.session.continue_string(presentation.menu_text().end) == fixture.continuation,
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
                if (fixture.session.continue_string(presentation.menu_text().end) == L"Next Round") { next_round_pixels = normal.pixels; }
                if (fixture.session.continue_string(presentation.menu_text().end) == L"Replay Draw")
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
    end_menu_item_checks(app, canvas, tuning, presentation, output);
    end_heading_checks(app, canvas, tuning, presentation, output);
}
