// Private focused pen proof. Included after common raster/fixture helpers and
// (Debug) tune_colour_checks.h. No event opacity: progress changes path length.
enum class RespawnPenReference { correct, straight, squared_base_alpha };
enum class RespawnPenDraw { outlines, below_boundary, at_boundary, player_only };

class RespawnPenProbe
{
public:
    RespawnPenProbe(Application& app, Registry<LevelLoadedInfo>& levels,
        const GameTuning& tuning, const Presentation& presentation) :
        app_(app), tuning_(tuning), presentation_(presentation), look_(presentation.respawn_redraw()),
        geometry_(look_), redraw_(&look_, app.render_resources()),
        sheet_(app.render_resources()->sprite_sheet(look_.sprite_sheet)),
        frame_(sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.pen_frame)))
    {
        require(look_.canvas_size == Vector2F(68, 120) && look_.pen_width == 4.0f,
            "Pen fixture requires the reviewed68x120/nib4 recipe");
        preferences_.music_volume = 0; preferences_.sfx_volume = 0;
        preferences_.view_shake = false; preferences_.reduce_effects = false;
        MatchSetup settings; settings.set_player_count(1); settings.set_stage("movement_course");
        settings.set_screen_layout(ScreenLayout::one_player); settings.begin_input_claims(3);
        settings.set_player_team(0, PlayerTeam::a); settings.set_player_weapon(0, WeaponType::sprayer);
        PlayerBuilder builder(&tuning, &presentation);
        auto players = builder.build_players(settings, levels.get("movement_course"),
            TeamColour(Colour(44, 168, 206), Colour(245, 143, 64)),
            app.render_resources(), app.audio_resources(), &preferences_, app.viewport_manager());
        require(players.size() == 1, "Pen fixture lost its actual sparse builder Player");
        player_ = std::move(players.front()); player_->kill();
        player_->update(0.0f); ++player_preparation_updates_;
        player_->update(tuning.player().respawn_delay * 0.5f); ++player_preparation_updates_;
        require(player_->state() == PlayerState::dead && player_->animation_state() == PlayerAnimationState::idle &&
            player_->combat_feedback().elimination_remaining() == 0.0f &&
            player_->respawn_progress() > look_.draw_from && player_->respawn_progress() < look_.fill_at,
            "Actual Player did not reach a frozen clean outline after two local preparations");
        const RectangleF canvas = player_->figure_canvas(player_->spawn_rectangle());
        require(canvas.width == 68.0f && canvas.height == 120.0f,
            "Actual Player pen canvas no longer fits the compact proof");
        player_camera_ = Camera(canvas.position() - Vector2F(510, 240), 1.0f);
        player_state_ = snapshot();
    }

    void unchanged() const
    {
        const auto& a = look_; const auto& b = geometry_;
        require(&look_ == &presentation_.respawn_redraw() && a.sprite_sheet == b.sprite_sheet &&
            a.pen_frame == b.pen_frame && a.canvas_size == b.canvas_size && a.pen_width == b.pen_width &&
            a.draw_from == b.draw_from && a.fill_at == b.fill_at && a.camera_hold == b.camera_hold &&
            a.strokes.size() == b.strokes.size(), "Pen edit changed borrowed geometry, resources or boundaries");
        for (size_t i = 0; i < a.strokes.size(); ++i)
        {
            require(a.strokes[i].part == b.strokes[i].part && a.strokes[i].points == b.strokes[i].points,
                "Pen edit changed ordered stroke point values");
        }
        require(snapshot() == player_state_, "Pen drawing/command changed frozen actual Player public state");
    }

    Frame actual(const Colour& background, RespawnPenDraw mode = RespawnPenDraw::outlines)
    {
        unchanged(); std::srand(7129); const int expected = std::rand(); std::srand(7129);
        const auto draw = [&](DrawList& list)
        {
            if (mode != RespawnPenDraw::player_only)
            {
                for (size_t i = 0; i < 4; ++i)
                {
                    const float progress = mode == RespawnPenDraw::below_boundary ? look_.draw_from * 0.5f :
                        mode == RespawnPenDraw::at_boundary ? look_.draw_from : path_progress(i);
                    redraw_.draw(list, figure(i), (i % 2) == 0, progress);
                }
            }
            if (mode == RespawnPenDraw::outlines || mode == RespawnPenDraw::player_only)
            { list.set_camera(player_camera_); player_->draw(list); }
        };
        Frame first = render(background, draw);
        require(first.pixels == render(background, draw).pixels, "Frozen pen raster changed on repeat");
        require(std::rand() == expected, "Pen drawing consumed gameplay randomness");
        unchanged(); return first;
    }

    Frame reference(const Colour& background, const Colour& colour,
        RespawnPenReference mode = RespawnPenReference::correct)
    {
        // A separate Manhattan prefix sweep, preserving every ordered nib
        // submission/overlap. No production draw_pen or premultiplication helper.
        const float factor = mode == RespawnPenReference::straight ? 1.0f :
            mode == RespawnPenReference::squared_base_alpha ? colour.a * colour.a : colour.a;
        const Colour tint(colour.r * factor, colour.g * factor, colour.b * factor, colour.a);
        return render(background, [&](DrawList& list)
        {
            const auto outline = [&](const RectangleF& canvas, bool facing, float progress)
            {
                double total = 0.0;
                for (const auto& stroke : geometry_.strokes)
                { for (size_t i = 1; i < stroke.points.size(); ++i)
                    { const Vector2F delta = stroke.points[i] - stroke.points[i - 1];
                      total += static_cast<double>(std::abs(delta.x) + std::abs(delta.y)); } }
                double budget = total * std::clamp(static_cast<double>((progress - geometry_.draw_from) /
                    (geometry_.fill_at - geometry_.draw_from)), 0.0, 1.0);
                for (const auto& stroke : geometry_.strokes)
                {
                    for (size_t i = 1; i < stroke.points.size() && budget > 0.0; ++i)
                    {
                        const Vector2F start = stroke.points[i - 1], delta = stroke.points[i] - start;
                        const float length = std::abs(delta.x) + std::abs(delta.y);
                        const float paid = static_cast<float>(std::min(budget, static_cast<double>(length)));
                        const Vector2F end = start + delta * (paid / length);
                        float x = std::min(start.x, end.x), y = std::min(start.y, end.y);
                        const float w = std::abs(end.x - start.x) + geometry_.pen_width;
                        const float h = std::abs(end.y - start.y) + geometry_.pen_width;
                        if (!facing) { x = geometry_.canvas_size.x - x - w; }
                        const float sx = canvas.width / geometry_.canvas_size.x;
                        const float sy = canvas.height / geometry_.canvas_size.y;
                        list.draw_sprite(sheet_->texture(), frame_.source_rectangle(),
                            RectangleF(canvas.x + x * sx, canvas.y + y * sy, w * sx, h * sy),
                            tint, 0.0f, frame_.origin(), SpriteFlip::none, 0.0f);
                        budget -= static_cast<double>(length);
                    }
                }
            };
            for (size_t i = 0; i < 4; ++i) { outline(figure(i), (i % 2) == 0, path_progress(i)); }
            list.set_camera(player_camera_);
            outline(player_->figure_canvas(player_->spawn_rectangle()), player_->facing_right(), player_->respawn_progress());
        });
    }

    void channel_and_overlap(const Frame& image, const Colour& background, const Colour& colour) const
    {
        // Both completed cap corners receive two ordered nib submissions at
        // quarter and three-quarter budgets. Progress must not fade their ink.
        for (const int x : { 239, 399 })
        {
            const std::uint32_t pixel = monitor_rgb(image, x, 257);
            const std::array<float, 3> source{ colour.r, colour.g, colour.b };
            const std::array<float, 3> under{ background.r, background.g, background.b };
            for (size_t channel = 0; channel < 3; ++channel)
            {
                const float first = std::round((source[channel] * colour.a + under[channel] * (1.0f - colour.a)) * 255.0f);
                const int expected = static_cast<int>(std::round(source[channel] * colour.a * 255.0f + first * (1.0f - colour.a)));
                const int shift = 16 - static_cast<int>(channel) * 8;
                const int actual = static_cast<int>((pixel >> shift) & 255U);
                require(std::abs(actual - expected) <= 1, "Ordered pen join lost explicit source-over channels or acquired progress fade");
            }
        }
    }

    void blank(const Frame& image, const Colour& background) const
    {
        const auto channel = [](float value) { return static_cast<unsigned char>(std::round(value * 255.0f)); };
        for (size_t i = 0; i < image.pixels.size(); i += 4)
        { require(image.pixels[i] == channel(background.r) && image.pixels[i + 1] == channel(background.g) &&
            image.pixels[i + 2] == channel(background.b) && image.pixels[i + 3] == 255,
            "Zero-alpha/boundary pen failed to restore its full underlay"); }
    }

    void outside_panel(const Frame& image, const Colour& background) const
    {
        const RectangleI crop = panel();
        const std::uint32_t colour = (static_cast<std::uint32_t>(std::round(background.r * 255)) << 16) |
            (static_cast<std::uint32_t>(std::round(background.g * 255)) << 8) |
            static_cast<std::uint32_t>(std::round(background.b * 255));
        for (int y = 0; y < image.height; ++y) { for (int x = 0; x < image.width; ++x)
            { if (x < crop.x || x >= crop.right() || y < crop.y || y >= crop.bottom())
                { require(monitor_rgb(image, x, y) == colour, "Pen output escaped its compact panel"); } } }
    }

    void visible_player(const Frame& image, const Colour& background) const
    {
        const std::uint32_t under = (static_cast<std::uint32_t>(std::round(background.r * 255)) << 16) |
            (static_cast<std::uint32_t>(std::round(background.g * 255)) << 8) |
            static_cast<std::uint32_t>(std::round(background.b * 255));
        bool visible = false;
        for (int y = 240; y < 360; ++y) { for (int x = 510; x < 578; ++x)
            { visible = visible || monitor_rgb(image, x, y) != under; } }
        require(visible, "Actual whole/filled Player bypass was vacuously blank");
    }

    Frame reduced(const Colour& background)
    {
        require(!preferences_.reduce_effects, "Pen Reduce Effects fixture already enabled");
        preferences_.reduce_effects = true; Frame result = actual(background);
        preferences_.reduce_effects = false; unchanged(); return result;
    }

    void whole(bool value)
    {
        if (value) { whole_saved_state_ = player_state_; player_->set_pose({ Vector2F::ZERO, 0.0f, false, true }); }
        else { player_->set_pose({}); require(snapshot() == whole_saved_state_, "Whole pose failed to restore the frozen Player"); }
        player_state_ = snapshot();
    }

    void prepare_filled()
    {
        unchanged(); player_->update(tuning_.player().respawn_delay *
            (look_.fill_at - player_->respawn_progress() + 0.01f)); ++player_preparation_updates_;
        require(player_->state() == PlayerState::dead && player_->respawn_progress() >= look_.fill_at &&
            player_->combat_feedback().elimination_remaining() == 0.0f,
            "Actual Player missed the retained filled bypass");
        player_state_ = snapshot();
    }

    size_t readbacks() const { return readbacks_; }
    size_t preparations() const { return player_preparation_updates_; }
    static RectangleI panel() { return { 180, 220, 440, 160 }; }

