// Opaque closing text48 shares the font/resource probe, but has distinct
// applicability and acceptance states. Alpha variants are refusals, not art.
void hud_closing_text_colour_consumer_checks(Application& app, const Presentation& presentation,
    const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    Presentation local = presentation; auto& selected = const_cast<HudPresentation&>(local.hud()).closing_clock.text;
    HudTimerColourProbe probe(app, local, output, HudTimerSubject::closing_text);
    const std::array<Colour, 4> colours{ Colour(20, 20, 20), Colour(255, 0, 0), Colour(53, 188, 225), Colour(32, 96, 192, 255) };
    constexpr std::array<const char*, 4> names{ "default", "red", "rgb", "rgba_ff" };
    std::array<Frame, 4> main{}; size_t rows = 0, crops = 0, controls = 0, control_pixels = 0;
    std::ofstream csv(output / "hud-closing-text-colour-main.csv");
    csv << "state,raw_rgba_match,adoption_match,repeat_match,locality_match,opaque_selected,controls,discriminating_pixels,frozen_style_resources_rng,scope\n";
    for (size_t state = 0; state < colours.size(); ++state)
    {
        selected = colours[state]; require(selected.a == 1.0f, "Closing shared palette must stay opaque");
        main[state] = probe.observe(2, selected, true); // Actual running10, not normal123.
        try { HudTimerColourProbe::locality(main[state], main[0], state != 0); }
        catch (...) { probe.preserve_failure(2, "opaque-locality", main[state], main[0]); throw; }
        size_t case_controls = 0, case_pixels = 0;
        if (state == 0)
        {
            case_pixels += probe.control(2, selected, HudTimerReference::wrong_opaque); ++case_controls;
            case_pixels += probe.control(2, selected, HudTimerReference::added_normal_shadow); ++case_controls;
        }
        csv << names[state] << ",true,true,true,true,true," << case_controls << ',' << case_pixels
            << ",true,opaque-closing-text-glyph-roi\n";
        save(HudTimerColourProbe::compact(main[state]), output / (std::string("hud-closing-text-colour-") + names[state] + ".ppm"));
        ++rows; ++crops; controls += case_controls; control_pixels += case_pixels;
    }
    std::array<std::array<Frame, 8>, 2> branch{};
    constexpr std::array<size_t, 7> contexts{ 0, 1, 3, 4, 5, 6, 7 };
    std::ofstream branches(output / "hud-closing-text-colour-branches.csv");
    branches << "state,context,closing_branch,raw_rgba_match,repeat_match,selection_guard,opaque_selected,frozen_style_resources_rng,scope\n";
    for (size_t state = 0; state < 2; ++state)
    {
        selected = colours[state == 0 ? 0 : 3];
        for (size_t context : contexts)
        {
            const auto& item = hud_timer_cases()[context];
            branch[state][context] = item.normal ? probe.unchanged_normal(context) : probe.observe(context, selected, false);
            if (state != 0)
            {
                try
                {
                    if (item.normal)
                    { require(branch[state][context].pixels == branch[0][context].pixels, "Closing-text edit changed an inapplicable normal frame"); }
                    else { HudTimerColourProbe::locality(branch[state][context], branch[0][context], true); }
                }
                catch (...) { probe.preserve_failure(context, "closing-selection", branch[state][context], branch[0][context]); throw; }
            }
            branches << (state == 0 ? "default" : "rgba_ff") << ',' << item.name << ',' << (item.normal ? "false" : "true") << ','
                << (item.normal ? "not-run-normal-invariance-only" : "true") << ",true,"
                << (state == 0 ? "baseline" : item.normal ? "normal-exact" : "closing-locality")
                << ",true,true,public-hud-closing-text-counterexample\n"; ++rows;
        }
        require(branch[state][0].pixels == branch[state][6].pixels, "Normal clock Reduce Effects relation changed");
        require(branch[state][3].pixels == branch[state][7].pixels, "Settled/reduced closing text frames differ");
        require(main[state == 0 ? 0 : 3].pixels != branch[state][7].pixels,
            "Closing text beat accents must still distinguish exact10 from reduced10");
    }
    require(csv.good() && branches.good() && probe.readbacks() == 50 && rows == 18 && crops == 4 && controls == 2 &&
        control_pixels > 0 && control_pixels <= 13624, "Closing shared finite read/row/crop/control budget differs");
    std::ofstream summary(output / "hud-closing-text-colour-consumer-summary.json");
    summary << "{\"scope\":\"opaque-closing-text-glyph-roi-and-normal-invariance\",\"main_context\":\"running-exact-ten\",\"readbacks\":50,\"csv_rows\":18,\"main_rows\":4,\"branch_rows\":14,"
        "\"crops\":4,\"crop_width\":131,\"crop_height\":52,\"crop_pixels\":27248,\"ppm_bytes\":81800,\"roi_pixels\":6812,"
        "\"independent_roi_observations\":8,\"roi_pixel_checks\":54496,\"repeat_checks\":18,\"copied_adoption_checks\":4,"
        "\"normal_colour_invariance_checks\":5,\"reduce_effects_checks\":6,\"controls\":2,\"control_discriminating_pixels\":" << control_pixels << ',' <<
        "\"calibration_pixel_checks\":1600,\"private_game_saves\":0,\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,"
        "\"scene_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,\"input_polls\":0,\"observation_weapon_updates\":0}\n";
    require(summary.good(), "Cannot retain closing-text shared summary");
    std::cout << "PASS opaque closing text: 50 reads, 18 rows, four native 131 by 52 crops; correct closing branch/no normal shadow, "
        "wrong opaque tint and added-shadow controls, exact normal/Reduce Effects guards. No alpha1 tint discriminators claimed.\n";
}

