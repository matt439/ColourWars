// Narrow both-preset retained BlockedShotFeedback colour proof.
// Included after tune_colour_checks.h (Debug) and ordinary raster
// helpers. Two retained instances, four cardinal contacts each, one local age
// preparation; no Level/StateContext/input update or gameplay classification.
enum class BlockedColourReference { premultiplied, legacy_straight, doubled_fade };

class BlockedColourProbe
{
public:
    BlockedColourProbe(Application& app, const BlockedShotPresentation& look) :
        app_(app), look_(look), geometry_(look),
        sheet_(app.render_resources()->sprite_sheet(look.sprite_sheet)),
        frame_(this->sheet_->sprite_frame(this->sheet_->resolve_sprite_frame(look.fill_frame)))
    {
        require(look.enabled && look.duration > 0.0f && look.repeat_delay > 0.0f,
            "Blocked colour fixture requires enabled finite cues");
        for (size_t row = 0; row < this->feedback_.size(); ++row)
        {
            auto feedback = std::make_unique<BlockedShotFeedback>(&look, app.render_resources());
            for (size_t column = 0; column < 4; ++column)
            { feedback->emit({ point(row, column), normal(column), team(column), owner(column) }); }
            require(feedback->cues().size() == 4, "Blocked colour fixture failed to retain four detached contacts");
            if (row == 1) { feedback->update(look.duration * 0.5f); ++this->effect_preparation_updates_; }
            for (const auto& cue : feedback->cues())
            {
                const float progress = cue.age / look.duration;
                require(progress == (row == 0 ? 0.0f : 0.5f) &&
                    1.0f - progress * progress == (row == 0 ? 1.0f : 0.75f),
                    "Blocked half-age preparation lost its quadratic 0.75 fade");
            }
            const RectangleF bounds = feedback->bounds();
            const RectangleI crop = panel();
            require(bounds.x >= static_cast<float>(crop.x) && bounds.y >= static_cast<float>(crop.y) &&
                bounds.right() <= static_cast<float>(crop.right()) && bounds.bottom() <= static_cast<float>(crop.bottom()),
                "Blocked crop does not contain the complete retained cue-bound envelope");
            this->states_[row] = snapshot(*feedback); this->feedback_[row] = std::move(feedback);
        }
        require(this->effect_preparation_updates_ == 1, "Blocked local effect preparation budget changed");
    }

    void unchanged() const
    {
        const auto& a = this->look_; const auto& b = this->geometry_;
        require(a.enabled == b.enabled && a.sprite_sheet == b.sprite_sheet && a.fill_frame == b.fill_frame &&
            a.duration == b.duration && a.repeat_delay == b.repeat_delay && a.width == b.width &&
            a.reach == b.reach && a.bead_size == b.bead_size && a.stroke_width == b.stroke_width &&
            a.keyline_width == b.keyline_width, "Blocked colours changed geometry, lifetime, repeat policy or assets");
        for (size_t row = 0; row < this->feedback_.size(); ++row)
        { require(snapshot(*this->feedback_[row]) == this->states_[row],
            "Blocked colour drawing/edit changed contact tint/normal/owner/age, complete repeat budgets or bounds"); }
    }

    Frame actual(const Colour& background)
    {
        this->unchanged();
        std::srand(8451); const int expected_random = std::rand(); std::srand(8451);
        const auto draw = [&](DrawList& list)
        { for (const auto& feedback : this->feedback_) { feedback->draw(list); } };
        Frame first = this->render(background, draw);
        require(first.pixels == this->render(background, draw).pixels,
            "Frozen blocked colour consumers changed between render views");
        require(std::rand() == expected_random, "Blocked colour drawing consumed match randomness");
        this->unchanged(); return first;
    }