private:
    float path_progress(size_t i) const
    { return geometry_.draw_from + (geometry_.fill_at - geometry_.draw_from) * (i < 2 ? 0.25f : 0.75f); }
    static RectangleF figure(size_t i) { return { 190.0f + static_cast<float>(i) * 80.0f, 240, 68, 120 }; }
    std::vector<float> snapshot() const
    {
        const Player& p = *player_;
        std::vector<float> v{ static_cast<float>(p.state()), p.health(), p.weapon_ammo(),
            p.velocity().x, p.velocity().y, p.respawn_timer(), p.respawn_progress(),
            p.facing_right() ? 1.0f : 0.0f, static_cast<float>(p.animation_state()), p.action_age(),
            static_cast<float>(p.action_frame()), p.drawn_whole() ? 1.0f : 0.0f,
            p.camera_focus().x, p.camera_focus().y, p.camera().translation.x, p.camera().translation.y,
            p.camera().scale, p.combat_feedback().hurt_remaining(), p.combat_feedback().elimination_remaining() };
        for (const RectangleF& r : { p.rectangle(), p.spawn_rectangle(), p.posed_rectangle(), p.bounds(), p.cull_bounds(1.0f) })
        { v.insert(v.end(), { r.x, r.y, r.width, r.height }); }
        return v;
    }
    Frame render(const Colour& background, const std::function<void(DrawList&)>& draw)
    {
        const Canvas canvas(app_.render_resources(), background);
        Renderer& renderer = *app_.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app_.viewport_manager()->fullscreen_viewport());
        list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        canvas.draw(list, { 0, 0, 1280, 720 }); draw(list); ++readbacks_; return read(renderer);
    }
    Application& app_; const GameTuning& tuning_; const Presentation& presentation_;
    const RespawnRedrawPresentation& look_; RespawnRedrawPresentation geometry_; RespawnRedraw redraw_;
    const SpriteSheet* sheet_; const SpriteFrame& frame_; PlaySettings preferences_;
    std::unique_ptr<Player> player_; Camera player_camera_; std::vector<float> player_state_, whole_saved_state_;
    size_t player_preparation_updates_ = 0, readbacks_ = 0;
};

