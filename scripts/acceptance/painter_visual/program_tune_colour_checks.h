// Retained Monitor::draw; independent visible program-mask ROI
// proves ordered glass/program alpha; whole Monitor reference is adoption only.
enum class ProgramColourReference { correct, straight, squared_base };

// Every opening field but the program colour this leaf tunes.
std::vector<float> program_fixed_style(OpeningPresentation p)
{
    p.program_colour = Colour();
    return opening_style(p);
}

class ProgramColourProbe
{
public:
    ProgramColourProbe(Application& app, const Presentation& presentation) :
        app_(app), presentation_(presentation), look_(presentation.opening()), baseline_(look_), expected_(baseline_),
        monitor_(&look_, app.render_resources(), {}), reference_(&expected_, app.render_resources(), {}),
        sheet_(app.render_resources()->sprite_sheet(look_.sprite_sheet)),
        frame_(sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame))), fixed_(program_fixed_style(look_)),
        persona_(monitor_persona_observation(monitor_.persona()))
    {
        require(look_.glass == RectangleF(0.1f, 0.07f, 0.8f, 0.76f) && look_.monitor_art.ink == Colour(0, 0, 0) &&
            look_.glass_colour == Colour(22, 22, 22) && look_.palette_height == 0.09f && look_.monitor_art.pen_width == 0.004f && look_.monitor_art.power_indicator == RectangleF(0.974f, 0.906f, 0.008f, 0.014f) &&
            frame_.source_rectangle() == RectangleI(32, 0, 4, 4) && frame_.origin() == Vector2F::zero,
            "Program fixture lost shipped glass/solid-frame assumptions");
    }
    void unchanged() const
    {
        require(&look_ == &presentation_.opening() && &look_.monitor_art == &presentation_.opening().monitor_art &&
            program_fixed_style(look_) == fixed_ && program_fixed_style(expected_) == fixed_ &&
            look_.sprite_sheet == baseline_.sprite_sheet && look_.fill_frame == baseline_.fill_frame &&
            look_.palette_sheet == baseline_.palette_sheet && look_.palette_frames == baseline_.palette_frames &&
            look_.percent_font == baseline_.percent_font && sheet_ == app_.render_resources()->sprite_sheet(look_.sprite_sheet) &&
            &frame_ == &sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame)) &&
            persona_ == monitor_persona_observation(monitor_.persona()) && persona_ == monitor_persona_observation(reference_.persona()),
            "Program edit/draw changed borrowed opening, unrelated style/resources or captured persona");
    }
    Frame actual(const Colour& background)
    {
        unchanged(); std::srand(31601); const int next = std::rand(); std::srand(31601);
        const auto draw = [&](DrawList& list) { monitor_.draw(list, { 0, 0, 1280, 720 }); };
        Frame result = render(background, draw); require(result.pixels == render(background, draw).pixels, "Frozen program changed on repeat");
        require(std::rand() == next, "Program draw consumed gameplay RNG"); unchanged(); return result;
    }
    Frame adoption(const Colour& background, const Colour& colour)
    { expected_.program_colour = colour; unchanged(); return render(background, [&](DrawList& list) { reference_.draw(list, { 0, 0, 1280, 720 }); }); }
    Frame raw(const Colour& background, const Colour& colour, ProgramColourReference mode = ProgramColourReference::correct)
    {
        const float rgb = mode == ProgramColourReference::straight ? 1.0f :
            mode == ProgramColourReference::squared_base ? colour.a * colour.a : colour.a;
        const Frame result = render(background, [&](DrawList& list)
        {
            list.draw_sprite(sheet_->texture(), frame_.source_rectangle(), { 135, 58, 1009, 482 }, Colour(22, 22, 22),
                0.0f, frame_.origin(), SpriteFlip::none, 0.0f);
            list.draw_sprite(sheet_->texture(), frame_.source_rectangle(), { 136, 59, 1007, 480 },
                Colour(colour.r * rgb, colour.g * rgb, colour.b * rgb, colour.a), 0.0f, frame_.origin(), SpriteFlip::none, 0.0f);
        });
        if (colour.a == 1.0f)
        {
            const std::uint32_t expected = (static_cast<std::uint32_t>(std::round(colour.r * 255)) << 16) |
                (static_cast<std::uint32_t>(std::round(colour.g * 255)) << 8) | static_cast<std::uint32_t>(std::round(colour.b * 255));
            require(monitor_rgb(result, 640, 320) == expected, "Program opaque calibration lost WHITE texel/alpha");
        }
        return result;
    }
    Frame glass_only(const Colour& background)
    { return render(background, [&](DrawList& list)
        { list.draw_sprite(sheet_->texture(), frame_.source_rectangle(), { 135, 58, 1009, 482 }, Colour(22, 22, 22),
            0.0f, frame_.origin(), SpriteFlip::none, 0.0f); }); }
    static RectangleI roi() { return { 135, 58, 1009, 482 }; }
    static bool roi_equal(const Frame& a, const Frame& b)
    { return foreground_crop(a, roi(), 0).pixels == foreground_crop(b, roi(), 0).pixels; }
    static void edges(const Frame& actual, const Frame& raw)
    {
        require(roi_equal(actual, raw), "Program ordered raw glass/mask ROI differs from actual Monitor raster");
        for (const Vector2I p : { Vector2I(640, 320), Vector2I(136, 59), Vector2I(1142, 538), Vector2I(135, 58), Vector2I(1143, 539) })
        { require(monitor_rgb(actual, p.x, p.y) == monitor_rgb(raw, p.x, p.y), "Program centre/edge/glass guard differs"); }
        require(monitor_rgb(actual, 135, 58) == 0x161616 && monitor_rgb(actual, 1143, 539) == 0x161616,
            "Program guard must retain opaque glass");
    }
    static void delta_only_mask(const Frame& actual, const Frame& baseline)
    {
        for (int y = 0; y < actual.height; ++y) { for (int x = 0; x < actual.width; ++x)
            { if (x < 136 || x >= 1143 || y < 59 || y >= 539)
                { const size_t at = (static_cast<size_t>(y) * static_cast<size_t>(actual.width) + static_cast<size_t>(x)) * 4;
                  require(same_pixel(actual, baseline, at), "Program palette delta escaped visible 1007x480 mask"); } } }
    }
    static void alpha0_glass(const Frame& actual)
    { for (int y = 58; y < 540; ++y) { for (int x = 135; x < 1144; ++x)
        { require(monitor_rgb(actual, x, y) == 0x161616, "Zero-alpha program must retain opaque glass, not Canvas"); } } }
    static Frame compact(const Frame& source)
    {
        Frame result{ 320, 192, {} }; result.pixels.reserve(320 * 192 * 4);
        for (int y = 0; y < 192; ++y) { for (const Vector2I origin : { Vector2I(120, 43), Vector2I(999, 363) })
            { const size_t at = (static_cast<size_t>(origin.y + y) * static_cast<size_t>(source.width) + static_cast<size_t>(origin.x)) * 4;
              result.pixels.insert(result.pixels.end(), source.pixels.begin() + at, source.pixels.begin() + at + 160 * 4); } }
        return result;
    }
    size_t readbacks() const { return readbacks_; }
