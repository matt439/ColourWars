// Narrow leaf probe, included after monitor_checks.h and (Debug only)
// tune_colour_checks.h. Shared checks render literal palettes in both presets;
// Debug adds real validated tune/reset/save commands on private content only.
// No StateContext updates, gameplay objects, scripted pads or input injection.
enum class FeedbackColourReference { premultiplied, legacy_straight, doubled_fade };

class FeedbackColourProbe
{
public:
    FeedbackColourProbe(Application& app, const WeaponFeedbackPresentation& look) :
        app_(app), look_(look), geometry_(look),
        sheet_(app.render_resources()->sprite_sheet(look.sprite_sheet)),
        frame_(this->sheet_->sprite_frame(this->sheet_->resolve_sprite_frame(look.fill_frame)))
    {
        require(look.enabled && look.muzzle_duration > 0.0f && look.empty_duration > 0.0f &&
            look.recovered_duration > 0.0f, "Feedback colour fixture requires enabled finite pulses");
        for (size_t index = 0; index < this->cues_.size(); ++index)
        {
            auto cue = std::make_unique<WeaponFeedback>(&look, app.render_resources());
            const size_t event = index % 3;
            if (event == 0) { cue->empty(); }
            else if (event == 1) { cue->recovered(); }
            else { cue->shot(WeaponType::sprayer); }
            if (index >= 3)
            {
                const float duration = event == 0 ? look.empty_duration :
                    (event == 1 ? look.recovered_duration : look.muzzle_duration);
                cue->update(duration * 0.5f);
            }
            this->states_[index] = cue->state(); this->cues_[index] = std::move(cue);
        }
        this->bounds_ = this->bounds();
    }

    void unchanged() const
    {
        const auto& a = this->look_; const auto& b = this->geometry_;
        require(a.enabled == b.enabled && a.sprite_sheet == b.sprite_sheet && a.fill_frame == b.fill_frame &&
            a.muzzle_duration == b.muzzle_duration && a.empty_duration == b.empty_duration &&
            a.recovered_duration == b.recovered_duration && a.keyline_width == b.keyline_width &&
            a.status_size == b.status_size && a.status_offset == b.status_offset &&
            a.aim_marker_distance == b.aim_marker_distance && a.aim_marker_size == b.aim_marker_size &&
            a.aim_marker_stroke == b.aim_marker_stroke && a.muzzle_sizes == b.muzzle_sizes &&
            this->bounds() == this->bounds_, "Feedback colour edit changed geometry, assets or culling bounds");
        for (size_t index = 0; index < this->cues_.size(); ++index)
        { require(this->cues_[index]->state() == this->states_[index], "Feedback colour drawing advanced a pulse timer"); }
    }

    Frame actual(const Colour& background, bool reduced = false)
    {
        this->unchanged();
        std::srand(8451); const int expected_random = std::rand(); std::srand(8451);
        const auto draw = [&](DrawList& list)
        {
            for (size_t index = 0; index < this->cues_.size(); ++index)
            {
                const Vector2F center = cell(index);
                this->cues_[index]->draw(list, center,
                    center + Vector2F(0.0f, this->geometry_.status_offset), 0.0f,
                    WeaponType::sprayer, team(), reduced);
            }
            for (size_t index = 0; index < 3; ++index)
            {
                this->cues_[0]->draw_aim_marker(list, cell(index + 6) -
                    Vector2F(this->geometry_.aim_marker_distance, 0.0f), 0.0f,
                    static_cast<WeaponReadiness>(index), team());
            }
        };
        Frame first = this->render(background, draw);
        require(first.pixels == this->render(background, draw).pixels,
            "Frozen feedback colour consumer changed pixels between views");
        require(std::rand() == expected_random, "Feedback colour drawing consumed match randomness");
        this->unchanged(); return first;
    }

