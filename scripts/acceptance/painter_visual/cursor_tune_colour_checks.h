// Resting/clicking ScreenCursor colour adoption and bounded raw alpha samples.
// Full-shape adoption references use ScreenCursor with immutable copied
// baseline dependencies. Independent alpha proof is the small raw patch and
// its straight/squared-base-alpha controls, not the second production cursor.
enum class CursorColourAlpha { correct, straight, squared };
enum class CursorColourTarget { ink, fill, both, neither };

struct CursorColourSample
{
    const char* name;
    bool click;
    int x, y;
    CursorColourTarget material;
};

class CursorColourProbe
{
public:
    CursorColourProbe(Application& app, Presentation& presentation) :
        app_(app), presentation_(presentation),
        look_(const_cast<OpeningPresentation&>(presentation.opening())),
        baseline_(this->look_), expected_(this->baseline_),
        cursor_(&this->look_, app.render_resources()),
        reference_(&this->expected_, app.render_resources()),
        original_(&this->baseline_, app.render_resources()),
        sheet_(app.render_resources()->sprite_sheet(this->baseline_.sprite_sheet)),
        frame_(this->sheet_->sprite_frame(this->sheet_->resolve_sprite_frame(this->baseline_.fill_frame)))
    {
        const auto& art = this->baseline_.cursor_art;
        require(this->baseline_.cursor_size == 0.045f && art.shadow_offset == 0.08f &&
            art.click_duration == 0.18f && art.click_reach == 0.3f &&
            art.ink == Colour(0, 0, 0) && art.fill == Colour(243, 243, 243) &&
            art.shade == Colour(180, 180, 180) && art.shadow == Colour(0, 0, 0, 102) &&
            this->frame_.origin() == Vector2F::ZERO,
            "Cursor fixed sample geometry/material dependencies changed; audit before widening the fixture");
    }
    void set(const Colour& ink, const Colour& fill)
    { this->look_.cursor_art.ink = ink; this->look_.cursor_art.fill = fill; }
    void expect(const Colour& ink, const Colour& fill)
    {
        this->expected_ = this->baseline_;
        this->expected_.cursor_art.ink = ink; this->expected_.cursor_art.fill = fill;
        require(this->look_.cursor_art.ink == ink && this->look_.cursor_art.fill == fill,
            "Cursor adopted channels differ from the literal expected authored pair");
        this->unchanged();
    }
    void unchanged() const
    {
        const auto fixed = [&](const OpeningPresentation& look)
        {
            const auto& a = look.cursor_art; const auto& b = this->baseline_.cursor_art;
            return look.sprite_sheet == this->baseline_.sprite_sheet && look.fill_frame == this->baseline_.fill_frame &&
                look.cursor_size == this->baseline_.cursor_size && a.shade == b.shade && a.shadow == b.shadow &&
                a.shadow_offset == b.shadow_offset && a.click_duration == b.click_duration && a.click_reach == b.click_reach;
        };
        require(&this->presentation_.opening() == &this->look_ &&
            &this->presentation_.opening().cursor_art == &this->look_.cursor_art &&
            fixed(this->look_) && fixed(this->expected_),
            "Cursor colour edit changed borrowed record identity, captured assets, shade/shadow or draw geometry");
    }
    Frame actual(const Colour& background, bool click, bool reduced = false, bool duplicate = true)
    {
        this->unchanged();
        std::srand(8451); const int expected_random = std::rand(); std::srand(8451);
        const auto draw = [&](DrawList& list)
        { this->cursor_.draw(list, tip(), 720.0f, click ? 0.09f : -1.0f, reduced); };
        Frame first = this->render(background, draw);
        if (duplicate) { require(first.pixels == this->render(background, draw).pixels, "Frozen cursor draw changed pixels"); }
        require(std::rand() == expected_random, "Cursor drawing consumed gameplay randomness");
        this->unchanged(); return first;
    }
    Frame adoption_reference(const Colour& background, bool click, bool original = false)
    {
        return this->render(background, [&](DrawList& list)
        { (original ? this->original_ : this->reference_).draw(list, tip(), 720.0f, click ? 0.09f : -1.0f, false); });
    }
    Frame blank(const Colour& background) { return this->render(background, [](DrawList&) {}); }

