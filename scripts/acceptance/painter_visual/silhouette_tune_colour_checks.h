// Included after common foreground and Debug tune helpers. The full artwork is
// observed, while only three declared interior mug patches have an independent
// raw-white-frame channel oracle. Child coverage is aggregate adoption only.
enum class SilhouetteColourReference { correct, straight, squared_base, one_ink, one_shade, full_shade_alpha };

struct SilhouetteColourPalette
{
    Colour ink, rim, shade;
};

class SilhouetteColourProbe
{
public:
    SilhouetteColourProbe(Application& app, const Presentation& presentation) :
        app_(app), presentation_(presentation), look_(presentation.opening()), geometry_(look_),
        person_(&look_, app.render_resources()), sheet_(app.render_resources()->sprite_sheet(look_.sprite_sheet)),
        frame_(sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame)))
    {
        require(look_.silhouette == Colour(14, 14, 14) && look_.adult_height == 0.62f &&
            look_.child_height == 0.44f && look_.silhouette_art.pen_width == 0.008f &&
            look_.silhouette_art.shade_width == 0.025f &&
            frame_.source_rectangle() == RectangleI(32, 0, 4, 4) && frame_.origin() == Vector2F::ZERO,
            "Silhouette mug-patch fixture no longer has its reviewed opaque body/height/width geometry");
        adult_.mug = true; child_.child = true;
        adult_before_ = monitor_persona_observation(adult_); child_before_ = monitor_persona_observation(child_);
    }

    void unchanged() const
    {
        require(&look_ == &presentation_.opening() && &look_.silhouette_art == &presentation_.opening().silhouette_art &&
            look_.sprite_sheet == geometry_.sprite_sheet && look_.fill_frame == geometry_.fill_frame &&
            look_.silhouette == geometry_.silhouette && look_.adult_height == geometry_.adult_height &&
            look_.child_height == geometry_.child_height &&
            look_.silhouette_art.pen_width == geometry_.silhouette_art.pen_width &&
            look_.silhouette_art.shade_width == geometry_.silhouette_art.shade_width &&
            sheet_ == app_.render_resources()->sprite_sheet(look_.sprite_sheet) &&
            &frame_ == &sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame)),
            "Silhouette colour edit changed borrowed record/resource identity, body or geometry");
        require(adult_before_ == monitor_persona_observation(adult_) && child_before_ == monitor_persona_observation(child_) &&
            pose_.sigh == 0.0f && pose_.stand == 0.0f && pose_.leave == 0.0f &&
            adult_screen_.x == 0.0f && adult_screen_.y == 0.0f && adult_screen_.width == 1280.0f && adult_screen_.height == 720.0f &&
            child_screen_.x == 0.0f && child_screen_.y == 0.0f && child_screen_.width == 640.0f && child_screen_.height == 360.0f &&
            child_camera_.translation == Vector2F(-800, -40) && child_camera_.scale == 1.0f,
            "Silhouette drawing/command changed fixed caller persona, pose, screen or camera");
    }

    Frame actual(const Colour& background)
    {
        unchanged(); std::srand(28571); const int expected = std::rand(); std::srand(28571);
        const auto draw = [&](DrawList& list)
        { person_.draw(list, adult_screen_, adult_, pose_); list.set_camera(child_camera_);
          person_.draw(list, child_screen_, child_, pose_); };
        Frame result = render(background, draw);
        require(result.pixels == render(background, draw).pixels, "Frozen adult/child silhouette changed on repeat");
        require(std::rand() == expected, "Silhouette drawing consumed gameplay RNG");
        unchanged(); outside_footprints(result, background); return result;
    }

    Frame reference(const Colour& background, const SilhouetteColourPalette& colours,
        SilhouetteColourReference control = SilhouetteColourReference::correct)
    {
        // The target performs and quantizes each ordered raw submission. No
        // Silhouette/CrudeDrawing helper or final-float compositing oracle.
        Frame result = render(background, [&](DrawList& list)
        {
            list.draw_sprite(sheet_->texture(), frame_.source_rectangle(), { 20, 20, 3, 3 },
                Colour(255, 255, 255), 0.0f, frame_.origin(), SpriteFlip::none, 0.0f);
            list.draw_sprite(sheet_->texture(), frame_.source_rectangle(), { 24, 20, 3, 3 },
                Colour(0, 0, 0), 0.0f, frame_.origin(), SpriteFlip::none, 0.0f);
            const auto fill = [&](size_t role, const Colour& tint)
            { const RectangleI area = patch(role);
              list.draw_sprite(sheet_->texture(), frame_.source_rectangle(),
                RectangleF(static_cast<float>(area.x), static_cast<float>(area.y), 3.0f, 3.0f), tint,
                0.0f, frame_.origin(), SpriteFlip::none, 0.0f); };
            fill(1, geometry_.silhouette); fill(2, geometry_.silhouette);
            const std::array<Colour, 3> authored{ colours.ink, colours.rim, colours.shade };
            for (size_t role = 0; role < authored.size(); ++role)
            {
                const Colour& value = authored[role];
                const float divisor = role == 2 && control != SilhouetteColourReference::full_shade_alpha ? 4.0f : 1.0f;
                const float alpha = value.a / divisor;
                const float rgb = control == SilhouetteColourReference::straight ? 1.0f :
                    control == SilhouetteColourReference::squared_base ? value.a * value.a / divisor : alpha;
                const Colour tint(value.r * rgb, value.g * rgb, value.b * rgb, alpha);
                const int count = role == 0 ? (control == SilhouetteColourReference::one_ink ? 1 : 2) :
                    role == 1 ? 1 : (control == SilhouetteColourReference::one_shade ? 1 : 4);
                for (int pass = 0; pass < count; ++pass) { fill(role, tint); }
            }
        });
        require(monitor_rgb(result, 21, 21) == 0xFFFFFFU && monitor_rgb(result, 25, 21) == 0,
            "Silhouette raw oracle frame failed white/opaque-black resource controls");
        return result;
    }

    static RectangleI patch(size_t role)
    {
        // build_sprite_quad truncates every destination edge before rasterizing.
        // These adjusted centres keep all nine pixels inside the declared writes.
        constexpr std::array<Vector2I, 3> centres{ Vector2I(963, 690), Vector2I(932, 627), Vector2I(959, 679) };
        return { centres[role].x - 1, centres[role].y - 1, 3, 3 };
    }
    static bool patch_equal(const Frame& a, const Frame& b, size_t role)
    {
        const RectangleI area = patch(role);
        for (int y = area.y; y < area.bottom(); ++y) { for (int x = area.x; x < area.right(); ++x)
            { const size_t at = (static_cast<size_t>(y) * static_cast<size_t>(a.width) + static_cast<size_t>(x)) * 4;
              if (!same_pixel(a, b, at)) { return false; } } }
        return true;
    }
    static void check_patches(const Frame& actual, const Frame& expected,
        const std::filesystem::path& output, const std::string& state)
    {
        for (size_t role = 0; role < 3; ++role)
        {
            if (!patch_equal(actual, expected, role))
            {
                save(compact(actual), output / ("silhouette-patch-failure-" + state + "-actual.ppm"));
                save(compact(expected), output / ("silhouette-patch-failure-" + state + "-expected.ppm"));
                const RectangleI failed = patch(role);
                std::cerr << "SILHOUETTE PATCH " << state << " role=" << role << " centre=" << failed.x + 1 << ',' << failed.y + 1
                    << " actualRGB=" << monitor_rgb(actual, failed.x + 1, failed.y + 1)
                    << " expectedRGB=" << monitor_rgb(expected, failed.x + 1, failed.y + 1) << '\n';
                require(false, "Actual silhouette mug patch failed independent ordered channel writes");
            }
            const RectangleI area = patch(role); const std::uint32_t centre = monitor_rgb(actual, area.x + 1, area.y + 1);
            for (int y = area.y; y < area.bottom(); ++y) { for (int x = area.x; x < area.right(); ++x)
                { require(monitor_rgb(actual, x, y) == centre, "Proposed silhouette oracle patch straddles a raster boundary"); } }
        }
    }
    static void nonzero_roles(const Frame& actual, const Frame& transparent)
    {
        for (size_t role = 0; role < 3; ++role)
        { require(!patch_equal(actual, transparent, role), "Silhouette colour role has no measured patch delta"); }
        require(foreground_crop(actual, { 1010, 220, 220, 240 }, 0).pixels !=
            foreground_crop(transparent, { 1010, 220, 220, 240 }, 0).pixels,
            "Child hair panel has no measured aggregate colour delta");
    }
    static void alpha0_body(const Frame& image, const Colour& background)
    {
        const std::uint32_t under = (static_cast<std::uint32_t>(std::round(background.r * 255)) << 16) |
            (static_cast<std::uint32_t>(std::round(background.g * 255)) << 8) |
            static_cast<std::uint32_t>(std::round(background.b * 255));
        require(monitor_rgb(image, 963, 690) == under && monitor_rgb(image, 932, 627) == 0x0E0E0EU &&
            monitor_rgb(image, 959, 679) == 0x0E0E0EU && monitor_rgb(image, 640, 400) == 0x0E0E0EU &&
            monitor_rgb(image, 1120, 285) == 0x0E0E0EU,
            "Transparent silhouette art failed exterior underlay or erased/recoloured unchanged adult/child body");
    }
    static Frame compact(const Frame& source)
    {
        // Native pixel montage: adult right shoulder/mug, then small child hair.
        Frame result{ 440, 240, {} }; result.pixels.reserve(440 * 240 * 4);
        for (int y = 0; y < 240; ++y)
        { for (const Vector2I origin : { Vector2I(780, 480), Vector2I(1010, 220) })
            { const size_t at = (static_cast<size_t>(origin.y + y) * static_cast<size_t>(source.width) +
                  static_cast<size_t>(origin.x)) * 4;
              result.pixels.insert(result.pixels.end(), source.pixels.begin() + at, source.pixels.begin() + at + 220 * 4); } }
        return result;
    }
    size_t readbacks() const { return readbacks_; }

