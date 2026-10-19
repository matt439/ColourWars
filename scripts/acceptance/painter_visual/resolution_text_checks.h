// Normal-reader Options proof. Resolution expectations are independent literals;
// this fixture never calls setting_resolution, OptionRow or a display trial.
void resolution_text_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    const Presentation original = presentation;
    const MatchTuning original_match = tuning.match();
    require(!app.fullscreen(), "Resolution text probe requires its ordinary hidden windowed application");
    const NestedSaveProfile profile(output / "resolution-profile");
    Save preferences;
    PlaySettings play; play.music_volume = play.sfx_volume = 0;
    play.aim_sensitivity = 4; play.view_shake = false; play.reduce_effects = true;
    const_cast<PlaySettings&>(preferences.play_settings()) = play;
    GameData data; data.set_application(&app); data.set_save(&preferences);
    data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MenuLevelSettings settings; settings.set_player_count(2); settings.set_best_of(3);
    settings.set_duration(73.0f); settings.set_stage("movement_course");
    settings.set_screen_layout(ScreenLayout::two_player); settings.begin_input_claims(input_slots::keyboard_and_mouse);
    settings.set_player_team(0, PlayerTeam::a); settings.set_player_team(1, PlayerTeam::b);
    settings.set_player_weapon(0, WeaponType::sprayer); settings.set_player_weapon(1, WeaponType::roller);
    MenuContext context(&data, &input, &settings); StateContext stack;
    struct RestoreResolution
    {
        StateContext& stack; Presentation& presentation; Presentation original;
        ~RestoreResolution() { this->stack.clear(); this->presentation = this->original; }
    } restore{ stack, presentation, original };
    size_t updates = 0, readbacks = 0, rows = 0, crops = 0, openings = 0, font_controls = 0, save_calls = 0;
    ScreenResolution saved_resolution = ScreenResolution::s_1920_1080;
    bool saved_fullscreen = false;
    std::string saved, saved_preset = "1920x1080";
    const auto saved_bytes = [&]
    {
        std::ifstream file(output / "resolution-profile/ColourWars/save_data.json", std::ios::binary);
        require(file.good(), "Resolution text probe lost its private save");
        return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    };
    const auto seed = [&](ScreenResolution resolution, bool fullscreen, const char* literal)
    {
        stack.clear();
        require(preferences.set_display_settings_and_save(resolution, fullscreen, TeamColourPair::blue_yellow),
            "Cannot seed resolution text probe metadata in its private save");
        ++save_calls; saved_resolution = resolution; saved_fullscreen = fullscreen;
        saved_preset = literal; saved = saved_bytes();
    };
    const auto unchanged = [&]
    {
        const auto players = settings.player_settings(); const auto& active = preferences.play_settings();
        require(settings.player_count() == 2 && settings.best_of() == 3 && settings.duration() == 73.0f &&
            settings.stage() == "movement_course" && settings.screen_layout() == ScreenLayout::two_player &&
            players.size() == 2 && players[0].num == input_slots::keyboard_and_mouse && players[1].num == -1 &&
            settings.player_team(0) == PlayerTeam::a && settings.player_team(1) == PlayerTeam::b &&
            settings.player_weapon(0) == WeaponType::sprayer && settings.player_weapon(1) == WeaponType::roller,
            "Resolution editing changed an unrelated plan field, roster or device claim");
        require(preferences.resolution() == saved_resolution && preferences.fullscreen() == saved_fullscreen &&
            preferences.team_palette() == TeamColourPair::blue_yellow && active.music_volume == 0 &&
            active.sfx_volume == 0 && active.aim_sensitivity == 4 && !active.view_shake && active.aim_marker &&
            active.reduce_effects && preferences.records().stages().empty() && !preferences.last_match_setup() &&
            saved_bytes() == saved && !app.fullscreen() && tuning.match().duration == original_match.duration &&
            tuning.match().duration_choices == original_match.duration_choices,
            "Resolution page changed active metadata, Play preferences, records, save bytes, fullscreen or tuning");
        for (const auto& slot : active.sticks)
            { require(slot.movement_deadzone == -1 && slot.aim_deadzone == -1, "Resolution page changed a pad profile"); }
    };
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    const auto resize = [&](Vector2I size)
    {
        const size_t before = updates;
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        require(updates == before && !app.fullscreen(), "Metadata-only client resizing updated state or entered fullscreen");
    };
    const auto tick = [&]
    {
        require(updates < 43, "Resolution text probe exceeded its fixed update budget");
        ++updates; app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.0f);
    };
    const auto tap = [&](Key key)
        { app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick(); unchanged(); };
    const auto open = [&]
    {
        stack.clear(); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::up, Key::down, Key::left, Key::right, Key::enter, Key::escape, Key::space })
            { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        stack.push(std::make_unique<MainMenuOptions>(&context)); ++openings; tick();
        require(stack.depth() == 1, "Resolution text probe did not retain its actual Options page"); unchanged();
    };
    const auto actual_frame = [&]
    {
        require(readbacks < 76, "Resolution text probe exceeded its fixed readback budget");
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app.viewport_manager()->fullscreen_viewport());
        list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        canvas.draw(list, RectangleF(Vector2F::ZERO, app.resolution_manager()->resolution_vec()));
        stack.draw(renderer); ++readbacks; return read(renderer);
    };
    const RectangleF first_rows(132.0f, 204.0f, 1668.0f, 216.0f);
    struct ResolutionMetrics
    {
        Vector2F raw; float width_scale = 0.0f, height_scale = 0.0f, scale = 0.0f;
        RectangleF row; RectangleF focus_ornament;
    } metrics;
    const auto reference = [&](const Presentation& snapshot, const std::wstring& literal,
        int focused, bool pending_fullscreen, const std::string& font)
    {
        const auto& look = snapshot.main_menu(); const auto& style = snapshot.menu();
        MenuPaperArtPresentation art = look.art;
        art.focus_padding = look.setup_focus_padding; art.shadow_offset = look.setup_shadow_offset;
        const RectangleF paper((Vector2F(1920.0f, style.footer_y) - look.options_box_size) / 2.0f, look.options_box_size);
        const Vector2F position = look.options_position;
        const Vector2F limit(paper.right() - art.shadow_offset - art.focus_padding.x - look.options_value_x,
            look.options_spacing.y - art.focus_padding.y * 2.0f);
        const auto label_scale = [&](UiText& text)
        {
            const Vector2F raw = text.bounds().size(); float scale = 1.0f;
            const float width = look.options_value_x - position.x - art.focus_padding.x;
            if (raw.x > width) { scale = width / raw.x; }
            if (raw.y * scale > limit.y) { scale = limit.y / raw.y; }
            text.set_scale(scale);
        };
        const auto value_scale = [&](UiText& text)
        {
            const Vector2F raw = text.bounds().size(); float scale = 1.0f;
            if (raw.x > limit.x) { scale = limit.x / raw.x; }
            if (raw.y * scale > limit.y) { scale = limit.y / raw.y; }
            text.set_scale(scale); return scale;
        };
        UiText label("independent-resolution-label", L"Resolution", font, position, app.render_resources(),
            saved_fullscreen ? art.disabled_ink : focused == 0 ? art.focus_ink : art.ink);
        label_scale(label);
        UiText value("independent-resolution-value", literal, font, Vector2F(look.options_value_x, position.y),
            app.render_resources(), art.ink);
        metrics.raw = value.bounds().size(); metrics.width_scale = limit.x / metrics.raw.x;
        metrics.height_scale = limit.y / metrics.raw.y; metrics.scale = value_scale(value);
        metrics.row = RectangleF::union_of(label.bounds(), value.bounds());
        const Vector2F fullscreen_position = position + Vector2F(0.0f, look.options_spacing.y);
        UiText fullscreen_label("independent-fullscreen-label", L"Fullscreen", font, fullscreen_position,
            app.render_resources(), focused == 1 ? art.focus_ink : art.ink);
        label_scale(fullscreen_label);
        UiText fullscreen_value("independent-fullscreen-value", pending_fullscreen ? L"On" : L"Off", font,
            Vector2F(look.options_value_x, fullscreen_position.y), app.render_resources(), art.ink);
        std::ignore = value_scale(fullscreen_value);
        RectangleF focus = focused == 0 ? metrics.row :
            RectangleF::union_of(fullscreen_label.bounds(), fullscreen_value.bounds());
        std::unique_ptr<UiText> extra;
        if (focused == 4 || focused == 6)
        {
            extra = std::make_unique<UiText>("independent-resolution-extra-focus",
                focused == 4 ? L"Restore defaults" : L"Back", font,
                position + Vector2F(0.0f, look.options_spacing.y * static_cast<float>(focused)),
                app.render_resources(), art.focus_ink);
            focus = extra->bounds();
        }
        const auto ornament = [&](RectangleF bounds)
        {
            bounds.inflate(art.focus_padding); const float pen = art.keyline_width;
            const RectangleF arrow(bounds.x - pen * 7.0f, bounds.center().y - pen * 3.0f, pen * 5.0f, pen * 6.0f);
            return RectangleF::union_of(arrow, RectangleF::union_of(bounds,
                RectangleF(bounds.position() + Vector2F(art.shadow_offset), bounds.size())));
        };
        metrics.focus_ornament = ornament(focus);
        require(paper.contains(metrics.focus_ornament) && first_rows.contains(ornament(metrics.row)) &&
            first_rows.contains(ornament(RectangleF::union_of(fullscreen_label.bounds(), fullscreen_value.bounds()))) &&
            value.bounds().width <= limit.x + 0.01f && value.bounds().height <= limit.y + 0.01f &&
            label.bounds().right() <= value.bounds().x && metrics.raw.x > 0.0f && metrics.raw.y > 0.0f &&
            metrics.width_scale > 0.0f && metrics.height_scale > 0.0f && metrics.scale > 0.0f && metrics.scale <= 1.0f &&
            metrics.row.width > 0.0f && metrics.row.height > 0.0f,
            "Independent resolution/fullscreen row, focus card, shade or arrow escaped its complete comparison area");
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
        list.set_viewport(viewport); list.set_camera(camera); list.set_filter(TextureFilter::point);
        Canvas(app.render_resources(), look.home_background_colour).draw(list, camera.visible_rectangle(viewport));
        // Full resting paper, including its bevel, is reconstructed under both
        // complete rows. Production's paper-art OptionRow uses plain UiText.
        setup_paper_reference(list, *app.render_resources(), style, art, paper, focus);
        list.set_filter(TextureFilter::linear); label.draw(list); value.draw(list);
        fullscreen_label.draw(list); fullscreen_value.draw(list); if (extra) { extra->draw(list); }
        ++readbacks; return read(renderer);
    };
    const auto project = [&](RectangleF rectangle)
    {
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        camera.calculate_view_rectangle(rectangle); return countdown_crop_bounds(rectangle);
    };
    std::ofstream records(output / "resolution-values.csv");
    records << "case,width,height,focus,saved_preset,saved_fullscreen,pending_fullscreen,enabled,expected_value,font,raw_width,raw_height,width_scale,height_scale,scale,row_width,row_height,fit,full_row_match,ornament_inside,frozen_state_rng\n";
    const auto compare = [&](const std::string& name, const Presentation& snapshot, const std::wstring& literal,
        int focused, bool pending_fullscreen, const char* fit = "none", const char* crop = "", bool negative_font = false)
    {
        unchanged(); std::srand(7439); const int next_random = std::rand(); std::srand(7439);
        const Frame actual = actual_frame();
        require(actual.pixels == actual_frame().pixels && std::rand() == next_random,
            "Resolution drawing changed frozen page pixels or consumed match RNG");
        const Frame wanted = reference(snapshot, literal, focused, pending_fullscreen, snapshot.menu().item_font);
        const ResolutionMetrics measured = metrics;
        const RectangleI area = project(first_rows);
        const Frame observed = foreground_crop(actual, area, 0), expected = foreground_crop(wanted, area, 0);
        if (observed.pixels != expected.pixels)
        {
            const RectangleI delta = foreground_delta_bounds(observed, expected);
            std::cerr << "Resolution case=" << name << " size=" << actual.width << 'x' << actual.height <<
                " crop=(" << area.x << ',' << area.y << ',' << area.width << ',' << area.height <<
                ") local_delta=(" << delta.x << ',' << delta.y << ',' << delta.width << ',' << delta.height << ")\n";
            throw std::runtime_error("Actual resolution/fullscreen rows differ from the independent complete fitted fixture");
        }
        if (focused == 4 || focused == 6)
        {
            RectangleF bounds = measured.focus_ornament; bounds.inflate(2.0f);
            const RectangleI extra_area = project(bounds);
            require(foreground_crop(actual, extra_area, 0).pixels == foreground_crop(wanted, extra_area, 0).pixels,
                "Disabled resolution navigation or Restore-defaults focus lost its complete actual ornament");
        }
        const std::string fitting(fit);
        require(fitting != "width_then_height" || (measured.width_scale < 1.0f && measured.height_scale < measured.width_scale),
            "Old resolution output did not exercise width first and then height fitting");
        require(fitting != "height" || (measured.width_scale >= 1.0f && measured.height_scale < 1.0f),
            "Old shorter resolution output did not retain its independent height fit");
        require(fitting != "width" || (measured.width_scale < 1.0f && measured.width_scale < measured.height_scale),
            "Long replacement resolution output did not exercise width fitting");
        require(fitting != "reset" || measured.scale == 1.0f, "Short replacement resolution did not reset its previous fit scale");
        if (negative_font)
        {
            const Frame wrong = reference(snapshot, literal, focused, pending_fullscreen, original.menu().item_font);
            require(observed.pixels != foreground_crop(wrong, area, 0).pixels,
                "Resolution numeric-bearing font control did not distinguish the captured actual font"); ++font_controls;
        }
        if (*crop != '\0')
        {
            require((actual.width == 1280 && actual.height == 720 && observed.width == 1112 && observed.height == 144) ||
                (actual.width == 1600 && actual.height == 900 && observed.width == 1390 && observed.height == 180),
                "Resolution compact crop drifted from its exact fixed payload dimensions");
            save(observed, output / crop); ++crops;
        }
        records << name << ',' << actual.width << ',' << actual.height << ',' << focused << ',' << saved_preset << ',' <<
            (saved_fullscreen ? "true" : "false") << ',' << (pending_fullscreen ? "true" : "false") << ',' <<
            (saved_fullscreen ? "false" : "true") << ',';
        for (const wchar_t character : literal) { records << static_cast<char>(character); }
        records << ',' << snapshot.menu().item_font << ',' << measured.raw.x << ',' << measured.raw.y << ',' <<
            measured.width_scale << ',' << measured.height_scale << ',' << measured.scale << ',' <<
            measured.row.width << ',' << measured.row.height << ',' << fit << ",true,true,true\n";
        ++rows; unchanged(); return actual;
    };
    seed(ScreenResolution::s_1920_1080, false, "1920x1080"); resize({ 1280, 720 }); open();
    const Frame initial = compare("saved-windowed-preset", original, L"1920x1080", 0, false, "none", "resolution-text-default.ppm");
    tap(Key::left); compare("left-1280", original, L"1280x720", 0, false);
    tap(Key::left); compare("left-wrap-3840", original, L"3840x2160", 0, false, "none", "resolution-text-wrap.ppm");
    tap(Key::right); compare("right-wrap-1280", original, L"1280x720", 0, false);
    tap(Key::right); tap(Key::right); compare("right-2560", original, L"2560x1440", 0, false);
    tap(Key::down); tap(Key::right); tap(Key::up);
    compare("pending-on-enabled", original, L"2560x1440", 0, true);
    tap(Key::left); compare("pending-on-cycle", original, L"1920x1080", 0, true);

    const Font* source = app.render_resources()->font(app.render_resources()->resolve_sprite_font(original.menu().item_font));
    std::vector<Glyph> tall, shifted;
    for (char32_t character = U' '; character <= U'~'; ++character)
    {
        const Glyph* source_glyph = source->find(character);
        require(source_glyph != nullptr, "Resolution font fixture lacks an original printable glyph");
        Glyph glyph = *source_glyph;
        if (character == U'~') { glyph.y_offset += 100.0f; }
        tall.push_back(glyph);
        if (character == U'~') { continue; }
        glyph = *source_glyph;
        if (character >= U'0' && character <= U'9') { glyph.x_offset += 1.25f; glyph.x_advance += 0.75f; }
        shifted.push_back(glyph);
    }
    const std::string old_font = "resolution_tall_tilde", next_font = "resolution_numeric_shift";
    app.render_resources()->add_font(old_font, std::make_unique<Font>(source->atlas(), std::move(tall), source->line_spacing()));
    app.render_resources()->add_font(next_font, std::make_unique<Font>(source->atlas(), std::move(shifted), source->line_spacing()));
    require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(next_font), L"~"),
        "Resolution replacement numeric font still draws the old retained template's tilde");
    Presentation old = original, next = original;
    const_cast<MenuText&>(old.menu_text()).values.resolution = L"OLD~ display pixels: {width}x{height}";
    const_cast<MenuStyle&>(old.menu()).item_font = old_font;
    const_cast<MenuText&>(next.menu_text()).values.resolution = L"Next display pixels: {width}x{height}.";
    const_cast<MenuStyle&>(next.menu()).item_font = next_font;
    old.validate_resources(*app.render_resources(), *app.audio_resources());
    next.validate_resources(*app.render_resources(), *app.audio_resources());
    presentation = old; open();
    const Frame old_frame = compare("old-width-height", old, L"OLD~ display pixels: 1920x1080", 0, false,
        "width_then_height", "resolution-text-old-fit.ppm");
    presentation = next;
    require(compare("retained-old-source", old, L"OLD~ display pixels: 1920x1080", 0, false,
        "width_then_height").pixels == old_frame.pixels, "Resolution source assignment changed a constructed page");
    tap(Key::left); compare("old-short-height", old, L"OLD~ display pixels: 1280x720", 0, false, "height");
    open();
    const Frame next_frame = compare("reopened-new-width", next, L"Next display pixels: 1920x1080.", 0, false,
        "width", "resolution-text-new-width.ppm", true);
    presentation = original;
    require(compare("retained-new-reversal", next, L"Next display pixels: 1920x1080.", 0, false,
        "width").pixels == next_frame.pixels, "Resolution source reversal changed a constructed page");
    tap(Key::left); const Frame short_frame = compare("new-short-reset", next, L"Next display pixels: 1280x720.", 0, false,
        "reset", "resolution-text-new-reset.ppm");
    tap(Key::left); compare("new-long-refit", next, L"Next display pixels: 3840x2160.", 0, false, "width");
    tap(Key::right);
    require(compare("new-short-reset-again", next, L"Next display pixels: 1280x720.", 0, false, "reset").pixels == short_frame.pixels,
        "Resolution long-to-short cycle did not restore its exact original fitted row");
    const size_t resize_start = updates; resize({ 1920, 1080 });
    compare("resized-1080", next, L"Next display pixels: 1280x720.", 0, false, "reset"); resize({ 1280, 720 });
    require(compare("resized-720", next, L"Next display pixels: 1280x720.", 0, false, "reset").pixels == short_frame.pixels &&
        updates == resize_start, "Resolution resize round trip changed cached text/font/fit or updated state");
    open();
    require(compare("restored-windowed", original, L"1920x1080", 0, false).pixels == initial.pixels,
        "Restoring source and reopening Options did not restore its saved-preset pixels");

    // Only Save metadata changes. The application stays windowed throughout;
    // no Apply, display-trial or physical fullscreen operation is invoked.
    seed(ScreenResolution::s_3840_2160, true, "3840x2160"); resize({ 1600, 900 }); presentation = next; open();
    compare("fullscreen-live-client", next, L"Next display pixels: 1600x900.", 1, true, "none", "resolution-text-fullscreen.ppm");
    resize({ 1001, 777 });
    compare("fullscreen-retained-size", next, L"Next display pixels: 1600x900.", 1, true);
    presentation = original; tap(Key::right);
    compare("pending-off-disabled", next, L"Next display pixels: 1600x900.", 1, false);
    tap(Key::up); compare("disabled-skip-back", next, L"Next display pixels: 1600x900.", 6, false);
    tap(Key::up); tap(Key::up); tap(Key::enter);
    compare("fullscreen-defaults-refresh", next, L"Next display pixels: 1001x777.", 4, false);
    open(); compare("fullscreen-reopened-client", original, L"1001x777", 1, true);
    seed(ScreenResolution::s_3840_2160, false, "3840x2160"); open();
    compare("metadata-windowed-preset", original, L"3840x2160", 0, false);
    require(updates == 43 && readbacks == 76 && rows == 25 && crops == 6 && openings == 7 && font_controls == 1 &&
        save_calls == 3 && records.good(), "Resolution text probe drifted from its fixed evidence budget");
    std::ofstream summary(output / "resolution-summary.json");
    summary << "{\"outcome\":\"passed\",\"scope\":\"resolution-text-only\",\"updates\":43,\"readbacks\":76,"
        "\"csv_rows\":25,\"compact_crops\":6,\"page_openings\":7,\"font_negative_controls\":1,"
        "\"resize_updates\":0,\"save_calls\":3,\"crop_pixels\":1050840,\"metadata_only\":true,\"windowed_application\":true}\n";
    require(summary.good(), "Cannot retain resolution text probe summary");
    std::cout << "PASS resolution text leaf: actual Options presets/Left/Right/wrap and independent complete plain-text fitted rows; "
        "width-then-height fit, short reset, numeric-bearing font, retained/reopened/reversed sources; "
        "saved fullscreen metadata/live arbitrary client dimensions, disabled focus skip and unchanged pending-toggle policy; "
        "existing Defaults/reopen refresh timing, zero-update resize, private save/plan/RNG; "
        "43 updates,76 readbacks,25 rows,six compact crops,three successful private metadata seeds. "
        "No Apply/display-trial/fullscreen activation/global-input/physical-pad/foreground/commands/human/performance acceptance.\n";
}
