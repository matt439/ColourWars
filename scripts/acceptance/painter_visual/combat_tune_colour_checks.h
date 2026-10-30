// Narrow both-preset CombatFeedback consumer proof. Debug additionally uses
// validated tune/reset/private-save commands from tune_colour_checks.h.
// Only two local half-age preparations per fixture call CombatFeedback::update;
// no Level, StateContext, input, audio-event or gameplay update is performed.
enum class CombatColourReference { premultiplied, legacy_straight, doubled_fade };

class CombatColourProbe
{
public:
    CombatColourProbe(Application& app, const CombatFeedbackPresentation& look) :
        app_(app), look_(look), geometry_(look),
        sheet_(app.render_resources()->sprite_sheet(look.sprite_sheet)),
        frame_(this->sheet_->sprite_frame(this->sheet_->resolve_sprite_frame(look.fill_frame)))
    {
        require(look.enabled && look.hurt_duration > 0.0f && look.elimination_duration > 0.0f,
            "Combat colour fixture requires enabled finite cues");
        for (size_t index = 0; index < this->cues_.size(); ++index)
        {
            auto cue = std::make_unique<CombatFeedback>(&look, app.render_resources());
            if (index % 2 == 0) { require(cue->hurt(), "Cannot prepare the combat hurt fixture"); }
            else { cue->eliminate(body(index)); }
            if (index >= 2)
            {
                cue->update((index % 2 == 0 ? look.hurt_duration : look.elimination_duration) * 0.5f);
                ++this->effect_preparation_updates_;
            }
            this->states_[index] = state(*cue); this->cues_[index] = std::move(cue);
        }
        this->bounds_ = this->bounds();
        require(this->effect_preparation_updates_ == 2, "Combat local age-preparation budget changed");
    }

    void unchanged() const
    {
        const auto& a = this->look_; const auto& b = this->geometry_;
        require(a.enabled == b.enabled && a.sprite_sheet == b.sprite_sheet && a.fill_frame == b.fill_frame &&
            a.hurt_duration == b.hurt_duration && a.hurt_repeat_delay == b.hurt_repeat_delay &&
            a.hurt_width == b.hurt_width && a.hurt_reach == b.hurt_reach &&
            a.elimination_duration == b.elimination_duration && a.elimination_spread == b.elimination_spread &&
            a.elimination_lift == b.elimination_lift && a.particle_size == b.particle_size &&
            a.keyline_width == b.keyline_width && this->bounds() == this->bounds_,
            "Combat colour edit changed geometry, assets or culling bounds");
        for (size_t index = 0; index < this->cues_.size(); ++index)
        { require(state(*this->cues_[index]) == this->states_[index], "Combat drawing advanced public timers or death state"); }
    }

    Frame actual(const Colour& background)
    {
        this->unchanged();
        std::srand(8451); const int expected_random = std::rand(); std::srand(8451);
        const auto draw = [&](DrawList& list)
        {
            for (size_t index = 0; index < this->cues_.size(); ++index)
            { this->cues_[index]->draw(list, body(index), team()); }
        };
        Frame first = this->render(background, draw);
        require(first.pixels == this->render(background, draw).pixels,
            "Frozen combat colour consumer changed pixels between views");
        require(std::rand() == expected_random, "Combat colour drawing consumed match randomness");
        this->unchanged(); return first;
    }