private:
    static void outside_footprints(const Frame& image, const Colour& background)
    {
        const auto in = [](int x, int y, const RectangleI& r)
        { return x >= r.x && x < r.right() && y >= r.y && y < r.bottom(); };
        const std::uint32_t under = (static_cast<std::uint32_t>(std::round(background.r * 255)) << 16) |
            (static_cast<std::uint32_t>(std::round(background.g * 255)) << 8) |
            static_cast<std::uint32_t>(std::round(background.b * 255));
        for (int y = 0; y < image.height; ++y) { for (int x = 0; x < image.width; ++x)
            { if (!in(x, y, { 300, 290, 700, 430 }) && !in(x, y, { 1000, 220, 240, 240 }))
                { require(monitor_rgb(image, x, y) == under, "Silhouette output escaped fixed adult/child footprints"); } } }
    }
    Frame render(const Colour& background, const std::function<void(DrawList&)>& draw)
    {
        const Canvas canvas(app_.render_resources(), background); Renderer& renderer = *app_.renderer();
        renderer.begin_frame(); renderer.set_view_count(1); DrawList list = renderer.view(0);
        list.set_viewport(app_.viewport_manager()->fullscreen_viewport()); list.set_camera(Camera::DEFAULT_CAMERA);
        list.set_filter(TextureFilter::point); canvas.draw(list, { 0, 0, 1280, 720 }); draw(list);
        ++readbacks_; return read(renderer);
    }
    Application& app_; const Presentation& presentation_; const OpeningPresentation& look_; OpeningPresentation geometry_;
    Silhouette person_; const SpriteSheet* sheet_; const SpriteFrame& frame_;
    MonitorPersona adult_, child_; SilhouettePose pose_; std::vector<float> adult_before_, child_before_;
    const RectangleF adult_screen_{ 0, 0, 1280, 720 }, child_screen_{ 0, 0, 640, 360 };
    const Camera child_camera_{ Vector2F(-800, -40), 1.0f }; size_t readbacks_ = 0;
};

