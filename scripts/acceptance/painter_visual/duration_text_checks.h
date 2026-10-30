// Focused normal-reader proof for Mode's unscaled duration OptionRow. Expected
// output strings below never call setting_duration or reproduce its formatter.
void duration_text_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    const Presentation original = presentation;
    const MatchTuning original_match = tuning.match();
    require(original_match.duration == 240.0f && original_match.duration_choices ==
        std::vector<float>{ 60.0f, 120.0f, 240.0f, 360.0f, 480.0f },
        "Duration probe requires the independent shipped 240-second default and five choices");
    const NestedSaveProfile profile(output / "duration-profile");
    Save preferences; SettingsSession preferences_session(preferences);
    PlaySettings initial; initial.music_volume = initial.sfx_volume = 0;
    initial.aim_sensitivity = 4; initial.view_shake = false; initial.reduce_effects = true;
    require(preferences_session.set_play_settings_and_save(initial), "Cannot seed the duration probe's private preferences");
    const auto saved_bytes = [&]
    {
        std::ifstream stream(output / "duration-profile/ColourWars/save_data.json", std::ios::binary);
        require(stream.good(), "Duration probe lost its private save");
        return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    };
    const std::string saved = saved_bytes();
    GameData data;
    data.set_application(&app); data.set_save(&preferences); data.set_settings_session(&preferences_session); data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MatchSetup settings;
    settings.set_player_count(2); settings.set_best_of(1); settings.set_stage("movement_course");
    settings.set_screen_layout(ScreenLayout::two_player); settings.begin_input_claims(input_slots::keyboard_and_mouse);
    settings.set_player_team(0, PlayerTeam::a); settings.set_player_team(1, PlayerTeam::b);
    settings.set_player_weapon(0, WeaponType::sprayer); settings.set_player_weapon(1, WeaponType::roller);
    MenuContext context(&data, &input, &settings);
    StateContext stack;
    struct RestoreDuration
    {
        StateContext& stack; Presentation& presentation; Presentation original;
        GameTuning& tuning; MatchTuning match;
        ~RestoreDuration()
        {
            this->stack.clear(); this->presentation = this->original;
            const_cast<MatchTuning&>(this->tuning.match()) = this->match;
        }
    } restore{ stack, presentation, original, tuning, original_match };
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    const auto resize = [&](Vector2I size)
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
    };
    resize({ 1280, 720 });
    size_t updates = 0, readbacks = 0, rows = 0, crops = 0, openings = 0, font_controls = 0;
    std::optional<float> expected_override;
    const auto unchanged = [&]
    {
        const auto players = settings.player_settings(); const auto& play = preferences_session.play_settings();
        require(settings.player_count() == 2 && settings.best_of() == 1 && settings.stage() == "movement_course" &&
            settings.screen_layout() == ScreenLayout::two_player && players.size() == 2 &&
            players[0].num == input_slots::keyboard_and_mouse && players[1].num == -1 &&
            settings.player_team(0) == PlayerTeam::a && settings.player_team(1) == PlayerTeam::b &&
            settings.player_weapon(0) == WeaponType::sprayer && settings.player_weapon(1) == WeaponType::roller &&
            settings.duration() == expected_override,
            "Duration editing changed an unrelated plan field, device claim or the intended override");
        require(play.music_volume == 0 && play.sfx_volume == 0 && play.aim_sensitivity == 4 && !play.view_shake &&
            play.aim_marker && play.reduce_effects && preferences.resolution() == ScreenResolution::s_1920_1080 &&
            !preferences.fullscreen() && preferences.team_palette() == TeamColourPair::random &&
            preferences.records().stages().empty() && !preferences.last_match_setup() && saved_bytes() == saved,
            "Duration page changed active preferences, stage records, remembered setup or private save bytes");
        for (const auto& slot : play.sticks)
            { require(slot.movement_deadzone == -1 && slot.aim_deadzone == -1, "Duration editing changed a pad profile"); }
    };
    const auto tick = [&]
    {
        require(updates < 120, "Duration leaf probe exceeded its explicit update budget");
        ++updates; app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.0f);
    };
    const auto tap = [&](Key key)
        { app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick(); };
    const auto open = [&](std::optional<float> duration)
    {
        stack.clear(); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::up, Key::down, Key::left, Key::right, Key::enter, Key::escape, Key::space })
            { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll();
        input.reset_actions(); settings.set_duration(duration); expected_override = duration;
        stack.push(std::make_unique<MainMenuModeSelect>(&context)); ++openings; tick();
        for (int move = 0; move < 3; ++move) { tap(Key::down); }
        require(stack.depth() == 1, "Duration probe did not retain its actual Mode page"); unchanged();
    };
    const auto cycle = [&](Key key, std::optional<float> expected)
    {
        expected_override = expected; tap(key); unchanged();
    };
    const auto actual_frame = [&]
    {
        require(readbacks < 76, "Duration leaf probe exceeded its readback budget");
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app.viewport_manager()->fullscreen_viewport());
        list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        canvas.draw(list, RectangleF(Vector2F::ZERO, app.resolution_manager()->resolution_vec()));
        stack.draw(renderer); ++readbacks; return read(renderer);
    };
    struct RowMetrics { RectangleF row; RectangleF ornament; Vector2F value; };
    RowMetrics metrics;
    const auto reference = [&](const Presentation& snapshot, const std::wstring& value,
        const std::string& font)
    {
        const auto& style = snapshot.menu(); const auto& main = snapshot.main_menu();
        auto art = main.art; art.focus_padding = main.setup_focus_padding; art.shadow_offset = main.setup_shadow_offset;
        const Vector2F position = style.layout.position + Vector2F(0.0f, main.mode_select_spacing.y * 5.0f);
        const colourwars::UiTextDropShadow label("independent-duration-label", L"Match length", font, position,
            app.render_resources(), art.focus_ink, Colour{}, style.item_shadow_offset);
        const colourwars::UiTextDropShadow number("independent-duration-value", value, font,
            position + Vector2F(main.mode_select_spacing.x * 2.0f, 0.0f), app.render_resources(),
            art.ink, Colour{}, style.item_shadow_offset);
        const auto font_handle = app.render_resources()->resolve_sprite_font(font);
        metrics.value = app.render_resources()->measure_text(font_handle, value);
        metrics.row = RectangleF::union_of(label.bounds(), number.bounds());
        RectangleF padded = metrics.row; padded.inflate(art.focus_padding);
        const float pen = art.keyline_width, shade = art.shadow_offset;
        const RectangleF arrow(padded.x - pen * 7.0f, padded.center().y - pen * 3.0f, pen * 5.0f, pen * 6.0f);
        metrics.ornament = RectangleF::union_of(arrow, RectangleF::union_of(padded,
            RectangleF(padded.position() + Vector2F(shade), padded.size())));
        const RectangleF paper((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - main.setup_box_size) / 2.0f,
            main.setup_box_size);
        const RectangleF inside(paper.position() + Vector2F(pen * 3.0f),
            paper.size() - Vector2F(pen * 6.0f + shade));
        require(inside.contains(metrics.ornament) && label.bounds().right() <= number.bounds().x &&
            metrics.value.x > 0.0f && metrics.value.y > 0.0f && metrics.row.width > 0.0f && metrics.row.height > 0.0f,
            "Independent unscaled duration row, invisible shadow, card, shade or arrow escaped its paper/column");
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
        list.set_viewport(viewport); list.set_camera(camera); list.set_filter(TextureFilter::point);
        Canvas(app.render_resources(), main.home_background_colour).draw(list, camera.visible_rectangle(viewport));
        // This independent paper helper redraws the complete resting bevel and
        // detached cursor from literal text bounds; it never draws OptionRow.
        setup_paper_reference(list, *app.render_resources(), style, art, paper, metrics.row);
        list.set_filter(TextureFilter::linear); label.draw(list); number.draw(list);
        ++readbacks; return read(renderer);
    };
    const auto pixels = [&]
    {
        // Covers the full row including shade/arrow, but no neighbouring row:
        // row 5 sits at 630 with Mode's rows 96 apart (ROAD#20).
        return app.resolution_manager()->resolution_ivec() == Vector2I(1280, 720) ?
            RectangleI(56, 410, 1168, 72) : RectangleI(84, 615, 1752, 108);
    };
    std::ofstream records(output / "duration-values.csv");
    records << "case,width,height,focus,duration_override,live_default,expected_value,font,value_width,value_height,row_width,row_height,scale,full_row_match,ornament_inside,frozen_state_rng\n";
    const auto compare = [&](const std::string& name, const Presentation& snapshot,
        const std::wstring& expected, const std::string& crop = "", bool negative_font = false)
    {
        unchanged(); std::srand(5329); const int next_random = std::rand(); std::srand(5329);
        const Frame actual = actual_frame();
        require(actual.pixels == actual_frame().pixels && std::rand() == next_random,
            "Duration drawing changed frozen page pixels or consumed match RNG");
        const Frame wanted = reference(snapshot, expected, snapshot.menu().item_font);
        const RowMetrics measured = metrics;
        require(RectangleF(84.0f, 615.0f, 1752.0f, 108.0f).contains(measured.ornament),
            "Duration comparison crop omitted part of the complete independent ornament");
        const RectangleI area = pixels();
        const Frame observed = foreground_crop(actual, area, 0), expected_crop = foreground_crop(wanted, area, 0);
        if (observed.pixels != expected_crop.pixels)
        {
            const RectangleI delta = foreground_delta_bounds(observed, expected_crop);
            std::cerr << "Duration case=" << name << " size=" << actual.width << 'x' << actual.height <<
                " crop=(" << area.x << ',' << area.y << ',' << area.width << ',' << area.height <<
                ") local_delta=(" << delta.x << ',' << delta.y << ',' << delta.width << ',' << delta.height << ")\n";
            throw std::runtime_error("Actual duration label/value/full ornament differs from its independent unscaled literal fixture");
        }
        if (negative_font)
        {
            const Frame wrong = reference(snapshot, expected, original.menu().item_font);
            require(observed.pixels != foreground_crop(wrong, area, 0).pixels,
                "Duration numeric-bearing font control failed to distinguish the captured actual font"); ++font_controls;
        }
        if (!crop.empty())
        {
            require(actual.width == 1280 && actual.height == 720, "Duration compact evidence changed its fixed dimensions");
            save(observed, output / crop); ++crops;
        }
        records << name << ',' << actual.width << ',' << actual.height << ",3," <<
            settings.duration().value_or(-1.0f) << ',' << tuning.match().duration << ',';
        for (const wchar_t character : expected) { records << static_cast<char>(character); }
        records << ',' << snapshot.menu().item_font << ',' << measured.value.x << ',' << measured.value.y << ',' <<
            measured.row.width << ',' << measured.row.height << ",1,true,true,true\n"; ++rows;
        unchanged(); return actual;
    };
    open(std::nullopt);
    const Frame default_240 = compare("default-240", original, L"Default (4 min)", "duration-text-default.ppm");
    cycle(Key::right, 60.0f); compare("cycle-60", original, L"1 min");
    cycle(Key::right, 120.0f); compare("cycle-120", original, L"2 min");
    cycle(Key::right, 240.0f); const Frame explicit_240 = compare("explicit-240", original, L"4 min");
    require(default_240.pixels != explicit_240.pixels, "Equal numeric default/explicit durations lost their distinct wrapper");
    cycle(Key::left, 120.0f); compare("left-120", original, L"2 min");
    cycle(Key::enter, 240.0f); compare("confirm-240", original, L"4 min");
    cycle(Key::right, 360.0f); cycle(Key::right, 480.0f); cycle(Key::right, std::nullopt);
    compare("wrapped-default", original, L"Default (4 min)");
    open(90.0f); compare("fractional-90", original, L"1.5 min");
    open(61.0f); compare("fractional-61", original, L"1.01667 min", "duration-text-fraction.ppm");
    open(600000000.0f); compare("scientific-off-choice", original, L"1e+07 min", "duration-text-scientific.ppm");
    cycle(Key::right, 60.0f); compare("off-choice-right", original, L"1 min");
    open(std::nullopt); compare("numeric-live-before", original, L"Default (4 min)");
    auto& match = const_cast<MatchTuning&>(tuning.match()); match.duration = 90.0f; match.duration_choices = { 90.0f };
    cycle(Key::right, 90.0f); compare("live-explicit-90", original, L"1.5 min");
    cycle(Key::right, std::nullopt); compare("live-default-90", original, L"Default (1.5 min)");

    Presentation old = original, next = original;
    auto& old_values = const_cast<MenuText&>(old.menu_text()).values;
    old_values.duration_minutes = L"Old~ {value} minutes";
    old_values.duration_default = L"Use tuned default [{value}]";
    auto& next_values = const_cast<MenuText&>(next.menu_text()).values;
    next_values.duration_minutes = L"{value} minutes";
    next_values.duration_default = L"Use default [{value}]";
    const Font* source = app.render_resources()->font(app.render_resources()->resolve_sprite_font(original.menu().item_font));
    std::vector<Glyph> glyphs;
    for (char32_t character = U' '; character < U'~'; ++character)
    {
        const Glyph* source_glyph = source->find(character);
        require(source_glyph != nullptr, "Duration font fixture lacks an original printable glyph");
        Glyph glyph = *source_glyph;
        if (character >= U'0' && character <= U'9') { glyph.x_offset += 1.25f; glyph.x_advance += 0.75f; }
        glyphs.push_back(glyph);
    }
    const std::string next_font = "duration_numeric_shift";
    app.render_resources()->add_font(next_font,
        std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
    const auto next_handle = app.render_resources()->resolve_sprite_font(next_font);
    require(!app.render_resources()->can_render(next_handle, L"~") &&
        app.render_resources()->measure_text(next_handle, L"1.01667").x >
            app.render_resources()->measure_text(app.render_resources()->resolve_sprite_font(original.menu().item_font), L"1.01667").x,
        "Duration replacement font did not exclude the old glyph and change numeric bearings/advances");
    const_cast<MenuStyle&>(next.menu()).item_font = next_font;
    old.validate_resources(*app.render_resources(), *app.audio_resources());
    next.validate_resources(*app.render_resources(), *app.audio_resources());
    match.duration = 240.0f; match.duration_choices = { 61.0f, 90.0f, 240.0f };
    presentation = old; open(std::nullopt);
    const Frame old_default = compare("old-default", old, L"Use tuned default [Old~ 4 minutes]", "duration-text-old-long.ppm");
    presentation = next;
    require(compare("retained-old-after-assignment", old, L"Use tuned default [Old~ 4 minutes]").pixels == old_default.pixels,
        "Duration source/font assignment changed an existing page before refresh");
    match.duration = 90.0f; match.duration_choices = { 90.0f };
    cycle(Key::right, 90.0f); compare("old-live-explicit", old, L"Old~ 1.5 minutes");
    cycle(Key::right, std::nullopt); compare("old-live-default", old, L"Use tuned default [Old~ 1.5 minutes]");
    open(std::nullopt);
    const Frame new_default = compare("reopened-new-default", next, L"Use default [1.5 minutes]", "duration-text-new-long.ppm", true);
    presentation = original; match.duration = 61.0f; match.duration_choices = { 61.0f };
    require(compare("retained-new-after-reversal", next, L"Use default [1.5 minutes]").pixels == new_default.pixels,
        "Duration reversal or numeric source edit changed an existing page before refresh");
    cycle(Key::right, 61.0f); compare("new-live-explicit", next, L"1.01667 minutes");
    cycle(Key::right, std::nullopt); const Frame before_resize = compare("new-live-default", next, L"Use default [1.01667 minutes]");
    const size_t resize_start = updates; resize({ 1920, 1080 });
    compare("resized-1080", next, L"Use default [1.01667 minutes]"); resize({ 1280, 720 });
    require(compare("resized-720", next, L"Use default [1.01667 minutes]").pixels == before_resize.pixels && updates == resize_start,
        "Duration resize round trip changed cached font/row geometry, input or update count");
    match = original_match; presentation = original; open(std::nullopt);
    require(compare("restored-default", original, L"Default (4 min)").pixels == default_240.pixels,
        "Restoring source and reopening Mode did not restore exact default duration pixels");
    require(updates == 86 && readbacks == 76 && rows == 25 && crops == 5 && openings == 8 && font_controls == 1 && records.good(),
        "Duration leaf probe drifted from its fixed update/readback/row/crop budgets");
    std::ofstream summary(output / "duration-summary.json");
    summary << "{\"outcome\":\"passed\",\"scope\":\"duration-text-only\",\"updates\":86,\"readbacks\":76,"
        "\"csv_rows\":25,\"compact_crops\":5,\"page_openings\":8,\"font_negative_controls\":1,"
        "\"resize_updates\":0,\"save_calls\":1,\"crop_pixels\":420480}\n";
    require(summary.good(), "Cannot retain duration leaf summary");
    std::cout << "PASS duration text leaf: actual Mode duration cycle/Left/Enter/wrap, independent default/explicit/fractional/scientific literals; "
        "live numeric tuning with captured old/new templates and numeric-bearing font through refresh/reopen/reversal; "
        "unscaled label/value/invisible-shadow/card/shade/arrow, zero-update resize and unchanged private save/roster/RNG; "
        "86 updates,76 readbacks,25 rows,five compact crops. No global-input,physical-pad,foreground,gameplay,listening,human or performance acceptance.\n";
}