    Frame reference(const Colour& background, const Colour& highlight, const Colour& keyline,
        bool reduced = false, FeedbackColourReference mode = FeedbackColourReference::premultiplied)
    {
        // Raw source rectangles and explicit channels deliberately bypass both
        // WeaponFeedback::draw_block and the client's premultiplication helper.
        return this->render(background, [&](DrawList& list)
        {
            const auto tint = [&](const Colour& colour, float life)
            {
                if (mode == FeedbackColourReference::legacy_straight)
                { return Colour(colour.r * life, colour.g * life, colour.b * life, colour.a * life); }
                const float rgb_fade = mode == FeedbackColourReference::doubled_fade ? life * life : life;
                return Colour(colour.r * colour.a * rgb_fade, colour.g * colour.a * rgb_fade,
                    colour.b * colour.a * rgb_fade, colour.a * life);
            };
            const auto block = [&](const Vector2F& center, const Vector2F& dimensions,
                float angle, const Colour& colour, float life)
            {
                const RectangleI& source = this->frame_.source_rectangle();
                const Vector2F origin = this->frame_.origin() +
                    Vector2F(static_cast<float>(source.width) * 0.5f, static_cast<float>(source.height) * 0.5f);
                const float border = this->geometry_.keyline_width * 2.0f;
                list.draw_sprite(this->sheet_->texture(), source,
                    RectangleF(center, dimensions + Vector2F(border, border)), tint(keyline, life),
                    angle, origin, SpriteFlip::none, 0.0f);
                list.draw_sprite(this->sheet_->texture(), source, RectangleF(center, dimensions),
                    tint(colour, life), angle, origin, SpriteFlip::none, 0.0f);
            };
            const float size = this->geometry_.status_size;
            for (size_t row = 0; row < 2; ++row)
            {
                const float life = row == 0 ? 1.0f : 0.5f;
                for (const float turn : { -PI / 4.0f, PI / 4.0f })
                { block(cell(row * 3), { size, size * 0.25f }, turn, highlight, life); }
                block(cell(row * 3 + 1), { size * 0.65f, size }, 0.0f, team(), life);
                if (!reduced)
                {
                    for (const float side : { -1.0f, 1.0f })
                    { block(cell(row * 3 + 1) + Vector2F(side * size, 0),
                        { size * 0.4f, size * 0.2f }, 0.0f, highlight, life); }
                    const Vector2F muzzle = this->geometry_.muzzle_sizes.at(static_cast<size_t>(WeaponType::sprayer));
                    const auto mark = [&](float x, float y, float width, float height, float turn)
                    {
                        block(cell(row * 3 + 2) + Vector2F((x + (1.0f - life) * 0.3f) * muzzle.x, y * muzzle.y),
                            { width * muzzle.x, height * muzzle.y }, turn, team(), life);
                    };
                    mark(0.35f, 0.0f, 0.6f, 0.35f, 0.0f);
                    mark(0.65f, -0.4f, 0.22f, 0.3f, -0.2f);
                    mark(0.8f, 0.3f, 0.18f, 0.4f, 0.15f);
                }
            }
            const float aim = this->geometry_.aim_marker_size, stroke = this->geometry_.aim_marker_stroke;
            block(cell(6) + Vector2F(-aim * 0.1f, -aim * 0.22f), { aim * 0.72f, stroke }, PI / 4.0f, highlight, 1.0f);
            block(cell(6) + Vector2F(-aim * 0.1f, aim * 0.22f), { aim * 0.72f, stroke }, -PI / 4.0f, highlight, 1.0f);
            block(cell(6) + Vector2F(-aim * 0.45f, 0.0f), { stroke, stroke }, 0.0f, team(), 1.0f);
            for (const float side : { -1.0f, 1.0f })
            { block(cell(7) + Vector2F(side * aim * 0.22f, 0), { stroke, aim * 0.8f }, 0.0f, highlight, 1.0f); }
            for (const float turn : { -PI / 4.0f, PI / 4.0f })
            { block(cell(8), { aim * 0.85f, stroke }, turn, highlight, 1.0f); }
        });
    }

    void check_team(const Frame& actual, bool reduced) const
    {
        // Sprite destinations truncate before applying their source origin.
        // Rounding 212.8 up to213 samples the three-pixel bead's excluded edge.
        const int bead_x = static_cast<int>(cell(6).x - this->geometry_.aim_marker_size * 0.45f);
        require(monitor_rgb(actual, 360, 240) == 0x2CA8CE,
            "Feedback tint changed the opaque recovered team centre");
        for (const int x : { bead_x - 1, bead_x })
        {
            for (const int y : { 449, 450 })
            { require(monitor_rgb(actual, x, y) == 0x2CA8CE,
                "Feedback tint changed an opaque interior readiness-bead pixel"); }
        }
        if (!reduced)
        {
            const int muzzle_x = static_cast<int>(std::lround(500.0f + this->geometry_.muzzle_sizes.at(
                static_cast<size_t>(WeaponType::sprayer)).x * 0.35f));
            require(monitor_rgb(actual, muzzle_x, 240) == 0x2CA8CE,
                "Feedback tint changed the opaque full-life paid-shot team centre");
        }
    }