SilhouetteColourPalette silhouette_colour_palette(const SilhouetteArtPresentation& art)
{ return { art.ink, art.rim, art.shade }; }
void silhouette_colour_set(SilhouetteArtPresentation& art, const SilhouetteColourPalette& colours)
{ art.ink = colours.ink; art.rim = colours.rim; art.shade = colours.shade; }

void silhouette_colour_consumer_checks(Application& app, const Presentation& presentation, const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player); app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    Presentation local = presentation; auto& art = const_cast<OpeningPresentation&>(local.opening()).silhouette_art;
    const SilhouetteColourPalette original = silhouette_colour_palette(art);
    require(original.ink == Colour(0, 0, 0) && original.rim == Colour(52, 52, 52) && original.shade == Colour(8, 8, 8),
        "Silhouette proof lost authored default art colours");
    const std::array<SilhouetteColourPalette, 4> palettes{ original,
        SilhouetteColourPalette{ Colour(224, 64, 128), Colour(48, 192, 96), Colour(32, 96, 192) },
        SilhouetteColourPalette{ Colour(224, 64, 128, 128), Colour(48, 192, 96, 96), Colour(32, 96, 192, 128) },
        SilhouetteColourPalette{ Colour(165, 50, 214, 0), Colour(35, 199, 161, 0), Colour(128, 85, 238, 0) } };
    constexpr std::array<const char*, 4> names{ "default", "rgb", "rgba", "alpha0" };
    const std::array<Colour, 2> backgrounds{ Colour(0, 0, 0), Colour(32, 64, 96) };
    std::array<std::array<Frame, 2>, 4> frames{}; size_t rows = 0, crops = 0, controls = 0;
    SilhouetteColourProbe probe(app, local); std::ofstream records(output / "silhouette-colour-consumers.csv");
    records << "state,background,raw_patch_channels,uniform_patch_interiors,frozen_inputs_resources_rng,body_outside_scope,scope\n";
    const auto row = [&](const char* state, size_t background)
    { records << state << ',' << (background == 0 ? "black" : "nonblack") << ",true,true,true,true,consumer-only\n"; ++rows; };
    for (size_t p = 0; p < palettes.size(); ++p)
    {
        silhouette_colour_set(art, palettes[p]);
        for (size_t b = 0; b < backgrounds.size(); ++b)
        {
            Frame actual = probe.actual(backgrounds[b]);
            SilhouetteColourProbe::check_patches(actual, probe.reference(backgrounds[b], palettes[p]),
                output, std::string(names[p]) + (b == 0 ? "-black" : "-nonblack"));
            if (p == 2)
            {
                for (const auto control : { SilhouetteColourReference::straight, SilhouetteColourReference::squared_base,
                    SilhouetteColourReference::one_ink, SilhouetteColourReference::one_shade, SilhouetteColourReference::full_shade_alpha })
                {
                    const Frame wrong = probe.reference(backgrounds[b], palettes[p], control);
                    const size_t role = control == SilhouetteColourReference::one_ink ? 0 : 2;
                    require(!SilhouetteColourProbe::patch_equal(actual, wrong, role), "Silhouette fixture cannot reject its declared alpha/pass-count control");
                    if (control == SilhouetteColourReference::straight || control == SilhouetteColourReference::squared_base)
                    { for (size_t channel_role = 0; channel_role < 3; ++channel_role)
                        { require(!SilhouetteColourProbe::patch_equal(actual, wrong, channel_role), "Silhouette alpha control failed to discriminate every colour role"); } }
                    ++controls;
                }
            }
            if (p == 3) { SilhouetteColourProbe::alpha0_body(actual, backgrounds[b]); }
            row(names[p], b); save(SilhouetteColourProbe::compact(actual), output /
                (std::string("silhouette-colour-") + names[p] + (b == 0 ? "-black.ppm" : "-nonblack.ppm"))); ++crops;
            frames[p][b] = std::move(actual);
        }
    }
    for (size_t b = 0; b < backgrounds.size(); ++b)
    {
        SilhouetteColourProbe::nonzero_roles(frames[1][b], frames[3][b]);
        SilhouetteColourProbe::nonzero_roles(frames[2][b], frames[3][b]);
        silhouette_colour_set(art, original);
        require(probe.actual(backgrounds[b]).pixels == frames[0][b].pixels, "Silhouette exact default raster did not restore");
        row("restored-default", b);
    }
    require(records.good() && probe.readbacks() == 38 && rows == 10 && crops == 8 && controls == 10,
        "Silhouette shared proof exceeded approved exact budgets");
    std::ofstream summary(output / "silhouette-colour-consumer-summary.json");
    summary << "{\"scope\":\"consumer-only\",\"readbacks\":38,\"csv_rows\":10,\"crops\":8,\"controls\":10,"
        "\"player_updates\":0,\"level_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,"
        "\"patch_roles\":3,\"patch_width\":3,\"patch_height\":3,\"ink_passes\":2,\"rim_passes\":1,\"shade_passes\":4,"
        "\"ink_sample_x\":963,\"ink_sample_y\":690,\"rim_sample_x\":932,\"rim_sample_y\":627,\"shade_sample_x\":959,\"shade_sample_y\":679,"
        "\"event_fade\":false,\"child_scope\":\"aggregate-colour-adoption\"}\n";
    require(summary.good(), "Cannot write silhouette shared summary");
    std::cout << "PASS silhouette colours:38 readbacks/10 rows/8 compact440x240 crops/10 controls; independent ordered mug patch channels, "
        "actual uniform interiors, unchanged body fill, child aggregate adoption and exact default restoration; zero simulation/input/time updates.\n";
}