void respawn_pen_consumer_checks(Application& app, Registry<LevelLoadedInfo>& levels,
    const GameTuning& tuning, const Presentation& presentation, const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    Presentation local = presentation; auto& look = const_cast<RespawnRedrawPresentation&>(local.respawn_redraw());
    require(look.pen_colour == Colour(0, 0, 0), "Reviewed opaque BLACK pen baseline changed");
    RespawnPenProbe probe(app, levels, tuning, local);
    struct Palette { const char* name; Colour colour; };
    const std::array<Palette, 4> palettes{{ { "default", Colour(0, 0, 0) }, { "rgb", Colour(53, 188, 225) },
        { "rgba128", Colour(32, 96, 192, 128) }, { "alpha0", Colour(165, 50, 214, 0) } }};
    const std::array<Colour, 2> backgrounds{ Colour(0, 0, 0), Colour(32, 64, 96) };
    std::array<Frame, 2> defaults, partial;
    size_t rows = 0, crops = 0, controls = 0;
    std::ofstream records(output / "respawn-pen-consumers.csv");
    records << "state,background,normal_match,ordered_nib_channels,frozen_player_geometry_rng,control,scope\n";
    const auto row = [&](const char* state, size_t background, const char* control)
    { records << state << ',' << (background == 0 ? "black" : "nonblack")
        << ",true,true,true," << control << ",consumer-only\n"; ++rows; };
    for (size_t p = 0; p < palettes.size(); ++p)
    {
        look.pen_colour = palettes[p].colour;
        for (size_t b = 0; b < backgrounds.size(); ++b)
        {
            const Frame actual = probe.actual(backgrounds[b]); probe.outside_panel(actual, backgrounds[b]);
            require(actual.pixels == probe.reference(backgrounds[b], palettes[p].colour).pixels,
                "Pen differs from independent ordered raw nib/channel oracle");
            const Frame control = probe.reference(backgrounds[b], palettes[p].colour, RespawnPenReference::straight);
            if (p < 2)
            { require(actual.pixels == control.pixels, "Opaque pen pixels lost legacy parity"); }
            else { require(actual.pixels != control.pixels, "Pen fixture cannot reject straight-alpha output"); }
            ++controls;
            if (p == 0) { defaults[b] = actual; }
            else { require(actual.pixels != defaults[b].pixels || (p == 3 && b == 0),
                "Nondefault pen did not change retained outlines"); }
            if (p == 2)
            {
                partial[b] = actual; probe.channel_and_overlap(actual, backgrounds[b], palettes[p].colour);
                require(actual.pixels != probe.reference(backgrounds[b], palettes[p].colour,
                    RespawnPenReference::squared_base_alpha).pixels, "Pen fixture cannot reject squared base alpha"); ++controls;
            }
            if (p == 3) { probe.blank(actual, backgrounds[b]); }
            row(palettes[p].name, b, p < 2 ? "opaque-legacy-parity" : p == 2 ? "straight-and-squared-alpha-rejected" : "straight-alpha-rejected");
            save(foreground_crop(actual, RespawnPenProbe::panel(), 0), output /
                (std::string("respawn-pen-") + palettes[p].name + (b == 0 ? "-black.ppm" : "-nonblack.ppm"))); ++crops;
        }
    }
    look.pen_colour = palettes[1].colour; // Boundary proof must use visible opaque ink.
    for (size_t b = 0; b < backgrounds.size(); ++b)
    {
        for (const auto mode : { RespawnPenDraw::below_boundary, RespawnPenDraw::at_boundary })
        { probe.blank(probe.actual(backgrounds[b], mode), backgrounds[b]);
          row(mode == RespawnPenDraw::below_boundary ? "below-draw-from" : "at-draw-from", b, "blank-boundary"); }
    }
    look.pen_colour = palettes[2].colour;
    for (size_t b = 0; b < backgrounds.size(); ++b)
    { require(probe.reduced(backgrounds[b]).pixels == partial[b].pixels, "Reduce Effects suppressed essential respawn pen");
      row("reduce-effects", b, "essential-outline-retained"); }
    const auto bypass = [&](const char* state)
    {
        for (size_t b = 0; b < backgrounds.size(); ++b)
        {
            look.pen_colour = palettes[1].colour; const Frame opaque = probe.actual(backgrounds[b], RespawnPenDraw::player_only);
            probe.visible_player(opaque, backgrounds[b]);
            look.pen_colour = palettes[3].colour;
            require(opaque.pixels == probe.actual(backgrounds[b], RespawnPenDraw::player_only).pixels,
                "Whole/filled actual Player unexpectedly uses the pen tint");
            row(state, b, "actual-player-pen-bypass");
        }
    };
    probe.whole(true); bypass("whole-dead"); probe.whole(false);
    probe.prepare_filled(); bypass("filled-dead");
    require(records.good() && probe.readbacks() == 62 && rows == 18 && crops == 8 && controls == 10 && probe.preparations() == 3,
        "Respawn pen shared proof exceeded approved exact budgets");
    std::ofstream summary(output / "respawn-pen-consumer-summary.json");
    summary << "{\"scope\":\"consumer-only\",\"state_updates\":0,\"level_updates\":0,\"input_updates\":0,"
        "\"player_preparation_updates\":3,\"readbacks\":62,\"csv_rows\":18,\"crops\":8,\"controls\":10,"
        "\"direct_outlines\":4,\"actual_players\":1,\"event_fade\":false}\n";
    require(summary.good(), "Cannot write respawn pen consumer summary");
    std::cout << "PASS respawn pen consumers:62readbacks/18rows/8compact440x160crops; ordered raw nib/channel oracle, "
        "two facings/two path budgets and actual frozen dead Player; boundaries, Reduce Effects and whole/filled bypass. "
        "Three local Player preparations, zero Level/StateContext/input updates; no event fade, hardware or route claim.\n";
}

