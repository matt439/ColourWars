// Narrow both-preset live common IdleDrip keyline consumer proof.
// Six retained attached/falling Sprayer/Sniper/Bucket consumers; no Level.
// Local advance calls prepare public cosmetic state only. Drawing's private
// xorshift invariance is source-audited constness, not an executed stream test.
enum class IdleDripColourReference { premultiplied, legacy_straight, squared_base_alpha };
enum class IdleDripColourGuard { enabled, reduced };

class IdleDripColourProbe
{
public:
    IdleDripColourProbe(Application& app, Presentation& presentation) :
        app_(app), presentation_(presentation),
        sheet_(app.render_resources()->sprite_sheet(presentation.idle_drip().sprite_sheet)),
        frame_(this->sheet_->sprite_frame(this->sheet_->resolve_sprite_frame(presentation.idle_drip().fill_frame)))
    {
        static_assert(static_cast<size_t>(WeaponType::max_prim_wep) == 5);
        for (size_t index = 0; index < this->records_.size(); ++index)
        {
            this->records_[index] = &const_cast<IdleDripPresentation&>(presentation.idle_drip(static_cast<WeaponType>(index)));
            this->geometry_[index] = *this->records_[index];
            require(this->records_[index]->enabled == (index == 0 || index == 1 || index == 4),
                "Idle colour fixture requires only Sprayer/Sniper/Bucket enabled");
        }
        std::srand(8451); const int expected_random = std::rand(); std::srand(8451);
        for (size_t index = 0; index < this->cues_.size(); ++index)
        {
            const size_t column = index % 3, profile = profile_index(column);
            const auto& look = this->geometry_[profile];
            require(look.interval_min > look.lifetime * 0.5f && look.grow_time > 0.0f && look.stretch_time > 0.0f,
                "Idle fixture cannot prepare separate natural attached/falling consumers");
            auto cue = std::make_unique<IdleDrip>(this->records_[profile], app.render_resources(), team(column),
                111u + static_cast<uint32_t>(index), &this->preferences_);
            const Vector2F point = nozzle(index);
            this->prepare(*cue, point, cue->interval_remaining());
            if (index < 3)
            {
                const float age = look.grow_time + look.stretch_time * 0.5f;
                this->prepare(*cue, point, age);
                require(cue->attached() && cue->bead_age() == age && cue->drops().empty(),
                    "Idle attached fixture did not retain its mature stretched bead");
            }
            else
            {
                this->prepare(*cue, point, look.grow_time + look.stretch_time);
                this->prepare(*cue, point, look.lifetime * 0.5f);
                require(!cue->attached() && cue->drops().size() == 1 && cue->drops().front().age == look.lifetime * 0.5f,
                    "Idle falling fixture did not retain exactly one half-lifetime drop");
            }
            const RectangleF bounds = cue->bounds(); const RectangleI crop = panel();
            require(bounds.x >= static_cast<float>(crop.x) && bounds.y >= static_cast<float>(crop.y) &&
                bounds.right() <= static_cast<float>(crop.right()) && bounds.bottom() <= static_cast<float>(crop.bottom()),
                "Idle colour crop does not contain every retained bead/drop bound");
            this->states_[index] = snapshot(*cue); this->cues_[index] = std::move(cue);
        }
        require(this->preparation_updates_ == 15 && std::rand() == expected_random,
            "Idle local preparation count changed or used gameplay randomness");
    }

