// Separate sibling proof; the committed Cursor23 ink/fill helper stays intact.
// Full-shape reference proves adoption only. Independent selected alpha samples
// submit each ordered white-frame write, preserving target UNORM quantization.
enum class CursorMaterialControl { shade_straight, shade_squared, shadow_straight, shadow_squared, pair_straight };
using CursorMaterialControls = std::array<std::optional<Frame>, 5>;

class CursorMaterialProbe
{
public:
    CursorMaterialProbe(Application& app, Presentation& presentation) : app_(app), presentation_(presentation),
        look_(const_cast<OpeningPresentation&>(presentation.opening())), baseline_(look_), expected_(baseline_),
        cursor_(&look_, app.render_resources()), reference_(&expected_, app.render_resources()), original_(&baseline_, app.render_resources()),
        sheet_(app.render_resources()->sprite_sheet(baseline_.sprite_sheet)), frame_(sheet_->sprite_frame(sheet_->resolve_sprite_frame(baseline_.fill_frame)))
    {
        const auto& a = baseline_.cursor_art;
        require(baseline_.cursor_size == 0.045f && a.shadow_offset == 0.08f && a.click_duration == 0.18f && a.click_reach == 0.3f &&
            a.ink == Colour(0, 0, 0) && a.fill == Colour(243, 243, 243) && a.shade == Colour(180, 180, 180) && a.shadow == Colour(0, 0, 0, 102) &&
            frame_.origin() == Vector2F::ZERO && frame_.source_rectangle() == RectangleI(32, 0, 4, 4), "Cursor material sample dependencies changed; re-audit");
    }
    void set(const Colour& shade, const Colour& shadow) { look_.cursor_art.shade = shade; look_.cursor_art.shadow = shadow; }
    void expect(const Colour& shade, const Colour& shadow)
    {
        expected_ = baseline_; expected_.cursor_art.shade = shade; expected_.cursor_art.shadow = shadow;
        require(look_.cursor_art.shade == shade && look_.cursor_art.shadow == shadow, "Cursor material adopted different literal RGBA channels"); unchanged();
    }
    void unchanged() const
    {
        const auto fixed = [&](const OpeningPresentation& look)
        { const auto& a = look.cursor_art; const auto& b = baseline_.cursor_art;
          return look.sprite_sheet == baseline_.sprite_sheet && look.fill_frame == baseline_.fill_frame && look.cursor_size == baseline_.cursor_size &&
              a.ink == b.ink && a.fill == b.fill && a.shadow_offset == b.shadow_offset && a.click_duration == b.click_duration && a.click_reach == b.click_reach; };
        require(&presentation_.opening() == &look_ && &presentation_.opening().cursor_art == &look_.cursor_art && fixed(look_) && fixed(expected_) &&
            sheet_ == app_.render_resources()->sprite_sheet(look_.sprite_sheet) && &frame_ == &sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame)),
            "Cursor material edit changed borrowed identity, captured resources, ink/fill or geometry");
    }
    Frame actual(const Colour& background, bool click, bool reduced = false, bool duplicate = false)
    {
        unchanged(); std::srand(8451); const int expected = std::rand(); std::srand(8451);
        const auto draw = [&](DrawList& list) { cursor_.draw(list, { 640, 240 }, 720.0f, click ? 0.09f : -1.0f, reduced); };
        Frame result = render(background, draw);
        if (duplicate) { require(result.pixels == render(background, draw).pixels, "Frozen cursor material draw changed pixels"); }
        require(std::rand() == expected, "Cursor material draw consumed global RNG"); unchanged(); return result;
    }
    Frame adoption_reference(const Colour& background, bool click, bool original = false)
    { return render(background, [&](DrawList& list) { (original ? original_ : reference_).draw(list, { 640, 240 }, 720.0f, click ? 0.09f : -1.0f, false); }); }
    Frame blank(const Colour& background) { return render(background, [](DrawList&) {}); }
    Frame raw(const Colour& background, const Colour& shade, const Colour& shadow, std::optional<CursorMaterialControl> control = {})
    {
        const auto tint = [&](const Colour& value, bool is_shade)
        {
            const bool straight = control == CursorMaterialControl::pair_straight || (is_shade ? control == CursorMaterialControl::shade_straight : control == CursorMaterialControl::shadow_straight);
            const bool squared = is_shade ? control == CursorMaterialControl::shade_squared : control == CursorMaterialControl::shadow_squared;
            const float rgb = straight ? 1.0f : squared ? value.a * value.a : value.a;
            return Colour(value.r * rgb, value.g * rgb, value.b * rgb, value.a);
        };
        return render(background, [&](DrawList& list)
        {
            const auto patch = [&](float x, float y, const Colour& colour)
            { list.draw_sprite(sheet_->texture(), frame_.source_rectangle(), { x, y, 1, 1 }, colour, 0.0f, frame_.origin(), SpriteFlip::none, 0.0f); };
            patch(20, 20, Colour(255, 255, 255)); patch(24, 20, Colour(0, 0, 0));
            // At649,262 the shifted row12 W run is below foreground row14 S.
            // Never collapse these writes to one final-float composite.
            patch(649, 262, tint(shadow, false)); patch(649, 262, tint(shade, true));
            // At656,273 only the shifted final-row run survives.
            patch(656, 273, tint(shadow, false));
        });
    }
    void sample_rows(std::ofstream& rows, const char* state, const char* background, const Frame& actual, const Frame& expected,
        const char* scope, const CursorMaterialControls& controls = {}) const
    {
        for (size_t site = 0; site < 2; ++site)
        {
            const int x = site == 0 ? 649 : 656, y = site == 0 ? 262 : 273;
            const auto value = monitor_rgb(actual, x, y), raw_rgb = monitor_rgb(expected, x, y);
            require(value == raw_rgb, "Cursor shade/shadow differs from independent ordered base-alpha writes");
            rows << state << ',' << background << ',' << (site == 0 ? "shade-over-shadow" : "shadow-only") << ',' << value << ',' << raw_rgb << ",true";
            for (size_t index = 0; index < controls.size(); ++index)
            {
                if (!controls[index] || (site == 1 && index < 2)) { rows << ",not-sampled"; continue; }
                const auto wrong = monitor_rgb(*controls[index], x, y);
                require(value != wrong, "Cursor material sample cannot reject its targeted wrong-alpha control"); rows << ',' << wrong;
            }
            rows << ',' << scope << '\n';
        }
        require(monitor_rgb(expected, 20, 20) == 0xFFFFFFU && monitor_rgb(expected, 24, 20) == 0, "Cursor material raw frame failed white/black controls");
    }
    void check_siblings(const Frame& rest, const Frame& click) const
    {
        require(foreground_crop(rest, { 640, 240, 28, 38 }, 0).pixels == foreground_crop(click, { 640, 240, 28, 38 }, 0).pixels && rest.pixels != click.pixels,
            "Cursor material changed arrow during click or removed retained ink/fill ticks");
        require(monitor_rgb(rest, 640, 240) == 0 && monitor_rgb(rest, 641, 243) == 0xF3F3F3U &&
            monitor_rgb(click, 625, 237) == 0 && monitor_rgb(click, 632, 239) == 0xF3F3F3U,
            "Cursor material changed unselected opaque ink/fill body or click channels");
    }
    size_t readbacks() const { return readbacks_; }
    static RectangleI panel() { return { 612, 212, 72, 80 }; }