    Frame reference(const Colour& background, const Colour& highlight, const Colour& keyline,
        BlockedColourReference mode = BlockedColourReference::premultiplied)
    {
        // Independent raw atlas submissions and explicit channel arithmetic;
        // no production draw_block, premultiplication helper or stored Cue is
        // consulted for expected geometry, age or colour.
        return this->render(background, [&](DrawList& list)
        {
            const auto tint = [&](const Colour& colour, float fade)
            {
                if (mode == BlockedColourReference::legacy_straight)
                { return Colour(colour.r * fade, colour.g * fade, colour.b * fade, colour.a * fade); }
                const float rgb_fade = mode == BlockedColourReference::doubled_fade ? fade * fade : fade;
                return Colour(colour.r * colour.a * rgb_fade, colour.g * colour.a * rgb_fade,
                    colour.b * colour.a * rgb_fade, colour.a * fade);
            };
            const auto block = [&](const Vector2F& center, const Vector2F& size,
                float angle, const Colour& colour, float fade)
            {
                const RectangleI& source = this->frame_.source_rectangle();
                const Vector2F origin = this->frame_.origin() + Vector2F(
                    static_cast<float>(source.width) * 0.5f, static_cast<float>(source.height) * 0.5f);
                const float border = this->geometry_.keyline_width * 2.0f;
                list.draw_sprite(this->sheet_->texture(), source, RectangleF(center, size + Vector2F(border, border)),
                    tint(keyline, fade), angle, origin, SpriteFlip::none, 0.0f);
                list.draw_sprite(this->sheet_->texture(), source, RectangleF(center, size),
                    tint(colour, fade), angle, origin, SpriteFlip::none, 0.0f);
            };
            for (size_t row = 0; row < 2; ++row)
            {
                const float progress = row == 0 ? 0.0f : 0.5f;
                const float fade = row == 0 ? 1.0f : 0.75f; // 1 - p*p, not 1 - p.
                for (size_t column = 0; column < 4; ++column)
                {
                    const Vector2F outward = normal(column), side(-outward.y, outward.x);
                    const Vector2F contact = point(row, column);
                    const float angle = std::atan2(outward.y, outward.x);
                    const float stroke = this->geometry_.stroke_width, edge = this->geometry_.keyline_width;
                    block(contact + outward * (edge + stroke * 0.5f),
                        { stroke, this->geometry_.width }, angle, highlight, fade);
                    block(contact + outward * (edge + stroke * 1.5f),
                        { stroke * 2.0f, stroke }, angle, highlight, fade);
                    for (size_t bead = 0; bead < 2; ++bead)
                    {
                        const float sign = bead == 0 ? -1.0f : 1.0f;
                        const float scale = bead == 0 ? 1.0f : 0.8f;
                        const float reach = stroke + this->geometry_.bead_size +
                            this->geometry_.reach * progress * (2.0f - progress) * scale;
                        const float spread = sign * this->geometry_.width * (0.2f + progress * 0.25f);
                        const float size = this->geometry_.bead_size * scale * (1.0f - progress * 0.25f);
                        block(contact + outward * reach + side * spread,
                            { size, size * 0.8f }, angle + sign * 0.3f, team(column), fade);
                    }
                }
            }
        });
    }

    void check_stored_team_beads(const Frame& frame) const
    {
        constexpr std::array<std::uint32_t, 4> expected{ 0x2CA8CE, 0xF58F40, 0xA156BE, 0x4FB84A };
        for (size_t column = 0; column < 4; ++column)
        {
            const Vector2F outward = normal(column), side(-outward.y, outward.x);
            const Vector2F center = point(0, column) + outward *
                (this->geometry_.stroke_width + this->geometry_.bead_size) - side * (this->geometry_.width * 0.2f);
            require(monitor_rgb(frame, static_cast<int>(center.x), static_cast<int>(center.y)) == expected[column],
                "Blocked colour edit changed or erased an opaque full-age stored team bead");
        }
    }

