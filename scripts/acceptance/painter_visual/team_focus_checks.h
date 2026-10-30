// Compact semantic-cursor coverage. The expected choices/readiness and icon
// rasters never consult FocusGroup or geometric navigation.
void team_focus_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    const Presentation original = presentation; Presentation captured = original;
    Save preferences; SettingsSession preferences_session(preferences); auto& play = const_cast<PlaySettings&>(preferences_session.play_settings());
    play.music_volume = play.sfx_volume = 0; play.reduce_effects = false;
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false } };
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_settings_session(&preferences_session);
    const StageCatalog stage_catalog = acceptance::catalog_of(levels, stages, { "movement_course" }); data.set_stage_catalog(&stage_catalog); data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MatchSetup settings, initial;
    MenuContext context(&data, &input, &settings); StateContext& stack = app;
    bool closed = false, warning = false; int ticks = 0, captures = 0, records_written = 0;
    std::wstring expected_title = L"Team Select";
    std::vector<PlayerTeam> teams; std::vector<bool> ready;
    struct RestoreTeamFocus
    {
        StateContext& stack; Presentation& look; Presentation original;
        ~RestoreTeamFocus() { this->stack.clear(); painter_scripted_pads::states = {}; this->look = this->original; }
    } restore{ stack, presentation, original };
    const std::array<int, 4> slots{ 3, input_slots::keyboard_and_mouse, 0, 2 };
    const std::array<std::wstring, 4> devices{ L"Pad 4", L"Keyboard/mouse", L"Pad 1", L"Pad 3" };
    const std::array<WeaponType, 4> tools{ WeaponType::sprayer, WeaponType::sniper, WeaponType::roller, WeaponType::bucket };
    const auto bit = [](GamepadButton button) { return static_cast<uint16_t>(1u << static_cast<unsigned int>(button)); };
    const auto tick = [&](float elapsed = 0.0f)
    {
        require(++ticks <= 240, "Team focus exceeded its bounded 240 update budget");
        app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed);
    };
    const auto hold = [&](int seat, GamepadButton button, bool down)
    {
        const int slot = slots[static_cast<size_t>(seat)];
        if (slot == input_slots::keyboard_and_mouse)
        {
            const Key key = button == GamepadButton::a ? Key::enter : button == GamepadButton::b ? Key::escape :
                button == GamepadButton::x ? Key::r : button == GamepadButton::dpad_left ? Key::left : Key::right;
            if (down) { app.keyboard()->on_key_down(key); } else { app.keyboard()->on_key_up(key); }
        }
        else
        {
            auto& buttons = painter_scripted_pads::states[static_cast<size_t>(slot)].buttons;
            buttons = down ? static_cast<uint16_t>(buttons | bit(button)) :
                static_cast<uint16_t>(buttons & static_cast<uint16_t>(~bit(button)));
        }
    };
    const auto tap = [&](int seat, GamepadButton button)
        { hold(seat, button, true); tick(); hold(seat, button, false); tick(); };
    const auto frame = [&] { return main_menu_frame(app); };
    const auto plan = [&](bool committed = false)
    {
        require(!closed && stack.depth() == 2, "Team/Weapon navigation invoked the ordinary page callback or left its sentinel root");
        require(settings.player_count() == initial.player_count() && settings.stage() == initial.stage() &&
            settings.duration() == initial.duration() && settings.best_of() == initial.best_of() &&
            settings.screen_layout() == initial.screen_layout(), "Team focus changed unrelated match choices");
        const auto actual = settings.player_settings(), before = initial.player_settings();
        for (size_t seat = 0; seat < before.size(); ++seat)
        {
            require(actual[seat].num == before[seat].num && actual[seat].weapon == before[seat].weapon &&
                actual[seat].team == (committed ? teams[seat] : before[seat].team),
                "Team focus changed a sparse claim/tool or committed a provisional team");
        }
    };
    const auto open = [&](int count, bool one_team = false)
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::enter, Key::escape, Key::r, Key::left, Key::right }) { app.keyboard()->on_key_up(key); }
        painter_scripted_pads::states = {};
        for (int slot : { 3, 0, 2 }) { painter_scripted_pads::states[static_cast<size_t>(slot)].connected = true; }
        app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll();
        app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        settings = MatchSetup(); settings.set_player_count(count); settings.begin_input_claims(slots[0]);
        for (int seat = 1; seat < count; ++seat) { require(settings.claim_input_slot(slots[static_cast<size_t>(seat)]), "Team focus claim failed"); }
        settings.set_stage("movement_course"); settings.set_duration(73.0f); settings.set_best_of(3);
        settings.set_screen_layout(count == 1 ? ScreenLayout::one_player : count == 2 ? ScreenLayout::two_player :
            count == 3 ? ScreenLayout::three_player : ScreenLayout::four_player);
        teams.clear(); ready.assign(static_cast<size_t>(count), false); warning = closed = false;
        for (int seat = 0; seat < count; ++seat)
        {
            teams.push_back(one_team || seat % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
            settings.set_player_team(seat, teams.back()); settings.set_player_weapon(seat, tools[static_cast<size_t>(seat)]);
        }
        initial = settings; captured = presentation; stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        stack.push(std::make_unique<MainMenuTeamSelect>(&context), [&] { closed = true; }); tick(); tick(); plan();
    };
    const auto resize = [&](Vector2I size)
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
    };
    std::ofstream records(output / "team-focus.csv");
    records << "case,width,height,players,seat,slot,choice,ready,inset,warning,raster_matches,settings_preserved\n";
    const auto compare = [&](const char* name, const std::vector<float>& insets = std::vector<float>{}, bool capture = false,
        bool committed = false, const Presentation* colours = nullptr)
    {
        plan(committed); const Frame actual = frame(); const auto& style = captured.menu(); const auto& look = captured.main_menu();
        const auto& live = (colours == nullptr ? presentation : *colours).main_menu().team_select;
        auto art = look.art; art.focus_padding = look.setup_focus_padding; art.shadow_offset = look.setup_shadow_offset;
        auto row_art = art; row_art.paper = art.focus_fill;
        const RectangleF panel((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - look.team_box_size) / 2.0f, look.team_box_size);
        const auto position = [&](int column, int row) { return style.layout.position + Vector2F(
            look.team_select.spacing.x * static_cast<float>(column), look.team_select.spacing.y * static_cast<float>(row)); };
        const SpriteSheet* atlas = app.render_resources()->sprite_sheet(look.team_select_sheet);
        std::vector<RectangleF> rows, strips; std::vector<std::unique_ptr<colourwars::UiTextDropShadow>> labels;
        for (size_t seat = 0; seat < teams.size(); ++seat)
        {
            labels.push_back(std::make_unique<colourwars::UiTextDropShadow>("independent-team-focus-label", L"P" + std::to_wstring(seat + 1) + L" - " + devices[seat],
                style.detail_font, position(0, static_cast<int>(seat) + 2), app.render_resources(), art.ink, Colour{}, style.item_shadow_offset));
            const RectangleF text = labels.back()->bounds(); const auto& identity = captured.player_identity();
            const RectangleF badge(text.x - identity.ui_size.x - identity.ui_gap,
                text.y + (text.height - identity.ui_size.y) / 2.0f, identity.ui_size.x, identity.ui_size.y);
            RectangleF row = RectangleF::union_of(text, badge), strip;
            for (int choice = 0; choice < 3; ++choice)
            {
                const auto& sprite = atlas->sprite_frame(atlas->resolve_sprite_frame(look.team_select_frames[static_cast<size_t>(choice)]));
                const Vector2F scale(look.team_select.size.x / static_cast<float>(sprite.source_rectangle().width),
                    look.team_select.size.y / static_cast<float>(sprite.source_rectangle().height));
                // Independent frame-origin arithmetic: authored pivots are source
                // texels, so they shift the canvas by origin * destination/source.
                const RectangleF bounds(position(2, static_cast<int>(seat) + 2) +
                    Vector2F(look.team_select.size.x * static_cast<float>(choice), 0.0f) -
                    Vector2F(sprite.origin().x * scale.x, sprite.origin().y * scale.y), look.team_select.size);
                row = RectangleF::union_of(row, bounds); strip = choice == 0 ? bounds : RectangleF::union_of(strip, bounds);
            }
            row.inflate(art.focus_padding); require(panel.contains(row), "Team focus row escaped its paper"); rows.push_back(row);
            strip.inflate(Vector2F(2.0f)); strips.push_back(strip);
        }
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        const auto viewport = app.viewport_manager()->fullscreen_viewport();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
        DrawList list = renderer.view(0); list.set_viewport(viewport); list.set_camera(camera); list.set_filter(TextureFilter::point);
        canvas.draw(list, camera.visible_rectangle(viewport)); setup_paper_reference(list, *app.render_resources(), style, art, panel, RectangleF());
        for (size_t seat = 0; seat < teams.size(); ++seat)
        {
            setup_paper_reference(list, *app.render_resources(), style, row_art, rows[seat], RectangleF());
            const int selected = teams[seat] == PlayerTeam::a ? 0 : teams[seat] == PlayerTeam::none ? 1 : 2;
            for (int choice = 0; choice < 3; ++choice)
            {
                RectangleF box(position(2, static_cast<int>(seat) + 2) + Vector2F(
                    look.team_select.size.x * static_cast<float>(choice), 0.0f), look.team_select.size);
                if (choice == selected && seat < insets.size()) { box.inflate(-std::min(box.width, box.height) * insets[seat]); }
                const auto& frames = choice == selected ? look.team_select_selected_frames : look.team_select_frames;
                const Colour tint = choice == selected && ready[seat] ? live.selected : live.unselected;
                const UiTexture icon("independent-team-focus-icon", look.team_select_sheet, frames[static_cast<size_t>(choice)],
                    box, app.render_resources(), tint); icon.draw(list);
            }
        }
        const colourwars::UiTextDropShadow title("independent-team-focus-title", expected_title, style.heading_font,
            position(0, 0), app.render_resources(), art.ink, Colour{}, style.heading_shadow_offset);
        const colourwars::UiTextDropShadow message("independent-team-focus-warning", L"Both teams need a player", style.detail_font,
            look.team_warning_position, app.render_resources(), art.ink, Colour{}, style.detail_shadow_offset);
        list.set_filter(TextureFilter::linear); title.draw(list);
        for (const auto& label : labels) { label->draw(list); }
        if (warning) { message.draw(list); }
        const Frame expected = read(renderer);
        const auto check = [&](RectangleF bounds, const std::string& role)
        {
            camera.calculate_view_rectangle(bounds); const RectangleI crop = countdown_crop_bounds(bounds);
            const Frame observed = foreground_crop(actual, crop, 0), wanted = foreground_crop(expected, crop, 0);
            if (observed.pixels != wanted.pixels)
            {
                if (captures <= 6) { save(observed, output / (role + "-failure-actual.ppm"));
                    save(wanted, output / (role + "-failure-expected.ppm")); captures += 2; }
                std::cerr << role << " crop=" << crop.x << ',' << crop.y << ',' << crop.width << ',' << crop.height << '\n';
                throw std::runtime_error("Team semantic cursor/readiness raster differs from independent icons/paper");
            }
        };
        const std::string stem = std::string("team-focus-") + name + '-' + std::to_string(teams.size()) + "p-" + std::to_string(actual.width);
        RectangleF evidence = strips.front();
        for (size_t seat = 0; seat < teams.size(); ++seat)
        {
            check(strips[seat], stem + "-seat" + std::to_string(seat)); evidence = RectangleF::union_of(evidence, strips[seat]);
            records << name << ',' << actual.width << ',' << actual.height << ',' << teams.size() << ',' << seat << ',' << slots[seat] << ',' <<
                (teams[seat] == PlayerTeam::a ? "A" : teams[seat] == PlayerTeam::b ? "B" : "none") << ',' << ready[seat] << ',' <<
                (seat < insets.size() ? insets[seat] : 0.0f) << ',' << warning << ",true,true\n"; ++records_written;
        }
        // Lettering and its fonts are captured when widgets are constructed.
        check(title.bounds(), stem + "-title");
        for (size_t seat = 0; seat < labels.size(); ++seat) { check(labels[seat]->bounds(), stem + "-label" + std::to_string(seat)); }
        RectangleF warning_bounds = message.bounds(); warning_bounds.inflate(Vector2F(2.0f)); check(warning_bounds, stem + "-warning");
        if (capture) { require(captures < 8, "Team focus exceeded eight compact crops");
            camera.calculate_view_rectangle(evidence);
            save(foreground_crop(actual, countdown_crop_bounds(evidence), 0), output / (stem + "-detail.ppm")); ++captures; }
        std::srand(1237); const int next = std::rand(); std::srand(1237);
        require(actual.pixels == frame().pixels && std::rand() == next, "Frozen Team focus drawing changed pixels or RNG"); plan(committed);
    };
    const float settle = std::max(original.menu().motion.focus_duration, original.menu().motion.press_duration) + 0.01f;
    const float inset = original.menu().motion.selection_inset_fraction;
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080) })
    {
        resize(size);
        for (int count = 1; count <= 4; ++count)
        {
            open(count); compare("initial", {}, size.x == 1280 || count == 4);
            tap(0, GamepadButton::dpad_left); compare("A-left-endpoint");
            tap(0, GamepadButton::dpad_right); teams[0] = PlayerTeam::none; compare("A-to-none", { inset });
            tap(0, GamepadButton::a); compare("none-confirm-refused", { inset });
            tap(0, GamepadButton::dpad_right); teams[0] = PlayerTeam::b; compare("none-to-B", { inset });
            tap(0, GamepadButton::dpad_right); compare("B-right-endpoint", { inset });
            tap(0, GamepadButton::dpad_left); teams[0] = PlayerTeam::none; compare("B-to-none", { inset });
            tap(0, GamepadButton::dpad_left); teams[0] = PlayerTeam::a; tick(settle); compare("none-to-A-rest");
            if (count == 1)
            {
                const Frame before = frame(); std::srand(173); const int next = std::rand(); std::srand(173);
                tap(0, GamepadButton::x); require(frame().pixels == before.pixels && std::rand() == next, "Solo Team accepted shuffle or consumed RNG");
            }
        }
    }
    resize({ 1280, 720 }); open(4); tap(0, GamepadButton::a); ready[0] = true; tick(settle);
    tap(1, GamepadButton::dpad_left); teams[1] = PlayerTeam::none;
    const Frame moving = frame(); compare("ready-seat-other-seat-moving", { 0.0f, inset, 0.0f, 0.0f }, true);
    tick(original.menu().motion.focus_duration * 0.4f); compare("cursor-middle", { 0.0f, inset * 0.36f, 0.0f, 0.0f }, true);
    play.reduce_effects = true; compare("reduced-cursor"); play.reduce_effects = false;
    tick(settle); compare("cursor-rest"); const Frame rest = frame(); resize({ 1920, 1080 }); compare("resized-readiness");
    resize({ 1280, 720 }); require(rest.pixels == frame().pixels && moving.pixels != rest.pixels, "Team cursor motion/resize did not preserve settled state");
    open(4); tap(0, GamepadButton::a); ready[0] = true;
    hold(0, GamepadButton::b, true); hold(1, GamepadButton::a, true); tick();
    hold(0, GamepadButton::b, false); hold(1, GamepadButton::a, false); tick(); ready[0] = false; ready[1] = true; tick(settle); compare("simultaneous-confirm-back");
    hold(0, GamepadButton::b, true); hold(1, GamepadButton::b, true); tick();
    hold(0, GamepadButton::b, false); hold(1, GamepadButton::b, false); tick(); ready[1] = false; tick(settle); compare("simultaneous-cancel-stays");
    // Shuffle owns the whole release frame, including another seat's Confirm
    // and Back. Pin its old ordered CRT contract without calling shuffle_teams.
    std::array<int, 4> order{ 0, 1, 2, 3 }; std::srand(59);
    for (int last = 3; last > 0; --last) { std::swap(order[static_cast<size_t>(last)], order[static_cast<size_t>(std::rand() % (last + 1))]); }
    const int after_shuffle = std::rand();
    for (int index = 0; index < 4; ++index) { teams[static_cast<size_t>(order[static_cast<size_t>(index)])] = index < 2 ? PlayerTeam::a : PlayerTeam::b; }
    hold(0, GamepadButton::x, true); hold(1, GamepadButton::a, true); hold(2, GamepadButton::b, true); tick();
    std::srand(59); hold(0, GamepadButton::x, false); hold(1, GamepadButton::a, false); hold(2, GamepadButton::b, false); tick();
    require(std::rand() == after_shuffle, "Team focus shuffle changed its seeded RNG budget"); tick(); tick(settle); compare("shuffle-priority-clears-readiness");
    tap(1, GamepadButton::b); plan();
    {
        MatchSetup copy = settings; MenuInput other_input(app.gamepads(), app.keyboard(), &tuning);
        MenuContext other_context(&data, &other_input, &copy); StateContext other;
        other.push(std::make_unique<PauseMenuProbeRoot>(app, canvas)); other.push(std::make_unique<MainMenuPlayerCount>(&other_context));
        const Frame actual = frame(); app.renderer()->begin_frame(); other.draw(*app.renderer());
        require(actual.pixels == read(*app.renderer()).pixels, "Fresh Back from wholly unready Team did not enter actual Count Select");
    }
    open(4, true);
    for (int seat = 0; seat < 3; ++seat) { tap(seat, GamepadButton::a); ready[static_cast<size_t>(seat)] = true; }
    tap(3, GamepadButton::a); warning = true; tick(settle); compare("empty-team-final-confirm-refused");
    tap(3, GamepadButton::dpad_right); teams[3] = PlayerTeam::none;
    tap(3, GamepadButton::dpad_right); teams[3] = PlayerTeam::b; warning = false; tick(settle); compare("playable-clears-warning");
    const auto weapon = [&]
    {
        MatchSetup copy = settings; MenuInput other_input(app.gamepads(), app.keyboard(), &tuning);
        MenuContext other_context(&data, &other_input, &copy); StateContext other;
        other.push(std::make_unique<PauseMenuProbeRoot>(app, canvas)); other.push(std::make_unique<MainMenuWeaponSelect>(&other_context));
        app.renderer()->begin_frame(); other.draw(*app.renderer()); return read(*app.renderer());
    };
    tap(3, GamepadButton::a); plan(true); require(frame().pixels == weapon().pixels, "Final Team confirmation did not commit teams and enter actual Weapon Select");
    tap(1, GamepadButton::b); ready.assign(4, false); initial = settings; tick(settle); compare("weapon-return-fresh-readiness");
    open(2); painter_scripted_pads::states[3].connected = false; tick(); tap(1, GamepadButton::a);
    tap(1, GamepadButton::dpad_left); compare("missing-pad-blocks-other-seat");
    hold(0, GamepadButton::a, true); painter_scripted_pads::states[3].connected = true; tick(); tick();
    hold(0, GamepadButton::a, false); tick(); tick(); compare("held-reconnect-needs-fresh-confirm");
    tap(0, GamepadButton::a); ready[0] = true; tick(settle); compare("fresh-reconnect-confirm");
    // Preserve the established adoption boundary: frames/atlas/text/fonts are
    // captured, while team colours adopt only on the next visual refresh.
    open(4); tap(0, GamepadButton::a); ready[0] = true; tick(settle);
    Presentation alternate = original;
    auto& alternate_look = const_cast<MainMenuPresentation&>(alternate.main_menu());
    alternate_look.team_select_sheet = "menu_paint"; alternate_look.team_select_frames.fill("backdrop");
    alternate_look.team_select_selected_frames.fill("backdrop");
    alternate_look.team_select.selected = Colour(1.0f, 0.30f, 0.20f);
    alternate_look.team_select.unselected = Colour(0.70f, 0.80f, 1.0f);
    auto& alternate_style = const_cast<MenuStyle&>(alternate.menu());
    alternate_style.heading_font = "lato_black_48"; alternate_style.detail_font = "lato_black_24";
    const_cast<MenuText&>(alternate.menu_text()).headings[static_cast<size_t>(MenuHeading::teams)] = L"Choose crews";
    alternate.validate_resources(*app.render_resources(), *app.audio_resources());
    presentation = alternate; compare("assignment-before-refresh-retains-colours", {}, false, false, &original);
    tick(); compare("old-atlas-text-fonts-live-colours");
    tap(1, GamepadButton::dpad_left); teams[1] = PlayerTeam::none; tick(settle); compare("old-ready-colour-retained-while-other-cursor-moves");
    expected_title = L"Choose crews"; open(4); compare("reopened-alternate-atlas-text-fonts");
    presentation = original; compare("reverse-before-refresh-retains-colours", {}, false, false, &alternate);
    tick(); compare("alternate-atlas-text-fonts-original-live-colours");
    expected_title = L"Team Select"; open(4); compare("reopened-original-atlas-text-fonts");
    // Six cloned frames share the existing GPU texture; no source asset or
    // resource is replaced. Opposite pivots swap A/B's physical positions.
    const auto& look = original.main_menu(); const SpriteSheet* shipped = app.render_resources()->sprite_sheet(look.team_select_sheet);
    NameTable<SpriteFrame> frames("sprite frame"); NameTable<AnimationStrip> strips("animation strip");
    for (size_t choice = 0; choice < 3; ++choice)
    {
        const Vector2F pivot(choice == 0 ? -128.0f : choice == 2 ? 128.0f : 0.0f, 0.0f);
        for (const auto* names : { &look.team_select_frames, &look.team_select_selected_frames })
        {
            const auto& sprite = shipped->sprite_frame(shipped->resolve_sprite_frame((*names)[choice]));
            require(sprite.source_rectangle().width == 64 && sprite.source_rectangle().height == 64 && sprite.origin() == Vector2F::ZERO,
                "The nonzero-origin oracle needs the shipped 64 square zero-pivot Team frames");
            frames.add((*names)[choice], SpriteFrame(sprite.source_rectangle(), pivot));
        }
    }
    app.render_resources()->add_sprite_sheet("team_focus_shifted", std::make_unique<SpriteSheet>(shipped->texture(), std::move(frames), std::move(strips)));
    auto& shifted = const_cast<MainMenuPresentation&>(presentation.main_menu()); shifted.team_select_sheet = "team_focus_shifted";
    presentation.validate_resources(*app.render_resources(), *app.audio_resources()); play.reduce_effects = true; open(4);
    require(look.team_select.size == Vector2F(50.0f), "The swapped-pivot oracle needs shipped50-unit choice canvases");
    const SpriteSheet* shifted_atlas = app.render_resources()->sprite_sheet("team_focus_shifted");
    const auto& shifted_a = shifted_atlas->sprite_frame(shifted_atlas->resolve_sprite_frame(look.team_select_frames[0]));
    const auto& shifted_b = shifted_atlas->sprite_frame(shifted_atlas->resolve_sprite_frame(look.team_select_frames[2]));
    const float a_shift = -shifted_a.origin().x * look.team_select.size.x / static_cast<float>(shifted_a.source_rectangle().width);
    const float b_shift = -shifted_b.origin().x * look.team_select.size.x / static_cast<float>(shifted_b.source_rectangle().width);
    require(a_shift == 100.0f && b_shift == -100.0f && a_shift > look.team_select.size.x * 2.0f + b_shift,
        "The validated nonzero-origin resource did not reverse the physical A/B order");
    compare("nonzero-origin-swapped-physical-order", {}, true);
    tap(0, GamepadButton::dpad_right); teams[0] = PlayerTeam::none; compare("origin-semantic-A-to-none");
    tap(0, GamepadButton::dpad_right); teams[0] = PlayerTeam::b; compare("origin-semantic-none-to-B");
    tap(0, GamepadButton::dpad_right); compare("origin-B-endpoint");
    tap(0, GamepadButton::dpad_left); teams[0] = PlayerTeam::none;
    tap(0, GamepadButton::dpad_left); teams[0] = PlayerTeam::a; compare("origin-semantic-reverse");
    require(captures == 8 && records_written > 0 && records.good(), "Team focus evidence is incomplete or exceeded its eight-crop cap");
    std::cout << "PASS Team focus actual Team/Weapon pages: independent choice/readiness icon rasters, sparse1-4 rosters, semantic nonwrap endpoints, ready-row tint retention, local cursor motion, confirm/Back/shuffle priority, empty-team refusal, reconnect fresh press, committed teams/Weapon return and nonzero-origin reversed physical order; " <<
        ticks << " updates, " << records_written << " CSV rows, eight compact crops; no broad matrix, full match, physical-pad or foreground acceptance\n";
}

