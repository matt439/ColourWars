// The paper reference accepts the already-independent expected hint. The older
// Stage leaf hardcodes Random's default count and cannot bound custom copy.
RectangleF stage_count_surface_reference(DrawList& list, RenderResources& resources,
    const Presentation& captured, const colourwars::UiTextDropShadow& hint, const colourwars::UiTextDropShadow& status)
{
    const auto& style = captured.menu(); const auto& main = captured.main_menu(); const auto& geometry = main.stage_select;
    const auto& words = captured.menu_text().stage; const Colour clear(0.0f, 0.0f, 0.0f, 0.0f);
    auto art = main.art; art.focus_padding = main.setup_focus_padding; art.shadow_offset = main.setup_shadow_offset;
    const RectangleF authored(geometry.briefing.position, geometry.briefing.size); RectangleF card = authored;
    const Vector2F info = authored.position() + geometry.briefing_padding;
    const auto include = [&](const colourwars::UiTextDropShadow& text)
    { RectangleF box = text.bounds(); box.inflate(art.keyline_width * 3.0f + art.shadow_offset); card = RectangleF::union_of(card, box); };
    struct Row { std::wstring text; std::string font; float line; };
    const std::array<Row, 5> rows{{ { words.briefing_label, style.brand_font, 0.0f },
        { words.solo_mode, style.detail_font, 1.0f }, { L"1 PLAYER", style.brand_font, 2.0f },
        { words.goal, style.brand_font, 3.0f }, { words.solo_objective, style.brand_font, 4.0f } }};
    for (const auto& row : rows)
    { const colourwars::UiTextDropShadow text("count-paper-fixed-bound", row.text, row.font, info + geometry.briefing.spacing * row.line,
        &resources, art.ink, clear, style.detail_shadow_offset); include(text); }
    include(status); include(hint);
    const RectangleF outer((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - main.stage_box_size) / 2.0f, main.stage_box_size);
    const float edge = art.keyline_width * 3.0f;
    const RectangleF inside(outer.position() + Vector2F(edge), outer.size() - Vector2F(edge * 2.0f + art.shadow_offset));
    const RectangleF preview(geometry.preview_position - Vector2F(style.rule_height), geometry.preview_size + Vector2F(style.rule_height * 2.0f));
    require(inside.contains(card) && !(card.x < preview.right() && card.right() > preview.x && card.y < preview.bottom() && card.bottom() > preview.y),
        "Independent count hint expanded its complete paper beyond the outer panel or into the preview");
    list.set_filter(TextureFilter::point);
    const UiTexture surround("count-paper-surround", style.sprite_sheet, style.fill_frame,
        RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), &resources, main.home_background_colour); surround.draw(list);
    setup_paper_reference(list, resources, style, art, outer, RectangleF());
    auto briefing = art; briefing.paper = art.focus_fill; setup_paper_reference(list, resources, style, briefing, card, RectangleF());
    const UiTexture accent("count-paper-accent", style.sprite_sheet, style.fill_frame,
        RectangleF(authored.position(), Vector2F(authored.width, style.accent_width)), &resources, art.ink); accent.draw(list);
    return card;
}

