// Included inside painter_visual.cpp's private namespace, after the shared
// raster helpers and Debug tune-colour helpers. Player identity ink probe.

    class IdentityInkProbe
    {
    public:
        IdentityInkProbe(Application& app, Level& level, const Presentation& presentation) :
            app_(app), level_(level), look_(presentation.player_identity()), geometry_(this->look_),
            badge_(&this->look_, app.render_resources()),
            hidden_("hidden-identity-ink", { 900, 340, 54, 42 }, 0, &this->look_, app.render_resources(), true),
            state_(observation(level))
        {
            require(this->look_.ink_colour == Colour::black && this->look_.background_colour == Colour::white,
                "Identity ink fixture no longer matches its reviewed opaque defaults");
            require(level.player_count() == 4, "Identity ink fixture needs four preconstructed player views");
#if COLOURWARS_DEV_TOOLS
            this->fingerprint_ = MatchFingerprint::of(level, 0);
#endif
            const HudPresentation& hud = presentation.hud();
            const FontHandle font = app.render_resources()->resolve_sprite_font(hud.player_identity_font);
            for (size_t roster = 0; roster < this->ui_.size(); ++roster)
            {
                const float x = 240.0f + 140.0f * static_cast<float>(roster);
                this->boxes_[roster] = RectangleF({ x, 240 }, this->look_.ui_size);
                this->ui_[roster] = std::make_unique<UiPlayerBadge>("identity-ink-" + std::to_string(roster),
                    RectangleF({ x, 340 }, this->look_.ui_size), static_cast<int>(roster),
                    &this->look_, app.render_resources());
                const int view = static_cast<int>(roster);
                const Viewport& viewport = app.viewport_manager()->player_viewport(view);
                const Vector2F origin(static_cast<float>(viewport.pixel_rect().x),
                    static_cast<float>(viewport.pixel_rect().y));
                this->world_[roster] = level.view_camera(view).calculate_view_rectangle(level.player_identity_bounds(view));
                this->world_[roster].x += origin.x; this->world_[roster].y += origin.y;
                const Vector2F resolution = viewport.size();
                const float scale = std::min(resolution.x / hud.player_identity_reference_width,
                    resolution.y / hud.reference_height);
                const std::wstring label = PlayerBadge::label(view) +
                    (level.player(view).team() == PlayerTeam::a ? L"  /  TEAM A\n" : L"  /  TEAM B\n") +
                    input_slots::device_name(level.player(view).player_num());
                const Vector2F text_size = app.render_resources()->measure_text(font, label) * scale;
                const Vector2F badge_size = this->look_.ui_size * scale;
                this->hud_[roster] = RectangleF(origin + Vector2F(resolution.x * hud.player_identity_offset.x,
                    resolution.y * hud.player_identity_offset.y + (text_size.y - badge_size.y) / 2.0f), badge_size);
            }
            this->baseline_badges_ = this->badges(); this->baseline_match_ = this->match();
            require(this->baseline_badges_.pixels == this->badges(false).pixels,
                "Hidden preconstructed UiPlayerBadge produced pixels");
            for (auto& ui : this->ui_) { ui->set_colour(Colour::magenta); }
            require(this->baseline_badges_.pixels == this->badges().pixels,
                "Focus tint recoloured a preconstructed UiPlayerBadge");
            this->unchanged();
        }

        void unchanged() const
        {
            const auto& a = this->look_; const auto& b = this->geometry_;
            require(a.enabled == b.enabled && a.world_size == b.world_size && a.world_offset == b.world_offset &&
                a.emphasis_duration == b.emphasis_duration && a.emphasis_scale == b.emphasis_scale &&
                a.ui_size == b.ui_size && a.ui_gap == b.ui_gap && a.background_colour == b.background_colour &&
                a.border == b.border && this->hidden_.hidden() && this->state_ == observation(this->level_),
                "Identity ink edit or drawing changed geometry, visibility, simulation or cosmetic timers");
            for (size_t roster = 0; roster < this->ui_.size(); ++roster)
            {
                require(!this->ui_[roster]->hidden() && this->ui_[roster]->bounds() ==
                    RectangleF(this->boxes_[roster].position() + Vector2F(0, 100), b.ui_size),
                    "Identity ink edit changed a captured UI badge rectangle or visibility");
            }
#if COLOURWARS_DEV_TOOLS
            require(MatchFingerprint::difference(this->fingerprint_, MatchFingerprint::of(this->level_, 0)).empty(),
                "Identity ink edit or drawing changed the frozen match fingerprint");
#endif
        }

        void check(const char* state, const Colour& expected, const std::filesystem::path& output,
            std::ofstream& records, bool retain_crop = false)
        {
            this->unchanged();
            require(this->look_.ink_colour == expected && expected.a == 1.0f,
                "Identity ink consumer received different opaque channels than its literal oracle");
            const auto channel = [](float value) { return static_cast<unsigned char>(std::lround(value * 255.0f)); };
            const std::uint32_t ink = (static_cast<std::uint32_t>(channel(expected.r)) << 16) |
                (static_cast<std::uint32_t>(channel(expected.g)) << 8) | channel(expected.b);
            const Frame badges = this->badges(); Frame reference = this->baseline_badges_;
            size_t badge_pixels = 0;
            for (size_t offset = 0; offset < reference.pixels.size(); offset += 4)
            {
                if (reference.pixels[offset] != 0 || reference.pixels[offset + 1] != 0 ||
                    reference.pixels[offset + 2] != 0) { continue; }
                reference.pixels[offset] = channel(expected.r); reference.pixels[offset + 1] = channel(expected.g);
                reference.pixels[offset + 2] = channel(expected.b); ++badge_pixels;
            }
            require(badge_pixels > 0 && badges.pixels == reference.pixels,
                "Preconstructed PlayerBadge or UiPlayerBadge did not recolour exactly its original opaque ink pixels");
            const Frame match = this->match(); std::array<size_t, 4> world{}, hud{};
            const auto inside = [](const RectangleF& box, int x, int y)
            { return static_cast<float>(x) >= box.x - 1 && static_cast<float>(x) <= box.right() + 1 &&
                static_cast<float>(y) >= box.y - 1 && static_cast<float>(y) <= box.bottom() + 1; };
            for (int y = 0; y < match.height; ++y)
            {
                for (int x = 0; x < match.width; ++x)
                {
                    const std::uint32_t old = monitor_rgb(this->baseline_match_, x, y), now = monitor_rgb(match, x, y);
                    if (old == now) { continue; }
                    require(old == 0 && now == ink, "Identity ink edit changed a non-ink frozen match pixel");
                    bool contained = false;
                    for (size_t roster = 0; roster < world.size(); ++roster)
                    {
                        if (inside(this->world_[roster], x, y)) { ++world[roster]; contained = true; }
                        if (inside(this->hud_[roster], x, y)) { ++hud[roster]; contained = true; }
                    }
                    require(contained, "Identity ink raster delta escaped every preconstructed world and HUD badge");
                }
            }
            size_t world_pixels = 0, hud_pixels = 0;
            for (size_t roster = 0; roster < world.size(); ++roster)
            {
                require(expected == Colour::black ? world[roster] == 0 && hud[roster] == 0 :
                    world[roster] > 0 && hud[roster] > 0,
                    "An existing player view did not recolour both its world and HUD identity without an update");
                world_pixels += world[roster]; hud_pixels += hud[roster];
            }
            records << state << ',' << badge_pixels << ',' << world_pixels << ',' << hud_pixels << ",true,true,live\n";
            if (retain_crop) { save(foreground_crop(badges, { 220, 220, 520, 190 }, 0), output /
                (std::string("identity-ink-") + state + "-badges.ppm")); }
            this->unchanged();
        }

        size_t readbacks() const { return this->readbacks_; }

    private:
        Frame frozen(const std::function<Frame()>& draw)
        {
            std::srand(8451); const int expected = std::rand(); std::srand(8451);
            const Frame first = draw();
            require(first.pixels == draw().pixels, "Frozen identity ink consumer changed pixels between redraws");
            require(std::rand() == expected, "Identity ink drawing consumed match randomness");
            return first;
        }

        Frame badges(bool draw_hidden = true)
        {
            return this->frozen([&]
            {
                Renderer& renderer = *this->app_.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
                DrawList list = renderer.view(0); list.set_viewport(this->app_.viewport_manager()->fullscreen_viewport());
                list.set_camera(Camera::DEFAULT_CAMERA);
                const Canvas background(this->app_.render_resources(), Colour(32, 64, 96));
                background.draw(list, { 0, 0, 1280, 720 });
                for (size_t roster = 0; roster < this->ui_.size(); ++roster)
                { this->badge_.draw(list, this->boxes_[roster], static_cast<int>(roster)); this->ui_[roster]->draw(list); }
                if (draw_hidden) { this->hidden_.draw(list); }
                ++this->readbacks_; return read(renderer);
            });
        }

        Frame match()
        {
            return this->frozen([&] { ++this->readbacks_; return render(this->level_, *this->app_.renderer()); });
        }

        Application& app_;
        Level& level_;
        const PlayerIdentityPresentation& look_;
        PlayerIdentityPresentation geometry_;
        PlayerBadge badge_;
        UiPlayerBadge hidden_;
        std::array<std::unique_ptr<UiPlayerBadge>, 4> ui_;
        std::array<RectangleF, 4> boxes_, world_, hud_;
        std::vector<float> state_;
        Frame baseline_badges_, baseline_match_;
        size_t readbacks_ = 0;
#if COLOURWARS_DEV_TOOLS
        MatchFingerprint fingerprint_;
#endif
    };

    void identity_ink_consumer_checks(Application& app, Level& level, Presentation& presentation,
        const std::filesystem::path& output)
    {
        auto& ink = const_cast<PlayerIdentityPresentation&>(presentation.player_identity()).ink_colour;
        struct RestoreIdentityInk { Colour& ink; Colour previous; ~RestoreIdentityInk() { this->ink = this->previous; } } restore{ ink, ink };
        IdentityInkProbe probe(app, level, presentation);
        std::ofstream records(output / "identity-ink-consumers.csv");
        records << "state,badge_ink_pixels,world_changed_pixels,hud_changed_pixels,frozen_geometry_state,hidden_focus_policy,adoption\n";
        probe.check("default", Colour::black, output, records, true);
        ink = Colour(255, 0, 0); probe.check("named-literal", ink, output, records, true);
        ink = Colour(53, 188, 225); probe.check("rgb-literal", ink, output, records, true);
        ink = Colour(32, 96, 192, 255); probe.check("rgba255-literal", ink, output, records, true);
        ink = restore.previous; probe.check("restored", ink, output, records);
        require(records.good(), "Cannot retain identity ink consumer records");
        std::ofstream summary(output / "identity-ink-consumer-summary.json");
        summary << "{\"scope\":\"both-preset-live-consumers\",\"state_updates\":0,\"readbacks\":" << probe.readbacks()
            << ",\"csv_rows\":5,\"crops\":4,\"world_views\":4,\"hud_views\":4,\"ui_rosters\":4}\n";
        require(summary.good(), "Cannot retain identity ink consumer summary");
        std::cout << "PASS identity ink consumers: four preconstructed PlayerBadge/UiPlayerBadge roster cells and frozen 4P world/HUD badges, "
            "opaque literal pixel-substitution oracle, restored raster, captured bounds, hidden/focus policy and draw RNG/fingerprint; "
            "both presets, no page reopening or state updates\n";
    }

