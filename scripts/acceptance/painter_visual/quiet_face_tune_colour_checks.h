// Path45 adds a separately counted blink-face scenario to the limb family.
// The old43/44 fixtures, captures, controls and budgets remain unchanged.
enum class QuietFaceReference { correct, straight_face, squared_face, reversed_order };

std::vector<float> quiet_face_fixed(const FigurePresentation& look)
{
    auto copy = look;
    copy.quiet.face_colour = Colour::white;
    return limb_figure_fixed(copy);
}

class QuietFaceColourProbe
{
public:
    QuietFaceColourProbe(Application& app, const Presentation& presentation) :
        app_(app), look_(presentation.figure()), fill_(presentation.movement_feedback()),
        original_(look_), disabled_look_(limb_disabled_look(look_)),
        live_(&fill_, &look_, app.render_resources()),
        disabled_(&fill_, &disabled_look_, app.render_resources()),
        absent_(&fill_, nullptr, app.render_resources()),
        sheet_(app.render_resources()->sprite_sheet(fill_.sprite_sheet)),
        pixel_(sheet_->sprite_frame(sheet_->resolve_sprite_frame(fill_.fill_frame))),
        fixed_(quiet_face_fixed(look_)), fill_fixed_(limb_fill_fixed(fill_))
    {
        require(look_.canvas_size == Vector2F(68, 120) && look_.quiet.enabled &&
            look_.quiet.eyes[0] == RectangleF(24, 24, 4, 8) &&
            look_.quiet.eyes[1] == RectangleF(40, 24, 4, 8) && look_.quiet.eyelid_height == 4.0f &&
            look_.quiet.face_colour == Colour::white && look_.arm_colour == Colour::black &&
            look_.hand_colour == Colour::white && fill_.sprite_sheet == "sprite_sheet_1" &&
            fill_.fill_frame == "pixel" && pixel_.source_rectangle() == RectangleI(32, 0, 4, 4) &&
            pixel_.origin() == Vector2F::ZERO,
            "Quiet face requires the authored opaque defaults, two literal eyes and solid source");
        unchanged();
    }

    void unchanged() const
    {
        const auto* resources = app_.render_resources();
        require(quiet_face_fixed(look_) == fixed_ && limb_figure_names(look_) == limb_figure_names(original_) &&
            look_.hand_colour == original_.hand_colour && limb_fill_fixed(fill_) == fill_fixed_ &&
            resources->sprite_sheet(fill_.sprite_sheet) == sheet_ &&
            &sheet_->sprite_frame(sheet_->resolve_sprite_frame(fill_.fill_frame)) == &pixel_ &&
            pixel_.source_rectangle() == RectangleI(32, 0, 4, 4) && pixel_.origin() == Vector2F::ZERO &&
            limb_figure_fixed(disabled_look_) == limb_figure_fixed(limb_disabled_look(original_)) &&
            disabled_look_.arm_colour == original_.arm_colour && disabled_look_.hand_colour == original_.hand_colour &&
            app_.viewport_manager()->fullscreen_viewport().pixel_rect() == RectangleI(0, 0, 1280, 720),
            "Quiet face changed unselected geometry, resources, hand tint, disabled record or viewport");
        for (bool right : { false, true })
        {
            const RectangleF canvas(32, 32, 68, 120);
            require(live_.bounds(canvas, right, nullptr) == canvas &&
                disabled_.bounds(canvas, right, nullptr) == canvas && absent_.bounds(canvas, right, nullptr) == canvas,
                "Quiet face drawing changed public attachment bounds");
        }
    }

    Frame actual(size_t background)
    {
        return render(background, [&](DrawList& list)
        {
            live_.draw_blink(list, { 32, 32, 68, 120 }, true);
            live_.draw_blink(list, { 160, 32, 68, 120 }, false);
            disabled_.draw_blink(list, { 32, 160, 68, 120 }, true);
            disabled_.draw_blink(list, { 160, 160, 68, 120 }, false);
            absent_.draw_blink(list, { 32, 288, 68, 120 }, true);
            absent_.draw_blink(list, { 160, 288, 68, 120 }, false);
        });
    }

