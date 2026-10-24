// Included in tune_checks.cpp's private namespace. Clock and HUD label/material
// colours share retained-HUD checks; closing text/ink/paper/accent require alpha1; shade requires positive alpha.
void check_timer_colour_tunes_for_field(const std::filesystem::path& game_directory, std::size_t index)
{
    const bool closing_material = index == closing_clock_ink_colour_index ||
        index == closing_clock_paper_colour_index || index == closing_clock_accent_colour_index;
    const bool shared_paper = hud_paper_colour_index(index);
    const bool closing_shade = index == closing_clock_shade_colour_index;
    const bool opaque = opaque_closing_colour_index(index) || opaque_hud_paper_colour_index(index);
    const bool positive_alpha = positive_alpha_colour_index(index);
    const bool complete_resources = closing_material || positive_alpha || shared_paper;
    const std::string sibling_word = (closing_material || positive_alpha || shared_paper) ? "4C83B5FF" : "4C83B580";
    const bool label = index == hud_label_colour_index || index == hud_identity_shadow_colour_index;
    const bool shadow = index == hud_timer_shadow_colour_index || index == hud_identity_shadow_colour_index;
    const std::string saved_word = opaque ? "2060C0FF" : "2060C080";
    const std::string pending_word = opaque ? "A532D6FF" : positive_alpha ? "A532D601" : "A532D600";
    const std::string saved_token = '"' + saved_word + '"';
    const std::string path = colour_paths[index];
    const std::string local = path.substr(std::string("presentation.").size());
    const std::size_t sibling_index = shared_paper ?
        (index == hud_paper_paper_colour_index ? hud_paper_ink_colour_index : hud_paper_paper_colour_index) :
        closing_shade ? closing_clock_paper_colour_index : closing_material ?
        (index == closing_clock_ink_colour_index ? closing_clock_paper_colour_index :
            index == closing_clock_paper_colour_index ? closing_clock_accent_colour_index : closing_clock_ink_colour_index) : label ?
        (index == hud_label_colour_index ? hud_identity_shadow_colour_index : hud_label_colour_index) :
        (index == hud_timer_colour_index ? hud_timer_shadow_colour_index : hud_timer_colour_index);
    const std::string sibling = colour_paths[sibling_index];
    const std::string sibling_local = sibling.substr(std::string("presentation.").size());
    {
        const std::string baseline = std::string("{\r\n\"hud\":{\"timer\":{\"colour\":\"\\u003141414\","
            "\"shadow_colour\":\"\\u00300000022\",\"font\":\"captured\"},") +
            (shared_paper ? "\"paper\":{\"ink\":\"\\u003141414\",\"paper\":\"\\u0046FF5DF\",\"shade\":\"\\u00300000055\"}," : "") +
            "\"closing_clock\":{\"text\":\"\\u003141414\",\"ink\":\"\\u003141414\",\"paper\":\"\\u0046FF5DF\",\"accent\":\"\\u0046FD34D\",\"shade\":\"\\u00300000055\"},"
            "\"player_identity\":{\"colour\":\"\\u003141414\",\"shadow_colour\":\"\\u00300000022\",\"font\":\"captured\",\"background_colour\":\"FFFFFF\"}},"
            "\r\n\"number\":1\r\n}\r\n";
        const std::string authored_token = positive_alpha ? "\"\\u00300000055\"" : shadow ? "\"\\u00300000022\"" :
            (index == closing_clock_paper_colour_index || index == hud_paper_paper_colour_index) ? "\"\\u0046FF5DF\"" :
            index == closing_clock_accent_colour_index ? "\"\\u0046FD34D\"" : "\"\\u003141414\"";
        const JsonSpanDocument reference(baseline, "normal-timer-document-reference");
        TuneDocument document("presentation", baseline);
        const auto* leaf = document.find(local);
        require(leaf != nullptr && leaf->kind == JsonLeaf::Kind::string &&
            document.current().token(*leaf) == authored_token &&
            document.find("hud.timer.font") == nullptr && document.find("hud.player_identity.font") == nullptr &&
            document.find("hud.player_identity.background_colour") != nullptr && document.find(sibling_local) != nullptr,
            "Each exact HUD clock/label colour is editable with its independently supported sibling; fonts stay unavailable and the separately captured preview backing is eligible");
        document.commit(document.edited(*leaf, saved_token), local);
        const std::string pending = document.current().text();const auto edit = document.edits().at(local);
        require(document.edits().size() == 1 && edit.baseline_token == authored_token &&
            edit.current_token == saved_token && document.reverted(local) == baseline,
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
            external_reference.replaced(*external_reference.find(local), saved_token),
            "The nested timer splice preserves unrelated external bytes, normal/closing sibling and font strings and line endings");
        for (const std::string& unavailable : { std::string("{}"),
            reference.replaced(*reference.find(local), "null"),
            reference.replaced(*reference.find(local), "{\"value\":\"141414\"}"),
            std::string("{\"") + local + "\":\"141414\"}",
            std::string("{\"hud\":{\"") + local.substr(4) + ".r\":\"141414\"}}" })
        {
            const TuneDocument unsupported("presentation", unavailable);
            const TuneSplice refused = document.splice(unavailable, true);
            require(unsupported.find(local) == nullptr && refused.written == 0 &&
                refused.conflicts.size() == 1 && refused.text == unavailable,
                "Force cannot insert a timer colour, replace null/a container or follow dotted aliases");
        }
    }

    // Generic 58-path sweeps and dedicated pair-aware LF/CRLF controls cover strict named/RGB/RGBA input, LF/CRLF
    // conflicts/force, atomic replacement and actual Presentation consumption.
    ScratchContent scratch(game_directory, shared_paper ?
        (index == hud_paper_ink_colour_index ? L"hud-paper-ink-records" :
            index == hud_paper_paper_colour_index ? L"hud-paper-paper-records" : L"hud-paper-shade-records") :
        closing_shade ? L"closing-clock-shade-records" : label ?
        (shadow ? L"hud-identity-shadow-records" : L"hud-label-foreground-records") :
        closing_material ? (index == closing_clock_ink_colour_index ? L"closing-clock-ink-records" :
            index == closing_clock_paper_colour_index ? L"closing-clock-paper-records" : L"closing-clock-accent-records") :
        opaque ? L"closing-clock-text-records" :
        index == hud_timer_colour_index ? L"normal-timer-colour-records" : L"normal-timer-shadow-colour-records");
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
    labrador::RenderResources resources; labrador::AudioResources complete_audio;
    std::vector<std::string> sheets{ original.sprite_sheet };
    if (original.sprite_sheet != "sprite_sheet_1") { sheets.push_back("sprite_sheet_1"); }
    if (complete_resources)
    {
        for (const char* name : { "sprite_sheet_1", "painter", "menu_paint", "weapon_paint", "paint_fx", "terrain" })
            { if (std::find(sheets.begin(), sheets.end(), name) == sheets.end()) { sheets.emplace_back(name); } }
    }
    for (const auto& name : sheets)
        { resources.add_sprite_sheet(name, labrador::read_sprite_sheet((game_directory / L"textures" / (name + ".json")).string().c_str(), labrador::TextureHandle{})); }
    const std::array<std::string, 3> fonts{ original.meter_label_font, original.timer_font, original.player_identity_font };
    std::vector<std::string> loaded_fonts(fonts.begin(), fonts.end());
    if (complete_resources)
    {
        for (const char* name : { "gill_sans_ultra_bold_144", "gill_sans_mt_bold_24", "gill_sans_mt_bold_36",
            "gill_sans_mt_bold_48", "gill_sans_mt_bold_72", "gill_sans_mt_bold_144", "courier_new_bold_16" })
            { if (std::find(loaded_fonts.begin(), loaded_fonts.end(), name) == loaded_fonts.end()) { loaded_fonts.emplace_back(name); } }
        complete_audio.add_sound_bank("paint_exe", labrador::SoundBank::silent());
    }
    for (const auto& name : loaded_fonts)
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
            resources.measure_text(resources.resolve_sprite_font(fonts[0]), L"PAINT"),
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
        std::vector<labrador::Colour> values{ look->health_colour, look->meter_track_colour,
            look->player_identity_background_colour, identity->ink_colour, identity->background_colour };
        if (!shared_paper)
            { values.insert(values.begin(), { look->paper.ink, look->paper.paper, look->paper.shade }); }
        return values;
    };
    const auto names = [&]
    {
        return std::array<std::string, 5>{ look->sprite_sheet, look->fill_frame,
            look->meter_label_font, look->timer_font, look->player_identity_font };
    };
    const auto held_geometry = geometry();const auto held_unselected = unselected();const auto held_names = names();
    const auto held_fonts = font_handles();const auto held_measurements = font_measurements();
    const auto* const document = impl.tune_document("presentation");
    const auto ledger = [&]
    {
        std::vector<std::string> values;
        for (const auto& [key, edit] : document->edits())
            { values.push_back(key + '=' + edit.baseline_token + "->" + edit.current_token); }
        return values;
    };
    require(document != nullptr && original.timer_colour == labrador::Colour(20, 20, 20) &&
        original.timer_shadow_colour == labrador::Colour(0, 0, 0, 34) &&
        original.closing_clock.text == labrador::Colour(20, 20, 20) && original.closing_clock.text.a == 1.0f &&
        original.closing_clock.ink == labrador::Colour(20, 20, 20) &&
        original.closing_clock.paper == labrador::Colour(255, 245, 223) &&
        original.closing_clock.accent == labrador::Colour(255, 211, 77) &&
        original.closing_clock.shade == labrador::Colour(0, 0, 0, 85) &&
        original.paper.ink == labrador::Colour(20, 20, 20) &&
        original.paper.paper == labrador::Colour(255, 245, 223) &&
        original.paper.shade == labrador::Colour(0, 0, 0, 85) &&
        original.player_identity_colour == labrador::Colour(20, 20, 20) &&
        original.player_identity_shadow_colour == labrador::Colour(0, 0, 0, 34) &&
        held_measurements[0].x > 0.0f && held_measurements[1].x > 0.0f &&
        held_measurements[2].x > 0.0f && held_measurements[3].x > 0.0f,
        "The retained CPU HUD loads authored141414/00000022 and actual HEALTH/PAINT/timer/identity font metadata");
    const auto unchanged = [&]
    {
        require(retained_hud == &hud && &presentation.hud() == look && &presentation.player_identity() == identity &&
            geometry() == held_geometry && unselected() == held_unselected && names() == held_names &&
            resources.sprite_sheet(look->sprite_sheet) == sheet && sheet->resolve_sprite_frame(look->fill_frame) == fill &&
            sheet->sprite_frame(fill).source_rectangle() == fill_source && sheet->sprite_frame(fill).origin() == fill_origin &&
            font_handles() == held_fonts && font_measurements() == held_measurements && colour_values(presentation) == expected &&
            read_text(scratch.source / L"tuning.json") == tuning_source && read_text(scratch.staged / L"tuning.json") == tuning_staged &&
            !impl.in_match() && !impl.can_apply() && impl.request().empty(),
            "Timer commands retain the real HUD, borrowed HUD/identity records, unselected clock/identity materials and root badge colours/geometry and captured resource metadata without match work");
        // No Application is attached: full-reader commands and separate direct
        // complete-resource checks remain distinct evidence, with no native claim.
        if (complete_resources) { presentation.validate_resources(resources, complete_audio); }
    };
    if (complete_resources)
    {
        // Resource checks use real loaded metadata independently of TestDirector.
        presentation.validate_resources(resources, complete_audio);
        const JsonSpanDocument resource_reference(baseline, "closing-material-resource-reference");
        const auto invalid_path = scratch.root / L"closing-material-missing-resource.json";
        write_text(invalid_path, resource_reference.replaced(*resource_reference.find("menu_style.fill_frame"), "\"__missing_closing_frame__\""));
        const Presentation invalid_resource(invalid_path.string().c_str());
        bool refused_resource = false;
        try { invalid_resource.validate_resources(resources, complete_audio); }
        catch (const std::exception& error)
            { refused_resource = std::string(error.what()).find("menu_style.fill_frame") != std::string::npos; }
        require(refused_resource && document->current().text() == baseline && document->baseline().text() == baseline &&
            document->edits().empty() && read_text(scratch.source / L"presentation.json") == baseline &&
            read_text(scratch.staged / L"presentation.json") == baseline,
            "Separate full-resource validation refuses a missing frame without altering live command documents or files"); unchanged();
    }
    // No draw or private HUD internals are observed by this CPU fixture.
    // Actual normal/closing/identity/HEALTH/PAINT pixels and alpha are native-owned checks.
    std::srand(0x54494d45);const int next_random = std::rand();std::srand(0x54494d45);
    const std::array<std::pair<const char*, labrador::Colour>, 4> accepted{ {
        { "RED", { 255, 0, 0 } }, { "a1B2c3", { 161, 178, 195 } },
        { opaque ? "2060C0FF" : "2060C080", { 32, 96, 192, opaque ? 255 : 128 } },
        { "A532D600", { 165, 50, 214, 0 } } } };
    for (std::size_t accepted_index = 0; accepted_index < ((opaque || positive_alpha) ? 3u : accepted.size()); ++accepted_index)
    {
        const auto& [word, colour] = accepted[accepted_index];
        expected[index] = colour;
        require(impl.execute("tune " + path + ' ' + word).ok && impl.tune_token(path) == std::string("\"") + word + '"' &&
            impl.execute("tune " + path).ok && log_contains(impl.log(), path + " = " + impl.tune_token(path), DebugSeverity::info),
            "The retained HUD accepts named/RGB/RGBA within the selected clock/label field opacity contract");unchanged();
    }
    if (positive_alpha)
    {
        for (const auto& [word, colour] : std::array<std::pair<const char*, labrador::Colour>, 4>{ {
            { "2060C001", { 32, 96, 192, 1 } }, { "2060C0FE", { 32, 96, 192, 254 } },
            { "2060C0FF", { 32, 96, 192 } }, { "141414", { 20, 20, 20 } } } })
        {
            expected[index] = colour;
            require(impl.execute("tune " + path + ' ' + word).ok &&
                impl.tune_token(path) == std::string("\"") + word + '"',
                "Positive HUD shade accepts AA01/FE/FF and may equal ink without a contrast restriction"); unchanged();
        }
    }
    if (opaque || positive_alpha)
    {
        const std::string held_current = document->current().text();
        const std::string held_baseline = document->baseline().text();
        const auto held_edits = ledger();
        const std::array<const char*, 2> refused_words{ positive_alpha ? "00000000" : "2060C080", "A532D600" };
        for (const char* word : refused_words)
        {
            require(!impl.execute("tune " + path + ' ' + word).ok &&
                document->current().text() == held_current && document->baseline().text() == held_baseline &&
                ledger() == held_edits && read_text(scratch.source / L"presentation.json") == baseline &&
                read_text(scratch.staged / L"presentation.json") == baseline,
                "Reader-invalid selected closing alpha is refused atomically with live colours, exact documents, both ledger tokens and both files intact");unchanged();
        }
    }
    if (closing_material)
    {
        const char* const equal_sibling_word = index == closing_clock_ink_colour_index ? "FFF5DF" :
            index == closing_clock_paper_colour_index ? "FFD34D" : "141414";
        expected[index] = initial[sibling_index];
        require(impl.execute("tune " + path + ' ' + equal_sibling_word).ok,
            "Opaque closing materials may equal their sibling; no new distinctness rule is introduced"); unchanged();
    }
    expected[index] = initial[index];
    require(impl.execute("tune.reset " + path).ok && document->edits().empty() && document->current().text() == baseline &&
        read_text(scratch.source / L"presentation.json") == baseline && read_text(scratch.staged / L"presentation.json") == baseline,
        "Normal timer reset restores exact baseline bytes without writing either content copy");unchanged();
    expected[index] = labrador::Colour(saved_word);
    require(impl.execute("tune " + path + ' ' + saved_word).ok && impl.execute("tune.diff presentation").ok &&
        log_contains(impl.log(), path, DebugSeverity::info), "The real command diff includes the nested timer edit");unchanged();
    const std::string external = " \r\n" + baseline;
    const JsonSpanDocument external_reference(external, "normal-timer-save-reference");
    const std::string saved = external_reference.replaced(*external_reference.find(local), saved_token);
    write_text(scratch.source / L"presentation.json", external);
    require(impl.execute("tune.save presentation").ok && read_text(scratch.source / L"presentation.json") == saved &&
        read_text(scratch.staged / L"presentation.json") == saved && document->baseline().text() == saved &&
        document->current().text() == saved && document->edits().empty() &&
        !std::filesystem::exists(scratch.source / L"presentation.json.tmp") && !std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
        "A timer-only Save changes one token, preserves external whitespace/sibling/font bytes and rebases both copies atomically");unchanged();

    expected[index] = labrador::Colour(pending_word);
    require(impl.execute("tune " + path + ' ' + pending_word).ok, "The saved timer baseline can be edited again");unchanged();
    expected[sibling_index] = labrador::Colour(sibling_word);
    require(impl.execute("tune " + sibling + " " + sibling_word).ok && document->edits().size() == 2,
        "The selected HUD colour and independent selected sibling retain separate pending edits");unchanged();
    expected[3] = labrador::Colour("789ABC");
    require(impl.execute("tune presentation.debug_overlay.colour 789ABC").ok && document->edits().size() == 3,
        "Selected HUD colour, its independent sibling and unrelated overlay have three pending ledger entries");unchanged();
    expected[index] = labrador::Colour(saved_word);
    require(impl.execute("tune.reset " + path).ok && document->edits().size() == 2 &&
        !document->edits().contains(local) && document->edits().contains(sibling_local) && document->edits().contains("debug_overlay.colour"),
        "Resetting only the selected timer colour retains its sibling and unrelated pending edit and uses its saved field-valid baseline");unchanged();
    expected[index] = labrador::Colour(pending_word);
    require(impl.execute("tune " + path + ' ' + pending_word).ok && document->edits().size() == 3,
        "The timer edit remains independent after selected reset");unchanged();
    const auto held_ledger = ledger();const std::string current = document->current().text();
    const JsonSpanDocument saved_reference(saved, "normal-timer-invalid-merge-reference");
    std::vector<std::string> invalid_sources{
        saved_reference.replaced(*saved_reference.find("hud.paper.keyline_width"), "4.1") };
    if (opaque || positive_alpha)
    {
        for (const char* token : { "\"FFF5DF80\"", "\"FFF5DF00\"" })
            { invalid_sources.push_back(saved_reference.replaced(*saved_reference.find((shared_paper || closing_material || closing_shade) ? "hud.closing_clock.text" : "hud.closing_clock.paper"), token)); }
    }
    for (const std::string& invalid_source : invalid_sources)
    {
        write_text(scratch.source / L"presentation.json", invalid_source);
        for (const char* command : { "tune.save presentation", "tune.save presentation force" })
        {
            require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == invalid_source &&
                read_text(scratch.staged / L"presentation.json") == saved && document->current().text() == current &&
                document->baseline().text() == saved && ledger() == held_ledger && document->edits().size() == 3 &&
                !std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
                !std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
                !std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
                "Ordinary/forced full-reader saves reject invalid external HUD geometry or partial/zero unedited opaque closing-neighbour alpha, restoring exact source and preserving staging/live/documents/three token pairs");unchanged();
        }
    }
    std::string invalid = invalid_sources.back();
    expected[index] = labrador::Colour(saved_word);
    require(impl.execute("tune.reset " + path).ok && document->edits().size() == 2 &&
        document->edits().contains(sibling_local) && document->edits().contains("debug_overlay.colour") && read_text(scratch.source / L"presentation.json") == invalid,
        "Selected timer reset retains its sibling, unrelated pending edit and invalid external source bytes");unchanged();
    if (opaque || positive_alpha)
    {
        // With the selected edit reset, invalid external selected alpha survives
        // splicing of only the independent sibling and overlay and must fail the reader.
        const std::string reset_current = document->current().text();
        const auto reset_ledger = ledger();
        const std::array<const char*, 2> invalid_selected{ positive_alpha ? "\"00000000\"" : "\"2060C080\"", "\"A532D600\"" };
        for (const char* token : invalid_selected)
        {
            invalid = saved_reference.replaced(*saved_reference.find(local), token);
            write_text(scratch.source / L"presentation.json", invalid);
            for (const char* command : { "tune.save presentation", "tune.save presentation force" })
            {
                require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == invalid &&
                    read_text(scratch.staged / L"presentation.json") == saved && document->current().text() == reset_current &&
                    document->baseline().text() == saved && ledger() == reset_ledger && document->edits().size() == 2 &&
                    !std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
                    !std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
                    !std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
                    "Saving only sibling/overlay edits cannot adopt reader-invalid external selected closing colour, even with force; exact source/staging/documents/remaining token pairs survive");unchanged();
            }
        }
    }
    expected = initial;expected[index] = labrador::Colour(saved_word);
    require(impl.execute("tune.reset presentation").ok && document->edits().empty() &&
        document->current().text() == saved && document->baseline().text() == saved &&
        read_text(scratch.staged / L"presentation.json") == saved && read_text(scratch.source / L"presentation.json") == invalid,
        "Resetting the remaining edit restores the saved timer baseline without writing external content");unchanged();
    if (label || closing_material || positive_alpha || shared_paper)
    {
        // Save both independently selected HUD colour tokens in one valid merge.
        // The source whitespace, other clock colours, root badges and captured resources
        // remain guarded separately from this selected colour pair.
        const std::string paired_external = "\r\n" + saved;
        write_text(scratch.source / L"presentation.json", paired_external);
        expected[index] = labrador::Colour(pending_word);
        expected[sibling_index] = labrador::Colour(sibling_word);
        require(impl.execute("tune " + path + ' ' + pending_word).ok &&
            impl.execute("tune " + sibling + " " + sibling_word).ok && document->edits().size() == 2,
            "Both HUD label/material colours are independently pending before their combined private save");unchanged();
        const JsonSpanDocument paired_reference(paired_external, "hud-label-paired-save-reference");
        const std::string first = paired_reference.replaced(*paired_reference.find(local), '"' + pending_word + '"');
        const JsonSpanDocument first_reference(first, "hud-label-paired-second-reference");
        const std::string paired_saved = first_reference.replaced(*first_reference.find(sibling_local), (std::string("\"") + sibling_word + "\""));
        require(impl.execute("tune.save presentation").ok && document->edits().empty() &&
            document->current().text() == paired_saved && document->baseline().text() == paired_saved &&
            read_text(scratch.source / L"presentation.json") == paired_saved &&
            read_text(scratch.staged / L"presentation.json") == paired_saved &&
            colour_values(Presentation((scratch.source / L"presentation.json").string().c_str())) == expected &&
            !std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
            !std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
            "One HUD label/material pair Save splices exactly two tokens, preserves external bytes and all unrelated values, loads and rebases both copies");unchanged();
        expected[index] = labrador::Colour::red;
        require(impl.execute("tune " + path + " RED").ok && document->edits().size() == 1,
            "A saved HUD label/material colour can be edited without changing its saved sibling");unchanged();
        expected[index] = labrador::Colour(pending_word);
        require(impl.execute("tune.reset " + path).ok && document->edits().empty() &&
            document->current().text() == paired_saved && document->baseline().text() == paired_saved &&
            read_text(scratch.source / L"presentation.json") == paired_saved &&
            read_text(scratch.staged / L"presentation.json") == paired_saved,
            "A selected reset after paired Save uses its field-valid saved baseline and preserves the independent sibling");unchanged();
    }
    require(std::rand() == next_random, "HUD clock/label-colour tuning consumed gameplay RNG");
}

void check_timer_colour_tunes(const std::filesystem::path& game_directory)
{
    for (const std::size_t index : { hud_timer_colour_index, hud_timer_shadow_colour_index, closing_clock_text_colour_index, hud_label_colour_index, hud_identity_shadow_colour_index, closing_clock_ink_colour_index, closing_clock_paper_colour_index, closing_clock_accent_colour_index, closing_clock_shade_colour_index, hud_paper_ink_colour_index, hud_paper_paper_colour_index, hud_paper_shade_colour_index })
        { check_timer_colour_tunes_for_field(game_directory, index); }
}
