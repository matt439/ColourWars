// Debug-only renderer acceptance. Commands use the real validated director;
// content, candidate files, user files and saves are confined to this run.

class TuneColourTemp
{
public:
    explicit TuneColourTemp(const std::filesystem::path& directory)
    {
        std::filesystem::create_directories(directory);
        for (size_t index = 0; index < 2; ++index)
        {
            wchar_t* previous = nullptr; size_t length = 0;
            require(_wdupenv_s(&previous, &length, this->names_[index]) == 0,
                "cannot read the tune probe temporary directory");
            if (previous != nullptr) { this->previous_[index] = previous; std::free(previous); }
            require(_wputenv_s(this->names_[index], directory.c_str()) == 0,
                "cannot isolate tune candidate temporary files");
        }
    }
    ~TuneColourTemp()
    {
        for (size_t index = 0; index < 2; ++index)
        { _wputenv_s(this->names_[index], this->previous_[index].c_str()); }
    }
private:
    const std::array<const wchar_t*, 2> names_{ L"TEMP", L"TMP" };
    std::array<std::wstring, 2> previous_;
};

std::string tune_colour_bytes(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary); require(file.good(), "cannot read tune probe content");
    return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
}

void tune_colour_write(const std::filesystem::path& path, const std::string& bytes)
{
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    require(file.good(), "cannot write tune probe scratch content");
}

size_t tune_colour_pixels(const Frame& frame, std::uint32_t colour)
{
    size_t count = 0;
    for (int y = 0; y < frame.height; ++y)
    {
        for (int x = 0; x < frame.width; ++x)
        { if (monitor_rgb(frame, x, y) == colour) { ++count; } }
    }
    return count;
}

Frame tune_colour_draw(Application& app, const Canvas& canvas,
    const std::function<void(DrawList&)>& draw, const Camera& camera = Camera::DEFAULT_CAMERA)
{
    Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
    DrawList list = renderer.view(0); list.set_viewport(app.viewport_manager()->fullscreen_viewport());
    list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
    canvas.draw(list, { 0, 0, 1280, 720 }); list.set_camera(camera); draw(list);
    return read(renderer);
}

Frame tune_colour_frozen(const std::function<Frame()>& draw)
{
    std::srand(8451); const int expected = std::rand(); std::srand(8451);
    const Frame first = draw();
    require(first.pixels == draw().pixels, "frozen tune-colour consumer changed pixels");
    require(std::rand() == expected, "tune-colour drawing consumed match randomness");
    return first;
}

