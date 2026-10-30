// Retained RevealArt adoption plus bounded, independently layered raw samples.
// Full-card/text comparison is production-derived adoption, not an independent
// glyph/card geometry oracle. Sample sites must qualify on the actual backend.
enum class RevealColourRole { ink, paper, shade };
enum class RevealColourControl { straight, squared_base, full_airbrush, single_airbrush, no_beat, linear_beat, double_beat };
struct RevealColourPalette { Colour ink, paper, shade; };
struct RevealColourSample { const char* name; int x, y; RevealColourRole role; bool tick; };

class RevealColourProbe
{
public:
    RevealColourProbe(Application& app, Presentation& presentation) :
        app_(app), presentation_(presentation), look_(const_cast<OpeningPresentation&>(presentation.opening())),
        baseline_(look_), expected_(baseline_), wrappers_(presentation.menu_text().results),
        labels_{ wrappers_.team_a_percentage_line(L"100.0%"), wrappers_.team_b_percentage_line(L"0.0%") },
        art_(&look_, app.render_resources()), reference_(&expected_, app.render_resources()), original_(&baseline_, app.render_resources()),
        sheet_(app.render_resources()->sprite_sheet(baseline_.sprite_sheet)),
        frame_(sheet_->sprite_frame(sheet_->resolve_sprite_frame(baseline_.fill_frame))),
        font_(app.render_resources()->resolve_sprite_font(baseline_.percent_font))
    {
        const auto& a = baseline_.reveal_art;
        require(a.ink == Colour(0, 0, 0) && a.paper == Colour(240, 235, 221) && a.shade == Colour(0, 0, 0, 85) &&
            a.keyline_width == 0.035f && a.padding == 0.12f && a.shadow_offset == 0.06f &&
            a.tick_length == 0.14f && a.beat_duration == 0.18f && frame_.origin() == Vector2F::ZERO &&
            frame_.source_rectangle() == RectangleI(32, 0, 4, 4), "Reveal sampled geometry/resources changed; re-audit sites");
        // An empty string returns before the body. SPACE is a real nonempty
        // material-only route only when the actual captured font measures ZERO.
        require(app.render_resources()->measure_text(font_, L" ") == Vector2F::ZERO &&
            app.render_resources()->measure_text(font_, labels_[0]).x > 0.0f &&
            app.render_resources()->measure_text(font_, labels_[1]).y > 0.0f,
            "Reveal material SPACE must actually measure ZERO; no empty-text or glyph-pixel substitution");
    }
    void set(const RevealColourPalette& value)
    { look_.reveal_art.ink = value.ink; look_.reveal_art.paper = value.paper; look_.reveal_art.shade = value.shade; }
    void expect(const RevealColourPalette& value)
    {
        expected_ = baseline_; expected_.reveal_art.ink = value.ink; expected_.reveal_art.paper = value.paper; expected_.reveal_art.shade = value.shade;
        require(look_.reveal_art.ink == value.ink && look_.reveal_art.paper == value.paper && look_.reveal_art.shade == value.shade,
            "Reveal adopted channels differ from the literal authored triple"); unchanged();
    }
    void unchanged() const
    {
        const auto fixed = [&](const OpeningPresentation& look)
        {
            const auto& a = look.reveal_art; const auto& b = baseline_.reveal_art;
            return look.sprite_sheet == baseline_.sprite_sheet && look.fill_frame == baseline_.fill_frame && look.percent_font == baseline_.percent_font &&
                look.percent_height == baseline_.percent_height && a.keyline_width == b.keyline_width && a.padding == b.padding &&
                a.shadow_offset == b.shadow_offset && a.tick_length == b.tick_length && a.beat_duration == b.beat_duration;
        };
        const auto& text = presentation_.menu_text().results;
        require(&presentation_.opening() == &look_ && &presentation_.opening().reveal_art == &look_.reveal_art && fixed(look_) && fixed(expected_) &&
            text.team_a_percentage == wrappers_.team_a_percentage && text.team_b_percentage == wrappers_.team_b_percentage &&
            labels_[0] == wrappers_.team_a_percentage_line(L"100.0%") && labels_[1] == wrappers_.team_b_percentage_line(L"0.0%") &&
            sheet_ == app_.render_resources()->sprite_sheet(look_.sprite_sheet) && font_ == app_.render_resources()->resolve_sprite_font(look_.percent_font),
            "Reveal colours changed borrowed identity, captured resource/wrapper dependencies or caller geometry");
    }
    Frame actual(const Colour& background, float age, bool reduced = false, bool duplicate = true)
    {
        unchanged(); std::srand(28571); const int expected = std::rand(); std::srand(28571);
        const auto draw = [&](DrawList& list) { cards(list, art_, age, reduced); };
        Frame result = render(background, draw);
        if (duplicate) { require(result.pixels == render(background, draw).pixels, "Frozen reveal card draw changed pixels"); }
        require(std::rand() == expected, "Reveal drawing consumed global gameplay RNG"); unchanged(); return result;
    }
    Frame adoption_reference(const Colour& background, float age, bool original = false)
    { return render(background, [&](DrawList& list) { cards(list, original ? original_ : reference_, age, false); }); }
    Frame blank(const Colour& background) { return render(background, [](DrawList&) {}); }