    Frame raw(size_t background, const Colour& face, const Colour& arm,
        QuietFaceReference reference = QuietFaceReference::correct)
    {
        return render(background, [&](DrawList& list)
        {
            // These are the four literal world-space eye rectangles, including
            // the left-facing x reflection. No production geometry helper or
            // FigureAttachments/CrudeDrawing drawing seam forms this oracle.
            const Colour face_tint = reference == QuietFaceReference::straight_face ? face :
                reference == QuietFaceReference::squared_face ?
                Colour(face.r * face.a * face.a, face.g * face.a * face.a, face.b * face.a * face.a, face.a) :
                Colour(face.r * face.a, face.g * face.a, face.b * face.a, face.a);
            const Colour lid(arm.r * arm.a, arm.g * arm.a, arm.b * arm.a, arm.a);
            for (const RectangleI& eye : eyes())
            {
                const RectangleI line(eye.x, eye.y + 2, 4, 4);
                if (reference == QuietFaceReference::reversed_order) { rectangle(list, line, lid); }
                rectangle(list, eye, face_tint);
                if (reference != QuietFaceReference::reversed_order) { rectangle(list, line, lid); }
            }
        });
    }

    Frame observe(size_t background, const Colour& face, const Colour& arm)
    {
        require(look_.quiet.face_colour == face && look_.arm_colour == arm,
            "Quiet face observation does not match the selected live channels");
        Frame actual_frame = actual(background);
        const Frame repeated = actual(background), reference = raw(background, face, arm);
        require(actual_frame.pixels == repeated.pixels, "Quiet face repeated draw changed frozen RGBA");
        require(actual_frame.pixels == reference.pixels,
            "Quiet face differs from the complete independent literal ordered reference");
        size_t pixels = 0, face_pixels = 0, lid_pixels = 0;
        for (const RectangleI& eye : eyes())
        {
            for (int y = eye.y; y < eye.bottom(); ++y) { for (int x = eye.x; x < eye.right(); ++x)
            {
                const size_t at = offset(x, y);
                require(same_pixel(actual_frame, reference, at) && actual_frame.pixels[at + 3] == 255,
                    "Quiet face raw RGBA eye pixel differs or lost opaque target alpha");
                ++pixels;
                if (y >= eye.y + 2 && y < eye.y + 6) { ++lid_pixels; }
                else { ++face_pixels; }
            } }
        }
        require(pixels == 128 && face_pixels == 64 && lid_pixels == 64,
            "Quiet face literal four-eye face/lid pixel accounting differs");
        return actual_frame;
    }

    size_t control(const Frame& correct, size_t background, const Colour& face, const Colour& arm,
        QuietFaceReference reference)
    {
        const Frame wrong = raw(background, face, arm, reference);
        size_t different = 0;
        for (int y = 0; y < 720; ++y) { for (int x = 0; x < 1280; ++x)
        {
            bool selected = false;
            for (const RectangleI& eye : eyes())
            {
                const bool inside = x >= eye.x && x < eye.right() && y >= eye.y && y < eye.bottom();
                const bool lid = y >= eye.y + 2 && y < eye.y + 6;
                selected = selected || (inside && (reference == QuietFaceReference::reversed_order ? lid : !lid));
            }
            const bool equal = same_pixel(correct, wrong, offset(x, y));
            require(equal != selected, "Quiet face negative reference lost exact role discrimination/exterior parity");
            if (!equal) { ++different; }
        } }
        require(different == 64, "Quiet face negative reference must discriminate exactly64 role pixels");
        return different;
    }