#if COLOURWARS_DEV_TOOLS
// Nine real commands;54 readbacks/18rows;two local Player preparations only.
void respawn_pen_tune_checks(Application& app, Registry<LevelLoadedInfo>& levels,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& content,
    const std::filesystem::path& output)
{
    const auto root = output / "respawn-pen-tune-scratch", source = root / "source", staged = root / "staged";
    std::filesystem::create_directories(source); std::filesystem::create_directories(staged);
    constexpr std::array<const char*, 2> files{ "tuning.json", "presentation.json" };
    std::array<std::string, 2> real_bytes;
    for (size_t i = 0; i < files.size(); ++i)
    { real_bytes[i] = tune_colour_bytes(content / files[i]);
      tune_colour_write(source / files[i], real_bytes[i]); tune_colour_write(staged / files[i], real_bytes[i]); }
    const NestedSaveProfile profile(root / "profile"); const TuneColourTemp temporary(root / "temp");
    const GameTuning original_tuning = tuning; const Presentation original = presentation;
    struct RestorePenContent
    { GameTuning& tuning; Presentation& presentation; GameTuning old_tuning; Presentation old_presentation;
      ~RestorePenContent() { tuning = old_tuning; presentation = old_presentation; } }
        restore{ tuning, presentation, original_tuning, original };
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    Save preferences; PlaySettings play; play.music_volume = 17; play.sfx_volume = 83;
    play.aim_sensitivity = 4; play.view_shake = false; play.aim_marker = true; play.reduce_effects = true;
    play.sticks[2].movement_deadzone = 25; play.sticks[2].aim_deadzone = 70;
    InputBindings bindings; bindings.keyboard_jump = Key::j;
    require(preferences.set_play_settings_and_save(play) && preferences.set_input_bindings_and_save(bindings),
        "Cannot seed private pen PlaySettings and JumpJ preferences");
    const auto save_path = root / "profile/ColourWars/save_data.json";
    const std::string saved_preferences = tune_colour_bytes(save_path);
    const auto preference_values = [&]
    {
        const auto& p = preferences.play_settings();
        std::vector<int> v{ static_cast<int>(preferences.resolution()), preferences.fullscreen() ? 1 : 0,
            static_cast<int>(preferences.team_palette()), p.aim_sensitivity, p.music_volume, p.sfx_volume,
            p.view_shake ? 1 : 0, p.aim_marker ? 1 : 0, p.reduce_effects ? 1 : 0,
            static_cast<int>(preferences.input_bindings().keyboard_jump) };
        for (const auto& stick : p.sticks) { v.push_back(stick.movement_deadzone); v.push_back(stick.aim_deadzone); }
        return v;
    };
    const auto saved_values = preference_values();
    StageList stages;
    for (const char* name : { "movement_course", "close_quarters", "king_of_the_hill", "turbulence" })
    { const auto* info = levels.get(name); stages.push_back({ name, info->display_name(), info->icon_frame(), info->include_in_random() }); }
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_level_infos(&levels);
    data.set_stages(&stages); data.set_tuning(&tuning); data.set_presentation(&presentation);
    DebugDirector director(&tuning, &presentation); data.set_debug(&director); auto& impl = *director.impl();
    impl.set_content_directories(source, staged); impl.set_user_directory(root / "user");
    impl.set_script_directories({ root / "scripts" }); director.attach(&app, &data);
    impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output / "respawn-pen-tune-commands.log").string()), "Cannot retain pen transcript");
    RespawnPenProbe probe(app, levels, tuning, presentation);
    JsonSpanDocument initial(real_bytes[1], "private pen source");
    const JsonLeaf* leaf = initial.find("respawn_redraw.pen_colour");
    require(leaf != nullptr, "Pen source has no existing exact colour token");
    const std::string expected_saved = initial.replaced(*leaf, "\"2060C080\"");
    bool persisted = false; size_t commands = 0, rows = 0;
    const auto unchanged = [&]
    {
        probe.unchanged();
        require(!impl.can_apply() && impl.request().empty() && !impl.in_match() &&
            tune_colour_bytes(save_path) == saved_preferences && preference_values() == saved_values,
            "Pen commands changed private preferences, JumpJ/default display or queued match work");
        for (size_t i = 0; i < files.size(); ++i)
        { const auto& expected = i == 1 && persisted ? expected_saved : real_bytes[i];
          require(tune_colour_bytes(content / files[i]) == real_bytes[i] &&
              tune_colour_bytes(source / files[i]) == expected && tune_colour_bytes(staged / files[i]) == expected,
              "Pen commands changed actual content or private bytes beyond one token"); }
    };
    const auto command = [&](const std::string& line)
    {
        const size_t before = impl.log().lines().size();
        std::srand(7129); const int expected = std::rand(); std::srand(7129);
        const auto status = impl.execute(line); ++commands;
        require(status.ok && std::rand() == expected, "Real pen command failed or consumed gameplay RNG");
        if (line.starts_with("tune "))
        { const auto& log = impl.log().lines();
          require(log.size() > before && log.back().severity == DebugSeverity::info &&
              log.back().text.starts_with("tune: presentation.respawn_redraw.pen_colour") &&
              log.back().text.ends_with("(live)"), "Pen command lost exact live adoption scope"); }
    };
    std::ofstream records(output / "respawn-pen-tune-colours.csv");
    records << "state,background,normal_match,ordered_nib_channels,frozen_player_geometry_rng,private_bytes,adoption\n";
    const std::array<Colour, 2> backgrounds{ Colour(0, 0, 0), Colour(32, 64, 96) };
    const auto check = [&](const char* state, const Colour& expected)
    {
        unchanged(); require(presentation.respawn_redraw().pen_colour == expected, "Pen command parsed unexpected literal channels");
        for (size_t b = 0; b < backgrounds.size(); ++b)
        {
            const Frame actual = probe.actual(backgrounds[b]); probe.outside_panel(actual, backgrounds[b]);
            require(actual.pixels == probe.reference(backgrounds[b], expected).pixels,
                "Preconstructed pen/actual Player failed independent live nib oracle");
            records << state << ',' << (b == 0 ? "black" : "nonblack") << ",true,true,true,true,live\n"; ++rows;
        }
        unchanged();
    };
    const Colour black(0, 0, 0), rgb(53, 188, 225), rgba(32, 96, 192, 128);
    check("baseline", black);
    command("tune presentation.respawn_redraw.pen_colour RED"); check("named", Colour(255, 0, 0));
    command("tune.reset presentation.respawn_redraw.pen_colour"); check("reset-original", black);
    command("tune presentation.respawn_redraw.pen_colour 35BCE1"); check("rgb", rgb);
    command("tune presentation.respawn_redraw.pen_colour 2060C080"); check("rgba128", rgba);
    const TuneDocument* document = impl.tune_document("presentation");
    require(document != nullptr && document->edits().size() == 1 && document->edits().contains("respawn_redraw.pen_colour"),
        "Pen command edited outside its single existing token");
    command("tune.save presentation"); persisted = true; unchanged();
    require(impl.tune_document("presentation")->edits().empty(), "Single-token pen save failed to rebase");
    command("tune presentation.respawn_redraw.pen_colour A532D600"); check("alpha0", Colour(165, 50, 214, 0));
    command("tune.reset presentation.respawn_redraw.pen_colour"); check("reset-saved", rgba);
    command("tune presentation.respawn_redraw.pen_colour 35BCE1"); check("rgb-after-save", rgb);
    command("tune.reset presentation"); check("reset-all-saved", rgba);
    require(impl.tune_document("presentation")->edits().empty() && impl.tune_document("tuning")->edits().empty(),
        "Pen final reset retained private edit ledgers");
    require(records.good() && commands == 9 && rows == 18 && probe.readbacks() == 54 && probe.preparations() == 2,
        "Respawn pen Debug proof exceeded approved exact counts");
    std::ofstream summary(output / "respawn-pen-tune-summary.json");
    summary << "{\"scope\":\"debug-validated-commands\",\"state_updates\":0,\"level_updates\":0,\"input_updates\":0,"
        "\"player_preparation_updates\":2,\"commands\":9,\"readbacks\":54,\"csv_rows\":18,\"crops\":0,"
        "\"saved_tokens\":1,\"event_fade\":false}\n";
    require(summary.good(), "Cannot write respawn pen Debug summary");
    std::cout << "PASS respawn pen tune:9real commands/54readbacks/18rows; single-token private save/rebase/reset, "
        "private PlaySettings/JumpJ/default display, frozen actual Player/nib geometry/RNG and no queued match work. "
        "Two local Player preparations; zero Level/StateContext/input updates.\n";
}
#endif
