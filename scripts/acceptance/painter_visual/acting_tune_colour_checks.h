// Included after shared painter fixtures. Retained FigureActing is the actual
// consumer; independent literal eye/pupil/strip writes prove ordered alpha.
enum class ActingColourReference { correct, straight_face, squared_face, straight_sweat, squared_sweat };
std::vector<float> acting_fixed_style(const OpeningPresentation& p)
{
    std::vector<float> v{ p.palette_height, p.adult_height, p.child_height, p.cursor_size, p.preview_strength,
        p.percent_height, p.hop, p.shake, p.slump, p.monitor_art.pen_width, p.monitor_art.corner_step,
        p.monitor_art.shadow_width, static_cast<float>(p.monitor_art.vent_count), p.silhouette_art.pen_width,
        p.silhouette_art.shade_width, p.cursor_art.shadow_offset, p.cursor_art.click_duration, p.cursor_art.click_reach,
        p.reveal_art.keyline_width, p.reveal_art.padding, p.reveal_art.shadow_offset, p.reveal_art.tick_length, p.reveal_art.beat_duration };
    for (const auto& r : { p.glass, p.monitor_art.vent_area, p.monitor_art.power_button, p.monitor_art.power_indicator })
    { v.insert(v.end(), { r.x, r.y, r.width, r.height }); }
    const auto colour = [&](const Colour& c) { v.insert(v.end(), { c.r, c.g, c.b, c.a }); };
    for (const auto& c : { p.bezel_shadow, p.monitor_art.ink, p.monitor_art.highlight, p.desktop_colour, p.program_colour,
        p.glass_colour, p.monitor_art.power_light, p.slot_outline, p.scribble, p.coffee, p.crack, p.silhouette,
        p.silhouette_art.ink, p.silhouette_art.rim, p.silhouette_art.shade, p.cursor_art.ink, p.cursor_art.fill,
        p.cursor_art.shade, p.cursor_art.shadow, p.reveal_art.ink, p.reveal_art.paper, p.reveal_art.shade,
        p.figure_grey }) { colour(c); }
    for (const auto* colours : { &p.bezel_shades, &p.sticky_notes, &p.stickers })
    { v.push_back(static_cast<float>(colours->size())); for (const auto& c : *colours) { colour(c); } }
    return v;
}