    Frame raw(const Colour& background, const RevealColourPalette& value,
        std::optional<RevealColourControl> control = {}, RevealColourRole target = RevealColourRole::ink)
    {
        return render(background, [&](DrawList& list)
        {
            const auto rectangle = [&](const RectangleF& area, const Colour& tint)
            { list.draw_sprite(sheet_->texture(), frame_.source_rectangle(), area, tint, 0.0f, frame_.origin(), SpriteFlip::none, 0.0f); };
            rectangle({ 20, 20, 3, 3 }, Colour(255, 255, 255)); rectangle({ 24, 20, 3, 3 }, Colour(0, 0, 0));
            const auto patch = [&](size_t site, RevealColourRole role, const Colour& colour, float opacity, int count, bool air = false, bool tick = false)
            {
                if (control == RevealColourControl::full_airbrush && air) { opacity = 1.0f; }
                if (control == RevealColourControl::single_airbrush && air) { count = 1; }
                if (role == RevealColourRole::ink && tick)
                {
                    if (control == RevealColourControl::no_beat) { opacity = 1.0f; }
                    if (control == RevealColourControl::linear_beat) { opacity = 0.5f; }
                    if (control == RevealColourControl::double_beat) { opacity = 0.0625f; }
                }
                const float alpha = colour.a * opacity;
                const float rgb = role == target && control == RevealColourControl::straight ? 1.0f :
                    role == target && control == RevealColourControl::squared_base ? colour.a * colour.a * opacity : alpha;
                const auto& sample = samples()[site];
                const Colour tint(colour.r * rgb, colour.g * rgb, colour.b * rgb, alpha);
                for (int pass = 0; pass < count; ++pass)
                { rectangle({ static_cast<float>(sample.x), static_cast<float>(sample.y), 1.0f, 1.0f }, tint); }
            };
            // Literal ordered membership under edge truncation. This tiny
            // oracle does not call RevealArt/CrudeDrawing or reproduce glyphs.
            patch(0, RevealColourRole::ink, value.ink, 1, 1);
            patch(1, RevealColourRole::ink, value.ink, 1, 2);
            patch(2, RevealColourRole::ink, value.ink, 1, 1); patch(2, RevealColourRole::paper, value.paper, 1, 1);
            for (const size_t site : { size_t(3), size_t(5), size_t(6) })
            {
                patch(site, RevealColourRole::shade, value.shade, 1, 2);
                patch(site, RevealColourRole::ink, value.ink, 1, site == 6 ? 1 : 2);
                patch(site, RevealColourRole::paper, value.paper, 1, site == 6 ? 1 : 2);
                if (site != 3) { patch(site, RevealColourRole::shade, value.shade, 0.12f, site == 6 ? 3 : 1, true); }
            }
            patch(4, RevealColourRole::shade, value.shade, 1, 1);
            for (size_t side = 0; side < 2; ++side)
            {
                patch(7 + side * 2, RevealColourRole::ink, value.ink, 0.25f, 1, false, true);
                patch(8 + side * 2, RevealColourRole::ink, value.ink, 0.25f, 1, false, true);
                const auto& sample = samples()[8 + side * 2]; const Colour& team = teams_[side];
                rectangle({ static_cast<float>(sample.x), static_cast<float>(sample.y), 1.0f, 1.0f },
                    Colour(team.r * team.a * 0.25f, team.g * team.a * 0.25f, team.b * team.a * 0.25f, team.a * 0.25f));
            }
        });
    }
    static const std::array<RevealColourSample, 11>& samples()
    {
        static constexpr std::array<RevealColourSample, 11> result{{
            { "ink-edge", 150, 81, RevealColourRole::ink, false }, { "ink-overlap", 112, 88, RevealColourRole::ink, false },
            { "paper-edge", 150, 84, RevealColourRole::paper, false }, { "paper-overlap", 250, 100, RevealColourRole::paper, false },
            { "shadow-lip", 250, 177, RevealColourRole::shade, false }, { "airbrush-one", 250, 161, RevealColourRole::shade, false },
            { "airbrush-three", 250, 168, RevealColourRole::shade, false },
            { "tick-ink-a", 92, 266, RevealColourRole::ink, true }, { "tick-core-a", 94, 267, RevealColourRole::ink, true },
            { "tick-ink-b", 512, 266, RevealColourRole::ink, true }, { "tick-core-b", 514, 267, RevealColourRole::ink, true }
        }}; return result;
    }
    using Controls = std::array<std::optional<Frame>, 7>;
    void sample_rows(std::ofstream& records, const char* state, const char* background,
        const Frame& rest, const Frame& half, const Frame& expected, const char* scope,
        const Controls& controls = {}, RevealColourRole target = RevealColourRole::ink) const
    {
        for (size_t site = 0; site < samples().size(); ++site)
        {
            const auto& sample = samples()[site]; const auto actual = monitor_rgb(sample.tick ? half : rest, sample.x, sample.y);
            const auto raw_rgb = monitor_rgb(expected, sample.x, sample.y);
            require(actual == raw_rgb, "Reveal material/tick sample differs from independent ordered raw-alpha reference");
            records << state << ',' << background << ',' << sample.name << ',' << (sample.tick ? "half" : "rest") << ',' << actual << ',' << raw_rgb << ",true";
            for (size_t index = 0; index < controls.size(); ++index)
            {
                const bool relevant = index <= 1 ? sample.role == target : index == 2 ? site == 5 || site == 6 : index == 3 ? site == 6 : sample.tick;
                if (!controls[index] || !relevant) { records << ",not-sampled"; continue; }
                const auto wrong = monitor_rgb(*controls[index], sample.x, sample.y);
                require(actual != wrong, "Reveal sample cannot reject its targeted alpha/overlap/quadratic control"); records << ',' << wrong;
            }
            records << ',' << scope << '\n';
        }
    }
    void check_raw_resource(const Frame& raw) const
    { require(monitor_rgb(raw, 21, 21) == 0xFFFFFFU && monitor_rgb(raw, 25, 21) == 0, "Reveal raw frame is not white or opaque-black resource control"); }
    void check_guards(const Frame& rest, const Frame& half, const Frame& reduced, const Frame& expired) const
    {
        require(rest.pixels == reduced.pixels && rest.pixels == expired.pixels && rest.pixels != half.pixels,
            "Reveal reduced/expired guard lost settled cards or retained half-age ticks");
        require(foreground_crop(rest, material_panel(), 0).pixels == foreground_crop(half, material_panel(), 0).pixels,
            "ZERO-measure SPACE material card gained glyph/tick activity");
    }
    static RectangleI material_panel() { return { 76, 76, 400, 120 }; }
    static RectangleI label_panel() { return { 76, 236, 800, 120 }; }
    size_t readbacks() const { return readbacks_; }
private:
    void cards(DrawList& list, const RevealArt& art, float age, bool reduced) const
    {
        art.draw(list, { 80, 80, 360, 100 }, L" ", teams_[0], age, reduced);
        for (size_t side = 0; side < 2; ++side)
        { art.draw(list, { side == 0 ? 80.0f : 500.0f, 240, 360, 100 }, labels_[side], teams_[side], age, reduced); }
    }
    Frame render(const Colour& background, const std::function<void(DrawList&)>& draw)
    {
        const Canvas canvas(app_.render_resources(), background); Renderer& renderer = *app_.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app_.viewport_manager()->fullscreen_viewport());
        list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point); canvas.draw(list, { 0, 0, 1280, 720 }); draw(list);
        ++readbacks_; return read(renderer);
    }
    Application& app_; Presentation& presentation_; OpeningPresentation& look_;
    const OpeningPresentation baseline_; OpeningPresentation expected_; const MenuResultsText wrappers_;
    const std::array<std::wstring, 2> labels_; const std::array<Colour, 2> teams_{ Colour(26, 164, 218), Colour(232, 112, 44) };
    RevealArt art_, reference_, original_; const SpriteSheet* sheet_; const SpriteFrame& frame_; FontHandle font_; size_t readbacks_ = 0;
};