private:
    Frame render(const Colour& background, const std::function<void(DrawList&)>& draw)
    {
        const Canvas canvas(app_.render_resources(), background); Renderer& renderer = *app_.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app_.viewport_manager()->fullscreen_viewport()); list.set_camera(Camera::default_camera);
        list.set_filter(TextureFilter::point); canvas.draw(list, { 0, 0, 1280, 720 }); draw(list); ++readbacks_; return read(renderer);
    }
    Application& app_; const Presentation& presentation_; const OpeningPresentation& look_; OpeningPresentation baseline_, expected_;
    Monitor monitor_, reference_; const SpriteSheet* sheet_; const SpriteFrame& frame_; std::vector<float> fixed_, persona_; size_t readbacks_ = 0;
};

void program_colour_consumer_checks(Application& app, const Presentation& presentation, const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player); app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720); Presentation local = presentation;
    auto& look = const_cast<OpeningPresentation&>(local.opening()); require(look.program_colour == Colour(44, 44, 44), "Program opaque default changed");
    ProgramColourProbe probe(app, local); const Colour background(0, 0, 0); Frame baseline{};
    const Frame glass_only = probe.glass_only(background);
    const std::array<Colour, 4> palettes{ Colour(44, 44, 44), Colour(53, 188, 225), Colour(32, 96, 192, 128), Colour(165, 50, 214, 0) };
    constexpr std::array<const char*, 4> names{ "default", "rgb", "rgba128", "alpha0" };
    std::ofstream records(output / "program-colour-consumers.csv");
    records << "state,background,roi_raw_match,adoption_match,glass_guards,frozen_style_persona_rng,control,scope\n";
    size_t rows = 0, crops = 0, controls = 0;
    for (size_t p = 0; p < palettes.size(); ++p)
    {
        look.program_colour = palettes[p];
        const Frame actual = probe.actual(background), adoption = probe.adoption(background, palettes[p]), raw = probe.raw(background, palettes[p]);
        if (!ProgramColourProbe::roi_equal(actual, raw))
        { save(ProgramColourProbe::compact(actual), output / "program-failure-actual.ppm"); save(ProgramColourProbe::compact(raw), output / "program-failure-raw.ppm"); }
        require(actual.pixels == adoption.pixels, "Program retained whole-Monitor adoption differs"); ProgramColourProbe::edges(actual, raw);
        if (p == 0) { baseline = actual; } else
        { ProgramColourProbe::delta_only_mask(actual, baseline); require(actual.pixels != baseline.pixels, "Program palette had no measured delta"); }
        if (p >= 2)
        { require(!ProgramColourProbe::roi_equal(actual, probe.raw(background, palettes[p], ProgramColourReference::straight)), "Program fixture cannot reject straight RGB"); ++controls; }
        if (p == 2)
        { require(!ProgramColourProbe::roi_equal(actual, probe.raw(background, palettes[p], ProgramColourReference::squared_base)), "Program fixture cannot reject squared base alpha"); ++controls; }
        if (p == 3) { ProgramColourProbe::alpha0_glass(actual); require(ProgramColourProbe::roi_equal(actual, glass_only), "Program alpha0 glass-only parity differs"); }
        records << names[p] << ",black,true,true,true,true," << (p < 2 ? "opaque-parity" : p == 2 ? "straight-and-squared-rejected" : "straight-rejected-glass-retained") << ",consumer-only\n"; ++rows;
        save(ProgramColourProbe::compact(actual), output / (std::string("program-colour-") + names[p] + "-black.ppm")); ++crops;
    }
    require(records.good() && probe.readbacks() == 20 && rows == 4 && crops == 4 && controls == 3, "Program shared budget differs");
    std::ofstream summary(output / "program-colour-consumer-summary.json");
    summary << "{\"scope\":\"consumer-only\",\"readbacks\":20,\"csv_rows\":4,\"crops\":4,\"controls\":3,\"glass_only_reads\":1,"
        "\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,"
        "\"roi\":[135,58,1009,482],\"inner\":[136,59,1007,480],\"viewport\":\"1280x720\",\"event_fade\":false,"
        "\"adoption_reference\":\"production-consumer-only\",\"alpha_reference\":\"independent-ordered-raw-roi\"}\n";
    require(summary.good(), "Cannot write program shared summary");
    std::cout << "PASS program:20readbacks/4rows/4native320x192corner montages/3controls; independent opaque glass then premult visible program mask, "
        "full-ROI equality, mask-only deltas and alpha0 glass retention. Whole-Monitor adoption only; zero all updates.\n";
}
#if COLOURWARS_DEV_TOOLS
void program_tune_colour_checks(Application& app, GameTuning& tuning, Presentation& presentation,
    const std::filesystem::path& content, const std::filesystem::path& output)
{
    TuneColourSession session(app, tuning, presentation, content, output,
        { .scratch = "program-colour-tune-scratch", .transcript = "program-colour-tune-commands.log",
        .seed = seed_tune_colour_jump });
    const std::filesystem::path& source = session.source();
    const std::filesystem::path& staged = session.staged();
    const auto& files = tune_colour_files;
    const auto& real_bytes = session.real_bytes();
    const auto save_path = session.save_path();
    DebugDirectorImpl& impl = session.impl();
    const std::string saved_preferences = session.seeded_save();
    const auto pref_values = [&] { return session.preference_values(); };
    const auto saved_values = session.preference_values();
    ProgramColourProbe probe(app, presentation); JsonSpanDocument initial(real_bytes[1], "program private source");
    const JsonLeaf* leaf = initial.find("opening.program_colour"); require(leaf != nullptr, "Program source lacks exact existing token");
    const std::string expected_saved = initial.replaced(*leaf, "\"2060C080\""); bool persisted = false; size_t commands = 0, rows = 0;
    const auto unchanged = [&]
    { probe.unchanged(); require(!impl.can_apply() && impl.request().empty() && !impl.in_match() &&
        tune_colour_bytes(save_path) == saved_preferences && pref_values() == saved_values, "Program commands changed private prefs/default display/JumpJ or queued work");
      for (size_t i = 0; i < files.size(); ++i) { const auto& expected = i == 1 && persisted ? expected_saved : real_bytes[i];
        require(tune_colour_bytes(content / files[i]) == real_bytes[i] && tune_colour_bytes(source / files[i]) == expected && tune_colour_bytes(staged / files[i]) == expected,
            "Program command changed actual content or private bytes beyond one token"); } };
    const auto command = [&](const std::string& line)
    { const size_t before = impl.log().lines().size(); std::srand(31601); const int next = std::rand(); std::srand(31601);
      const auto status = impl.execute(line); ++commands; require(status.ok && std::rand() == next, "Actual program command failed or consumed RNG");
      if (line.starts_with("tune ")) { const auto& log = impl.log().lines(); require(log.size() > before && log.back().severity == DebugSeverity::info &&
          log.back().text.starts_with("tune: presentation.opening.program_colour") && log.back().text.ends_with("(live)"), "Program live reply differs"); } };
    const std::array<Colour, 1> backgrounds{ Colour(0, 0, 0) }; std::array<Frame, 1> saved_raster{}, initial_raster{};
    std::ofstream records(output / "program-colour-tune.csv"); records << "state,background,roi_raw_match,adoption_match,frozen_style_persona_rng,private_bytes,adoption\n";
    const auto check = [&](const char* state, const Colour& expected)
    { unchanged(); require(presentation.opening().program_colour == expected, "Program command parsed wrong literal channels");
      for (size_t b = 0; b < backgrounds.size(); ++b) { const Frame actual = probe.actual(backgrounds[b]);
        require(actual.pixels == probe.adoption(backgrounds[b], expected).pixels && ProgramColourProbe::roi_equal(actual, probe.raw(backgrounds[b], expected)),
            "Retained program failed independent ordered ROI or whole-Monitor adoption after real command");
        if (std::string_view(state) == "baseline") { initial_raster[b] = actual; }
        else { ProgramColourProbe::delta_only_mask(actual, initial_raster[b]); }
        if (expected.a == 0.0f) { ProgramColourProbe::alpha0_glass(actual); }
        if (std::string_view(state) == "rgba128") { saved_raster[b] = actual; }
        if (std::string_view(state) == "reset-saved" || std::string_view(state) == "reset-all-saved")
        { require(actual.pixels == saved_raster[b].pixels, "Program saved reset failed exact raster restoration"); }
        records << state << ',' << (b == 0 ? "black" : "nonblack") << ",true,true,true,true,live\n"; ++rows; } unchanged(); };
    check("baseline", Colour(44, 44, 44)); command("tune presentation.opening.program_colour RED"); check("named", Colour(255, 0, 0));
    command("tune.reset presentation.opening.program_colour"); check("reset-original", Colour(44, 44, 44));
    command("tune presentation.opening.program_colour 35BCE1"); check("rgb", Colour(53, 188, 225));
    command("tune presentation.opening.program_colour 2060C080"); check("rgba128", Colour(32, 96, 192, 128));
    const auto* document = impl.tune_document("presentation"); require(document != nullptr && document->edits().size() == 1 &&
        document->edits().contains("opening.program_colour"), "Program save ledger differs from one exact token");
    command("tune.save presentation"); persisted = true; unchanged(); require(impl.tune_document("presentation")->edits().empty(), "Program save failed to rebase");
    command("tune presentation.opening.program_colour A532D600"); check("alpha0", Colour(165, 50, 214, 0));
    command("tune.reset presentation.opening.program_colour"); check("reset-saved", Colour(32, 96, 192, 128));
    command("tune presentation.opening.program_colour 35BCE1"); check("rgb-after-save", Colour(53, 188, 225));
    command("tune.reset presentation"); check("reset-all-saved", Colour(32, 96, 192, 128));
    require(impl.tune_document("presentation")->edits().empty() && impl.tune_document("tuning")->edits().empty(), "Program final reset left edits");
    require(records.good() && commands == 9 && rows == 9 && probe.readbacks() == 36, "Program Debug budget differs");
    std::ofstream summary(output / "program-colour-tune-summary.json"); summary << "{\"scope\":\"debug-validated-commands\",\"commands\":9,\"states\":9,"
        "\"readbacks\":36,\"csv_rows\":9,\"crops\":0,\"saved_tokens\":1,\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0}\n";
    require(summary.good(), "Cannot write program Debug summary");
    std::cout << "PASS program tune:9real statements/36readbacks/9rows; one-token private save/rebase/reset, exact saved raster, retained resources/style/persona/RNG "
        "and unchanged real content/private PlaySettings+JumpJ/default display. Zero local/Player/Level/State/input/time updates.\n";
}
#endif
