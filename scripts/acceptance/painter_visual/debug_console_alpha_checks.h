// Debug-only source draft. Included inside painter_visual's anonymous namespace,
// after its ordinary renderer helpers and tune_colour_checks.h. No input events,
// state updates, active match, wall clock, Save writes or production tint helper.
#if COLOURWARS_DEV_TOOLS
void debug_console_alpha_checks(Application& app, Registry<LevelLoadedInfo>& levels,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& content,
    const std::filesystem::path& output)
{
    const auto root = output / "alpha-console-scratch";
    require(!std::filesystem::exists(root), "Console alpha scratch directory must be fresh");
    const auto source = root / "source", staged = root / "staged";
    std::filesystem::create_directories(source); std::filesystem::create_directories(staged);
    const std::array<const char*, 2> files{ "tuning.json", "presentation.json" };
    std::array<std::string, 2> real_bytes;
    for (size_t file_index = 0; file_index < files.size(); ++file_index)
    {
        real_bytes[file_index] = tune_colour_bytes(content / files[file_index]);
        tune_colour_write(source / files[file_index], real_bytes[file_index]);
        tune_colour_write(staged / files[file_index], real_bytes[file_index]);
    }
    const NestedSaveProfile profile(root / "profile"); const TuneColourTemp temporary(root / "temp");
    const GameTuning original_tuning = tuning; const Presentation original = presentation;
    struct RestoreConsoleAlphaContent
    {
        GameTuning& tuning; Presentation& presentation; GameTuning original_tuning; Presentation original;
        ~RestoreConsoleAlphaContent() { this->tuning = this->original_tuning; this->presentation = this->original; }
    } restore{ tuning, presentation, original_tuning, original };
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);

    // Seed bytes directly: loading an existing file never invokes Save's writer.
    // Non-default preferences make accidental memory changes observable too.
    const auto save_path = root / "profile/ColourWars/save_data.json";
    std::filesystem::create_directories(save_path.parent_path());
    const std::string saved_preferences = "{\"save_version\":2,\"resolution\":{\"x\":1280,\"y\":720},"
        "\"fullscreen\":false,\"team_palette\":\"blue_orange\",\"aim_sensitivity\":4,"
        "\"music_volume\":37,\"sfx_volume\":61,\"view_shake\":false,\"aim_marker\":true,"
        "\"reduce_effects\":false,\"pad_sticks\":[{\"movement_deadzone\":17,\"aim_deadzone\":29}]}\r\n";
    tune_colour_write(save_path, saved_preferences);
    Save preferences; preferences.load_save_file();
    require(preferences.play_settings().music_volume == 37 && preferences.play_settings().sfx_volume == 61 &&
        preferences.play_settings().sticks[0].movement_deadzone == 17,
        "Console alpha private preference fixture was not loaded");
    const auto preference_values = [&]()
    {
        const PlaySettings& play = preferences.play_settings();
        std::vector<int> values{ static_cast<int>(preferences.resolution()), preferences.fullscreen(),
            static_cast<int>(preferences.team_palette()), play.aim_sensitivity, play.music_volume,
            play.sfx_volume, play.view_shake, play.aim_marker, play.reduce_effects };
        for (const StickSettings& stick : play.sticks)
        { values.push_back(stick.movement_deadzone); values.push_back(stick.aim_deadzone); }
        return values;
    };
    const auto initial_preferences = preference_values();
    StageList stages;
    for (const char* name : { "movement_course", "close_quarters", "king_of_the_hill", "turbulence" })
    {
        const auto* info = levels.get(name);
        require(info != nullptr, "Console alpha fixture requires the shipped stage catalog");
        stages.push_back({ name, info->display_name(), info->icon_frame(), info->include_in_random() });
    }
    GameData data; data.set_application(&app); data.set_save(&preferences);
    const StageCatalog stage_catalog = acceptance::catalog_of(levels, stages, { "close_quarters", "king_of_the_hill", "movement_course", "turbulence" }); data.set_stage_catalog(&stage_catalog); data.set_tuning(&tuning); data.set_presentation(&presentation);
    DebugDirector director(&tuning, &presentation); data.set_debug(&director);
    DebugDirectorImpl& impl = *director.impl();
    impl.set_content_directories(source, staged); impl.set_user_directory(root / "user");
    impl.set_script_directories({ root / "scripts" }); director.attach(&app, &data);
    impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output / "alpha-console-commands.log").string()),
        "Cannot retain console alpha command transcript");

    const DebugOverlayPresentation geometry = original.debug_overlay();
    const DebugOverlayPresentation* const borrowed_look = &presentation.debug_overlay();
    require(geometry.colour == Colour(211, 211, 211) && geometry.shadow_colour == Colour(0, 0, 0) &&
        geometry.line_spacing == 18.0f && geometry.font_scale == 1.0f && geometry.shadow_scale == 1.0f &&
        geometry.shadow_offset == Vector2F(1.0f, 1.0f),
        "Console alpha opaque-default fixture no longer matches the reviewed presentation");
    RenderResources* resources = app.render_resources();
    const FontHandle font_handle = resources->resolve_sprite_font(geometry.font);
    const Font* const captured_font = resources->font(font_handle);
    const Glyph* const letter_m = captured_font->find(U'M');
    require(letter_m != nullptr && letter_m->x_offset >= 0.0f,
        "Console alpha caret fixture needs its actual monospaced M glyph");
    const float caret_advance = std::max(1.0f, (letter_m->x_offset +
        static_cast<float>(letter_m->subrect.width) + letter_m->x_advance) * geometry.font_scale);
    require(caret_advance == (resources->measure_text(font_handle, L"MM").x -
        resources->measure_text(font_handle, L"M").x) * geometry.font_scale,
        "Console alpha independent caret advance differs from the captured font");
    const SpriteSheet* const sheet = resources->sprite_sheet("sprite_sheet_1");
    const SpriteFrame& pixel = sheet->sprite_frame(sheet->resolve_sprite_frame("pixel"));
    require(pixel.origin() == Vector2F::ZERO, "Console alpha raw pixel fixture needs zero source origin");
    DebugConsoleView frozen(false), live(true); frozen.init(data); live.init(data);
    DebugPerf perf; perf.set_counts(17, 3); perf.begin_draw(0.0); perf.end_draw(0.5);
    perf.add_update(0.5); perf.begin_draw(16.0); perf.end_draw(18.0);
    perf.add_update(0.5); perf.begin_draw(40.0); perf.end_draw(42.0);
    const std::vector<std::string> perf_lines = perf.lines();
    require(perf_lines.size() == 8 && perf.size() == 2 && perf.at(0).frame_ms == 24.0 &&
        perf.at(1).frame_ms == 16.0, "Console alpha deterministic perf fixture is not the intended two frames");
    const std::vector<std::string> paint_lines{ "paint  A 12.5%  B 25.0%  free 62.5%" };
    const std::vector<std::string> status_lines{ "PAUSED ALPHA" };
    const std::array<Colour, 2> backgrounds{ Colour(0, 0, 0), Colour(39, 73, 101) };
    const std::array<const char*, 2> background_names{ "black", "blue" };
    const std::array<const char*, 4> consumer_names{ "frozen", "live", "panels", "perf" };
    size_t readbacks = 0, rows = 0, commands = 0, crops = 0, comparisons = 0, resets = 0, repeats = 0;
    std::array<Frame, 8> baseline{};
    std::ofstream records(output / "alpha-console.csv");
    records << "case,consumer,background,pending_edits,oracle_equal,repeat_checked\n";

    const auto unchanged = [&]()
    {
        const DebugOverlayPresentation& current = presentation.debug_overlay();
        require(&current == borrowed_look && current.font == geometry.font &&
            current.font_scale == geometry.font_scale && current.line_spacing == geometry.line_spacing &&
            current.position == geometry.position && current.shadow_offset == geometry.shadow_offset &&
            current.shadow_scale == geometry.shadow_scale && resources->font(font_handle) == captured_font &&
            resources->resolve_sprite_font(current.font) == font_handle,
            "Console alpha command changed captured font resources or overlay geometry");
        require(preference_values() == initial_preferences && tune_colour_bytes(save_path) == saved_preferences,
            "Console alpha command or drawing changed private save bytes or live preferences");
        for (size_t file_index = 0; file_index < files.size(); ++file_index)
        {
            require(tune_colour_bytes(content / files[file_index]) == real_bytes[file_index] &&
                tune_colour_bytes(source / files[file_index]) == real_bytes[file_index] &&
                tune_colour_bytes(staged / files[file_index]) == real_bytes[file_index],
                "Console alpha tune/reset wrote real, source or staged content");
        }
        require(impl.level_updates() == 0 && impl.total_updates() == 0 && !impl.in_match() && !impl.can_apply(),
            "Console alpha probe performed gameplay work or queued a match");
        require(perf.size() == 2 && perf.lines() == perf_lines && perf.objects() == 17 && perf.projectiles() == 3,
            "Console alpha drawing changed synthetic performance data");
        for (size_t age = 0; age < 2; ++age)
        {
            const DebugPerf::Frame& sample = perf.at(age);
            require(sample.frame_ms == (age == 0 ? 24.0 : 16.0) && sample.update_ms == 0.5 &&
                sample.draw_ms == 2.0 && sample.updates == 1 && sample.objects == 17 && sample.projectiles == 3,
                "Console alpha drawing changed a fixed performance sample");
        }
    };
    const auto ui_state = [&]()
    {
        std::string state = impl.field().text() + "|" + std::to_string(impl.field().caret()) + "|" + impl.status_strip();
        for (const DebugLogLine& line : impl.log().lines())
        { state += "\n" + std::to_string(static_cast<int>(line.severity)) + ":" + line.text; }
        return state;
    };
    const auto fixture_log = [&]()
    {
        impl.log().clear(); impl.log().info("INFO ALPHA"); impl.log().write(DebugSeverity::echo, "ECHO FIXED");
        impl.log().warning("WARN FIXED"); impl.log().error("ERROR FIXED"); impl.field().set_text("ALPHA INPUT");
    };
    const auto command = [&](const std::string& line, const std::string& success_prefix)
    {
        impl.log().write(DebugSeverity::echo, "] " + line);
        const size_t first_new = impl.log().lines().size();
        const DebugStatus result = impl.execute(line); ++commands;
        require(result.ok, "Real console alpha tune/reset command was rejected");
        bool fresh_success = false;
        for (size_t log_index = first_new; log_index < impl.log().lines().size(); ++log_index)
        {
            const DebugLogLine& entry = impl.log().lines()[log_index];
            fresh_success = fresh_success || (entry.severity == DebugSeverity::info &&
                entry.text.find(success_prefix) == 0 && entry.text.find("(live)") != std::string::npos);
        }
        require(fresh_success, "Console alpha command has no fresh scoped live-success info line");
        unchanged();
    };
    const auto actual = [&](size_t consumer, size_t background)
    {
        unchanged(); const std::string before = ui_state();
        std::srand(9287); const int next_random = std::rand(); std::srand(9287);
        const Canvas canvas(resources, backgrounds[background]); Renderer& renderer = *app.renderer();
        renderer.begin_frame(); renderer.set_view_count(1); DrawList list = renderer.view(0);
        list.set_viewport(app.viewport_manager()->fullscreen_viewport()); list.set_camera(Camera::DEFAULT_CAMERA);
        list.set_filter(TextureFilter::point); canvas.draw(list, { 0, 0, 1280, 720 });
        if (consumer == 0) { frozen.draw(renderer, data, impl); }
        else if (consumer == 1) { live.draw(renderer, data, impl); }
        else if (consumer == 2)
        {
            const RectangleF first = DebugConsoleView::draw_panel(renderer, data, paint_lines,
                DebugConsoleView::Anchor::bottom_left);
            const RectangleF second = DebugConsoleView::draw_panel(renderer, data, status_lines,
                DebugConsoleView::Anchor::bottom_right, { first });
            require(!first.intersects(second), "Console alpha corner fixtures unexpectedly overlap");
        }
        else { std::ignore = DebugConsoleView::draw_perf(renderer, data, perf, {}); }
        require(++readbacks <= 104, "Console alpha exceeded its hard readback budget");
        Frame frame = read(renderer);
        require(ui_state() == before && std::rand() == next_random,
            "Console alpha drawing changed frozen UI state or consumed match randomness");
        unchanged(); return frame;
    };

    // These references submit literal developer geometry and explicit straight
    // byte fractions converted here, bypassing all four production drawing seams
    // and CrudeDrawing::premultiplied. Raw unexposed panel/graph tint is retained.
    const auto reference = [&](size_t consumer, size_t background, const Colour& foreground,
        const Colour& shadow, bool pending)
    {
        const Canvas canvas(resources, backgrounds[background]); Renderer& renderer = *app.renderer();
        renderer.begin_frame(); renderer.set_view_count(1); DrawList list = renderer.view(0);
        list.set_viewport(app.viewport_manager()->fullscreen_viewport()); list.set_camera(Camera::DEFAULT_CAMERA);
        list.set_filter(TextureFilter::point); canvas.draw(list, { 0, 0, 1280, 720 });
        const auto rectangle = [&](const RectangleF& box, const Colour& tint)
        { list.draw_sprite(sheet->texture(), pixel.source_rectangle(), box, tint,
            0.0f, Vector2F::ZERO, SpriteFlip::none, 0.0f); };
        const auto premultiplied = [](const Colour& tint)
        { return Colour(tint.r * tint.a, tint.g * tint.a, tint.b * tint.a, tint.a); };
        const auto text = [&](const std::string& value, const Vector2F& position, const Colour& tint)
        {
            const std::wstring wide(value.begin(), value.end());
            list.draw_text(font_handle, wide, position + geometry.shadow_offset, premultiplied(shadow),
                geometry.shadow_scale, 0.0f, Vector2F::ZERO, 0.0f);
            list.draw_text(font_handle, wide, position, premultiplied(tint),
                geometry.font_scale, 0.0f, Vector2F::ZERO, 0.0f);
        };
        const auto width = [&](const std::string& value)
        { return resources->measure_text(font_handle, std::wstring(value.begin(), value.end())).x * geometry.font_scale; };
        const Colour ordinary_panel(0.02f, 0.03f, 0.05f, 0.80f), strip(1.0f, 0.85f, 0.35f, 1.0f);
        const float line_height = geometry.line_spacing;
        if (consumer < 2)
        {
            const float panel_height = consumer == 0 ? 324.0f : 216.0f;
            rectangle({ 0.0f, 0.0f, 1280.0f, panel_height }, consumer == 0 ? ordinary_panel :
                Colour(0.02f, 0.03f, 0.05f, 0.85f));
            rectangle({ 0.0f, panel_height - 2.0f, 1280.0f, 2.0f }, original.menu().highlight);
            list.set_filter(TextureFilter::linear);
            const std::string prompt = consumer == 0 ? "] " : "live] ";
            const float input_y = panel_height - 12.0f - line_height;
            require(width(prompt + "ALPHA INPUT") < 1200.0f, "Console alpha fixture unexpectedly wraps input");
            text(prompt + "ALPHA INPUT", { 10.0f, input_y }, foreground);
            text("_", { 10.0f + caret_advance * static_cast<float>(prompt.size() + 11), input_y + 2.0f },
                Colour(1.0f, 1.0f, 1.0f, 1.0f));
            if (pending) { text("TUNED 1", { 1270.0f - width("TUNED 1"), 10.0f }, strip); }
            text("error: ERROR FIXED", { 10.0f, input_y - line_height }, Colour(1.0f, 0.45f, 0.40f, 1.0f));
            text("warning: WARN FIXED", { 10.0f, input_y - line_height * 2.0f }, strip);
            text("ECHO FIXED", { 10.0f, input_y - line_height * 3.0f }, Colour(1.0f, 1.0f, 1.0f, 1.0f));
            text("INFO ALPHA", { 10.0f, input_y - line_height * 4.0f }, foreground);
        }
        else if (consumer == 2)
        {
            // The two short one-line backings are disjoint; no avoidance branch
            // or production placement helper participates in the reference.
            const float paint_width = width(paint_lines[0]), status_width = width(status_lines[0]);
            require(paint_width + status_width + 40.0f < 1280.0f, "Console alpha panel fixture unexpectedly stacks");
            for (size_t panel_index = 0; panel_index < 2; ++panel_index)
            {
                const float panel_width = panel_index == 0 ? paint_width : status_width;
                const Vector2F origin(panel_index == 0 ? 10.0f : 1270.0f - panel_width, 710.0f - line_height);
                list.set_filter(TextureFilter::point);
                rectangle({ origin.x - 6.0f, origin.y - 3.0f, panel_width + 12.0f, line_height + 6.0f }, ordinary_panel);
                list.set_filter(TextureFilter::linear);
                text(panel_index == 0 ? paint_lines[0] : status_lines[0], origin, strip);
            }
        }
        else
        {
            float panel_width = 240.0f;
            for (const std::string& value : perf_lines) { panel_width = std::max(panel_width, width(value)); }
            const float panel_height = 46.0f + line_height * 8.0f;
            const Vector2F origin(10.0f, 710.0f - panel_height);
            rectangle({ origin.x - 6.0f, origin.y - 3.0f, panel_width + 12.0f, panel_height + 6.0f }, ordinary_panel);
            rectangle({ origin.x, origin.y, 240.0f, 40.0f }, Colour(0.10f, 0.13f, 0.17f, 0.95f));
            const float slow_height = static_cast<float>(24.0 / 33.3) * 40.0f;
            const float good_height = static_cast<float>(16.0 / 33.3) * 40.0f;
            rectangle({ origin.x + 238.0f, origin.y + 40.0f - slow_height, 1.0f, slow_height },
                Colour(1.0f, 0.80f, 0.30f, 0.95f));
            rectangle({ origin.x + 236.0f, origin.y + 40.0f - good_height, 1.0f, good_height },
                Colour(0.40f, 0.85f, 0.45f, 0.95f));
            const float reference_y = origin.y + 40.0f - static_cast<float>((1000.0 / 60.0) / 33.3) * 40.0f;
            rectangle({ origin.x, reference_y, 240.0f, 1.0f }, Colour(1.0f, 1.0f, 1.0f, 0.85f));
            list.set_filter(TextureFilter::linear);
            for (size_t line_index = 0; line_index < perf_lines.size(); ++line_index)
            { text(perf_lines[line_index], origin + Vector2F(0.0f, 46.0f + line_height * static_cast<float>(line_index)), strip); }
        }
        list.set_filter(TextureFilter::point);
        require(++readbacks <= 104, "Console alpha reference exceeded its hard readback budget");
        return read(renderer);
    };
    const auto check = [&](const char* case_name, const Colour& foreground, const Colour& shadow,
        bool pending, bool repeated, bool is_baseline, bool is_reset)
    {
        require(presentation.debug_overlay().colour == foreground && presentation.debug_overlay().shadow_colour == shadow,
            "Console alpha validated command did not adopt the independently specified straight RGBA channels");
        const TuneDocument* document = impl.tune_document("presentation");
        require(document != nullptr && document->edits().size() == (pending ? 1U : 0U) &&
            impl.status_strip() == (pending ? "TUNED 1" : ""), "Console alpha edit ledger/status is unexpected");
        fixture_log();
        for (size_t consumer = 0; consumer < consumer_names.size(); ++consumer)
        {
            for (size_t background = 0; background < backgrounds.size(); ++background)
            {
                const size_t index = consumer * 2 + background;
                const Frame frame = actual(consumer, background);
                if (is_reset)
                {
                    require(frame.pixels == baseline[index].pixels, "Console alpha reset did not restore opaque-default pixels");
                    ++resets;
                }
                else
                {
                    require(frame.pixels == reference(consumer, background, foreground, shadow, pending).pixels,
                        "Console alpha consumer differs from explicit premultiplied font/developer-geometry reference");
                    ++comparisons;
                }
                if (is_baseline) { baseline[index] = frame; }
                if (repeated)
                {
                    require(frame.pixels == actual(consumer, background).pixels,
                        "Console alpha frozen UI/perf drawing changed between repeated calls"); ++repeats;
                }
                if (std::string(case_name).find("foreground-") == 0 && consumer >= 2)
                { require(frame.pixels == baseline[index].pixels, "Console alpha foreground edit changed fixed panel/perf colours"); }
                if (background == 1 && std::string(case_name) == "foreground-partial" && consumer < 2)
                {
                    save(foreground_crop(frame, consumer == 0 ? RectangleI(0, 160, 640, 160) : RectangleI(0, 52, 640, 160), 0),
                        output / (consumer == 0 ? "alpha-console-frozen-partial.ppm" : "alpha-console-live-partial.ppm")); ++crops;
                }
                if (background == 1 && std::string(case_name) == "shadow-partial" && consumer >= 2)
                {
                    save(foreground_crop(frame, consumer == 2 ? RectangleI(0, 624, 1280, 96) : RectangleI(0, 480, 640, 240), 0),
                        output / (consumer == 2 ? "alpha-console-panels-shadow-partial.ppm" : "alpha-console-perf-shadow-partial.ppm")); ++crops;
                }
                records << case_name << ',' << consumer_names[consumer] << ',' << background_names[background]
                    << ',' << (pending ? 1 : 0) << ",true," << (repeated ? "true" : "false") << '\n'; ++rows;
            }
        }
    };
    const Colour default_foreground(211, 211, 211), default_shadow(0, 0, 0);
    check("baseline", default_foreground, default_shadow, false, true, true, false);
    const auto foreground_path = std::string("presentation.debug_overlay.colour");
    const auto shadow_path = std::string("presentation.debug_overlay.shadow_colour");
    command("tune " + foreground_path + " 2060C080", "tune: " + foreground_path + " ");
    check("foreground-partial", Colour(32, 96, 192, 128), default_shadow, true, true, false, false);
    command("tune.reset " + foreground_path, "tune.reset: " + foreground_path + " ");
    command("tune " + foreground_path + " A040E000", "tune: " + foreground_path + " ");
    check("foreground-zero", Colour(160, 64, 224, 0), default_shadow, true, false, false, false);
    command("tune.reset " + foreground_path, "tune.reset: " + foreground_path + " ");
    command("tune " + shadow_path + " C0408080", "tune: " + shadow_path + " ");
    check("shadow-partial", default_foreground, Colour(192, 64, 128, 128), true, false, false, false);
    command("tune.reset " + shadow_path, "tune.reset: " + shadow_path + " ");
    command("tune " + shadow_path + " 30D09000", "tune: " + shadow_path + " ");
    check("shadow-zero", default_foreground, Colour(48, 208, 144, 0), true, false, false, false);
    command("tune.reset " + shadow_path, "tune.reset: " + shadow_path + " ");
    check("reset", default_foreground, default_shadow, false, false, false, true);
    unchanged();
    require(readbacks == 104 && comparisons == 40 && resets == 8 && repeats == 16 && rows == 48 && commands == 8 && crops == 4,
        "Console alpha fixture did not meet its exact readback/command/row/crop budget");
    require(records.good(), "Cannot retain console alpha CSV");
    std::ofstream summary(output / "alpha-console-summary.json");
    summary << "{\"outcome\":\"passed\",\"readbacks\":104,\"oracle_comparisons\":40,\"reset_comparisons\":8,"
        "\"frozen_repeats\":16,\"csv_rows\":48,\"commands\":8,\"crops\":4,\"gameplay_updates\":0,"
        "\"save_constructions\":1,\"save_write_commands\":0,\"backgrounds\":2,\"consumers\":4}\n";
    require(summary.good(), "Cannot retain console alpha summary");
}
#endif