class ActingColourProbe
{
public:
    ActingColourProbe(Application& app, const Presentation& presentation) :
        app_(app), presentation_(presentation), look_(presentation.opening()), baseline_(look_), expected_(look_),
        strokes_(presentation.respawn_redraw()), baseline_strokes_(strokes_), expected_strokes_(strokes_),
        actor_(&look_, &strokes_, app.render_resources()), reference_(&expected_, &expected_strokes_, app.render_resources()),
        sheet_(app.render_resources()->sprite_sheet(look_.sprite_sheet)),
        frame_(sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame))), fixed_(acting_fixed_style(look_))
    {
        require(look_.sprite_sheet == "sprite_sheet_1" && look_.fill_frame == "pixel" &&
            frame_.source_rectangle() == RectangleI(32, 0, 4, 4) && frame_.origin() == Vector2F::ZERO &&
            strokes_.canvas_size == Vector2F(68, 120) && strokes_.pen_width == 4.0f,
            "Acting fixture lost captured solid frame or literal68x120/nib4 geometry");
        const std::vector<RespawnStroke> authored{
            { "cap", { { 20, 4 }, { 44, 4 } } },
            { "cap", { { 4, 16 }, { 48, 16 }, { 48, 8 } } },
            { "cap", { { 16, 8 }, { 16, 16 } } },
            { "head", { { 12, 28 }, { 16, 28 } } },
            { "head", { { 48, 28 }, { 52, 28 } } },
            { "head", { { 12, 32 }, { 16, 32 } } },
            { "head", { { 48, 32 }, { 52, 32 } } },
            { "head", { { 16, 44 }, { 48, 44 }, { 48, 20 } } },
            { "head", { { 16, 20 }, { 16, 44 } } },
            { "eye", { { 24, 24 }, { 24, 28 } } },
            { "eye", { { 40, 24 }, { 40, 28 } } },
            { "mouth", { { 28, 40 }, { 36, 40 }, { 36, 36 } } },
            { "neck", { { 28, 48 }, { 28, 44 } } },
            { "neck", { { 36, 48 }, { 36, 44 } } },
            { "body", { { 16, 52 }, { 48, 52 } } },
            { "body", { { 16, 84 }, { 48, 84 }, { 48, 64 } } },
            { "body", { { 16, 76 }, { 16, 84 } } },
            { "body", { { 32, 80 }, { 32, 84 } } },
            { "arm", { { 12, 72 }, { 16, 72 } } },
            { "arm", { { 52, 72 }, { 56, 72 } } },
            { "arm", { { 8, 56 }, { 8, 68 } } },
            { "arm", { { 56, 56 }, { 56, 60 } } },
            { "arm", { { 60, 64 }, { 60, 68 } } },
            { "leg", { { 16, 116 }, { 28, 116 }, { 28, 88 } } },
            { "leg", { { 36, 116 }, { 48, 116 } } },
            { "leg", { { 20, 88 }, { 20, 116 } } },
            { "leg", { { 36, 88 }, { 36, 116 } } },
            { "leg", { { 44, 88 }, { 44, 116 } } },
        };
        require(stroke_values(strokes_.strokes, authored), "Acting fixture lost shipped parts/point order; literal oracle is no longer qualified");
    }
    void unchanged() const
    {
        require(&look_ == &presentation_.opening() && &strokes_ == &presentation_.respawn_redraw() &&
            acting_fixed_style(look_) == fixed_ && acting_fixed_style(expected_) == fixed_ &&
            look_.sprite_sheet == baseline_.sprite_sheet && look_.fill_frame == baseline_.fill_frame &&
            look_.palette_sheet == baseline_.palette_sheet && look_.palette_frames == baseline_.palette_frames &&
            look_.percent_font == baseline_.percent_font && sheet_ == app_.render_resources()->sprite_sheet(look_.sprite_sheet) &&
            &frame_ == &sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame)) &&
            redraw_values(strokes_, baseline_strokes_) && redraw_values(expected_strokes_, baseline_strokes_),
            "Acting edit/draw changed borrowed records, unselected style, ordered strokes or captured resources");
    }
    Frame actual(const Colour& background, bool right)
    {
        unchanged(); std::srand(51613); const int next = std::rand(); std::srand(51613);
        const auto draw = [&](DrawList& list) { draw_actor(actor_, list, right); };
        Frame result = render(background, draw); require(result.pixels == render(background, draw).pixels, "Frozen acting draw changed on repeat");
        require(std::rand() == next, "Acting draw consumed gameplay RNG"); unchanged(); return result;
    }
    Frame adoption(const Colour& background, bool right, const Colour& face, const Colour& sweat)
    { expected_.face = face; expected_.sweat = sweat; unchanged(); return render(background, [&](DrawList& list) { draw_actor(reference_, list, right); }); }
    Frame raw(const Colour& background, bool right, const Colour& face, const Colour& sweat,
        ActingColourReference mode = ActingColourReference::correct)
    {
        const float face_factor = mode == ActingColourReference::straight_face ? 1.0f :
            mode == ActingColourReference::squared_face ? face.a * face.a : face.a;
        const float sweat_factor = mode == ActingColourReference::straight_sweat ? 1.0f :
            mode == ActingColourReference::squared_sweat ? sweat.a * sweat.a : sweat.a;
        const Colour face_tint(face.r * face_factor, face.g * face_factor, face.b * face_factor, face.a);
        const Colour sweat_tint(sweat.r * sweat_factor, sweat.g * sweat_factor, sweat.b * sweat_factor, sweat.a);
        const Frame result = render(background, [&](DrawList& list)
        {
            const auto fill = [&](const RectangleI& box, const Colour& tint)
            { list.draw_sprite(sheet_->texture(), frame_.source_rectangle(),
                RectangleF(static_cast<float>(box.x), static_cast<float>(box.y), static_cast<float>(box.width), static_cast<float>(box.height)),
                tint, 0.0f, frame_.origin(), SpriteFlip::none, 0.0f); };
            // Actual eye order mirrors when facing left. The disjoint masks are
            // literal audited rectangles; raw geometry never calls eye_rectangles/blob.
            for (const int eye : { 0, 1 })
            { const int x = right ? (eye == 0 ? 220 : 300) : (eye == 0 ? 300 : 220);
              fill({ x, 220, 20, 40 }, face_tint); fill({ x + (right ? 7 : 1), 236, 11, 24 }, Colour(0, 0, 0)); }
            for (const RectangleI& box : sweat_strips(right)) { fill(box, sweat_tint); }
        });
        if (face.a == 1.0f && mode != ActingColourReference::squared_face && mode != ActingColourReference::straight_face)
        { require(monitor_rgb(result, 222, 222) == rgb(face), "Acting face opaque calibration failed captured WHITE texels"); }
        if (sweat.a == 1.0f && mode != ActingColourReference::squared_sweat && mode != ActingColourReference::straight_sweat)
        { require(monitor_rgb(result, right ? 399 : 140, 177) == rgb(sweat), "Acting sweat opaque calibration failed captured WHITE texels"); }
        return result;
    }
    Frame blank(const Colour& background) { return render(background, [](DrawList&) {}); }
    static std::array<RectangleI, 8> sweat_strips(bool right)
    {
        return right ? std::array<RectangleI, 8>{ RectangleI(381,175,36,6), RectangleI(382,169,33,6), RectangleI(385,163,28,6), RectangleI(391,157,16,6),
            RectangleI(381,181,36,6), RectangleI(382,187,33,6), RectangleI(385,193,28,6), RectangleI(391,199,16,6) } :
            std::array<RectangleI, 8>{ RectangleI(122,175,36,6), RectangleI(124,169,33,6), RectangleI(126,163,28,6), RectangleI(132,157,16,6),
            RectangleI(122,181,36,6), RectangleI(124,187,33,6), RectangleI(126,193,28,6), RectangleI(132,199,16,6) };
    }
    static bool in_box(int x, int y, const RectangleI& box)
    { return x >= box.x && x < box.right() && y >= box.y && y < box.bottom(); }
    static bool face_mask(int x, int y)
    { return in_box(x, y, { 220,220,20,40 }) || in_box(x, y, { 300,220,20,40 }); }
    static bool sweat_mask(int x, int y, bool right)
    { if (y < 157 || y >= 205 || x < (right ? 381 : 122) || x >= (right ? 417 : 158)) { return false; }
      for (const auto& box : sweat_strips(right)) { if (in_box(x, y, box)) { return true; } } return false; }
    static bool pupil_mask(int x, int y, bool right)
    { return in_box(x, y, { right ? 227 : 221,236,11,24 }) || in_box(x, y, { right ? 307 : 301,236,11,24 }); }
    static void mask_guards(const Frame& actual, const Frame& blank, bool right)
    {
        for (int y = 0; y < actual.height; ++y) { for (int x = 0; x < actual.width; ++x)
        { if (!face_mask(x, y) && !sweat_mask(x, y, right))
          { const size_t at = (static_cast<size_t>(y) * static_cast<size_t>(actual.width) + static_cast<size_t>(x)) * 4;
            require(same_pixel(actual, blank, at), "Acting pixel escaped literal eye/sweat union"); } } }
    }
    static void wrong_field_only(const Frame& actual, const Frame& wrong, bool right, bool face)
    {
        require(actual.pixels != wrong.pixels, "Acting fixture cannot reject selected per-field alpha defect");
        for (int y = 0; y < actual.height; ++y) { for (int x = 0; x < actual.width; ++x)
        { if (!(face ? face_mask(x, y) : sweat_mask(x, y, right)))
          { const size_t at = (static_cast<size_t>(y) * static_cast<size_t>(actual.width) + static_cast<size_t>(x)) * 4;
            require(same_pixel(actual, wrong, at), "Acting negative control changed the correctly modeled sibling/outside mask"); } } }
    }
    static void alpha0(const Frame& actual, const Frame& blank, bool right, bool face_zero, bool sweat_zero)
    {
        for (int y = 0; y < actual.height; ++y) { for (int x = 0; x < actual.width; ++x)
        { if (pupil_mask(x, y, right)) { require(monitor_rgb(actual, x, y) == 0, "Acting pupil must remain opaque BLACK"); }
          else if ((face_zero && face_mask(x, y)) || (sweat_zero && sweat_mask(x, y, right)))
          { const size_t at = (static_cast<size_t>(y) * static_cast<size_t>(actual.width) + static_cast<size_t>(x)) * 4;
            require(same_pixel(actual, blank, at), "Zero-alpha acting material leaked RGB over backdrop"); } } }
    }
    static Frame compact(const Frame& source, bool right)
    {
        Frame result{ 320,128,{} }; result.pixels.reserve(320 * 128 * 4);
        for (int y = 0; y < 128; ++y) { for (const Vector2I origin : { Vector2I(190,196), Vector2I(right ? 320 : 60,128) })
        { const size_t at = (static_cast<size_t>(origin.y + y) * static_cast<size_t>(source.width) + static_cast<size_t>(origin.x)) * 4;
          result.pixels.insert(result.pixels.end(), source.pixels.begin() + at, source.pixels.begin() + at + 160 * 4); } }
        return result;
    }
    size_t readbacks() const { return readbacks_; }