#if COLOURWARS_DEV_TOOLS
// Exercise both real targets' resource-only path without a level registry,
// editor document, live director, state update, readback or additional Save.
void duration_target_checks(Application& app, GameTuning& tuning, Presentation& presentation,
    const std::filesystem::path& content, const std::filesystem::path& output)
{
    const auto bytes = [](const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        require(file.good(), "Cannot read duration target content or candidate bytes");
        return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    };
    const std::array<std::string, 2> originals{
        bytes(content / "tuning.json"), bytes(content / "presentation.json") };
    const auto scratch = output / "duration-target-scratch";
    std::filesystem::create_directories(scratch);
    size_t candidate_writes = 0, legacy_acceptances = 0, paired_refusals = 0;
    size_t primed = 0, target_refusals = 0, failed_inert = 0, retries = 0, consumed = 0, validations = 0;
    const auto write = [&](const std::filesystem::path& path, const std::string& text)
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        file.write(text.data(), static_cast<std::streamsize>(text.size()));
        require(file.good(), "Cannot write a duration target's private candidate"); ++candidate_writes;
    };
    const auto replace = [](const std::string& text, const char* path, const char* token)
    {
        const JsonSpanDocument document(text, "duration-target-private-candidate");
        const JsonLeaf* leaf = document.find(path);
        require(leaf != nullptr && leaf->kind == JsonLeaf::Kind::number,
            "Duration target fixture lost its exact original numeric leaf");
        return document.replaced(*leaf, token);
    };
    const auto refusal = [](const auto& action, const char* diagnostic)
    {
        bool rejected = false;
        try { action(); }
        catch (const std::runtime_error& error)
        {
            require(std::string(error.what()).find(diagnostic) != std::string::npos,
                "Duration target rejected through an unexpected validation path");
            rejected = true;
        }
        require(rejected, "Duration target accepted a candidate outside its complete duration-row contract");
    };
    GameTuning private_tuning = tuning;
    Presentation private_presentation = presentation;
    GameData data; data.set_application(&app); data.set_tuning(&private_tuning);
    data.set_presentation(&private_presentation);
    std::srand(8264); const int next_random = std::rand(); std::srand(8264);

    // Literal templates draw with this font, as do the original 4/1/2/6/8
    // minute values. The scientific candidate alone introduces an absent '+'.
    RenderResources& resources = *app.render_resources();
    const Font* source = resources.font(resources.resolve_sprite_font(presentation.menu().item_font));
    std::vector<Glyph> glyphs;
    for (char32_t character = U' '; character <= U'~'; ++character)
    {
        if (character == U'+') { continue; }
        const Glyph* glyph = source->find(character);
        require(glyph != nullptr, "Duration target font fixture lacks an original printable glyph");
        glyphs.push_back(*glyph);
    }
    const std::string font_name = "duration_target_without_plus";
    resources.add_font(font_name, std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
    require(!resources.can_render(resources.resolve_sprite_font(font_name), L"+"),
        "Duration target private font did not exclude the scientific exponent sign");
    const_cast<MenuStyle&>(private_presentation.menu()).item_font = font_name;
    private_presentation.validate_resources(resources, *app.audio_resources());
    validate_mode_durations(&resources, private_tuning, private_presentation);
    const auto tuning_path = scratch / "tuning.candidate";
    const std::string valid_tuning = replace(originals[0], "match.duration", "90.0");
    const std::string bad_tuning = replace(originals[0], "match.duration", "600000000.0");
    TuningTarget tuning_target(&private_tuning, &private_presentation);
    write(tuning_path, valid_tuning);
    tuning_target.validate(valid_tuning, tuning_path.string(), &data, false); ++validations; ++primed;
    require(private_tuning.match().duration == 240.0f, "Duration target applied its primed candidate before apply");
    write(tuning_path, bad_tuning);
    const GameTuning bad_balance(tuning_path.string().c_str());
    bad_balance.validate_resources(resources, *app.audio_resources()); ++legacy_acceptances;
    refusal([&] { validate_mode_durations(&resources, bad_balance, private_presentation); },
        "menu_text.values.duration_default"); ++paired_refusals;
    refusal([&] { ++validations; tuning_target.validate(bad_tuning, tuning_path.string(), &data, false); },
        "menu_text.values.duration_default"); ++target_refusals;
    tuning_target.apply(false);
    require(private_tuning.match().duration == 240.0f,
        "Failed duration tuning validation retained a previously pending candidate"); ++failed_inert;
    write(tuning_path, valid_tuning);
    tuning_target.validate(valid_tuning, tuning_path.string(), &data, false); ++validations;
    tuning_target.apply(false);
    require(private_tuning.match().duration == 90.0f, "Duration tuning target did not accept a valid retry"); ++retries;
    const_cast<MatchTuning&>(private_tuning.match()).duration = 61.0f; tuning_target.apply(false);
    require(private_tuning.match().duration == 61.0f, "Duration tuning target applied its consumed retry twice"); ++consumed;

    // A positive value column is insufficient: x=820 leaves room before the
    // paper edge but the original, unscaled Default (4 min) cannot fit there.
    private_tuning = tuning; private_presentation = presentation;
    const auto presentation_path = scratch / "presentation.candidate";
    const std::string valid_presentation = replace(originals[1], "main_menu.mode_select_spacing.x", "251.0");
    const std::string bad_presentation = replace(originals[1], "main_menu.mode_select_spacing.x", "820.0");
    PresentationTarget presentation_target(&private_presentation, &private_tuning);
    write(presentation_path, valid_presentation);
    presentation_target.validate(valid_presentation, presentation_path.string(), &data, false); ++validations; ++primed;
    require(private_presentation.main_menu().mode_select_spacing.x == 250.0f,
        "Duration presentation target applied its primed candidate before apply");
    write(presentation_path, bad_presentation);
    const Presentation bad_layout(presentation_path.string().c_str());
    bad_layout.validate_resources(resources, *app.audio_resources()); ++legacy_acceptances;
    refusal([&] { validate_mode_durations(&resources, private_tuning, bad_layout); },
        "the complete duration row"); ++paired_refusals;
    refusal([&] { ++validations; presentation_target.validate(bad_presentation, presentation_path.string(), &data, false); },
        "the complete duration row"); ++target_refusals;
    presentation_target.apply(false);
    require(private_presentation.main_menu().mode_select_spacing.x == 250.0f,
        "Failed duration presentation validation retained a previously pending candidate"); ++failed_inert;
    write(presentation_path, valid_presentation);
    presentation_target.validate(valid_presentation, presentation_path.string(), &data, false); ++validations; presentation_target.apply(false);
    require(private_presentation.main_menu().mode_select_spacing.x == 251.0f,
        "Duration presentation target did not accept a valid retry"); ++retries;
    const_cast<MainMenuPresentation&>(private_presentation.main_menu()).mode_select_spacing.x = 250.0f;
    presentation_target.apply(false);
    require(private_presentation.main_menu().mode_select_spacing.x == 250.0f,
        "Duration presentation target applied its consumed retry twice"); ++consumed;
    require(bytes(content / "tuning.json") == originals[0] && bytes(content / "presentation.json") == originals[1] &&
        std::rand() == next_random, "Duration target proof changed real content bytes or consumed match RNG");
    require(candidate_writes == 6 && legacy_acceptances == 2 && paired_refusals == 2 && primed == 2 &&
        target_refusals == 2 && failed_inert == 2 && retries == 2 && consumed == 2 && validations == 6,
        "Duration target proof drifted from its fixed validation/candidate budgets");
    std::ofstream summary(output / "duration-target-summary.json");
    summary << "{\"outcome\":\"passed\",\"scope\":\"duration-targets-only\",\"targets\":2,"
        "\"legacy_candidate_acceptances\":2,\"paired_refusals\":2,\"primed_candidates\":2,\"target_refusals\":2,"
        "\"failed_apply_inert\":2,\"valid_retries\":2,\"consumed_candidates\":2,\"target_validate_calls\":6,"
        "\"candidate_writes\":6,\"state_updates\":0,\"readbacks\":0,\"save_calls\":0,"
        "\"real_content_unchanged\":true,\"frozen_rng\":true}\n";
    require(summary.good(), "Cannot retain duration targets' resource-only summary");
    std::cout << "PASS duration real resource-only TuningTarget/PresentationTarget: two legacy-resource acceptances, "
        "paired and target refusals, primed-candidate discard, inert failed apply, valid retries and consumed apply; "
        "six validations/six private candidate writes, zero state updates/readbacks/Saves, real content bytes and RNG unchanged.\n";
}
#endif
