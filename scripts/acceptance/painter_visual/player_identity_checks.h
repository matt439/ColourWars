// Included inside painter_visual.cpp's private namespace, after the shared
// raster helpers and Debug tune-colour helpers. Player identity matrix.

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

// Included inside painter_visual.cpp's private namespace, after the shared
// raster helpers and Debug tune-colour helpers. Player identity matrix.

    class IdentityBackgroundProbe
    {
    public:
        IdentityBackgroundProbe(Application& app, Level& level, const Presentation& presentation) :
            app_(app), level_(level), look_(presentation.player_identity()), geometry_(this->look_),
            badge_(&this->look_, app.render_resources()),
            hidden_("hidden-identity-background", { 900, 340, 54, 42 }, 0, &this->look_, app.render_resources(), true),
            state_(observation(level))
        {
            require(this->look_.ink_colour == Colour::black && this->look_.background_colour == Colour::white,
                "Identity background fixture no longer matches its reviewed opaque defaults");
            require(level.player_count() == 4, "Identity background fixture needs four preconstructed player views");
#if COLOURWARS_DEV_TOOLS
            this->fingerprint_ = MatchFingerprint::of(level, 0);
#endif
            const HudPresentation& hud = presentation.hud();
            const FontHandle font = app.render_resources()->resolve_sprite_font(hud.player_identity_font);
            for (size_t roster = 0; roster < this->ui_.size(); ++roster)
            {
                const float x = 240.0f + 140.0f * static_cast<float>(roster);
                this->boxes_[roster] = RectangleF({ x, 240 }, this->look_.ui_size);
                this->ui_[roster] = std::make_unique<UiPlayerBadge>("identity-background-" + std::to_string(roster),
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
                a.ui_size == b.ui_size && a.ui_gap == b.ui_gap && a.ink_colour == b.ink_colour &&
                a.border == b.border && this->hidden_.hidden() && this->state_ == observation(this->level_),
                "Identity background edit or drawing changed geometry, visibility, simulation or cosmetic timers");
            for (size_t roster = 0; roster < this->ui_.size(); ++roster)
            {
                require(!this->ui_[roster]->hidden() && this->ui_[roster]->bounds() ==
                    RectangleF(this->boxes_[roster].position() + Vector2F(0, 100), b.ui_size),
                    "Identity background edit changed a captured UI badge rectangle or visibility");
            }
#if COLOURWARS_DEV_TOOLS
            require(MatchFingerprint::difference(this->fingerprint_, MatchFingerprint::of(this->level_, 0)).empty(),
                "Identity background edit or drawing changed the frozen match fingerprint");
#endif
        }

        void check(const char* state, const Colour& expected, const std::filesystem::path& output,
            std::ofstream& records, bool retain_crop = false)
        {
            this->unchanged();
            require(this->look_.background_colour == expected && expected.a == 1.0f,
                "Identity background consumer received different opaque paper channels than its literal oracle");
            const auto channel = [](float value) { return static_cast<unsigned char>(std::lround(value * 255.0f)); };
            const auto inside = [](const RectangleF& box, int x, int y)
            {
                // The independently computed rectangle covers pixel centres;
                // no expanded margin may admit neighbouring white scene art.
                const float centre_x = static_cast<float>(x) + 0.5f;
                const float centre_y = static_cast<float>(y) + 0.5f;
                return centre_x >= box.x && centre_x < box.right() &&
                    centre_y >= box.y && centre_y < box.bottom();
            };
            const auto substitute = [&](Frame& frame, int x, int y)
            {
                const size_t offset = (static_cast<size_t>(y) * static_cast<size_t>(frame.width) +
                    static_cast<size_t>(x)) * 4;
                frame.pixels[offset] = channel(expected.r); frame.pixels[offset + 1] = channel(expected.g);
                frame.pixels[offset + 2] = channel(expected.b);
            };
            const Frame badges = this->badges(); Frame reference = this->baseline_badges_;
            std::array<size_t, 4> direct_paper{}, ui_paper{}, direct_ink{}, ui_ink{};
            size_t badge_pixels = 0, ink_pixels = 0;
            for (int y = 0; y < badges.height; ++y)
            {
                for (int x = 0; x < badges.width; ++x)
                {
                    const std::uint32_t old = monitor_rgb(this->baseline_badges_, x, y);
                    bool paper = false, ink = false;
                    for (size_t roster = 0; roster < this->boxes_.size(); ++roster)
                    {
                        if (inside(this->boxes_[roster], x, y))
                        {
                            if (old == 0xFFFFFFu) { ++direct_paper[roster]; paper = true; }
                            if (old == 0u) { ++direct_ink[roster]; ink = true; }
                        }
                        if (inside(this->ui_[roster]->bounds(), x, y))
                        {
                            if (old == 0xFFFFFFu) { ++ui_paper[roster]; paper = true; }
                            if (old == 0u) { ++ui_ink[roster]; ink = true; }
                        }
                    }
                    if (paper) { substitute(reference, x, y); ++badge_pixels; }
                    if (ink) { require(monitor_rgb(badges, x, y) == 0u,
                        "Background edit recoloured an isolated black glyph or border"); ++ink_pixels; }
                }
            }
            require(badge_pixels > 0 && badges.pixels == reference.pixels,
                "Preconstructed PlayerBadge or UiPlayerBadge did not recolour exactly its original opaque paper pixels");
            const Frame match = this->match(); Frame match_reference = this->baseline_match_;
            std::array<size_t, 4> world{}, hud{}, world_paper{}, hud_paper{}, world_ink{}, hud_ink{};
            for (int y = 0; y < match.height; ++y)
            {
                for (int x = 0; x < match.width; ++x)
                {
                    const std::uint32_t old = monitor_rgb(this->baseline_match_, x, y), now = monitor_rgb(match, x, y);
                    bool paper = false, ink = false;
                    for (size_t roster = 0; roster < world.size(); ++roster)
                    {
                        if (inside(this->world_[roster], x, y))
                        {
                            if (old == 0xFFFFFFu) { ++world_paper[roster]; paper = true; }
                            if (old == 0u) { ++world_ink[roster]; ink = true; }
                            if (old != now) { ++world[roster]; }
                        }
                        if (inside(this->hud_[roster], x, y))
                        {
                            if (old == 0xFFFFFFu) { ++hud_paper[roster]; paper = true; }
                            if (old == 0u) { ++hud_ink[roster]; ink = true; }
                            if (old != now) { ++hud[roster]; }
                        }
                    }
                    if (paper) { substitute(match_reference, x, y); }
                    if (ink) { require(now == 0u, "Background edit recoloured a world/HUD black glyph or border"); ++ink_pixels; }
                }
            }
            require(match.pixels == match_reference.pixels,
                "Identity background edit changed non-paper ink, unrelated scene white or pixels beyond known badge bounds");
            size_t world_pixels = 0, hud_pixels = 0;
            for (size_t roster = 0; roster < world.size(); ++roster)
            {
                require(direct_paper[roster] > 0 && ui_paper[roster] > 0 && world_paper[roster] > 0 &&
                    hud_paper[roster] > 0 && direct_ink[roster] > 0 && ui_ink[roster] > 0 &&
                    world_ink[roster] > 0 && hud_ink[roster] > 0,
                    "A preconstructed direct/UI/world/HUD badge did not exercise independent paper and retained ink");
                require(expected == Colour::white ? world[roster] == 0 && hud[roster] == 0 :
                    world[roster] == world_paper[roster] && hud[roster] == hud_paper[roster],
                    "An existing player view did not recolour both its world and HUD identity without an update");
                world_pixels += world[roster]; hud_pixels += hud[roster];
            }
            records << state << ',' << badge_pixels << ',' << world_pixels << ',' << hud_pixels << ',' <<
                ink_pixels << ",true,true,live\n";
            if (retain_crop) { save(foreground_crop(badges, { 220, 220, 520, 190 }, 0), output /
                (std::string("identity-background-") + state + "-badges.ppm")); }
            this->unchanged();
        }

        size_t readbacks() const { return this->readbacks_; }

    private:
        Frame frozen(const std::function<Frame()>& draw)
        {
            std::srand(8451); const int expected = std::rand(); std::srand(8451);
            const Frame first = draw();
            require(first.pixels == draw().pixels, "Frozen identity background consumer changed pixels between redraws");
            require(std::rand() == expected, "Identity background drawing consumed match randomness");
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

    void identity_background_consumer_checks(Application& app, Level& level, Presentation& presentation,
        const std::filesystem::path& output)
    {
        auto& paper = const_cast<PlayerIdentityPresentation&>(presentation.player_identity()).background_colour;
        struct RestoreIdentityBackground { Colour& paper; Colour previous; ~RestoreIdentityBackground() { this->paper = this->previous; } } restore{ paper, paper };
        IdentityBackgroundProbe probe(app, level, presentation);
        std::ofstream records(output / "identity-background-consumers.csv");
        records << "state,badge_paper_pixels,world_changed_pixels,hud_changed_pixels,black_ink_unchanged_pixels,frozen_geometry_state,hidden_focus_policy,adoption\n";
        probe.check("default", Colour::white, output, records, true);
        paper = Colour(255, 0, 0); probe.check("named-literal", paper, output, records, true);
        paper = Colour(53, 188, 225); probe.check("rgb-literal", paper, output, records, true);
        paper = Colour(32, 96, 192, 255); probe.check("rgba255-literal", paper, output, records, true);
        paper = restore.previous; probe.check("restored", paper, output, records);
        require(records.good() && probe.readbacks() == 28, "Cannot retain bounded identity background consumer records");
        std::ofstream summary(output / "identity-background-consumer-summary.json");
        summary << "{\"scope\":\"both-preset-live-consumers\",\"state_updates\":0,\"readbacks\":" << probe.readbacks()
            << ",\"csv_rows\":5,\"crops\":4,\"world_views\":4,\"hud_views\":4,\"ui_rosters\":4}\n";
        require(summary.good(), "Cannot retain identity background consumer summary");
        std::cout << "PASS identity background consumers: four preconstructed PlayerBadge/UiPlayerBadge roster cells and frozen 4P world/HUD badges, "
            "opaque literal pixel-substitution oracle, restored raster, captured bounds, hidden/focus policy and draw RNG/fingerprint; "
            "both presets, no page reopening or state updates\n";
    }

#if COLOURWARS_DEV_TOOLS
    void identity_background_tune_checks(Application& app, Level& level, Registry<LevelLoadedInfo>& levels,
        GameTuning& tuning, Presentation& presentation, const std::filesystem::path& content,
        const std::filesystem::path& output)
    {
        const auto root = output / "identity-background-tune-scratch", source = root / "source", staged = root / "staged";
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
        require(preferences.set_play_settings_and_save(play), "Cannot seed private identity background preferences");
        require(preferences.set_input_bindings_and_save(InputBindings{ Key::j }),
            "Cannot seed the unrelated private identity background Jump preference");
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
        require(impl.log().set_transcript((output / "identity-background-tune-commands.log").string()),
            "Cannot retain identity background command transcript");
        IdentityBackgroundProbe probe(app, level, presentation);
        JsonSpanDocument baseline(real_bytes[1], "private identity background source");
        const JsonLeaf* leaf = baseline.find("player_identity.background_colour");
        require(leaf != nullptr, "Identity background source has no audited colour leaf");
        const std::string expected_saved = baseline.replaced(*leaf, "\"2060C0FF\"");
        bool persisted = false; size_t commands = 0;
        const auto unchanged = [&]
        {
            probe.unchanged();
            require(!impl.can_apply() && impl.request().empty() && !impl.in_match() &&
                tune_colour_bytes(save_path) == saved_preferences && preferences.play_settings().music_volume == 17 &&
                preferences.play_settings().sfx_volume == 83 && preferences.play_settings().aim_marker &&
                preferences.play_settings().reduce_effects && preferences.input_bindings().keyboard_jump == Key::j,
                "Identity background commands changed play/input preferences or queued gameplay work");
            for (size_t index = 0; index < files.size(); ++index)
            {
                const std::string& expected = index == 1 && persisted ? expected_saved : real_bytes[index];
                require(tune_colour_bytes(content / files[index]) == real_bytes[index] &&
                    tune_colour_bytes(source / files[index]) == expected && tune_colour_bytes(staged / files[index]) == expected,
                    "Identity background commands wrote actual content or private bytes beyond their one saved token");
            }
        };
        const auto command = [&](const std::string& line)
        {
            const size_t previous_lines = impl.log().lines().size();
            const DebugStatus result = impl.execute(line); ++commands;
            if (!result.ok) { std::cout << "COMMAND " << line << ": " << result.message << '\n'; }
            require(result.ok, "Validated identity background tune/reset/save command was rejected");
            if (line.starts_with("tune "))
            {
                const auto& lines = impl.log().lines();
                require(lines.size() > previous_lines && lines.back().severity == DebugSeverity::info &&
                    lines.back().text.starts_with("tune: presentation.player_identity.background_colour") &&
                    lines.back().text.ends_with("(live)"), "Identity background command lost its exact live adoption scope");
                const auto* document = impl.tune_document("presentation");
                require(document->edits().size() == 1 && document->edits().contains("player_identity.background_colour"),
                    "Identity background command changed more than its one audited leaf");
            }
        };
        std::ofstream records(output / "identity-background-tune-colours.csv");
        records << "state,badge_paper_pixels,world_changed_pixels,hud_changed_pixels,black_ink_unchanged_pixels,frozen_geometry_state,hidden_focus_policy,adoption\n";
        const auto check = [&](const char* state, const Colour& expected)
        { unchanged(); probe.check(state, expected, output, records); unchanged(); };
        check("baseline", Colour::white);
        command("tune presentation.player_identity.background_colour RED"); check("named", Colour(255, 0, 0));
        for (const char* word : { "11223380", "11223300", "BLACK" })
        {
            const auto* document = impl.tune_document("presentation");
            const std::string text = document->current().text();
            const TuneEdit edit = document->edits().at("player_identity.background_colour");
            const DebugStatus rejected = impl.execute(std::string("tune presentation.player_identity.background_colour ") + word); ++commands;
            require(!rejected.ok && rejected.message.find("distinct and opaque") != std::string::npos &&
                document->current().text() == text && document->edits().size() == 1 &&
                document->edits().contains("player_identity.background_colour") &&
                document->edits().at("player_identity.background_colour").baseline_token == edit.baseline_token &&
                document->edits().at("player_identity.background_colour").current_token == edit.current_token &&
                presentation.player_identity().background_colour == Colour(255, 0, 0),
                "Identity background semantic refusal changed live colour or its pending text/ledger");
            unchanged();
        }
        command("tune.reset presentation.player_identity.background_colour"); check("reset-original", Colour::white);
        require(impl.tune_document("presentation")->edits().empty(), "Identity background leaf reset retained a pending edit");
        command("tune presentation.player_identity.background_colour 35BCE1"); check("rgb", Colour(53, 188, 225));
        command("tune presentation.player_identity.background_colour 2060C0FF"); check("rgba255", Colour(32, 96, 192, 255));
        command("tune.save presentation"); persisted = true;
        require(impl.tune_document("presentation")->edits().empty() &&
            impl.tune_document("presentation")->baseline().text() == expected_saved,
            "Identity background save did not persist and rebase exactly its one private token");
        check("saved-rgba255", Colour(32, 96, 192, 255));
        command("tune presentation.player_identity.background_colour RED"); check("edited-after-save", Colour(255, 0, 0));
        command("tune.reset presentation"); check("reset-saved-rgba255", Colour(32, 96, 192, 255));
        require(records.good() && commands == 10 && probe.readbacks() == 40 && impl.tune_document("presentation")->edits().empty() &&
            impl.tune_document("tuning")->edits().empty(), "Identity background command evidence left pending edits or exceeded its bounded commands");
        std::ofstream summary(output / "identity-background-tune-summary.json");
        summary << "{\"scope\":\"debug-validated-private-commands\",\"state_updates\":0,\"commands\":10,\"semantic_refusals\":3,\"readbacks\":"
            << probe.readbacks() << ",\"csv_rows\":8,\"crops\":0,\"private_seed_save_calls\":2}\n";
        require(summary.good(), "Cannot retain identity background command summary");
        std::cout << "PASS identity background tuning: ten real named/RGB/opaque-RGBA/reset/private-save commands including three atomic "
            "opaque/distinct refusals, one-token persistence/rebase and saved-baseline reset on preconstructed 4P world/HUD/UI badges; "
            "frozen geometry/state, private preferences and actual content retained, no StateContext updates\n";
    }
#endif

    Frame identity_context_crop(const Frame& frame, const Level& level, int view,
        const Viewport& viewport)
    {
        const RectangleF body = level.player(view).rectangle();
        const RectangleF badge = level.player_identity_bounds(view);
        const float left = std::min(body.left(), badge.left());
        const float top = std::min(body.top(), badge.top());
        const float right = std::max(body.right(), badge.right());
        const float bottom = std::max(body.bottom(), badge.bottom());
        const RectangleF projected = level.view_camera(view).calculate_view_rectangle(
            RectangleF(left, top, right - left, bottom - top));
        const RectangleI pane = viewport.pixel_rect();
        const int x0 = std::max(pane.x, pane.x + static_cast<int>(std::floor(projected.x)) - 12);
        const int y0 = std::max(pane.y, pane.y + static_cast<int>(std::floor(projected.y)) - 12);
        const int x1 = std::min(pane.right(), pane.x + static_cast<int>(std::ceil(projected.right())) + 12);
        const int y1 = std::min(pane.bottom(), pane.y + static_cast<int>(std::ceil(projected.bottom())) + 12);
        require(x1 > x0 && y1 > y0, "Identity context did not intersect its own pane");
        Frame crop{ x1 - x0, y1 - y0, {} };
        for (int y = y0; y < y1; ++y)
        {
            const size_t start = (static_cast<size_t>(y) * static_cast<size_t>(frame.width) +
                static_cast<size_t>(x0)) * 4;
            crop.pixels.insert(crop.pixels.end(), frame.pixels.begin() + start,
                frame.pixels.begin() + start + static_cast<size_t>(crop.width) * 4);
        }
        return crop;
    }

    void identity_capture(Application& app, Level& level, Presentation& presentation,
        const std::filesystem::path& output, const std::string& stem, std::ofstream& records)
    {
        const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
        const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
        const Frame enabled = render(level, *app.renderer());
        require(enabled.pixels == render(level, *app.renderer()).pixels,
            "Frozen identity Level redraw changed pixels");
        auto& look = const_cast<PlayerIdentityPresentation&>(presentation.player_identity());
        // Hide only the world badge at the identical state. The same shared
        // renderer still draws the pane's UI badge, so every delta below must
        // belong to the owner above the figure, not its HUD or another player.
        const Vector2F world_size = look.world_size;
        look.world_size = Vector2F::ZERO;
        const Frame disabled = render(level, *app.renderer());
        look.world_size = world_size;
        require(enabled.pixels == render(level, *app.renderer()).pixels,
            "Restoring world identity size did not restore its raster");
        save(enabled, output / (stem + ".ppm"));
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            const RectangleF bounds = level.player_identity_bounds(view);
            Frame crop{};
            const size_t changed = check_projected_bounds(enabled, disabled,
                level.view_camera(view).calculate_view_rectangle(bounds), viewport, crop);
            require(level.player_identity_visible(view) ? changed > 0 : changed == 0,
                "World identity visibility or own-pane raster bounds did not match lifecycle");
            if (changed > 0)
            {
                save(identity_context_crop(enabled, level, view, viewport), output /
                    (stem + "-context-player" + std::to_string(view + 1) + ".ppm"));
                save(crop, output / (stem + "-badge-player" + std::to_string(view + 1) + ".ppm"));
            }
            records << stem << ',' << view + 1 << ',' << level.player(view).player_num() << ','
                << level.player_identity_visible(view) << ',' << level.player_identity_emphasis(view) << ','
                << changed << ',' << bounds.x << ',' << bounds.y << ',' << bounds.width << ',' << bounds.height << '\n';
        }
        require(before == observation(level), "Identity draws changed simulation or cosmetic timers");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
            "Identity draws changed replay fingerprint");
#endif
    }

    void identity_matrix(Application& app, LevelBuilder& builder, Registry<LevelLoadedInfo>& levels,
        GameTuning& tuning, Presentation& presentation, PlaySettings& preferences,
        const std::filesystem::path& content, const std::filesystem::path& output)
    {
        const RestoreFlag restore_reduced{ preferences.reduce_effects, preferences.reduce_effects };
        preferences.reduce_effects = false;
        std::ofstream records(output / "player-identity.csv");
        records << "capture,view,input_slot,visible,emphasis,changed_pixels,bounds_x,bounds_y,bounds_width,bounds_height\n";
        const auto inputs = neutral_inputs();
        for (const TeamColourPair palette : { TeamColourPair::blue_orange,
            TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            auto level = build(app, builder, 4, palette, false, true, WeaponType::sprayer);
            settle(*level, inputs);
            resize(app, *level, { 1280, 720 });
            // All four identical same-team tools overlap. Only roster identity
            // distinguishes the figures; teammates share the team colour.
            for (int view = 0; view < 4; ++view)
            {
                require(level->place_player(view, { 1400.0f + 12.0f * static_cast<float>(view), 1275.0f }),
                    "Could not overlap same-team identity fixtures");
            }
            for (int tick = 0; tick < 600 && level->player_identity_emphasis(0) > 0.0f; ++tick)
            {
                level->update(inputs, dt);
            }
            require(level->player_identity_emphasis(0) == 0.0f, "Identity emphasis did not settle");
            const std::string prefix = std::string("4p-identity-") + TeamColourTools::palette_key(palette);
            identity_capture(app, *level, presentation, output, prefix + "-overlap-quiet-1280", records);
            const RectangleF quiet = level->player_identity_bounds(0);
            if (palette == TeamColourPair::blue_orange)
            {
                identity_ink_consumer_checks(app, *level, presentation, output);
                identity_background_consumer_checks(app, *level, presentation, output);
#if COLOURWARS_DEV_TOOLS
                identity_ink_tune_checks(app, *level, levels, tuning, presentation, content, output);
                identity_background_tune_checks(app, *level, levels, tuning, presentation, content, output);
#else
                std::ignore = levels; std::ignore = tuning; std::ignore = content;
#endif
                // Pin the shared renderer at exactly one pane-local position
                // and native gameplay size. Team tint, bodies and subpixel
                // camera differences cannot make two roster digits differ.
                const Viewport& viewport = app.viewport_manager()->player_viewport(0);
                const RectangleF projected = level->view_camera(0).calculate_view_rectangle(quiet);
                const RectangleF box(Vector2F(24, 24), projected.size());
                const Canvas canvas(app.render_resources());
                const PlayerBadge badge(&presentation.player_identity(), app.render_resources());
                const auto glyph = [&](int roster)
                {
                    Renderer& renderer = *app.renderer();
                    renderer.begin_frame();
                    renderer.set_view_count(1);
                    DrawList list = renderer.view(0);
                    list.set_viewport(viewport);
                    list.set_camera(Camera::DEFAULT_CAMERA);
                    canvas.draw(list, RectangleF(Vector2F::ZERO, viewport.size()));
                    if (roster >= 0) { badge.draw(list, box, roster); }
                    return read(renderer);
                };
                const Frame blank = glyph(-1);
                std::vector<Frame> digits;
                for (int roster = 0; roster < 4; ++roster)
                {
                    const Frame image = glyph(roster);
                    Frame crop{};
                    require(check_projected_bounds(image, blank, box, viewport, crop) > 0,
                        "Shared roster badge produced no native-size glyph");
                    for (const Frame& earlier : digits)
                    {
                        require(crop.pixels != earlier.pixels,
                            "Two roster badge numbers have identical native-size rasters");
                    }
                    save(crop, output / ("4p-identity-glyph-quiet-1280-player" +
                        std::to_string(roster + 1) + ".ppm"));
                    digits.push_back(std::move(crop));
                }
            }
            level->emphasise_players();
            require(level->player_identity_emphasis(0) == 1.0f &&
                level->player_identity_bounds(0).width > quiet.width,
                "Requested identity emphasis did not expand the world badge");
            identity_capture(app, *level, presentation, output, prefix + "-overlap-emphasis-1280", records);
            preferences.reduce_effects = true;
            require(level->player_identity_visible(0) && level->player_identity_emphasis(0) == 0.0f &&
                level->player_identity_bounds(0).width == quiet.width,
                "Reduce effects hid identity or retained its animated emphasis");
            identity_capture(app, *level, presentation, output, prefix + "-overlap-reduced-1280", records);
            preferences.reduce_effects = false;
            if (palette != TeamColourPair::blue_orange) { continue; }
            for (const Vector2I size : { Vector2I(1920, 1080), Vector2I(2560, 1440) })
            {
                resize(app, *level, size);
                identity_capture(app, *level, presentation, output,
                    prefix + "-overlap-emphasis-" + std::to_string(size.x), records);
            }
            resize(app, *level, { 1280, 720 });
            auto crossing = inputs;
            require(level->place_player(0, { 1400, 1275 }) && level->place_player(1, { 1512, 1275 }),
                "Could not place teammate crossing fixture");
            crossing[static_cast<size_t>(level->player(0).player_num())].x_movement = 1.0f;
            crossing[static_cast<size_t>(level->player(1).player_num())].x_movement = -1.0f;
            for (int tick = 0; tick < 60 &&
                level->player(0).rectangle().x <= level->player(1).rectangle().x; ++tick)
            {
                level->update(crossing, dt);
            }
            require(level->player(0).rectangle().x > level->player(1).rectangle().x,
                "Same-team scripted players did not cross");
            identity_capture(app, *level, presentation, output, prefix + "-crossed-1280", records);
            level->suspend();
            identity_capture(app, *level, presentation, output, prefix + "-paused-1280", records);
            level->resume();
            require(level->kill_player(0), "Could not kill identity fixture");
            level->update(inputs, dt);
            require(!level->player_identity_visible(0), "Dead player retained an owner badge");
            identity_capture(app, *level, presentation, output, prefix + "-dead-1280", records);
            for (int tick = 0; tick < 600 && level->player(0).state() != PlayerState::alive; ++tick)
            {
                level->update(inputs, dt);
            }
            require(level->player(0).state() == PlayerState::alive &&
                level->player_identity_visible(0) && level->player_identity_emphasis(0) > 0.0f,
                "Respawn did not restore and emphasise the owning identity");
            identity_capture(app, *level, presentation, output, prefix + "-respawn-1280", records);
        }
        auto sparse = build(app, builder, 2, TeamColourPair::blue_orange, false, true);
        settle(*sparse, inputs);
        resize(app, *sparse, { 1280, 720 });
        identity_capture(app, *sparse, presentation, output, "2p-identity-sparse-slots-1280", records);
        auto story = build(app, builder, 4, TeamColourPair::blue_orange,
            false, true, WeaponType::sprayer, OpeningMode::quick);
        resize(app, *story, { 1280, 720 });
        require(story->intro_playing() && !story->player_identity_visible(0),
            "Story fixture did not suppress world identity");
        identity_capture(app, *story, presentation, output, "4p-identity-story-hidden-1280", records);
        require(records.good(), "Identity record write failed");
        std::cout << "PASS player identity: three 4P720 palettes, same-team overlap/crossing, sparse/reordered input slots, quiet/emphasis/reduced effects, 1080p/QHD bounds, dead/respawn/story/pause, own-pane world-only raster deltas, frozen cosmetic state and fingerprint\n";
    }
