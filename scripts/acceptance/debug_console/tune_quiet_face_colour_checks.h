// Included in tune_checks.cpp's private namespace. Path 45 is the live blink
// paint-out, separate from the already-audited story acting face and lid ink.
void check_quiet_face_colour_tunes(const std::filesystem::path& game_directory)
{
    const std::string path = "presentation.figure.quiet.face_colour";
    const std::string local = "figure.quiet.face_colour";
    {
        const std::string baseline = "{\r\n\"figure\":{\"quiet\":{\"face_colour\":\"\\u0057HITE\",\"enabled\":true},"
            "\"arm_colour\":\"BLACK\",\"sprite_sheet\":\"captured\"},\r\n\"number\":1\r\n}\r\n";
        const JsonSpanDocument reference(baseline, "quiet-face-document-reference");
        TuneDocument document("presentation", baseline);
        const JsonLeaf* leaf = document.find(local);
        require(leaf != nullptr && leaf->kind == JsonLeaf::Kind::string &&
            document.current().token(*leaf) == "\"\\u0057HITE\"" &&
            document.find("figure.arm_colour") != nullptr && document.find("figure.sprite_sheet") == nullptr,
            "The nested quiet face is an existing audited string while captured art names stay unavailable");
        document.commit(document.edited(*leaf, "\"2060C080\""), local);
        const std::string pending = document.current().text();
        const auto edit = document.edits().at(local);
        require(document.edits().size() == 1 && edit.baseline_token == "\"\\u0057HITE\"" &&
            edit.current_token == "\"2060C080\"" && document.reverted(local) == baseline,
            "The face edit retains both exact quoted tokens and every escaped CRLF reset byte");
        for (const char* token : { "false", "null", "17", "{}", "[]" })
        {
            bool rejected = false;
            try { document.commit(document.edited(*document.find(local), token), local); }
            catch (const std::invalid_argument&) { rejected = true; }
            require(rejected && document.current().text() == pending && document.baseline().text() == baseline &&
                document.edits().size() == 1 && document.edits().at(local).baseline_token == edit.baseline_token &&
                document.edits().at(local).current_token == edit.current_token,
                "Wrong-type face commits preserve current/baseline bytes and both ledger tokens");
        }
        const std::string external = reference.replaced(*reference.find("number"), "25");
        const JsonSpanDocument external_reference(external, "quiet-face-external-reference");
        const TuneSplice spliced = document.splice(external, false);
        require(spliced.conflicts.empty() && spliced.written == 1 && spliced.text ==
            external_reference.replaced(*external_reference.find(local), "\"2060C080\""),
            "A nested face splice preserves the external number, escaped sibling and all CRLF bytes");
        for (const std::string& unavailable : { std::string("{}"),
            reference.replaced(*reference.find(local), "null"),
            reference.replaced(*reference.find(local), "{\"value\":\"WHITE\"}"),
            std::string("{\"figure.quiet.face_colour\":\"WHITE\"}"),
            std::string("{\"figure\":{\"quiet\":{\"face_colour.r\":\"WHITE\"}}}") })
        {
            const TuneDocument unsupported("presentation", unavailable);
            const TuneSplice refused = document.splice(unavailable, true);
            require(unsupported.find(local) == nullptr && refused.written == 0 &&
                refused.conflicts.size() == 1 && refused.text == unavailable,
                "Force cannot insert a missing face, replace null/a container or follow literal dotted aliases");
        }
    }

    // Generic 45-path checks cover named/RGB/RGBA, invalid words, LF/CRLF
    // conflicts/force/atomic replacement and actual reader consumption.
    ScratchContent scratch(game_directory, L"quiet-face-records");
    GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
    Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
    TestDirector test(tuning, presentation);auto& impl = test.impl;
    impl.set_content_directories(scratch.source, scratch.staged);
    const FigurePresentation* const look = &presentation.figure();
    const QuietCharacterPresentation* const quiet = &look->quiet;
    const FigurePresentation original = *look;
    const auto* const fill = &presentation.movement_feedback();
    const auto initial = colour_values(presentation);auto expected = initial;
    const std::string source_before = read_text(scratch.source / L"presentation.json");
    const std::string tuning_before = read_text(scratch.source / L"tuning.json");
    labrador::RenderResources resources;
    for (const auto& name : { fill->sprite_sheet, look->sprite_sheet })
        { resources.add_sprite_sheet(name, labrador::read_sprite_sheet((game_directory / L"textures" / (name + ".json")).string().c_str(), labrador::TextureHandle{})); }
    const FigureAttachments attachment(fill, look, &resources);
    const auto* const art_sheet = resources.sprite_sheet(look->sprite_sheet);
    const auto* const fill_sheet = resources.sprite_sheet(fill->sprite_sheet);
    const std::array<std::string, 4> names{ look->rest_arms_frame, look->rest_arms_paint_frame,
        look->sleeves_frame, look->sleeves_paint_frame };
    const auto art_metadata = [&]
    {
        std::vector<float> result;
        for (const auto& name : names)
        {
            const auto& frame = art_sheet->sprite_frame(art_sheet->resolve_sprite_frame(name));
            const auto box = frame.source_rectangle();const auto origin = frame.origin();
            result.insert(result.end(), { static_cast<float>(box.x), static_cast<float>(box.y),
                static_cast<float>(box.width), static_cast<float>(box.height), origin.x, origin.y });
        }
        return result;
    };
    const auto geometry = [&]
    {
        std::vector<float> result{ look->canvas_size.x, look->canvas_size.y, look->body_inset,
            look->rear_shoulder.x, look->rear_shoulder.y, look->front_shoulder.x, look->front_shoulder.y,
            look->arm_width, look->elbow_drop, look->hand_size, look->hand_edge,
            quiet->enabled ? 1.0f : 0.0f, quiet->settle_time, quiet->breath_period, quiet->breath_height,
            quiet->shift_period, quiet->shift_distance, quiet->blink_min_interval, quiet->blink_max_interval,
            quiet->blink_duration, quiet->lean_distance, quiet->lean_acceleration, quiet->lean_settle_time, quiet->eyelid_height };
        for (const auto& eye : quiet->eyes) { result.insert(result.end(), { eye.x, eye.y, eye.width, eye.height }); }
        for (const auto& grip : look->grips) { result.insert(result.end(), { grip.rear.x, grip.rear.y, grip.front.x, grip.front.y }); }
        return result;
    };
    const mattmath::RectangleF body(100, 120, 104, 240);
    const auto canvas = look->canvas_rectangle(body);
    const auto held_geometry = geometry(), held_art = art_metadata();
    const auto* const document = impl.tune_document("presentation");
    require(document != nullptr && original.quiet.face_colour == labrador::Colour::white,
        "The private quiet-face fixture uses the authored WHITE baseline");
    const auto unchanged = [&]
    {
        require(&presentation.figure() == look && &look->quiet == quiet &&
            &presentation.movement_feedback() == fill && geometry() == held_geometry && art_metadata() == held_art &&
            look->sprite_sheet == original.sprite_sheet && look->rest_arms_frame == names[0] &&
            look->rest_arms_paint_frame == names[1] && look->sleeves_frame == names[2] && look->sleeves_paint_frame == names[3] &&
            resources.sprite_sheet(look->sprite_sheet) == art_sheet && resources.sprite_sheet(fill->sprite_sheet) == fill_sheet &&
            look->canvas_rectangle(body) == canvas && attachment.bounds(canvas, true, nullptr) == canvas &&
            attachment.bounds(canvas, false, nullptr) == canvas && colour_values(presentation) == expected &&
            read_text(scratch.source / L"tuning.json") == tuning_before && read_text(scratch.staged / L"tuning.json") == tuning_before &&
            !impl.in_match() && !impl.can_apply() && impl.request().empty(),
            "Quiet-face commands retain the real borrowed attachment/quiet records, geometry, resources, public bounds and unrelated live values/queues");
    };
    std::srand(0x51464143);const int next_random = std::rand();std::srand(0x51464143);
    for (const auto& [word, colour] : std::array<std::pair<const char*, labrador::Colour>, 4>{ {
        { "RED", { 255, 0, 0 } }, { "a1B2c3", { 161, 178, 195 } },
        { "2060C080", { 32, 96, 192, 128 } }, { "A532D600", { 165, 50, 214, 0 } } } })
    {
        expected[quiet_face_colour_index] = colour;
        require(impl.execute("tune " + path + ' ' + word).ok &&
            impl.tune_token(path) == std::string("\"") + word + '"' && impl.execute("tune " + path).ok &&
            log_contains(impl.log(), path + " = " + impl.tune_token(path), DebugSeverity::info),
            "The retained quiet face accepts named, opaque, partial and nonzero-RGB alpha-zero values live");
        unchanged();
    }
    expected[quiet_face_colour_index] = initial[quiet_face_colour_index];
    require(impl.execute("tune.reset " + path).ok && document->edits().empty() &&
        document->current().text() == source_before && read_text(scratch.source / L"presentation.json") == source_before &&
        read_text(scratch.staged / L"presentation.json") == source_before,
        "Quiet-face reset restores exact authored bytes without writing either content copy");unchanged();

    expected[quiet_face_colour_index] = labrador::Colour("2060C080");
    require(impl.execute("tune " + path + " 2060C080").ok && impl.execute("tune.diff presentation").ok &&
        log_contains(impl.log(), path, DebugSeverity::info), "The nested face appears in its real command diff");unchanged();
    const std::string external = " \r\n" + source_before;
    const JsonSpanDocument external_reference(external, "quiet-face-save-reference");
    const std::string saved = external_reference.replaced(*external_reference.find(local), "\"2060C080\"");
    write_text(scratch.source / L"presentation.json", external);
    require(impl.execute("tune.save presentation").ok && read_text(scratch.source / L"presentation.json") == saved &&
        read_text(scratch.staged / L"presentation.json") == saved && document->baseline().text() == saved &&
        document->current().text() == saved && document->edits().empty() &&
        !std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
        !std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
        "A face-only save changes exactly one token, preserves external whitespace and rebases both copies atomically");unchanged();

    const std::array<std::pair<std::size_t, const char*>, 4> edits{ {
        { quiet_face_colour_index, "A532D600" }, { limb_arm_colour_index, "5167B980" },
        { limb_hand_colour_index, "C6A25D40" }, { 3, "789ABC" } } };
    for (const auto& [index, word] : edits)
    {
        expected[index] = labrador::Colour(word);
        require(impl.execute(std::string("tune ") + colour_paths[index] + ' ' + word).ok,
            "Face, arm, hand and unrelated overlay retain independent pending edits");unchanged();
    }
    expected[quiet_face_colour_index] = labrador::Colour("2060C080");
    require(impl.execute("tune.reset " + path).ok && document->edits().size() == 3 &&
        !document->edits().contains(local) && document->edits().contains("figure.arm_colour") &&
        document->edits().contains("figure.hand_colour") && document->edits().contains("debug_overlay.colour"),
        "Resetting only the face preserves the independent arm, hand and unrelated overlay edits");unchanged();
    expected[quiet_face_colour_index] = labrador::Colour("A532D600");
    require(impl.execute("tune " + path + " A532D600").ok && document->edits().size() == 4,
        "The saved face can be edited independently while its siblings remain pending");unchanged();
    const auto ledger = [&]
    {
        std::vector<std::string> values;
        for (const auto& [key, edit] : document->edits())
            { values.push_back(key + '=' + edit.baseline_token + "->" + edit.current_token); }
        return values;
    };
    const auto held_ledger = ledger();const std::string current = document->current().text();
    const JsonSpanDocument saved_reference(saved, "quiet-face-invalid-merge-reference");
    const std::string invalid = saved_reference.replaced(*saved_reference.find("figure.quiet.eyelid_height"),
        saved_reference.token(*saved_reference.find("figure.quiet.eyes[0].height")));
    write_text(scratch.source / L"presentation.json", invalid);
    for (const char* command : { "tune.save presentation", "tune.save presentation force" })
    {
        require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == invalid &&
            read_text(scratch.staged / L"presentation.json") == saved && document->current().text() == current &&
            document->baseline().text() == saved && ledger() == held_ledger && document->edits().size() == 4,
            "Ordinary and forced full-reader merges reject a positive but invalid eyelid/eye relationship without changing source, staging, live values or any ledger token");unchanged();
    }
    expected[quiet_face_colour_index] = labrador::Colour("2060C080");
    require(impl.execute("tune.reset " + path).ok && document->edits().size() == 3 &&
        read_text(scratch.source / L"presentation.json") == invalid,
        "Selected face reset uses its saved partial-alpha token and leaves siblings and invalid external bytes intact");unchanged();
    expected = initial;expected[quiet_face_colour_index] = labrador::Colour("2060C080");
    require(impl.execute("tune.reset presentation").ok && document->edits().empty() &&
        document->current().text() == saved && document->baseline().text() == saved &&
        read_text(scratch.staged / L"presentation.json") == saved && read_text(scratch.source / L"presentation.json") == invalid,
        "Resetting the remaining siblings restores the saved face baseline without overwriting external content");unchanged();
    require(std::rand() == next_random, "Quiet-face commands consumed gameplay RNG");
}
