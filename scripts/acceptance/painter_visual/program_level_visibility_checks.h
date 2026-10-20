// Separate composed-Level witness. Program34's direct Monitor alpha proof and
// counters stay unchanged; this leaf observes one declared opaque story frame.
void program_level_visibility_checks(Application& app, Registry<LevelLoadedInfo>& levels,
    const GameTuning& tuning, const Presentation& presentation, const PlaySettings& preferences,
    const std::filesystem::path& output)
{
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    Presentation local = presentation;
    auto& look = const_cast<OpeningPresentation&>(local.opening());
    const OpeningPresentation original = look;
    require(look.program_colour == Colour(44, 44, 44) && look.glass_colour == Colour(22, 22, 22) &&
        tuning.match().opening.settle == .8f && preferences.music_volume == 0 && preferences.sfx_volume == 0,
        "Program Level witness lost authored opaque defaults, settle or quiet fixture");
    // This builder must borrow local, rather than the outer builder's different
    // Presentation. Level dies before both its builder and its borrowed data.
    LevelBuilder local_builder(app.viewport_manager(), app.render_resources(), app.audio_resources(),
        &preferences, &levels, &tuning, &local, app.resolution_manager(), app.thread_pool(),
        app.partitioner(), app.mouse());
    auto level = countdown_level(app, local_builder, 2, OpeningMode::full, "crossfeed");
    const float setup_elapsed = tuning.match().opening.settle * .5f;
    require(setup_elapsed == .4f, "Program Level setup checkpoint is no longer .4f");
    level->update(neutral_inputs(), setup_elapsed); // Exactly one positive setup update.
    require(level->state() == LevelState::opening && level->player_count() == 2 &&
        level->player(0).player_num() == input_slots::keyboard_and_mouse && level->player(1).player_num() == 3 &&
        level->player(0).team() == PlayerTeam::a && level->player(1).team() == PlayerTeam::b,
        "Program Level fixture lost actual two-player Crossfeed opening roster");

    const auto before = observation(*level), fixed = program_fixed_style(look);
    const int collisions = level->collision_object_count(), projectiles = level->projectile_count();
    const std::array<const Player*, 2> players{ &level->player(0), &level->player(1) };
    const auto* opening_address = &look; const auto* art_address = &look.monitor_art;
    const auto* sheet = app.render_resources()->sprite_sheet(look.sprite_sheet);
    const auto* frame = &sheet->sprite_frame(sheet->resolve_sprite_frame(look.fill_frame));
    const auto* palette_sheet = app.render_resources()->sprite_sheet(look.palette_sheet);
    const auto preference_values = [&]
    {
        std::vector<int> v{ preferences.aim_sensitivity, preferences.music_volume, preferences.sfx_volume,
            preferences.view_shake ? 1 : 0, preferences.aim_marker ? 1 : 0, preferences.reduce_effects ? 1 : 0 };
        for (const auto& stick : preferences.sticks) { v.push_back(stick.movement_deadzone); v.push_back(stick.aim_deadzone); }
        return v;
    };
    const auto saved_preferences = preference_values();
#if COLOURWARS_DEV_TOOLS
    const MatchFingerprint fingerprint = MatchFingerprint::of(*level, 1);
#endif
    const auto unchanged = [&]
    {
        require(level->state() == LevelState::opening && level->player_count() == 2 &&
            &level->player(0) == players[0] && &level->player(1) == players[1] &&
            level->player(0).player_num() == input_slots::keyboard_and_mouse && level->player(1).player_num() == 3 &&
            level->player(0).team() == PlayerTeam::a && level->player(1).team() == PlayerTeam::b &&
            level->collision_object_count() == collisions && level->projectile_count() == projectiles &&
            observation(*level) == before && preference_values() == saved_preferences,
            "Program Level drawing changed public world, roster or borrowed preferences");
        require(&local.opening() == opening_address && &local.opening().monitor_art == art_address &&
            program_fixed_style(look) == fixed && look.sprite_sheet == original.sprite_sheet &&
            look.fill_frame == original.fill_frame && look.palette_sheet == original.palette_sheet &&
            look.palette_frames == original.palette_frames && look.percent_font == original.percent_font &&
            app.render_resources()->sprite_sheet(look.sprite_sheet) == sheet &&
            &sheet->sprite_frame(sheet->resolve_sprite_frame(look.fill_frame)) == frame &&
            app.render_resources()->sprite_sheet(look.palette_sheet) == palette_sheet,
            "Program Level drawing/edit changed borrowed style addresses or captured resource dependencies");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(*level, 1)).empty(),
            "Program Level drawing changed public replay fingerprint");
