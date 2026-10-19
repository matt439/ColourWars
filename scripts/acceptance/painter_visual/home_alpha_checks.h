// Draft for inclusion after save_status_checks.h and the ordinary menu probes.
// Both presets use real production pages and the normal input backend. A local
// state stack initializes 13 pages directly: no StateContext update or input
// event is needed to observe the captured background tint.
void home_alpha_checks(Application& app, Registry<LevelLoadedInfo>& levels,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& content,
    const std::filesystem::path& output)
{
    const auto bytes = [](const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        require(file.good(), "Cannot read Home alpha fixture content or private save");
        return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    };
    constexpr std::array<const char*, 2> content_names{ "tuning.json", "presentation.json" };
    const std::array<std::string, 2> original_content{
        bytes(content / content_names[0]), bytes(content / content_names[1]) };
    const Presentation original = presentation;
    require(original.main_menu().home_background_colour == Colour(176, 176, 168),
        "Home alpha opaque parity fixture differs from the reviewed default");
    const NestedSaveProfile profile(output / "alpha-home-profile");
    const auto save_path = output / "alpha-home-profile/ColourWars/save_data.json";
    Save preferences;
    PlaySettings play;
    play.music_volume = play.sfx_volume = 0; play.aim_sensitivity = 4;
    play.view_shake = false; play.aim_marker = true; play.reduce_effects = true;
    for (size_t slot = 0; slot < play.sticks.size(); ++slot)
    {
        play.sticks[slot].movement_deadzone = 10 + static_cast<int>(slot) * 10;
        play.sticks[slot].aim_deadzone = 20 + static_cast<int>(slot) * 10;
    }
    require(preferences.set_play_settings_and_save(play) && preferences.set_display_settings_and_save(
        ScreenResolution::s_1280_720, false, TeamColourPair::blue_yellow),
        "Cannot seed the Home alpha fixture's private save");
    const std::string saved = bytes(save_path);
    StageList stages;
    const auto* movement = levels.get("movement_course");
    require(movement != nullptr, "Home alpha fixture requires the reviewed movement_course stage");
    stages.push_back({ "movement_course", movement->display_name(), movement->icon_frame(), movement->include_in_random() });
    GameData data;
    data.set_application(&app); data.set_save(&preferences); data.set_level_infos(&levels);
    data.set_stages(&stages); data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MenuLevelSettings plan;
    plan.set_player_count(1); plan.set_best_of(1); plan.set_stage("movement_course");
    plan.set_duration(73.0f); plan.set_screen_layout(ScreenLayout::one_player);
    plan.begin_input_claims(input_slots::keyboard_and_mouse);
    plan.set_player_team(0, PlayerTeam::a); plan.set_player_weapon(0, WeaponType::sprayer);
    MenuContext context(&data, &input, &plan);
    const HWND window = static_cast<HWND>(app.window());
    require(IsWindow(window) != FALSE && !app.fullscreen(), "Home alpha fixture needs its own windowed process");
    const SettingsProbeActivationGuard activation(window);
    require(SetWindowPos(window, nullptr, -32000, -32000, 0, 0,
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE) != FALSE,
        "Cannot keep the Home alpha fixture's private window offscreen");
    ShowWindow(window, SW_HIDE);
    app.set_resolution(ScreenResolution::s_1280_720);
    ShowWindow(window, SW_HIDE);
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    require(app.resolution_manager()->resolution_ivec() == Vector2I(1280, 720) &&
        IsWindowVisible(window) == FALSE && GetForegroundWindow() != window && !app.fullscreen(),
        "Home alpha initialization exposed or activated its private window");
    StateContext stack;
    struct RestoreHomeAlpha
    {
        StateContext& stack; Presentation& presentation; Presentation original; HWND window;
        ~RestoreHomeAlpha()
        {
            this->stack.clear(); ShowWindow(this->window, SW_HIDE); this->presentation = this->original;
        }
    } restore{ stack, presentation, original, window };
    const auto unchanged = [&]
    {
        const PlaySettings& active = preferences.play_settings();
        require(preferences.resolution() == ScreenResolution::s_1280_720 && !preferences.fullscreen() &&
            preferences.team_palette() == TeamColourPair::blue_yellow && active.aim_sensitivity == play.aim_sensitivity &&
            active.music_volume == 0 && active.sfx_volume == 0 && active.view_shake == play.view_shake &&
            active.aim_marker == play.aim_marker && active.reduce_effects == play.reduce_effects &&
            preferences.records().stages().empty() && !preferences.last_match_setup(),
            "Home alpha construction or drawing changed private active preferences or records");
        for (size_t slot = 0; slot < play.sticks.size(); ++slot)
        {
            require(active.sticks[slot].movement_deadzone == play.sticks[slot].movement_deadzone &&
                active.sticks[slot].aim_deadzone == play.sticks[slot].aim_deadzone,
                "Home alpha construction changed a private pad profile");
        }
        require(plan.player_count() == 1 && plan.best_of() == 1 && plan.stage() == "movement_course" &&
            plan.duration() == std::optional<float>(73.0f) && plan.screen_layout() == ScreenLayout::one_player &&
            plan.player_settings()[0].num == input_slots::keyboard_and_mouse && plan.player_team(0) == PlayerTeam::a &&
            plan.player_weapon(0) == WeaponType::sprayer && bytes(save_path) == saved,
            "Home alpha construction changed its setup plan or private save bytes");
        require(IsWindowVisible(window) == FALSE && GetForegroundWindow() != window && !app.fullscreen() &&
            app.resolution_manager()->resolution_ivec() == Vector2I(1280, 720),
            "Home alpha page construction exposed, activated or resized its private window");
    };
    constexpr std::array<const char*, 13> names{ "home", "mode", "player-count", "join", "credits", "options",
        "exit-confirmation", "display-confirmation", "team", "weapon", "stage", "controls", "sticks" };
    size_t openings = 0, readbacks = 0, rows = 0, crops = 0, controls = 0;
    const auto open = [&](size_t site)
    {
        stack.clear();
        // These are the 13 actual constructor/init sites that capture the
        // shared Home background; no synthetic MenuPage substitutes for one.
        switch (site)
        {
        case 0: stack.push(std::make_unique<MainMenuHome>(&context)); break;
        case 1: stack.push(std::make_unique<MainMenuModeSelect>(&context)); break;
        case 2: stack.push(std::make_unique<MainMenuPlayerCount>(&context)); break;
        case 3: stack.push(std::make_unique<MainMenuJoin>(&context)); break;
        case 4: stack.push(std::make_unique<MainMenuCredits>(&context)); break;
        case 5: stack.push(std::make_unique<MainMenuOptions>(&context)); break;
        case 6: stack.push(std::make_unique<MainMenuExitConfirmation>(&context)); break;
        case 7:
            stack.push<DisplayTrialResult>(std::make_unique<MainMenuDisplayConfirmation>(&context,
                DisplaySettingsChoice{ ScreenResolution::s_1280_720, false, TeamColourPair::blue_yellow }),
                [](const DisplayTrialResult&) { throw std::runtime_error("Home alpha fixture unexpectedly answered a display trial"); });
            break;
        case 8: stack.push(std::make_unique<MainMenuTeamSelect>(&context)); break;
        case 9: stack.push(std::make_unique<MainMenuWeaponSelect>(&context)); break;
        case 10: stack.push(std::make_unique<MainMenuStageSelect>(&context)); break;
        case 11: stack.push(std::make_unique<ControlsSettingsMenu>(&context)); break;
        case 12: stack.push(std::make_unique<StickSettingsMenu>(&context)); break;
        default: throw std::runtime_error("Home alpha fixture contains an unknown capture site");
        }
        ++openings;
        // Trial Apply and rollback target the same private window/preset. The
        // activation hook stays active until every retained trial is destroyed.
        ShowWindow(window, SW_HIDE);
        require(stack.depth() == 1, "Home alpha page factory changed its local stack depth");
        unchanged();
    };
    const auto& style = original.menu();
    const SpriteSheet* sheet = app.render_resources()->sprite_sheet(style.sprite_sheet);
    const SpriteFrame& pixel = sheet->sprite_frame(sheet->resolve_sprite_frame(style.fill_frame));
    const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
    const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), viewport);
    const RectangleF design(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION);
    const RectangleI corner(8, 8, 16, 16);
    const std::array<Colour, 2> backgrounds{ Colour(0, 0, 0), Colour(32, 64, 96) };
    const std::array<Colour, 3> palettes{ Colour(176, 176, 168), Colour(32, 96, 192, 128), Colour(165, 50, 214, 0) };
    constexpr std::array<const char*, 3> palette_names{ "default", "rgba128", "alpha0" };
    const auto render = [&](size_t background, const std::function<void(DrawList&)>& draw)
    {
        require(readbacks < 235, "Home alpha fixture exceeded its explicit readback budget");
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(viewport); list.set_camera(Camera::DEFAULT_CAMERA);
        list.set_filter(TextureFilter::point);
        Canvas(app.render_resources(), backgrounds[background]).draw(list, RectangleF(0.0f, 0.0f, 1280.0f, 720.0f));
        list.set_camera(camera); draw(list); ++readbacks;
        Frame frame = read(renderer);
        require(frame.width == 1280 && frame.height == 720, "Home alpha readback changed its fixed native size");
        return frame;
    };
    const auto oracle = [&](size_t background, const Colour& colour, bool straight)
    {
        return render(background, [&](DrawList& list)
        {
            // Preserve the pre-existing scrim submission as-is. Only this
            // exposed Home tint is under test; other compositing is unchanged.
            list.draw_sprite(sheet->texture(), pixel.source_rectangle(), design, style.scrim_colour,
                0.0f, pixel.origin(), SpriteFlip::none, 0.0f);
            const Colour tint = straight ? colour : Colour(colour.r * colour.a, colour.g * colour.a,
                colour.b * colour.a, colour.a);
            // Raw source submission bypasses UiTexture and every production
            // premultiplication helper, including CrudeDrawing::premultiplied.
            list.draw_sprite(sheet->texture(), pixel.source_rectangle(), design, tint,
                0.0f, pixel.origin(), SpriteFlip::none, 0.0f);
        });
    };
    std::array<std::array<Frame, 2>, 3> expected;
    for (size_t palette = 0; palette < palettes.size(); ++palette)
    {
        for (size_t background = 0; background < backgrounds.size(); ++background)
        {
            expected[palette][background] = foreground_crop(oracle(background, palettes[palette], false), corner, 0);
            const Frame legacy = foreground_crop(oracle(background, palettes[palette], true), corner, 0);
            require((expected[palette][background].pixels == legacy.pixels) == (palette == 0),
                "Home alpha oracle cannot distinguish straight RGBA or preserve opaque legacy pixels");
            ++controls;
            if (palette == 2)
            {
                require(expected[palette][background].pixels == foreground_crop(oracle(background,
                    Colour(0, 0, 0, 0), true), corner, 0).pixels,
                    "Nonblack alpha-zero Home oracle differs from zero RGBA");
                ++controls;
            }
        }
    }
    const auto draw_once = [&](size_t background)
    {
        return render(background, [&](DrawList&) { stack.draw(*app.renderer()); });
    };
    const auto actual = [&](size_t background)
    {
        std::srand(6197); const int next_random = std::rand(); std::srand(6197);
        const Frame first = draw_once(background);
        require(first.pixels == draw_once(background).pixels && std::rand() == next_random,
            "Home alpha page drawing changed frozen pixels or consumed match RNG");
        unchanged(); return first;
    };
    std::ofstream evidence(output / "alpha-home.csv");
    evidence << "site,state,background,expected_corner,captured_retained,reopened,frozen_state_rng,outcome\n";
    const auto check_corner = [&](const Frame& frame, size_t palette, size_t background)
    {
        require(foreground_crop(frame, corner, 0).pixels == expected[palette][background].pixels,
            "Production Home background differs from the independent raw premultiplied corner oracle");
    };
    const auto record = [&](size_t site, const std::string& state, size_t background, bool retained)
    {
        evidence << names[site] << ',' << state << ',' << (background == 0 ? "black" : "nonblack") <<
            ",true," << (retained ? "true,false" : "false,true") << ",true,passed\n"; ++rows;
    };
    const auto retain = [&](size_t site, const char* state, size_t palette, size_t background, const Frame& before)
    {
        std::srand(6197); const int next_random = std::rand(); std::srand(6197);
        const Frame current = draw_once(background);
        require(current.pixels == before.pixels && std::rand() == next_random,
            "An existing Home background capture adopted a later source edit/reset or changed frozen draw RNG");
        check_corner(current, palette, background); unchanged(); record(site, state, background, true);
    };
    for (size_t site = 0; site < names.size(); ++site)
    {
        const_cast<MainMenuPresentation&>(presentation.main_menu()).home_background_colour = palettes[0]; open(site);
        std::array<Frame, 2> opaque;
        for (size_t background = 0; background < backgrounds.size(); ++background)
        {
            opaque[background] = actual(background); check_corner(opaque[background], 0, background);
            record(site, palette_names[0], background, false);
        }
        const_cast<MainMenuPresentation&>(presentation.main_menu()).home_background_colour = palettes[1];
        retain(site, "retained-default", 0, 0, opaque[0]); open(site);
        std::array<Frame, 2> rgba;
        for (size_t background = 0; background < backgrounds.size(); ++background)
        {
            rgba[background] = actual(background); check_corner(rgba[background], 1, background);
            require(rgba[background].pixels != opaque[background].pixels,
                "Reopened page did not adopt the RGBA Home background");
            record(site, palette_names[1], background, false);
            if (site == 0)
            {
                save(foreground_crop(rgba[background], RectangleI(0, 0, 128, 96), 0), output /
                    (std::string("alpha-home-rgba128-") + (background == 0 ? "black.ppm" : "nonblack.ppm"))); ++crops;
            }
        }
        const_cast<MainMenuPresentation&>(presentation.main_menu()).home_background_colour = palettes[2];
        retain(site, "retained-rgba128", 1, 1, rgba[1]); open(site);
        std::array<Frame, 2> transparent;
        for (size_t background = 0; background < backgrounds.size(); ++background)
        {
            transparent[background] = actual(background); check_corner(transparent[background], 2, background);
            require(transparent[background].pixels != rgba[background].pixels,
                "Reopened page retained colour energy instead of adopting alpha-zero Home");
            record(site, palette_names[2], background, false);
            if (site == 0)
            {
                save(foreground_crop(transparent[background], RectangleI(0, 0, 128, 96), 0), output /
                    (std::string("alpha-home-alpha0-") + (background == 0 ? "black.ppm" : "nonblack.ppm"))); ++crops;
            }
        }
        const_cast<MainMenuPresentation&>(presentation.main_menu()).home_background_colour = palettes[0];
        retain(site, "retained-alpha0", 2, 0, transparent[0]); open(site);
        const Frame restored = actual(0); check_corner(restored, 0, 0);
        require(restored.pixels == opaque[0].pixels, "Restored/reopened page differs from its exact opaque default raster");
        record(site, "restored-default", 0, false);
    }
    stack.clear(); unchanged();
    for (size_t index = 0; index < content_names.size(); ++index)
        { require(bytes(content / content_names[index]) == original_content[index], "Home alpha probe changed staged content bytes"); }
    require(readbacks == 235 && openings == 52 && rows == 130 && crops == 4 && controls == 8 && evidence.good(),
        "Home alpha evidence differs from its fixed bounded counts");
    std::ofstream summary(output / "alpha-home-summary.json");
    summary << "{\"scope\":\"consumer-only\",\"state_updates\":0,\"readbacks\":235,\"page_openings\":52,"
        "\"csv_rows\":130,\"crops\":4,\"controls\":8,\"sites\":13,\"save_calls\":2}\n";
    require(summary.good(), "Cannot retain Home alpha bounded summary");
    std::cout << "PASS exposed Home alpha: 13 production captures, 52 direct page openings, 235 readbacks, "
        "130 CSV rows and four compact crops; independent raw premultiplication over two underlays, nonblack alpha-zero, "
        "zero-RGBA/opaque parity and straight-alpha negative controls; retained/reopened/reset captures, frozen state/RNG "
        "and byte-identical private saves/content. No StateContext updates, injected input, visible window, "
        "developer-command, physical-device, listening, human or performance acceptance.\n";
}
