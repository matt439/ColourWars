// Actual binding-page navigation uses private snapshots and its own save.
// Included by the focused scripted-pad painter alongside Keyboard Jump.
#if defined(COLOURWARS_PAINTER_SCRIPTED_PADS)
void pad_buttons_matrix(Application& app, Registry<LevelLoadedInfo>& levels,
    const Canvas& canvas, GameTuning& tuning, Presentation& presentation,
    const std::filesystem::path& content, const std::filesystem::path& output)
{
    const auto profile = output / "pad-jump-profile";
    const NestedSaveProfile isolated(profile);
    const auto path = profile / "ColourWars/save_data.json";
    const auto blocked = std::filesystem::path(path.wstring() + L".tmp");
    Save preferences; PlaySettings quiet; quiet.music_volume = quiet.sfx_volume = 0;
    require(preferences.set_play_settings_and_save(quiet), "Cannot quiet private Pad Jump preferences");
    InputBindings initial; initial.keyboard_jump = Key::j;
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false } };
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_tuning(&tuning);
    data.set_presentation(&presentation); data.set_level_infos(&levels); data.set_stages(&stages);
    DebugDirector director(&tuning, &presentation); data.set_debug(&director);
#if COLOURWARS_DEV_TOOLS
    director.impl()->set_content_directories(content, content);
    director.impl()->set_user_directory(profile / "debug");
#else
    static_cast<void>(content);