    size_t readbacks() const { return this->readbacks_; }
    static RectangleI panel() { return { 180, 200, 360, 280 }; }

private:
    static Colour team() { return Colour(44, 168, 206); }
    static Vector2F cell(size_t index)
    { return { 220.0f + static_cast<float>(index % 3) * 140.0f, index < 6 ?
        (240.0f + static_cast<float>(index / 3) * 100.0f) : 450.0f }; }

    std::array<RectangleF, 9> bounds() const
    {
        std::array<RectangleF, 9> result;
        for (size_t index = 0; index < this->cues_.size(); ++index)
        { result[index] = this->cues_[index]->bounds(cell(index), cell(index) +
            Vector2F(0, this->geometry_.status_offset), WeaponType::sprayer); }
        for (size_t index = 6; index < result.size(); ++index)
        { result[index] = this->cues_[0]->aim_marker_bounds(cell(index) -
            Vector2F(this->geometry_.aim_marker_distance, 0), 0.0f); }
        return result;
    }

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
    const WeaponFeedbackPresentation& look_;
    WeaponFeedbackPresentation geometry_;
    const SpriteSheet* sheet_;
    const SpriteFrame& frame_;
    std::array<std::unique_ptr<WeaponFeedback>, 6> cues_;
    std::array<WeaponFeedbackState, 6> states_;
    std::array<RectangleF, 9> bounds_;
    size_t readbacks_ = 0;
};