    size_t readbacks() const { return readbacks_; }
    static Frame compact(const Frame& frame) { return foreground_crop(frame, { 48, 48, 176, 24 }, 0); }
    static const char* background_name(size_t background) { return background == 0 ? "black" : "blue"; }

private:
    static const std::array<RectangleI, 4>& eyes()
    {
        static const std::array<RectangleI, 4> value{ RectangleI(56, 56, 4, 8), RectangleI(72, 56, 4, 8),
            RectangleI(200, 56, 4, 8), RectangleI(184, 56, 4, 8) };
        return value;
    }
    static size_t offset(int x, int y) { return (static_cast<size_t>(y) * 1280 + static_cast<size_t>(x)) * 4; }
    static bool same_pixel(const Frame& left, const Frame& right, size_t at)
    { return std::equal(left.pixels.begin() + at, left.pixels.begin() + at + 4, right.pixels.begin() + at); }
    void rectangle(DrawList& list, const RectangleI& box, const Colour& tint) const
    {
        list.draw_sprite(sheet_->texture(), pixel_.source_rectangle(),
            RectangleF(static_cast<float>(box.x), static_cast<float>(box.y),
                static_cast<float>(box.width), static_cast<float>(box.height)),
            tint, 0.0f, Vector2F::ZERO, SpriteFlip::none, 0.0f);
    }
    Frame render(size_t background, const std::function<void(DrawList&)>& draw)
    {
        unchanged();
        require(background < 2 && readbacks_ < 54, "Quiet face exceeded its background/readback budget");
        const std::array<Colour, 3> selected{ look_.quiet.face_colour, look_.arm_colour, look_.hand_colour };
        std::srand(45017); const int next = std::rand(); std::srand(45017);
        const Canvas canvas(app_.render_resources(), background == 0 ? Colour::black : Colour(32, 64, 96));
        Renderer& renderer = *app_.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app_.viewport_manager()->fullscreen_viewport());
        list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        canvas.draw(list, { 0, 0, 1280, 720 }); draw(list);
        rectangle(list, { 20, 20, 4, 4 }, Colour::white);
        rectangle(list, { 25, 20, 4, 4 }, Colour::black);
        Frame result = read(renderer); ++readbacks_;
        require(result.width == 1280 && result.height == 720 && result.pixels.size() == 1280 * 720 * 4,
            "Quiet face requires a native1280x720 RGBA backbuffer");
        for (int y = 20; y < 24; ++y) { for (int x = 20; x < 29; ++x)
        {
            if (x == 24) { continue; }
            const size_t at = offset(x, y); const unsigned char channel = x < 24 ? 255 : 0;
            require(result.pixels[at] == channel && result.pixels[at + 1] == channel &&
                result.pixels[at + 2] == channel && result.pixels[at + 3] == 255,
                "Quiet face actual WHITE/BLACK RGBA calibration differs");
        } }
        require(std::rand() == next && selected == std::array<Colour, 3>{ look_.quiet.face_colour, look_.arm_colour, look_.hand_colour },
            "Quiet face drawing changed selected channels or consumed gameplay RNG");
        unchanged(); return result;
    }
    Application& app_;
    const FigurePresentation& look_;
    const MovementFeedbackPresentation& fill_;
    FigurePresentation original_, disabled_look_;
    FigureAttachments live_, disabled_, absent_;
    const SpriteSheet* sheet_;
    const SpriteFrame& pixel_;
    std::vector<float> fixed_, fill_fixed_;
    size_t readbacks_ = 0;
};

