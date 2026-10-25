// A bounded readout fixture, independent of the broad Pad sticks navigation
// matrix. Numeric expectations are supplied literals, never the formatter or
// stick_response helper being exercised by the actual page.
void stick_readout_matrix(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output)
{
    const Presentation original = presentation; Presentation replacement = original;
    auto& words = const_cast<MenuText&>(replacement.menu_text()).stick_readouts;
    words.connected = L"{device}: live"; words.disconnected = L"{device}: absent";
    words.raw_movement = L"Raw left {value}%"; words.movement_response = L"Output move {value}%";
    words.aim_inactive = L"Stick X = {raw_x}%\nStick Y = {raw_y}%\nAim idle\nHollow = raw\nSolid = aim";
    words.aim_active = L"Stick X = {raw_x}%\nStick Y = {raw_y}%\nDirection X = {aim_x}%\nDirection Y = {aim_y}%\nHollow = raw\nSolid = aim";
    const_cast<ControlsSettingsPresentation&>(replacement.controls_settings()).help_font = "courier_prime_bold_16";
    replacement.validate_resources(*app.render_resources(), *app.audio_resources());
    const NestedSaveProfile isolated(output / "stick-readout-profile");
    const auto save_path = output / "stick-readout-profile/ColourWars/save_data.json";
    Save preferences; PlaySettings play = preferences.play_settings();
    play.music_volume = play.sfx_volume = 0; play.reduce_effects = true;
    for (auto& stick : play.sticks) { stick = { 20, 50 }; }
    play.sticks[3] = { -1, -1 };
    require(tuning.input().movement_deadzone == 0.5f && play.aim_deadzone() == 0.5f,
        "The independent inherited-deadzone fixture needs the authored/default 50 percent cutoffs");
    require(preferences.set_play_settings_and_save(play), "Cannot seed the private stick readout profile");
    const auto bytes = [&]
    {
        std::ifstream file(save_path, std::ios::binary);
        require(static_cast<bool>(file), "Cannot read the private stick readout save");
        return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    };
    const std::string saved = bytes();
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MenuLevelSettings settings;
    settings.set_player_count(2); settings.begin_input_claims(4);
    require(settings.claim_input_slot(3), "Cannot specify the readout fixture's retained sparse roster");
    settings.set_stage("movement_course"); settings.set_best_of(3); settings.set_duration(73.0f);
    settings.set_screen_layout(ScreenLayout::two_player);
    settings.set_player_team(0, PlayerTeam::a); settings.set_player_team(1, PlayerTeam::b);
    const MenuLevelSettings retained = settings;
    MenuContext context(&data, &input, &settings); StateContext& stack = app;
    struct RestoreReadouts
    {
        StateContext& stack; Presentation& look; Presentation original;
        ~RestoreReadouts() { this->stack.clear(); painter_scripted_pads::states = {}; this->look = this->original; }
    } restore{ stack, presentation, original };
    int ticks = 0, captures = 0, rows = 0, selected = 0;
    const auto unchanged = [&]
    {
        require(bytes() == saved, "A stick readout refresh or pending preview wrote the save");
        const PlaySettings& actual = preferences.play_settings();
        for (size_t slot = 0; slot < play.sticks.size(); ++slot)
        {
            require(actual.sticks[slot].movement_deadzone == play.sticks[slot].movement_deadzone &&
                actual.sticks[slot].aim_deadzone == play.sticks[slot].aim_deadzone,
                "A pending stick readout preview changed active deadzones");
        }
        require(settings.player_count() == retained.player_count() && settings.stage() == retained.stage() &&
            settings.duration() == retained.duration() && settings.best_of() == retained.best_of() &&
            settings.screen_layout() == retained.screen_layout(), "Stick readouts changed match choices");
        const auto actual_seats = settings.player_settings(), expected_seats = retained.player_settings();
        for (size_t seat = 0; seat < expected_seats.size(); ++seat)
        {
            require(actual_seats[seat].num == expected_seats[seat].num && actual_seats[seat].team == expected_seats[seat].team &&
                actual_seats[seat].weapon == expected_seats[seat].weapon, "Stick readouts changed sparse roster choices");
        }
    };
    const auto tick = [&]
    {
        require(++ticks <= 180, "Stick readouts exceeded the bounded 180 update budget");
        app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.0f);
    };
    const auto tap = [&](Key key)
        { app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick(); };
    const auto open = [&]
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::enter, Key::space, Key::escape, Key::up, Key::down, Key::left, Key::right })
            { app.keyboard()->on_key_up(key); }
        painter_scripted_pads::states = {};
        for (auto& state : painter_scripted_pads::states) { state.connected = true; }
        app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll();
        app.mouse()->poll(); app.mouse()->poll(); input.reset_actions(); selected = 0;
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        stack.push(std::make_unique<StickSettingsMenu>(&context)); tick(); tick();
        require(stack.depth() == 2, "Actual StickSettingsMenu consumed neutral entry input");
    };
    const auto choose = [&](int slot)
    {
        require(slot >= selected && slot < 4, "Readout fixture only cycles Pad forward from the first row");
        while (selected < slot) { tap(Key::right); ++selected; }
    };
    const auto sample = [&](bool connected, Vector2F left, Vector2F right)
    {
        auto& pad = painter_scripted_pads::states[static_cast<size_t>(selected)];
        pad.connected = connected; pad.left_stick = left; pad.right_stick = right; tick();
    };
    using Copy = std::array<std::wstring, 4>;
    const auto frame = [&] { return main_menu_frame(app); };
    std::ofstream records(output / "stick-readouts.csv");
    records << "case,role,slot,width,height,font,x,y,text_width,text_height,save_unchanged,raster_matches\n";
    const auto compare = [&](const char* name, const Presentation& snapshot, const Copy& expected, unsigned capture_mask = 0u)
    {
        unchanged(); const Frame actual = frame(); const auto& look = snapshot.controls_settings();
        const auto& style = snapshot.menu(); const auto& view = look.stick_preview;
        const auto font = app.render_resources()->resolve_sprite_font(look.help_font);
        // This existing literal anchor is intentionally independent of the new
        // readout templates; changing their words must not move the tracks.
        const float label_height = app.render_resources()->measure_text(font, L"Move").y;
        const std::array<Vector2F, 4> positions{ look.controls_position,
            view.raw_movement.position() - Vector2F(0.0f, label_height + look.art.focus_padding.y),
            view.movement_response.position() - Vector2F(0.0f, label_height + look.art.focus_padding.y), view.aim_readout_position };
        constexpr std::array<const char*, 4> roles{ "connection", "raw-move", "response", "aim" };
        const RectangleF panel((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - look.box_size) / 2.0f, look.box_size);
        const RectangleF region(look.controls_position, look.controls_size);
        std::array<std::unique_ptr<UiText>, 4> literals;
        for (size_t role = 0; role < expected.size(); ++role)
        {
            literals[role] = std::make_unique<UiText>("independent-stick-readout", expected[role], look.help_font,
                positions[role], app.render_resources(), look.art.ink);
        }
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); const auto viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
        list.set_viewport(viewport); list.set_camera(camera); list.set_filter(TextureFilter::point);
        canvas.draw(list, camera.visible_rectangle(viewport)); FocusGroup none;
        const MenuPaperArtwork paper(panel, app.render_resources(), &style, &look.art, &none); paper.draw(list);
        list.set_filter(TextureFilter::linear);
        for (const auto& literal : literals) { literal->draw(list); }
        const Frame wanted = read(renderer);
        for (size_t role = 0; role < expected.size(); ++role)
        {
            const RectangleF text_bounds = literals[role]->bounds();
            require(panel.contains(text_bounds) && region.contains(text_bounds) && text_bounds.width > 0.0f && text_bounds.height > 0.0f,
                "Stick readout complete unscaled bounds escape the paper or preview region");
            // The historical anchor measures "Move", whereas actual glyphs
            // such as p can descend below that line height into the padding.
            // Require complete ink bounds to remain above the track itself.
            if (role == 1) { require(text_bounds.bottom() <= view.raw_movement.y,
                "Raw movement readout moved into its track"); }
            if (role == 2) { require(text_bounds.bottom() <= view.movement_response.y,
                "Movement response readout moved into its track"); }
            if (role == 3) { require(text_bounds.x >= view.aim.right() && text_bounds.bottom() <= view.association_position.y,
                "Aim readout overlaps the radial preview or association copy"); }
            RectangleF bounds = text_bounds; bounds.inflate(Vector2F(2.0f)); camera.calculate_view_rectangle(bounds);
            const RectangleI crop = countdown_crop_bounds(bounds);
            const Frame observed = foreground_crop(actual, crop, 0), reference = foreground_crop(wanted, crop, 0);
            const std::string stem = std::string("stick-readouts-") + name + '-' + roles[role] + '-' + std::to_string(actual.width);
            if (observed.pixels != reference.pixels)
            {
                if (captures <= 6) { save(observed, output / (stem + "-failure-actual.ppm"));
                    save(reference, output / (stem + "-failure-expected.ppm")); captures += 2; }
                std::cerr << stem << " crop=" << crop.x << ',' << crop.y << ',' << crop.width << ',' << crop.height << '\n';
                throw std::runtime_error("Actual stick readout differs from independent literal numeric copy");
            }
            records << name << ',' << roles[role] << ',' << selected << ',' << actual.width << ',' << actual.height << ',' << look.help_font << ',' <<
                text_bounds.x << ',' << text_bounds.y << ',' << text_bounds.width << ',' << text_bounds.height << ",true,true\n"; ++rows;
            if ((capture_mask & (1u << static_cast<unsigned int>(role))) != 0u)
            {
                require(captures < 8, "Stick readouts exceeded eight compact crops");
                save(observed, output / (stem + "-detail.ppm")); ++captures;
            }
        }
        std::srand(9353); const int next = std::rand(); std::srand(9353);
        require(actual.pixels == frame().pixels && std::rand() == next, "Frozen stick readout drawing changed pixels or RNG"); unchanged();
    };
    const Copy zero{ L"Pad 1 - connected", L"Left stick raw X: 0%", L"Move response: 0%",
        L"Raw X: 0%\nRaw Y: 0%\nNo new aim\nRaw: hollow\nAim: solid" };
    const Copy disconnected{ L"Pad 1 - disconnected", L"Left stick raw X: 0%", L"Move response: 0%", zero[3] };
    const Copy half_positive{ L"Pad 1 - connected", L"Left stick raw X: 1%", L"Move response: 0%",
        L"Raw X: -1%\nRaw Y: 1%\nNo new aim\nRaw: hollow\nAim: solid" };
    const Copy half_negative{ L"Pad 1 - connected", L"Left stick raw X: -2%", L"Move response: 0%",
        L"Raw X: 2%\nRaw Y: -2%\nNo new aim\nRaw: hollow\nAim: solid" };
    const Copy half_travel{ L"Pad 1 - connected", L"Left stick raw X: 50%", L"Move response: 38%",
        L"Raw X: 50%\nRaw Y: 0%\nNo new aim\nRaw: hollow\nAim: solid" };
    const Copy negative_half_travel{ L"Pad 1 - connected", L"Left stick raw X: -50%", L"Move response: -38%",
        L"Raw X: -50%\nRaw Y: 0%\nNo new aim\nRaw: hollow\nAim: solid" };
    const Copy cutoff{ L"Pad 1 - connected", L"Left stick raw X: 60%", L"Move response: 50%",
        L"Raw X: 30%\nRaw Y: 40%\nNo new aim\nRaw: hollow\nAim: solid" };
    const Copy axis{ L"Pad 2 - connected", L"Left stick raw X: 60%", L"Move response: 50%",
        L"Raw X: 100%\nRaw Y: 0%\nAim X: 100%\nAim Y: 0%\nRaw: hollow\nAim: solid" };
    const Copy diagonal{ L"Pad 3 - connected", L"Left stick raw X: -60%", L"Move response: -50%",
        L"Raw X: 60%\nRaw Y: 80%\nAim X: 60%\nAim Y: 80%\nRaw: hollow\nAim: solid" };
    const Copy negative_diagonal{ L"Pad 3 - connected", L"Left stick raw X: -60%", L"Move response: -50%",
        L"Raw X: -60%\nRaw Y: -80%\nAim X: -60%\nAim Y: -80%\nRaw: hollow\nAim: solid" };
    const Copy inherited{ L"Pad 4 - connected", L"Left stick raw X: 75%", L"Move response: 50%",
        L"Raw X: 52%\nRaw Y: 0%\nAim X: 100%\nAim Y: 0%\nRaw: hollow\nAim: solid" };
    Copy pending_move = inherited; pending_move[2] = L"Move response: 44%";
    Copy pending_aim = pending_move; pending_aim[3] = L"Raw X: 52%\nRaw Y: 0%\nNo new aim\nRaw: hollow\nAim: solid";
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080) })
    {
        presentation = original; app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y); open();
        compare("zero", original, zero);
        sample(false, { -1.0f, 0.0f }, { 1.0f, 1.0f }); compare("disconnected-stale-raw", original, disconnected);
        sample(true, { 0.0049f, 0.0f }, { -0.0049f, 0.0049f }); compare("below-positive-half-percent", original, zero);
        sample(true, { -0.0049f, 0.0f }, { 0.0049f, -0.0049f }); compare("below-negative-half-percent", original, zero);
        sample(true, { 0.005f, 0.0f }, { -0.005f, 0.005f }); compare("positive-half-percent", original, half_positive);
        sample(true, { -0.015f, 0.0f }, { 0.015f, -0.015f }); compare("negative-half-percent", original, half_negative);
        sample(true, { 0.5f, 0.0f }, { 0.5f, 0.0f }); compare("half-travel-axis", original, half_travel);
        sample(true, { -0.5f, 0.0f }, { -0.5f, 0.0f }); compare("negative-half-travel-axis", original, negative_half_travel);
        sample(true, { 0.6f, 0.0f }, { 0.3f, 0.4f }); compare("radial-cutoff", original, cutoff, size.x == 1920 ? 8u : 0u);
        choose(1); sample(true, { 0.6f, 0.0f }, { 1.0f, 0.0f }); compare("positive-axis", original, axis);
        choose(2); sample(true, { -0.6f, 0.0f }, { 0.6f, 0.8f }); compare("negative-move-diagonal-aim", original, diagonal, size.x == 1280 ? 15u : 8u);
        sample(true, { -0.6f, 0.0f }, { -0.6f, -0.8f }); compare("negative-active-aim", original, negative_diagonal);
        choose(3); sample(true, { 0.75f, 0.0f }, { 0.52f, 0.0f }); compare("inherited-deadzones", original, inherited);
        tap(Key::down); tap(Key::right); compare("pending-move-cutoff", original, pending_move);
        tap(Key::down); tap(Key::right); compare("pending-aim-cutoff", original, pending_aim);
        tap(Key::escape); require(stack.depth() == 1, "Back did not discard the actual pending Pad sticks page"); unchanged();
        open(); choose(3); sample(true, { 0.75f, 0.0f }, { 0.52f, 0.0f }); compare("discard-reopened", original, inherited);
    }
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720); presentation = original;
    open(); choose(2); sample(true, { -0.6f, 0.0f }, { 0.6f, 0.8f }); const Frame old_frame = frame();
    presentation = replacement; tick(); compare("assigned-keeps-old-active", original, diagonal);
    require(frame().pixels == old_frame.pixels, "An open Pad sticks page adopted assigned templates/help font");
    Copy old_inactive = cutoff; old_inactive[0] = L"Pad 3 - connected";
    sample(true, { 0.6f, 0.0f }, { 0.3f, 0.4f }); compare("old-inactive-refresh", original, old_inactive);
    Copy old_disconnected = disconnected; old_disconnected[0] = L"Pad 3 - disconnected";
    sample(false, { -1.0f, 0.0f }, { 1.0f, 1.0f }); compare("old-disconnected-refresh", original, old_disconnected);
    const Copy custom_active{ L"Pad 3: live", L"Raw left -60%", L"Output move -50%",
        L"Stick X = 60%\nStick Y = 80%\nDirection X = 60%\nDirection Y = 80%\nHollow = raw\nSolid = aim" };
    const Copy custom_inactive{ L"Pad 3: live", L"Raw left 60%", L"Output move 50%",
        L"Stick X = 30%\nStick Y = 40%\nAim idle\nHollow = raw\nSolid = aim" };
    const Copy custom_disconnected{ L"Pad 3: absent", L"Raw left 0%", L"Output move 0%",
        L"Stick X = 0%\nStick Y = 0%\nAim idle\nHollow = raw\nSolid = aim" };
    open(); choose(2); sample(true, { -0.6f, 0.0f }, { 0.6f, 0.8f }); compare("reopened-custom-active", replacement, custom_active, 9u);
    const Frame custom_frame = frame(); require(custom_frame.pixels != old_frame.pixels, "Reopened Pad sticks did not adopt custom templates/help font");
    sample(true, { 0.6f, 0.0f }, { 0.3f, 0.4f }); compare("custom-inactive-refresh", replacement, custom_inactive);
    sample(false, { -1.0f, 0.0f }, { 1.0f, 1.0f }); compare("custom-disconnected-refresh", replacement, custom_disconnected);
    sample(true, { -0.6f, 0.0f }, { 0.6f, 0.8f }); presentation = original; tick();
    compare("restored-keeps-custom", replacement, custom_active);
    require(frame().pixels == custom_frame.pixels, "An open custom Pad sticks page adopted restored templates/help font");
    sample(true, { 0.6f, 0.0f }, { 0.3f, 0.4f }); compare("restored-custom-inactive-refresh", replacement, custom_inactive);
    open(); choose(2); sample(true, { -0.6f, 0.0f }, { 0.6f, 0.8f }); compare("reopened-original-reversed", original, diagonal);
    require(frame().pixels == old_frame.pixels, "Reopening did not restore original readout copy/help font/geometry"); unchanged();
    require(captures == 8 && rows > 0 && records.good(), "Stick readout evidence is incomplete or exceeded its crop cap");
    std::cout << "PASS stick readouts actual page: independent literal numeric copy, all four slots, zero/disconnected/negative/half-percent rounding, active/inactive aim, effective pending deadzones/discard without save, captured templates/help font across refresh/reopen/reversal; " <<
        rows << " CSV checks, " << ticks << " updates, eight compact crops; no physical-pad, foreground, listening or human readability acceptance\n";
}