#if COLOURWARS_DEV_TOOLS
void silhouette_tune_colour_checks(Application& app, Registry<LevelLoadedInfo>& levels,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& content, const std::filesystem::path& output)
{
    const auto root = output / "silhouette-colour-tune-scratch", source = root / "source", staged = root / "staged";
    std::filesystem::create_directories(source); std::filesystem::create_directories(staged);
    constexpr std::array<const char*, 2> files{ "tuning.json", "presentation.json" }; std::array<std::string, 2> real_bytes;
    for (size_t i = 0; i < files.size(); ++i)
    { real_bytes[i] = tune_colour_bytes(content / files[i]); tune_colour_write(source / files[i], real_bytes[i]); tune_colour_write(staged / files[i], real_bytes[i]); }
    const NestedSaveProfile profile(root / "profile"); const TuneColourTemp temporary(root / "temp");
    struct RestoreSilhouetteContent
    { GameTuning& tuning; Presentation& presentation; GameTuning old_tuning; Presentation old_presentation;
      ~RestoreSilhouetteContent() { tuning = old_tuning; presentation = old_presentation; } }
        restore{ tuning, presentation, tuning, presentation };
    app.viewport_manager()->set_layout(ScreenLayout::one_player); app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    Save preferences; PlaySettings play; play.music_volume = 17; play.sfx_volume = 83; play.aim_sensitivity = 4;
    play.view_shake = false; play.aim_marker = true; play.reduce_effects = true;
    play.sticks[2].movement_deadzone = 25; play.sticks[2].aim_deadzone = 70;
    InputBindings bindings; bindings.keyboard_jump = Key::j;
    require(preferences.set_play_settings_and_save(play) && preferences.set_input_bindings_and_save(bindings), "Cannot seed private silhouette preferences");
    const auto save_path = root / "profile/ColourWars/save_data.json"; const std::string saved_preferences = tune_colour_bytes(save_path);
    const auto preference_values = [&]
    {
        const auto& p = preferences.play_settings();
        std::vector<int> values{ static_cast<int>(preferences.resolution()), preferences.fullscreen() ? 1 : 0,
            static_cast<int>(preferences.team_palette()), p.aim_sensitivity, p.music_volume, p.sfx_volume,
            p.view_shake ? 1 : 0, p.aim_marker ? 1 : 0, p.reduce_effects ? 1 : 0, static_cast<int>(preferences.input_bindings().keyboard_jump) };
        for (const auto& stick : p.sticks) { values.push_back(stick.movement_deadzone); values.push_back(stick.aim_deadzone); } return values;
    };
    const auto saved_values = preference_values(); StageList stages;
    for (const char* name : { "movement_course", "close_quarters", "king_of_the_hill", "turbulence" })
    { const auto* info = levels.get(name); stages.push_back({ name, info->display_name(), info->icon_frame(), info->include_in_random() }); }
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_level_infos(&levels); data.set_stages(&stages);
    data.set_tuning(&tuning); data.set_presentation(&presentation); DebugDirector director(&tuning, &presentation);
    data.set_debug(&director); auto& impl = *director.impl(); impl.set_content_directories(source, staged);
    impl.set_user_directory(root / "user"); impl.set_script_directories({ root / "scripts" }); director.attach(&app, &data);
    impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output / "silhouette-colour-tune-commands.log").string()), "Cannot retain silhouette transcript");
    SilhouetteColourProbe probe(app, presentation);
    const SilhouetteColourPalette original = silhouette_colour_palette(presentation.opening().silhouette_art);
    const SilhouetteColourPalette rgba{ Colour(224, 64, 128, 128), Colour(48, 192, 96, 96), Colour(32, 96, 192, 128) };
    const SilhouetteColourPalette zero{ Colour(165, 50, 214, 0), Colour(35, 199, 161, 0), Colour(128, 85, 238, 0) };
    std::string expected_saved = real_bytes[1];
    for (const auto& token : { std::pair{ "opening.silhouette_art.ink", "E0408080" },
        std::pair{ "opening.silhouette_art.rim", "30C06060" }, std::pair{ "opening.silhouette_art.shade", "2060C080" } })
    { JsonSpanDocument document(expected_saved, "private silhouette source"); const JsonLeaf* leaf = document.find(token.first);
      require(leaf != nullptr, "Silhouette source lacks exact existing colour token"); expected_saved = document.replaced(*leaf, std::string("\"") + token.second + "\""); }
    bool persisted = false; size_t commands = 0, rows = 0;
    const auto unchanged = [&]
    {
        probe.unchanged(); require(!impl.can_apply() && impl.request().empty() && !impl.in_match() &&
            tune_colour_bytes(save_path) == saved_preferences && preference_values() == saved_values,
            "Silhouette command changed private preferences/Jump J/default display or queued match work");
        for (size_t i = 0; i < files.size(); ++i)
        { const auto& expected = i == 1 && persisted ? expected_saved : real_bytes[i];
          require(tune_colour_bytes(content / files[i]) == real_bytes[i] && tune_colour_bytes(source / files[i]) == expected &&
              tune_colour_bytes(staged / files[i]) == expected, "Silhouette command changed real content or private bytes beyond three tokens"); }
    };
    const auto command = [&](const std::string& line)
    {
        const size_t before = impl.log().lines().size(); std::srand(28571); const int expected = std::rand(); std::srand(28571);
        const auto status = impl.execute(line); ++commands;
        require(status.ok && std::rand() == expected, "Actual silhouette command failed or consumed gameplay RNG");
        if (line.starts_with("tune "))
        { const auto& log = impl.log().lines(); require(log.size() > before && log.back().severity == DebugSeverity::info &&
              log.back().text.starts_with("tune: presentation.opening.silhouette_art.") && log.back().text.ends_with("(live)"),
              "Silhouette command lost exact live adoption scope"); }
    };
    const std::array<Colour, 2> backgrounds{ Colour(0, 0, 0), Colour(32, 64, 96) };
    std::ofstream records(output / "silhouette-colour-tune.csv");
    records << "state,background,raw_patch_channels,uniform_patch_interiors,frozen_inputs_resources_rng,private_bytes,adoption\n";
    std::array<std::vector<unsigned char>, 2> previous_child;
    std::array<Frame, 2> saved_raster{};
    const auto check = [&](const char* state, const SilhouetteColourPalette& expected)
    {
        unchanged(); const auto& art = presentation.opening().silhouette_art;
        require(art.ink == expected.ink && art.rim == expected.rim && art.shade == expected.shade, "Silhouette command lost exact channels or sibling retention");
        for (size_t b = 0; b < backgrounds.size(); ++b)
        { const Frame actual = probe.actual(backgrounds[b]); SilhouetteColourProbe::check_patches(actual, probe.reference(backgrounds[b], expected),
              output, std::string(state) + (b == 0 ? "-black" : "-nonblack"));
          auto child_pixels = foreground_crop(actual, { 1010, 220, 220, 240 }, 0).pixels;
          require(previous_child[b].empty() || child_pixels != previous_child[b], "Child panel did not adopt a changed live colour state");
          previous_child[b] = std::move(child_pixels);
          if (std::string_view(state) == "restored-ink") { saved_raster[b] = actual; }
          if (std::string_view(state) == "reset-all-saved" || std::string_view(state) == "final-reset-saved")
          { require(actual.pixels == saved_raster[b].pixels, "Saved silhouette reset did not restore exact adult/child raster"); }
          if (expected.ink.a == 0.0f && expected.rim.a == 0.0f && expected.shade.a == 0.0f)
          { SilhouetteColourProbe::alpha0_body(actual, backgrounds[b]); }
          records << state << ',' << (b == 0 ? "black" : "nonblack") << ",true,true,true,true,live\n"; ++rows; }
        unchanged();
    };
    SilhouetteColourPalette expected = original; check("baseline", expected);
    command("tune presentation.opening.silhouette_art.ink E0408080"); expected.ink = rgba.ink; check("ink-rgba", expected);
    command("tune presentation.opening.silhouette_art.rim 30C06060"); expected.rim = rgba.rim; check("rim-rgba", expected);
    command("tune presentation.opening.silhouette_art.shade 2060C080"); expected.shade = rgba.shade; check("shade-rgba", expected);
    command("tune.reset presentation.opening.silhouette_art.ink"); expected.ink = original.ink; check("reset-ink-siblings", expected);
    command("tune presentation.opening.silhouette_art.ink E0408080"); expected.ink = rgba.ink; check("restored-ink", expected);
    const TuneDocument* document = impl.tune_document("presentation");
    require(document != nullptr && document->edits().size() == 3 && document->edits().contains("opening.silhouette_art.ink") &&
        document->edits().contains("opening.silhouette_art.rim") && document->edits().contains("opening.silhouette_art.shade"),
        "Silhouette private save ledger differs from exactly three allowed tokens");
    command("tune.save presentation"); persisted = true; unchanged();
    require(impl.tune_document("presentation")->edits().empty(), "Three-token silhouette save failed to rebase");
    command("tune presentation.opening.silhouette_art.ink A532D600"); expected.ink = zero.ink; check("ink-alpha0", expected);
    command("tune presentation.opening.silhouette_art.rim 23C7A100"); expected.rim = zero.rim; check("rim-alpha0", expected);
    command("tune presentation.opening.silhouette_art.shade 8055EE00"); expected.shade = zero.shade; check("all-alpha0", expected);
    command("tune.reset presentation.opening.silhouette_art.rim"); expected.rim = rgba.rim; check("reset-rim-siblings", expected);
    command("tune.reset presentation"); expected = rgba; check("reset-all-saved", expected);
    command("tune presentation.opening.silhouette_art.rim RED"); expected.rim = Colour(255, 0, 0); check("named-rim", expected);
    command("tune.reset presentation"); expected = rgba; check("final-reset-saved", expected);
    require(impl.tune_document("presentation")->edits().empty() && impl.tune_document("tuning")->edits().empty(), "Silhouette final reset retained edits");
    require(records.good() && commands == 13 && rows == 26 && probe.readbacks() == 78, "Silhouette Debug proof exceeded approved exact budgets");
    std::ofstream summary(output / "silhouette-colour-tune-summary.json");
    summary << "{\"scope\":\"debug-validated-commands\",\"commands\":13,\"readbacks\":78,\"csv_rows\":26,\"crops\":0,\"saved_tokens\":3,"
        "\"player_updates\":0,\"level_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,\"event_fade\":false}\n";
    require(summary.good(), "Cannot write silhouette Debug summary");
    std::cout << "PASS silhouette tune:13 actual commands/78 readbacks/26 rows; three-token private save/rebase/reset, "
        "sibling retention, fixed caller inputs/body/resources/RNG and unchanged private PlaySettings/Jump J/default display. Zero simulation/input/time updates.\n";
}
#endif