    Frame reference(const Colour& background, const Colour& highlight, const Colour& keyline,
        CombatColourReference mode = CombatColourReference::premultiplied)
    {
        // Explicit channels and source rectangles bypass CombatFeedback's
        // draw_fill/draw_keyline and the production premultiplication helper.
        return this->render(background, [&](DrawList& list)
        {
            const auto tint = [&](const Colour& colour, float life)
            {
                if (mode == CombatColourReference::legacy_straight)
                { return Colour(colour.r * life, colour.g * life, colour.b * life, colour.a * life); }
                const float rgb_fade = mode == CombatColourReference::doubled_fade ? life * life : life;
                return Colour(colour.r * colour.a * rgb_fade, colour.g * colour.a * rgb_fade,
                    colour.b * colour.a * rgb_fade, colour.a * life);
            };
            const auto fill = [&](const RectangleF& rectangle, const Colour& colour, float life)
            {
                list.draw_sprite(this->sheet_->texture(), this->frame_.source_rectangle(), rectangle,
                    tint(colour, life), 0.0f, this->frame_.origin(), SpriteFlip::none, 0.0f);
            };
            const auto border = [&](const RectangleF& rectangle, float life)
            {
                const float width = this->geometry_.keyline_width;
                fill({ rectangle.x - width, rectangle.y - width,
                    rectangle.width + width * 2.0f, rectangle.height + width * 2.0f }, keyline, life);
            };
            // The authored eight-fragment layout is restated as literal oracle
            // data; no production geometry or fragment helper is called.
            struct Fragment { float x, y, scale; bool tall; };
            constexpr std::array<Fragment, 8> fragments{{
                { -1.0f, -0.65f, 1.0f, false }, { -0.55f, -1.0f, 0.8f, true },
                { 0.3f, -0.9f, 1.15f, false }, { 0.95f, -0.55f, 0.9f, true },
                { 1.0f, 0.25f, 1.1f, false }, { 0.5f, 0.9f, 0.85f, true },
                { -0.35f, 1.0f, 1.0f, false }, { -0.95f, 0.4f, 0.75f, true }
            }};
            for (size_t row = 0; row < 2; ++row)
            {
                const float life = row == 0 ? 1.0f : 0.5f;
                const RectangleF hurt = body(row * 2);
                for (const bool left : { true, false })
                {
                    const float edge = left ? hurt.x : hurt.x + hurt.width;
                    const float reach = this->geometry_.hurt_reach;
                    const RectangleF top(left ? edge - reach : edge,
                        hurt.y + hurt.height * 0.12f, reach, this->geometry_.hurt_width);
                    const RectangleF lower(left ? edge - reach * 0.6f : edge,
                        hurt.y + hurt.height * 0.34f, reach * 0.6f, this->geometry_.hurt_width);
                    border(top, life); border(lower, life);
                    fill(top, highlight, life); fill(lower, highlight, life);
                }
                const RectangleF death = body(row * 2 + 1);
                const Vector2F center(death.x + death.width * 0.5f, death.y + death.height * 0.5f);
                const float progress = 1.0f - life;
                for (const Fragment& fragment : fragments)
                {
                    const float size = this->geometry_.particle_size * fragment.scale;
                    const float x = std::round(center.x + fragment.x *
                        (death.width * 0.25f + this->geometry_.elimination_spread * progress));
                    const float y = std::round(center.y + fragment.y *
                        (death.height * 0.25f + this->geometry_.elimination_spread * progress) -
                        this->geometry_.elimination_lift * 4.0f * progress * (1.0f - progress));
                    const RectangleF main(x - size * 0.5f, y - size * 0.5f,
                        size, size * (fragment.tall ? 1.25f : 0.75f));
                    const RectangleF corner(x, y, size * 0.75f, size * 0.75f);
                    border(main, life); border(corner, life);
                    fill(main, team(), life); fill(corner, team(), life);
                }
            }
        });
    }