    size_t readbacks() const { return this->readbacks_; }
    static RectangleI panel() { return { 180, 220, 440, 240 }; }
    static constexpr size_t owner_budgets_per_instance = static_cast<size_t>(paint_owners::count + 1);

private:
    static Vector2F point(size_t row, size_t column)
    { return { 220.0f + static_cast<float>(column) * 120.0f, row == 0 ? 260.0f : 420.0f }; }
    static Vector2F normal(size_t column)
    {
        const std::array<Vector2F, 4> normals{ Vector2F(-1, 0), Vector2F(1, 0), Vector2F(0, -1), Vector2F(0, 1) };
        return normals.at(column);
    }
    static Colour team(size_t column)
    {
        const std::array<Colour, 4> colours{ Colour(44, 168, 206), Colour(245, 143, 64),
            Colour(161, 86, 190), Colour(79, 184, 74) };
        return colours.at(column);
    }
    static int owner(size_t column)
    {
        constexpr std::array<int, 4> owners{ 0, input_slots::count, paint_owners::count - 1, -1 };
        return owners.at(column);
    }
    static std::vector<float> snapshot(const BlockedShotFeedback& feedback)
    {
        std::vector<float> result{ static_cast<float>(feedback.cues().size()) };
        const auto rectangle = [&](const RectangleF& value)
        { result.insert(result.end(), { value.x, value.y, value.width, value.height }); };
        for (const auto& cue : feedback.cues())
        {
            const auto& contact = cue.contact;
            result.insert(result.end(), { contact.position.x, contact.position.y, contact.normal.x, contact.normal.y,
                contact.team_colour.r, contact.team_colour.g, contact.team_colour.b, contact.team_colour.a,
                static_cast<float>(contact.player_num), cue.age });
            rectangle(feedback.cue_bounds(cue));
        }
        // The five input slots are not the complete owner domain. Device-less
        // owners and the shared invalid-owner bucket must also stay frozen.
        for (int paint_owner = 0; paint_owner < paint_owners::count; ++paint_owner)
        { result.push_back(feedback.repeat_remaining(paint_owner)); }
        result.push_back(feedback.repeat_remaining(-1));
        require(feedback.repeat_remaining(-1) == feedback.repeat_remaining(paint_owners::count),
            "Blocked invalid-owner aliases disagree");
        rectangle(feedback.bounds()); return result;
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
    const BlockedShotPresentation& look_;
    BlockedShotPresentation geometry_;
    const SpriteSheet* sheet_;
    const SpriteFrame& frame_;
    std::array<std::unique_ptr<BlockedShotFeedback>, 2> feedback_;
    std::array<std::vector<float>, 2> states_;
    size_t effect_preparation_updates_ = 0, readbacks_ = 0;
};

// Shared budget: exactly four readbacks for each palette/underlay
// pair: duplicate actuals, independent correct reference and one control.
void blocked_colour_consumer_checks(Application& app, const Presentation& presentation,
    const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    Presentation local = presentation;
    auto& look = const_cast<BlockedShotPresentation&>(local.blocked_shot());
    require(look.highlight == Colour(255, 255, 255) && look.keyline == Colour(0, 0, 0),
        "Blocked opaque-default parity no longer matches the reviewed authored colours");
    BlockedColourProbe probe(app, look);
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
    std::ofstream records(output / "blocked-colour-consumers.csv");
    records << "palette,background,normal_match,stored_team_beads,complete_owner_cue_bounds_rng,control,scope\n";
    for (size_t palette = 0; palette < palettes.size(); ++palette)
    {
        look.highlight = palettes[palette].highlight; look.keyline = palettes[palette].keyline;
        for (size_t background = 0; background < backgrounds.size(); ++background)
        {
            const Frame actual = probe.actual(backgrounds[background]);
            require(actual.pixels == probe.reference(backgrounds[background], palettes[palette].highlight,
                palettes[palette].keyline).pixels, "Blocked literal RGBA differs from independent quadratic-fade raw oracle");
            probe.check_stored_team_beads(actual);
            const char* control = nullptr;
            if (palette == 0)
            {
                defaults[background] = actual;
                require(actual.pixels == probe.reference(backgrounds[background], palettes[palette].highlight,
                    palettes[palette].keyline, BlockedColourReference::legacy_straight).pixels,
                    "Premultiplication changed opaque-default blocked pixels or quadratic fade");
                control = "opaque-legacy-parity";
            }
            else
            {
                require(actual.pixels != defaults[background].pixels, "Blocked colours did not reach retained contacts");
                const BlockedColourReference negative = palette == 1 ?
                    BlockedColourReference::doubled_fade : BlockedColourReference::legacy_straight;
                require(actual.pixels != probe.reference(backgrounds[background], palettes[palette].highlight,
                    palettes[palette].keyline, negative).pixels, "Blocked fixture cannot distinguish its wrong-alpha/fade control");
                control = palette == 1 ? "double-fade-rejected-and-changed-rgb" : "legacy-straight-alpha-rejected";
            }
            ++controls;
            records << palettes[palette].name << ',' << (background == 0 ? "black" : "nonblack")
                << ",true,true,true," << control << ",consumer-only\n"; ++rows;
            save(foreground_crop(actual, BlockedColourProbe::panel(), 0), output /
                (std::string("blocked-colour-") + palettes[palette].name + (background == 0 ? "-black.ppm" : "-nonblack.ppm"))); ++crops;
        }
    }
    require(records.good() && probe.readbacks() == 32 && rows == 8 && crops == 8 && controls == 8,
        "Blocked shared consumer evidence exceeded its bounded budget");
    std::ofstream summary(output / "blocked-colour-consumer-summary.json");
    summary << "{\"scope\":\"consumer-only\",\"state_updates\":0,\"level_updates\":0,\"effect_preparation_updates\":1,"
        "\"owner_budgets_per_instance\":" << BlockedColourProbe::owner_budgets_per_instance << ','
        << "\"readbacks\":32,\"csv_rows\":8,\"crops\":8,\"controls\":8}\n";
    require(summary.good(), "Cannot retain blocked colour consumer summary");
    std::cout << "PASS blocked colour consumers:32 readbacks,eight compact crops; four normals, full/half ages with0.75 fade, "
        "independent RGBA geometry/channels, opaque parity, alpha/fade controls, stored team beads and complete owner/cue/bounds/RNG snapshots. "
        "One local age preparation, zero Level/StateContext updates; no gameplay or input claims.\n";
}

#if COLOURWARS_DEV_TOOLS
// Debug adds 15 real commands, 60 readbacks and 20 CSV rows; no extra crops.
// The same two four-contact instances borrow the live presentation before the first edit.
void blocked_tune_colour_checks(Application& app, Registry<LevelLoadedInfo>& levels,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& content,
    const std::filesystem::path& output)
{
    const auto root = output / "blocked-tune-scratch", source = root / "source", staged = root / "staged";
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
    struct RestoreBlockedTuneContent
    {
        GameTuning& tuning; Presentation& presentation; GameTuning original_tuning; Presentation original;
        ~RestoreBlockedTuneContent() { this->tuning = this->original_tuning; this->presentation = this->original; }
    } restore{ tuning, presentation, original_tuning, original };
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    Save preferences; SettingsSession preferences_session(preferences); PlaySettings play; play.music_volume = 17; play.sfx_volume = 83;
    play.aim_sensitivity = 4; play.view_shake = false; play.aim_marker = true; play.reduce_effects = true;
    play.sticks[2].movement_deadzone = 25; play.sticks[2].aim_deadzone = 70;
    require(preferences_session.set_play_settings_and_save(play) && preferences.set_display_settings_and_save(
        ScreenResolution::s_1280_720, false, TeamColourPair::blue_orange), "Cannot seed private blocked tune preferences");
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
    require(impl.log().set_transcript((output / "blocked-tune-commands.log").string()), "Cannot retain blocked command transcript");
    BlockedColourProbe probe(app, presentation.blocked_shot());
    const std::array<Colour, 2> backgrounds{ Colour(0, 0, 0), Colour(32, 64, 96) };
    size_t commands = 0, rows = 0; bool persisted = false;
    JsonSpanDocument first(real_bytes[1], "private blocked source");
    const JsonLeaf* highlight = first.find("blocked_shot.highlight");
    require(highlight != nullptr, "Blocked source has no highlight leaf");
    JsonSpanDocument second(first.replaced(*highlight, "\"2060C080\""), "private blocked source");
    const JsonLeaf* keyline = second.find("blocked_shot.keyline");
    require(keyline != nullptr, "Blocked source has no keyline leaf");
    const std::string expected_saved = second.replaced(*keyline, "\"C0602080\"");
    const auto unchanged = [&]
    {
        probe.unchanged();
        require(!impl.can_apply() && impl.request().empty() && !impl.in_match() &&
            tune_colour_bytes(save_path) == saved_preferences && preference_values() == initial_preferences,
            "Blocked tune changed private preferences, queued work or entered a match");
        for (size_t index = 0; index < files.size(); ++index)
        {
            const std::string& expected = index == 1 && persisted ? expected_saved : real_bytes[index];
            require(tune_colour_bytes(content / files[index]) == real_bytes[index] &&
                tune_colour_bytes(source / files[index]) == expected && tune_colour_bytes(staged / files[index]) == expected,
                "Blocked tune changed actual content or private bytes beyond its two saved tokens");
        }
    };
    const auto command = [&](const std::string& line)
    {
        const size_t previous_lines = impl.log().lines().size();
        std::srand(8451); const int expected_random = std::rand(); std::srand(8451);
        const DebugStatus result = impl.execute(line); ++commands;
        require(std::rand() == expected_random, "Blocked tune command consumed match randomness");
        if (!result.ok) { std::cout << "COMMAND " << line << ": " << result.message << '\n'; }
        require(result.ok, "Validated blocked tune/reset/save command was rejected");
        if (line.starts_with("tune "))
        {
            // Successful statement replies are written to the console log;
            // the aggregate execute result retains messages only for failures.
            const auto& lines = impl.log().lines();
            require(lines.size() > previous_lines && lines.back().severity == DebugSeverity::info &&
                lines.back().text.starts_with("tune: presentation.blocked_shot.") &&
                lines.back().text.ends_with("(live)"), "Blocked colour command lost live adoption scope");
        }
    };
    const auto pair = [&](const char* highlight_word, const char* keyline_word)
    {
        command(std::string("tune presentation.blocked_shot.highlight ") + highlight_word);
        command(std::string("tune presentation.blocked_shot.keyline ") + keyline_word);
        const TuneDocument* doc = impl.tune_document("presentation");
        require(doc != nullptr && doc->edits().size() == 2 && doc->edits().contains("blocked_shot.highlight") &&
            doc->edits().contains("blocked_shot.keyline"), "Blocked commands edited outside their two audited leaves");
    };
    std::ofstream records(output / "blocked-tune-colours.csv");
    records << "state,background,normal_match,stored_team_beads,complete_owner_cue_bounds_rng,private_bytes,adoption\n";
    const auto check = [&](const char* state, const Colour& expected_highlight, const Colour& expected_keyline)
    {
        unchanged();
        require(presentation.blocked_shot().highlight == expected_highlight && presentation.blocked_shot().keyline == expected_keyline,
            "Blocked command parsed different straight RGBA channels than the literal oracle");
        for (size_t background = 0; background < backgrounds.size(); ++background)
        {
            const Frame actual = probe.actual(backgrounds[background]);
            require(actual.pixels == probe.reference(backgrounds[background], expected_highlight, expected_keyline).pixels,
                "Preconstructed blocked did not adopt validated live colours with independent RGBA pixels");
            probe.check_stored_team_beads(actual);
            records << state << ',' << (background == 0 ? "black" : "nonblack") << ",true,true,true,true,live\n"; ++rows;
        }
        unchanged();
    };
    const Colour white(255, 255, 255), black(0, 0, 0), rgba_highlight(32, 96, 192, 128), rgba_keyline(192, 96, 32, 128);
    check("baseline", white, black);
    pair("RED", "BLUE"); check("named", Colour(255, 0, 0), Colour(0, 0, 255));
    command("tune.reset presentation.blocked_shot.highlight"); command("tune.reset presentation.blocked_shot.keyline");
    require(impl.tune_document("presentation")->edits().empty(), "Blocked leaf reset retained edits");
    check("reset-original", white, black);
    pair("35BCE1", "9141C2"); check("rgb", Colour(53, 188, 225), Colour(145, 65, 194));
    pair("2060C080", "C0602080"); check("rgba128", rgba_highlight, rgba_keyline);
    command("tune.save presentation"); persisted = true;
    require(impl.tune_document("presentation")->edits().empty() &&
        impl.tune_document("presentation")->baseline().text() == expected_saved,
        "Blocked save failed to rebase exactly its two private tokens");
    check("saved-rgba", rgba_highlight, rgba_keyline);
    pair("RED", "BLUE"); check("edited-after-save", Colour(255, 0, 0), Colour(0, 0, 255));
    command("tune.reset presentation"); check("reset-saved-rgba", rgba_highlight, rgba_keyline);
    pair("A532D600", "2A904000"); check("alpha0", Colour(165, 50, 214, 0), Colour(42, 144, 64, 0));
    command("tune.reset presentation"); check("reset-alpha0-to-saved", rgba_highlight, rgba_keyline);
    require(impl.tune_document("presentation")->edits().empty() && impl.tune_document("tuning")->edits().empty(),
        "Blocked final reset left pending private edits");
    unchanged();
    require(records.good() && commands == 15 && probe.readbacks() == 60 && rows == 20,
        "Blocked Debug command evidence exceeded its bounded counts");
    std::ofstream summary(output / "blocked-tune-summary.json");
    summary << "{\"scope\":\"debug-validated-commands\",\"state_updates\":0,\"level_updates\":0,\"effect_preparation_updates\":1,"
        "\"owner_budgets_per_instance\":" << BlockedColourProbe::owner_budgets_per_instance << ','
        << "\"commands\":15,\"readbacks\":60,\"csv_rows\":20,\"crops\":0}\n";
    require(summary.good(), "Cannot retain blocked Debug command summary");
    std::cout << "PASS blocked tune colours: 15 validated named/RGB/RGBA/reset/save commands, 60 readbacks, "
        "twenty CSV rows; preconstructed live consumers, two-token private save/rebase, frozen stored contacts/complete owner budgets/bounds/RNG and preferences. "
        "One local half-age preparation; no Level/StateContext updates, match, hardware or global-input claims.\n";
}
#endif