#if COLOURWARS_DEV_TOOLS
    void identity_ink_tune_checks(Application& app, Level& level, Registry<LevelLoadedInfo>& levels,
        GameTuning& tuning, Presentation& presentation, const std::filesystem::path& content,
        const std::filesystem::path& output)
    {
        const auto root = output / "identity-ink-tune-scratch", source = root / "source", staged = root / "staged";
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
        struct RestoreIdentityTuneContent
        {
            GameTuning& tuning; Presentation& presentation; GameTuning original_tuning; Presentation original;
            ~RestoreIdentityTuneContent() { this->tuning = this->original_tuning; this->presentation = this->original; }
        } restore{ tuning, presentation, original_tuning, original };
        Save preferences; PlaySettings play; play.music_volume = 17; play.sfx_volume = 83;
        play.aim_marker = true; play.reduce_effects = true;
        require(preferences.set_play_settings_and_save(play), "Cannot seed private identity ink preferences");
        const auto save_path = root / "profile/ColourWars/save_data.json";
        const std::string saved_preferences = tune_colour_bytes(save_path);
        StageList stages;
        for (const char* name : { "king_of_the_hill", "turbulence", "close_quarters", "movement_course", "crossfeed" })
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
        require(impl.log().set_transcript((output / "identity-ink-tune-commands.log").string()),
            "Cannot retain identity ink command transcript");
        IdentityInkProbe probe(app, level, presentation);
        JsonSpanDocument baseline(real_bytes[1], "private identity ink source");
        const JsonLeaf* leaf = baseline.find("player_identity.ink_colour");
        require(leaf != nullptr, "Identity ink source has no audited colour leaf");
        const std::string expected_saved = baseline.replaced(*leaf, "\"2060C0FF\"");
        bool persisted = false; size_t commands = 0;
        const auto unchanged = [&]
        {
            probe.unchanged();
            require(!impl.can_apply() && impl.request().empty() && !impl.in_match() &&
                tune_colour_bytes(save_path) == saved_preferences && preferences.play_settings().music_volume == 17 &&
                preferences.play_settings().sfx_volume == 83 && preferences.play_settings().aim_marker &&
                preferences.play_settings().reduce_effects, "Identity ink commands changed preferences or queued gameplay work");
            for (size_t index = 0; index < files.size(); ++index)
            {
                const std::string& expected = index == 1 && persisted ? expected_saved : real_bytes[index];
                require(tune_colour_bytes(content / files[index]) == real_bytes[index] &&
                    tune_colour_bytes(source / files[index]) == expected && tune_colour_bytes(staged / files[index]) == expected,
                    "Identity ink commands wrote actual content or private bytes beyond their one saved token");
            }
        };
        const auto command = [&](const std::string& line)
        {
            const size_t previous_lines = impl.log().lines().size();
            const DebugStatus result = impl.execute(line); ++commands;
            if (!result.ok) { std::cout << "COMMAND " << line << ": " << result.message << '\n'; }
            require(result.ok, "Validated identity ink tune/reset/save command was rejected");
            if (line.starts_with("tune "))
            {
                const auto& lines = impl.log().lines();
                require(lines.size() > previous_lines && lines.back().severity == DebugSeverity::info &&
                    lines.back().text.starts_with("tune: presentation.player_identity.ink_colour") &&
                    lines.back().text.ends_with("(live)"), "Identity ink command lost its exact live adoption scope");
                const auto* document = impl.tune_document("presentation");
                require(document->edits().size() == 1 && document->edits().contains("player_identity.ink_colour"),
                    "Identity ink command changed more than its one audited leaf");
            }
        };
        std::ofstream records(output / "identity-ink-tune-colours.csv");
        records << "state,badge_ink_pixels,world_changed_pixels,hud_changed_pixels,frozen_geometry_state,hidden_focus_policy,adoption\n";
        const auto check = [&](const char* state, const Colour& expected)
        { unchanged(); probe.check(state, expected, output, records); unchanged(); };
        check("baseline", Colour::black);
        command("tune presentation.player_identity.ink_colour RED"); check("named", Colour(255, 0, 0));
        for (const char* word : { "11223380", "11223300", "WHITE" })
        {
            const auto* document = impl.tune_document("presentation");
            const std::string text = document->current().text();
            const TuneEdit edit = document->edits().at("player_identity.ink_colour");
            const DebugStatus rejected = impl.execute(std::string("tune presentation.player_identity.ink_colour ") + word); ++commands;
            require(!rejected.ok && rejected.message.find("distinct and opaque") != std::string::npos &&
                document->current().text() == text && document->edits().size() == 1 &&
                document->edits().contains("player_identity.ink_colour") &&
                document->edits().at("player_identity.ink_colour").baseline_token == edit.baseline_token &&
                document->edits().at("player_identity.ink_colour").current_token == edit.current_token &&
                presentation.player_identity().ink_colour == Colour(255, 0, 0),
                "Identity ink semantic refusal changed live colour or its pending text/ledger");
            unchanged();
        }
        command("tune.reset presentation.player_identity.ink_colour"); check("reset-original", Colour::black);
        require(impl.tune_document("presentation")->edits().empty(), "Identity ink leaf reset retained a pending edit");
        command("tune presentation.player_identity.ink_colour 35BCE1"); check("rgb", Colour(53, 188, 225));
        command("tune presentation.player_identity.ink_colour 2060C0FF"); check("rgba255", Colour(32, 96, 192, 255));
        command("tune.save presentation"); persisted = true;
        require(impl.tune_document("presentation")->edits().empty() &&
            impl.tune_document("presentation")->baseline().text() == expected_saved,
            "Identity ink save did not persist and rebase exactly its one private token");
        check("saved-rgba255", Colour(32, 96, 192, 255));
        command("tune presentation.player_identity.ink_colour RED"); check("edited-after-save", Colour(255, 0, 0));
        command("tune.reset presentation"); check("reset-saved-rgba255", Colour(32, 96, 192, 255));
        require(records.good() && commands == 10 && impl.tune_document("presentation")->edits().empty() &&
            impl.tune_document("tuning")->edits().empty(), "Identity ink command evidence left pending edits or exceeded its bounded commands");
        std::ofstream summary(output / "identity-ink-tune-summary.json");
        summary << "{\"scope\":\"debug-validated-private-commands\",\"state_updates\":0,\"commands\":10,\"semantic_refusals\":3,\"readbacks\":"
            << probe.readbacks() << ",\"csv_rows\":8,\"crops\":0}\n";
        require(summary.good(), "Cannot retain identity ink command summary");
        std::cout << "PASS identity ink tuning: ten real named/RGB/opaque-RGBA/reset/private-save commands including three atomic "
            "opaque/distinct refusals, one-token persistence/rebase and saved-baseline reset on preconstructed 4P world/HUD/UI badges; "
            "frozen geometry/state, private preferences and actual content retained, no StateContext updates\n";
    }
#endif