void quiet_face_colour_consumer_checks(Application& app, const Presentation& presentation,
    const std::filesystem::path& output)
{
    Presentation local = presentation;
    auto& look = const_cast<FigurePresentation&>(local.figure());
    QuietFaceColourProbe probe(app, local);
    const std::array<Colour, 5> faces{ Colour::white, Colour(53, 188, 225), Colour(32, 96, 192, 128),
        Colour(165, 50, 214, 0), Colour(32, 96, 192, 128) };
    const std::array<Colour, 5> arms{ Colour::black, Colour::black, Colour::black, Colour::black, Colour(192, 64, 96, 128) };
    constexpr std::array<const char*, 5> names{ "default", "rgb", "rgba128", "alpha0", "joint" };
    size_t rows = 0, crops = 0, controls = 0, discriminating = 0;
    std::ofstream csv(output / "quiet-face-colour-consumers.csv");
    csv << "state,background,raw_rgba_match,repeat_match,disabled_null_blank,frozen_geometry_resources_rng,controls,discriminating_pixels,scope\n";
    for (size_t state = 0; state < faces.size(); ++state)
    {
        look.quiet.face_colour = faces[state]; look.arm_colour = arms[state];
        for (size_t background = 0; background < 2; ++background)
        {
            const Frame actual = probe.observe(background, faces[state], arms[state]);
            size_t case_controls = 0, case_pixels = 0;
            const auto control = [&](QuietFaceReference reference)
            { case_pixels += probe.control(actual, background, faces[state], arms[state], reference); ++case_controls; };
            if (state == 2) { control(QuietFaceReference::straight_face); control(QuietFaceReference::squared_face); }
            else if (state == 3) { control(QuietFaceReference::straight_face); }
            else if (state == 4) { control(QuietFaceReference::reversed_order); }
            csv << names[state] << ',' << QuietFaceColourProbe::background_name(background)
                << ",true,true,true,true," << case_controls << ',' << case_pixels << ",direct-consumer-only\n";
            save(QuietFaceColourProbe::compact(actual), output / (std::string("quiet-face-colour-") + names[state] +
                '-' + QuietFaceColourProbe::background_name(background) + ".ppm"));
            ++rows; ++crops; controls += case_controls; discriminating += case_pixels;
        }
    }
    require(csv.good() && probe.readbacks() == 38 && rows == 10 && crops == 10 && controls == 8 && discriminating == 512,
        "Quiet face shared exact readback/row/crop/negative-role budget differs");
    std::ofstream summary(output / "quiet-face-colour-consumer-summary.json");
    summary << "{\"scope\":\"direct-consumer-only\",\"readbacks\":38,\"csv_rows\":10,\"crops\":10,"
        "\"crop_width\":176,\"crop_height\":24,\"crop_pixels\":42240,\"ppm_bytes\":126860,"
        "\"states\":5,\"backgrounds\":2,\"active_contexts\":2,\"disabled_contexts\":2,\"null_contexts\":2,"
        "\"roi_pixels\":128,\"face_pixels\":64,\"lid_pixels\":64,\"roi_pixel_checks\":1280,\"repeat_checks\":10,"
        "\"controls\":8,\"discriminating_pixels\":512,\"calibration_pixel_checks\":1216,\"private_game_saves\":0,"
        "\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"scene_updates\":0,\"state_updates\":0,"
        "\"input_updates\":0,\"time_updates\":0,\"input_polls\":0,\"observation_weapon_updates\":0}\n";
    require(summary.good(), "Cannot retain Quiet face shared summary");
    std::cout << "PASS quiet face consumer:38readbacks/10rows/10native176x24crops;128literalRGBA/case,"
        "8controls512role inequalities,1216calibrationRGBA; zero updates/input polls/private game Saves.\n";
}