#if COLOURWARS_DEV_TOOLS
void hud_closing_text_tune_colour_checks(Application& app, Registry<LevelLoadedInfo>& levels, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& content, const std::filesystem::path& output)
{
    const auto root = output / "hud-closing-text-tune-scratch", source = root / "source", staged = root / "staged";
    require(!std::filesystem::exists(root), "Closing-text private scratch must be fresh");
    std::filesystem::create_directories(source); std::filesystem::create_directories(staged);
    constexpr std::array<const char*, 2> files{ "tuning.json", "presentation.json" }; std::array<std::string, 2> bytes;
    for (size_t i = 0; i < files.size(); ++i)
    {
        bytes[i] = tune_colour_bytes(content / files[i]);
        tune_colour_write(source / files[i], bytes[i]); tune_colour_write(staged / files[i], bytes[i]);
    }
    const NestedSaveProfile profile(root / "profile"); const TuneColourTemp temporary(root / "temp");
    struct RestoreClosing { GameTuning& t; Presentation& p; GameTuning old_t; Presentation old_p; ~RestoreClosing() { t = old_t; p = old_p; } }
        restore{ tuning, presentation, tuning, presentation };
    Save preferences; SettingsSession preferences_session(preferences);
    const auto preferences_now = [&]()
    {
        auto values = limb_preference_values(preferences_session.play_settings());
        values.insert(values.end(), { static_cast<int>(preferences.resolution()), preferences.fullscreen() ? 1 : 0,
            static_cast<int>(preferences.team_palette()), static_cast<int>(preferences_session.input_bindings().keyboard_jump) }); return values;
    };
    const auto original_preferences = preferences_now(); StageList stages;
    for (const char* name : { "movement_course", "close_quarters", "king_of_the_hill", "turbulence", "crossfeed" })
    {
        const auto* info = levels.get(name); require(info != nullptr, "Closing text requires shipped stage metadata");
        stages.push_back({ name, info->display_name(), info->icon_frame(), info->include_in_random() });
    }
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_settings_session(&preferences_session); const StageCatalog stage_catalog = acceptance::catalog_of(levels, stages, { "close_quarters", "crossfeed", "king_of_the_hill", "movement_course", "turbulence" }); data.set_stage_catalog(&stage_catalog);
    data.set_tuning(&tuning); data.set_presentation(&presentation); DebugDirector director(&tuning, &presentation); data.set_debug(&director);
    auto& impl = *director.impl(); impl.set_content_directories(source, staged); impl.set_user_directory(root / "user");
    impl.set_script_directories({ root / "scripts" }); director.attach(&app, &data); impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output / "hud-closing-text-tune-commands.log").string()), "Cannot retain closing-text transcript");
    HudTimerColourProbe probe(app, presentation, output, HudTimerSubject::closing_text);
    const std::string path = "hud.closing_clock.text";
    JsonSpanDocument initial(bytes[1], "Closing-text private source"); const auto* token = initial.find(path);
    require(token != nullptr, "Existing closing text token is absent");
    const std::string saved_bytes = initial.replaced(*token, "\"2060C0FF\""); bool persisted = false;
    size_t commands = 0, accepted = 0, refused = 0, rows = 0, refusal_rows = 0, normal_rows = 0;
    Frame baseline{}, saved{}, normal_before{};
    const auto unchanged = [&]()
    {
        probe.unchanged(); require(preferences_now() == original_preferences &&
            !std::filesystem::exists(root / "profile/ColourWars/save_data.json") && !impl.in_match() && !impl.can_apply() && impl.request().empty(),
            "Closing text changed private defaults, wrote a game Save or queued gameplay");
        require(impl.total_updates() == 0 && impl.level_updates() == 0, "Closing native commands advanced director update hooks");
        for (size_t i = 0; i < files.size(); ++i)
        {
            const auto& expected = i == 1 && persisted ? saved_bytes : bytes[i];
            require(tune_colour_bytes(content / files[i]) == bytes[i] && tune_colour_bytes(source / files[i]) == expected &&
                tune_colour_bytes(staged / files[i]) == expected, "Closing text changed private bytes beyond one exact saved token");
        }
    };
    const auto command = [&](const std::string& line, bool should_accept)
    {
        const size_t before = impl.log().lines().size(); std::srand(48019); const int next = std::rand(); std::srand(48019);
        const auto status = impl.execute(line); ++commands;
        require(status.ok == should_accept && std::rand() == next, "Closing text command acceptance/RNG differs");
        if (should_accept)
        {
            ++accepted;
            if (line.starts_with("tune "))
            {
                const auto& log = impl.log().lines(); require(log.size() > before && log.back().severity == DebugSeverity::info &&
                    log.back().text.starts_with("tune: presentation.hud.closing_clock.text ") && log.back().text.ends_with("(live)"),
                    "Closing-text command lacks a fresh live-adoption reply");
            }
        }
        else { ++refused; }
    };
    const auto document_state = [&]()
    {
        std::vector<std::string> state;
        for (const char* document_name : { "presentation", "tuning" })
        {
            const auto* document = impl.tune_document(document_name); require(document != nullptr, "Closing document disappeared");
            state.insert(state.end(), { document_name, document->baseline().text(), document->current().text(), std::to_string(document->revision()) });
            for (const auto& [key, edit] : document->edits())
            { state.insert(state.end(), { key, edit.baseline_token, edit.current_token }); }
        }
        for (const char* name : files)
        {
            state.push_back(tune_colour_bytes(content / name)); state.push_back(tune_colour_bytes(source / name));
            state.push_back(tune_colour_bytes(staged / name));
        }
        return state;
    };
    std::ofstream csv(output / "hud-closing-text-tune.csv");
    csv << "state,raw_rgba_match,adoption_match,repeat_match,private_bytes,opaque_selected,frozen_style_resources_rng,token,scope\n";
    const auto check = [&](const char* state, const Colour& colour, const char* word)
    {
        unchanged(); require(presentation.hud().closing_clock.text == colour && colour.a == 1.0f, "Closing accepted colour must be exact and opaque");
        const auto* document = impl.tune_document("presentation"); require(document != nullptr, "Closing document disappeared");
        const auto* leaf = document->current().find(path);
        require(leaf != nullptr && document->current().token(*leaf) == std::string("\"") + word + '"', "Closing exact quoted token differs");
        const Frame frame = probe.observe(2, colour, true);
        if (std::string_view(state) == "baseline") { baseline = frame; }
        if (std::string_view(state) == "reset-original") { require(frame.pixels == baseline.pixels, "Closing authored reset lost complete HUD RGBA"); }
        if (std::string_view(state) == "rgba-ff") { saved = frame; }
        if (std::string_view(state) == "reset-saved") { require(frame.pixels == saved.pixels, "Closing saved reset lost complete HUD RGBA"); }
        if (std::string_view(state) == "named")
        {
            require(document->edits().size() == 1 && document->edits().contains(path) &&
                document->edits().at(path).current_token == "\"RED\"", "Closing named RED token was not retained");
        }
        csv << state << ",true,true,true,true,true,true," << word << ",debug-opaque-closing-text\n"; ++rows; unchanged();
        return frame;
    };
    std::ofstream refusal_csv(output / "hud-closing-text-tune-refusals.csv");
    refusal_csv << "input,accepted,atomic_preserved,raster_unchanged,opaque_selected,current_token,frozen_style_resources_rng,scope\n";
    const auto refuse = [&](const char* input)
    {
        unchanged(); const auto* document = impl.tune_document("presentation");
        const auto held = document_state(); const Colour held_colour = presentation.hud().closing_clock.text;
        command(std::string("tune presentation.") + path + ' ' + input, false);
        require(impl.tune_document("presentation") == document && document_state() == held &&
            presentation.hud().closing_clock.text == held_colour && held_colour.a == 1.0f,
            "Partial/zero closing edit changed live colour, full ledger/tokens, baseline/current or source/staged bytes");
        require(probe.actual(2).pixels == saved.pixels, "Refused closing edit changed actual applicable pixels");
        refusal_csv << input << ",false,true,true,true,2060C0FF,true,debug-opaque-refusal\n"; ++refusal_rows; unchanged();
    };
    std::ofstream normal_csv(output / "hud-closing-text-tune-normal.csv");
    normal_csv << "state,context,normal_unchanged,frozen_style_resources_rng,scope\n";
    check("baseline", Colour(20, 20, 20), "141414"); normal_before = probe.actual(0);
    normal_csv << "baseline,normal,baseline,true,debug-normal-invariance\n"; ++normal_rows;
    command("tune presentation.hud.closing_clock.text RED", true); check("named", Colour(255, 0, 0), "RED");
    command("tune.reset presentation.hud.closing_clock.text", true); check("reset-original", Colour(20, 20, 20), "141414");
    command("tune presentation.hud.closing_clock.text 35BCE1", true); check("rgb", Colour(53, 188, 225), "35BCE1");
    command("tune presentation.hud.closing_clock.text 2060C0FF", true); check("rgba-ff", Colour(32, 96, 192, 255), "2060C0FF");
    require(probe.actual(0).pixels == normal_before.pixels, "Actual closing edit changed the normal timer branch");
    normal_csv << "rgba-ff,normal,true,true,debug-normal-invariance\n"; ++normal_rows;
    refuse("2060C080"); refuse("A532D600");
    command("tune.save presentation", true); persisted = true; unchanged();
    require(impl.tune_document("presentation")->edits().empty(), "Closing one-token Save did not rebase");
    command("tune presentation.hud.closing_clock.text E17C35", true); check("after-save-rgb", Colour(225, 124, 53), "E17C35");
    command("tune.reset presentation.hud.closing_clock.text", true); check("reset-saved", Colour(32, 96, 192, 255), "2060C0FF");
    require(csv.good() && refusal_csv.good() && normal_csv.good() && commands == 9 && accepted == 7 && refused == 2 &&
        rows == 7 && refusal_rows == 2 && normal_rows == 2 && probe.readbacks() == 32 &&
        impl.tune_document("presentation")->edits().empty() && impl.tune_document("tuning")->edits().empty(),
        "Closing Debug finite accepted/refused/state/readback budget or final ledger differs");
    std::ofstream summary(output / "hud-closing-text-tune-summary.json");
    summary << "{\"scope\":\"debug-opaque-closing-text-and-refusal\",\"commands\":9,\"accepted_commands\":7,\"refused_commands\":2,\"states\":7,"
        "\"readbacks\":32,\"csv_rows\":11,\"accepted_rows\":7,\"refusal_rows\":2,\"normal_rows\":2,\"crops\":0,\"saved_tokens\":1,"
        "\"presentation_save_commands\":1,\"private_game_saves\":0,\"roi_pixel_checks\":43554,\"repeat_checks\":7,\"copied_adoption_checks\":7,"
        "\"normal_colour_invariance_checks\":1,\"refusal_pixel_checks\":2,\"calibration_pixel_checks\":1024,\"local_updates\":0,\"player_updates\":0,"
        "\"level_updates\":0,\"scene_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,\"input_polls\":0,\"observation_weapon_updates\":0}\n";
    require(summary.good(), "Cannot retain closing-text Debug summary");
    std::cout << "PASS opaque closing tune: 9 real statements (7 accepted, 2 atomic alpha refusals), 7 rendered accepted states, "
        "32 additional reads, 11 rows and one-token Save/rebase/reset. No game Saves, updates or input polls.\n";
}
#endif