#endif
    };
    const auto patch = [](const Frame& actual, const RectangleI& area, std::uint32_t rgb)
    {
        for (int y = area.y; y < area.y + area.height; ++y)
        { for (int x = area.x; x < area.x + area.width; ++x)
            { require(monitor_rgb(actual, x, y) == rgb, "Program Level declared patch is covered or has wrong literal RGB"); } }
    };
    constexpr std::array<const char*, 3> names{ "baseline", "rgb", "restored" };
    const std::array<Colour, 3> colours{ original.program_colour, Colour(53, 188, 225), original.program_colour };
    constexpr std::array<std::uint32_t, 3> expected{ 0x2C2C2Cu, 0x35BCE1u, 0x2C2C2Cu };
    std::array<Frame, 3> frames{}; std::array<size_t, 3> deltas{};
    size_t readbacks = 0, state = 0;
    // One sentinel spans ALL draws, selected edits, restoration and snapshots.
    // Setup includes a real persona roll and a positive update before this bracket.
    std::srand(31721); const int next_random = std::rand(); std::srand(31721);
    try
    {
        for (; state < names.size(); ++state)
        {
            look.program_colour = colours[state]; unchanged();
            frames[state] = render(*level, *app.renderer()); ++readbacks;
            // Retain the declared frame before any pixel guard can fail. A failure
            // is evidence to inspect, never permission to move the sample silently.
            if (state < 2) { save(frames[state], output / (std::string("program-level-") + names[state] + ".ppm")); }
            const Frame repeat = render(*level, *app.renderer()); ++readbacks;
            if (frames[state].pixels != repeat.pixels)
            { save(repeat, output / "program-level-failure-repeat.ppm"); }
            require(frames[state].width == 1280 && frames[state].height == 720 && frames[state].pixels == repeat.pixels,
                "Program Level completed composition changed on frozen repeat");
            patch(frames[state], { 632, 92, 16, 16 }, expected[state]);
            patch(frames[state], { 392, 242, 16, 16 }, 0xC9D3D9u);
            patch(frames[state], { 132, 100, 1, 1 }, 0x161616u);
            for (const RectangleI area : { RectangleI(392, 242, 16, 16), RectangleI(132, 100, 1, 1) })
            {
                for (int y = area.y; y < area.y + area.height; ++y)
                { for (int x = area.x; x < area.x + area.width; ++x)
                    { const size_t at = (static_cast<size_t>(y) * 1280 + static_cast<size_t>(x)) * 4;
                      require(same_pixel(frames[state], frames[0], at), "Program Level paper/glass control changed RGBA bytes"); } }
            }
            unchanged();
            ProgramColourProbe::delta_only_mask(frames[state], frames[0]);
            for (size_t at = 0; at < frames[state].pixels.size(); at += 4)
            { if (!same_pixel(frames[state], frames[0], at)) { ++deltas[state]; } }
            if (state == 1) { require(deltas[state] >= 256, "Program Level opaque edit had no complete exposed-patch delta"); }
            else { require(frames[state].pixels == frames[0].pixels, "Program Level restoration did not recover whole baseline raster"); }
        }
        unchanged(); require(std::rand() == next_random, "Program Level observation window consumed gameplay RNG");
        require(readbacks == 6, "Program Level readback budget changed");
    }
    catch (...)
    {
        if (state < frames.size() && !frames[state].pixels.empty())
        { save(frames[state], output / "program-level-failure-actual.ppm"); }
        std::ofstream failure(output / "program-level-failure.json");
        failure << "{\"scope\":\"actual-level-single-opening-checkpoint\",\"state_index\":" << state <<
            ",\"readbacks\":" << readbacks << ",\"diagnostic\":\"see-native-stderr-and-retained-frame\"}\n";
        throw;
    }

    std::ofstream cases(output / "program-level-cases.csv"), samples(output / "program-level-samples.csv");
    cases << "state,readbacks,repeat_equal,baseline_equal,delta_pixels,outside_mask_delta_pixels,frozen_public_style,rng_scope,scope\n";
    samples << "state,site,x,y,width,height,expected_rgb,observed_rgb,all_pixels_equal,baseline_unchanged,scope\n";
    size_t case_rows = 0, sample_rows = 0;
    for (size_t index = 0; index < names.size(); ++index)
    {
        cases << names[index] << ",2,true," << (index == 1 ? "false" : "true") << ',' << deltas[index] <<
            ",0,true,single-six-draw-bracket,actual-level-single-opening-checkpoint\n"; ++case_rows;
        const std::array<RectangleI, 3> areas{ RectangleI(632, 92, 16, 16), RectangleI(392, 242, 16, 16), RectangleI(132, 100, 1, 1) };
        constexpr std::array<const char*, 3> sites{ "exposed-program", "covered-paper", "outside-program-glass" };
        const std::array<std::uint32_t, 3> rgb{ expected[index], 0xC9D3D9u, 0x161616u };
        for (size_t site = 0; site < sites.size(); ++site)
        {
            const auto& area = areas[site];
            samples << names[index] << ',' << sites[site] << ',' << area.x << ',' << area.y << ',' << area.width << ',' << area.height << ',' <<
                rgb[site] << ',' << monitor_rgb(frames[index], area.x, area.y) << ",true," <<
                (index == 1 && site == 0 ? "false" : "true") << ",actual-level-single-opening-checkpoint\n"; ++sample_rows;
        }
    }
    require(cases.good() && samples.good() && case_rows == 3 && sample_rows == 9, "Program Level CSV budget changed");
    std::ofstream summary(output / "program-level-summary.json");
    summary << "{\"scope\":\"actual-level-single-opening-checkpoint\",\"stage\":\"crossfeed\",\"player_count\":2,\"input_slots\":[4,3],"
        "\"opening\":\"full\",\"state\":\"opening\",\"viewport\":\"1280x720\",\"builder_seed\":9411,\"level_builds\":1,"
        "\"setup_level_updates\":1,\"setup_elapsed_seconds\":0.4,\"construction_admission\":\"source-derived-end-tick-not-instrumented\","
        "\"observation_level_updates\":0,\"observation_player_updates\":0,\"observation_scene_updates\":0,\"observation_state_updates\":0,"
        "\"observation_input_updates\":0,\"observation_time_updates\":0,\"readbacks\":6,\"case_rows\":3,\"sample_rows\":9,\"csv_rows\":12,"
        "\"full_ppms\":2,\"crops\":0,\"commands\":0,\"rng_brackets\":1,\"repeat_pairs\":3,\"whole_frame_reset\":true,"
        "\"frozen_public_style_resources\":true,\"private_story_clocks_observed\":false,\"program_mask\":[136,59,1007,480],"
        "\"program_patch\":[632,92,16,16],\"paper_patch\":[392,242,16,16],\"glass_patch\":[132,100,1,1],\"event_fade\":false}\n";
    require(summary.good(), "Cannot write Program Level summary");
    std::cout << "PASS program-level-visibility: one actual 2P Crossfeed full opening, one positive .4f setup update; "
        "6readbacks/3cases+9samples/2fullPPMs/0crops, zero observation updates, one RNG bracket. "
        "Literal exposed program and covered paper/glass guards, whole-frame repeats/restoration, mask-only delta. "
        "One declared composed checkpoint only; private story clocks are source-derived, no human/performance/all-phase claim.\n";
}