private:
    static std::uint32_t rgb(const Colour& c)
    { return (static_cast<std::uint32_t>(std::round(c.r * 255)) << 16) | (static_cast<std::uint32_t>(std::round(c.g * 255)) << 8) |
        static_cast<std::uint32_t>(std::round(c.b * 255)); }
    static bool stroke_values(const std::vector<RespawnStroke>& a, const std::vector<RespawnStroke>& b)
    { if (a.size() != b.size()) { return false; } for (size_t i = 0; i < a.size(); ++i)
        { if (a[i].part != b[i].part || a[i].points != b[i].points) { return false; } } return true; }
    static bool redraw_values(const RespawnRedrawPresentation& a, const RespawnRedrawPresentation& b)
    { return a.sprite_sheet == b.sprite_sheet && a.pen_frame == b.pen_frame && a.pen_colour == b.pen_colour &&
        a.canvas_size == b.canvas_size && a.pen_width == b.pen_width && a.draw_from == b.draw_from && a.fill_at == b.fill_at &&
        a.camera_hold == b.camera_hold && stroke_values(a.strokes, b.strokes); }
    static void draw_actor(const FigureActing& actor, DrawList& list, bool right)
    { actor.draw_eyes(list, { 100,100,340,600 }, right, { right ? 0.6f : -0.6f,1.0f }); actor.draw_sweat(list, { 100,100,340,600 }, right, 0.2f); }
    Frame render(const Colour& background, const std::function<void(DrawList&)>& draw)
    {
        const Canvas canvas(app_.render_resources(), background); Renderer& renderer = *app_.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app_.viewport_manager()->fullscreen_viewport()); list.set_camera(Camera::DEFAULT_CAMERA);
        list.set_filter(TextureFilter::point); canvas.draw(list, { 0,0,1280,720 }); draw(list); ++readbacks_; return read(renderer);
    }
    Application& app_; const Presentation& presentation_; const OpeningPresentation& look_; OpeningPresentation baseline_, expected_;
    const RespawnRedrawPresentation& strokes_; RespawnRedrawPresentation baseline_strokes_, expected_strokes_;
    FigureActing actor_, reference_; const SpriteSheet* sheet_; const SpriteFrame& frame_; std::vector<float> fixed_; size_t readbacks_ = 0;
};