private:
    Frame render(const Colour& background, const std::function<void(DrawList&)>& draw)
    {
        const Canvas canvas(app_.render_resources(), background); Renderer& renderer = *app_.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app_.viewport_manager()->fullscreen_viewport()); list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        canvas.draw(list, { 0, 0, 1280, 720 }); draw(list); ++readbacks_; return read(renderer);
    }
    Application& app_; Presentation& presentation_; OpeningPresentation& look_; const OpeningPresentation baseline_; OpeningPresentation expected_;
    ScreenCursor cursor_, reference_, original_; const SpriteSheet* sheet_; const SpriteFrame& frame_; size_t readbacks_ = 0;
};

void cursor_material_consumer_checks(Application& app, const Presentation& presentation, const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player); app.resolution_manager()->set_resolution_exactly({ 1280, 720 }); std::ignore = app.renderer()->window_size_changed(1280, 720);
    Presentation local = presentation; CursorMaterialProbe probe(app, local);
    struct Palette { const char* name; Colour shade, shadow; const char* control; };
    const std::array<Palette, 4> palettes{{ { "default", Colour(180, 180, 180), Colour(0, 0, 0, 102), "unchanged-default-adoption" },
        { "rgb", Colour(53, 188, 225), Colour(145, 65, 194), "distinct-opaque-materials" },
        { "rgba128", Colour(32, 96, 192, 128), Colour(192, 96, 32, 128), "per-field-straight-squared-controls" },
        { "alpha0", Colour(165, 50, 214, 0), Colour(42, 144, 64, 0), "selected-zero-retains-ink-fill-ticks" } }};
    const std::array<Colour, 2> backgrounds{ Colour(0, 0, 0), Colour(32, 64, 96) };
    const std::array<Frame, 2> blank{ probe.blank(backgrounds[0]), probe.blank(backgrounds[1]) }; std::array<Frame, 2> defaults;
    size_t rows = 0, sample_rows = 0, crops = 0, negative_frames = 0;
    std::ofstream records(output / "cursor-material-consumers.csv"), samples(output / "cursor-material-samples.csv");
    records << "state,background,rest_match,click_match,reduced_rest,ordered_samples,ink_fill_ticks_rng,control,scope\n";
    samples << "state,background,sample,actual_rgb,expected_rgb,matched,shade_straight_rgb,shade_squared_rgb,shadow_straight_rgb,shadow_squared_rgb,pair_straight_rgb,scope\n";
    for (size_t palette = 0; palette < palettes.size(); ++palette)
    {
        const auto& entry = palettes[palette]; probe.set(entry.shade, entry.shadow); probe.expect(entry.shade, entry.shadow);
        for (size_t background = 0; background < backgrounds.size(); ++background)
        {
            const char* underlay = background == 0 ? "black" : "nonblack";
            const Frame rest = probe.actual(backgrounds[background], false, false, true), click = probe.actual(backgrounds[background], true);
            require(rest.pixels == probe.actual(backgrounds[background], true, true).pixels &&
                rest.pixels == probe.adoption_reference(backgrounds[background], false, palette == 0).pixels &&
                click.pixels == probe.adoption_reference(backgrounds[background], true, palette == 0).pixels, "Cursor material reduced/adoption parity failed");
            probe.check_siblings(rest, click); const Frame raw = probe.raw(backgrounds[background], entry.shade, entry.shadow); CursorMaterialControls controls{};
            if (palette == 2 || palette == 3)
            {
                const size_t first = palette == 2 ? 0 : 4, last = palette == 2 ? 4 : 5;
                for (size_t index = first; index < last; ++index)
                { controls[index] = probe.raw(backgrounds[background], entry.shade, entry.shadow, static_cast<CursorMaterialControl>(index)); ++negative_frames; }
            }
            probe.sample_rows(samples, entry.name, underlay, rest, raw, "consumer-only", controls); sample_rows += 2;
            if (palette == 0) { defaults[background] = rest; }
            else { require(rest.pixels != defaults[background].pixels, "Cursor material edit failed to reach the retained consumer"); }
            if (palette == 3)
            { require(rest.pixels != blank[background].pixels && monitor_rgb(rest, 649, 262) == monitor_rgb(blank[background], 649, 262) &&
                monitor_rgb(rest, 656, 273) == monitor_rgb(blank[background], 656, 273), "Cursor selected alpha0 lost ink/fill or failed to remove selected samples"); }
            records << entry.name << ',' << underlay << ",true,true,true,true,true," << entry.control << ",consumer-only\n"; ++rows;
            save(foreground_crop(click, CursorMaterialProbe::panel(), 0), output / (std::string("cursor-material-") + entry.name + '-' + underlay + ".ppm")); ++crops;
        }
    }
    require(records.good() && samples.good() && probe.readbacks() == 68 && rows == 8 && sample_rows == 16 && crops == 8 && negative_frames == 10, "Cursor material shared counters exceeded scope");
    std::ofstream summary(output / "cursor-material-consumer-summary.json");
    summary << "{\"scope\":\"consumer-only\",\"state_updates\":0,\"level_updates\":0,\"input_updates\":0,\"local_updates\":0,\"readbacks\":68,\"csv_rows\":24,\"case_rows\":8,\"sample_rows\":16,\"crops\":8,"
        "\"crop_pixels\":46080,\"negative_reference_frames\":10,\"negative_sample_comparisons\":16,\"blank_readbacks\":2,\"draw_screen_height\":720,\"viewport\":\"1280x720\",\"tip\":[640,240],\"click_age\":0.09,\"event_fade\":false,"
        "\"full_shape_reference_scope\":\"production-consumer-adoption-only\",\"alpha_reference_scope\":\"independent-ordered-raw-patches\"}\n";
    require(summary.good(), "Cannot retain cursor material summary");
    std::cout << "PASS cursor material consumers:68 readbacks/24 rows/8 crops; ordered shadow-then-S and shadow-only raw channels,per-field alpha controls,retained ink/fill/ticks,Reduce Effects,zero updates.\n";
}