void reveal_colour_consumer_checks(Application& app, const Presentation& presentation, const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player); app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    Presentation local = presentation; RevealColourProbe probe(app, local);
    struct Palette { const char* name; RevealColourPalette colours; const char* control; };
    const Colour black(0, 0, 0), paper(240, 235, 221), shade(0, 0, 0, 85), ink_rgb(53, 188, 225), paper_rgb(145, 65, 194);
    const Colour partial(32, 96, 192, 128), partial_paper(192, 96, 32, 128), zero(165, 50, 214, 0);
    const std::array<Palette, 11> palettes{{
        { "default", { black, paper, shade }, "default-adoption-parity" },
        { "ink-rgb", { ink_rgb, paper, shade }, "independent-ink-rgb" },
        { "paper-rgb", { ink_rgb, paper_rgb, shade }, "independent-paper-rgb" },
        { "shade-rgb", { ink_rgb, paper_rgb, Colour(76, 163, 91) }, "independent-shade-rgb" },
        { "ink-rgba", { partial, paper_rgb, shade }, "ink-base-and-quadratic-controls" },
        { "paper-rgba", { ink_rgb, partial_paper, shade }, "paper-base-and-stepped-overlap" },
        { "shade-rgba", { ink_rgb, paper_rgb, partial }, "shade-base-airbrush-scale-and-overlap" },
        { "ink-alpha0", { zero, paper_rgb, shade }, "ink-zero-restores-underlayers-and-straight-rejected" },
        { "paper-alpha0", { ink_rgb, zero, shade }, "paper-zero-restores-ink-and-straight-rejected" },
        { "shade-alpha0", { ink_rgb, paper_rgb, zero }, "shade-zero-removes-shadow-airbrush-and-straight-rejected" },
        { "all-rgba", { partial, partial_paper, Colour(64, 160, 80, 128) }, "combined-ordered-materials-and-caller-teams" }
    }};
    const std::array<Colour, 2> backgrounds{ Colour(0, 0, 0), Colour(32, 64, 96) };
    const std::array<Frame, 2> blank{ probe.blank(backgrounds[0]), probe.blank(backgrounds[1]) };
    std::array<Frame, 2> defaults;
    size_t rows = 0, sample_rows = 0, crops = 0, negative_frames = 0;
    std::ofstream records(output / "reveal-colour-consumers.csv");
    records << "state,background,rest_match,half_match,reduced_rest,expired_rest,raw_samples,dependencies_rng,control,scope\n";
    std::ofstream samples(output / "reveal-colour-samples.csv");
    samples << "state,background,sample,pose,actual_rgb,expected_rgb,matched,straight_rgb,squared_rgb,full_airbrush_rgb,single_airbrush_rgb,no_beat_rgb,linear_beat_rgb,double_beat_rgb,scope\n";
    for (size_t palette = 0; palette < palettes.size(); ++palette)
    {
        const auto& entry = palettes[palette]; probe.set(entry.colours); probe.expect(entry.colours);
        for (size_t background = 0; background < backgrounds.size(); ++background)
        {
            const char* underlay = background == 0 ? "black" : "nonblack";
            const Frame rest = probe.actual(backgrounds[background], -1), half = probe.actual(backgrounds[background], 0.09f);
            const Frame reduced = probe.actual(backgrounds[background], 0.09f, true, false), expired = probe.actual(backgrounds[background], 0.18f, false, false);
            require(rest.pixels == probe.adoption_reference(backgrounds[background], -1, palette == 0).pixels &&
                half.pixels == probe.adoption_reference(backgrounds[background], 0.09f, palette == 0).pixels,
                "Retained reveal differs from copied-dependency production full-card/text adoption reference");
            probe.check_guards(rest, half, reduced, expired);
            const Frame raw = probe.raw(backgrounds[background], entry.colours); probe.check_raw_resource(raw);
            RevealColourProbe::Controls wrong{};
            const RevealColourRole target = palette == 5 || palette == 8 ? RevealColourRole::paper :
                palette == 6 || palette == 9 ? RevealColourRole::shade : RevealColourRole::ink;
            const auto negative = [&](RevealColourControl mode)
            { wrong[static_cast<size_t>(mode)] = probe.raw(backgrounds[background], entry.colours, mode, target); ++negative_frames; };
            if (palette >= 4 && palette <= 9) { negative(RevealColourControl::straight); }
            if (palette >= 4 && palette <= 6) { negative(RevealColourControl::squared_base); }
            if (palette == 4)
            { negative(RevealColourControl::no_beat); negative(RevealColourControl::linear_beat); negative(RevealColourControl::double_beat); }
            if (palette == 6) { negative(RevealColourControl::full_airbrush); negative(RevealColourControl::single_airbrush); }
            probe.sample_rows(samples, entry.name, underlay, rest, half, raw, "consumer-only", wrong, target);
            sample_rows += RevealColourProbe::samples().size();
            if (palette == 0) { defaults[background] = rest; }
            else { require(rest.pixels != defaults[background].pixels, "Reveal material edit failed to reach the preconstructed card"); }
            require(rest.pixels != blank[background].pixels, "Reveal partial/zero material edit incorrectly erased all other materials/team labels");
            if (palette == 7) { require(monitor_rgb(rest, 150, 81) == monitor_rgb(blank[background], 150, 81), "Zero ink failed to restore bare edge underlay"); }
            if (palette == 8) { require(monitor_rgb(rest, 150, 84) == 0x35BCE1U, "Zero paper failed to expose the independent opaque ink underlayer"); }
            if (palette == 9) { require(monitor_rgb(rest, 250, 177) == monitor_rgb(blank[background], 250, 177), "Zero shade failed to restore the shadow lip underlay"); }
            records << entry.name << ',' << underlay << ",true,true,true,true,true,true," << entry.control << ",consumer-only\n"; ++rows;
            if (palette >= 7 && palette <= 9)
            {
                save(foreground_crop(rest, RevealColourProbe::material_panel(), 0), output /
                    (std::string("reveal-colour-") + entry.name + '-' + underlay + "-materials.ppm")); ++crops;
            }
            if (palette == 0 || palette == 10)
            {
                save(foreground_crop(half, RevealColourProbe::label_panel(), 0), output /
                    (std::string("reveal-colour-") + entry.name + '-' + underlay + "-labels.ppm")); ++crops;
            }
        }
    }
    require(records.good() && samples.good() && probe.readbacks() == 228 && rows == 22 && sample_rows == 242 && crops == 10 && negative_frames == 28,
        "Reveal consumer evidence exceeded the approved counters");
    std::ofstream summary(output / "reveal-colour-consumer-summary.json");
    summary << "{\"scope\":\"consumer-only\",\"state_updates\":0,\"level_updates\":0,\"input_updates\":0,\"local_updates\":0,"
        "\"readbacks\":228,\"csv_rows\":264,\"case_rows\":22,\"sample_rows\":242,\"crops\":10,\"crop_pixels\":672000,"
        "\"negative_reference_frames\":28,\"blank_readbacks\":2,\"space_measurement\":\"actual-font-zero\",\"material_samples\":7,\"tick_samples\":4,"
        "\"viewport\":\"1280x720\",\"bounds\":[80,80,360,100],\"half_age\":0.09,\"quadratic_half_beat\":0.25,\"airbrush_scale\":0.12,"
        "\"full_shape_reference_scope\":\"production-consumer-adoption-only\",\"alpha_reference_scope\":\"independent-ordered-raw-patches\"}\n";
    require(summary.good(), "Cannot retain reveal consumer summary");
    std::cout << "PASS reveal colour consumers:228readbacks/264rows/10crops; actual-font ZERO-measure SPACE body,retained two-team labels,"
        "ordered single/double material and one/three airbrush patches,base-alpha/quadratic-half-beat controls,three per-field alpha0 underlayers,"
        "Reduce Effects/expired settled parity. Full-card comparison is adoption only; zero local/Level/State/input updates.\n";
}