void acting_colour_consumer_checks(Application& app, const Presentation& presentation, const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player); app.resolution_manager()->set_resolution_exactly({ 1280,720 });
    std::ignore = app.renderer()->window_size_changed(1280,720); Presentation local = presentation;
    auto& look = const_cast<OpeningPresentation&>(local.opening());
    require(look.face == Colour(255,255,255) && look.sweat == Colour(143,211,255), "Acting authored opaque defaults changed");
    ActingColourProbe probe(app, local); const std::array<Colour,2> backgrounds{ Colour(0,0,0),Colour(32,64,96) };
    const std::array<Frame,2> blanks{ probe.blank(backgrounds[0]),probe.blank(backgrounds[1]) };
    const std::array<Colour,4> faces{ Colour(255,255,255),Colour(53,188,225),Colour(32,96,192,128),Colour(165,50,214,0) };
    const std::array<Colour,4> sweats{ Colour(143,211,255),Colour(224,122,49),Colour(192,64,160,96),Colour(75,156,39,0) };
    constexpr std::array<const char*,4> names{ "default","rgb","rgba","alpha0" };
    std::ofstream records(output / "acting-colour-consumers.csv");
    records << "state,facing,background,full_raw_match,adoption_match,mask_guards,pupils_retained,frozen_records_strokes_rng,controls,scope\n";
    size_t rows = 0,crops = 0,controls = 0;
    for (size_t p = 0; p < faces.size(); ++p) { look.face = faces[p]; look.sweat = sweats[p];
      for (const bool right : { false,true }) { for (size_t b = 0; b < backgrounds.size(); ++b)
      {
        const Frame actual = probe.actual(backgrounds[b],right),adoption = probe.adoption(backgrounds[b],right,faces[p],sweats[p]),
            raw = probe.raw(backgrounds[b],right,faces[p],sweats[p]);
        if (actual.pixels != raw.pixels)
        { save(ActingColourProbe::compact(actual,right),output / "acting-failure-actual.ppm"); save(ActingColourProbe::compact(raw,right),output / "acting-failure-raw.ppm"); }
        require(actual.pixels == raw.pixels && actual.pixels == adoption.pixels, "Acting actual pixels differ from full independent raw or adoption reference");
        ActingColourProbe::mask_guards(actual,blanks[b],right); ActingColourProbe::alpha0(actual,blanks[b],right,p == 3,p == 3);
        if (p >= 2)
        { for (const auto mode : { ActingColourReference::straight_face,ActingColourReference::straight_sweat })
          { ActingColourProbe::wrong_field_only(actual,probe.raw(backgrounds[b],right,faces[p],sweats[p],mode),right,mode == ActingColourReference::straight_face); ++controls; } }
        if (p == 2)
        { for (const auto mode : { ActingColourReference::squared_face,ActingColourReference::squared_sweat })
          { ActingColourProbe::wrong_field_only(actual,probe.raw(backgrounds[b],right,faces[p],sweats[p],mode),right,mode == ActingColourReference::squared_face); ++controls; } }
        records << names[p] << ',' << (right ? "right" : "left") << ',' << (b == 0 ? "black" : "nonblack") << ",true,true,true,true,true,"
            << (p < 2 ? "opaque-parity" : p == 2 ? "each-field-straight-and-squared-rejected" : "each-field-straight-rejected-pupils-retained") << ",consumer-only\n"; ++rows;
        save(ActingColourProbe::compact(actual,right),output / (std::string("acting-colour-") + names[p] + (right ? "-right" : "-left") + (b == 0 ? "-black.ppm" : "-nonblack.ppm"))); ++crops;
      } }
    }
    require(records.good() && probe.readbacks() == 90 && rows == 16 && crops == 16 && controls == 24, "Acting shared budget differs");
    std::ofstream summary(output / "acting-colour-consumer-summary.json");
    summary << "{\"scope\":\"consumer-only\",\"readbacks\":90,\"csv_rows\":16,\"crops\":16,\"controls\":24,\"blank_reads\":2,"
        "\"straight_face_controls\":8,\"straight_sweat_controls\":8,\"squared_face_controls\":4,\"squared_sweat_controls\":4,"
        "\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,"
        "\"figure\":[100,100,340,600],\"eye_masks\":[[220,220,20,40],[300,220,20,40]],\"sweat_strips_per_facing\":8,\"facings\":2,\"canvas_backgrounds\":2,"
        "\"viewport\":\"1280x720\",\"event_fade\":false,\"adoption_reference\":\"production-consumer-only\",\"alpha_reference\":\"independent-ordered-full-raw\"}\n";
    require(summary.good(), "Cannot write acting shared summary");
    std::cout << "PASS acting:90readbacks/16rows/16native320x128eye+sweat crops/24per-field controls; full independent ordered raw, both facings/two Canvas, "
        "opaque parity/calibration, all mask guards and alpha0 pupils. Retained FigureActing adoption; zero all updates.\n";
}

