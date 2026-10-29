// Application::set_resolution resizes the window with SetWindowPos and no
// SWP_NOACTIVATE, which activates even a hidden window and makes it the
// foreground (Labrador 19cf650; report 16 kept the window hidden but not this,
// and report 23 asks for it).
// A thread-local CBT hook refuses activation of only that probe while
// preserving every other application's activation and the production call.
class SettingsProbeActivationGuard
{
public:
    explicit SettingsProbeActivationGuard(HWND window)
    {
        require(target_ == nullptr, "Settings activation guards must not nest");
        target_ = window;
        this->hook_ = SetWindowsHookExW(WH_CBT, &SettingsProbeActivationGuard::filter, nullptr, GetCurrentThreadId());
        if (this->hook_ == nullptr) { target_ = nullptr; }
        require(this->hook_ != nullptr, "Cannot prevent the settings probe from taking foreground");
    }
    ~SettingsProbeActivationGuard()
    {
        UnhookWindowsHookEx(this->hook_);
        target_ = nullptr;
    }
    SettingsProbeActivationGuard(const SettingsProbeActivationGuard&) = delete;
    SettingsProbeActivationGuard& operator=(const SettingsProbeActivationGuard&) = delete;
private:
    static LRESULT CALLBACK filter(int code, WPARAM word, LPARAM parameter)
    {
        if (code == HCBT_ACTIVATE && reinterpret_cast<HWND>(word) == target_) { return 1; }
        return CallNextHookEx(nullptr, code, word, parameter);
    }
    inline static thread_local HWND target_ = nullptr;
    HHOOK hook_ = nullptr;
};