    Frame raw_patch(const Colour& background, const Colour& ink, const Colour& fill,
        CursorColourAlpha mode = CursorColourAlpha::correct, CursorColourTarget target = CursorColourTarget::both)
    {
        // This is deliberately seven sample sites, not an independent copy of
        // the twenty-row cursor. Selected-cell sites avoid the real shadow;
        // shade is opaque, and the shadow site is outside the arrow entirely.
        const auto tint = [](const Colour& value, CursorColourAlpha alpha)
        {
            if (alpha == CursorColourAlpha::straight) { return value; }
            const float factor = alpha == CursorColourAlpha::squared ? value.a * value.a : value.a;
            return Colour(value.r * factor, value.g * factor, value.b * factor, value.a);
        };
        const Colour line = tint(ink, target == CursorColourTarget::fill ? CursorColourAlpha::correct : mode);
        const Colour inside = tint(fill, target == CursorColourTarget::ink ? CursorColourAlpha::correct : mode);
        const auto& art = this->baseline_.cursor_art;
        const float size = this->baseline_.cursor_size * 720.0f, cell = size / 20.0f;
        return this->render(background, [&](DrawList& list)
        {
            const auto rectangle = [&](const RectangleF& box, const Colour& colour)
            {
                list.draw_sprite(this->sheet_->texture(), this->frame_.source_rectangle(), box, colour,
                    0.0f, this->frame_.origin(), SpriteFlip::none, 0.0f);
            };
            const Vector2F at = tip(); const float shadow = size * art.shadow_offset;
            // The shadow's final row has a two-cell run at column 8; this
            // sample is below all foreground rows. Shade overrides its real
            // shadow underlayer because the unselected authored shade is opaque.
            rectangle({ at.x + shadow + cell * 8.0f, at.y + shadow + cell * 19.0f, cell * 2.0f, cell },
                tint(art.shadow, CursorColourAlpha::correct));
            rectangle({ at.x, at.y, cell, cell }, line);
            rectangle({ at.x, at.y + cell * 2.0f, cell, cell }, line);
            rectangle({ at.x + cell, at.y + cell * 2.0f, cell, cell }, inside);
            rectangle({ at.x + cell * 6.0f, at.y + cell * 14.0f, cell, cell },
                tint(art.shade, CursorColourAlpha::correct));

            // Only the horizontal left click stroke is needed at the two
            // isolated click sites. Raw dabs account for ink beneath fill and
            // repeat overlap; no CrudeDrawing helper participates in this oracle.
            const float progress = 0.09f / art.click_duration;
            const float distance = size * art.click_reach * (0.55f + progress * 0.45f);
            const float length = size * art.click_reach * (0.65f - progress * 0.25f);
            const Vector2F from(at.x - distance, at.y), to(from.x - length, from.y);
            const Vector2F delta = to - from;
            const auto dabs = [&](float width, const Colour& colour)
            {
                const float extent = std::sqrt(delta.x * delta.x + delta.y * delta.y);
                const int count = std::clamp(static_cast<int>(extent / (width * 0.6f)) + 1, 1, 256);
                for (int dab = 0; dab <= count; ++dab)
                {
                    const Vector2F point = from + delta * (static_cast<float>(dab) / static_cast<float>(count));
                    rectangle({ point.x - width * 0.5f, point.y - width * 0.5f, width, width }, colour);
                }
            };
            dabs(cell * 2.5f, line); dabs(cell, inside);
        });
    }