#if COLOURWARS_DEV_TOOLS
// Debug adds15 real statements,144 readbacks and72 rows; no additional crops.
void cursor_material_tune_checks(Application& app, Registry<LevelLoadedInfo>& levels,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& content,
    const std::filesystem::path& output)
{
    const auto root = output / "cursor-material-tune-scratch", source = root / "source", staged = root / "staged";
    std::filesystem::create_directories(source); std::filesystem::create_directories(staged);
    constexpr std::array<const char*, 2> files{ "tuning.json", "presentation.json" };
    std::array<std::string, 2> real_bytes;
    for (size_t index = 0; index < files.size(); ++index)
    {
        real_bytes[index] = tune_colour_bytes(content / files[index]);
        tune_colour_write(source / files[index], real_bytes[index]); tune_colour_write(staged / files[index], real_bytes[index]);
    }
    const NestedSaveProfile profile(root / "profile"); const TuneColourTemp temporary(root / "temp");
    struct RestoreCursorMaterialTuneContent
    {
        GameTuning& tuning; Presentation& presentation; GameTuning original_tuning; Presentation original;
        ~RestoreCursorMaterialTuneContent() { this->tuning = this->original_tuning; this->presentation = this->original; }
    } restore{ tuning, presentation, tuning, presentation };
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    Save preferences; PlaySettings play; play.music_volume = 17; play.sfx_volume = 83;
    play.aim_sensitivity = 4; play.view_shake = false; play.aim_marker = true; play.reduce_effects = true;
    play.sticks[2].movement_deadzone = 25; play.sticks[2].aim_deadzone = 70;
    InputBindings bindings; bindings.keyboard_jump = Key::j;
    require(preferences.set_play_settings_and_save(play) && preferences.set_input_bindings_and_save(bindings),
        "Cannot seed private cursor material PlaySettings/JumpJ preferences");
    const auto save_path = root / "profile/ColourWars/save_data.json";
    const std::string saved_preferences = tune_colour_bytes(save_path);
    const auto preference_values = [&]
    {
        const PlaySettings& active = preferences.play_settings();
        std::vector<int> values{ static_cast<int>(preferences.resolution()), preferences.fullscreen() ? 1 : 0,
            static_cast<int>(preferences.team_palette()), active.aim_sensitivity, active.music_volume,
            active.sfx_volume, active.view_shake ? 1 : 0, active.aim_marker ? 1 : 0,
            active.reduce_effects ? 1 : 0, static_cast<int>(preferences.input_bindings().keyboard_jump) };
        for (const StickSettings& stick : active.sticks)
        { values.push_back(stick.movement_deadzone); values.push_back(stick.aim_deadzone); }
        return values;
    };
    const std::vector<int> initial_preferences = preference_values();
    StageList stages;
    for (const char* name : { "movement_course", "close_quarters", "king_of_the_hill", "turbulence" })
    {
        const auto* info = levels.get(name);
        stages.push_back({ name, info->display_name(), info->icon_frame(), info->include_in_random() });
    }
    GameData data; data.set_application(&app); data.set_save(&preferences);
    const StageCatalog stage_catalog = acceptance::catalog_of(levels, stages, { "close_quarters", "king_of_the_hill", "movement_course", "turbulence" }); data.set_stage_catalog(&stage_catalog); data.set_tuning(&tuning); data.set_presentation(&presentation);
    DebugDirector director(&tuning, &presentation); data.set_debug(&director); DebugDirectorImpl& impl = *director.impl();
    impl.set_content_directories(source, staged); impl.set_user_directory(root / "user");
    impl.set_script_directories({ root / "scripts" }); director.attach(&app, &data);
    impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output / "cursor-material-tune-commands.log").string()), "Cannot retain cursor command transcript");
    CursorMaterialProbe probe(app, presentation);
    const std::array<Colour, 2> backgrounds{ Colour(0, 0, 0), Colour(32, 64, 96) };
    size_t commands = 0, rows = 0, sample_rows = 0; bool persisted = false;
    JsonSpanDocument first(real_bytes[1], "private cursor source");
    const JsonLeaf* shade_leaf = first.find("opening.cursor_art.shade");
    require(shade_leaf != nullptr, "Cursor material source has no shade leaf");
    JsonSpanDocument second(first.replaced(*shade_leaf, "\"2060C080\""), "private cursor material source");
    const JsonLeaf* shadow_leaf = second.find("opening.cursor_art.shadow");
    require(shadow_leaf != nullptr, "Cursor material source has no shadow leaf");
    const std::string expected_saved = second.replaced(*shadow_leaf, "\"C0602080\"");
    const auto unchanged = [&]
    {
        probe.unchanged();
        require(!impl.can_apply() && impl.request().empty() && !impl.in_match() &&
            tune_colour_bytes(save_path) == saved_preferences && preference_values() == initial_preferences,
            "Cursor tune changed private preferences, queued work or entered a match");
        for (size_t index = 0; index < files.size(); ++index)
        {
            const std::string& expected = index == 1 && persisted ? expected_saved : real_bytes[index];
            require(tune_colour_bytes(content / files[index]) == real_bytes[index] &&
                tune_colour_bytes(source / files[index]) == expected && tune_colour_bytes(staged / files[index]) == expected,
                "Cursor tune changed actual content or private bytes beyond its two saved tokens");
        }
    };
    const auto command = [&](const std::string& line)
    {
        const size_t previous_lines = impl.log().lines().size();
        std::srand(8451); const int expected_random = std::rand(); std::srand(8451);
        const DebugStatus result = impl.execute(line); ++commands;
        require(std::rand() == expected_random, "Cursor tune command consumed match randomness");
        if (!result.ok) { std::cout << "COMMAND " << line << ": " << result.message << '\n'; }
        require(result.ok, "Validated cursor tune/reset/save command was rejected");
        if (line.starts_with("tune "))
        {
            const auto& lines = impl.log().lines();
            require(lines.size() > previous_lines && lines.back().severity == DebugSeverity::info &&
                lines.back().text.starts_with("tune: presentation.opening.cursor_art.") && lines.back().text.ends_with("(live)"),
                "Cursor colour command lost live adoption scope");
        }
        const TuneDocument* document = impl.tune_document("presentation");
        require(document != nullptr && document->edits().size() <= 2, "Cursor command edited beyond its two audited leaves");
        for (const auto& edit : document->edits())
        {
            require(edit.first == "opening.cursor_art.shade" || edit.first == "opening.cursor_art.shadow",
                "Cursor command edited an unaudited sibling");
        }
    };
    const auto pair = [&](const char* shade_word, const char* shadow_word)
    {
        command(std::string("tune presentation.opening.cursor_art.shade ") + shade_word);
        command(std::string("tune presentation.opening.cursor_art.shadow ") + shadow_word);
        const TuneDocument* document = impl.tune_document("presentation");
        require(document->edits().size() == 2 && document->edits().contains("opening.cursor_art.shade") &&
            document->edits().contains("opening.cursor_art.shadow"), "Cursor pair did not retain exactly its two audited edits");
    };
    std::ofstream records(output / "cursor-material-tune-colours.csv");
    records << "state,background,rest_match,click_match,ordered_samples,ink_fill_ticks_rng,private_bytes,adoption\n";
    std::ofstream samples(output / "cursor-material-tune-samples.csv");
    samples << "state,background,sample,actual_rgb,expected_rgb,matched,shade_straight_rgb,shade_squared_rgb,shadow_straight_rgb,shadow_squared_rgb,pair_straight_rgb,scope\n";
    const auto check = [&](const char* state, const Colour& expected_shade, const Colour& expected_shadow)
    {
        unchanged(); probe.expect(expected_shade, expected_shadow);
        for (size_t background = 0; background < backgrounds.size(); ++background)
        {
            const char* underlay = background == 0 ? "black" : "nonblack";
            const Frame rest = probe.actual(backgrounds[background], false, false, true), click = probe.actual(backgrounds[background], true);
            require(rest.pixels == probe.adoption_reference(backgrounds[background], false).pixels &&
                click.pixels == probe.adoption_reference(backgrounds[background], true).pixels,
                "Retained cursor did not adopt real validated command colours with copied-dependency shape parity");
            probe.check_siblings(rest, click);
            const Frame raw = probe.raw(backgrounds[background], expected_shade, expected_shadow);
            probe.sample_rows(samples, state, underlay, rest, raw, "debug-validated-commands");
            sample_rows += 2;
            records << state << ',' << underlay << ",true,true,true,true,true,live\n"; ++rows;
        }
        unchanged();
    };
    const Colour default_shade(180, 180, 180), default_shadow(0, 0, 0, 102), red(255, 0, 0), blue(0, 0, 255);
    const Colour rgba_shade(32, 96, 192, 128), rgba_shadow(192, 96, 32, 128);
    check("baseline", default_shade, default_shadow);
    command("tune presentation.opening.cursor_art.shade RED"); check("shade-named", red, default_shadow);
    command("tune presentation.opening.cursor_art.shadow BLUE"); check("named-pair", red, blue);
    command("tune.reset presentation.opening.cursor_art.shade");
    require(impl.tune_document("presentation")->edits().size() == 1 &&
        impl.tune_document("presentation")->edits().contains("opening.cursor_art.shadow"), "Cursor material shade reset lost its pending shadow sibling");
    check("reset-shade", default_shade, blue);
    command("tune.reset presentation.opening.cursor_art.shadow");
    require(impl.tune_document("presentation")->edits().empty(), "Cursor separate leaf resets retained edits");
    check("reset-original", default_shade, default_shadow);
    pair("35BCE1", "9141C2"); check("rgb-pair", Colour(53, 188, 225), Colour(145, 65, 194));
    pair("2060C080", "C0602080"); check("rgba-pair", rgba_shade, rgba_shadow);
    command("tune.save presentation"); persisted = true;
    require(impl.tune_document("presentation")->edits().empty() && impl.tune_document("presentation")->baseline().text() == expected_saved,
        "Cursor save failed to rebase exactly its two private tokens");
    check("saved-rgba", rgba_shade, rgba_shadow);
    pair("RED", "BLUE"); check("edited-after-save", red, blue);
    command("tune.reset presentation"); check("reset-saved-rgba", rgba_shade, rgba_shadow);
    pair("A532D600", "2A904000"); check("alpha0-pair", Colour(165, 50, 214, 0), Colour(42, 144, 64, 0));
    command("tune.reset presentation"); check("reset-alpha0-to-saved", rgba_shade, rgba_shadow);
    require(impl.tune_document("presentation")->edits().empty() && impl.tune_document("tuning")->edits().empty(),
        "Cursor final reset left pending private edits");
    unchanged();
    require(records.good() && samples.good() && commands == 15 && probe.readbacks() == 144 && rows == 24 && sample_rows == 48,
        "Cursor Debug command evidence exceeded its bounded counts");
    std::ofstream summary(output / "cursor-material-tune-summary.json");
    summary << "{\"scope\":\"debug-validated-commands\",\"state_updates\":0,\"level_updates\":0,\"input_updates\":0,\"local_updates\":0,"
        "\"draw_screen_height\":720,\"viewport\":\"1280x720\",\"tip\":[640,240],\"click_age\":0.09,\"commands\":15,\"readbacks\":144,"
        "\"csv_rows\":72,\"case_rows\":24,\"sample_rows\":48,\"crops\":0,\"saved_tokens\":2,"
        "\"full_shape_reference_scope\":\"production-consumer-adoption-only\",\"alpha_reference_scope\":\"independent-ordered-raw-patches\",\"event_fade\":false}\n";
    require(summary.good(), "Cannot retain cursor Debug command summary");
    std::cout << "PASS cursor tune colours:15 validated named/RGB/RGBA/separate-reset/save commands,144 readbacks,72 rows; "
        "two-token private save/rebase,retained resting/clicking consumers,independent ordered shadow/S samples and preserved ink/fill/preferences/RNG. "
        "Synthetic draw screen_height720,zero local/Level/State updates; no physical input or small-view/human acceptance.\n";
}
#endif