void stage_count_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    const Presentation original = presentation; Presentation old = original, next = original;
    const std::wstring wide(32, L'W');
    auto& old_words = const_cast<MenuText&>(old.menu_text()).stage;
    old_words.numbered_navigation = L"Course~ {index} of {total}";
    old_words.random_hint = L"{count} candidates~: " + wide + L"\nConfirm to lock.";
    auto& new_words = const_cast<MenuText&>(next.menu_text()).stage;
    new_words.numbered_navigation = L"S{index} / {total}"; new_words.random_hint = L"{count} choices\nConfirm to lock.";
    auto& new_style = const_cast<MenuStyle&>(next.menu());
    new_style.detail_font = "acceptance_stage_count_detail"; new_style.brand_font = "acceptance_stage_count_brand";
    const auto font = [&](const std::string& source_name, const std::string& name)
    {
        const Font* source = app.render_resources()->font(app.render_resources()->resolve_sprite_font(source_name)); std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character < U'~'; ++character)
        {
            const Glyph* glyph = source->find(character); require(glyph != nullptr, "Count font source lacks a printable baseline glyph"); Glyph copy = *glyph;
            if (character >= U'0' && character <= U'9') { copy.x_offset += 1.0f; } glyphs.push_back(copy);
        }
        app.render_resources()->add_font(name, std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
        const FontHandle handle = app.render_resources()->resolve_sprite_font(name);
        require(!app.render_resources()->can_render(handle, L"~") && app.render_resources()->font(handle)->drawn(U'2').x_offset == source->drawn(U'2').x_offset + 1.0f,
            "Count replacement font does not distinguish captured old copy and visible new digits");
    };
    font(original.menu().detail_font, new_style.detail_font); font(original.menu().brand_font, new_style.brand_font);
    new_style.detail_shadow_offset = Vector2F(4.0f, 3.0f);
    auto& new_geometry = const_cast<MainMenuPresentation&>(next.main_menu()).stage_select;
    new_geometry.navigation_position.x += 8.0f; new_geometry.briefing_padding.x += 8.0f;
    old.validate_resources(*app.render_resources(), *app.audio_resources()); next.validate_resources(*app.render_resources(), *app.audio_resources());
    const StageList mixed{ { "movement_course", "Movement Course", "stage_turbulence", false },
        { "close_quarters", "Close Quarters", "stage_close_quarters", true },
        { "king_of_the_hill", "King of the Hill", "stage_king_of_the_hill", true },
        { "turbulence", "Turbulence", "stage_turbulence", true } };
    const StageList excluded{ mixed[0] }, one_eligible{ mixed[0], mixed[1] };
    require(random_stage_indices(mixed) == std::vector<size_t>({ 1, 2, 3 }) && random_stage_indices(excluded).empty() &&
        random_stage_indices(one_eligible) == std::vector<size_t>{ 1 }, "Count fixture lists do not have the independent 0/1/3 eligible counts");
    const NestedSaveProfile isolated(output / "stage-count-profile"); Save preferences; SettingsSession preferences_session(preferences);
    PlaySettings play = preferences_session.play_settings(); play.music_volume = play.sfx_volume = 0; play.reduce_effects = true;
    require(preferences_session.set_play_settings_and_save(play), "Cannot seed the quiet private Stage count profile");
    for (const auto& stage : mixed)
    {
        StageRecord record; record.stage = stage.name; record.practice_played = 2147483647; record.best_coverage_percentage = 62.5f;
        require(const_cast<MatchRecords&>(preferences.records()).insert(record), "Cannot seed the private Stage count record");
    }
    require(preferences.save_to_file(), "Cannot persist the private Stage count fixture records");
    const auto bytes = [&]
    { std::ifstream file(output / "stage-count-profile/ColourWars/save_data.json", std::ios::binary); require(file.good(), "Cannot read the private Stage count save"); return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()); };
    const std::string saved = bytes(); StageList stages;
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_settings_session(&preferences_session); StageCatalog stage_catalog = acceptance::catalog_of(levels, stages, { "close_quarters", "king_of_the_hill", "movement_course", "turbulence" }); data.set_stage_catalog(&stage_catalog); data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MatchSetup settings, expected; MenuContext context(&data, &input, &settings); StateContext& stack = app;
    struct RestoreStageCounts
    {
        StateContext& stack; MenuContext& context; Presentation& look; Presentation original;
        ~RestoreStageCounts() { this->context.cancel_music_exit(); script_menu_music(this->context, {}); this->stack.clear(); this->look = this->original; }
    } restore{ stack, context, presentation, original };
    int ticks = 0, comparisons = 0, rows = 0, captures = 0;
    const auto unchanged = [&]
    {
        require(stack.depth() == 2 && !context.music_exit_pending() && settings.input_roster_complete() && settings.player_count() == 1 &&
            settings.stage() == expected.stage() && settings.best_of() == expected.best_of() && settings.duration() == expected.duration() &&
            settings.screen_layout() == expected.screen_layout() && settings.player_settings()[0].num == input_slots::keyboard_and_mouse &&
            settings.player_team(0) == PlayerTeam::a && settings.player_weapon(0) == WeaponType::bucket && bytes() == saved,
            "Count navigation/refresh changed the plan, private save, keyboard claim or page stack");
        for (const auto& stage : mixed)
        { const StageRecord* record = preferences.records().find(stage.name); require(record != nullptr && record->practice_played == 2147483647 &&
            record->best_coverage_percentage == 62.5f && record->versus_played == 0, "Count display mutated a private stage record"); }
    };
    const auto tick = [&]
    { require(++ticks <= 120, "Stage counts exceeded 120 bounded updates"); app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.0f); };
    const auto tap = [&](Key key)
    {
        std::srand(7183); const int random = std::rand(); std::srand(7183); app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick();
        require(std::rand() == random, "Count browse/first lock/unlock consumed RNG before final Random confirmation"); unchanged();
    };
    const auto open = [&](const StageList& list, const char* stage)
    {
        stack.clear(); context.cancel_music_exit(); script_menu_music(context, {}); stages = list;
        stage_catalog = acceptance::catalog_of(levels, stages, { "close_quarters", "king_of_the_hill", "movement_course", "turbulence" });
        presentation.validate_stage_counts(*app.render_resources(), stages.size(), random_stage_indices(stages).size());
        stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::enter, Key::escape, Key::left, Key::right, Key::up, Key::down }) { app.keyboard()->on_key_up(key); }
        app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        settings = MatchSetup(); settings.set_player_count(1); settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_stage(stage); settings.set_player_team(0, PlayerTeam::a); settings.set_player_weapon(0, WeaponType::bucket); settings.set_duration(83.0f); settings.set_screen_layout(ScreenLayout::one_player); expected = settings;
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas)); stack.push(std::make_unique<MainMenuStageSelect>(&context)); tick(); tick(); unchanged();
    };
    const std::wstring browse = L"Left/Right to browse.\nConfirm to lock your choice.", locked_hint = L"Confirm again to start.\nBack to change your choice.";
    const std::wstring random_three = L"Random: 3 available arenas.\nConfirm to lock your choice.", random_one = L"Random: 1 available arenas.\nConfirm to lock your choice.";
    const std::wstring old_hint = L"1 candidates~: " + wide + L"\nConfirm to lock.", new_hint = L"1 choices\nConfirm to lock.";
    std::ofstream records(output / "stage-counts.csv"); records << "case,role,width,height,total,eligible,random_slot,locked,captured_font,raw_width,raw_height,scale,shadow_x,shadow_y,record_room,pixels_match\n";
    const auto compare = [&](const char* name, const Presentation& captured, const Presentation& fitted,
        const std::wstring& navigation, const std::wstring& hint_words, bool random, bool locked, int fit_kind = 2, int capture_role = -1)
    {
        unchanged(); const auto& style = captured.menu(); const auto& main = captured.main_menu(); const auto& geometry = main.stage_select; const auto& live = fitted.main_menu().stage_select;
        const Vector2F info = geometry.briefing.position + geometry.briefing_padding, shadow = fitted.menu().detail_shadow_offset; const Colour clear(0.0f, 0.0f, 0.0f, 0.0f);
        colourwars::UiTextDropShadow nav("independent-count-navigation", navigation, style.detail_font, geometry.navigation_position, app.render_resources(), main.art.ink, clear, style.detail_shadow_offset);
        colourwars::UiTextDropShadow hint("independent-count-hint", hint_words, style.brand_font, info + geometry.briefing.spacing * 7.0f, app.render_resources(), main.art.ink, clear, shadow);
        colourwars::UiTextDropShadow status("independent-count-status", locked ? L"ARENA LOCKED" : L"FIND YOUR ARENA", style.detail_font, info + geometry.briefing.spacing * 6.0f,
            app.render_resources(), locked ? presentation.main_menu().stage_select.selected_ink : presentation.main_menu().stage_select.unselected, clear, style.detail_shadow_offset);
        const RectangleF raw_hint = RectangleF::union_of(hint.text_bounds_at(Vector2F::ZERO, 1.0f), hint.text_bounds_at(shadow, 1.0f));
        const float budget = live.briefing.size.x - live.briefing_padding.x * 2.0f, scale = std::min(1.0f, budget / raw_hint.width);
        require(std::isfinite(budget) && budget > 0.0f && std::isfinite(scale) && scale > 0.0f && (fit_kind != 1 || scale < 1.0f) && (fit_kind != 2 || scale == 1.0f),
            "Count hint did not exercise its independent width-only fit or no-upscale reset"); hint.set_scale(scale); hint.set_shadow_scale(scale); hint.set_shadow_offset(shadow * scale);
        const float preview_right = live.preview_position.x + live.preview_size.x, room = preview_right - nav.bounds().right() - fitted.menu().focus_padding.x;
        require(std::isfinite(room) && room > 0.0f, "Independent numbered/Random navigation left no finite positive record room");
        colourwars::UiTextDropShadow record("independent-count-record", L"BEST COVER 62.5% / 2147483647 PLAYED", geometry.record_font, Vector2F::ZERO, app.render_resources(), main.art.ink, clear, shadow);
        const RectangleF raw_record = RectangleF::union_of(record.text_bounds_at(Vector2F::ZERO, 1.0f), record.text_bounds_at(shadow, 1.0f));
        require(std::isfinite(raw_record.width) && raw_record.width > 0.0f, "Independent count record has no finite positive measured width");
        const float record_scale = std::min(1.0f, room / raw_record.width); require(std::isfinite(record_scale) && record_scale > 0.0f, "Independent count record fit is not finite and positive");
        if (std::string(name) == "mixed-four") { require(record_scale < 1.0f, "The seeded long practice record did not exercise actual width fitting"); }
        record.set_scale(record_scale); record.set_shadow_scale(record_scale); record.set_shadow_offset(shadow * record_scale); record.set_position(Vector2F(preview_right - raw_record.width * record_scale, live.record_y));
        require(record.bounds().x >= nav.bounds().right() + fitted.menu().focus_padding.x - 0.01f && record.bounds().right() <= preview_right + 0.01f &&
            record.bounds().y >= live.preview_position.y + live.preview_size.y + style.rule_height, "Independent record fit crossed the navigation gutter or preview");
        std::srand(9143); const int next_random = std::rand(); std::srand(9143); const Frame actual = main_menu_frame(app);
        require(actual.pixels == main_menu_frame(app).pixels && std::rand() == next_random, "Frozen Stage count drawing changed pixels or match RNG");
        const Viewport pane = app.viewport_manager()->fullscreen_viewport(); const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), pane);
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1); DrawList list = renderer.view(0); list.set_viewport(pane); list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        canvas.draw(list, RectangleF(Vector2F::ZERO, app.resolution_manager()->resolution_vec())); list.set_camera(camera);
        const SpriteSheet* sheet = app.render_resources()->sprite_sheet(style.sprite_sheet); sheet->draw(list, sheet->resolve_sprite_frame(style.fill_frame), camera.visible_rectangle(pane), style.scrim_colour);
        const RectangleF card = stage_count_surface_reference(list, *app.render_resources(), captured, hint, status);
        require(card.contains(hint.bounds()) && hint.bounds().width <= budget + 0.01f && hint.bounds().bottom() <= style.footer_y,
            "Independent composed count hint escapes its expanded paper, width budget or footer");
        list.set_filter(TextureFilter::linear); nav.draw(list); hint.draw(list); status.draw(list); if (!random) { record.draw(list); } const Frame reference = read(renderer);
        const std::array<const colourwars::UiTextDropShadow*, 4> texts{ &nav, &hint, &status, &record };
        const std::array<const char*, 4> roles{ "navigation", "hint", "status", "record" };
        for (size_t role = 0; role < texts.size(); ++role)
        {
            RectangleF box = texts[role]->bounds();
            if (role == 3 && random) { box = RectangleF(nav.bounds().right() + fitted.menu().focus_padding.x, live.record_y, room, raw_record.height); }
            box.inflate(Vector2F(2.0f)); camera.calculate_view_rectangle(box); const RectangleI crop = countdown_crop_bounds(box);
            const Frame observed = foreground_crop(actual, crop, 0), wanted = foreground_crop(reference, crop, 0);
            require(observed.pixels == wanted.pixels, "Actual Stage count navigation/hint/status/record state differs from independent literals and fitted raster bounds");
            records << name << ',' << roles[role] << ',' << actual.width << ',' << actual.height << ',' << stages.size() << ',' << random_stage_indices(stages).size() << ',' << random << ',' << locked << ',' <<
                (role == 1 ? style.brand_font : role == 3 ? geometry.record_font : style.detail_font) << ',' << (role == 1 ? raw_hint.width : role == 3 ? raw_record.width : texts[role]->bounds().width) << ',' <<
                (role == 1 ? raw_hint.height : role == 3 ? raw_record.height : texts[role]->bounds().height) << ',' << (role == 1 ? scale : role == 3 ? record_scale : 1.0f) << ',' <<
                (role == 0 || role == 2 ? style.detail_shadow_offset.x : shadow.x) << ',' << (role == 0 || role == 2 ? style.detail_shadow_offset.y : shadow.y) << ',' << room << ",true\n"; ++rows;
            if (static_cast<int>(role) == capture_role)
            { save(observed, output / ("stage-count-" + std::string(name) + '-' + roles[role] + '-' + std::to_string(actual.width) + "-detail.ppm")); ++captures; }
        }
        ++comparisons; unchanged();
    };
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080) })
    {
        app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y); presentation = original;
        open(mixed, "turbulence"); compare("mixed-four", original, original, L"<   ARENA 4 / 4   >", browse, false, false, 2, 0);
        tap(Key::left); compare("mixed-three", original, original, L"<   ARENA 3 / 4   >", browse, false, false);
        tap(Key::left); compare("mixed-two", original, original, L"<   ARENA 2 / 4   >", browse, false, false);
        tap(Key::left); compare("mixed-excluded-one", original, original, L"<   ARENA 1 / 4   >", browse, false, false);
        tap(Key::left); compare("mixed-random-three", original, original, L"<   SURPRISE ME   >", random_three, true, false, 2, 1);
        tap(Key::enter); compare("mixed-random-locked", original, original, L"<   SURPRISE ME   >", locked_hint, true, true);
        tap(Key::escape); compare("mixed-random-unlocked", original, original, L"<   SURPRISE ME   >", random_three, true, false);
        tap(Key::right); compare("mixed-concrete-return", original, original, L"<   ARENA 1 / 4   >", browse, false, false);
        open(excluded, "movement_course"); compare("excluded-only", original, original, L"<   ARENA 1 / 1   >", browse, false, false);
        tap(Key::right); compare("excluded-right-refused", original, original, L"<   ARENA 1 / 1   >", browse, false, false);
        tap(Key::left); compare("excluded-left-refused", original, original, L"<   ARENA 1 / 1   >", browse, false, false);
        open(one_eligible, "close_quarters"); compare("one-eligible-concrete", original, original, L"<   ARENA 2 / 2   >", browse, false, false);
        tap(Key::right); compare("one-eligible-grammar", original, original, L"<   SURPRISE ME   >", random_one, true, false);
        tap(Key::left); compare("one-eligible-return", original, original, L"<   ARENA 2 / 2   >", browse, false, false);
        presentation = old; open(one_eligible, "close_quarters"); compare("custom-old", old, old, L"Course~ 2 of 2", browse, false, false);
        presentation = next; tick(); compare("old-cached-fit", old, old, L"Course~ 2 of 2", browse, false, false);
        tap(Key::right); compare("old-long-live-fit", old, next, L"<   SURPRISE ME   >", old_hint, true, false, 1, 1);
        tap(Key::enter); compare("old-locked-short-reset", old, next, L"<   SURPRISE ME   >", locked_hint, true, true);
        tap(Key::escape); compare("old-unlocked-long-fit", old, next, L"<   SURPRISE ME   >", old_hint, true, false, 1);
        tap(Key::left); compare("old-concrete-live-fit", old, next, L"Course~ 2 of 2", browse, false, false);
        open(one_eligible, "close_quarters"); compare("reopened-new", next, next, L"S2 / 2", browse, false, false);
        tap(Key::right); compare("reopened-new-short", next, next, L"<   SURPRISE ME   >", new_hint, true, false, 2, 1);
        presentation = original; tick(); compare("new-cached-before-reversal-refresh", next, next, L"<   SURPRISE ME   >", new_hint, true, false);
        tap(Key::enter); compare("new-locked-original-fit", next, original, L"<   SURPRISE ME   >", locked_hint, true, true);
        tap(Key::escape); compare("new-unlocked-original-fit", next, original, L"<   SURPRISE ME   >", new_hint, true, false);
        tap(Key::left); compare("new-concrete-original-fit", next, original, L"S2 / 2", browse, false, false);
        open(one_eligible, "close_quarters"); compare("restored-default", original, original, L"<   ARENA 2 / 2   >", browse, false, false);
    }
    require(ticks == 104 && comparisons == 54 && rows == 216 && captures == 8, "Stage count matrix differs from its bounded source counts");
    require(records.good(), "Stage count evidence CSV write failed");
    std::cout << "PASS Stage counts: literal actual navigation/hint/status/record rasters at 720/1080; all four mixed indices, eligible counts 0/1/3 and unchanged singular grammar, excluded-only refusal, positive record room and independent right-aligned record fit/absence. Captured templates/detail+brand font IDs/positions/ink/navigation shadows, cached fit versus live hint width/shadow on actual browse/first Random lock/unlock/concrete refresh, reopen/reversal, exact private save/records/keyboard claim/settings/frozen/provisional RNG. " <<
        ticks << " updates (hard 120), " << comparisons << " observed states, " << rows << " CSV role checks, " << captures << " compact crops; no final Random draw, match launch, broad matrix, foreground/global input or physical/human/performance acceptance\n";
}