#if COLOURWARS_DEV_TOOLS
void acting_tune_colour_checks(Application& app, Registry<LevelLoadedInfo>& levels, GameTuning& tuning, Presentation& presentation,
    const std::filesystem::path& content, const std::filesystem::path& output)
{
    const auto root = output / "acting-colour-tune-scratch", source = root / "source", staged = root / "staged";
    std::filesystem::create_directories(source); std::filesystem::create_directories(staged);
    constexpr std::array<const char*, 2> files{ "tuning.json", "presentation.json" }; std::array<std::string, 2> real_bytes;
    for (size_t i = 0; i < files.size(); ++i) { real_bytes[i] = tune_colour_bytes(content / files[i]);
        tune_colour_write(source / files[i], real_bytes[i]); tune_colour_write(staged / files[i], real_bytes[i]); }
    const NestedSaveProfile profile(root / "profile"); const TuneColourTemp temporary(root / "temp");
    struct RestoreActing { GameTuning& t; Presentation& p; GameTuning old_t; Presentation old_p; ~RestoreActing() { t = old_t; p = old_p; } }
        restore{ tuning, presentation, tuning, presentation };
    Save preferences; PlaySettings play; play.music_volume = 17; play.sfx_volume = 83; play.aim_sensitivity = 4;
    play.view_shake = false; play.aim_marker = true; play.reduce_effects = true; play.sticks[2].movement_deadzone = 25; play.sticks[2].aim_deadzone = 70;
    InputBindings bindings; bindings.keyboard_jump = Key::j;
    require(preferences.set_play_settings_and_save(play) && preferences.set_input_bindings_and_save(bindings), "Cannot seed acting private preferences");
    const auto save_path = root / "profile/ColourWars/save_data.json"; const std::string saved_preferences = tune_colour_bytes(save_path);
    const auto pref_values = [&]
    { const auto& p = preferences.play_settings(); std::vector<int> v{ static_cast<int>(preferences.resolution()), preferences.fullscreen() ? 1 : 0,
        static_cast<int>(preferences.team_palette()), p.aim_sensitivity, p.music_volume, p.sfx_volume, p.view_shake ? 1 : 0,
        p.aim_marker ? 1 : 0, p.reduce_effects ? 1 : 0, static_cast<int>(preferences.input_bindings().keyboard_jump) };
      for (const auto& stick : p.sticks) { v.push_back(stick.movement_deadzone); v.push_back(stick.aim_deadzone); } return v; };
    const auto saved_values = pref_values(); StageList stages;
    for (const char* name : { "movement_course", "close_quarters", "king_of_the_hill", "turbulence" })
    { const auto* info = levels.get(name); stages.push_back({ name, info->display_name(), info->icon_frame(), info->include_in_random() }); }
    GameData data; data.set_application(&app); data.set_save(&preferences); const StageCatalog stage_catalog = acceptance::catalog_of(levels, stages, { "close_quarters", "king_of_the_hill", "movement_course", "turbulence" }); data.set_stage_catalog(&stage_catalog);
    data.set_tuning(&tuning); data.set_presentation(&presentation); DebugDirector director(&tuning, &presentation); data.set_debug(&director);
    auto& impl = *director.impl(); impl.set_content_directories(source, staged); impl.set_user_directory(root / "user");
    impl.set_script_directories({ root / "scripts" }); director.attach(&app, &data); impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output / "acting-colour-tune-commands.log").string()), "Cannot retain acting transcript");
    ActingColourProbe probe(app, presentation); JsonSpanDocument initial(real_bytes[1], "acting private source");
    const JsonLeaf* leaf = initial.find("opening.acting.face"); require(leaf != nullptr, "Acting source lacks exact existing Face token");
    const std::string expected_saved = initial.replaced(*leaf, "\"2060C080\""); bool persisted = false; size_t commands = 0, rows = 0;
    const auto unchanged = [&]
    { probe.unchanged(); require(!impl.can_apply() && impl.request().empty() && !impl.in_match() &&
        tune_colour_bytes(save_path) == saved_preferences && pref_values() == saved_values, "Acting commands changed private prefs/default display/JumpJ or queued work");
      for (size_t i = 0; i < files.size(); ++i) { const auto& expected = i == 1 && persisted ? expected_saved : real_bytes[i];
        require(tune_colour_bytes(content / files[i]) == real_bytes[i] && tune_colour_bytes(source / files[i]) == expected && tune_colour_bytes(staged / files[i]) == expected,
            "Acting command changed actual content or private bytes beyond one Face token"); } };
    const auto command = [&](const std::string& line)
    { const size_t before = impl.log().lines().size(); std::srand(51613); const int next = std::rand(); std::srand(51613);
      const auto status = impl.execute(line); ++commands; require(status.ok && std::rand() == next, "Actual acting command failed or consumed RNG");
      if (line.starts_with("tune ")) { const auto& log = impl.log().lines(); require(log.size() > before && log.back().severity == DebugSeverity::info &&
          (log.back().text.starts_with("tune: presentation.opening.acting.face") || log.back().text.starts_with("tune: presentation.opening.acting.sweat")) &&
          log.back().text.ends_with("(live)"), "Acting live reply differs"); } };
    const Colour background(0,0,0); Frame saved_raster{};
    std::ofstream records(output / "acting-colour-tune.csv"); records << "state,facing,background,full_raw_match,adoption_match,frozen_records_strokes_rng,private_bytes,adoption\n";
    const auto check = [&](const char* state, const Colour& face, const Colour& sweat)
    {
        unchanged(); require(presentation.opening().face == face && presentation.opening().sweat == sweat, "Acting command parsed wrong channels or failed sibling retention");
        const Frame actual = probe.actual(background,true),adoption = probe.adoption(background,true,face,sweat),raw = probe.raw(background,true,face,sweat);
        require(actual.pixels == adoption.pixels && actual.pixels == raw.pixels, "Retained acting failed full independent raw or adoption after real command");
        require(monitor_rgb(actual,227,236) == 0 && monitor_rgb(actual,307,236) == 0, "Acting real command erased opaque pupils");
        if (face.a == 0.0f) { require(monitor_rgb(actual,222,222) == 0, "Zero-alpha commanded Face leaked RGB"); }
        if (sweat.a == 0.0f) { require(monitor_rgb(actual,399,177) == 0, "Zero-alpha commanded Sweat leaked RGB"); }
        if (std::string_view(state) == "sweat-reset-original") { saved_raster = actual; }
        if (std::string_view(state) == "face-reset-saved")
        { require(foreground_crop(actual,{220,220,100,40},0).pixels == foreground_crop(saved_raster,{220,220,100,40},0).pixels,
            "Acting Face reset failed exact saved eye raster while retaining alpha0 Sweat"); }
        if (std::string_view(state) == "reset-all-saved") { require(actual.pixels == saved_raster.pixels, "Acting saved pair reset failed exact raster restoration"); }
        records << state << ",right,black,true,true,true,true,live\n"; ++rows; unchanged();
    };
    const Colour white(255,255,255),original_sweat(143,211,255),red(255,0,0),blue(0,0,255),
        rgb_face(53,188,225),rgb_sweat(224,122,49),rgba_face(32,96,192,128),rgba_sweat(192,64,160,96),
        zero_face(165,50,214,0),zero_sweat(75,156,39,0);
    check("baseline",white,original_sweat);
    command("tune presentation.opening.acting.face RED"); check("named-face",red,original_sweat);
    command("tune presentation.opening.acting.sweat BLUE"); check("named-both",red,blue);
    command("tune presentation.opening.acting.face 35BCE1"); check("rgb-face",rgb_face,blue);
    command("tune presentation.opening.acting.sweat E07A31"); check("rgb-both",rgb_face,rgb_sweat);
    command("tune presentation.opening.acting.face 2060C080"); check("rgba-face",rgba_face,rgb_sweat);
    command("tune presentation.opening.acting.sweat C040A060"); check("rgba-both",rgba_face,rgba_sweat);
    command("tune.reset presentation.opening.acting.face"); check("face-reset-original",white,rgba_sweat);
    command("tune presentation.opening.acting.face 2060C080"); check("face-restored-rgba",rgba_face,rgba_sweat);
    command("tune.reset presentation.opening.acting.sweat"); check("sweat-reset-original",rgba_face,original_sweat);
    const auto* document = impl.tune_document("presentation"); require(document != nullptr && document->edits().size() == 1 &&
        document->edits().contains("opening.acting.face"), "Acting save ledger differs from exact one Face token after Sweat reset");
    command("tune.save presentation"); persisted = true; unchanged(); require(impl.tune_document("presentation")->edits().empty(), "Acting save failed to rebase");
    command("tune presentation.opening.acting.face A532D600"); check("face-alpha0",zero_face,original_sweat);
    command("tune presentation.opening.acting.sweat 4B9C2700"); check("alpha0-both",zero_face,zero_sweat);
    command("tune.reset presentation.opening.acting.face"); check("face-reset-saved",rgba_face,zero_sweat);
    command("tune.reset presentation"); check("reset-all-saved",rgba_face,original_sweat);
    require(impl.tune_document("presentation")->edits().empty() && impl.tune_document("tuning")->edits().empty(), "Acting final reset left edits");
    require(records.good() && commands == 14 && rows == 14 && probe.readbacks() == 56, "Acting Debug budget differs");
    std::ofstream summary(output / "acting-colour-tune-summary.json"); summary << "{\"scope\":\"debug-validated-commands\",\"commands\":14,\"states\":14,"
        "\"readbacks\":56,\"csv_rows\":14,\"crops\":0,\"saved_tokens\":1,\"facings\":1,\"canvas_backgrounds\":1,"
        "\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0}\n";
    require(summary.good(), "Cannot write acting Debug summary");
    std::cout << "PASS acting tune:14real statements/14states/56reads/14rows, right-facing black Canvas only; real both-path named/RGB/RGBA/alpha0 commands, "
        "sibling-retaining resets, exact one-Face-token save/rebase/saved-eye and pair reversal. Frozen records/strokes/resources/RNG and private PlaySettings+JumpJ/default display; zero all updates.\n";
}
#endif