void tune_colour_matrix(Application& app, Registry<LevelLoadedInfo>& levels,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& content,
    const std::filesystem::path& output)
{
    const auto root = output / "tune-colour-scratch";
    const auto source = root / "source", staged = root / "staged";
    std::filesystem::create_directories(source); std::filesystem::create_directories(staged);
    std::array<std::string, 2> real_bytes;
    constexpr const char* files[] = { "tuning.json", "presentation.json" };
    for (size_t index = 0; index < 2; ++index)
    {
        real_bytes[index] = tune_colour_bytes(content / files[index]);
        tune_colour_write(source / files[index], real_bytes[index]);
        tune_colour_write(staged / files[index], real_bytes[index]);
    }
    const NestedSaveProfile profile(root / "profile");
    const TuneColourTemp temporary(root / "temp");
    const GameTuning original_tuning = tuning; const Presentation original_presentation = presentation;
    struct RestoreContent
    {
        GameTuning& tuning; Presentation& presentation; GameTuning original_tuning; Presentation original_presentation;
        ~RestoreContent() { this->tuning = this->original_tuning; this->presentation = this->original_presentation; }
    } restore{ tuning, presentation, original_tuning, original_presentation };
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    Save preferences; auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0; play.reduce_effects = false;
    StageList stages;
    for (const char* name : { "movement_course", "close_quarters", "king_of_the_hill", "turbulence" })
    {
        const auto* info = levels.get(name);
        stages.push_back({ name, info->display_name(), info->icon_frame(), info->include_in_random() });
    }
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_level_infos(&levels);
    data.set_stages(&stages); data.set_tuning(&tuning); data.set_presentation(&presentation);
    DebugDirector director(&tuning, &presentation); data.set_debug(&director);
    DebugDirectorImpl& impl = *director.impl();
    impl.set_content_directories(source, staged); impl.set_user_directory(root / "user");
    impl.set_script_directories({ root / "scripts" }); director.attach(&app, &data);
    impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output / "tune-colour-commands.log").string()),
        "cannot retain tune-colour command transcript");
    StateContext& stack = app;
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MenuLevelSettings menu_settings;
    MenuContext context(&data, &input, &menu_settings);
    struct ClearStack { StateContext& stack; ~ClearStack() { this->stack.clear(); } } clear{ stack };
    const Canvas black(app.render_resources(), Colour::black);
    std::ofstream records(output / "tune-colours.csv");
    records << "check,path,adoption,matching_pixels,outcome\n";
    const auto command = [&](const std::string& line)
    {
        const DebugStatus result = impl.execute(line);
        if (!result.ok) { std::cout << "COMMAND " << line << ": " << result.message << '\n'; }
        require(result.ok, "valid native tune-colour command was rejected");
    };
    const auto reset = [&](const std::string& path) { command("tune.reset " + path); };

    MovementFeedback dust(&presentation.movement_feedback(), app.render_resources());
    dust.contact(MovementFeedbackKind::skid, { 300, 300 }, 1.0f, std::nullopt);
    require(dust.effects().size() == 1 && !dust.effects()[0].paint_colour,
        "live movement fallback fixture did not create unpainted dust");
    const auto draw_dust = [&] { return tune_colour_draw(app, black, [&](DrawList& list) { dust.draw(list); }); };
    const Frame dust_before = tune_colour_frozen(draw_dust);
    struct Change { const char* path; const char* word; std::uint32_t pixels; };
    const std::array<Change, 3> movement{ {
        { "presentation.movement_feedback.colour", "RED", 0xFF0000 },
        { "presentation.movement_feedback.highlight", "35BCE1", 0x35BCE1 },
        { "presentation.movement_feedback.keyline", "2266CC", 0x2266CC }
    } };
    for (const Change& change : movement)
    {
        command(std::string("tune ") + change.path + ' ' + change.word);
        const Frame after = tune_colour_frozen(draw_dust);
        const size_t matching = tune_colour_pixels(after, change.pixels);
        require(matching > 0 && foreground_local_delta(after, dust_before, { 240, 240, 120, 120 }) > 0,
            "existing movement feedback did not adopt the tuned colour locally");
        require(dust.effects().size() == 1 && dust.effects()[0].age == 0 && !dust.effects()[0].paint_colour,
            "tuning movement dust advanced or replaced its captured effect");
        records << "movement," << change.path << ",live," << matching << ",passed\n";
        save(foreground_crop(after, { 240, 240, 120, 120 }, 0), output /
            ("tune-colours-" + std::string(change.path).substr(std::string(change.path).find_last_of('.') + 1) + "-detail.ppm"));
        reset(change.path);
        require(tune_colour_frozen(draw_dust).pixels == dust_before.pixels,
            "reset did not restore the existing movement feedback raster");
    }

    MenuLevelSettings plan; plan.set_stage("movement_course"); plan.set_player_count(1);
    plan.set_screen_layout(ScreenLayout::one_player); plan.begin_input_claims(input_slots::keyboard_and_mouse);
    plan.set_player_team(0, PlayerTeam::a); plan.set_player_weapon(0, WeaponType::roller);
    auto match = std::make_unique<GameLevel>(&data, plan); GameLevel* game = match.get();
    stack.notify_activation(true); stack.push(std::move(match));
    Level& level = const_cast<Level&>(game->level());
    const DebugText text(app.render_resources(), app.resolution_manager(), &presentation.debug_overlay());
    const auto draw_text = [&]
    { return tune_colour_draw(app, black, [&](DrawList& list) { text.draw_debug_info(list, &level.player(0), 0, 0, 0); }); };
    const Frame text_before = tune_colour_frozen(draw_text);
    const auto state_before = observation(level); const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
    const std::array<Change, 2> debug_text{ {
        { "presentation.debug_overlay.colour", "51C782", 0x51C782 },
        { "presentation.debug_overlay.shadow_colour", "9154D2", 0x9154D2 }
    } };
    for (const Change& change : debug_text)
    {
        command(std::string("tune ") + change.path + ' ' + change.word);
        const Frame after = tune_colour_frozen(draw_text);
        const size_t matching = tune_colour_pixels(after, change.pixels);
        require(matching > 0 && after.pixels != text_before.pixels,
            "existing ordinary DebugText did not adopt its tuned foreground or shadow");
        require(state_before == observation(level) &&
            MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
            "debug colour tuning or drawing changed the frozen match");
        records << "debug-text," << change.path << ",live," << matching << ",passed\n";
        save(foreground_crop(after, { 0, 0, 640, 160 }, 0), output /
            (std::string("tune-colours-debug-") + (change.pixels == 0x51C782 ? "foreground" : "shadow") + "-detail.ppm"));
        reset(change.path); require(tune_colour_frozen(draw_text).pixels == text_before.pixels,
            "reset did not restore the existing ordinary DebugText raster");
    }

    command("overlay.stations on"); require(level.station_label_count() > 0, "actual match created no station labels");
    const Label* station = &level.station_label(0); const int station_count = level.station_label_count();
    const auto draw_station = [&]
    {
        return tune_colour_draw(app, black, [&](DrawList& list) { station->draw(list); },
            Camera(station->bounds().top_left() - Vector2F(32, 32), 1.0f));
    };
    const Frame station_before = tune_colour_frozen(draw_station);
    const std::string station_path = "presentation.debug_overlay.station_label.colour";
    command("tune " + station_path + " 2060C080"); level.refresh_layout();
    const Frame station_after = tune_colour_frozen(draw_station);
    require(station == &level.station_label(0) && station_count == level.station_label_count() &&
        tune_colour_pixels(station_after, 0x103060) > 0 && station_before.pixels != station_after.pixels,
        "same-size station refresh did not recolour the existing label with premultiplied RGBA");
    command("overlay.stations off"); level.refresh_layout();
    const Colour hidden = station->colour();
    require(hidden.r == 0 && hidden.g == 0 && hidden.b == 0 && hidden.a == 0,
        "hidden station label retained colour energy");
    command("overlay.stations on"); level.refresh_layout();
    require(tune_colour_frozen(draw_station).pixels == station_after.pixels,
        "re-enabled existing station label lost its tuned RGBA colour");
    save(foreground_crop(station_after, { 0, 0, 480, 96 }, 0), output / "tune-colours-station-rgba-detail.ppm");
    reset(station_path); level.refresh_layout();
    require(tune_colour_frozen(draw_station).pixels == station_before.pixels,
        "station colour reset did not restore the same label raster");
    command("overlay.stations off");
    records << "station," << station_path << ",live," << tune_colour_pixels(station_after, 0x103060) << ",passed\n";

    // A valid colour is rejected when an unrelated resource in the complete
    // scratch candidate is invalid. This never edits an actual content file.
    JsonSpanDocument staged_document(real_bytes[1], "native scratch presentation");
    const JsonLeaf* font = staged_document.find("debug_overlay.font");
    require(font != nullptr, "native resource validation fixture has no debug font leaf");
    const std::string invalid = staged_document.replaced(*font, "\"native_missing_debug_font\"");
    tune_colour_write(staged / "presentation.json", invalid); impl.set_content_directories(source, staged);
    const DebugStatus rejected = impl.execute("tune presentation.debug_overlay.colour RED");
    require(!rejected.ok && rejected.message.find("native_missing_debug_font") != std::string::npos &&
        impl.tune_document("presentation")->edits().empty() &&
        tune_colour_frozen(draw_text).pixels == text_before.pixels,
        "invalid full resource candidate changed live presentation or its edit ledger");
    tune_colour_write(staged / "presentation.json", real_bytes[1]); impl.set_content_directories(source, staged);
    records << "resource-rejection,presentation.debug_overlay.colour,unchanged,0,passed\n";

    command("tune presentation.debug_overlay.colour RED");
    command("reload"); require(impl.can_apply() && impl.request().restart, "actual match reload did not queue validated scratch content");
    const std::string token = impl.tune_token("presentation.debug_overlay.colour");
    for (const char* line : { "tune presentation.debug_overlay.colour BLUE", "tune.reset presentation",
        "tune.save presentation" })
    {
        const DebugStatus queued = impl.execute(line);
        require(!queued.ok && queued.message.find("reload") != std::string::npos &&
            impl.can_apply() && impl.tune_token("presentation.debug_overlay.colour") == token,
            "queued actual reload allowed a colour edit/reset/save or changed its pending state");
    }
    stack.update(0.0f);
    require(!impl.can_apply() && impl.tune_document("presentation")->edits().empty() &&
        presentation.debug_overlay().colour == original_presentation.debug_overlay().colour &&
        game->level().state() == LevelState::start_countdown,
        "actual scratch reload did not rebuild and rebase before resuming colour edits");
    records << "queued-reload,presentation.debug_overlay.colour,rebuild,0,passed\n";
    impl.set_content_directories(root / "absent-source", staged);
    command("reload"); require(impl.can_apply(), "missing source files did not use valid scratch staged reload content");
    stack.update(0.0f);
    require(!impl.can_apply() && presentation.debug_overlay().colour == original_presentation.debug_overlay().colour &&
        std::any_of(impl.log().lines().begin(), impl.log().lines().end(), [](const DebugLogLine& line)
            { return line.text.find("(staged copy)") != std::string::npos; }),
        "scratch reload fallback did not apply or report its staged source");
    impl.set_content_directories(source, staged);
    records << "staged-reload,presentation,staged-fallback,0,passed\n";
    stack.clear();

    const auto home = [&] { stack.clear(); stack.push(std::make_unique<MainMenuHome>(&context)); };
    const auto mode = [&] { stack.push(std::make_unique<MainMenuModeSelect>(&context)); };
    const auto menu_frame = [&] { return main_menu_frame(app); };
    home(); const Frame old_home = tune_colour_frozen(menu_frame);
    mode(); const Frame old_mode = tune_colour_frozen(menu_frame);
    const std::string home_path = "presentation.main_menu.backgrounds.home";
    command("tune " + home_path + " 345678FF");
    require(tune_colour_frozen(menu_frame).pixels == old_mode.pixels,
        "an already open Mode page adopted a next-screen colour");
    stack.pop(); require(tune_colour_frozen(menu_frame).pixels == old_home.pixels,
        "the retained Home page adopted a next-screen colour");
    home(); const Frame new_home = tune_colour_frozen(menu_frame);
    require(new_home.pixels != old_home.pixels && monitor_rgb(new_home, 8, 8) == 0x345678,
        "reopened Home did not adopt its validated RGBA background");
    mode(); const Frame new_mode = tune_colour_frozen(menu_frame);
    require(new_mode.pixels != old_mode.pixels && monitor_rgb(new_mode, 8, 8) == 0x345678,
        "reopened Mode did not adopt the same next-screen background");
    reset(home_path);
    require(tune_colour_frozen(menu_frame).pixels == new_mode.pixels,
        "reset recoloured the already open Mode page");
    stack.pop(); require(tune_colour_frozen(menu_frame).pixels == new_home.pixels,
        "reset recoloured the retained tuned Home page");
    home(); require(tune_colour_frozen(menu_frame).pixels == old_home.pixels,
        "reset and reopening did not restore the original Home pixels");
    mode(); require(tune_colour_frozen(menu_frame).pixels == old_mode.pixels,
        "reset and reopening did not restore the original Mode pixels");
    save(old_home, output / "tune-colours-home-original-detail.ppm");
    save(new_home, output / "tune-colours-home-reopened-detail.ppm");
    save(foreground_crop(new_mode, { 0, 0, 160, 96 }, 0), output / "tune-colours-mode-reopened-detail.ppm");
    records << "home-and-mode," << home_path << ",next-screen,1,passed\n";
    command("tune " + home_path + " 345678FF"); command("tune.diff presentation"); command("tune.save presentation");
    const Presentation saved((source / "presentation.json").string().c_str());
    require(saved.main_menu().home_background_colour == Colour("345678FF") &&
        impl.tune_document("presentation")->edits().empty(), "scratch colour save did not persist/rebase its token");
    records << "scratch-save," << home_path << ",saved,1,passed\n";
    for (size_t index = 0; index < 2; ++index)
    { require(tune_colour_bytes(content / files[index]) == real_bytes[index], "native colour validation changed actual staged content"); }
    require(records.good(), "tune-colour evidence record write failed");
    std::cout << "PASS tune colours: validated named/RGB/RGBA commands, six existing live consumers and "
        "captured/reopened Home+Mode, reset reversal, real resource rejection and queued scratch reload/rebase; "
        "private save, frozen pixels/state/RNG and original content preserved. Debug-only hidden720p renderer scope; "
        "no physical-device, listening, human readability or performance acceptance.\n";
}
