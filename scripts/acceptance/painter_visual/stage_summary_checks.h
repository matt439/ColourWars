// Only the two composed briefing rows. Expected strings are supplied literals;
// neither MenuText summary formatter is used by the raster/bounds oracle.
void stage_summary_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    struct SummaryCase
    {
        int players, best_of; const char* name;
        const wchar_t* mode; const wchar_t* roster;
        const wchar_t* old_mode; const wchar_t* old_roster;
        const wchar_t* new_mode; const wchar_t* new_roster;
    };
    const std::array<SummaryCase, 4> cases{{
        { 1, 5, "solo-best-five", L"SOLO PRACTICE", L"1 PLAYER", L"SOLO PRACTICE", L"1 SOLO~", L"SOLO PRACTICE", L"1 SOLO" },
        { 2, 3, "two-best-three", L"BEST OF 3", L"2 PLAYERS / 2 TEAMS", L"SERIES~ 3", L"2 PLAYERS~", L"3 rounds", L"2 CREW" },
        { 3, 5, "three-best-five", L"BEST OF 5", L"3 PLAYERS / 2 TEAMS", L"SERIES~ 5", L"3 PLAYERS~", L"5 rounds", L"3 CREW" },
        { 4, 1, "four-single", L"SINGLE MATCH", L"4 PLAYERS / 2 TEAMS", L"SINGLE MATCH", L"4 PLAYERS~", L"SINGLE MATCH", L"4 CREW" }
    }};
    const Presentation original = presentation; Presentation old = original, next = original;
    auto& old_words = const_cast<MenuText&>(old.menu_text()).stage;
    old_words.best_of_mode = L"SERIES~ {best_of}"; old_words.solo_roster = L"{players} SOLO~"; old_words.versus_roster = L"{players} PLAYERS~";
    auto& next_words = const_cast<MenuText&>(next.menu_text()).stage;
    next_words.best_of_mode = L"{best_of} rounds"; next_words.solo_roster = L"{players} SOLO"; next_words.versus_roster = L"{players} CREW";
    const auto replacement_font = [&](const std::string& source_name, const std::string& name)
    {
        const Font* source = app.render_resources()->font(app.render_resources()->resolve_sprite_font(source_name)); std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character < U'~'; ++character)
        {
            const Glyph* glyph = source->find(character); require(glyph != nullptr, "Summary font source lacks a baseline printable glyph");
            Glyph copy = *glyph;
            // A new ID alone could have identical pixels. Shift numeric glyphs
            // by one design unit to make reopening's font adoption observable.
            if (character >= U'0' && character <= U'9') { copy.x_offset += 1.0f; }
            glyphs.push_back(copy);
        }
        app.render_resources()->add_font(name, std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
        const FontHandle handle = app.render_resources()->resolve_sprite_font(name);
        require(!app.render_resources()->can_render(handle, L"~") &&
            app.render_resources()->font(handle)->drawn(U'3').x_offset == source->drawn(U'3').x_offset + 1.0f,
            "Summary replacement font does not distinguish retained old copy and new numeric glyphs");
    };
    auto& next_style = const_cast<MenuStyle&>(next.menu());
    next_style.detail_font = "acceptance_stage_summary_detail"; next_style.brand_font = "acceptance_stage_summary_brand";
    replacement_font(original.menu().detail_font, next_style.detail_font); replacement_font(original.menu().brand_font, next_style.brand_font);
    next_style.detail_shadow_offset = Vector2F(4.0f, 3.0f);
    old.validate_resources(*app.render_resources(), *app.audio_resources()); next.validate_resources(*app.render_resources(), *app.audio_resources());
    const NestedSaveProfile isolated(output / "stage-summary-profile"); Save preferences;
    PlaySettings play = preferences.play_settings(); play.music_volume = play.sfx_volume = 0; play.reduce_effects = true;
    require(preferences.set_play_settings_and_save(play), "Cannot seed private summary preferences");
    const auto saved_bytes = [&]
    {
        std::ifstream file(output / "stage-summary-profile/ColourWars/save_data.json", std::ios::binary);
        require(file.good(), "Cannot read the private summary save"); return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    };
    const std::string saved = saved_bytes();
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false },
        { "close_quarters", "Close Quarters", "stage_close_quarters", true } };
    require(random_stage_indices(stages) == std::vector<size_t>{ 1 }, "Summary refresh fixture needs an actual eligible Random slot");
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_level_infos(&levels);
    data.set_stages(&stages); data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MatchSetup settings, expected;
    MenuContext context(&data, &input, &settings); StateContext& stack = app;
    struct RestoreStageSummary
    {
        StateContext& stack; MenuContext& context; Presentation& look; Presentation original;
        ~RestoreStageSummary()
        { this->context.cancel_music_exit(); this->context.set_music_update({}); this->stack.clear(); painter_scripted_pads::states = {}; this->look = this->original; }
    } restore{ stack, context, presentation, original };
    constexpr std::array<int, 4> slots{ 4, 3, 0, 2 };
    constexpr std::array<WeaponType, 4> tools{ WeaponType::bucket, WeaponType::sniper, WeaponType::sprayer, WeaponType::mister };
    int ticks = 0, comparisons = 0, rows = 0, captures = 0, state_guards = 0;
    const auto unchanged = [&]
    {
        require(stack.depth() == 2 && !context.music_exit_pending() && settings.input_roster_complete() &&
            settings.player_count() == expected.player_count() && settings.best_of() == expected.best_of() && settings.duration() == expected.duration() &&
            settings.stage() == expected.stage() && settings.screen_layout() == expected.screen_layout(), "Summary refresh changed the stack, provisional match plan or roster completeness");
        const auto actual = settings.player_settings(), wanted = expected.player_settings();
        for (size_t seat = 0; seat < wanted.size(); ++seat)
        { require(actual[seat].num == wanted[seat].num && actual[seat].team == wanted[seat].team && actual[seat].weapon == wanted[seat].weapon,
            "Summary refresh changed a sparse device claim, team or weapon"); }
        for (size_t slot = 0; slot < painter_scripted_pads::states.size(); ++slot)
        {
            const auto& pad = painter_scripted_pads::states[slot]; const bool connected = std::find(slots.begin(), slots.begin() + settings.player_count(), static_cast<int>(slot)) != slots.begin() + settings.player_count();
            require(pad.connected == connected && pad.buttons == 0 && pad.left_stick == Vector2F::ZERO && pad.right_stick == Vector2F::ZERO &&
                pad.left_trigger == 0.0f && pad.right_trigger == 0.0f, "Summary refresh changed neutral scripted pad input");
        }
        require(saved_bytes() == saved, "Summary formatting/navigation persisted private preferences");
    };
    const auto tick = [&]
    { require(++ticks <= 120, "Stage summary exceeded120 bounded page updates"); app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.0f); };
    const auto tap = [&](Key key)
    {
        std::srand(3581); const int random = std::rand(); std::srand(3581);
        app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick();
        require(std::rand() == random, "Provisional summary browse/lock/unlock consumed RNG before final confirmation"); unchanged();
    };
    const auto open = [&](const SummaryCase& fixture)
    {
        stack.clear(); context.cancel_music_exit(); context.set_music_update({}); stack.notify_activation(true);
        app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::enter, Key::escape, Key::left, Key::right, Key::up, Key::down }) { app.keyboard()->on_key_up(key); }
        painter_scripted_pads::states = {}; settings = MatchSetup(); settings.set_player_count(fixture.players);
        settings.begin_input_claims(slots[0]);
        for (int seat = 1; seat < fixture.players; ++seat)
        { require(settings.claim_input_slot(slots[static_cast<size_t>(seat)]), "Cannot claim the connected summary fixture roster"); painter_scripted_pads::states[static_cast<size_t>(slots[static_cast<size_t>(seat)])].connected = true; }
        settings.set_stage("close_quarters"); settings.set_best_of(fixture.best_of); settings.set_duration(81.0f);
        settings.set_screen_layout(fixture.players == 1 ? ScreenLayout::one_player : fixture.players == 2 ? ScreenLayout::two_player : fixture.players == 3 ? ScreenLayout::three_player : ScreenLayout::four_player);
        for (int seat = 0; seat < fixture.players; ++seat)
        { settings.set_player_team(seat, seat % 2 == 0 ? PlayerTeam::a : PlayerTeam::b); settings.set_player_weapon(seat, tools[static_cast<size_t>(seat)]); }
        expected = settings; app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas)); stack.push(std::make_unique<MainMenuStageSelect>(&context)); tick(); tick(); unchanged();
    };
    std::ofstream records(output / "stage-summary.csv"); records << "case,context,role,random_slot,locked,width,height,font,x,y,text_width,text_height,shadow_x,shadow_y,scale,fixture_matches,refresh_state_verified\n";
    const auto compare = [&](const char* name, const SummaryCase& fixture, const Presentation& snapshot,
        const wchar_t* mode, const wchar_t* roster, bool random, bool locked, bool capture, bool verify_state = false)
    {
        unchanged(); const auto& style = snapshot.menu(); const auto& main = snapshot.main_menu(); const auto& geometry = main.stage_select;
        const Vector2F info = geometry.briefing.position + geometry.briefing_padding; const RectangleF authored(geometry.briefing.position, geometry.briefing.size);
        const Colour clear(0.0f, 0.0f, 0.0f, 0.0f);
        const std::array<colourwars::UiTextDropShadow, 2> texts{{
            colourwars::UiTextDropShadow("independent-summary-mode", mode, style.detail_font, info + geometry.briefing.spacing, app.render_resources(), main.art.ink, clear, style.detail_shadow_offset),
            colourwars::UiTextDropShadow("independent-summary-roster", roster, style.brand_font, info + geometry.briefing.spacing * 2.0f, app.render_resources(), main.art.ink, clear, style.detail_shadow_offset)
        }};
        // Delivery alone cannot prove a refresh: these two literal rasters
        // distinguish the actual slot and confirmation state after each tap.
        const std::array<colourwars::UiTextDropShadow, 2> state_texts{{
            colourwars::UiTextDropShadow("independent-summary-navigation", random ? L"<   SURPRISE ME   >" : L"<   ARENA 2 / 2   >",
                style.detail_font, geometry.navigation_position, app.render_resources(), main.art.ink, clear, style.detail_shadow_offset),
            colourwars::UiTextDropShadow("independent-summary-status", locked ? L"ARENA LOCKED" : L"FIND YOUR ARENA",
                style.detail_font, info + geometry.briefing.spacing * 6.0f, app.render_resources(),
                locked ? presentation.main_menu().stage_select.selected_ink : presentation.main_menu().stage_select.unselected,
                clear, style.detail_shadow_offset)
        }};
        for (size_t role = 0; role < texts.size(); ++role)
        {
            const RectangleF bounds = texts[role].bounds(); RectangleF padded = bounds; padded.inflate(main.art.keyline_width * 3.0f + main.setup_shadow_offset);
            require(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION).contains(bounds) && authored.contains(padded) &&
                bounds.bottom() <= (info + geometry.briefing.spacing * static_cast<float>(role + 2)).y,
                "Independent unscaled summary/shadow bounds escape the paper or reach the following row");
        }
        std::srand(8131); const int random_value = std::rand(); std::srand(8131); const Frame actual = main_menu_frame(app);
        require(actual.pixels == main_menu_frame(app).pixels && std::rand() == random_value, "Frozen summary drawing changed pixels or consumed match randomness");
        const Viewport pane = app.viewport_manager()->fullscreen_viewport(); const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), pane);
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1); DrawList list = renderer.view(0);
        list.set_viewport(pane); list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        canvas.draw(list, RectangleF(Vector2F::ZERO, app.resolution_manager()->resolution_vec())); list.set_camera(camera);
        const SpriteSheet* sheet = app.render_resources()->sprite_sheet(style.sprite_sheet);
        sheet->draw(list, sheet->resolve_sprite_frame(style.fill_frame), camera.visible_rectangle(pane), style.scrim_colour);
        // Reuse only the existing paper leaf, never its broad fixture. Both
        // supplied short summary rows are strictly inside the authored card,
        // so they cannot alter its expansion; their expected pixels are below.
        stage_paper_surface_reference(list, *app.render_resources(), snapshot, presentation, settings, random, locked);
        list.set_filter(TextureFilter::linear); for (const auto& text : texts) { text.draw(list); }
        if (verify_state) { for (const auto& text : state_texts) { text.draw(list); } }
        const Frame reference = read(renderer);
        if (verify_state)
        {
            for (const auto& text : state_texts)
            {
                RectangleF bounds = text.bounds(); bounds.inflate(Vector2F(2.0f)); camera.calculate_view_rectangle(bounds);
                const RectangleI crop = countdown_crop_bounds(bounds);
                require(foreground_crop(actual, crop, 0).pixels == foreground_crop(reference, crop, 0).pixels,
                    "Actual Stage navigation/status did not reach the independently expected Random/browse/locked/unlocked/concrete state");
                ++state_guards;
            }
        }
        for (size_t role = 0; role < texts.size(); ++role)
        {
            RectangleF box = texts[role].bounds(); const RectangleF design = box; box.inflate(Vector2F(2.0f)); camera.calculate_view_rectangle(box);
            const RectangleI crop = countdown_crop_bounds(box); const Frame observed = foreground_crop(actual, crop, 0), wanted = foreground_crop(reference, crop, 0);
            require(observed.pixels == wanted.pixels, "Actual Stage summary differs from its independently supplied literal/font/shadow raster");
            records << name << ',' << fixture.name << ',' << (role == 0 ? "mode" : "roster") << ',' << random << ',' << locked << ',' << actual.width << ',' << actual.height << ',' <<
                (role == 0 ? style.detail_font : style.brand_font) << ',' << design.x << ',' << design.y << ',' << design.width << ',' << design.height << ',' <<
                style.detail_shadow_offset.x << ',' << style.detail_shadow_offset.y << ",1,true," << verify_state << '\n'; ++rows;
        }
        if (capture)
        {
            RectangleF strip = RectangleF::union_of(texts[0].bounds(), texts[1].bounds()); strip.inflate(Vector2F(2.0f)); camera.calculate_view_rectangle(strip);
            save(foreground_crop(actual, countdown_crop_bounds(strip), 0), output / ("stage-summary-" + std::string(name) + '-' + fixture.name + "-detail.ppm")); ++captures;
        }
        ++comparisons; unchanged();
    };
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080) })
    {
        app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        for (const auto& fixture : cases) { presentation = original; open(fixture); compare("default", fixture, original, fixture.mode, fixture.roster, false, false, size.x == 1280); }
    }
    for (size_t index = 0; index < cases.size(); ++index)
    {
        const auto& fixture = cases[index]; const Vector2I size = index % 2 == 0 ? Vector2I(1280, 720) : Vector2I(1920, 1080);
        app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        presentation = old; open(fixture); compare("custom-old", fixture, old, fixture.old_mode, fixture.old_roster, false, false, false);
        presentation = next; tick(); compare("retained-old-cached", fixture, old, fixture.old_mode, fixture.old_roster, false, false, false);
        const auto refresh = [&](const char* prefix, const Presentation& captured, const wchar_t* mode, const wchar_t* roster)
        {
            tap(Key::right); compare(prefix, fixture, captured, mode, roster, true, false, false, true);
            tap(Key::enter); compare(prefix, fixture, captured, mode, roster, true, true, false, true);
            tap(Key::escape); compare(prefix, fixture, captured, mode, roster, true, false, false, true);
            tap(Key::left); compare(prefix, fixture, captured, mode, roster, false, false, false, true);
        };
        refresh("retained-old-refresh", old, fixture.old_mode, fixture.old_roster);
        open(fixture); compare("reopened-new", fixture, next, fixture.new_mode, fixture.new_roster, false, false, true);
        presentation = original; tick(); compare("retained-new-cached", fixture, next, fixture.new_mode, fixture.new_roster, false, false, false);
        refresh("retained-new-reversal-refresh", next, fixture.new_mode, fixture.new_roster);
        open(fixture); compare("restored-default", fixture, original, fixture.mode, fixture.roster, false, false, false);
    }
    require(ticks == 112 && comparisons == 60 && rows == 120 && captures == 8 && state_guards == 64, "Stage summary checked counts differ from the bounded source loops");
    require(records.good(), "Stage summary CSV write failed");
    std::cout << "PASS stage summaries: all1-4 sparse rosters, literal BEST OF3/5, solo-bestof5 precedence and Single Match at720/1080; custom3template copy and separate visible detail/brand font IDs, captured unscaled positions/clear shadows/paper bounds through actual browse/Random-lock/unlock refresh, reopen and reversal; unchanged plan/claims/save/input/frozen RNG, no final Random draw. " <<
        ticks << " updates(hard120), " << comparisons << " summary comparisons, " << state_guards << " independent navigation/status raster guards after32 taps, " << rows << " CSV data rows, " << captures << " compact crops; no broad matrix, match launch or physical/human/performance acceptance\n";
}
