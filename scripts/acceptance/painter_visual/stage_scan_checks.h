// ROAD#32: actual optional-folder scans, Stage Select and the diagnostic child.
// Controlled raw diagnostics exercise wrapping; no physical input is claimed.
struct StageScanStderrCapture
{
    int saved = -1;
    explicit StageScanStderrCapture(const std::filesystem::path& path)
    {
        std::fflush(stderr); this->saved = _dup(_fileno(stderr));
        require(this->saved >= 0, "Cannot retain the probe stderr descriptor");
        FILE* redirected = nullptr;
        if (_wfreopen_s(&redirected, path.c_str(), L"w", stderr) != 0)
        { _close(this->saved); this->saved = -1; throw std::runtime_error("Cannot capture isolated stage-scan diagnostics"); }
    }
    void finish()
    {
        if (this->saved >= 0) { std::fflush(stderr); _dup2(this->saved, _fileno(stderr)); _close(this->saved); this->saved = -1; std::clearerr(stderr); }
    }
    ~StageScanStderrCapture() { this->finish(); }
};

void stage_scan_matrix(Application& app, const Canvas& canvas, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& content, const std::filesystem::path& output)
{
    const NestedSaveProfile isolated(output / "stage-scan-profile");
    Save preferences; auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0; play.reduce_effects = true;
    Registry<LevelLoadedInfo> levels("stage-scan-inspection"); StageList stages;
    auto shipped = read_stage_definition(path_to_utf8(content / "levels/movement_course.json"), "movement_course");
    stages.push_back(shipped.stage); levels.add("movement_course", std::move(shipped.info));
    const auto folder_path = output / "stage-scan-profile/ColourWars/stages";
    std::filesystem::create_directories(folder_path);
    const auto valid_path = folder_path / "my_course.json", broken_path = folder_path / "broken_stage.json";
    std::ifstream source(content / "levels/movement_course.json", std::ios::binary);
    require(source.good(), "Cannot read stage-scan valid source");
    const std::string valid((std::istreambuf_iterator<char>(source)), std::istreambuf_iterator<char>());
    const auto write = [&](const std::filesystem::path& path, const std::string& value)
    { std::ofstream file(path, std::ios::binary); file << value; file.close(); require(file.good(), "Cannot write isolated stage-scan fixture"); };
    write(valid_path, valid); write(broken_path, "{broken");
    UserStageFolder folder; folder.attach(folder_path, { &tuning, &presentation, app.render_resources(),
        app.audio_resources(), &play }, app.audio_resources(), &levels, &stages);
    const auto diagnostic_path = output / "stage-scan-stderr.txt";
    StageScanStderrCapture captured_errors(diagnostic_path);
    int scans = 0; folder.scan(); ++scans;
    require(folder.latest_scan().accepted == std::vector<std::string>{ "my_course" } &&
        folder.latest_scan().refused.size() == 1 && stages.size() == 2 && stages.back().user,
        "One broken optional stage prevented the good stage from joining");
    const std::string first_refusal = folder.latest_scan().refused[0];
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_tuning(&tuning);
    data.set_presentation(&presentation); data.set_level_infos(&levels); data.set_stages(&stages);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MenuLevelSettings settings;
    settings.set_player_count(1); settings.begin_input_claims(input_slots::keyboard_and_mouse);
    settings.set_screen_layout(ScreenLayout::one_player); settings.set_stage("movement_course");
    settings.set_player_team(0, PlayerTeam::a); settings.set_player_weapon(0, WeaponType::sprayer); settings.set_duration(73.0f);
    MenuContext context(&data, &input, &settings); StateContext& stack = app;
    context.set_stage_rescan([&] { folder.rescan(); ++scans; });
    context.set_stage_scan([&]() -> const UserStageScan* { return &folder.latest_scan(); });
    struct ClearStageScanStates { StateContext& stack; ~ClearStageScanStates() { this->stack.clear(); } } clear{ stack };
    int updates = 0, level_updates = 0, reads = 0, frames = 0, crops = 0, rows = 0, comparisons = 0;
    const auto tick = [&]
    { require(++updates <= 500, "Stage scan native input exceeded500 updates"); app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(0.0f); };
    const auto tap = [&](Key key)
    { app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick(); };
    const auto frame = [&]() { ++reads; return main_menu_frame(app); };
    const auto resize = [&](const Vector2I& size)
    { app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y); };
    const auto prepare = [&]
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::enter, Key::escape, Key::left, Key::right, Key::up, Key::down, Key::space }) { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll(); input.reset_actions(); input.reset_directions();
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
    };
    const auto unchanged = [&]
    {
        require(settings.player_count() == 1 && settings.player_settings()[0].num == input_slots::keyboard_and_mouse &&
            settings.player_team(0) == PlayerTeam::a && settings.player_weapon(0) == WeaponType::sprayer &&
            settings.duration() == 73.0f && !context.music_exit_pending(), "Stage diagnostic navigation changed the chosen roster/rules");
    };
    const auto ascii = [](const std::string& value)
    { std::wstring result; for (const char character : value) { const auto byte = static_cast<unsigned char>(character);
        require(byte >= 0x20 && byte <= 0x7e, "ASCII oracle contains non-printable bytes"); result.push_back(byte); } return result; };
    const auto& look = presentation.main_menu().stage_scan; const auto& copy = presentation.menu_text().stage_scan;
    const auto& art = presentation.main_menu().art; const auto& style = presentation.menu();
    const Font* font = app.render_resources()->font(app.render_resources()->resolve_sprite_font(look.font));
    const auto layout = stage_scan_diagnostics::layout(look, art, style.footer_y, *font, copy);
    std::ofstream records(output / "stage-scan.csv");
    records << "case,width,height,role,file,page,pages,x,y,text_width,text_height,crop_x,crop_y,crop_width,crop_height,raster_equal\n";
    const auto progress = [&](size_t file, size_t files, size_t page, size_t pages)
    { return L"Issue " + std::to_wstring(file) + L" of " + std::to_wstring(files) + L"   Page " + std::to_wstring(page) + L" of " + std::to_wstring(pages); };
    require(copy.counter == L"Issue {file} of {files}   Page {page} of {pages}", "Stage scan provisional progress wording changed");
    const auto compare = [&](const char* name, const std::wstring& body, size_t file, size_t files, size_t page, size_t pages,
        bool capture_context = false, bool capture_body = false)
    {
        unchanged(); const Frame actual = frame(); std::srand(93232); const int next = std::rand(); std::srand(93232);
        require(actual.pixels == frame().pixels && std::rand() == next, "Stage diagnostic drawing changed frozen pixels or RNG");
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), app.viewport_manager()->fullscreen_viewport());
        const std::array<RectangleF, 4> boxes{ layout.heading, layout.unicode_hint, layout.body, layout.progress };
        const std::array<std::wstring, 4> words{ copy.heading, copy.unicode_hint, body, progress(file, files, page, pages) };
        constexpr std::array<const char*, 4> roles{ "heading", "unicode", "body", "progress" };
        for (size_t role = 0; role < roles.size(); ++role)
        {
            const float scale = role == 0 ? look.heading_scale : look.text_scale;
            UiText text("independent-diagnostic-text", words[role], look.font, boxes[role].position(), app.render_resources(), art.ink);
            text.set_scale(scale);
            if (role == 3 && text.bounds().width > boxes[role].width) { text.set_scale(scale * boxes[role].width / text.bounds().width); }
            const RectangleF bounds = text.bounds();
            require(boxes[role].contains(bounds) && layout.paper.contains(bounds) && bounds.height > 0.0f && bounds.width > 0.0f,
                "Complete stage diagnostic text escapes its reserved paper area");
            Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1); DrawList list = renderer.view(0);
            list.set_viewport(app.viewport_manager()->fullscreen_viewport()); list.set_camera(camera); list.set_filter(TextureFilter::point);
            const UiTexture background("diagnostic-reference-background", style.sprite_sheet, style.fill_frame,
                RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), app.render_resources(), CrudeDrawing::premultiplied(presentation.main_menu().home_background_colour));
            background.draw(list); FocusGroup no_focus;
            const MenuPaperArtwork paper(layout.paper, app.render_resources(), &style, &art, &no_focus); paper.draw(list);
            list.set_filter(TextureFilter::linear); text.draw(list); const Frame wanted = read(renderer); ++reads;
            RectangleF projected = bounds; projected.inflate(2.0f); camera.calculate_view_rectangle(projected);
            const RectangleI roi = countdown_crop_bounds(projected);
            const Frame observed = foreground_crop(actual, roi, 0), reference = foreground_crop(wanted, roi, 0);
            if (observed.pixels != reference.pixels)
            {
                save(observed, output / "stage-scan-failure-actual.ppm"); save(reference, output / "stage-scan-failure-expected.ppm");
                throw std::runtime_error(std::string("Actual stage diagnostic text differs from independent supplied copy: ") + name + '/' + roles[role]);
            }
            records << name << ',' << actual.width << ',' << actual.height << ',' << roles[role] << ',' << file << ',' << page << ',' << pages << ',' <<
                bounds.x << ',' << bounds.y << ',' << bounds.width << ',' << bounds.height << ',' << roi.x << ',' << roi.y << ',' << roi.width << ',' << roi.height << ",true\n"; ++rows; ++comparisons;
            if (capture_body && role == 2) { save(observed, output / (std::string("stage-scan-") + name + "-detail.ppm")); ++crops; }
        }
        PromptBarPresentation prompt_look = presentation.prompt_bar(); prompt_look.font = look.font;
        prompt_look.position = look.prompt_position + Vector2F(0.0f, style.footer_y);
        prompt_look.shadow_offset = look.prompt_shadow_offset * look.text_scale;
        PromptBar prompt("independent-diagnostic-prompt", copy.details_prompt, prompt_look, app.render_resources());
        prompt.set_scale(look.text_scale); prompt.set_shadow_scale(look.text_scale);
        const RectangleF footer(0.0f, style.footer_y, menu_consts::DESIGN_RESOLUTION.x, menu_consts::DESIGN_RESOLUTION.y - style.footer_y);
        require(footer.contains(prompt.bounds()), "Diagnostic prompt font and complete shadow escaped the native footer");
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1); DrawList list = renderer.view(0);
        list.set_viewport(app.viewport_manager()->fullscreen_viewport()); list.set_camera(camera); list.set_filter(TextureFilter::point);
        const UiTexture background("prompt-reference-background", style.sprite_sheet, style.fill_frame,
            RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), app.render_resources(), CrudeDrawing::premultiplied(presentation.main_menu().home_background_colour));
        background.draw(list);
        const UiTexture dim("prompt-reference-footer", style.sprite_sheet, style.fill_frame, footer, app.render_resources(), style.scrim_colour); dim.draw(list);
        list.set_filter(TextureFilter::linear); prompt.draw(list); const Frame wanted = read(renderer); ++reads;
        RectangleF projected = prompt.bounds(); projected.inflate(2.0f); camera.calculate_view_rectangle(projected);
        const RectangleI roi = countdown_crop_bounds(projected);
        require(foreground_crop(actual, roi, 0).pixels == foreground_crop(wanted, roi, 0).pixels,
            "Actual diagnostic footer differs from its supplied legend/font/position/scale reference");
        records << name << ',' << actual.width << ',' << actual.height << ",prompt," << file << ',' << page << ',' << pages << ',' <<
            prompt.bounds().x << ',' << prompt.bounds().y << ',' << prompt.bounds().width << ',' << prompt.bounds().height << ',' << roi.x << ',' << roi.y << ',' << roi.width << ',' << roi.height << ",true\n"; ++rows; ++comparisons;
        if (capture_context) { save(actual, output / (std::string("stage-scan-") + name + '-' + std::to_string(actual.width) + 'x' + std::to_string(actual.height) + "-context.ppm")); ++frames; }
        return actual;
    };
    resize({ 1280, 720 }); prepare(); std::optional<MatchStart> choice;
    stack.push<MatchStart>(std::make_unique<MainMenuStageSelect>(&context), [&](const MatchStart& selected) { choice = selected; });
    tick(); require(stack.depth() == 2 && scans == 2 && !choice, "A refused startup file automatically opened a diagnostic modal");
    tap(Key::right); const Frame before_details = frame(); const int before_scans = scans;
    {
        const Vector2F footer_offset(0.0f, style.footer_y);
        const RectangleF footer(footer_offset, Vector2F(menu_consts::DESIGN_RESOLUTION.x, menu_consts::DESIGN_RESOLUTION.y - style.footer_y));
        const RectangleF status_box(look.status.position + footer_offset, look.status.size);
        UiText status("independent-parent-scan-status", L"User-stage scan: 1 issue(s).", look.font,
            status_box.position(), app.render_resources(), style.unhighlight);
        status.set_scale(look.status_scale);
        const float fit = std::min({ 1.0f, status_box.width / status.bounds().width, status_box.height / status.bounds().height });
        status.set_scale(look.status_scale * fit);
        PromptBarPresentation prompt_look = presentation.prompt_bar(); prompt_look.font = look.font;
        prompt_look.position = look.prompt_position + footer_offset; prompt_look.shadow_offset = look.prompt_shadow_offset * look.text_scale;
        PromptBar browse_prompt("independent-parent-scan-prompt", copy.browse_prompt, prompt_look, app.render_resources());
        browse_prompt.set_scale(look.text_scale); browse_prompt.set_shadow_scale(look.text_scale);
        require(footer.contains(status_box) && status_box.contains(status.bounds()) && footer.contains(browse_prompt.bounds()) &&
            status.bounds().top() >= style.footer_y + style.rule_height && status.bounds().bottom() < browse_prompt.bounds().top(),
            "Parent scan summary complete font bounds escape their box/footer or collide with the rule/prompt");
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1); DrawList list = renderer.view(0);
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), app.viewport_manager()->fullscreen_viewport());
        list.set_viewport(app.viewport_manager()->fullscreen_viewport()); list.set_camera(camera); list.set_filter(TextureFilter::point);
        const UiTexture background("status-reference-background", style.sprite_sheet, style.fill_frame,
            RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), app.render_resources(), CrudeDrawing::premultiplied(presentation.main_menu().home_background_colour));
        background.draw(list);
        const UiTexture dim("status-reference-footer", style.sprite_sheet, style.fill_frame, footer, app.render_resources(), style.scrim_colour); dim.draw(list);
        const UiTexture rule("status-reference-rule", style.sprite_sheet, style.fill_frame,
            RectangleF(style.brand_position.x, style.footer_y, menu_consts::DESIGN_RESOLUTION.x - style.brand_position.x * 2.0f, style.rule_height),
            app.render_resources(), style.panel_edge_colour); rule.draw(list);
        list.set_filter(TextureFilter::linear); status.draw(list); const Frame wanted = read(renderer); ++reads;
        RectangleF projected = status.bounds(); projected.inflate(2.0f); camera.calculate_view_rectangle(projected);
        const RectangleI roi = countdown_crop_bounds(projected); const Frame observed = foreground_crop(before_details, roi, 0);
        require(observed.pixels == foreground_crop(wanted, roi, 0).pixels,
            "Actual Stage Select scan summary is absent or differs from its independent literal/font/fit/footer reference");
        save(before_details, output / "stage-scan-parent-status-1280x720-context.ppm"); ++frames;
        save(observed, output / "stage-scan-parent-status-detail.ppm"); ++crops;
        records << "parent-status,1280,720,status,1,1,1," << status.bounds().x << ',' << status.bounds().y << ',' <<
            status.bounds().width << ',' << status.bounds().height << ',' << roi.x << ',' << roi.y << ',' << roi.width << ',' << roi.height << ",true\n";
        ++rows; ++comparisons;
    }
    tap(Key::down); require(stack.depth() == 3 && scans == before_scans, "Stage Select Down did not reach details without rescanning");
    const auto real_pages = stage_scan_diagnostics::pages(*font, first_refusal, layout.body.width / look.text_scale, layout.body.height / look.text_scale);
    std::wstring real_joined; for (auto value : real_pages) { value.erase(std::remove(value.begin(), value.end(), L'\n'), value.end()); real_joined += value; }
    require(real_joined == ascii(first_refusal), "Real refusal pagination lost its retained filename or validation reason");
    compare("actual-refusal", real_pages[0], 1, 1, 1, real_pages.size(), true, true);
    tap(Key::escape); require(stack.depth() == 2 && scans == before_scans && frame().pixels == before_details.pixels,
        "Dismissing details rescanned or lost the browsed Stage Select choice");
    tap(Key::enter); require(stack.depth() == 2 && !choice, "A details dismissal confirmed the stage accidentally");
    tap(Key::down); require(stack.depth() == 2, "A locked Stage Select opened scan details");
    tap(Key::enter); require(choice == MatchStart::menu && stack.depth() == 1 && settings.stage() == "my_course" && scans == before_scans,
        "The preserved good user stage did not reach the actual match-start callback");
    LevelBuilder builder(app.viewport_manager(), app.render_resources(), app.audio_resources(), &play, &levels,
        &tuning, &presentation, app.resolution_manager(), app.thread_pool(), app.partitioner(), app.mouse());
    {
        std::srand(932); auto accepted = builder.build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
        std::vector<PlayerInputData> neutral(input_slots::count); for (auto& action : neutral) { action.connected = true; }
        for (int step = 0; step < 400 && accepted->state() != LevelState::active; ++step) { accepted->update(neutral, dt); ++level_updates; }
        require(accepted->state() == LevelState::active && scans == before_scans, "A valid optional stage could not start without another scan");
    }
    prepare(); stack.push(std::make_unique<MainMenuStageSelect>(&context, Session(3))); tick(); tap(Key::down);
    require(stack.depth() == 2 && scans == before_scans, "Between-round Stage Select scanned or exposed diagnostics");
    context.set_stage_scan({}); context.set_stage_rescan({}); prepare(); stack.push(std::make_unique<MainMenuStageSelect>(&context)); tick(); tap(Key::down);
    require(stack.depth() == 2 && scans == before_scans, "An unattached scan context opened diagnostics or scanned");
    context.set_stage_rescan([&] { folder.rescan(); ++scans; });
    context.set_stage_scan([&]() -> const UserStageScan* { return &folder.latest_scan(); });
    const auto reopen = [&]() { prepare(); stack.push(std::make_unique<MainMenuStageSelect>(&context)); tick(); };
    write(broken_path, "{}"); reopen(); require(folder.latest_scan().refused.size() == 1 && folder.latest_scan().refused[0] != first_refusal,
        "Changing a broken stage did not refresh its latest reason");
    require(std::filesystem::remove(broken_path), "Cannot remove only the isolated bad file"); reopen();
    require(folder.latest_scan().refused.empty(), "Removing a broken stage retained stale diagnostics"); tap(Key::down); require(stack.depth() == 2, "A clean scan still offered old details");
    write(broken_path, "{broken"); reopen(); require(folder.latest_scan().refused.size() == 1, "Restoring a broken file did not reappear on permitted rescan");
    write(broken_path, valid); reopen(); require(folder.latest_scan().refused.empty() && folder.latest_scan().accepted.size() == 2,
        "Fixing the broken file failed to clear its diagnostic and accept both valid stages");
    const int frozen_scans = scans;
    const std::string raw_long = std::string("user stage refused: C:/private/caf\xC3\xA9_\xE9\x9B\xAA.json: field ") + std::string(7000, 'W') + " is invalid\nretain this reason";
    const std::string raw_invalid = std::string("user stage refused: invalid_") + '\xFF' + ".json: bad" + '\x01' + "payload";
    const std::vector<std::string> refusals{ raw_long, raw_invalid };
    const std::array<std::wstring, 2> expected{ std::wstring(L"user stage refused: C:/private/caf\\u00E9_\\u96EA.json: field ") + std::wstring(7000, L'W') + L" is invalid\\u000Aretain this reason",
        L"user stage refused: invalid_\\xFF.json: bad\\x01payload" };
    std::array<std::vector<std::wstring>, 2> pages;
    for (size_t file = 0; file < pages.size(); ++file)
    {
        pages[file] = stage_scan_diagnostics::pages(*font, refusals[file], layout.body.width / look.text_scale, layout.body.height / look.text_scale);
        std::wstring joined; for (auto value : pages[file]) { value.erase(std::remove(value.begin(), value.end(), L'\n'), value.end()); joined += value; }
        require(joined == expected[file], "Diagnostic paging lost the exact independently escaped Unicode/invalid bytes");
    }
    require(pages[0].size() >= 3 && pages[0].size() <= 6 && pages[1].size() == 1, "Long diagnostic fixture missed its bounded page controls");
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440),
        Vector2I(1600, 1200), Vector2I(1920, 1200), Vector2I(2560, 1080) })
    {
        resize(size); prepare(); stack.push(std::make_unique<UserStageRefusalsMenu>(&context, refusals)); tick();
        const Frame first = compare("long-first", pages[0][0], 1, 2, 1, pages[0].size(), true, size.x == 1280);
        tap(Key::up); require(frame().pixels == first.pixels, "The first diagnostic page wrapped on Up");
        tap(Key::down); compare("long-next", pages[0][1], 1, 2, 2, pages[0].size());
        for (size_t page = 2; page < pages[0].size(); ++page) { tap(Key::down); }
        const Frame last = compare("long-last", pages[0].back(), 1, 2, pages[0].size(), pages[0].size(), size.x == 1280, size.x == 1280);
        tap(Key::down); require(frame().pixels == last.pixels, "The last diagnostic page wrapped on Down");
        tap(Key::right); const Frame second = compare("invalid-bytes", pages[1][0], 2, 2, 1, 1, size.x == 1280, size.x == 1280);
        tap(Key::right); compare("file-wrap-reset", pages[0][0], 1, 2, 1, pages[0].size());
        tap(Key::left); require(frame().pixels == second.pixels, "Issue wrap failed to reset the diagnostic page");
        require(scans == frozen_scans && stack.depth() == 2, "Reading diagnostic pages rescanned or mutated the stack");
        tap(Key::enter); require(stack.depth() == 1 && scans == frozen_scans, "Confirm did not dismiss read-only details");
    }
    captured_errors.finish();
    std::ifstream errors(diagnostic_path, std::ios::binary);
    require(errors.good(), "Cannot read the completed isolated stage-scan stderr capture");
    const std::string stderr_bytes((std::istreambuf_iterator<char>(errors)), std::istreambuf_iterator<char>());
    const auto stderr_lines = std::count(stderr_bytes.begin(), stderr_bytes.end(), '\n');
    require(stderr_lines == 3 && scans == 6,
        ("Isolated startup/rescans expected 3 stderr lines and 6 scans, found " + std::to_string(stderr_lines) +
        " lines and " + std::to_string(scans) + " scans").c_str());
    records.close(); require(records.good() && comparisons == 156 && rows == 156 && frames == 10 && crops == 5,
        "Stage scan native evidence inventory changed");
    std::ofstream summary(output / "stage-scan-summary.json");
    summary << "{\"outcome\":\"passed\",\"updates\":" << updates << ",\"level_updates\":" << level_updates << ",\"readbacks\":" << reads << ",\"scans\":" << scans <<
        ",\"comparisons\":" << comparisons << ",\"rows\":" << rows << ",\"full_frames\":" << frames << ",\"native_crops\":" << crops <<
        ",\"sizes\":6,\"stderr_lines\":3,\"long_pages\":" << pages[0].size() << '}'; summary.close(); require(summary.good(), "Cannot flush stage-scan summary");
    std::cout << "PASS stage scan native: real good/bad isolated folder, Stage Select Down/details/dismiss/valid match start, "
        "latest changed/removed/fixed results, series/unattached/clean boundaries, six native sizes, independently escaped long/invalid-byte "
        "copy, exact text rasters, bounded pages and frozen draw RNG. Controlled input; author wording/readability remain separate.\n";
}