    static const std::array<CursorColourSample, 7>& samples()
    {
        static constexpr std::array<CursorColourSample, 7> result{{
            { "tip", false, 640, 240, CursorColourTarget::ink },
            { "contour", false, 640, 243, CursorColourTarget::ink },
            { "interior", false, 641, 243, CursorColourTarget::fill },
            { "shade", false, 649, 262, CursorColourTarget::neither },
            { "shadow", false, 656, 273, CursorColourTarget::neither },
            { "click-ink", true, 625, 237, CursorColourTarget::ink },
            { "click-fill", true, 632, 239, CursorColourTarget::fill }
        }};
        return result;
    }
    void sample_rows(std::ofstream& records, const char* state, const char* background,
        const Frame& rest, const Frame& click, const Frame& raw, const char* scope,
        const Frame* straight = nullptr, const Frame* squared = nullptr,
        CursorColourTarget target = CursorColourTarget::neither) const
    {
        for (const auto& sample : samples())
        {
            const auto actual = monitor_rgb(sample.click ? click : rest, sample.x, sample.y);
            const auto expected = monitor_rgb(raw, sample.x, sample.y);
            require(actual == expected, "Cursor sample differs from independent raw base-alpha/layer reference");
            const bool selected = sample.material != CursorColourTarget::neither &&
                (target == CursorColourTarget::both || target == sample.material);
            const auto negative = [&](const Frame* wrong)
            {
                if (wrong == nullptr || !selected) { return std::string("not-sampled"); }
                const auto value = monitor_rgb(*wrong, sample.x, sample.y);
                require(actual != value, "Cursor fixture cannot distinguish its targeted wrong-alpha sample");
                return std::to_string(value);
            };
            records << state << ',' << background << ',' << sample.name << ',' << (sample.click ? "click" : "rest")
                << ',' << actual << ',' << expected << ",true," << negative(straight) << ',' << negative(squared) << ',' << scope << '\n';
        }
    }
    void check_arrow(const Frame& rest, const Frame& click) const
    {
        require(foreground_crop(rest, { 640, 240, 28, 38 }, 0).pixels ==
            foreground_crop(click, { 640, 240, 28, 38 }, 0).pixels,
            "Cursor click marks changed the fixed resting arrow or its unselected materials");
    }
    size_t readbacks() const { return this->readbacks_; }
    static RectangleI panel() { return { 612, 212, 72, 80 }; }

private:
    static Vector2F tip() { return { 640.0f, 240.0f }; }
    Frame render(const Colour& background, const std::function<void(DrawList&)>& draw)
    {
        const Canvas canvas(this->app_.render_resources(), background);
        Renderer& renderer = *this->app_.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(this->app_.viewport_manager()->fullscreen_viewport());
        list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        canvas.draw(list, { 0, 0, 1280, 720 }); draw(list); ++this->readbacks_;
        return read(renderer);
    }
    Application& app_;
    Presentation& presentation_;
    OpeningPresentation& look_;
    const OpeningPresentation baseline_;
    OpeningPresentation expected_;
    ScreenCursor cursor_, reference_, original_;
    const SpriteSheet* sheet_;
    const SpriteFrame& frame_;
    size_t readbacks_ = 0;
};

