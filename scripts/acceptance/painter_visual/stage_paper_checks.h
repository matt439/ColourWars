// Full actual Stage pages: the paper and text are independent constructions;
// the unchanged StagePreview is rendered separately into its original pane.
void stage_paper_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    const Presentation original = presentation;
    const StageList shipped{
        { "king_of_the_hill", "King of the Hill", "stage_king_of_the_hill", true },
        { "turbulence", "Turbulence", "stage_turbulence", true },
        { "close_quarters", "Close Quarters", "stage_close_quarters", true },
        { "movement_course", "Movement Course", "stage_turbulence", false } };
    for (const auto& stage : shipped)
    {
        const auto* info = levels.get(stage.name);
        require(info->display_name() == stage.display_name && info->icon_frame() == stage.icon_frame && info->include_in_random() == stage.include_in_random,
            "Shipped Stage paper metadata drifted from its independent arena list");
    }
    const auto& words = original.menu_text();
    require(words.heading(MenuHeading::arena) == L"Choose your arena" && words.values.random == L"Random" &&
        words.stage.briefing_label == L"MATCH SETUP" && words.stage.goal == L"PAINT TO WIN" && words.stage.solo_mode == L"SOLO PRACTICE" && words.stage.single_match_mode == L"SINGLE MATCH" &&
        words.stage.solo_objective == L"Make the arena your canvas.\nTry weapons. Cover more ground." && words.stage.versus_objective == L"Cover the arena in your colour.\nMost painted surface wins." &&
        words.stage.browse_status == L"FIND YOUR ARENA" && words.stage.locked_status == L"ARENA LOCKED" && words.stage.random_navigation == L"<   SURPRISE ME   >" &&
        words.stage.browse_hint == L"Left/Right to browse.\nConfirm to lock your choice." && words.stage.locked_hint == L"Confirm again to start.\nBack to change your choice.",
        "Shipped Stage paper copy drifted from its independent literals");
    Save preferences; auto& play = const_cast<PlaySettings&>(preferences.play_settings()); play.music_volume = play.sfx_volume = 0; play.reduce_effects = false;
    StageList stages = shipped;
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_level_infos(&levels); data.set_stages(&stages); data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MenuLevelSettings settings, initial;
    MenuContext context(&data, &input, &settings); StateContext& stack = app; MenuPage* active = nullptr;
    struct ClearStagePaper
    {
        StateContext& stack; MenuContext& context; Presentation& presentation; Presentation original;
        ~ClearStagePaper() { this->context.cancel_music_exit(); this->context.set_music_update({}); this->stack.clear(); painter_scripted_pads::states = {}; this->presentation = this->original; }
    } clear{ stack, context, presentation, original };
    const std::array<int, 4> slots{ input_slots::keyboard_and_mouse, 3, 0, 2 };
    const std::array<WeaponType, 4> tools{ WeaponType::sprayer, WeaponType::sniper, WeaponType::roller, WeaponType::bucket };
    Presentation captured = original, refreshed = original; int slot = 0; bool locked = false, missing = false; std::string committed;
    const auto tick = [&](float elapsed = 0.0f) { app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed); };
    const auto hold = [&](int player, GamepadButton button, bool down)
    {
        const int device = slots[static_cast<size_t>(player)];
        if (device == input_slots::keyboard_and_mouse)
        {
            const Key key = button == GamepadButton::a ? Key::enter : button == GamepadButton::b ? Key::escape : button == GamepadButton::dpad_left ? Key::left : Key::right;
            if (down) { app.keyboard()->on_key_down(key); } else { app.keyboard()->on_key_up(key); }
        }
        else { const auto bit = static_cast<uint16_t>(1u << static_cast<unsigned int>(button)); auto& buttons = painter_scripted_pads::states[static_cast<size_t>(device)].buttons; buttons = down ? static_cast<uint16_t>(buttons | bit) : static_cast<uint16_t>(buttons & static_cast<uint16_t>(~bit)); }
    };
    const auto tap = [&](int player, GamepadButton button) { hold(player, button, true); tick(); hold(player, button, false); tick(); };
    const auto frame = [&] { return main_menu_frame(app); };
    const auto motion = original.menu().motion; const float settled = std::max(motion.focus_duration, motion.press_duration) + 0.01f;
    const auto eligible = [&]
    {
        std::vector<size_t> result; for (size_t i = 0; i < stages.size(); ++i) { if (stages[i].include_in_random) { result.push_back(i); } } return result;
    };
    const auto open = [&](int count, int best_of = 1, int selected = 0)
    {
        stack.clear(); context.cancel_music_exit(); context.set_music_update({}); stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (Key key : { Key::enter, Key::escape, Key::left, Key::right }) { app.keyboard()->on_key_up(key); }
        painter_scripted_pads::states = {}; for (int player = 1; player < count; ++player) { painter_scripted_pads::states[static_cast<size_t>(slots[static_cast<size_t>(player)])].connected = true; }
        app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        settings = MenuLevelSettings(); settings.set_player_count(count); settings.set_best_of(best_of); settings.set_duration(73.0f); settings.set_stage(stages[static_cast<size_t>(selected)].name);
        settings.set_screen_layout(count == 1 ? ScreenLayout::one_player : count == 2 ? ScreenLayout::two_player : ScreenLayout::four_player);
        settings.begin_input_claims(slots[0]); for (int player = 1; player < count; ++player) { require(settings.claim_input_slot(slots[static_cast<size_t>(player)]), "Stage paper fixture claim failed"); }
        for (int player = 0; player < count; ++player) { settings.set_player_team(player, player % 2 ? PlayerTeam::b : PlayerTeam::a); settings.set_player_weapon(player, tools[static_cast<size_t>(player)]); }
        initial = settings; captured = refreshed = presentation; slot = selected; locked = missing = false; committed.clear();
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        std::srand(617); const int next_random = std::rand(); std::srand(617);
        auto page = std::make_unique<MainMenuStageSelect>(&context); active = page.get(); stack.push(std::move(page)); tick(); tick();
        require(stack.depth() == 2 && std::rand() == next_random && !context.music_exit_pending(), "Stage paper neutral entry changed state or consumed RNG");
    };
    const auto plan = [&]
    {
        require(settings.player_count() == initial.player_count() && settings.duration() == initial.duration() && settings.best_of() == initial.best_of() && settings.screen_layout() == initial.screen_layout() && settings.stage() == (committed.empty() ? initial.stage() : committed), "Stage paper committed a provisional arena or changed match settings");
        const auto rows = settings.player_settings(), before = initial.player_settings();
        for (size_t i = 0; i < rows.size(); ++i) { require(rows[i].num == before[i].num && rows[i].team == before[i].team && rows[i].weapon == before[i].weapon, "Stage paper changed slots/teams/tools"); }
    };
    const auto browse = [&](bool right, int player = 0)
    {
        const int last = static_cast<int>(stages.size()) - (eligible().empty() ? 1 : 0); const int next = right ? (slot == last ? 0 : slot + 1) : (slot == 0 ? last : slot - 1);
        std::srand(9341); const int after = std::rand(); std::srand(9341); tap(player, right ? GamepadButton::dpad_right : GamepadButton::dpad_left);
        require(std::rand() == after, "Stage paper provisional browse consumed RNG"); if (next != slot) { slot = next; refreshed = presentation; } plan();
    };
    const auto lock = [&](bool value, int player = 0)
    {
        std::srand(9341); const int after = std::rand(); std::srand(9341); tap(player, value ? GamepadButton::a : GamepadButton::b);
        require(std::rand() == after, "Stage paper provisional lock/unlock consumed RNG"); locked = value; refreshed = presentation; plan();
    };
    std::ofstream records(output / "stage-paper.csv"); records << "case,width,height,players,best_of,slot,locked,missing,pending,moving,preview_matches,full_frame_matches\n";
    const auto compare = [&](const std::string& name, float arrow = 0.0f, float press = 0.0f, bool capture = false)
    {
        const Frame actual = frame(); const auto& style = captured.menu(); const auto& look = captured.main_menu(); const auto& geometry = look.stage_select; const auto& live = refreshed.main_menu().stage_select;
        const auto& text = captured.menu_text(); auto art = look.art; art.focus_padding = look.setup_focus_padding; art.shadow_offset = look.setup_shadow_offset;
        const bool random = slot == static_cast<int>(stages.size()); const size_t candidate_count = eligible().size();
        const RectangleF design(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), preview(geometry.preview_position, geometry.preview_size);
        RectangleF edge = preview; edge.inflate(style.rule_height);
        const RectangleF paper((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - look.stage_box_size) / 2.0f, look.stage_box_size);
        const float border = art.keyline_width * 3.0f; const RectangleF inside(paper.position() + Vector2F(border), paper.size() - Vector2F(border * 2.0f + art.shadow_offset));
        std::vector<std::unique_ptr<UiTextDropShadow>> labels;
        const auto label = [&](const std::wstring& value, const std::string& font, const Vector2F& position, const Colour& ink, const Vector2F& shadow)
        {
            auto result = std::make_unique<UiTextDropShadow>("independent-stage-text", value, font, position, app.render_resources(), ink, Colour{}, shadow); auto* raw = result.get(); labels.push_back(std::move(result)); return raw;
        };
        const auto* heading = label(text.heading(MenuHeading::arena), style.heading_font, style.layout.position, art.ink, style.heading_shadow_offset);
        const auto& display = stages[static_cast<size_t>(random ? 0 : slot)].display_name;
        const std::wstring arena = random ? text.values.random : std::wstring(display.cbegin(), display.cend());
        auto* arena_name = label(arena, style.item_font, geometry.name_position, locked ? live.selected_ink : live.unselected, refreshed.menu().item_shadow_offset);
        const RectangleF raw_name = RectangleF::union_of(arena_name->text_bounds_at(Vector2F::ZERO, 1.0f), arena_name->text_bounds_at(refreshed.menu().item_shadow_offset, 1.0f));
        const float name_scale = std::min(1.0f, live.preview_size.x / raw_name.width); arena_name->set_scale(name_scale); arena_name->set_shadow_scale(name_scale); arena_name->set_shadow_offset(refreshed.menu().item_shadow_offset * name_scale);
        label(random ? text.stage.random_navigation : L"<   ARENA " + std::to_wstring(slot + 1) + L" / " + std::to_wstring(stages.size()) + L"   >", style.detail_font, geometry.navigation_position, art.ink, style.detail_shadow_offset);
        UiTextDropShadow question("independent-stage-question", L"?", look.announcement_font, Vector2F::ZERO, app.render_resources(), style.highlight, style.shadow_colour, style.title_shadow_offset);
        question.set_position(preview.center() - question.bounds().center());
        const Vector2F info = geometry.briefing.position + geometry.briefing_padding, row = geometry.briefing.spacing;
        const bool solo = settings.player_count() == 1; const int best_of = settings.best_of();
        std::vector<RectangleF> briefing_bounds;
        const auto brief = [&](const std::wstring& value, const std::string& font, float index, const Colour& ink)
        {
            auto* result = label(value, font, info + row * index, ink, style.detail_shadow_offset); briefing_bounds.push_back(result->bounds()); return result;
        };
        brief(text.stage.briefing_label, style.brand_font, 0.0f, art.focus_ink);
        brief(solo ? text.stage.solo_mode : best_of == 1 ? text.stage.single_match_mode : L"BEST OF " + std::to_wstring(best_of), style.detail_font, 1.0f, art.ink);
        brief(std::to_wstring(settings.player_count()) + (solo ? L" PLAYER" : L" PLAYERS / 2 TEAMS"), style.brand_font, 2.0f, art.ink);
        brief(text.stage.goal, style.brand_font, 3.0f, art.focus_ink);
        brief(solo ? text.stage.solo_objective : text.stage.versus_objective, style.brand_font, 4.0f, art.ink);
        brief(locked ? text.stage.locked_status : text.stage.browse_status, style.detail_font, 6.0f, locked ? live.selected_ink : live.unselected);
        const std::wstring hint = locked ? text.stage.locked_hint : random ? L"Random: " + std::to_wstring(candidate_count) + L" available arenas.\nConfirm to lock your choice." : text.stage.browse_hint;
        auto* hint_text = label(hint, style.brand_font, info + row * 7.0f, art.ink, refreshed.menu().detail_shadow_offset);
        const RectangleF raw_hint = RectangleF::union_of(hint_text->text_bounds_at(Vector2F::ZERO, 1.0f), hint_text->text_bounds_at(refreshed.menu().detail_shadow_offset, 1.0f));
        const float hint_scale = std::min(1.0f, (live.briefing.size.x - live.briefing_padding.x * 2.0f) / raw_hint.width);
        hint_text->set_scale(hint_scale); hint_text->set_shadow_scale(hint_scale); hint_text->set_shadow_offset(refreshed.menu().detail_shadow_offset * hint_scale); briefing_bounds.push_back(hint_text->bounds());
        RectangleF expected_card(geometry.briefing.position, geometry.briefing.size);
        for (auto bounds : briefing_bounds) { bounds.inflate(art.keyline_width * 3.0f + art.shadow_offset); expected_card = RectangleF::union_of(expected_card, bounds); }
        require(inside.contains(expected_card) && inside.contains(heading->bounds()) && inside.contains(arena_name->bounds()) && !expected_card.intersects(edge), "Independent Stage paper/card/text escapes its captured outer paper or crosses the preview");
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1); const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(design, viewport); DrawList list = renderer.view(0); list.set_viewport(viewport); list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        canvas.draw(list, RectangleF(Vector2F::ZERO, app.resolution_manager()->resolution_vec())); list.set_camera(camera);
        const SpriteSheet* fill_sheet = app.render_resources()->sprite_sheet(style.sprite_sheet); const auto fill = fill_sheet->resolve_sprite_frame(style.fill_frame);
        fill_sheet->draw(list, fill, camera.visible_rectangle(viewport), style.scrim_colour);
        const float offset = std::min(style.accent_width * (arrow + press), std::max(0.0f, geometry.briefing_padding.y - style.accent_width));
        const RectangleF card = stage_paper_surface_reference(list, *app.render_resources(), captured, refreshed, settings, random, locked, candidate_count, offset);
        require(card == expected_card, "Shared independent Stage surface differs from the full expected-text card union");
        const UiTexture preview_edge("independent-stage-edge", style.sprite_sheet, style.fill_frame, edge, app.render_resources(), locked ? live.selected : style.panel_edge_colour);
        const UiTexture preview_back("independent-stage-pane", style.sprite_sheet, style.fill_frame, preview, app.render_resources(), captured.hud().player_identity_background_colour);
        preview_edge.draw(list); preview_back.draw(list);
        StagePreview stage_preview("independent-actual-stage-preview", preview, &data); if (!random) { stage_preview.set_stage(stages[static_cast<size_t>(slot)].name); } stage_preview.draw(list);
        fill_sheet->draw(list, fill, RectangleF(0.0f, style.footer_y, menu_consts::DESIGN_RESOLUTION.x, menu_consts::DESIGN_RESOLUTION.y - style.footer_y), style.scrim_colour);
        fill_sheet->draw(list, fill, RectangleF(style.brand_position.x, style.footer_y, menu_consts::DESIGN_RESOLUTION.x - style.brand_position.x * 2.0f, style.rule_height), style.panel_edge_colour);
        list.set_filter(TextureFilter::linear);
        for (size_t i = 0; i < labels.size(); ++i) { labels[i]->draw(list); if (i == 2 && random) { question.draw(list); } }
        const auto& footer = captured.prompt_bar(); const std::wstring prompt = missing ? L" Reconnect controller    B/Esc Change players" : locked ? L"A/Enter Start match    B/Esc Unlock arena" : L"Left/Right Browse arenas    A/Enter Lock arena    B/Esc Back";
        const UiTextDropShadow legend("independent-stage-prompt", prompt, footer.font, footer.position, app.render_resources(), footer.colour, footer.shadow_colour, footer.shadow_offset); legend.draw(list);
        const Frame expected = read(renderer); const std::string stem = "stage-paper-" + name + '-' + std::to_string(settings.player_count()) + "p-best" + std::to_string(best_of) + "-slot" + std::to_string(slot) + '-' + std::to_string(actual.width) + 'x' + std::to_string(actual.height);
        if (actual.pixels != expected.pixels) { save(actual, output / (stem + "-failure-actual.ppm")); save(expected, output / (stem + "-failure-expected.ppm")); const RectangleI delta = foreground_delta_bounds(actual, expected); std::cerr << "STAGE PAPER " << name << " delta=" << delta.x << ',' << delta.y << ',' << delta.width << ',' << delta.height << '\n'; }
        require(actual.pixels == expected.pixels, "Actual Stage paper differs from independent paper/briefing/text and unchanged StagePreview raster");
        const RectangleI pane = countdown_crop_bounds(camera.calculate_view_rectangle(preview)); require(foreground_crop(actual, pane, 0).pixels == foreground_crop(expected, pane, 0).pixels, "Stage paper changed the actual preview raster or its physical pane/camera");
        const auto contrast = [&](RectangleF bounds)
        {
            camera.calculate_view_rectangle(bounds); const RectangleI box = countdown_crop_bounds(bounds); float dark = 1.0f, light = 0.0f; size_t ink = 0, pale = 0;
            for (int y = box.y; y < box.bottom(); ++y) { for (int x = box.x; x < box.right(); ++x) { const float value = hud_luminance(actual, hud_pixel_offset(actual, x, y)); dark = std::min(dark, value); light = std::max(light, value); if (value < 0.1f) { ++ink; } if (value > 0.5f) { ++pale; } } }
            require(ink > 2 && pale > 2 && (light + 0.05f) / (dark + 0.05f) >= 7.0f, "Stage paper lost native dark lettering/pale-paper contrast");
        };
        contrast(heading->bounds()); contrast(arena_name->bounds()); contrast(labels[2]->bounds()); for (const auto& bounds : briefing_bounds) { contrast(bounds); }
        if (capture) { save(actual, output / (stem + ".ppm")); save(foreground_crop(actual, countdown_crop_bounds(camera.calculate_view_rectangle(paper)), 0), output / (stem + "-detail.ppm")); }
        std::srand(7919); const int next_random = std::rand(); std::srand(7919); require(actual.pixels == frame().pixels && actual.pixels == frame().pixels && std::rand() == next_random, "Frozen Stage paper changed pixels or RNG"); plan();
        records << name << ',' << actual.width << ',' << actual.height << ',' << settings.player_count() << ',' << best_of << ',' << slot << ',' << locked << ',' << missing << ',' << context.music_exit_pending() << ',' << (offset > 0.0f) << ",true,true\n"; return actual;
    };
    const auto destination = [&](bool count)
    {
        MenuLevelSettings copy = settings; MenuInput other_input(app.gamepads(), app.keyboard(), &tuning); MenuContext other_context(&data, &other_input, &copy); StateContext other;
        other.push(std::make_unique<PauseMenuProbeRoot>(app, canvas)); if (count) { other.push(std::make_unique<MainMenuPlayerCount>(&other_context)); } else { other.push(std::make_unique<MainMenuWeaponSelect>(&other_context)); }
        app.renderer()->begin_frame(); other.draw(*app.renderer()); return read(*app.renderer());
    };
    const std::array<std::pair<int, int>, 4> sessions{{ { 1, 1 }, { 2, 1 }, { 2, 3 }, { 4, 5 } }};
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440), Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        for (const auto [count, best_of] : sessions)
        {
            open(count, best_of); const bool capture = count == 4 || size.x == 1280; const Frame first = compare("browse", 0, 0, capture);
            for (int selected = 0; selected <= static_cast<int>(stages.size()); ++selected)
            {
                if (selected) { compare("browse", 0, 0, capture); }
                lock(true, count - 1); tick(settled); const Frame fixed = compare("locked", 0, 0, capture);
                tap(0, GamepadButton::dpad_left); tap(count - 1, GamepadButton::dpad_right); require(fixed.pixels == frame().pixels, "Locked Stage paper accepted arrows or invented motion"); plan();
                lock(false); tick(settled); compare("unlocked"); browse(true); tick(settled);
            }
            require(first.pixels == frame().pixels, "Stage paper right wrap changed the original full frame"); browse(false); tick(settled); compare("left-wrap-random"); browse(true); tick(settled); require(first.pixels == frame().pixels, "Stage paper left wrap did not restore the original full frame");
            tap(0, GamepadButton::b); require(frame().pixels == destination(false).pixels, "Stage paper Back failed its ordinary Weapon route"); plan();
            for (int selected = 0; selected <= static_cast<int>(stages.size()); ++selected)
            {
                open(count, best_of, selected == static_cast<int>(stages.size()) ? 0 : selected); if (selected == static_cast<int>(stages.size())) { browse(false); tick(settled); }
                lock(true); tick(settled); const Frame outgoing = compare("before-start"); int music_updates = 0; context.set_music_update([&](float) { ++music_updates; });
                std::srand(4937); const auto candidates = eligible(); committed = selected == static_cast<int>(stages.size()) ? stages[candidates[static_cast<size_t>(std::rand()) % candidates.size()]].name : stages[static_cast<size_t>(selected)].name;
                const int after = std::rand(); std::srand(4937); tap(count - 1, GamepadButton::a);
                require(std::rand() == after && settings.stage() == committed && context.music_exit_pending() && stack.depth() == 2 && music_updates == 1 && outgoing.pixels == frame().pixels, "Stage paper final start changed its RNG budget, outgoing page or state boundary");
                compare("pending-start", 0, 0, capture && selected == static_cast<int>(stages.size()));
                tap(0, GamepadButton::dpad_right); tap(count - 1, GamepadButton::b); tap(0, GamepadButton::a); tick(settled); require(outgoing.pixels == frame().pixels, "Pending Stage paper fade accepted repeated input or refreshed outgoing Random"); plan();
                context.complete_music_exit(); context.complete_music_exit(); require(stack.depth() == 1 && !context.music_exit_pending(), "Stage paper fade duplicated its state pop");
            }
            if (count > 1)
            {
                open(count, best_of); painter_scripted_pads::states[static_cast<size_t>(slots[static_cast<size_t>(count - 1)])].connected = false; tick(); missing = true; compare("missing-player", 0, 0, capture);
                painter_scripted_pads::states[static_cast<size_t>(slots[static_cast<size_t>(count - 1)])].connected = true; hold(count - 1, GamepadButton::a, true); tick(); tick(); hold(count - 1, GamepadButton::a, false); tick(); tick(); missing = false; compare("held-reconnect");
                painter_scripted_pads::states[static_cast<size_t>(slots[static_cast<size_t>(count - 1)])].connected = false; tick(); tap(0, GamepadButton::b); require(frame().pixels == destination(true).pixels, "Missing Stage player rescue failed its ordinary Count route");
            }
        }
        std::cout << "PASS static Stage paper " << size.x << 'x' << size.y << '\n';
    }
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
    {
        app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y); open(4, 3); browse(true);
        const Frame moving = compare("browse-arrival", motion.arrow_travel, 0, true); tick(0.0f); tick(-0.5f); require(moving.pixels == frame().pixels, "Nonpositive time advanced Stage paper accent");
        play.reduce_effects = true; compare("reduced-immediate"); play.reduce_effects = false; require(moving.pixels == frame().pixels, "Reduced Stage drawing consumed its accent clock");
        tick(motion.focus_duration * 0.4f); compare("browse-middle", motion.arrow_travel * 0.36f, 0, true); tick(settled); compare("browse-rest", 0, 0, true);
        lock(true); compare("lock-press", 0, motion.press_fraction, true); tick(settled); lock(false); compare("unlock-press", 0, motion.press_fraction); stack.notify_activation(false); compare("deactivated"); stack.notify_activation(true); tick();
        browse(true); stack.push(std::make_unique<MainMenuCredits>(&context)); stack.pop(); compare("cover-return"); tick(); browse(false); active->debug_reset_input(slots[0]); compare("developer-reset");
        play.reduce_effects = true; browse(true); const Frame quiet = compare("reduced-input"); tick(settled); require(quiet.pixels == frame().pixels, "Reduced Stage input restarted its accent"); play.reduce_effects = false;
        open(4, 3); browse(false); lock(true); compare("rapid-lock", motion.arrow_travel, motion.press_fraction); context.set_music_update([](float) {});
        std::srand(4937); const auto candidates = eligible(); committed = stages[candidates[static_cast<size_t>(std::rand()) % candidates.size()]].name; const int after = std::rand(); std::srand(4937); tap(0, GamepadButton::a);
        require(std::rand() == after && context.music_exit_pending(), "Rapid Stage paper final start changed Random RNG or failed to latch"); const Frame pending = compare("rapid-final-random", 0, 0, true);
        static_cast<WindowNotify&>(app).on_deactivated(); tick(); static_cast<WindowNotify&>(app).on_activated(); tick(); tap(0, GamepadButton::dpad_left); tap(1, GamepadButton::b); tap(0, GamepadButton::a); tick(settled);
        require(pending.pixels == frame().pixels, "Focus interruption changed the latched outgoing Stage paper"); plan(); context.complete_music_exit();
        open(4, 3); hold(0, GamepadButton::a, true); tick(); static_cast<WindowNotify&>(app).on_deactivated(); tick(); static_cast<WindowNotify&>(app).on_activated(); tick(); hold(0, GamepadButton::a, false); tick(); tick(); compare("interrupted-lock");
        require(stack.depth() == 2 && !context.music_exit_pending(), "Interrupted Stage lock accepted a stale release or started the match");
        const Frame resize = frame(); app.resolution_manager()->set_resolution_exactly({ 1600, 1200 }); std::ignore = app.renderer()->window_size_changed(1600, 1200); std::ignore = frame(); app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y); require(resize.pixels == frame().pixels, "Stage paper changed after resize round-trip");
    }
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 }); std::ignore = app.renderer()->window_size_changed(1280, 720);
    Presentation old = original, next = original; const_cast<MenuText&>(old.menu_text()).headings[static_cast<size_t>(MenuHeading::arena)] = L"Choose~";
    auto& next_words = const_cast<MenuText&>(next.menu_text()); next_words.headings[static_cast<size_t>(MenuHeading::arena)] = L"Pick an arena"; next_words.values.random = L"Lucky Arena";
    next_words.stage.briefing_label = L"ROUND PLAN"; next_words.stage.goal = L"COVER THE FLOOR"; next_words.stage.solo_mode = L"PRACTICE"; next_words.stage.single_match_mode = L"ONE MATCH";
    next_words.stage.solo_objective = L"Try every tool.\nCover the arena."; next_words.stage.versus_objective = L"Paint the arena.\nMore surface wins."; next_words.stage.browse_status = L"PICK A PLACE"; next_words.stage.locked_status = L"PLACE LOCKED";
    next_words.stage.random_navigation = L"<   CHANCE   >"; next_words.stage.browse_hint = L"Browse stages.\nConfirm to lock."; next_words.stage.locked_hint = L"Confirm to begin.\nBack to unlock.";
    auto& next_style = const_cast<MenuStyle&>(next.menu()); next_style.heading_font = "gill_sans_mt_bold_48"; next_style.item_font = "gill_sans_mt_bold_36"; next_style.detail_font = "gill_sans_mt_bold_24"; next_style.brand_font = "courier_new_bold_16";
    next_style.item_shadow_offset = Vector2F(3.0f, 2.0f); next_style.detail_shadow_offset = Vector2F(2.0f, 1.0f);
    auto& next_look = const_cast<MainMenuPresentation&>(next.main_menu()); next_look.art.paper = Colour(0.90f, 0.95f, 0.98f); next_look.art.focus_fill = Colour(0.80f, 0.86f, 0.92f); next_look.art.ink = Colour(0.04f, 0.09f, 0.17f); next_look.art.focus_ink = Colour(0.02f, 0.05f, 0.1f);
    next_look.stage_select.unselected = Colour(0.04f, 0.09f, 0.17f); next_look.stage_select.selected_ink = Colour(0.02f, 0.05f, 0.1f); next_look.stage_select.selected = Colour(0.5f, 0.9f, 0.8f);
    old.validate_resources(*app.render_resources(), *app.audio_resources()); next.validate_resources(*app.render_resources(), *app.audio_resources());
    presentation = old; open(2, 3); compare("old-art-font-copy"); presentation = next; tick(); compare("unchanged-before-refresh"); browse(false); tick(settled); compare("old-random-after-replacement"); lock(true); tick(settled); compare("old-locked-after-replacement");
    open(2, 3); compare("reopened-art-font-copy", 0, 0, true); presentation = original; tick(); compare("captured-before-reversal-refresh"); browse(false); tick(settled); compare("new-random-after-reversal"); lock(true); tick(settled); compare("new-locked-after-reversal");
    presentation = original; stages = shipped; for (auto& stage : stages) { stage.include_in_random = false; } open(1); browse(false); tick(settled); compare("no-eligible-left-wrap"); browse(true); tick(settled); compare("no-eligible-right-wrap");
    stages = { shipped.back() }; open(1); const Frame only = compare("single-excluded-stage"); browse(true); tick(settled); browse(false); require(only.pixels == frame().pixels, "Single excluded Stage offered Random or invented motion");
    stages = shipped; open(4, 5); compare("restored-default");
    // Saved records: one line on the navigation row, right-aligned to the
    // preview and shrunk clear of the arena counter, for the shown stage and
    // this roster's kind of match only. Random and unplayed modes show none.
    {
        auto& board = const_cast<MatchRecords&>(preferences.records());
        struct ClearRecords { MatchRecords& board; ~ClearRecords() { this->board = MatchRecords(); } } clear_records{ board };
        const auto& geometry = original.main_menu().stage_select; const auto& style = original.menu(); const auto& record_words = original.menu_text().records;
        const RectangleF preview(geometry.preview_position, geometry.preview_size);
        const auto record_font = app.render_resources()->resolve_sprite_font(geometry.record_font);
        const auto detail_font = app.render_resources()->resolve_sprite_font(style.detail_font);
        StageRecord versus_record; versus_record.stage = shipped[0].name; versus_record.versus_played = 12; versus_record.team_a_wins = 7;
        versus_record.team_b_wins = 4; versus_record.draws = 1; versus_record.best_win_percentage = 68.46f; versus_record.practice_played = 2; versus_record.best_coverage_percentage = 37.0f;
        StageRecord draws_only; draws_only.stage = shipped[1].name; draws_only.versus_played = 3; draws_only.draws = 3;
        StageRecord crowded; crowded.stage = shipped[2].name; crowded.versus_played = 123456789; crowded.team_b_wins = 123456789; crowded.best_win_percentage = 100.0f;
        const auto shown = [&](int count, int selected, bool random, std::initializer_list<StageRecord> kept)
        {
            board = MatchRecords(); for (const StageRecord& record : kept) { board.insert(record); }
            open(count, 1, selected); if (random) { browse(false); tick(settled); }
            return frame();
        };
        for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080) })
        {
            app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y);
            const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), app.viewport_manager()->fullscreen_viewport());
            const auto pixels = [&](RectangleF bounds) { bounds.inflate(Vector2F(2.0f)); camera.calculate_view_rectangle(bounds); return countdown_crop_bounds(bounds); };
            const auto expect = [&](const char* name, int count, int selected, const std::wstring& line)
            {
                const Frame without = shown(count, selected, false, {});
                const Frame with = shown(count, selected, false, { versus_record, draws_only, crowded });
                const std::wstring counter = L"<   ARENA " + std::to_wstring(selected + 1) + L" / " + std::to_wstring(stages.size()) + L"   >";
                const RectangleF navigation(geometry.navigation_position, app.render_resources()->measure_text(detail_font, counter) + style.detail_shadow_offset);
                const Vector2F measured = app.render_resources()->measure_text(record_font, line) + style.detail_shadow_offset;
                const float scale = std::min(1.0f, (preview.right() - navigation.right() - style.focus_padding.x) / measured.x);
                const RectangleF box(Vector2F(preview.right() - measured.x * scale, geometry.record_y), measured * scale);
                const RectangleI delta = foreground_delta_bounds(with, without);
                require(delta.width > 0 && delta.height > 0 && pixels(box).contains(delta) &&
                    box.x >= navigation.right() + style.focus_padding.x - 0.5f && box.y >= preview.bottom() + style.rule_height,
                    "Stage record escaped its right-aligned row beside the arena counter or below the preview");
                hud_text_contrast(with, camera.calculate_view_rectangle(box));
                save(with, output / ("stage-record-" + std::string(name) + '-' + std::to_string(size.x) + 'x' + std::to_string(size.y) + ".ppm"));
                return scale;
            };
            require(expect("versus", 2, 0, record_words.best_win + L" 68.4% / 12 " + record_words.played) == 1.0f, "A typical versus stage record was shrunk");
            require(expect("solo", 1, 0, record_words.best_coverage + L" 37.0% / 2 " + record_words.played) == 1.0f, "A typical practice stage record was shrunk");
            expect("draws-only", 4, 1, L"3 " + record_words.played);
            require(expect("crowded", 2, 2, record_words.best_win + L" 100.0% / 123456789 " + record_words.played) < 1.0f, "A record too long for its row was not shrunk");
            require(shown(2, 0, true, {}).pixels == shown(2, 0, true, { versus_record }).pixels, "Random showed a stage record");
            require(shown(1, 1, false, {}).pixels == shown(1, 1, false, { draws_only }).pixels, "A stage unplayed in this kind of match showed a record");
        }
    }
    stack.clear();
    for (const bool overlap : { false, true })
    {
        Presentation malformed = original;
        auto& look = const_cast<MainMenuPresentation&>(malformed.main_menu());
        if (overlap) { look.stage_select.briefing.position.x = 1165.0f; look.stage_select.briefing_padding.x = 6.0f; }
        else { look.stage_box_size.y = 880.0f; }
        malformed.validate_resources(*app.render_resources(), *app.audio_resources());
        presentation = malformed; StateContext invalid_stack; bool rejected = false;
        try { invalid_stack.push(std::make_unique<MainMenuStageSelect>(&context)); }
        catch (const std::runtime_error& error) { rejected = std::string(error.what()).find(overlap ? "main_menu.stage_select.briefing: Stage Select paper must remain separate from the preview edge" : "main_menu.stage_box_size: Stage Select navigation must fit its paper") != std::string::npos; }
        require(rejected, "Actual Stage paper accepted navigation overflow or text-padded preview overlap after resource validation, or reported the wrong diagnostic");
    }
    presentation = original;
    require(records.good(), "Stage paper evidence write failed"); require(IsWindowVisible(static_cast<HWND>(app.window())) == FALSE && GetForegroundWindow() != static_cast<HWND>(app.window()), "Stage paper probe exposed or activated its hidden window");
    std::cout << "PASS Stage paper independent full paper/briefing/text plus unchanged actual StagePreview at six sizes; shipped arenas/eligible Random, solo/2P/4P modes, lock/unlock/start/back/recovery, exact Random latch/RNG, captured art/font/copy, local accent/reduction/lifecycle/frozen redraw and resize; scripted reader only\n";
}