#endif
    director.attach(&app, &data);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MatchSetup settings;
    MenuContext context(&data, &input, &settings); StateContext& stack = app;
    const Presentation original = presentation;
    struct RestorePadJump
    {
        StateContext& stack; Application& app; Presentation& presentation; const Presentation& original;
        std::filesystem::path blocked;
        ~RestorePadJump()
        {
            this->stack.clear(); this->presentation = this->original; painter_scripted_pads::states = {};
            this->app.gamepads()->poll(); this->app.gamepads()->poll();
            std::error_code ignored; std::filesystem::remove(this->blocked, ignored);
        }
    } restore{ stack, app, presentation, original, blocked };
    const auto bytes = [&]
    {
        std::ifstream file(path, std::ios::binary); require(file.good(), "Private Pad Jump save disappeared");
        return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    };
    int updates = 0, copies = 0, reloads = 0;
    const auto tick = [&](float elapsed = 0.0f)
    {
        require(++updates < 1800, "Pad Jump exceeded bounded scripted update budget");
        app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed);
    };
    const auto tap = [&](Key key)
    { app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick(); };
    const auto move = [&](Key key, int count) { for (int at = 0; at < count; ++at) { tap(key); } };
    const auto tap_pad = [&](int slot, GamepadButton button)
    {
        auto& pad = painter_scripted_pads::states[slot];
        pad.buttons = static_cast<uint16_t>(1u << static_cast<unsigned int>(button)); tick();
        pad.buttons = 0; tick();
    };
    const auto move_pad = [&](int slot, GamepadButton button, int count)
    { for (int at = 0; at < count; ++at) { tap_pad(slot, button); } };
    const auto neutral = [&]
    {
        app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (unsigned int key = 1; key < static_cast<unsigned int>(Key::count); ++key)
        { app.keyboard()->on_key_up(static_cast<Key>(key)); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll();
        app.gamepads()->poll(); app.gamepads()->poll(); input.reset_actions();
    };
    const auto size_to = [&](Vector2I size)
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
    };
    PadButtonsMenu* page = nullptr;
    const auto open = [&](int first_connected, int owner = -1)
    {
        stack.clear(); stack.notify_activation(true); painter_scripted_pads::states = {};
        if (first_connected >= 0) { painter_scripted_pads::states[first_connected].connected = true; }
        neutral(); stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        auto child = std::make_unique<PadButtonsMenu>(&context, owner); page = child.get(); stack.push(std::move(child)); tick();
        require(stack.depth() == 2 && page->selected_slot() == (owner >= 0 ? owner : std::max(0, first_connected)),
            "Pad Jump did not choose pausing slot / first connected slot / disconnected Pad1 default");
    };
    // Jump's first four choices to the right of A; none conflicts with the default profile.
    constexpr std::array<PadControl, 4> choices{ PadControl::a, PadControl::b, PadControl::x, PadControl::y };
    const auto jump_of = [](const PadProfile& pad) { return pad[PadAction::jump].primary; };
    const auto with_jumps = [](InputBindings bindings, const std::array<PadControl, 4>& jumps)
    {
        for (size_t slot = 0; slot < jumps.size(); ++slot) { bindings.pads[slot][PadAction::jump].primary = jumps[slot]; }
        return bindings;
    };
    std::ofstream records(output / "pad-jump.csv");
    records << "case,width,height,slot,current,pending,stack_depth,updates\n";
    const auto copy_check = [&](const char* name, int slot, PadControl current, PadControl pending,
        const std::wstring& status, bool warning = false)
    {
        const auto& look = original.controls_settings(); const auto& style = original.menu(); const auto& art = look.art;
        const std::wstring& help = original.menu_text().pad_buttons.help;
        UiText help_text("independent-pad-help", help, look.help_font, look.controls_position, app.render_resources(), art.ink);
        const RectangleF raw_help = help_text.bounds();
        help_text.set_scale(std::min({ 1.0f, look.controls_size.x / raw_help.width, look.controls_size.y / raw_help.height }));
        const RectangleF panel((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - look.box_size) / 2.0f, look.box_size);
        UiText status_text("independent-pad-status", status, look.help_font, look.status_position, app.render_resources(),
            warning ? original.main_menu().options_warning_colour : art.focus_ink);
        const RectangleF raw_status = status_text.bounds();
        status_text.set_scale(std::min({ 1.0f,
            (panel.right() - art.shadow_offset - art.focus_padding.x - look.status_position.x) / raw_status.width,
            (style.footer_y - look.status_position.y) / raw_status.height }));
        require(RectangleF(look.controls_position, look.controls_size).contains(help_text.bounds()) && panel.contains(status_text.bounds()),
            "Pad Buttons actual-font fitted help/status escaped the paper");
        const Frame actual = main_menu_frame(app);
        std::srand(7331); const int next = std::rand(); std::srand(7331);
        require(actual.pixels == main_menu_frame(app).pixels && std::rand() == next,
            "Frozen Pad Jump draw mutated pixels or consumed RNG");
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app.viewport_manager()->fullscreen_viewport());
        list.set_camera(camera); list.set_filter(TextureFilter::point);
        Canvas paper(app.render_resources(), art.paper); paper.draw(list, camera.visible_rectangle(app.viewport_manager()->fullscreen_viewport()));
        FocusGroup empty; const MenuPaperArtwork artwork(panel, app.render_resources(), &style, &art, &empty);
        artwork.draw(list); list.set_filter(TextureFilter::linear); help_text.draw(list); status_text.draw(list);
        const Frame expected = read(renderer);
        const std::string stem = std::string("pad-jump-") + name + '-' + std::to_string(slot) + '-' +
            std::to_string(actual.width) + 'x' + std::to_string(actual.height);
        for (const auto& word : { std::pair{ &help_text, "help" }, std::pair{ &status_text, "status" } })
        {
            const RectangleI crop = countdown_crop_bounds(camera.calculate_view_rectangle(word.first->bounds()));
            const Frame observed = foreground_crop(actual, crop, 0), wanted = foreground_crop(expected, crop, 0);
            if (observed.pixels != wanted.pixels)
            {
                save(observed, output / (stem + '-' + word.second + "-failure-actual.ppm"));
                save(wanted, output / (stem + '-' + word.second + "-failure-expected.ppm"));
                throw std::runtime_error("Actual Pad Jump help/status differs from independent native glyphs");
            }
        }
        if (std::string_view(name) == "entry" || std::string_view(name) == "failed-write")
        { save(actual, output / (stem + ".ppm")); }
        records << name << ',' << actual.width << ',' << actual.height << ',' << slot << ',' <<
            static_cast<int>(current) << ',' << static_cast<int>(pending) << ',' << stack.depth() << ',' << updates << '\n'; ++copies;
    };
    const auto& copy = original.menu_text().pad_buttons;
    for (Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080) })
    {
        size_to(size);
        for (int slot = 0; slot < input_slots::pad_count; ++slot)
        {
            for (int choice = 0; choice < static_cast<int>(choices.size()); ++choice)
            {
                require(preferences.set_input_bindings_and_save(initial), "Cannot reset private per-slot page choices");
                open(slot); const std::string saved = bytes(); tap(Key::down); move(Key::right, choice);
                require(jump_of(page->pending_profile()) == choices[choice] && preferences.input_bindings() == initial && bytes() == saved,
                    "Pad Buttons selector applied before Apply or used the roster index");
                copy_check(choice == 0 ? "entry" : "pending", slot, PadControl::a, choices[choice], copy.pending_status);
                move(Key::down, 7); tap(Key::enter);
                InputBindings expected = initial; expected.pads[slot][PadAction::jump].primary = choices[choice];
                Save reloaded; reloaded.load_save_file(); ++reloads;
                require(preferences.input_bindings() == expected && reloaded.input_bindings() == expected &&
                    reloaded.play_settings().music_volume == 0 && reloaded.play_settings().sfx_volume == 0,
                    "Actual Pad Jump Apply/reload changed another slot, keyboard or audio");
                copy_check("applied", slot, choices[choice], choices[choice], copy.saved_status);
                tap(Key::escape); require(stack.depth() == 1, "Pad Jump Back did not close only its own page");
            }
        }
    }
    // Two pending slots survive selector changes; unrelated active choices
    // changed while this page was open are merged from the latest save.
    require(preferences.set_input_bindings_and_save(initial), "Cannot prepare Pad Jump merge fixture"); open(-1);
    tap(Key::down); move(Key::right, 2); tap(Key::up); tap(Key::right); tap(Key::down); tap(Key::right);
    InputBindings external = preferences.input_bindings(); external.keyboard_jump = Key::k;
    external.pads[3][PadAction::jump].primary = PadControl::y;
    require(preferences.set_input_bindings_and_save(external), "Cannot seed independent newer binding preferences");
    move(Key::down, 7); tap(Key::enter);
    require(preferences.input_bindings().keyboard_jump == Key::k && preferences.input_bindings() ==
        with_jumps(external, { PadControl::x, PadControl::b, PadControl::a, PadControl::y }),
        "Pad Buttons Apply replaced a newer keyboard/unedited slot or lost an edited slot");
    tap(Key::up); tap(Key::enter); // Reset selected Pad2 only, still pending.
    require(page->pending_profile() == PadProfile{} && jump_of(preferences.input_bindings().pads[1]) == PadControl::b,
        "Reset applied immediately instead of proposing the default profile for the selected pad");
    const std::string before_back = bytes(); tap(Key::escape);
    require(bytes() == before_back && jump_of(preferences.input_bindings().pads[1]) == PadControl::b,
        "Back persisted a reset proposal");
    // Shoot left from RT skips LT, which Jump already uses, and lands on RB.
    require(preferences.set_input_bindings_and_save(initial), "Cannot prepare Pad Buttons conflict fixture"); open(0);
    move(Key::down, 3); tap(Key::left);
    require(page->pending_profile()[PadAction::shoot].primary == PadControl::right_shoulder &&
        page->pending_profile()[PadAction::jump] == PadProfile{}[PadAction::jump] && preferences.input_bindings() == initial,
        "Choosing a control in use did not skip it, or changed Jump or the save before Apply");
    copy_check("conflict", 0, PadControl::a, PadControl::a, copy.conflict_status, true);
    move(Key::down, 5); tap(Key::enter);
    {
        Save reloaded; reloaded.load_save_file(); ++reloads;
        require(reloaded.input_bindings().pads[0][PadAction::shoot].primary == PadControl::right_shoulder &&
            reloaded.input_bindings().pads[1] == PadProfile{}, "Rebound Shoot did not apply and reload for only its pad");
    }
    tap(Key::escape);
    // One controlled file refusal, followed by the same actual Apply widget.
    require(preferences.set_input_bindings_and_save(initial), "Cannot prepare Pad Jump failed write"); open(-1);
    tap(Key::down); tap(Key::right); move(Key::down, 7); const std::string old = bytes();
    require(std::filesystem::create_directory(blocked), "Cannot block private Pad Jump temporary write");
    tap(Key::enter); require(jump_of(preferences.input_bindings().pads[0]) == PadControl::b && bytes() == old,
        "Failed Pad Buttons write rolled back active choice or changed old JSON");
    copy_check("failed-write", 0, PadControl::b, PadControl::b, original.menu_text().status.active_save_failed, true);
    require(std::filesystem::remove(blocked), "Cannot clear private Pad Jump temporary-write refusal");
    tap(Key::enter); Save retry; retry.load_save_file(); ++reloads;
    require(jump_of(retry.input_bindings().pads[0]) == PadControl::b, "Actual Pad Buttons Apply could not retry persistence");
    tap(Key::escape);
    // The real Controls entry opens its third action, with parent edits pending.
    stack.clear(); painter_scripted_pads::states = {}; neutral(); stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
    stack.push(std::make_unique<ControlsSettingsMenu>(&context)); tick(); move(Key::down, 4); tap(Key::right);
    move(Key::up, 2); tap(Key::enter); tick(); require(stack.depth() == 3, "Actual Controls Pad Jump entry did not push its child");
    const Frame retained = main_menu_frame(app);
    const_cast<MenuText&>(presentation.menu_text()).pad_buttons.help = L"Help for a later page.";
    tick(); require(main_menu_frame(app).pixels == retained.pixels, "Live text replacement changed existing Pad Jump copy");
    tap(Key::escape);
    tick(); // The resumed parent must observe neutral before fresh direction input.
    move(Key::down, 7); tap(Key::enter);
    require(preferences.play_settings().music_volume == 10 && jump_of(preferences.input_bindings().pads[0]) == PadControl::b,
        "Parent Controls Apply lost pending audio or independently applied pad choices");
    presentation = original;

    // A nonowner cannot drive a connected owner's page. Default selection
    // follows the physical owner even when an earlier slot is connected.
    InputBindings owned_preferences = preferences.input_bindings(); owned_preferences.pads[3][PadAction::jump].primary = PadControl::b;
    require(preferences.set_input_bindings_and_save(owned_preferences), "Cannot prepare selected-B menu Back control");
    open(1, 3); painter_scripted_pads::states[3].connected = true; tick(); tick();
    const int owned_slot = page->selected_slot(); const PadProfile owned_pending = page->pending_profile(); const std::string owned_bytes = bytes();
    tap_pad(1, GamepadButton::dpad_down); tap_pad(1, GamepadButton::dpad_right); tap_pad(1, GamepadButton::a);
    tap(Key::down); tap(Key::right); tap(Key::enter);
    require(page->selected_slot() == owned_slot && page->pending_profile() == owned_pending && bytes() == owned_bytes && stack.depth() == 2,
        "Another pad/keyboard drove the connected owner's Pad Jump page");
    tap_pad(3, GamepadButton::b); require(stack.depth() == 1, "Menu B stopped being Back after pad Jump preferences changed");

    int paused_reminders = 0, mixed_reminders = 0;
    require(preferences.set_play_settings_and_save(quiet), "Cannot quiet private Pad Jump reminder matches");
    for (Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080) })
    {
        require(preferences.set_input_bindings_and_save(initial), "Cannot reset paused Pad Jump bindings");
        size_to(size); stack.clear(); stack.notify_activation(true); painter_scripted_pads::states = {};
        painter_scripted_pads::states[3].connected = true; neutral();
        settings.set_stage("movement_course"); settings.set_duration(60.0f); settings.set_player_count(1);
        settings.begin_input_claims(3); settings.set_screen_layout(ScreenLayout::one_player);
        settings.set_player_team(0, PlayerTeam::a); settings.set_player_weapon(0, WeaponType::sprayer);
        stack.push(std::make_unique<ResumeProbeRoot>());
        auto owned = std::make_unique<GameLevel>(&data, settings); GameLevel* game = owned.get();
        stack.push<EndMenuAction>(std::move(owned), [](const EndMenuAction&)
        { throw std::runtime_error("Pad Jump reminder fixture unexpectedly ended its match"); });
        const auto draw_game = [&]
        { app.renderer()->begin_frame(); game->draw(*app.renderer()); return read(*app.renderer()); };
        const Frame before = draw_game();
        require(game->level().gamepad_controls_text().find(L"A / LT  Jump") != std::wstring_view::npos,
            "Default pad reminder lost its actual A / LT mapping");
        tap_pad(3, GamepadButton::start); tick(); require(stack.depth() == 3, "Mapped pad Start did not open actual Pause");
        move_pad(3, GamepadButton::dpad_down, 2); tap_pad(3, GamepadButton::a); tick();
        require(stack.depth() == 4, "Paused Pad Jump route did not open actual Options");
        move_pad(3, GamepadButton::dpad_down, 3); tap_pad(3, GamepadButton::a); tick();
        require(stack.depth() == 5, "Paused Pad Jump route did not open actual Controls");
        move_pad(3, GamepadButton::dpad_down, 2); tap_pad(3, GamepadButton::a); tick();
        require(stack.depth() == 6, "Paused Controls did not open actual Pad Jump child");
        const auto frozen = resume_observation(game->level());
#if COLOURWARS_DEV_TOOLS
        const auto fingerprint = MatchFingerprint::of(game->level(), 0);
#endif
        tap_pad(3, GamepadButton::dpad_down); move_pad(3, GamepadButton::dpad_right, 2);
        move_pad(3, GamepadButton::dpad_down, 7); tap_pad(3, GamepadButton::a); tick();
        require(jump_of(preferences.input_bindings().pads[3]) == PadControl::x &&
            jump_of(preferences.input_bindings().pads[0]) == PadControl::a,
            "Paused Pad Jump defaulted to a player/view index instead of physical owner slot3");
        const Frame changed = draw_game();
        const RectangleF card = game->level().gamepad_controls_card_bounds();
        const RectangleF bounds = game->level().gamepad_controls_text_bounds();
        require(game->level().gamepad_controls_text().find(L"X / LT  Jump") != std::wstring_view::npos &&
            card.width > 0.0f && card.contains(bounds) && RectangleF(Vector2F::ZERO, Vector2F(size)).contains(card) &&
            game->resume_remaining() == 0.0f && frozen == resume_observation(game->level()),
            "Paused pad reminder missed its live mapping, escaped its card or advanced the covered match");
        const RectangleI crop = countdown_crop_bounds(card);
        require(foreground_crop(before, crop, 0).pixels != foreground_crop(changed, crop, 0).pixels &&
            changed.pixels == draw_game().pixels, "Paused pad mapping produced no native text delta or unstable redraw");
        require(preferences.set_input_bindings_and_save(initial), "Cannot reverse only the private paused pad mapping");
        const Frame reversed = draw_game();
        const RectangleI reversed_crop = countdown_crop_bounds(game->level().gamepad_controls_card_bounds());
        require(foreground_crop(reversed, reversed_crop, 0).pixels == foreground_crop(before, reversed_crop, 0).pixels &&
            frozen == resume_observation(game->level()),
            "Paused pad mapping reversal changed simulation or failed to restore original native reminder");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(game->level(), 0)).empty(),
            "Pad Jump page/reminder refresh altered the covered match replay fingerprint");