    void set_keyline(const Colour& colour)
    { for (IdleDripPresentation* record : this->records_) { record->keyline = colour; } }
    void common(const Colour& expected) const
    {
        for (const IdleDripPresentation* record : this->records_)
        { require(record->keyline == expected, "The one common idle colour did not reach all five profile records"); }
    }
    void unchanged(bool disabled = false, bool reduced = false) const
    {
        for (size_t index = 0; index < this->records_.size(); ++index)
        {
            const auto& a = *this->records_[index]; const auto& b = this->geometry_[index];
            require(this->records_[index] == &this->presentation_.idle_drip(static_cast<WeaponType>(index)) &&
                a.enabled == (disabled ? false : b.enabled) && a.sprite_sheet == b.sprite_sheet && a.fill_frame == b.fill_frame &&
                a.interval_min == b.interval_min && a.interval_max == b.interval_max && a.grow_time == b.grow_time &&
                a.stretch_time == b.stretch_time && a.lifetime == b.lifetime && a.gravity == b.gravity &&
                a.size == b.size && a.stretch == b.stretch && a.nozzle_inset == b.nozzle_inset &&
                a.nozzle_underside == b.nozzle_underside && a.keyline_width == b.keyline_width,
                "Idle colour edit changed profile addresses, enabled policy, assets or geometry");
        }
        require(this->preferences_.reduce_effects == reduced, "Idle fixture leaked its borrowed Reduce Effects guard");
        for (size_t index = 0; index < this->cues_.size(); ++index)
        { require(snapshot(*this->cues_[index]) == this->states_[index],
            "Idle colour draw/edit changed public attachment/timers/drop/bounds state"); }
    }
    Frame actual(const Colour& background) { return this->frozen(background); }
    Frame suppressed(const Colour& background, IdleDripColourGuard guard)
    {
        this->unchanged();
        const auto restore = [&]
        {
            for (size_t index = 0; index < this->records_.size(); ++index)
            { this->records_[index]->enabled = this->geometry_[index].enabled; }
            this->preferences_.reduce_effects = false;
        };
        if (guard == IdleDripColourGuard::enabled)
        { for (IdleDripPresentation* record : this->records_) { record->enabled = false; } }
        else { this->preferences_.reduce_effects = true; }
        try
        {
            Frame result = this->frozen(background, guard == IdleDripColourGuard::enabled,
                guard == IdleDripColourGuard::reduced);
            restore(); this->unchanged(); return result;
        }
        catch (...) { restore(); throw; }
    }
    Frame blank(const Colour& background) { return this->render(background, [](DrawList&) {}); }
    Frame reference(const Colour& background, const Colour& keyline,
        IdleDripColourReference mode = IdleDripColourReference::premultiplied)
    {
        // No event-age fade: only the authored keyline's base alpha is
        // converted once. Captured opaque paint is submitted unchanged.
        Colour line(keyline.r * keyline.a, keyline.g * keyline.a, keyline.b * keyline.a, keyline.a);
        if (mode == IdleDripColourReference::legacy_straight) { line = keyline; }
        else if (mode == IdleDripColourReference::squared_base_alpha)
        { line = Colour(keyline.r * keyline.a * keyline.a, keyline.g * keyline.a * keyline.a,
            keyline.b * keyline.a * keyline.a, keyline.a); }
        return this->render(background, [&](DrawList& list)
        {
            for (size_t index = 0; index < this->cues_.size(); ++index)
            {
                const RectangleF inside = this->reference_rectangle(index);
                const float edge = this->geometry_[profile_index(index % 3)].keyline_width;
                const RectangleF outer(inside.x - edge, inside.y - edge, inside.width + edge * 2.0f, inside.height + edge * 2.0f);
                list.draw_sprite(this->sheet_->texture(), this->frame_.source_rectangle(), outer, line,
                    0.0f, this->frame_.origin(), SpriteFlip::none, 0.0f);
                list.draw_sprite(this->sheet_->texture(), this->frame_.source_rectangle(), inside, team(index % 3),
                    0.0f, this->frame_.origin(), SpriteFlip::none, 0.0f);
            }
        });
    }
    void check_paint(const Frame& frame) const
    {
        constexpr std::array<std::uint32_t, 3> colours{ 0x2CA8CE, 0xF58F40, 0xA156BE };
        for (size_t index = 0; index < this->cues_.size(); ++index)
        {
            const RectangleF inside = this->reference_rectangle(index);
            require(monitor_rgb(frame, static_cast<int>(inside.x + inside.width * 0.5f),
                static_cast<int>(inside.y + inside.height * 0.5f)) == colours[index % 3],
                "Idle keyline edit recoloured, faded or erased captured opaque attached/falling paint");
        }
    }
    size_t readbacks() const { return this->readbacks_; }
    size_t preparation_updates() const { return this->preparation_updates_; }
    static RectangleI panel() { return { 200, 230, 380, 260 }; }

private:
    static size_t profile_index(size_t column)
    {
        constexpr std::array<size_t, 3> indices{ 0, 1, 4 }; return indices.at(column);
    }
    static Colour team(size_t column)
    {
        const std::array<Colour, 3> colours{ Colour(44, 168, 206), Colour(245, 143, 64), Colour(161, 86, 190) };
        return colours.at(column);
    }
    static Vector2F nozzle(size_t index)
    { return { 240.0f + static_cast<float>(index % 3) * 180.0f, index < 3 ? 260.0f : 390.0f }; }
    void prepare(IdleDrip& cue, const Vector2F& point, float elapsed)
    { cue.advance(true, point, 0.0f, elapsed, {}); ++this->preparation_updates_; }
    RectangleF reference_rectangle(size_t index) const
    {
        const auto& look = this->geometry_[profile_index(index % 3)];
        const Vector2F point = nozzle(index);
        const Vector2F attachment(point.x - look.nozzle_inset, point.y + look.nozzle_underside);
        const float age = look.grow_time + look.stretch_time * (index < 3 ? 0.5f : 1.0f);
        const float growth = std::clamp(age / look.grow_time, 0.0f, 1.0f);
        const float stretch = std::clamp((age - look.grow_time) / look.stretch_time, 0.0f, 1.0f);
        const float width = look.size * (0.35f + 0.65f * growth);
        const RectangleF bead(attachment.x - width * 0.5f, attachment.y, width, width + stretch * look.stretch);
        if (index < 3) { return bead; }
        Vector2F position(bead.x + bead.width * 0.5f, bead.y + bead.height * 0.5f);
        const float elapsed = look.lifetime * 0.5f;
        position.y += look.gravity * (0.0f * elapsed + 0.5f * elapsed * elapsed);
        return { position.x - look.size * 0.5f, position.y - look.size * 0.5f, look.size, look.size };
    }
    static std::vector<float> snapshot(const IdleDrip& cue)
    {
        const RectangleF bead = cue.bead_rectangle(), bounds = cue.bounds();
        std::vector<float> result{ cue.attached() ? 1.0f : 0.0f, cue.bead_age(), cue.interval_remaining(),
            bead.x, bead.y, bead.width, bead.height, bounds.x, bounds.y, bounds.width, bounds.height,
            static_cast<float>(cue.drops().size()) };
        for (const auto& drop : cue.drops()) { result.insert(result.end(), { drop.position.x, drop.position.y, drop.age }); }
        return result;
    }
    Frame frozen(const Colour& background, bool disabled = false, bool reduced = false)
    {
        this->unchanged(disabled, reduced);
        std::srand(8451); const int expected_random = std::rand(); std::srand(8451);
        const auto draw = [&](DrawList& list) { for (const auto& cue : this->cues_) { cue->draw(list); } };
        Frame first = this->render(background, draw);
        require(first.pixels == this->render(background, draw).pixels && std::rand() == expected_random,
            "Idle frozen drawing changed pixels or gameplay RNG");
        this->unchanged(disabled, reduced); return first;
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
    Presentation& presentation_;
    PlaySettings preferences_;
    const SpriteSheet* sheet_;
    const SpriteFrame& frame_;
    std::array<IdleDripPresentation*, 5> records_{};
    std::array<IdleDripPresentation, 5> geometry_;
    std::array<std::unique_ptr<IdleDrip>, 6> cues_;
    std::array<std::vector<float>, 6> states_;
    size_t preparation_updates_ = 0, readbacks_ = 0;
};

// 32 palette/parity/control readbacks plus 14 disabled/reduced/restored ones.
// Exactly 15 local advance preparations, no Level/StateContext updates.
void idle_drip_colour_consumer_checks(Application& app, const Presentation& presentation,
    const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    Presentation local = presentation; IdleDripColourProbe probe(app, local);
    probe.common(Colour(0, 0, 0));
    struct Palette { const char* name; Colour keyline; };
    const std::array<Palette, 4> palettes{{ { "default", Colour(0, 0, 0) },
        { "rgb", Colour(53, 188, 225) }, { "rgba128", Colour(32, 96, 192, 128) },
        { "alpha0", Colour(165, 50, 214, 0) } }};
    const std::array<Colour, 2> backgrounds{ Colour(0, 0, 0), Colour(32, 64, 96) };
    std::array<Frame, 2> defaults;
    size_t rows = 0, guard_rows = 0, crops = 0, controls = 0, guard_checks = 0;
    std::ofstream records(output / "idle-drip-colour-consumers.csv");
    records << "palette,background,normal_match,captured_paint,all_profiles_public_state_rng,control,scope\n";
    std::ofstream guards(output / "idle-drip-colour-guards.csv");
    guards << "background,disabled_blank,reduced_blank,restored_match,profiles_state_restored,scope\n";
    for (size_t palette = 0; palette < palettes.size(); ++palette)
    {
        probe.set_keyline(palettes[palette].keyline); probe.common(palettes[palette].keyline);
        for (size_t background = 0; background < backgrounds.size(); ++background)
        {
            const Frame actual = probe.actual(backgrounds[background]);
            require(actual.pixels == probe.reference(backgrounds[background], palettes[palette].keyline).pixels,
                "Idle keyline differs from independent raw geometry/base-alpha reference");
            probe.check_paint(actual);
            const char* control = "changed-rgb";
            if (palette == 0)
            {
                defaults[background] = actual;
                require(actual.pixels == probe.reference(backgrounds[background], palettes[palette].keyline,
                    IdleDripColourReference::legacy_straight).pixels,
                    "Idle premultiplication changed opaque-default pixels"); ++controls;
                control = "opaque-legacy-parity";
            }
            else
            {
                if (palette != 3 || background != 0)
                { require(actual.pixels != defaults[background].pixels, "Idle keyline did not reach preconstructed consumers"); }
                if (palette >= 2)
                {
                    require(actual.pixels != probe.reference(backgrounds[background], palettes[palette].keyline,
                        IdleDripColourReference::legacy_straight).pixels,
                        "Idle fixture cannot reject straight base-alpha tint"); ++controls;
                    control = "straight-alpha-rejected-and-paint-retained";
                }
                if (palette == 2)
                {
                    require(actual.pixels != probe.reference(backgrounds[background], palettes[palette].keyline,
                        IdleDripColourReference::squared_base_alpha).pixels,
                        "Idle fixture cannot reject applying the keyline base alpha twice"); ++controls;
                    control = "straight-and-squared-base-alpha-rejected";
                    const Frame blank = probe.blank(backgrounds[background]);
                    const Frame disabled = probe.suppressed(backgrounds[background], IdleDripColourGuard::enabled);
                    const Frame reduced = probe.suppressed(backgrounds[background], IdleDripColourGuard::reduced);
                    const Frame restored = probe.actual(backgrounds[background]);
                    require(disabled.pixels == blank.pixels && reduced.pixels == blank.pixels && restored.pixels == actual.pixels,
                        "Idle enabled/Reduce Effects guards leaked sprites, changed state or failed restoration");
                    probe.check_paint(restored); probe.unchanged(); guard_checks += 3;
                    guards << (background == 0 ? "black" : "nonblack") << ",true,true,true,true,consumer-guards\n"; ++guard_rows;
                }
            }
            records << palettes[palette].name << ',' << (background == 0 ? "black" : "nonblack")
                << ",true,true,true," << control << ",consumer-only\n"; ++rows;
            save(foreground_crop(actual, IdleDripColourProbe::panel(), 0), output /
                (std::string("idle-drip-colour-") + palettes[palette].name + (background == 0 ? "-black.ppm" : "-nonblack.ppm"))); ++crops;
        }
    }
    require(records.good() && guards.good() && probe.readbacks() == 46 && probe.preparation_updates() == 15 &&
        rows == 8 && guard_rows == 2 && crops == 8 && controls == 8 && guard_checks == 6,
        "Idle consumer evidence exceeded its bounded counts");
    std::ofstream summary(output / "idle-drip-colour-consumer-summary.json");
    summary << "{\"scope\":\"consumer-only\",\"state_updates\":0,\"level_updates\":0,\"effect_preparation_updates\":15,"
        "\"profiles\":5,\"consumers\":6,\"readbacks\":46,\"csv_rows\":10,\"colour_rows\":8,\"guard_rows\":2,\"crops\":8,"
        "\"controls\":8,\"guard_checks\":6,\"private_sequence_scope\":\"source-constness-only\"}\n";
    require(summary.good(), "Cannot retain idle consumer summary");
    std::cout << "PASS idle-drip colour consumers:46 readbacks,ten rows,eight crops; six attached/falling consumers, "
        "five stable profiles, single base-alpha conversion/no event fade, captured paint and enabled/Reduce Effects guards. "
        "Fifteen local advance preparations, zero Level/StateContext updates; private stream source constness only.\n";
}

#if COLOURWARS_DEV_TOOLS
// Debug adds nine real commands, 60 readbacks and 20 CSV rows; no extra crops.
// All six consumers borrow their profile records before the first edit.
void idle_drip_tune_colour_checks(Application& app, Registry<LevelLoadedInfo>& levels,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& content,
    const std::filesystem::path& output)
{
    const auto root = output / "idle-drip-tune-scratch", source = root / "source", staged = root / "staged";
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
    struct RestoreIdleDripTuneContent
    {
        GameTuning& tuning; Presentation& presentation; GameTuning original_tuning; Presentation original;
        ~RestoreIdleDripTuneContent() { this->tuning = this->original_tuning; this->presentation = this->original; }
    } restore{ tuning, presentation, original_tuning, original };
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    Save preferences; PlaySettings play; play.music_volume = 17; play.sfx_volume = 83;
    play.aim_sensitivity = 4; play.view_shake = false; play.aim_marker = true; play.reduce_effects = true;
    play.sticks[2].movement_deadzone = 25; play.sticks[2].aim_deadzone = 70;
    require(preferences.set_play_settings_and_save(play) && preferences.set_display_settings_and_save(
        ScreenResolution::s_1280_720, false, TeamColourPair::blue_orange), "Cannot seed private idle drip tune preferences");
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
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_level_infos(&levels);
    data.set_stages(&stages); data.set_tuning(&tuning); data.set_presentation(&presentation);
    DebugDirector director(&tuning, &presentation); data.set_debug(&director); DebugDirectorImpl& impl = *director.impl();
    impl.set_content_directories(source, staged); impl.set_user_directory(root / "user");
    impl.set_script_directories({ root / "scripts" }); director.attach(&app, &data);
    impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output / "idle-drip-tune-commands.log").string()), "Cannot retain idle drip command transcript");
    IdleDripColourProbe probe(app, presentation);
    const std::array<Colour, 2> backgrounds{ Colour(0, 0, 0), Colour(32, 64, 96) };
    size_t commands = 0, rows = 0; bool persisted = false;
    JsonSpanDocument first(real_bytes[1], "private idle drip source");
    const JsonLeaf* keyline = first.find("idle_drip.keyline");
    require(keyline != nullptr, "Idle drip source has no keyline leaf");
    const std::string expected_saved = first.replaced(*keyline, "\"2060C080\"");
    const auto unchanged = [&]
    {
        probe.unchanged();
        require(!impl.can_apply() && impl.request().empty() && !impl.in_match() &&
            tune_colour_bytes(save_path) == saved_preferences && preference_values() == initial_preferences,
            "Idle drip tune changed private preferences, queued work or entered a match");
        for (size_t index = 0; index < files.size(); ++index)
        {
            const std::string& expected = index == 1 && persisted ? expected_saved : real_bytes[index];
            require(tune_colour_bytes(content / files[index]) == real_bytes[index] &&
                tune_colour_bytes(source / files[index]) == expected && tune_colour_bytes(staged / files[index]) == expected,
                "Idle drip tune changed actual content or private bytes beyond its one saved token");
        }
    };
    const auto command = [&](const std::string& line)
    {
        const size_t previous_lines = impl.log().lines().size();
        std::srand(8451); const int expected_random = std::rand(); std::srand(8451);
        const DebugStatus result = impl.execute(line); ++commands;
        require(std::rand() == expected_random, "Idle drip tune command consumed match randomness");
        if (!result.ok) { std::cout << "COMMAND " << line << ": " << result.message << '\n'; }
        require(result.ok, "Validated idle drip tune/reset/save command was rejected");
        if (line.starts_with("tune "))
        {
            // Successful statement replies are written to the console log;
            // the aggregate execute result retains messages only for failures.
            const auto& lines = impl.log().lines();
            require(lines.size() > previous_lines && lines.back().severity == DebugSeverity::info &&
                lines.back().text.starts_with("tune: presentation.idle_drip.") &&
                lines.back().text.ends_with("(live)"), "Idle drip colour command lost live adoption scope");
        }
    };
    const auto set = [&](const char* keyline_word)
    {
        command(std::string("tune presentation.idle_drip.keyline ") + keyline_word);
        const TuneDocument* doc = impl.tune_document("presentation");
        require(doc != nullptr && doc->edits().size() == 1 && doc->edits().contains("idle_drip.keyline"),
            "Idle drip command edited outside its one audited common leaf");
    };
    std::ofstream records(output / "idle-drip-tune-colours.csv");
    records << "state,background,normal_match,captured_paint,all_profiles_public_state_rng,private_bytes,adoption\n";
    const auto check = [&](const char* state, const Colour& expected_keyline)
    {
        unchanged();
        probe.common(expected_keyline);
        for (size_t background = 0; background < backgrounds.size(); ++background)
        {
            const Frame actual = probe.actual(backgrounds[background]);
            require(actual.pixels == probe.reference(backgrounds[background], expected_keyline).pixels,
                "Preconstructed idle drip did not adopt validated live colours with independent RGBA pixels");
            probe.check_paint(actual);
            records << state << ',' << (background == 0 ? "black" : "nonblack") << ",true,true,true,true,live\n"; ++rows;
        }
        unchanged();
    };
    const Colour black(0, 0, 0), rgba_keyline(32, 96, 192, 128);
    check("baseline", black);
    set("RED"); check("named", Colour(255, 0, 0));
    command("tune.reset presentation.idle_drip.keyline");
    require(impl.tune_document("presentation")->edits().empty(), "Idle drip leaf reset retained edits");
    check("reset-original", black);
    set("35BCE1"); check("rgb", Colour(53, 188, 225));
    set("2060C080"); check("rgba128", rgba_keyline);
    command("tune.save presentation"); persisted = true;
    require(impl.tune_document("presentation")->edits().empty() &&
        impl.tune_document("presentation")->baseline().text() == expected_saved,
        "Idle drip save failed to rebase exactly its one private token");
    check("saved-rgba", rgba_keyline);
    set("BLUE"); check("edited-after-save", Colour(0, 0, 255));
    command("tune.reset presentation.idle_drip.keyline"); check("reset-saved-rgba", rgba_keyline);
    set("A532D600"); check("alpha0", Colour(165, 50, 214, 0));
    command("tune.reset presentation"); check("reset-alpha0-to-saved", rgba_keyline);
    require(impl.tune_document("presentation")->edits().empty() && impl.tune_document("tuning")->edits().empty(),
        "Idle drip final reset left pending private edits");
    unchanged();
    require(records.good() && commands == 9 && probe.readbacks() == 60 && probe.preparation_updates() == 15 && rows == 20,
        "Idle drip Debug command evidence exceeded its bounded counts");
    std::ofstream summary(output / "idle-drip-tune-summary.json");
    summary << "{\"scope\":\"debug-validated-commands\",\"state_updates\":0,\"level_updates\":0,\"effect_preparation_updates\":15,"
        "\"profiles\":5,\"consumers\":6,\"commands\":9,\"readbacks\":60,\"csv_rows\":20,\"crops\":0,\"private_sequence_scope\":\"source-constness-only\"}\n";
    require(summary.good(), "Cannot retain idle drip Debug command summary");
    std::cout << "PASS idle drip tune colours: nine validated named/RGB/RGBA/reset/save commands,60 readbacks, "
        "twenty rows; six retained consumers,five stable profiles,one-token private save/rebase,frozen public state/global RNG/preferences. "
        "Fifteen local advance preparations,zero Level/StateContext updates; private stream source constness only.\n";
}
#endif