#if COLOURWARS_DEV_TOOLS
// Two live colour commands only; this never invokes the broad Team focus or
// general tune-colour matrices. All command targets and saves are scratch.
void team_tune_colour_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& content,
    const std::filesystem::path& output)
{
    const auto root = output / "team-tune-scratch", source = root / "source", staged = root / "staged";
    std::filesystem::create_directories(source); std::filesystem::create_directories(staged);
    constexpr std::array<const char*, 2> files{ "tuning.json", "presentation.json" };
    std::array<std::string, 2> real_bytes;
    for (size_t index = 0; index < files.size(); ++index)
    {
        real_bytes[index] = tune_colour_bytes(content / files[index]);
        tune_colour_write(source / files[index], real_bytes[index]); tune_colour_write(staged / files[index], real_bytes[index]);
    }
    const NestedSaveProfile profile(root / "profile"); const TuneColourTemp temporary(root / "temp");
    const GameTuning original_tuning = tuning; const Presentation original = presentation;
    struct RestoreTeamTuneContent
    {
        GameTuning& tuning; Presentation& look; GameTuning original_tuning; Presentation original;
        ~RestoreTeamTuneContent() { this->tuning = this->original_tuning; this->look = this->original; }
    } restore{ tuning, presentation, original_tuning, original };
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    Save preferences; SettingsSession preferences_session(preferences); PlaySettings play = preferences_session.play_settings();
    play.music_volume = play.sfx_volume = 0; play.reduce_effects = true;
    require(preferences_session.set_play_settings_and_save(play), "Cannot seed the private Team tune profile");
    const auto save_path = root / "profile/ColourWars/save_data.json"; const std::string saved = tune_colour_bytes(save_path);
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false } };
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_settings_session(&preferences_session);
    const StageCatalog stage_catalog = acceptance::catalog_of(levels, stages, { "movement_course" }); data.set_stage_catalog(&stage_catalog); data.set_tuning(&tuning); data.set_presentation(&presentation);
    DebugDirector director(&tuning, &presentation); data.set_debug(&director); DebugDirectorImpl& impl = *director.impl();
    impl.set_content_directories(source, staged); impl.set_user_directory(root / "user");
    impl.set_script_directories({ root / "scripts" }); director.attach(&app, &data);
    impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output / "team-tune-commands.log").string()), "Cannot retain Team tune transcript");
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MatchSetup settings;
    settings.set_player_count(2); settings.begin_input_claims(3); require(settings.claim_input_slot(4), "Cannot claim Team tune keyboard seat");
    settings.set_stage("movement_course"); settings.set_duration(73.0f); settings.set_best_of(3); settings.set_screen_layout(ScreenLayout::two_player);
    settings.set_player_team(0, PlayerTeam::a); settings.set_player_team(1, PlayerTeam::b);
    settings.set_player_weapon(0, WeaponType::roller); settings.set_player_weapon(1, WeaponType::sniper);
    const MatchSetup before = settings; MenuContext context(&data, &input, &settings); StateContext& stack = app;
    struct ClearTeamTuneStack
    {
        StateContext& stack;
        ~ClearTeamTuneStack() { this->stack.clear(); painter_scripted_pads::states = {}; }
    } clear{ stack };
    stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
    for (const Key key : { Key::enter, Key::escape, Key::r, Key::left, Key::right }) { app.keyboard()->on_key_up(key); }
    painter_scripted_pads::states = {}; painter_scripted_pads::states[3].connected = true;
    app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll();
    app.mouse()->poll(); app.mouse()->poll(); input.reset_actions(); int ticks = 0;
    const auto tick = [&](float elapsed = 0.0f)
    {
        require(++ticks <= 12, "Team tune exceeded twelve bounded page updates");
        app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed);
    };
    stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas)); stack.push(std::make_unique<MainMenuTeamSelect>(&context)); tick(); tick();
    const auto confirm = static_cast<uint16_t>(1u << static_cast<unsigned int>(GamepadButton::a));
    painter_scripted_pads::states[3].buttons = confirm; tick(); painter_scripted_pads::states[3].buttons = 0; tick();
    tick(0.51f); const auto input_before = painter_scripted_pads::states;
    const auto unchanged = [&]
    {
        require(stack.depth() == 2 && settings.player_count() == before.player_count() && settings.stage() == before.stage() &&
            settings.duration() == before.duration() && settings.best_of() == before.best_of() && settings.screen_layout() == before.screen_layout(),
            "Team tune changed the page stack or match settings");
        const auto actual = settings.player_settings(), expected = before.player_settings();
        for (size_t seat = 0; seat < expected.size(); ++seat)
        { require(actual[seat].num == expected[seat].num && actual[seat].team == expected[seat].team && actual[seat].weapon == expected[seat].weapon,
            "Team tune changed a sparse roster claim, team or tool"); }
        for (size_t slot = 0; slot < input_before.size(); ++slot)
        {
            const auto& actual_pad = painter_scripted_pads::states[slot]; const auto& expected_pad = input_before[slot];
            require(actual_pad.connected == expected_pad.connected && actual_pad.buttons == expected_pad.buttons &&
                actual_pad.left_stick == expected_pad.left_stick && actual_pad.right_stick == expected_pad.right_stick &&
                actual_pad.left_trigger == expected_pad.left_trigger && actual_pad.right_trigger == expected_pad.right_trigger,
                "Team tune changed a scripted input snapshot");
        }
        require(tune_colour_bytes(save_path) == saved, "Team tune wrote the private save");
    };
    const auto draw = [&] { unchanged(); return main_menu_frame(app); };
    const Frame baseline = tune_colour_frozen(draw);
    const auto& look = original.main_menu(); const auto& style = original.menu();
    auto art = look.art; art.focus_padding = look.setup_focus_padding; art.shadow_offset = look.setup_shadow_offset;
    auto row_art = art; row_art.paper = art.focus_fill;
    const RectangleF panel((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - look.team_box_size) / 2.0f, look.team_box_size);
    const auto position = [&](int column, int row) { return style.layout.position + Vector2F(
        look.team_select.spacing.x * static_cast<float>(column), look.team_select.spacing.y * static_cast<float>(row)); };
    RectangleF evidence(position(2, 2), Vector2F(look.team_select.size.x * 3.0f, look.team_select.spacing.y + look.team_select.size.y));
    evidence.inflate(Vector2F(2.0f)); const auto viewport = app.viewport_manager()->fullscreen_viewport();
    const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
    camera.calculate_view_rectangle(evidence); const RectangleI crop = countdown_crop_bounds(evidence);
    // Fixed A/B choices and P1-only readiness are specified independently of
    // FocusGroup. Only the two literal tints vary in this oracle.
    const auto expected_strip = [&](const Colour& selected, const Colour& unselected)
    {
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(viewport); list.set_camera(camera); list.set_filter(TextureFilter::point);
        canvas.draw(list, camera.visible_rectangle(viewport)); setup_paper_reference(list, *app.render_resources(), style, art, panel, RectangleF());
        const std::array<std::wstring, 2> labels{ L"P1 - Pad 4", L"P2 - Keyboard/mouse" };
        for (int seat = 0; seat < 2; ++seat)
        {
            const colourwars::UiTextDropShadow label("independent-team-tune-label", labels[static_cast<size_t>(seat)], style.detail_font,
                position(0, seat + 2), app.render_resources(), art.ink, Colour{}, style.item_shadow_offset);
            const RectangleF text = label.bounds(); const auto& identity = original.player_identity();
            const RectangleF badge(text.x - identity.ui_size.x - identity.ui_gap,
                text.y + (text.height - identity.ui_size.y) / 2.0f, identity.ui_size.x, identity.ui_size.y);
            RectangleF row = RectangleF::union_of(RectangleF::union_of(text, badge),
                RectangleF(position(2, seat + 2), Vector2F(look.team_select.size.x * 3.0f, look.team_select.size.y)));
            row.inflate(art.focus_padding); setup_paper_reference(list, *app.render_resources(), style, row_art, row, RectangleF());
            for (int choice = 0; choice < 3; ++choice)
            {
                const bool focused = choice == (seat == 0 ? 0 : 2);
                const auto& frames = focused ? look.team_select_selected_frames : look.team_select_frames;
                const UiTexture icon("independent-team-tune-icon", look.team_select_sheet, frames[static_cast<size_t>(choice)],
                    RectangleF(position(2, seat + 2) + Vector2F(look.team_select.size.x * static_cast<float>(choice), 0.0f), look.team_select.size),
                    app.render_resources(), seat == 0 && focused ? selected : unselected); icon.draw(list);
            }
        }
        return foreground_crop(read(renderer), crop, 0);
    };
    const auto check_strip = [&](const Frame& actual, const Colour& selected, const Colour& unselected)
    { require(foreground_crop(actual, crop, 0).pixels == expected_strip(selected, unselected).pixels,
        "Team tune differs from independently fixed A/B cursors and P1-ready/P2-unready icon tints"); };
    check_strip(baseline, look.team_select.selected, look.team_select.unselected);
    std::ofstream records(output / "team-tune-colours.csv"); records << "check,adoption,selected,unselected,cursor_readiness_roster_preserved,pixels_match\n";
    const auto command = [&](const std::string& line)
    {
        impl.log().write(DebugSeverity::echo, "] " + line); const DebugStatus status = impl.execute(line);
        if (!status.ok) { std::cerr << line << ": " << status.message << '\n'; }
        require(status.ok, "Actual Team tune colour command was rejected");
    };
    const std::string selected_path = "presentation.main_menu.team_select.selected", unselected_path = "presentation.main_menu.team_select.unselected";
    command("tune " + selected_path + " 2266CC"); command("tune " + unselected_path + " D94518");
    const Colour selected("2266CC"), unselected("D94518");
    require(presentation.main_menu().team_select.selected == selected && presentation.main_menu().team_select.unselected == unselected &&
        impl.tune_document("presentation")->edits().size() == 2, "Team commands did not update exactly both audited colour leaves");
    require(tune_colour_frozen(draw).pixels == baseline.pixels, "Frozen Team drawing adopted commands before its existing refresh point");
    records << "commands-before-update,retained,2266CC,D94518,true,true\n";
    tick(); const Frame tuned = tune_colour_frozen(draw); check_strip(tuned, selected, unselected);
    require(tuned.pixels != baseline.pixels, "The active connected Team page did not adopt its tuned colours");
    for (int y = 0; y < baseline.height; ++y) { for (int x = 0; x < baseline.width; ++x)
    {
        if (x >= crop.x && x < crop.right() && y >= crop.y && y < crop.bottom()) { continue; }
        const size_t pixel = (static_cast<size_t>(y) * static_cast<size_t>(baseline.width) + static_cast<size_t>(x)) * 4;
        for (size_t channel = 0; channel < 4; ++channel) { require(baseline.pixels[pixel + channel] == tuned.pixels[pixel + channel],
            "Team tune changed captured fonts, lettering, paper or other pixels outside its icon strip"); }
    } }
    records << "active-connected-refresh,live,2266CC,D94518,true,true\n";
    command("tune.reset " + selected_path); command("tune.reset " + unselected_path);
    require(impl.tune_document("presentation")->edits().empty() && tune_colour_frozen(draw).pixels == tuned.pixels,
        "Team reset changed frozen widgets before refresh or retained edits");
    tick(); require(tune_colour_frozen(draw).pixels == baseline.pixels, "Team tune reset did not restore its exact same-page baseline"); unchanged();
    records << "reset-refresh,restored,baseline,baseline,true,true\n";
    save(foreground_crop(baseline, crop, 0), output / "team-tune-colours-original-detail.ppm");
    save(foreground_crop(tuned, crop, 0), output / "team-tune-colours-tuned-detail.ppm");
    for (size_t index = 0; index < files.size(); ++index)
    { require(tune_colour_bytes(content / files[index]) == real_bytes[index] && tune_colour_bytes(source / files[index]) == real_bytes[index] &&
        tune_colour_bytes(staged / files[index]) == real_bytes[index], "Team tune wrote actual content or its scratch source/staged files"); }
    require(records.good(), "Team tune CSV write failed");
    std::cout << "PASS Team tune colours: real validated DebugDirector commands for both Team tints, same-page P1-ready/P2-unready independent icon rasters, frozen-before-refresh retention, connected-update adoption, exact reset reversal and unchanged captured frames/fonts/paper/roster/settings/inputs/save/draw RNG; " <<
        ticks << " bounded updates, two compact crops and private scratch transcript/CSV; no broad matrix, full match or performance acceptance\n";
}
#endif