#endif
        save(foreground_crop(changed, crop), output / ("pad-jump-paused-reminder-" + std::to_string(size.x) + ".ppm"));
        move_pad(3, GamepadButton::b, 3);
        require(stack.depth() == 3 && game->resume_remaining() == 0.0f && frozen == resume_observation(game->level()),
            "Returning through Pad Jump/Controls/Options resumed or changed the covered match");
        ++paused_reminders; stack.clear();

        const InputBindings mixed = with_jumps(initial, choices);
        require(preferences.set_input_bindings_and_save(mixed), "Cannot seed mixed four-pad reminder choices");
        struct RestorePadReminderLayout
        {
            ViewportManager& viewports; ScreenLayout layout;
            ~RestorePadReminderLayout() { this->viewports.set_layout(this->layout); }
        } layout{ *app.viewport_manager(), app.viewport_manager()->layout() };
        LevelBuilder builder(app.viewport_manager(), app.render_resources(), app.audio_resources(),
            &preferences.play_settings(), &levels, &tuning, &presentation,
            app.resolution_manager(), app.thread_pool(), app.partitioner(), &preferences.input_bindings());
        auto level = build(app, builder, 4, TeamColourPair::blue_yellow);
        const Frame four = render(*level, *app.renderer());
        for (const std::wstring_view line : { L"Pad 4: Y / LT Jump, RT Shoot, Start Pause", L"Pad 2: B / LT Jump, RT Shoot, Start Pause",
            L"Pad 3: X / LT Jump, RT Shoot, Start Pause", L"Pad 1: A / LT Jump, RT Shoot, Start Pause" })
        { require(level->gamepad_controls_text().find(line) != std::wstring_view::npos,
            "Mixed four-pad opening reminder lost a physical slot's selected face or LT"); }
        const RectangleF four_card = level->gamepad_controls_card_bounds(), four_text = level->gamepad_controls_text_bounds();
        require(four_text.width > 0.0f && four_text.height > 0.0f && four_card.contains(four_text) &&
            RectangleF(Vector2F::ZERO, Vector2F(size)).contains(four_card),
            "Mixed four-pad native-font reminder escaped its card or supported screen");
        require(four.pixels == render(*level, *app.renderer()).pixels, "Mixed four-pad reminder changed on frozen redraw");
        save(foreground_crop(four, countdown_crop_bounds(four_card)), output /
            ("pad-jump-four-pad-reminder-" + std::to_string(size.x) + ".ppm"));
        ++mixed_reminders;
    }