#if COLOURWARS_DEV_TOOLS
void reveal_tune_colour_checks(Application& app, Registry<LevelLoadedInfo>& levels, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& content, const std::filesystem::path& output)
{
    const auto root = output / "reveal-tune-scratch", source = root / "source", staged = root / "staged";
    std::filesystem::create_directories(source); std::filesystem::create_directories(staged);
    constexpr std::array<const char*, 2> files{ "tuning.json", "presentation.json" }; std::array<std::string, 2> real_bytes;
    for (size_t index = 0; index < files.size(); ++index)
    { real_bytes[index] = tune_colour_bytes(content / files[index]); tune_colour_write(source / files[index], real_bytes[index]); tune_colour_write(staged / files[index], real_bytes[index]); }
    const NestedSaveProfile profile(root / "profile"); const TuneColourTemp temporary(root / "temp");
    struct RestoreRevealTuneContent
    {
        GameTuning& tuning; Presentation& presentation; GameTuning original_tuning; Presentation original;
        ~RestoreRevealTuneContent() { tuning = original_tuning; presentation = original; }
    } restore{ tuning, presentation, tuning, presentation };
    app.viewport_manager()->set_layout(ScreenLayout::one_player); app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    Save preferences; PlaySettings play; play.music_volume = 17; play.sfx_volume = 83; play.aim_sensitivity = 4;
    play.view_shake = false; play.aim_marker = true; play.reduce_effects = true; play.sticks[2].movement_deadzone = 25; play.sticks[2].aim_deadzone = 70;
    InputBindings bindings; bindings.keyboard_jump = Key::j;
    require(preferences.set_play_settings_and_save(play) && preferences.set_input_bindings_and_save(bindings), "Cannot seed private reveal PlaySettings/JumpJ preferences");
    const auto save_path = root / "profile/ColourWars/save_data.json"; const auto saved_preferences = tune_colour_bytes(save_path);
    const auto preference_values = [&]
    {
        const auto& p = preferences.play_settings();
        std::vector<int> values{ static_cast<int>(preferences.resolution()), preferences.fullscreen() ? 1 : 0, static_cast<int>(preferences.team_palette()),
            p.aim_sensitivity, p.music_volume, p.sfx_volume, p.view_shake ? 1 : 0, p.aim_marker ? 1 : 0, p.reduce_effects ? 1 : 0,
            static_cast<int>(preferences.input_bindings().keyboard_jump) };
        for (const auto& stick : p.sticks) { values.push_back(stick.movement_deadzone); values.push_back(stick.aim_deadzone); } return values;
    };
    const auto saved_values = preference_values(); StageList stages;
    for (const char* name : { "movement_course", "close_quarters", "king_of_the_hill", "turbulence" })
    { const auto* info = levels.get(name); stages.push_back({ name, info->display_name(), info->icon_frame(), info->include_in_random() }); }
    GameData data; data.set_application(&app); data.set_save(&preferences); const StageCatalog stage_catalog = acceptance::catalog_of(levels, stages, { "close_quarters", "king_of_the_hill", "movement_course", "turbulence" }); data.set_stage_catalog(&stage_catalog);
    data.set_tuning(&tuning); data.set_presentation(&presentation); DebugDirector director(&tuning, &presentation);
    data.set_debug(&director); auto& impl = *director.impl(); impl.set_content_directories(source, staged); impl.set_user_directory(root / "user");
    impl.set_script_directories({ root / "scripts" }); director.attach(&app, &data); impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output / "reveal-tune-commands.log").string()), "Cannot retain reveal transcript");
    RevealColourProbe probe(app, presentation); const std::array<Colour, 2> backgrounds{ Colour(0, 0, 0), Colour(32, 64, 96) };
    JsonSpanDocument first(real_bytes[1], "private reveal source"); const JsonLeaf* ink = first.find("opening.reveal_art.ink");
    require(ink != nullptr, "Reveal source missing ink"); JsonSpanDocument second(first.replaced(*ink, "\"2060C080\""), "private reveal source");
    const JsonLeaf* paper = second.find("opening.reveal_art.paper"); require(paper != nullptr, "Reveal source missing paper");
    JsonSpanDocument third(second.replaced(*paper, "\"C0602080\""), "private reveal source"); const JsonLeaf* shade = third.find("opening.reveal_art.shade");
    require(shade != nullptr, "Reveal source missing shade"); const std::string expected_saved = third.replaced(*shade, "\"40A05080\"");
    bool persisted = false; size_t commands = 0, rows = 0, sample_rows = 0;
    const auto unchanged = [&]
    {
        probe.unchanged(); require(!impl.can_apply() && impl.request().empty() && !impl.in_match() &&
            tune_colour_bytes(save_path) == saved_preferences && preference_values() == saved_values,
            "Reveal commands changed private preferences/JumpJ/default display or queued match work");
        for (size_t index = 0; index < files.size(); ++index)
        { const auto& expected = index == 1 && persisted ? expected_saved : real_bytes[index];
          require(tune_colour_bytes(content / files[index]) == real_bytes[index] && tune_colour_bytes(source / files[index]) == expected &&
              tune_colour_bytes(staged / files[index]) == expected, "Reveal commands changed actual content/private bytes beyond three saved tokens"); }
    };
    const auto command = [&](const std::string& line)
    {
        const size_t before = impl.log().lines().size(); std::srand(28571); const int expected = std::rand(); std::srand(28571);
        const DebugStatus status = impl.execute(line); ++commands;
        require(std::rand() == expected, "Reveal tune command consumed global RNG");
        if (!status.ok) { std::cout << "COMMAND " << line << ": " << status.message << '\n'; }
        require(status.ok, "Validated reveal command was rejected");
        if (line.starts_with("tune "))
        { const auto& lines = impl.log().lines(); require(lines.size() > before && lines.back().severity == DebugSeverity::info &&
            lines.back().text.starts_with("tune: presentation.opening.reveal_art.") && lines.back().text.ends_with("(live)"), "Reveal colour lost real live reply"); }
        const auto* document = impl.tune_document("presentation"); require(document != nullptr && document->edits().size() <= 3, "Reveal command exceeded three leaves");
        for (const auto& edit : document->edits())
        { require(edit.first == "opening.reveal_art.ink" || edit.first == "opening.reveal_art.paper" || edit.first == "opening.reveal_art.shade", "Reveal command edited an unaudited sibling"); }
    };
    const auto triple = [&](const char* ink_word, const char* paper_word, const char* shade_word)
    {
        command(std::string("tune presentation.opening.reveal_art.ink ") + ink_word);
        command(std::string("tune presentation.opening.reveal_art.paper ") + paper_word);
        command(std::string("tune presentation.opening.reveal_art.shade ") + shade_word);
        require(impl.tune_document("presentation")->edits().size() == 3, "Reveal triple did not retain all three edits");
    };
    std::ofstream records(output / "reveal-tune-colours.csv");
    records << "state,background,rest_match,half_match,raw_samples,dependencies_rng,private_bytes,adoption\n";
    std::ofstream samples(output / "reveal-tune-samples.csv");
    samples << "state,background,sample,pose,actual_rgb,expected_rgb,matched,straight_rgb,squared_rgb,full_airbrush_rgb,single_airbrush_rgb,no_beat_rgb,linear_beat_rgb,double_beat_rgb,scope\n";
    const auto check = [&](const char* state, const RevealColourPalette& expected)
    {
        unchanged(); probe.expect(expected);
        for (size_t background = 0; background < backgrounds.size(); ++background)
        {
            const char* underlay = background == 0 ? "black" : "nonblack";
            const Frame rest = probe.actual(backgrounds[background], -1), half = probe.actual(backgrounds[background], 0.09f);
            require(rest.pixels == probe.adoption_reference(backgrounds[background], -1).pixels &&
                half.pixels == probe.adoption_reference(backgrounds[background], 0.09f).pixels, "Reveal real command lost retained full-card/text adoption parity");
            const Frame raw = probe.raw(backgrounds[background], expected); probe.check_raw_resource(raw);
            probe.sample_rows(samples, state, underlay, rest, half, raw, "debug-validated-commands"); sample_rows += RevealColourProbe::samples().size();
            records << state << ',' << underlay << ",true,true,true,true,true,live\n"; ++rows;
        }
        unchanged();
    };
    const Colour black(0, 0, 0), original_paper(240, 235, 221), original_shade(0, 0, 0, 85), red(255, 0, 0), blue(0, 0, 255), green(0, 128, 0);
    const RevealColourPalette rgba{ Colour(32, 96, 192, 128), Colour(192, 96, 32, 128), Colour(64, 160, 80, 128) };
    check("baseline", { black, original_paper, original_shade });
    command("tune presentation.opening.reveal_art.ink RED"); check("ink-named", { red, original_paper, original_shade });
    command("tune presentation.opening.reveal_art.paper BLUE"); check("paper-named", { red, blue, original_shade });
    command("tune presentation.opening.reveal_art.shade GREEN"); check("named-triple", { red, blue, green });
    command("tune.reset presentation.opening.reveal_art.ink");
    require(impl.tune_document("presentation")->edits().size() == 2 && impl.tune_document("presentation")->edits().contains("opening.reveal_art.paper") &&
        impl.tune_document("presentation")->edits().contains("opening.reveal_art.shade"), "Reveal ink reset lost pending paper/shade siblings");
    check("reset-ink", { black, blue, green });
    command("tune.reset presentation.opening.reveal_art.paper");
    require(impl.tune_document("presentation")->edits().size() == 1 && impl.tune_document("presentation")->edits().contains("opening.reveal_art.shade"), "Reveal paper reset lost pending shade sibling");
    check("reset-paper", { black, original_paper, green });
    command("tune.reset presentation.opening.reveal_art.shade"); require(impl.tune_document("presentation")->edits().empty(), "Reveal independent resets retained edits");
    check("reset-original", { black, original_paper, original_shade });
    triple("35BCE1", "9141C2", "4CA35B"); check("rgb-triple", { Colour(53, 188, 225), Colour(145, 65, 194), Colour(76, 163, 91) });
    triple("2060C080", "C0602080", "40A05080"); check("rgba-triple", rgba);
    command("tune.save presentation"); persisted = true;
    require(impl.tune_document("presentation")->edits().empty() && impl.tune_document("presentation")->baseline().text() == expected_saved, "Reveal Save failed to rebase exactly three private tokens");
    check("saved-rgba", rgba);
    triple("RED", "BLUE", "GREEN"); check("edited-after-save", { red, blue, green });
    command("tune.reset presentation"); check("reset-saved-rgba", rgba);
    triple("A532D600", "2A904000", "8A4CC300"); check("alpha0-triple", { Colour(165, 50, 214, 0), Colour(42, 144, 64, 0), Colour(138, 76, 195, 0) });
    command("tune.reset presentation"); check("reset-alpha0-to-saved", rgba);
    require(impl.tune_document("presentation")->edits().empty() && impl.tune_document("tuning")->edits().empty(), "Reveal final reset retained pending edits"); unchanged();
    require(records.good() && samples.good() && commands == 21 && probe.readbacks() == 196 && rows == 28 && sample_rows == 308,
        "Reveal Debug evidence exceeded approved counters");
    std::ofstream summary(output / "reveal-tune-summary.json");
    summary << "{\"scope\":\"debug-validated-commands\",\"state_updates\":0,\"level_updates\":0,\"input_updates\":0,\"local_updates\":0,"
        "\"commands\":21,\"states\":14,\"readbacks\":196,\"csv_rows\":336,\"case_rows\":28,\"sample_rows\":308,\"crops\":0,\"saved_tokens\":3,"
        "\"space_measurement\":\"actual-font-zero\",\"material_samples\":7,\"tick_samples\":4,\"viewport\":\"1280x720\",\"bounds\":[80,80,360,100],"
        "\"half_age\":0.09,\"quadratic_half_beat\":0.25,\"airbrush_scale\":0.12,\"full_shape_reference_scope\":\"production-consumer-adoption-only\","
        "\"alpha_reference_scope\":\"independent-ordered-raw-patches\"}\n";
    require(summary.good(), "Cannot retain reveal Debug summary");
    std::cout << "PASS reveal tune colours:21real statements/196readbacks/336rows; independent resets retain siblings,three-token private Save/rebase,"
        "two private PlaySettings+JumpJ seed Saves/default display retained,full-card adoption and independent raw eleven-site channels. Zero local/Level/State/input updates.\n";
}
#endif