// Both presets: 56 readbacks, eight 360x280 crops and eight CSV rows. Release
// runs only this literal-colour consumer proof, and exercises no developer API.
void feedback_colour_consumer_checks(Application& app, const Presentation& presentation,
    const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    Presentation local = presentation;
    auto& look = const_cast<WeaponFeedbackPresentation&>(local.weapon_feedback());
    require(look.highlight == Colour(255, 255, 255) && look.keyline == Colour(0, 0, 0),
        "Feedback legacy parity fixture no longer matches the opaque reviewed defaults");
    FeedbackColourProbe probe(app, look);
    struct Palette { const char* name; Colour highlight; Colour keyline; };
    const std::array<Palette, 4> palettes{{
        { "default", Colour(255, 255, 255), Colour(0, 0, 0) },
        { "rgb", Colour(53, 188, 225), Colour(145, 65, 194) },
        { "rgba128", Colour(32, 96, 192, 128), Colour(192, 96, 32, 128) },
        { "alpha0", Colour(165, 50, 214, 0), Colour(42, 144, 64, 0) }
    }};
    const std::array<Colour, 2> backgrounds{ Colour(0, 0, 0), Colour(32, 64, 96) };
    std::array<Frame, 2> defaults;
    size_t rows = 0, crops = 0, controls = 0;
    std::ofstream records(output / "feedback-colour-consumers.csv");
    records << "palette,background,normal_match,reduced_match,team_centres,frozen_geometry_timers_rng,control,scope\n";
    for (size_t palette = 0; palette < palettes.size(); ++palette)
    {
        look.highlight = palettes[palette].highlight; look.keyline = palettes[palette].keyline;
        for (size_t background = 0; background < backgrounds.size(); ++background)
        {
            const Frame actual = probe.actual(backgrounds[background]);
            const Frame expected = probe.reference(backgrounds[background], palettes[palette].highlight, palettes[palette].keyline);
            require(actual.pixels == expected.pixels, "Feedback literal palette differs from independent premultiplied raw-source oracle");
            const Frame reduced = probe.actual(backgrounds[background], true);
            const Frame reduced_expected = probe.reference(backgrounds[background], palettes[palette].highlight, palettes[palette].keyline, true);
            require(reduced.pixels == reduced_expected.pixels && reduced.pixels != actual.pixels,
                "Feedback Reduce Effects changed persistent information or retained cosmetic muzzle/rays");
            for (const RectangleI area : { RectangleI(195, 215, 50, 50), RectangleI(195, 315, 50, 50), RectangleI(195, 425, 330, 50) })
            { require(foreground_crop(actual, area, 0).pixels == foreground_crop(reduced, area, 0).pixels,
                "Reduce Effects changed empty acknowledgement or stateless readiness shapes"); }
            probe.check_team(actual, false); probe.check_team(reduced, true);
            const char* control = "changed-rgb";
            if (palette == 0)
            {
                defaults[background] = actual;
                require(actual.pixels == probe.reference(backgrounds[background], palettes[palette].highlight,
                    palettes[palette].keyline, false, FeedbackColourReference::legacy_straight).pixels,
                    "Premultiplication changed opaque-default feedback pixels or fades"); ++controls;
                require(actual.pixels != probe.reference(backgrounds[background], palettes[palette].highlight,
                    palettes[palette].keyline, false, FeedbackColourReference::doubled_fade).pixels,
                    "Feedback fixture cannot detect applying the event fade twice"); ++controls;
                control = "opaque-legacy-parity-and-double-fade-rejected";
            }
            else
            {
                require(actual.pixels != defaults[background].pixels, "Literal palette did not reach the preconstructed feedback consumer");
                if (palette >= 2)
                {
                    require(actual.pixels != probe.reference(backgrounds[background], palettes[palette].highlight,
                        palettes[palette].keyline, false, FeedbackColourReference::legacy_straight).pixels,
                        "Feedback RGBA fixture cannot detect straight tint submitted to ONE blending"); ++controls;
                    control = "legacy-straight-alpha-rejected";
                }
            }
            records << palettes[palette].name << ',' << (background == 0 ? "black" : "nonblack")
                << ",true,true,true,true," << control << ",consumer-only\n"; ++rows;
            save(foreground_crop(actual, FeedbackColourProbe::panel(), 0), output /
                (std::string("feedback-colour-") + palettes[palette].name + (background == 0 ? "-black.ppm" : "-nonblack.ppm"))); ++crops;
        }
    }
    require(records.good() && probe.readbacks() == 56 && rows == 8 && crops == 8 && controls == 8,
        "Feedback consumer evidence exceeded its bounded counts");
    std::ofstream summary(output / "feedback-colour-consumer-summary.json");
    summary << "{\"scope\":\"consumer-only\",\"state_updates\":0,\"readbacks\":56,\"csv_rows\":8,\"crops\":8,\"controls\":8}\n";
    require(summary.good(), "Cannot retain feedback consumer summary");
    std::cout << "PASS feedback colour consumers: 56 readbacks, eight compact crops; independent literal RGBA, "
        "opaque legacy parity, fade/straight-alpha negative controls, Reduce Effects, team centres and frozen timers/bounds/RNG. "
        "Both-preset consumer-only renderer scope; no developer-command, match, hardware or global-input claims.\n";
}