// Fixed synthetic consumer inputs: tip(640,240), draw screen_height=720,
// viewport1280x720, click_age=.09. These samples establish no small-view or
// human cursor-readability acceptance. No local/Level/State update is needed.
void cursor_colour_consumer_checks(Application& app, const Presentation& presentation,
    const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    Presentation local = presentation; CursorColourProbe probe(app, local);
    struct Palette { const char* name; Colour ink, fill; const char* control; };
    const std::array<Palette, 7> palettes{{
        { "default", Colour(0, 0, 0), Colour(243, 243, 243), "opaque-baseline-parity" },
        { "ink-rgb", Colour(53, 188, 225), Colour(243, 243, 243), "changed-ink-rgb" },
        { "fill-rgb", Colour(53, 188, 225), Colour(145, 65, 194), "changed-fill-rgb" },
        { "ink-rgba", Colour(32, 96, 192, 128), Colour(145, 65, 194), "ink-straight-and-squared-base-alpha-rejected" },
        { "fill-rgba", Colour(53, 188, 225), Colour(192, 96, 32, 128), "fill-straight-and-squared-base-alpha-rejected" },
        { "pair-rgba", Colour(32, 96, 192, 128), Colour(192, 96, 32, 128), "paired-raw-alpha-composite" },
        { "pair-alpha0", Colour(165, 50, 214, 0), Colour(42, 144, 64, 0), "selected-alpha0-shade-shadow-retained-and-straight-rejected" }
    }};
    const std::array<Colour, 2> backgrounds{ Colour(0, 0, 0), Colour(32, 64, 96) };
    const std::array<Frame, 2> blank{ probe.blank(backgrounds[0]), probe.blank(backgrounds[1]) };
    std::array<Frame, 2> defaults, previous;
    size_t rows = 0, sample_rows = 0, crops = 0, negative_frames = 0;
    std::ofstream records(output / "cursor-colour-consumers.csv");
    records << "state,background,rest_match,click_match,reduced_rest,raw_samples,shade_shadow_fixed,control,scope\n";
    std::ofstream samples(output / "cursor-colour-samples.csv");
    samples << "state,background,sample,pose,actual_rgb,expected_rgb,matched,straight_rgb,squared_rgb,scope\n";
    for (size_t palette = 0; palette < palettes.size(); ++palette)
    {
        const auto& colours = palettes[palette]; probe.set(colours.ink, colours.fill); probe.expect(colours.ink, colours.fill);
        for (size_t background = 0; background < backgrounds.size(); ++background)
        {
            const char* underlay = background == 0 ? "black" : "nonblack";
            const Frame rest = probe.actual(backgrounds[background], false), click = probe.actual(backgrounds[background], true);
            const Frame reduced = probe.actual(backgrounds[background], true, true, false);
            require(rest.pixels == probe.adoption_reference(backgrounds[background], false, palette == 0).pixels &&
                click.pixels == probe.adoption_reference(backgrounds[background], true, palette == 0).pixels,
                "Cursor preconstructed consumer differs from copied-dependency full-shape adoption reference");
            require(reduced.pixels == rest.pixels, "Cursor Reduce Effects changed the arrow or retained click ticks");
            probe.check_arrow(rest, click);
            const Frame raw = probe.raw_patch(backgrounds[background], colours.ink, colours.fill);
            Frame straight{}, squared{}; CursorColourTarget target = CursorColourTarget::neither;
            if (palette == 3 || palette == 4 || palette == 6)
            {
                target = palette == 3 ? CursorColourTarget::ink : (palette == 4 ? CursorColourTarget::fill : CursorColourTarget::both);
                straight = probe.raw_patch(backgrounds[background], colours.ink, colours.fill, CursorColourAlpha::straight, target); ++negative_frames;
                if (palette != 6)
                { squared = probe.raw_patch(backgrounds[background], colours.ink, colours.fill, CursorColourAlpha::squared, target); ++negative_frames; }
            }
            probe.sample_rows(samples, colours.name, underlay, rest, click, raw, "consumer-only",
                target == CursorColourTarget::neither ? nullptr : &straight, palette == 3 || palette == 4 ? &squared : nullptr, target);
            sample_rows += CursorColourProbe::samples().size();
            if (palette == 0)
            {
                defaults[background] = rest;
                require(monitor_rgb(rest, 640, 240) == 0 &&
                    (background == 0 || monitor_rgb(rest, 640, 240) != monitor_rgb(blank[background], 640, 240)),
                    "Opaque default cursor tip lost its baseline channels");
            }
            else
            {
                require(rest.pixels != defaults[background].pixels, "Cursor colour edit did not reach the retained consumer");
                if (palette == 1)
                {
                    require(monitor_rgb(rest, 640, 243) != monitor_rgb(defaults[background], 640, 243) &&
                        monitor_rgb(rest, 641, 243) == monitor_rgb(defaults[background], 641, 243),
                        "Ink-only edit changed fill or failed to reach the contour");
                }
                if (palette == 2)
                {
                    require(monitor_rgb(rest, 640, 243) == monitor_rgb(previous[background], 640, 243) &&
                        monitor_rgb(rest, 641, 243) != monitor_rgb(previous[background], 641, 243),
                        "Fill-only edit changed ink or failed to reach the interior");
                }
            }
            require((click.pixels == rest.pixels) == (palette == 6), "Cursor selected alpha0/click response differs from expected residual materials");
            if (palette == 6)
            {
                require(rest.pixels != blank[background].pixels && monitor_rgb(rest, 649, 262) == monitor_rgb(defaults[background], 649, 262) &&
                    monitor_rgb(rest, 656, 273) == monitor_rgb(defaults[background], 656, 273),
                    "Cursor alpha0 pair lost unselected shade/shadow residuals");
            }
            previous[background] = rest;
            records << colours.name << ',' << underlay << ",true,true,true,true,true," << colours.control << ",consumer-only\n"; ++rows;
            if (palette == 0 || palette == 5 || palette == 6)
            {
                const std::array<std::pair<const char*, const Frame*>, 2> poses{{ { "rest", &rest }, { "click", &click } }};
                for (const auto& pose : poses)
                {
                    save(foreground_crop(*pose.second, CursorColourProbe::panel(), 0), output /
                        (std::string("cursor-colour-") + colours.name + '-' + underlay + '-' + pose.first + ".ppm")); ++crops;
                }
            }
        }
    }
    require(records.good() && samples.good() && probe.readbacks() == 124 && rows == 14 && sample_rows == 98 && crops == 12 && negative_frames == 10,
        "Cursor consumer evidence exceeded its bounded counts");
    std::ofstream summary(output / "cursor-colour-consumer-summary.json");
    summary << "{\"scope\":\"consumer-only\",\"state_updates\":0,\"level_updates\":0,\"effect_preparation_updates\":0,"
        "\"draw_screen_height\":720,\"viewport\":\"1280x720\",\"tip\":[640,240],\"click_age\":0.09,\"readbacks\":124,"
        "\"csv_rows\":112,\"case_rows\":14,\"sample_rows\":98,\"crops\":12,\"negative_reference_frames\":10,\"blank_readbacks\":2,"
        "\"full_shape_reference_scope\":\"production-consumer-adoption-only\",\"alpha_reference_scope\":\"independent-raw-sample-patches\",\"event_fade\":false}\n";
    require(summary.good(), "Cannot retain cursor consumer summary");
    std::cout << "PASS cursor colour consumers:124 readbacks,112 rows,twelve72x80 crops; preconstructed resting/clicking consumers, "
        "copied-baseline full-shape adoption parity,independent raw contour/interior/click alpha samples and negative controls, "
        "shade/shadow residuals and Reduce Effects arrow preservation. Synthetic draw screen_height720,zero local/Level/State updates.\n";
}