#if COLOURWARS_DEV_TOOLS
    // The production tape stores inputs after physical-slot mapping. Rebuild
    // from decoded setup with different saved face choices; real live inputs
    // must now differ while ReplayPlayer still restores every recorded seat.
    stack.clear();
    std::ifstream tuning_source(content / "tuning.json", std::ios::binary);
    require(tuning_source.good(), "Cannot read tuning for bounded mapped-input replay");
    const std::string tuning_bytes(std::istreambuf_iterator<char>(tuning_source), {});
    rapidjson::Document tuning_json; tuning_json.Parse(tuning_bytes.c_str());
    require(!tuning_json.HasParseError(), "Cannot parse mapped-input replay tuning");
    tuning_json["match"]["countdown"].SetFloat(dt / 4.0f);
    tuning_json["match"]["opening"]["push"].SetFloat(dt / 4.0f);
    rapidjson::StringBuffer tuning_buffer; rapidjson::Writer<rapidjson::StringBuffer> tuning_writer(tuning_buffer);
    require(tuning_json.Accept(tuning_writer), "Cannot serialize short-countdown replay tuning");
    const GameTuning replay_tuning(parse_json(tuning_buffer.GetString(), "pad-jump-replay-short-countdown"));
    ReplayFile header; header.build = "actual-pad-reader-public-loader";
    header.dt_bits = std::bit_cast<std::uint32_t>(dt); header.seed = 7337u;
    header.tuning_hash = fnv1a(tuning_buffer.GetString());
    header.settings.set_stage("movement_course"); header.settings.set_duration(60.0f);
    header.settings.set_player_count(2); header.settings.begin_input_claims(3);
    require(header.settings.claim_input_slot(1), "Cannot build sparse reversed replay roster");
    header.settings.set_screen_layout(ScreenLayout::two_player);
    header.settings.set_player_team(0, PlayerTeam::a); header.settings.set_player_team(1, PlayerTeam::b);
    header.settings.set_player_weapon(0, WeaponType::sprayer); header.settings.set_player_weapon(1, WeaponType::sprayer);
    const InputBindings recording_bindings = with_jumps(initial, { PadControl::a, PadControl::b, PadControl::a, PadControl::y });
    require(preferences.set_input_bindings_and_save(recording_bindings), "Cannot save actual recording pad choices");
    const auto snapshots = [&](int step)
    {
        painter_scripted_pads::states = {};
        auto& first = painter_scripted_pads::states[1]; auto& second = painter_scripted_pads::states[3];
        first.connected = second.connected = true;
        const int phase = step < 0 ? -1 : step % 120;
        const auto mask = [](GamepadButton button)
        { return static_cast<uint16_t>(1u << static_cast<unsigned int>(button)); };
        if (phase >= 2 && phase <= 5) { first.buttons = second.buttons = mask(GamepadButton::a); }
        if (phase >= 10 && phase < 30) { first.buttons = mask(GamepadButton::b); }
        if (phase >= 45 && phase < 65) { second.buttons = mask(GamepadButton::y); }
        first.left_trigger = phase >= 20 && phase <= 22 ? 1.0f : 0.0f;
        first.right_trigger = phase >= 80 && phase < 84 ? 1.0f : 0.0f;
        first.right_stick = { 1.0f, 0.0f }; second.right_stick = { -1.0f, 0.0f };
        app.gamepads()->poll();
    };
    struct RestoreMappedReplayLayout
    {
        ViewportManager& viewports; ScreenLayout layout;
        ~RestoreMappedReplayLayout() { this->viewports.set_layout(this->layout); }
    } replay_layout{ *app.viewport_manager(), app.viewport_manager()->layout() };
    const auto fresh = [&](const MatchSetup& setup, unsigned int seed)
    {
        app.viewport_manager()->set_layout(setup.screen_layout());
        LevelBuilder builder(app.viewport_manager(), app.render_resources(), app.audio_resources(),
            &preferences.play_settings(), &levels, &replay_tuning, &presentation,
            app.resolution_manager(), app.thread_pool(), app.partitioner(), &preferences.input_bindings());
        std::srand(seed);
        return builder.build_level(setup, TeamColourPair::blue_yellow, OpeningMode::off);
    };
    snapshots(-1); snapshots(-1); neutral();
    PlayerInput mapped(app.gamepads(), app.keyboard(), app.mouse(), &replay_tuning,
        &preferences.play_settings(), &preferences.input_bindings());
    auto recorded = fresh(header.settings, *header.seed);
    const auto initial_print = MatchFingerprint::of(*recorded, 0);
    constexpr int replay_length = 360;
    ReplayRecorder recorder("actual-pad-reader", header, nullptr, replay_length);
    std::vector<MatchFingerprint> prints; prints.reserve(replay_length);
    std::ofstream replay_trace(output / "pad-jump-replay.csv");
    replay_trace << "update,hash,seat0_jump,seat1_jump,raw_remap_differs\n";
    for (int step = 0; step < replay_length; ++step)
    {
        snapshots(step); const auto inputs = mapped.player_inputs();
        const int phase = step % 120;
        const bool first_held = phase >= 10 && phase < 30;
        const bool first_pressed = phase == 10 || (phase == 20 && replay_tuning.input().trigger_jump_threshold < 1.0f);
        const bool second_held = phase >= 45 && phase < 65, second_pressed = phase == 45;
        require(inputs[1].jump_held == first_held && inputs[1].jump_pressed == first_pressed &&
            inputs[3].jump_held == second_held && inputs[3].jump_pressed == second_pressed,
            "Actual recording reader differs from independent chosen-face/former-A/LT Jump oracle");
        acceptance::update_from_slots(*recorded, inputs, dt, &recorder);
        require(recorder.file().inputs.size() == 2 && recorder.file().inputs[0].size() == static_cast<size_t>(step + 1) &&
            replay_codec::same(recorder.file().inputs[0].back(), inputs[3]) &&
            replay_codec::same(recorder.file().inputs[1].back(), inputs[1]),
            "Production recorder did not store actual reader outputs after sparse slot-to-seat mapping");
        prints.push_back(MatchFingerprint::of(*recorded, step + 1));
        if ((step + 1) % ReplayFile::checkpoint_interval == 0) { recorder.add_checkpoint(prints.back()); }
    }
    recorder.file().updates = replay_length; recorder.file().final_print = prints.back();
    const auto tape = profile / "mapped-pad-replay.json"; recorder.file().write(tape);
    const unsigned int terminal_rng = static_cast<unsigned int>(std::rand()); recorded.reset();
    const ReplayFile decoded = ReplayFile::read(tape);
    require(decoded.inputs.size() == 2 && decoded.inputs[0].size() == replay_length &&
        decoded.inputs[1].size() == replay_length && decoded.updates == replay_length && decoded.seed &&
        decoded.checkpoints.size() == 6 && decoded.settings.player_settings()[0].num == 3 &&
        decoded.settings.player_settings()[1].num == 1, "Mapped tape lost decoded setup, six checkpoints or complete seat streams");
    const InputBindings playback_bindings = with_jumps(initial, { PadControl::a, PadControl::x, PadControl::a, PadControl::b });
    require(preferences.set_input_bindings_and_save(playback_bindings), "Cannot persist changed replay-time face choices");
    Save replay_save; replay_save.load_save_file();
    require(replay_save.input_bindings() == playback_bindings && playback_bindings != recording_bindings,
        "Replay remapping control did not persist different face choices");
    snapshots(-1); snapshots(-1);
    auto playback = fresh(decoded.settings, *decoded.seed);
    require(MatchFingerprint::difference(initial_print, MatchFingerprint::of(*playback, 0)).empty(),
        "Fresh decoded-setup rebuild changed initial simulation when only saved bindings changed");
    ReplayPlayer player("actual-pad-reader", decoded);
    int raw_differences = 0; size_t checkpoint = 0;
    for (int step = 0; step < replay_length; ++step)
    {
        snapshots(step); const auto live = mapped.player_inputs();
        const bool different = live[3].jump_pressed != decoded.inputs[0][step].jump_pressed ||
            live[3].jump_held != decoded.inputs[0][step].jump_held ||
            live[1].jump_pressed != decoded.inputs[1][step].jump_pressed || live[1].jump_held != decoded.inputs[1][step].jump_held;
        if (different) { ++raw_differences; }
        acceptance::update_from_slots(*playback, live, std::bit_cast<float>(decoded.dt_bits), &player);
        const auto print = MatchFingerprint::of(*playback, step + 1);
        require(MatchFingerprint::difference(prints[step], print).empty() &&
            replay_codec::same(playback->player(0).input(), decoded.inputs[0][step]) &&
            replay_codec::same(playback->player(1).input(), decoded.inputs[1][step]),
            "Changed live bindings altered a recorded seat input or an exact playback fingerprint");
        if ((step + 1) % ReplayFile::checkpoint_interval == 0)
        {
            require(decoded.checkpoints[checkpoint].update == step + 1 && decoded.checkpoints[checkpoint].hash == print.hash,
                "Mapped-input playback checkpoint differs"); ++checkpoint;
        }
        replay_trace << step + 1 << ',' << hex64(print.hash) << ',' << decoded.inputs[0][step].jump_pressed << ',' <<
            decoded.inputs[1][step].jump_pressed << ',' << different << '\n';
    }
    require(raw_differences > 60 && checkpoint == 6 && replay_trace.good() &&
        MatchFingerprint::difference(decoded.final_print, MatchFingerprint::of(*playback, replay_length)).empty() &&
        static_cast<unsigned int>(std::rand()) == terminal_rng,
        "Mapped replay did not exercise changed live input, exact final state/checkpoints or terminal RNG");
    std::cout << "PASS Pad Jump mapped replay:360 actual-reader post-mapping updates, six checkpoints, persisted alternate "
        "face choices differ on " << raw_differences << " live frames; decoded public rebuild preserves every seat/fingerprint/final/RNG\n";
#else
    std::cout << "LIMIT Pad Jump post-mapping replay requires Debug developer tools; native page/reader scope runs in both presets\n";
#endif
    require(copies == 66 && reloads == 34 && records.good(), "Pad Buttons native matrix missed its bounded page cases");
    require(paused_reminders == 2 && mixed_reminders == 2, "Pad Jump reminder matrix missed native size/roster cases");
    std::cout << "PASS Pad Buttons native:32 physical-slot/face Apply reloads, a skipped-conflict Shoot Apply, "
        "66 independently fitted help/status pairs; pending multi-slot merge, reset/discard, failed-write retry, Controls entry/parent audio, captured text and owner isolation; "
        "two actual owned paused countdown refresh/reversal routes and two mixed4-pad reminder captures; "
        "private snapshots/save at720/1080, no physical-device or human acceptance\n";
}
#endif