    void check_team(const Frame& frame) const
    { require(monitor_rgb(frame, 510, 241) == 0x2CA8CE, "Combat tint changed an opaque full-life elimination interior"); }
    size_t readbacks() const { return this->readbacks_; }
    static RectangleI panel() { return { 180, 160, 420, 360 }; }

private:
    static Colour team() { return Colour(44, 168, 206); }
    static RectangleF body(size_t index)
    { return { index % 2 == 0 ? 240.0f : 480.0f, index < 2 ? 210.0f : 410.0f, 40.0f, 60.0f }; }
    static std::array<float, 6> state(const CombatFeedback& cue)
    {
        const RectangleF& death = cue.death_rectangle();
        return { cue.hurt_remaining(), cue.elimination_remaining(), death.x, death.y, death.width, death.height };
    }
    std::array<RectangleF, 4> bounds() const
    {
        std::array<RectangleF, 4> result;
        for (size_t index = 0; index < this->cues_.size(); ++index)
        { result[index] = this->cues_[index]->bounds(body(index)); }
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
    const CombatFeedbackPresentation& look_;
    CombatFeedbackPresentation geometry_;
    const SpriteSheet* sheet_;
    const SpriteFrame& frame_;
    std::array<std::unique_ptr<CombatFeedback>, 4> cues_;
    std::array<std::array<float, 6>, 4> states_;
    std::array<RectangleF, 4> bounds_;
    size_t readbacks_ = 0, effect_preparation_updates_ = 0;
};

// Shared evidence: 32 readbacks, eight rows/crops and eight controls. Release
// executes this consumer-only path with no developer director or commands.
void combat_colour_consumer_checks(Application& app, const Presentation& presentation,
    const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    Presentation local = presentation;
    auto& look = const_cast<CombatFeedbackPresentation&>(local.combat_feedback());
    require(look.highlight == Colour(255, 255, 255) && look.keyline == Colour(0, 0, 0),
        "Combat legacy parity no longer matches the opaque reviewed defaults");
    CombatColourProbe probe(app, look);
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
    std::ofstream records(output / "combat-colour-consumers.csv");
    records << "palette,background,normal_match,team_centres,frozen_geometry_timers_rng,control,scope\n";
    for (size_t palette = 0; palette < palettes.size(); ++palette)
    {
        look.highlight = palettes[palette].highlight; look.keyline = palettes[palette].keyline;
        for (size_t background = 0; background < backgrounds.size(); ++background)
        {
            const Frame actual = probe.actual(backgrounds[background]);
            require(actual.pixels == probe.reference(backgrounds[background], palettes[palette].highlight,
                palettes[palette].keyline).pixels, "Combat literal palette differs from independent raw-source oracle");
            probe.check_team(actual);
            const char* control = "changed-rgb";
            if (palette == 0)
            {
                defaults[background] = actual;
                require(actual.pixels == probe.reference(backgrounds[background], palettes[palette].highlight,
                    palettes[palette].keyline, CombatColourReference::legacy_straight).pixels,
                    "Premultiplication changed opaque-default combat pixels or fades"); ++controls;
                require(actual.pixels != probe.reference(backgrounds[background], palettes[palette].highlight,
                    palettes[palette].keyline, CombatColourReference::doubled_fade).pixels,
                    "Combat fixture cannot detect applying event fade twice"); ++controls;
                control = "opaque-legacy-parity-and-double-fade-rejected";
            }
            else
            {
                require(actual.pixels != defaults[background].pixels, "Literal colours did not reach preconstructed combat cues");
                if (palette >= 2)
                {
                    require(actual.pixels != probe.reference(backgrounds[background], palettes[palette].highlight,
                        palettes[palette].keyline, CombatColourReference::legacy_straight).pixels,
                        "Combat RGBA fixture cannot reject straight tint under ONE blending"); ++controls;
                    control = "legacy-straight-alpha-rejected";
                }
            }
            records << palettes[palette].name << ',' << (background == 0 ? "black" : "nonblack")
                << ",true,true,true," << control << ",consumer-only\n"; ++rows;
            save(foreground_crop(actual, CombatColourProbe::panel(), 0), output /
                (std::string("combat-colour-") + palettes[palette].name + (background == 0 ? "-black.ppm" : "-nonblack.ppm"))); ++crops;
        }
    }
    require(records.good() && probe.readbacks() == 32 && rows == 8 && crops == 8 && controls == 8,
        "Combat consumer evidence exceeded its bounded counts");
    std::ofstream summary(output / "combat-colour-consumer-summary.json");
    summary << "{\"scope\":\"consumer-only\",\"state_updates\":0,\"level_updates\":0,\"effect_preparation_updates\":2,"
        "\"readbacks\":32,\"csv_rows\":8,\"crops\":8,\"controls\":8}\n";
    require(summary.good(), "Cannot retain combat consumer summary");
    std::cout << "PASS combat colour consumers: 32 readbacks, eight compact crops; independent literal RGBA, "
        "opaque legacy parity, fade/straight-alpha controls, team interior and frozen public timers/death/bounds/RNG. "
        "Two local half-age preparations, zero Level/StateContext updates; no gameplay or input claims.\n";
}

#if COLOURWARS_DEV_TOOLS
// Debug adds 15 real commands, 60 readbacks and 20 CSV rows; no extra crops.
// The same four consumers borrow the live presentation before the first edit.
void combat_tune_colour_checks(Application& app, Registry<LevelLoadedInfo>& levels,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& content,
    const std::filesystem::path& output)
{
    const auto root = output / "combat-tune-scratch", source = root / "source", staged = root / "staged";
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
    struct RestoreCombatTuneContent
    {
        GameTuning& tuning; Presentation& presentation; GameTuning original_tuning; Presentation original;
        ~RestoreCombatTuneContent() { this->tuning = this->original_tuning; this->presentation = this->original; }
    } restore{ tuning, presentation, original_tuning, original };
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    Save preferences; PlaySettings play; play.music_volume = 17; play.sfx_volume = 83;
    play.aim_sensitivity = 4; play.view_shake = false; play.aim_marker = true; play.reduce_effects = true;
    play.sticks[2].movement_deadzone = 25; play.sticks[2].aim_deadzone = 70;
    require(preferences.set_play_settings_and_save(play) && preferences.set_display_settings_and_save(
        ScreenResolution::s_1280_720, false, TeamColourPair::blue_orange), "Cannot seed private combat tune preferences");
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
    require(impl.log().set_transcript((output / "combat-tune-commands.log").string()), "Cannot retain combat command transcript");
    CombatColourProbe probe(app, presentation.combat_feedback());
    const std::array<Colour, 2> backgrounds{ Colour(0, 0, 0), Colour(32, 64, 96) };
    size_t commands = 0, rows = 0; bool persisted = false;
    JsonSpanDocument first(real_bytes[1], "private combat source");
    const JsonLeaf* highlight = first.find("combat_feedback.highlight");
    require(highlight != nullptr, "Combat source has no highlight leaf");
    JsonSpanDocument second(first.replaced(*highlight, "\"2060C080\""), "private combat source");
    const JsonLeaf* keyline = second.find("combat_feedback.keyline");
    require(keyline != nullptr, "Combat source has no keyline leaf");
    const std::string expected_saved = second.replaced(*keyline, "\"C0602080\"");
    const auto unchanged = [&]
    {
        probe.unchanged();
        require(!impl.can_apply() && impl.request().empty() && !impl.in_match() &&
            tune_colour_bytes(save_path) == saved_preferences && preference_values() == initial_preferences,
            "Combat tune changed private preferences, queued work or entered a match");
        for (size_t index = 0; index < files.size(); ++index)
        {
            const std::string& expected = index == 1 && persisted ? expected_saved : real_bytes[index];
            require(tune_colour_bytes(content / files[index]) == real_bytes[index] &&
                tune_colour_bytes(source / files[index]) == expected && tune_colour_bytes(staged / files[index]) == expected,
                "Combat tune changed actual content or private bytes beyond its two saved tokens");
        }
    };
    const auto command = [&](const std::string& line)
    {
        const size_t previous_lines = impl.log().lines().size();
        std::srand(8451); const int expected_random = std::rand(); std::srand(8451);
        const DebugStatus result = impl.execute(line); ++commands;
        require(std::rand() == expected_random, "Combat tune command consumed match randomness");
        if (!result.ok) { std::cout << "COMMAND " << line << ": " << result.message << '\n'; }
        require(result.ok, "Validated combat tune/reset/save command was rejected");
        if (line.starts_with("tune "))
        {
            // Successful statement replies are written to the console log;
            // the aggregate execute result retains messages only for failures.
            const auto& lines = impl.log().lines();
            require(lines.size() > previous_lines && lines.back().severity == DebugSeverity::info &&
                lines.back().text.starts_with("tune: presentation.combat_feedback.") &&
                lines.back().text.ends_with("(live)"), "Combat colour command lost live adoption scope");
        }
    };
    const auto pair = [&](const char* highlight_word, const char* keyline_word)
    {
        command(std::string("tune presentation.combat_feedback.highlight ") + highlight_word);
        command(std::string("tune presentation.combat_feedback.keyline ") + keyline_word);
        const TuneDocument* doc = impl.tune_document("presentation");
        require(doc != nullptr && doc->edits().size() == 2 && doc->edits().contains("combat_feedback.highlight") &&
            doc->edits().contains("combat_feedback.keyline"), "Combat commands edited outside their two audited leaves");
    };
    std::ofstream records(output / "combat-tune-colours.csv");
    records << "state,background,normal_match,team_centres,frozen_geometry_timers_rng,private_bytes,adoption\n";
    const auto check = [&](const char* state, const Colour& expected_highlight, const Colour& expected_keyline)
    {
        unchanged();
        require(presentation.combat_feedback().highlight == expected_highlight && presentation.combat_feedback().keyline == expected_keyline,
            "Combat command parsed different straight RGBA channels than the literal oracle");
        for (size_t background = 0; background < backgrounds.size(); ++background)
        {
            const Frame actual = probe.actual(backgrounds[background]);
            require(actual.pixels == probe.reference(backgrounds[background], expected_highlight, expected_keyline).pixels,
                "Preconstructed combat did not adopt validated live colours with independent RGBA pixels");
            probe.check_team(actual);
            records << state << ',' << (background == 0 ? "black" : "nonblack") << ",true,true,true,true,live\n"; ++rows;
        }
        unchanged();
    };
    const Colour white(255, 255, 255), black(0, 0, 0), rgba_highlight(32, 96, 192, 128), rgba_keyline(192, 96, 32, 128);
    check("baseline", white, black);
    pair("RED", "BLUE"); check("named", Colour(255, 0, 0), Colour(0, 0, 255));
    command("tune.reset presentation.combat_feedback.highlight"); command("tune.reset presentation.combat_feedback.keyline");
    require(impl.tune_document("presentation")->edits().empty(), "Combat leaf reset retained edits");
    check("reset-original", white, black);
    pair("35BCE1", "9141C2"); check("rgb", Colour(53, 188, 225), Colour(145, 65, 194));
    pair("2060C080", "C0602080"); check("rgba128", rgba_highlight, rgba_keyline);
    command("tune.save presentation"); persisted = true;
    require(impl.tune_document("presentation")->edits().empty() &&
        impl.tune_document("presentation")->baseline().text() == expected_saved,
        "Combat save failed to rebase exactly its two private tokens");
    check("saved-rgba", rgba_highlight, rgba_keyline);
    pair("RED", "BLUE"); check("edited-after-save", Colour(255, 0, 0), Colour(0, 0, 255));
    command("tune.reset presentation"); check("reset-saved-rgba", rgba_highlight, rgba_keyline);
    pair("A532D600", "2A904000"); check("alpha0", Colour(165, 50, 214, 0), Colour(42, 144, 64, 0));
    command("tune.reset presentation"); check("reset-alpha0-to-saved", rgba_highlight, rgba_keyline);
    require(impl.tune_document("presentation")->edits().empty() && impl.tune_document("tuning")->edits().empty(),
        "Combat final reset left pending private edits");
    unchanged();
    require(records.good() && commands == 15 && probe.readbacks() == 60 && rows == 20,
        "Combat Debug command evidence exceeded its bounded counts");
    std::ofstream summary(output / "combat-tune-summary.json");
    summary << "{\"scope\":\"debug-validated-commands\",\"state_updates\":0,\"level_updates\":0,\"effect_preparation_updates\":2,\"commands\":15,\"readbacks\":60,\"csv_rows\":20,\"crops\":0}\n";
    require(summary.good(), "Cannot retain combat Debug command summary");
    std::cout << "PASS combat tune colours: 15 validated named/RGB/RGBA/reset/save commands, 60 readbacks, "
        "twenty CSV rows; preconstructed live consumers, two-token private save/rebase, frozen public timers/death/bounds/RNG and preferences. "
        "Two local half-age preparations; no Level/StateContext updates, match, hardware or global-input claims.\n";
}
#endif