#if COLOURWARS_DEV_TOOLS
// Debug adds15 real statements,168 readbacks and192 rows; no additional crops.
void cursor_tune_colour_checks(Application& app, Registry<LevelLoadedInfo>& levels,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& content,
    const std::filesystem::path& output)
{
    const auto root = output / "cursor-tune-scratch", source = root / "source", staged = root / "staged";
    std::filesystem::create_directories(source); std::filesystem::create_directories(staged);
    constexpr std::array<const char*, 2> files{ "tuning.json", "presentation.json" };
    std::array<std::string, 2> real_bytes;
    for (size_t index = 0; index < files.size(); ++index)
    {
        real_bytes[index] = tune_colour_bytes(content / files[index]);
        tune_colour_write(source / files[index], real_bytes[index]); tune_colour_write(staged / files[index], real_bytes[index]);
    }
    const NestedSaveProfile profile(root / "profile"); const TuneColourTemp temporary(root / "temp");
    struct RestoreCursorTuneContent
    {
        GameTuning& tuning; Presentation& presentation; GameTuning original_tuning; Presentation original;
        ~RestoreCursorTuneContent() { this->tuning = this->original_tuning; this->presentation = this->original; }
    } restore{ tuning, presentation, tuning, presentation };
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    Save preferences; SettingsSession preferences_session(preferences); PlaySettings play; play.music_volume = 17; play.sfx_volume = 83;
    play.aim_sensitivity = 4; play.view_shake = false; play.aim_marker = true; play.reduce_effects = true;
    play.sticks[2].movement_deadzone = 25; play.sticks[2].aim_deadzone = 70;
    require(preferences_session.set_play_settings_and_save(play) && preferences.set_display_settings_and_save(
        ScreenResolution::s_1280_720, false, TeamColourPair::blue_orange), "Cannot seed private cursor tune preferences");
    const auto save_path = root / "profile/ColourWars/save_data.json";
    const std::string saved_preferences = tune_colour_bytes(save_path);
    const auto preference_values = [&]
    {
        const PlaySettings& active = preferences_session.play_settings();
        std::vector<int> values{ static_cast<int>(preferences.resolution()), preferences.fullscreen() ? 1 : 0,
            static_cast<int>(preferences.team_palette()), active.aim_sensitivity, active.music_volume,
            active.sfx_volume, active.view_shake ? 1 : 0, active.aim_marker ? 1 : 0,
            active.reduce_effects ? 1 : 0, static_cast<int>(preferences_session.input_bindings().keyboard_jump) };
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
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_settings_session(&preferences_session);
    const StageCatalog stage_catalog = acceptance::catalog_of(levels, stages, { "close_quarters", "king_of_the_hill", "movement_course", "turbulence" }); data.set_stage_catalog(&stage_catalog); data.set_tuning(&tuning); data.set_presentation(&presentation);
    DebugDirector director(&tuning, &presentation); data.set_debug(&director); DebugDirectorImpl& impl = *director.impl();
    impl.set_content_directories(source, staged); impl.set_user_directory(root / "user");
    impl.set_script_directories({ root / "scripts" }); director.attach(&app, &data);
    impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output / "cursor-tune-commands.log").string()), "Cannot retain cursor command transcript");
    CursorColourProbe probe(app, presentation);
    const std::array<Colour, 2> backgrounds{ Colour(0, 0, 0), Colour(32, 64, 96) };
    size_t commands = 0, rows = 0, sample_rows = 0; bool persisted = false;
    JsonSpanDocument first(real_bytes[1], "private cursor source");
    const JsonLeaf* ink = first.find("opening.cursor_art.ink");
    require(ink != nullptr, "Cursor source has no ink leaf");
    JsonSpanDocument second(first.replaced(*ink, "\"2060C080\""), "private cursor source");
    const JsonLeaf* fill = second.find("opening.cursor_art.fill");
    require(fill != nullptr, "Cursor source has no fill leaf");
    const std::string expected_saved = second.replaced(*fill, "\"C0602080\"");
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
            require(edit.first == "opening.cursor_art.ink" || edit.first == "opening.cursor_art.fill",
                "Cursor command edited an unaudited sibling");
        }
    };
    const auto pair = [&](const char* ink_word, const char* fill_word)
    {
        command(std::string("tune presentation.opening.cursor_art.ink ") + ink_word);
        command(std::string("tune presentation.opening.cursor_art.fill ") + fill_word);
        const TuneDocument* document = impl.tune_document("presentation");
        require(document->edits().size() == 2 && document->edits().contains("opening.cursor_art.ink") &&
            document->edits().contains("opening.cursor_art.fill"), "Cursor pair did not retain exactly its two audited edits");
    };
    std::ofstream records(output / "cursor-tune-colours.csv");
    records << "state,background,rest_match,click_match,raw_samples,shade_shadow_fixed,private_bytes,adoption\n";
    std::ofstream samples(output / "cursor-tune-samples.csv");
    samples << "state,background,sample,pose,actual_rgb,expected_rgb,matched,straight_rgb,squared_rgb,scope\n";
    const auto check = [&](const char* state, const Colour& expected_ink, const Colour& expected_fill)
    {
        unchanged(); probe.expect(expected_ink, expected_fill);
        for (size_t background = 0; background < backgrounds.size(); ++background)
        {
            const char* underlay = background == 0 ? "black" : "nonblack";
            const Frame rest = probe.actual(backgrounds[background], false), click = probe.actual(backgrounds[background], true);
            require(rest.pixels == probe.adoption_reference(backgrounds[background], false).pixels &&
                click.pixels == probe.adoption_reference(backgrounds[background], true).pixels,
                "Retained cursor did not adopt real validated command colours with copied-dependency shape parity");
            probe.check_arrow(rest, click);
            const Frame raw = probe.raw_patch(backgrounds[background], expected_ink, expected_fill);
            probe.sample_rows(samples, state, underlay, rest, click, raw, "debug-validated-commands");
            sample_rows += CursorColourProbe::samples().size();
            records << state << ',' << underlay << ",true,true,true,true,true,live\n"; ++rows;
        }
        unchanged();
    };
    const Colour black(0, 0, 0), white(243, 243, 243), red(255, 0, 0), blue(0, 0, 255);
    const Colour rgba_ink(32, 96, 192, 128), rgba_fill(192, 96, 32, 128);
    check("baseline", black, white);
    command("tune presentation.opening.cursor_art.ink RED"); check("ink-named", red, white);
    command("tune presentation.opening.cursor_art.fill BLUE"); check("named-pair", red, blue);
    command("tune.reset presentation.opening.cursor_art.ink");
    require(impl.tune_document("presentation")->edits().size() == 1 &&
        impl.tune_document("presentation")->edits().contains("opening.cursor_art.fill"), "Cursor ink reset lost its pending fill sibling");
    check("reset-ink", black, blue);
    command("tune.reset presentation.opening.cursor_art.fill");
    require(impl.tune_document("presentation")->edits().empty(), "Cursor separate leaf resets retained edits");
    check("reset-original", black, white);
    pair("35BCE1", "9141C2"); check("rgb-pair", Colour(53, 188, 225), Colour(145, 65, 194));
    pair("2060C080", "C0602080"); check("rgba-pair", rgba_ink, rgba_fill);
    command("tune.save presentation"); persisted = true;
    require(impl.tune_document("presentation")->edits().empty() && impl.tune_document("presentation")->baseline().text() == expected_saved,
        "Cursor save failed to rebase exactly its two private tokens");
    check("saved-rgba", rgba_ink, rgba_fill);
    pair("RED", "BLUE"); check("edited-after-save", red, blue);
    command("tune.reset presentation"); check("reset-saved-rgba", rgba_ink, rgba_fill);
    pair("A532D600", "2A904000"); check("alpha0-pair", Colour(165, 50, 214, 0), Colour(42, 144, 64, 0));
    command("tune.reset presentation"); check("reset-alpha0-to-saved", rgba_ink, rgba_fill);
    require(impl.tune_document("presentation")->edits().empty() && impl.tune_document("tuning")->edits().empty(),
        "Cursor final reset left pending private edits");
    unchanged();
    require(records.good() && samples.good() && commands == 15 && probe.readbacks() == 168 && rows == 24 && sample_rows == 168,
        "Cursor Debug command evidence exceeded its bounded counts");
    std::ofstream summary(output / "cursor-tune-summary.json");
    summary << "{\"scope\":\"debug-validated-commands\",\"state_updates\":0,\"level_updates\":0,\"effect_preparation_updates\":0,"
        "\"draw_screen_height\":720,\"viewport\":\"1280x720\",\"tip\":[640,240],\"click_age\":0.09,\"commands\":15,\"readbacks\":168,"
        "\"csv_rows\":192,\"case_rows\":24,\"sample_rows\":168,\"crops\":0,\"saved_tokens\":2,"
        "\"full_shape_reference_scope\":\"production-consumer-adoption-only\",\"alpha_reference_scope\":\"independent-raw-sample-patches\",\"event_fade\":false}\n";
    require(summary.good(), "Cannot retain cursor Debug command summary");
    std::cout << "PASS cursor tune colours:15 validated named/RGB/RGBA/separate-reset/save commands,168 readbacks,192 rows; "
        "two-token private save/rebase,retained resting/clicking consumers,independent raw samples and preserved shade/shadow/preferences/RNG. "
        "Synthetic draw screen_height720,zero local/Level/State updates; no physical input or small-view/human acceptance.\n";
}
#endif