#if COLOURWARS_DEV_TOOLS
void quiet_face_tune_colour_checks(Application& app, Registry<LevelLoadedInfo>& levels, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& content, const std::filesystem::path& output)
{
    const auto root = output / "quiet-face-tune-scratch", source = root / "source", staged = root / "staged";
    require(!std::filesystem::exists(root), "Quiet face private scratch must be fresh");
    std::filesystem::create_directories(source); std::filesystem::create_directories(staged);
    constexpr std::array<const char*, 2> files{ "tuning.json", "presentation.json" };
    std::array<std::string, 2> original_bytes;
    for (size_t i = 0; i < files.size(); ++i)
    {
        original_bytes[i] = tune_colour_bytes(content / files[i]);
        tune_colour_write(source / files[i], original_bytes[i]); tune_colour_write(staged / files[i], original_bytes[i]);
    }
    const NestedSaveProfile profile(root / "profile"); const TuneColourTemp temporary(root / "temp");
    struct RestoreQuietFace
    {
        GameTuning& tuning; Presentation& presentation; GameTuning old_tuning; Presentation old_presentation;
        ~RestoreQuietFace() { tuning = old_tuning; presentation = old_presentation; }
    } restore{ tuning, presentation, tuning, presentation };
    Save preferences;
    const auto preference_values = [&]()
    {
        auto value = limb_preference_values(preferences.play_settings());
        value.insert(value.end(), { static_cast<int>(preferences.resolution()), preferences.fullscreen() ? 1 : 0,
            static_cast<int>(preferences.team_palette()), static_cast<int>(preferences.input_bindings().keyboard_jump) });
        return value;
    };
    const auto original_preferences = preference_values(); StageList stages;
    for (const char* name : { "movement_course", "close_quarters", "king_of_the_hill", "turbulence", "crossfeed" })
    {
        const auto* info = levels.get(name); require(info != nullptr, "Quiet face requires shipped stage metadata");
        stages.push_back({ name, info->display_name(), info->icon_frame(), info->include_in_random() });
    }
    GameData data; data.set_application(&app); data.set_save(&preferences);
    const StageCatalog stage_catalog = acceptance::catalog_of(levels, stages, { "close_quarters", "crossfeed", "king_of_the_hill", "movement_course", "turbulence" }); data.set_stage_catalog(&stage_catalog); data.set_tuning(&tuning); data.set_presentation(&presentation);
    DebugDirector director(&tuning, &presentation); data.set_debug(&director);
    auto& impl = *director.impl(); impl.set_content_directories(source, staged); impl.set_user_directory(root / "user");
    impl.set_script_directories({ root / "scripts" }); director.attach(&app, &data); impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output / "quiet-face-tune-commands.log").string()),
        "Cannot retain Quiet face command transcript");
    QuietFaceColourProbe probe(app, presentation);
    JsonSpanDocument initial(original_bytes[1], "Quiet face private source");
    const auto* face_leaf = initial.find("figure.quiet.face_colour");
    require(face_leaf != nullptr, "Quiet face source lacks its exact existing nested token");
    JsonSpanDocument face_saved(initial.replaced(*face_leaf, "\"C0406080\""), "Quiet face saved token");
    const auto* arm_leaf = face_saved.find("figure.arm_colour");
    require(arm_leaf != nullptr, "Quiet face source lacks its independently supported arm sibling");
    const std::string expected_saved = face_saved.replaced(*arm_leaf, "\"2060C080\"");
    bool persisted = false; size_t commands = 0, rows = 0, named_checks = 0;
    const auto unchanged = [&]()
    {
        probe.unchanged();
        require(preference_values() == original_preferences &&
            !std::filesystem::exists(root / "profile/ColourWars/save_data.json") &&
            !impl.in_match() && !impl.can_apply() && impl.request().empty(),
            "Quiet face command changed private defaults, wrote a game Save or queued gameplay work");
        for (size_t i = 0; i < files.size(); ++i)
        {
            const auto& expected = i == 1 && persisted ? expected_saved : original_bytes[i];
            require(tune_colour_bytes(content / files[i]) == original_bytes[i] &&
                tune_colour_bytes(source / files[i]) == expected && tune_colour_bytes(staged / files[i]) == expected,
                "Quiet face command changed real/private content beyond its exact saved face/arm tokens");
        }
    };
    const auto command = [&](const std::string& line)
    {
        const size_t before = impl.log().lines().size();
        std::srand(45019); const int next = std::rand(); std::srand(45019);
        const auto result = impl.execute(line); ++commands;
        require(result.ok && std::rand() == next, "Actual Quiet face command failed or consumed gameplay RNG");
        if (line.starts_with("tune "))
        {
            const auto& log = impl.log().lines();
            require(log.size() > before && log.back().severity == DebugSeverity::info &&
                log.back().text.starts_with("tune: presentation.figure.") && log.back().text.ends_with("(live)"),
                "Quiet face command lost its fresh live-adoption reply");
        }
    };
    std::array<Frame, 2> baseline{}, saved{};
    std::ofstream csv(output / "quiet-face-tune.csv");
    csv << "state,background,raw_rgba_match,repeat_match,disabled_null_blank,frozen_geometry_resources_rng,private_bytes,adoption,face_token,arm_token\n";
    const auto check = [&](const char* state, const Colour& face, const Colour& arm,
        const char* face_token, const char* arm_token)
    {
        unchanged();
        const auto* document = impl.tune_document("presentation"); require(document != nullptr, "Quiet face document disappeared");
        const auto& current = document->current();
        const auto* f = current.find("figure.quiet.face_colour"), *a = current.find("figure.arm_colour");
        require(f != nullptr && a != nullptr && current.token(*f) == std::string("\"") + face_token + '"' &&
            current.token(*a) == std::string("\"") + arm_token + '"', "Quiet face live document lost exact selected/sibling tokens");
        for (size_t background = 0; background < 2; ++background)
        {
            const Frame frame = probe.observe(background, face, arm);
            if (std::string_view(state) == "baseline") { baseline[background] = frame; }
            if (std::string_view(state) == "reset-original")
            { require(frame.pixels == baseline[background].pixels, "Quiet face original reset lost exact complete RGBA"); }
            if (std::string_view(state) == "partial-both") { saved[background] = frame; }
            if (std::string_view(state) == "reset-arm-saved")
            { require(frame.pixels == saved[background].pixels, "Quiet face final saved reset lost exact joint RGBA"); }
            csv << state << ',' << QuietFaceColourProbe::background_name(background)
                << ",true,true,true,true,true,live," << face_token << ',' << arm_token << '\n'; ++rows;
        }
        if (std::string_view(state) == "named-face")
        {
            require(document->edits().size() == 1 && document->edits().contains("figure.quiet.face_colour") &&
                document->edits().at("figure.quiet.face_colour").current_token == "\"RED\"",
                "Quiet face named input lost its exact quoted RED token"); ++named_checks;
        }
        if (std::string_view(state) == "reset-face-saved")
        {
            require(document->edits().size() == 1 && document->edits().contains("figure.arm_colour") &&
                document->edits().at("figure.arm_colour").current_token == "\"D65A3200\"",
                "Quiet face selected saved reset discarded its independent pending arm sibling");
        }
        unchanged();
    };
    check("baseline", Colour::white, Colour::black, "WHITE", "BLACK");
    command("tune presentation.figure.quiet.face_colour RED");
    check("named-face", Colour(255, 0, 0), Colour::black, "RED", "BLACK");
    command("tune.reset presentation.figure.quiet.face_colour");
    check("reset-original", Colour::white, Colour::black, "WHITE", "BLACK");
    command("tune presentation.figure.arm_colour 2060C080");
    check("partial-arm", Colour::white, Colour(32, 96, 192, 128), "WHITE", "2060C080");
    command("tune presentation.figure.quiet.face_colour C0406080");
    check("partial-both", Colour(192, 64, 96, 128), Colour(32, 96, 192, 128), "C0406080", "2060C080");
    require(impl.tune_document("presentation")->edits().size() == 2 &&
        impl.tune_document("presentation")->edits().contains("figure.quiet.face_colour") &&
        impl.tune_document("presentation")->edits().contains("figure.arm_colour"),
        "Quiet face private Save requires exactly the selected nested and independent arm tokens");
    command("tune.save presentation"); persisted = true; unchanged();
    require(impl.tune_document("presentation")->edits().empty(), "Quiet face private Save failed exact rebase");
    command("tune presentation.figure.quiet.face_colour A532D600");
    check("zero-face", Colour(165, 50, 214, 0), Colour(32, 96, 192, 128), "A532D600", "2060C080");
    command("tune presentation.figure.arm_colour D65A3200");
    check("zero-both", Colour(165, 50, 214, 0), Colour(214, 90, 50, 0), "A532D600", "D65A3200");
    command("tune.reset presentation.figure.quiet.face_colour");
    check("reset-face-saved", Colour(192, 64, 96, 128), Colour(214, 90, 50, 0), "C0406080", "D65A3200");
    command("tune.reset presentation.figure.arm_colour");
    check("reset-arm-saved", Colour(192, 64, 96, 128), Colour(32, 96, 192, 128), "C0406080", "2060C080");
    require(impl.tune_document("presentation")->edits().empty() && impl.tune_document("tuning")->edits().empty() &&
        csv.good() && commands == 9 && named_checks == 1 && rows == 18 && probe.readbacks() == 54,
        "Quiet face Debug exact command/state/readback budget or final saved reset differs");
    std::ofstream summary(output / "quiet-face-tune-summary.json");
    summary << "{\"scope\":\"debug-validated-commands\",\"adoption\":\"live\",\"commands\":9,\"states\":9,"
        "\"backgrounds\":2,\"readbacks\":54,\"csv_rows\":18,\"crops\":0,\"saved_tokens\":2,\"named_face_checks\":1,"
        "\"roi_pixel_checks\":2304,\"repeat_checks\":18,\"calibration_pixel_checks\":1728,\"private_game_saves\":0,"
        "\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"scene_updates\":0,\"state_updates\":0,"
        "\"input_updates\":0,\"time_updates\":0,\"input_polls\":0,\"observation_weapon_updates\":0}\n";
    require(summary.good(), "Cannot retain Quiet face Debug summary");
    std::cout << "PASS quiet face tune:9real commands/9states/54additional reads/18rows; two quoted face/arm tokens Save/rebase,"
        " sibling-retaining saved resets and unchanged private defaults; zero game Saves/updates/input polls.\n";
}
#endif