#if COLOURWARS_DEV_TOOLS
// Debug adds 15 real commands, 66 readbacks and 20 CSV rows; no extra crops.
// The same six consumers borrow the live presentation before the first edit.
void feedback_tune_colour_checks(Application& app, Registry<LevelLoadedInfo>& levels,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& content,
    const std::filesystem::path& output)
{
    const auto root = output / "feedback-tune-scratch", source = root / "source", staged = root / "staged";
    std::filesystem::create_directories(source); std::filesystem::create_directories(staged);
    constexpr std::array<const char*, 2> files{ "tuning.json", "presentation.json" };
    std::array<std::string, 2> real_bytes;
    for (size_t index = 0; index < files.size(); ++index)
    {
        real_bytes[index] = tune_colour_bytes(content / files[index]);
        tune_colour_write(source / files[index], real_bytes[index]); tune_colour_write(staged / files[index], real_bytes[index]);
    }
    const NestedSaveProfile profile(root / "profile"); const TuneColourTemp temporary(root / "temp");
    const GameTuning original_tuning = tuning; const Presentation original = presentation;
    struct RestoreFeedbackTuneContent
    {
        GameTuning& tuning; Presentation& presentation; GameTuning original_tuning; Presentation original;
        ~RestoreFeedbackTuneContent() { this->tuning = this->original_tuning; this->presentation = this->original; }
    } restore{ tuning, presentation, original_tuning, original };
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    Save preferences; PlaySettings play; play.music_volume = 17; play.sfx_volume = 83;
    play.aim_sensitivity = 4; play.view_shake = false; play.aim_marker = true; play.reduce_effects = true;
    play.sticks[2].movement_deadzone = 25; play.sticks[2].aim_deadzone = 70;
    require(preferences.set_play_settings_and_save(play) && preferences.set_display_settings_and_save(
        ScreenResolution::s_1280_720, false, TeamColourPair::blue_orange), "Cannot seed private feedback tune preferences");
    const auto save_path = root / "profile/ColourWars/save_data.json";
    const std::string saved_preferences = tune_colour_bytes(save_path);
    StageList stages;
    for (const char* name : { "movement_course", "close_quarters", "king_of_the_hill", "turbulence" })
    {
        const auto* info = levels.get(name);
        stages.push_back({ name, info->display_name(), info->icon_frame(), info->include_in_random() });
    }
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_level_infos(&levels);
    data.set_stages(&stages); data.set_tuning(&tuning); data.set_presentation(&presentation);
    DebugDirector director(&tuning, &presentation); data.set_debug(&director); DebugDirectorImpl& impl = *director.impl();
    impl.set_content_directories(source, staged); impl.set_user_directory(root / "user");
    impl.set_script_directories({ root / "scripts" }); director.attach(&app, &data);
    impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output / "feedback-tune-commands.log").string()), "Cannot retain feedback command transcript");
    FeedbackColourProbe probe(app, presentation.weapon_feedback());
    const std::array<Colour, 2> backgrounds{ Colour(0, 0, 0), Colour(32, 64, 96) };
    size_t commands = 0, rows = 0; bool persisted = false;
    JsonSpanDocument first(real_bytes[1], "private feedback source");
    const JsonLeaf* highlight = first.find("weapon_feedback.highlight");
    require(highlight != nullptr, "Feedback source has no highlight leaf");
    JsonSpanDocument second(first.replaced(*highlight, "\"2060C080\""), "private feedback source");
    const JsonLeaf* keyline = second.find("weapon_feedback.keyline");
    require(keyline != nullptr, "Feedback source has no keyline leaf");
    const std::string expected_saved = second.replaced(*keyline, "\"C0602080\"");
    const auto unchanged = [&]
    {
        probe.unchanged();
        require(!impl.can_apply() && impl.request().empty() && !impl.in_match() &&
            tune_colour_bytes(save_path) == saved_preferences && preferences.play_settings().music_volume == 17 &&
            preferences.play_settings().sfx_volume == 83 && preferences.play_settings().aim_sensitivity == 4 &&
            !preferences.play_settings().view_shake && preferences.play_settings().aim_marker &&
            preferences.play_settings().reduce_effects && preferences.play_settings().sticks[2].movement_deadzone == 25 &&
            preferences.play_settings().sticks[2].aim_deadzone == 70 &&
            preferences.team_palette() == TeamColourPair::blue_orange && !preferences.fullscreen() &&
            preferences.resolution() == ScreenResolution::s_1280_720,
            "Feedback tune changed private preferences, queued work or entered a match");
        for (size_t index = 0; index < files.size(); ++index)
        {
            const std::string& expected = index == 1 && persisted ? expected_saved : real_bytes[index];
            require(tune_colour_bytes(content / files[index]) == real_bytes[index] &&
                tune_colour_bytes(source / files[index]) == expected && tune_colour_bytes(staged / files[index]) == expected,
                "Feedback tune changed actual content or private bytes beyond its two saved tokens");
        }
    };
    const auto command = [&](const std::string& line)
    {
        const size_t previous_lines = impl.log().lines().size();
        const DebugStatus result = impl.execute(line); ++commands;
        if (!result.ok) { std::cout << "COMMAND " << line << ": " << result.message << '\n'; }
        require(result.ok, "Validated feedback tune/reset/save command was rejected");
        if (line.starts_with("tune "))
        {
            // Successful statement replies are written to the console log;
            // the aggregate execute result retains messages only for failures.
            const auto& lines = impl.log().lines();
            require(lines.size() > previous_lines && lines.back().severity == DebugSeverity::info &&
                lines.back().text.starts_with("tune: presentation.weapon_feedback.") &&
                lines.back().text.ends_with("(live)"), "Feedback colour command lost live adoption scope");
        }
    };
    const auto pair = [&](const char* highlight_word, const char* keyline_word)
    {
        command(std::string("tune presentation.weapon_feedback.highlight ") + highlight_word);
        command(std::string("tune presentation.weapon_feedback.keyline ") + keyline_word);
        const TuneDocument* doc = impl.tune_document("presentation");
        require(doc != nullptr && doc->edits().size() == 2 && doc->edits().contains("weapon_feedback.highlight") &&
            doc->edits().contains("weapon_feedback.keyline"), "Feedback commands edited outside their two audited leaves");
    };
    std::ofstream records(output / "feedback-tune-colours.csv");
    records << "state,background,normal_match,reduced_match,team_centres,frozen_geometry_timers_rng,private_bytes,adoption\n";
    const auto check = [&](const char* state, const Colour& expected_highlight, const Colour& expected_keyline, bool reduce = false)
    {
        unchanged();
        require(presentation.weapon_feedback().highlight == expected_highlight && presentation.weapon_feedback().keyline == expected_keyline,
            "Feedback command parsed different straight RGBA channels than the literal oracle");
        for (size_t background = 0; background < backgrounds.size(); ++background)
        {
            const Frame actual = probe.actual(backgrounds[background]);
            require(actual.pixels == probe.reference(backgrounds[background], expected_highlight, expected_keyline).pixels,
                "Preconstructed feedback did not adopt validated live colours with independent RGBA pixels");
            probe.check_team(actual, false);
            if (reduce)
            {
                const Frame reduced = probe.actual(backgrounds[background], true);
                require(reduced.pixels == probe.reference(backgrounds[background], expected_highlight, expected_keyline, true).pixels,
                    "Validated feedback RGBA changed Reduce Effects policy");
                probe.check_team(reduced, true);
            }
            records << state << ',' << (background == 0 ? "black" : "nonblack") << ",true,"
                << (reduce ? "true" : "not-sampled") << ",true,true,true,live\n"; ++rows;
        }
        unchanged();
    };
    const Colour white(255, 255, 255), black(0, 0, 0), rgba_highlight(32, 96, 192, 128), rgba_keyline(192, 96, 32, 128);
    check("baseline", white, black);
    pair("RED", "BLUE"); check("named", Colour(255, 0, 0), Colour(0, 0, 255));
    command("tune.reset presentation.weapon_feedback.highlight"); command("tune.reset presentation.weapon_feedback.keyline");
    require(impl.tune_document("presentation")->edits().empty(), "Feedback leaf reset retained edits");
    check("reset-original", white, black);
    pair("35BCE1", "9141C2"); check("rgb", Colour(53, 188, 225), Colour(145, 65, 194));
    pair("2060C080", "C0602080"); check("rgba128", rgba_highlight, rgba_keyline, true);
    command("tune.save presentation"); persisted = true;
    require(impl.tune_document("presentation")->edits().empty() &&
        impl.tune_document("presentation")->baseline().text() == expected_saved,
        "Feedback save failed to rebase exactly its two private tokens");
    check("saved-rgba", rgba_highlight, rgba_keyline);
    pair("RED", "BLUE"); check("edited-after-save", Colour(255, 0, 0), Colour(0, 0, 255));
    command("tune.reset presentation"); check("reset-saved-rgba", rgba_highlight, rgba_keyline);
    pair("A532D600", "2A904000"); check("alpha0", Colour(165, 50, 214, 0), Colour(42, 144, 64, 0));
    command("tune.reset presentation"); check("reset-alpha0-to-saved", rgba_highlight, rgba_keyline);
    require(impl.tune_document("presentation")->edits().empty() && impl.tune_document("tuning")->edits().empty(),
        "Feedback final reset left pending private edits");
    unchanged();
    require(records.good() && commands == 15 && probe.readbacks() == 66 && rows == 20,
        "Feedback Debug command evidence exceeded its bounded counts");
    std::ofstream summary(output / "feedback-tune-summary.json");
    summary << "{\"scope\":\"debug-validated-commands\",\"state_updates\":0,\"commands\":15,\"readbacks\":66,\"csv_rows\":20,\"crops\":0}\n";
    require(summary.good(), "Cannot retain feedback Debug command summary");
    std::cout << "PASS feedback tune colours: 15 validated named/RGB/RGBA/reset/save commands, 66 readbacks, "
        "twenty CSV rows; preconstructed live consumers, two-token private save/rebase, frozen timers/bounds/RNG and preferences. "
        "Debug-only command scope; no StateContext updates, match, hardware or global-input claims.\n";
}
#endif