// Independent status copy, fitting and save/reload checks use their own profile.
// They deliberately exercise callbacks after replacing the source text/font.
void save_status_checks(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& output, bool controls_pages)
{
    using Status = decltype(MenuText{}.status);
    const Presentation original = presentation;
    const Status legacy = [&]
    {
        Status words = original.menu_text().status;
        words.options_pending = L"Apply to save changes.";
        words.options_saved = L"Settings applied and saved.";
        words.options_save_failed = L"Settings active for this run. Could not save for next time.";
        words.controls_pending = L"Apply to save changes for all players.";
        words.controls_saved = L"Applied and saved for all players.";
        words.active_save_failed = L"Active for this run. Could not save.";
        words.sticks_pending = L"Apply to save pad settings.";
        words.sticks_saved = L"Pad settings applied and saved.";
        return words;
    }();
    const auto profile = output / (controls_pages ? "status-controls-profile" : "status-options-profile");
    struct StatusProfile
    {
        std::wstring previous;
        explicit StatusProfile(const std::filesystem::path& path)
        {
            wchar_t* raw = nullptr; size_t length = 0;
            require(_wdupenv_s(&raw, &length, L"LOCALAPPDATA") == 0, "Cannot read isolated status profile");
            if (raw != nullptr) { this->previous = raw; std::free(raw); }
            std::filesystem::create_directories(path);
            require(_wputenv_s(L"LOCALAPPDATA", path.c_str()) == 0, "Cannot isolate save-status writes");
        }
        ~StatusProfile() { _wputenv_s(L"LOCALAPPDATA", this->previous.c_str()); }
    } isolated(profile);
    const auto save_path = profile / "ColourWars" / "save_data.json";
    const auto blocked = std::filesystem::path(save_path.wstring() + L".tmp");
    Save preferences;
    const auto reset = [&]
    {
        // Save lends out an effective copy of its stored play settings, so a
        // reset goes through the setter: both copies, and the file, start over.
        PlaySettings play; play.music_volume = play.sfx_volume = 0;
        require(preferences.set_play_settings_and_save(play) &&
            preferences.set_display_settings_and_save(ScreenResolution::s_1280_720,
            false, TeamColourPair::blue_yellow), "Cannot seed the isolated status save");
    };
    const auto saved_bytes = [&]
    {
        std::ifstream stream(save_path, std::ios::binary);
        require(stream.good(), "Status fixture lost its isolated save");
        return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    };
    reset();
    GameData data; data.set_application(&app); data.set_save(&preferences);
    data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MatchSetup settings;
    MenuContext context(&data, &input, &settings); StateContext& stack = app;
    struct RestoreStatus
    {
        StateContext& stack; Presentation& presentation; Presentation original;
        std::filesystem::path blocked;
        ~RestoreStatus()
        {
            this->stack.clear(); this->presentation = this->original;
            std::error_code ignored; std::filesystem::remove(this->blocked, ignored);
        }
    } restore{ stack, presentation, original, blocked };
    const auto tick = [&] { app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.0f); };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick();
        // This historical raster fixture samples the settled ornament pose.
        app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.51f);
    };
    const auto resize = [&](Vector2I size)
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
    };
    // 0: Options, 1: Controls/audio, 2: Pad sticks child.
    const auto open = [&](int page)
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (Key key : { Key::up, Key::down, Key::left, Key::right, Key::enter, Key::space, Key::escape })
            { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll();
        input.reset_actions(); stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        if (page == 0) { stack.push(std::make_unique<MainMenuOptions>(&context)); }
        else { stack.push(std::make_unique<ControlsSettingsMenu>(&context)); }
        tick(); tick();
        if (page == 2) { tap(Key::enter); tick(); }
        require(stack.depth() == (page == 2 ? 3 : 2), "Status fixture opened the wrong production page");
    };
    const auto stage = [&](int page)
    {
        tap(Key::down);
        if (page == 0 || page == 1) { tap(Key::down); }
        if (page == 1) { tap(Key::down); tap(Key::down); }
        tap(Key::right);
        const int remaining = page == 0 ? 3 : page == 1 ? 5 : 6;
        for (int row = 0; row < remaining; ++row) { tap(Key::down); }
    };
    const auto apply = [&](int page)
    {
        if (page == 0)
        {
            // The real windowed Apply path keeps a hidden window hidden and
            // inactive; this pins that engine contract. It is the direct-save
            // route: a display trial would call set_resolution, whose resize
            // activates a hidden window (report 23).
            const HWND window = static_cast<HWND>(app.window());
            tap(Key::enter); tick();
            require(stack.depth() == 2, "Status Apply pushed a display trial; it must take the direct-save route");
            require(IsWindowVisible(window) == FALSE && !app.fullscreen() && GetForegroundWindow() != window,
                "Status Apply exposed or activated its hidden probe");
        }
        else { tap(Key::enter); }
    };
    const auto copy = [](const Status& words, int page, int state) -> const std::wstring&
    {
        if (page == 0) { return state == 0 ? words.options_pending : state == 1 ? words.options_saved : words.options_save_failed; }
        if (page == 1) { return state == 0 ? words.controls_pending : state == 1 ? words.controls_saved : words.active_save_failed; }
        return state == 0 ? words.sticks_pending : state == 1 ? words.sticks_saved : words.active_save_failed;
    };
    const auto font_for = [](const Presentation& snapshot, int page) -> const std::string&
        { return page == 0 ? snapshot.prompt_bar().font : snapshot.controls_settings().help_font; };
    std::ofstream records(output / (controls_pages ? "status-controls.csv" : "status-options.csv"));
    records << "case,page,state,width,height,font,raw_width,raw_height,scale,fitted_width,fixture_matches\n";
    const auto compare = [&](const std::string& name, int page, int state, const Presentation& snapshot,
        const Status& words, bool must_fit = false)
    {
        MenuPaperArtPresentation art = page == 0 ? snapshot.main_menu().art : snapshot.controls_settings().art;
        if (page == 0)
        {
            art.focus_padding = snapshot.main_menu().setup_focus_padding;
            art.shadow_offset = snapshot.main_menu().setup_shadow_offset;
        }
        const auto position = page == 0 ? snapshot.main_menu().options_status_position : snapshot.controls_settings().status_position;
        const auto paper_size = page == 0 ? snapshot.main_menu().options_box_size : snapshot.controls_settings().box_size;
        const RectangleF paper((Vector2F(1920.0f, snapshot.menu().footer_y) - paper_size) / 2.0f, paper_size);
        const float available = paper.right() - art.shadow_offset - art.focus_padding.x - position.x;
        const std::wstring& value = copy(words, page, state);
        const Colour colour = state == 2 ? snapshot.main_menu().options_warning_colour : art.focus_ink;
        UiText text("reference-save-status", value, font_for(snapshot, page), position, app.render_resources(), colour);
        const Vector2F raw = text.bounds().size(); const float scale = std::min(1.0f, available / raw.x);
        require(!must_fit || scale < 1.0f, "Authored status did not exercise width fitting"); text.set_scale(scale);
        require(paper.contains(text.bounds()), "Independent fitted status escaped the paper");
        // Preserve every measured glyph, including descenders within the
        // paper's bevel. The reference reconstructs that complete background.
        const RectangleF region = text.bounds();
        const Frame actual = main_menu_frame(app);
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app.viewport_manager()->fullscreen_viewport());
        list.set_camera(camera); list.set_filter(TextureFilter::point);
        Canvas(app.render_resources(), snapshot.main_menu().home_background_colour).draw(list,
            camera.visible_rectangle(app.viewport_manager()->fullscreen_viewport()));
        FocusGroup focus;
        const MenuPaperArtwork artwork(paper, app.render_resources(), &snapshot.menu(), &art, &focus);
        artwork.draw(list);
        list.set_filter(TextureFilter::linear); text.draw(list); const Frame reference = read(renderer);
        RectangleF projected = region; camera.calculate_view_rectangle(projected);
        const RectangleI pixels = countdown_crop_bounds(projected);
        const Frame observed = foreground_crop(actual, pixels, 0), wanted = foreground_crop(reference, pixels, 0);
        const std::string stem = "save-status-" + name + '-' + std::to_string(page) + '-' + std::to_string(state) + '-' +
            std::to_string(actual.width) + 'x' + std::to_string(actual.height);
        if (observed.pixels != wanted.pixels)
        {
            save(observed, output / (stem + "-failure-actual.ppm")); save(wanted, output / (stem + "-failure-expected.ppm"));
            throw std::runtime_error("Actual save status differs from its independent full-line linear fixture");
        }
        save(observed, output / (stem + "-detail.ppm"));
        records << name << ',' << page << ',' << state << ',' << actual.width << ',' << actual.height << ',' <<
            font_for(snapshot, page) << ',' << raw.x << ',' << raw.y << ',' << scale << ',' << text.bounds().width << ",true\n";
        std::srand(7331); const int next_random = std::rand(); std::srand(7331);
        require(actual.pixels == main_menu_frame(app).pixels && std::rand() == next_random,
            "Status redraw changed frozen pixels or consumed random numbers");
    };
    const auto failure = [&](int page)
    {
        require(std::filesystem::create_directory(blocked), "Cannot refuse the isolated status save");
        apply(page);
        require(std::filesystem::remove(blocked), "Cannot release the isolated status save refusal");
    };
    const std::array<int, 2> pages = controls_pages ? std::array<int, 2>{ 1, 2 } : std::array<int, 2>{ 0, -1 };
    for (int page : pages)
    {
        if (page < 0) { continue; }
        for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440),
            Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
        {
            presentation = original; reset(); resize(size); open(page); const std::string before = saved_bytes(); stage(page);
            require(saved_bytes() == before, "Legacy pending status wrote preferences before Apply");
            compare("legacy-pending", page, 0, original, legacy); apply(page); resize(size);
            compare("legacy-saved", page, 1, original, legacy); const std::string committed = saved_bytes();
            failure(page); resize(size); require(saved_bytes() == committed, "Failed legacy status replaced saved JSON");
            compare("legacy-failed", page, 2, original, legacy); apply(page); resize(size);
            compare("legacy-recovered", page, 1, original, legacy);
        }
    }
    Status old_words = legacy, next_words = legacy;
    const std::array<std::wstring Status::*, 8> members{ &Status::options_pending, &Status::options_saved,
        &Status::options_save_failed, &Status::controls_pending, &Status::controls_saved, &Status::active_save_failed,
        &Status::sticks_pending, &Status::sticks_saved };
    for (size_t index = 0; index < members.size(); ++index)
    {
        if ((controls_pages && index < 3) || (!controls_pages && index >= 3)) { continue; }
        old_words.*members[index] = L"Retained~ " + std::to_wstring(index) + L": ";
        next_words.*members[index] = L"Reopened " + std::to_wstring(index) + L": ";
        for (int repeat = 0; repeat < 16; ++repeat)
        {
            old_words.*members[index] += L"previous settings status; ";
            next_words.*members[index] += L"current settings status; ";
        }
        // Saved messages are deliberately short to prove scale resets after
        // a fitted pending/failure line, rather than keeping its old shrink.
        if (index == 1 || index == 4 || index == 7)
        {
            old_words.*members[index] = L"Saved~ " + std::to_wstring(index) + L'.';
            next_words.*members[index] = L"Saved " + std::to_wstring(index) + L'.';
        }
    }
    const Font* source = app.render_resources()->font(app.render_resources()->resolve_sprite_font(
        controls_pages ? original.controls_settings().help_font : original.prompt_bar().font));
    std::vector<Glyph> glyphs;
    for (char32_t character = U' '; character < U'~'; ++character)
    {
        const Glyph* glyph = source->find(character); require(glyph != nullptr, "Status font lacks printable baseline glyph");
        glyphs.push_back(*glyph);
    }
    const std::string replacement = controls_pages ? "acceptance_controls_status_no_tilde" : "acceptance_options_status_no_tilde";
    app.render_resources()->add_font(replacement, std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
    require(!app.render_resources()->can_render(app.render_resources()->resolve_sprite_font(replacement), L"~"),
        "Replacement status font still renders the retained page's old glyph");
    Presentation old = original; const_cast<MenuText&>(old.menu_text()).status = old_words;
    Presentation next = original; const_cast<MenuText&>(next.menu_text()).status = next_words;
    // Full-width fitted status messages can occupy the bottom of the help
    // column. Compact authored help deliberately leaves that space clear.
    if (controls_pages)
    {
        for (Presentation* snapshot : { &old, &next })
        {
            auto& help = const_cast<MenuText&>(snapshot->menu_text()).help;
            help.controls = L"Controls: move, aim and paint. Jump: {jump}.";
            help.pad_association = L"Profiles follow pad slots.";
        }
    }
    if (controls_pages) { const_cast<ControlsSettingsPresentation&>(next.controls_settings()).help_font = replacement; }
    else { const_cast<PromptBarPresentation&>(next.prompt_bar()).font = replacement; }
    old.validate_resources(*app.render_resources(), *app.audio_resources()); next.validate_resources(*app.render_resources(), *app.audio_resources());
    resize({ 1280, 720 });
    for (int page : pages)
    {
        if (page < 0) { continue; }
        for (bool reopened : { false, true })
        {
            reset(); presentation = reopened ? next : old; open(page); const auto before = saved_bytes(); stage(page);
            const Presentation& snapshot = reopened ? next : old; const Status& words = reopened ? next_words : old_words;
            compare(reopened ? "reopened-new-pending" : "custom-old-pending", page, 0, snapshot, words, true);
            presentation = reopened ? old : next; tick();
            const Frame pending = main_menu_frame(app);
            app.keyboard()->on_key_down(Key::enter); tick(); static_cast<WindowNotify&>(app).on_deactivated(); tick();
            static_cast<WindowNotify&>(app).on_activated(); tick(); app.keyboard()->on_key_up(Key::enter); tick(); tick();
            require(saved_bytes() == before && pending.pixels == main_menu_frame(app).pixels,
                "Interrupted status Apply saved or changed the retained pending page");
            compare("retained-after-interruption", page, 0, snapshot, words, true);
            apply(page); compare("retained-saved", page, 1, snapshot, words);
            const auto committed = saved_bytes(); require(committed != before, "Fresh status Apply did not save pending edits");
            apply(page); require(saved_bytes() == committed, "Repeated status Apply changed saved preferences");
            compare("retained-repeated-saved", page, 1, snapshot, words);
            failure(page); require(saved_bytes() == committed, "Retained failed status replaced previous JSON");
            compare("retained-failed", page, 2, snapshot, words, true);
            apply(page); compare("retained-recovered", page, 1, snapshot, words);
            Save reloaded; reloaded.load_save_file();
            require(reloaded.resolution() == preferences.resolution() && reloaded.team_palette() == preferences.team_palette() &&
                reloaded.play_settings().music_volume == preferences.play_settings().music_volume &&
                reloaded.play_settings().sticks[0].movement_deadzone == preferences.play_settings().sticks[0].movement_deadzone,
                "Retained status callbacks lost independently reloaded preferences");
            // Reopen adopts the current source and both discard routes remain write-free.
            for (bool escape : { false, true })
            {
                presentation = snapshot; open(page); const auto discard_before = saved_bytes(); stage(page);
                compare("reopened-before-discard", page, 0, snapshot, words, true);
                if (escape) { tap(Key::escape); }
                else { tap(Key::down); tap(Key::enter); }
                tick(); require(saved_bytes() == discard_before, "Status reload changed Back/Escape discard into a save");
            }
        }
    }
    if (controls_pages)
    {
        reset(); presentation = old; open(1); stage(1);
        for (int row = 0; row < 9; ++row) { tap(Key::up); }
        presentation = next; tap(Key::enter); tick(); require(stack.depth() == 3, "Status parent failed to open a newer child");
        stage(2); compare("new-child-pending", 2, 0, next, next_words, true); apply(2);
        compare("new-child-saved", 2, 1, next, next_words);
        const int cutoff = preferences.play_settings().sticks[0].movement_deadzone;
        tap(Key::down); tap(Key::enter); tick(); require(stack.depth() == 2, "Status child Back lost its suspended parent");
        compare("old-parent-after-child", 1, 0, old, old_words, true);
        for (int row = 0; row < 9; ++row) { tap(Key::down); }
        apply(1); compare("old-parent-saved-after-child", 1, 1, old, old_words);
        Save reloaded; reloaded.load_save_file();
        require(reloaded.play_settings().music_volume == 10 && reloaded.play_settings().sticks[0].movement_deadzone == cutoff,
            "Retained parent status Apply lost its newer child's committed pad profile");
    }
    require(records.good(), "Save-status evidence write failed");
    std::cout << "PASS save-status copy: independent pending/saved/failure defaults across six sizes/aspects; width-only "
        "fitting and short-message scale reset; missing-old-glyph fonts, retained/reopened/reversed callbacks, "
        "interrupted/repeated Apply, controlled failure/recovery, isolated reload and Back/Escape discard" <<
        (controls_pages ? "; old parent/new child merge\n" : "\n");
}
