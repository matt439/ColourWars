// Included in tune_checks.cpp's private namespace. Normal timer foreground
// and shadow share focused checks; the independent closing signal stays fixed.
void check_timer_colour_tunes_for_field(const std::filesystem::path& game_directory, std::size_t index)
{
    const std::string path = colour_paths[index];
    const std::string local = path.substr(std::string("presentation.").size());
    const std::size_t sibling_index = index == hud_timer_colour_index ? hud_timer_shadow_colour_index : hud_timer_colour_index;
    const std::string sibling = colour_paths[sibling_index];
    const std::string sibling_local = sibling.substr(std::string("presentation.").size());
    {
        const std::string baseline = "{\r\n\"hud\":{\"timer\":{\"colour\":\"\\u003141414\","
            "\"shadow_colour\":\"\\u00300000022\",\"font\":\"captured\"}},\r\n\"number\":1\r\n}\r\n";
        const std::string authored_token = index == hud_timer_colour_index ? "\"\\u003141414\"" : "\"\\u00300000022\"";
        const JsonSpanDocument reference(baseline, "normal-timer-document-reference");
        TuneDocument document("presentation", baseline);
        const auto* leaf = document.find(local);
        require(leaf != nullptr && leaf->kind == JsonLeaf::Kind::string &&
            document.current().token(*leaf) == authored_token &&
            document.find("hud.timer.font") == nullptr && document.find(sibling_local) != nullptr,
            "Each exact normal timer colour is editable with its independently supported sibling; font stays unavailable");
        document.commit(document.edited(*leaf, "\"2060C080\""), local);
        const std::string pending = document.current().text();const auto edit = document.edits().at(local);
        require(document.edits().size() == 1 && edit.baseline_token == authored_token &&
            edit.current_token == "\"2060C080\"" && document.reverted(local) == baseline,
            "Normal timer edits preserve escaped RGB spelling, both ledger tokens and exact CRLF reset bytes");
        for (const char* token : { "false", "null", "17", "{}", "[]" })
        {
            bool rejected = false;
            try { document.commit(document.edited(*document.find(local), token), local); }
            catch (const std::invalid_argument&) { rejected = true; }
            require(rejected && document.current().text() == pending && document.baseline().text() == baseline &&
                document.edits().size() == 1 && document.edits().at(local).baseline_token == edit.baseline_token &&
                document.edits().at(local).current_token == edit.current_token,
                "Wrong-type timer commits preserve all current/baseline bytes and both ledger tokens");
        }
        const std::string external = reference.replaced(*reference.find("number"), "25");
        const JsonSpanDocument external_reference(external, "normal-timer-external-reference");
        const TuneSplice merged = document.splice(external, false);
        require(merged.conflicts.empty() && merged.written == 1 && merged.text ==
            external_reference.replaced(*external_reference.find(local), "\"2060C080\""),
            "The nested timer splice preserves unrelated external bytes, shadow/font strings and line endings");
        for (const std::string& unavailable : { std::string("{}"),
            reference.replaced(*reference.find(local), "null"),
            reference.replaced(*reference.find(local), "{\"value\":\"141414\"}"),
            std::string("{\"hud.timer.colour\":\"141414\"}"),
            std::string("{\"hud\":{\"timer\":{\"colour.r\":\"141414\"}}}") })
        {
            const TuneDocument unsupported("presentation", unavailable);
            const TuneSplice refused = document.splice(unavailable, true);
            require(unsupported.find(local) == nullptr && refused.written == 0 &&
                refused.conflicts.size() == 1 && refused.text == unavailable,
                "Force cannot insert a timer colour, replace null/a container or follow dotted aliases");
        }
    }

    // Generic 47-path sweeps cover strict named/RGB/RGBA input, LF/CRLF
    // conflicts/force, atomic replacement and actual Presentation consumption.
    ScratchContent scratch(game_directory, index == hud_timer_colour_index ? L"normal-timer-colour-records" : L"normal-timer-shadow-colour-records");
    GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
    Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
    TestDirector test(tuning, presentation);auto& impl = test.impl;
    impl.set_content_directories(scratch.source, scratch.staged);
    const HudPresentation* const look = &presentation.hud();
    const PlayerIdentityPresentation* const identity = &presentation.player_identity();
    const HudPresentation original = *look;
    const auto initial = colour_values(presentation);auto expected = initial;
    const std::string baseline = read_text(scratch.source / L"presentation.json");
    const std::string tuning_source = read_text(scratch.source / L"tuning.json");
    const std::string tuning_staged = read_text(scratch.staged / L"tuning.json");
    labrador::RenderResources resources;
    std::vector<std::string> sheets{ original.sprite_sheet };
    if (original.sprite_sheet != "sprite_sheet_1") { sheets.push_back("sprite_sheet_1"); }
    for (const auto& name : sheets)
        { resources.add_sprite_sheet(name, labrador::read_sprite_sheet((game_directory / L"textures" / (name + ".json")).string().c_str(), labrador::TextureHandle{})); }
    const std::array<std::string, 3> fonts{ original.meter_label_font, original.timer_font, original.player_identity_font };
    for (const auto& name : fonts)
    {
        const auto file = labrador::read_sprite_font_file((game_directory / L"fonts" / (name + ".spritefont")).string());
        auto loaded = std::make_unique<labrador::Font>(labrador::TextureHandle{}, file.glyphs, file.line_spacing);
        if (loaded->contains(U'?')) { loaded->set_stand_in(U'?'); }
        resources.add_font(name, std::move(loaded));
    }
    const InterfaceGameplay hud(&resources, look, identity);
    const InterfaceGameplay* const retained_hud = &hud;
    const auto* const sheet = resources.sprite_sheet(original.sprite_sheet);
    const auto fill = sheet->resolve_sprite_frame(original.fill_frame);
    const auto fill_source = sheet->sprite_frame(fill).source_rectangle();
    const auto fill_origin = sheet->sprite_frame(fill).origin();
    const auto font_handles = [&]
    {
        std::vector<labrador::FontHandle> handles;
        for (const auto& font : fonts) { handles.push_back(resources.resolve_sprite_font(font)); }
        return handles;
    };
    const auto font_measurements = [&]
    {
        return std::vector<mattmath::Vector2F>{ resources.measure_text(resources.resolve_sprite_font(fonts[0]), L"HEALTH"),
            resources.measure_text(resources.resolve_sprite_font(fonts[1]), L"2:04"),
            resources.measure_text(resources.resolve_sprite_font(fonts[2]), L"P1  /  TEAM A\nKeyboard + mouse") };
    };
    const auto geometry = [&]
    {
        return std::vector<float>{ look->paper.keyline_width, look->paper.shadow_offset,
            look->meter_label_gap, look->reference_height, look->right_margin, look->ammo_top_margin,
            look->health_top_margin, look->box_size.x, look->box_size.y, look->box_fill_border,
            look->timer_offset.x, look->timer_offset.y, look->timer_shadow_offset.x, look->timer_shadow_offset.y,
            look->timer_reference_width, look->closing_clock.keyline_width, look->closing_clock.shadow_offset,
            look->closing_clock.tick_reach, look->closing_clock.beat_duration, look->player_identity_offset.x,
            look->player_identity_offset.y, look->player_identity_shadow_offset.x, look->player_identity_shadow_offset.y,
            look->player_identity_padding.x, look->player_identity_padding.y, look->player_identity_reference_width,
            identity->enabled ? 1.0f : 0.0f, identity->world_size.x, identity->world_size.y,
            identity->world_offset.x, identity->world_offset.y, identity->emphasis_duration,
            identity->emphasis_scale, identity->ui_size.x, identity->ui_size.y, identity->ui_gap, identity->border };
    };
    const auto unselected = [&]
    {
        return std::vector<labrador::Colour>{ look->paper.ink, look->paper.paper, look->paper.shade,
            look->health_colour, look->meter_track_colour, look->closing_clock.ink,
            look->closing_clock.paper, look->closing_clock.shade, look->closing_clock.text, look->closing_clock.accent,
            look->player_identity_colour, look->player_identity_shadow_colour, look->player_identity_background_colour,
            identity->ink_colour, identity->background_colour };
    };
    const auto names = [&]
    {
        return std::array<std::string, 5>{ look->sprite_sheet, look->fill_frame,
            look->meter_label_font, look->timer_font, look->player_identity_font };
    };
    const auto held_geometry = geometry();const auto held_unselected = unselected();const auto held_names = names();
    const auto held_fonts = font_handles();const auto held_measurements = font_measurements();
    const auto* const document = impl.tune_document("presentation");
    require(document != nullptr && original.timer_colour == labrador::Colour(20, 20, 20) &&
        original.timer_shadow_colour == labrador::Colour(0, 0, 0, 34) &&
        held_measurements[0].x > 0.0f && held_measurements[1].x > 0.0f && held_measurements[2].x > 0.0f,
        "The retained CPU HUD loads authored141414/00000022 and actual label/timer/identity font metadata");
    const auto unchanged = [&]
    {
        require(retained_hud == &hud && &presentation.hud() == look && &presentation.player_identity() == identity &&
            geometry() == held_geometry && unselected() == held_unselected && names() == held_names &&
            resources.sprite_sheet(look->sprite_sheet) == sheet && sheet->resolve_sprite_frame(look->fill_frame) == fill &&
            sheet->sprite_frame(fill).source_rectangle() == fill_source && sheet->sprite_frame(fill).origin() == fill_origin &&
            font_handles() == held_fonts && font_measurements() == held_measurements && colour_values(presentation) == expected &&
            read_text(scratch.source / L"tuning.json") == tuning_source && read_text(scratch.staged / L"tuning.json") == tuning_staged &&
            !impl.in_match() && !impl.can_apply() && impl.request().empty(),
            "Timer commands retain the real HUD, borrowed HUD/identity records, unselected shadow/closing/material/geometry and captured resource metadata without match work");
    };
    // No draw or private HUD internals are observed by this CPU fixture.
    // Actual normal/closing branch pixels and alpha are native-owned checks.
    std::srand(0x54494d45);const int next_random = std::rand();std::srand(0x54494d45);
    for (const auto& [word, colour] : std::array<std::pair<const char*, labrador::Colour>, 4>{ {
        { "RED", { 255, 0, 0 } }, { "a1B2c3", { 161, 178, 195 } },
        { "2060C080", { 32, 96, 192, 128 } }, { "A532D600", { 165, 50, 214, 0 } } } })
    {
        expected[index] = colour;
        require(impl.execute("tune " + path + ' ' + word).ok && impl.tune_token(path) == std::string("\"") + word + '"' &&
            impl.execute("tune " + path).ok && log_contains(impl.log(), path + " = " + impl.tune_token(path), DebugSeverity::info),
            "The retained normal timer accepts named, RGB, partial-alpha and nonzero-RGB alpha-zero input live");unchanged();
    }
    expected[index] = initial[index];
    require(impl.execute("tune.reset " + path).ok && document->edits().empty() && document->current().text() == baseline &&
        read_text(scratch.source / L"presentation.json") == baseline && read_text(scratch.staged / L"presentation.json") == baseline,
        "Normal timer reset restores exact baseline bytes without writing either content copy");unchanged();
    expected[index] = labrador::Colour("2060C080");
    require(impl.execute("tune " + path + " 2060C080").ok && impl.execute("tune.diff presentation").ok &&
        log_contains(impl.log(), path, DebugSeverity::info), "The real command diff includes the nested timer edit");unchanged();
    const std::string external = " \r\n" + baseline;
    const JsonSpanDocument external_reference(external, "normal-timer-save-reference");
    const std::string saved = external_reference.replaced(*external_reference.find(local), "\"2060C080\"");
    write_text(scratch.source / L"presentation.json", external);
    require(impl.execute("tune.save presentation").ok && read_text(scratch.source / L"presentation.json") == saved &&
        read_text(scratch.staged / L"presentation.json") == saved && document->baseline().text() == saved &&
        document->current().text() == saved && document->edits().empty() &&
        !std::filesystem::exists(scratch.source / L"presentation.json.tmp") && !std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
        "A timer-only Save changes one token, preserves external whitespace/sibling/font bytes and rebases both copies atomically");unchanged();

    expected[index] = labrador::Colour("A532D600");
    require(impl.execute("tune " + path + " A532D600").ok, "The saved timer baseline can be edited again");unchanged();
    expected[sibling_index] = labrador::Colour("4C83B580");
    require(impl.execute("tune " + sibling + " 4C83B580").ok && document->edits().size() == 2,
        "The selected timer colour and independent foreground/shadow sibling retain separate pending edits");unchanged();
    expected[3] = labrador::Colour("789ABC");
    require(impl.execute("tune presentation.debug_overlay.colour 789ABC").ok && document->edits().size() == 3,
        "Both normal timer colours and unrelated overlay have three independent pending ledger entries");unchanged();
    expected[index] = labrador::Colour("2060C080");
    require(impl.execute("tune.reset " + path).ok && document->edits().size() == 2 &&
        !document->edits().contains(local) && document->edits().contains(sibling_local) && document->edits().contains("debug_overlay.colour"),
        "Resetting only the selected timer colour retains its sibling and unrelated pending edit and uses its saved partial-alpha baseline");unchanged();
    expected[index] = labrador::Colour("A532D600");
    require(impl.execute("tune " + path + " A532D600").ok && document->edits().size() == 3,
        "The timer edit remains independent after selected reset");unchanged();
    const auto ledger = [&]
    {
        std::vector<std::string> values;
        for (const auto& [key, edit] : document->edits())
            { values.push_back(key + '=' + edit.baseline_token + "->" + edit.current_token); }
        return values;
    };
    const auto held_ledger = ledger();const std::string current = document->current().text();
    const JsonSpanDocument saved_reference(saved, "normal-timer-invalid-merge-reference");
    const std::string invalid = saved_reference.replaced(*saved_reference.find("hud.paper.keyline_width"), "4.1");
    write_text(scratch.source / L"presentation.json", invalid);
    for (const char* command : { "tune.save presentation", "tune.save presentation force" })
    {
        require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == invalid &&
            read_text(scratch.staged / L"presentation.json") == saved && document->current().text() == current &&
            document->baseline().text() == saved && ledger() == held_ledger && document->edits().size() == 3,
            "Ordinary and forced full-reader saves reject invalid external HUD paper without changing source/staging/live values or any of three ledger token pairs");unchanged();
    }
    expected[index] = labrador::Colour("2060C080");
    require(impl.execute("tune.reset " + path).ok && document->edits().size() == 2 &&
        document->edits().contains(sibling_local) && document->edits().contains("debug_overlay.colour") && read_text(scratch.source / L"presentation.json") == invalid,
        "Selected timer reset retains its sibling, unrelated pending edit and invalid external source bytes");unchanged();
    expected = initial;expected[index] = labrador::Colour("2060C080");
    require(impl.execute("tune.reset presentation").ok && document->edits().empty() &&
        document->current().text() == saved && document->baseline().text() == saved &&
        read_text(scratch.staged / L"presentation.json") == saved && read_text(scratch.source / L"presentation.json") == invalid,
        "Resetting the remaining edit restores the saved timer baseline without writing external content");unchanged();
    require(std::rand() == next_random, "Normal timer tuning consumed gameplay RNG");
}

void check_timer_colour_tunes(const std::filesystem::path& game_directory)
{
    for (const std::size_t index : { hud_timer_colour_index, hud_timer_shadow_colour_index })
        { check_timer_colour_tunes_for_field(game_directory, index); }
}
