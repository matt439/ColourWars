// ROAD#31: actual GameLevel qualification on the hidden native shell. The
// explicit template instantiations inspect private state without changing the
// production layout or adding test-only accessors. No drawing or readback.
namespace practice_replay_acceptance
{
    template<class Tag, typename Tag::Type Member> struct Access
    {
        friend typename Tag::Type member(Tag) { return Member; }
    };
    struct TapeTag { using Type = RoundTape GameLevel::*; friend Type member(TapeTag); };
    struct SettingsTag { using Type = MatchSetup GameLevel::*; friend Type member(SettingsTag); };
    struct StartTag { using Type = MatchStart GameLevel::*; friend Type member(StartTag); };
    struct BuildTag { using Type = void (GameLevel::*)(); friend Type member(BuildTag); };
    struct EndMenuTag { using Type = void (GameLevel::*)(); friend Type member(EndMenuTag); };
    struct StepReplayTag { using Type = void (GameLevel::*)(); friend Type member(StepReplayTag); };
    struct EndReplayTag { using Type = void (GameLevel::*)(); friend Type member(EndReplayTag); };
    struct UpdateTag
    {
        using Type = void (GameLevel::*)(const std::vector<SlotInput>&, float, SeatInputFilter*);
        friend Type member(UpdateTag);
    };
    template struct Access<TapeTag, &GameLevel::tape_>;
    template struct Access<SettingsTag, &GameLevel::settings_>;
    template struct Access<StartTag, &GameLevel::start_>;
    template struct Access<BuildTag, &GameLevel::build_and_enter_level>;
    template struct Access<EndMenuTag, &GameLevel::open_end_menu>;
    template struct Access<StepReplayTag, &GameLevel::step_replay>;
    template struct Access<EndReplayTag, &GameLevel::end_replay>;
    template struct Access<UpdateTag, &GameLevel::step_match>;

    struct GapsTag { friend size_t gaps(GapsTag, const RoundTape&); };
    template<auto Member> struct GapsAccess
    {
        friend size_t gaps(GapsTag, const RoundTape& tape) { return (tape.*Member).size(); }
    };
    template struct GapsAccess<&RoundTape::gaps_>;

    const RoundTape& tape(const GameLevel& game) { return game.*member(TapeTag{}); }
    size_t gaps(const GameLevel& game) { return gaps(GapsTag{}, tape(game)); }
    void empty(const GameLevel& game)
    {
        require(game.level().practice() && tape(game).steps() == 0 && gaps(game) == 0 && !tape(game).playable(),
            "Untimed actual GameLevel retained shipping replay inputs or suspend/resume gaps");
    }

    MatchSetup settings(const char* stage, int count = 1)
    {
        MatchSetup result; result.set_stage(stage); result.set_duration(3.0f);
        result.set_player_count(count); result.begin_input_claims(input_slots::keyboard_and_mouse);
        for (int seat = 0; seat < count; ++seat)
        {
            if (seat > 0) { result.set_player_bot(seat, BotDifficulty::normal); }
            result.set_player_team(seat, seat % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
            result.set_player_weapon(seat, WeaponType::sprayer);
        }
        result.set_screen_layout(ScreenLayout::one_player);
        return result;
    }

    // Exact bits of public gameplay quantities, plus every scoring tile's
    // identity/geometry/ownership. Cosmetic countdown and emphasis clocks are
    // deliberately absent: replay bypasses that opening and re-emphasizes seats.
    struct Moment
    {
        std::vector<std::uint32_t> players;
        int projectiles = 0;
        std::uint64_t paint = 14695981039346656037ULL;
        bool operator==(const Moment&) const = default;
    };
    Moment moment(const Level& level)
    {
        Moment result; result.projectiles = level.projectile_count();
        for (int seat = 0; seat < level.player_count(); ++seat)
        {
            const Player& player = level.player(seat);
            const RectangleF body = player.rectangle();
            const Colour colour = player.team_colour();
            for (float value : { body.x, body.y, body.width, body.height, player.velocity().x,
                player.velocity().y, player.health(), player.weapon_ammo(), player.respawn_timer(),
                static_cast<float>(player.state()), static_cast<float>(player.move_state()),
                colour.r, colour.g, colour.b, colour.a })
            { result.players.push_back(std::bit_cast<std::uint32_t>(value)); }
        }
        const auto hash = [&](std::uint64_t value)
        { result.paint ^= value; result.paint *= 1099511628211ULL; };
        for (const PaintSurfaceSample& tile : level.paint_surfaces())
        {
            hash(tile.source_index); hash(static_cast<unsigned int>(tile.face));
            hash(static_cast<unsigned int>(tile.team)); hash(static_cast<unsigned int>(tile.paint_owner));
            hash(std::bit_cast<std::uint32_t>(tile.scored_area));
            for (const Vector2F& corner : tile.corners)
            { hash(std::bit_cast<std::uint32_t>(corner.x)); hash(std::bit_cast<std::uint32_t>(corner.y)); }
        }
        return result;
    }
}

void practice_replay_checks(Application& app, Registry<LevelLoadedInfo>& levels,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    using namespace practice_replay_acceptance;
    const NestedSaveProfile profile(output / "practice-replay-profile");
    Save preferences; SettingsSession preferences_session(preferences);
    auto& play = const_cast<PlaySettings&>(preferences_session.play_settings());
    play.music_volume = play.sfx_volume = 0; play.aim_marker = false;
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false },
        { "crossfeed", "Crossfeed", "stage_turbulence", true } };
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_settings_session(&preferences_session);
    const StageCatalog stage_catalog = acceptance::catalog_of(levels, stages, { "crossfeed", "movement_course" }); data.set_stage_catalog(&stage_catalog); data.set_tuning(&tuning); data.set_presentation(&presentation);
    // The inert director leaves the shipping path intact in both presets.
    // Debug's independent recorder is supplied explicitly below.
    StateContext& stack = app;
    const bool old_active = stack.active(), old_keyboard = app.keyboard()->focused(), old_mouse = app.mouse()->focused();
    struct Cleanup
    {
        StateContext& stack; Application& app; bool active, keyboard, mouse;
        ~Cleanup()
        {
            stack.clear();
            for (Key key : { Key::a, Key::d, Key::space, Key::escape, Key::enter, Key::up, Key::down })
            { app.keyboard()->on_key_up(key); }
            app.mouse()->on_button_up(MouseButton::left);
            app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll();
            stack.notify_activation(active); app.keyboard()->set_focused(keyboard); app.mouse()->set_focused(mouse);
        }
    } cleanup{ stack, app, old_active, old_keyboard, old_mouse };
    stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
    app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll();
    int updates = 0, practice_steps = 0, replay_steps = 0, timed_cases = 0;
    std::ofstream csv(output / "practice-replay.csv");
    csv << "case,practice,tape_steps,gaps,reproduced_steps\n";
    const auto note = [&](const char* name, const GameLevel& game, int reproduced = 0)
    { csv << name << ',' << game.level().practice() << ',' << tape(game).steps() << ',' << gaps(game) << ',' << reproduced << '\n'; };
    const auto tick = [&](float elapsed = dt)
    { app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed); ++updates; };
    const auto tap = [&](Key key)
    { app.keyboard()->on_key_down(key); tick(0.0f); app.keyboard()->on_key_up(key); tick(0.0f); };
    const auto push = [&](const MatchSetup& plan, MatchStart start)
    {
        stack.clear(); stack.push(std::make_unique<ResumeProbeRoot>());
        auto owned = std::make_unique<GameLevel>(&data, plan, start); GameLevel* game = owned.get();
        stack.push<EndMenuAction>(std::move(owned), [](const EndMenuAction&) {});
        return game;
    };
    const auto live = [&](GameLevel& game)
    {
        for (int step = 0; step < 1200 && game.level().state() != LevelState::active; ++step) { tick(); }
        require(stack.depth() == 2 && game.level().state() == LevelState::active, "Actual GameLevel did not enter play");
    };
    const auto pause = [&]
    { tap(Key::escape); require(stack.depth() == 3, "Actual practice/replay fixture failed to open Pause"); };
    const auto resume = [&](GameLevel& game)
    {
        tap(Key::enter); require(stack.depth() == 2 && game.resume_remaining() > 0.0f, "Actual Resume omitted count-in");
        tick(game.resume_remaining() + 1.0f); require(game.resume_remaining() == 0.0f, "Actual Resume did not complete");
    };

    GameLevel* game = push(settings("movement_course"), MatchStart::menu);
    live(*game); empty(*game);
    for (int step = 0; step < 3600; ++step) { tick(); ++practice_steps; }
    empty(*game); note("sustained-practice-60-seconds", *game);
    pause(); empty(*game); resume(*game); empty(*game);
    stack.notify_activation(false); stack.notify_activation(true);
    require(game->resume_remaining() > 0.0f, "Practice focus return omitted count-in");
    tick(game->resume_remaining() + 1.0f); empty(*game); note("practice-pause-focus-resume", *game);
    pause(); tap(Key::down); tap(Key::down); tap(Key::enter); tick(0.0f); tap(Key::up); tap(Key::enter);
    require(stack.depth() == 2 && game->level().state() == LevelState::start_countdown,
        "Actual practice Restart did not rebuild the match");
    live(*game);
    for (int step = 0; step < 600; ++step) { tick(); ++practice_steps; }
    empty(*game); note("practice-restart", *game);

#if COLOURWARS_DEV_TOOLS
    // The actual Debug recorder sits downstream of the shipping filter, as in
    // GameLevel::update's step loop. This deliberately keeps the existing practice
    // Level: the user-facing record command rebuilds a developer/timed course.
    const std::vector<SlotInput> neutral = acceptance::slot_inputs(neutral_inputs());
    const Vector2F before_debug = game->level().player(0).rectangle().position();
    class Drive final : public SeatInputFilter
    {
    public:
        void filter_seats(std::vector<PlayerInputData>& seats, const Level&) override
        { seats.at(0).x_movement = 1.0f; }
    } drive;
    ReplayRecorder driven_recorder("practice-driven", ReplayFile{}, &drive, 0);
    for (int step = 0; step < 120; ++step)
    { (game->*member(UpdateTag{}))(neutral, dt, &driven_recorder); ++practice_steps; }
    empty(*game);
    require(driven_recorder.file().inputs.size() == 1 && driven_recorder.file().inputs[0].size() == 120 &&
        game->level().player(0).rectangle().position() != before_debug,
        "Disabled shipping tape blocked actual Debug recording or its downstream input driver");
    for (const PlayerInputData& input : driven_recorder.file().inputs[0])
    { require(input.x_movement == 1.0f, "Debug recorder missed the downstream driver's final input"); }
    note("independent-debug-recorder", *game);
#endif

    // Real timed GameLevels: ordinary menu solo, developer timed course, and a
    // human-versus-shipping-bot match. All use the normal physical backend;
    // only the keyboard/mouse host is claimed, so no pad is fabricated.
    for (int which = 0; which < 3; ++which)
    {
        const char* name = which == 0 ? "timed-menu-solo" : which == 1 ? "timed-developer-course" : "timed-multiplayer";
        game = push(settings(which == 1 ? "movement_course" : "crossfeed", which == 2 ? 2 : 1),
            which == 1 ? MatchStart::developer : MatchStart::menu);
        require(!game->level().practice(), "Timed actual GameLevel was mistaken for untimed practice");
        // Retain the actual authored opening/countdown. It supplies no taped
        // steps; collect from the first real simulation step, including 2P's
        // full opening before the delayed pause.
        std::vector<Moment> seen;
        app.mouse()->on_move({ 700, 360 }); app.mouse()->on_button_down(MouseButton::left);
        app.keyboard()->on_key_down(Key::d);
        bool paused = false;
        for (int step = 0; step < 2400 && game->level().state() != LevelState::zoom_out; ++step)
        {
            if (tape(*game).steps() == 60 && !paused) { pause(); resume(*game); paused = true; }
            const int before = tape(*game).steps(); tick();
            if (tape(*game).steps() > before) { seen.push_back(moment(game->level())); }
        }
        app.keyboard()->on_key_up(Key::d); app.mouse()->on_button_up(MouseButton::left); tick(0.0f);
        std::cout << "Timed control " << name << ": state=" << static_cast<int>(game->level().state()) <<
            " steps=" << tape(*game).steps() << " seen=" << seen.size() << " gaps=" << gaps(*game) <<
            " paused=" << paused << '\n';
        require(paused && game->level().state() == LevelState::zoom_out && tape(*game).playable() &&
            tape(*game).steps() == static_cast<int>(seen.size()) && gaps(*game) >= 2,
            "Timed GameLevel lost its shipping tape or pause gaps");
        const RoundTape recorded = tape(*game);
        // Invoke the production offer gate, then choose Watch Replay through
        // the actual End page. Results animation/record persistence is separate.
        (game->*member(EndMenuTag{}))(); require(stack.depth() == 3, "Actual End menu did not open"); tick(0.0f);
        tap(Key::down); tap(Key::enter);
        require(stack.depth() == 2 && game->level().state() == LevelState::start_countdown,
            "Actual End page did not offer/launch Watch Replay on row one");
        for (int step = 0; step < recorded.steps(); ++step)
        {
            (game->*member(StepReplayTag{}))();
            require(moment(game->level()) == seen.at(static_cast<size_t>(step)),
                "Actual GameLevel replay diverged in public gameplay bits or exact paint-tile hash");
            ++replay_steps;
        }
        require(game->level().state() == LevelState::zoom_out, "Actual replay did not reach time up");
        note(name, *game, recorded.steps()); ++timed_cases;
        (game->*member(EndReplayTag{}))();
    }
    require(tape(*game).steps() > 0 && gaps(*game) > 0, "Transition control had no old tape/gaps to clear");
    game->*member(SettingsTag{}) = settings("movement_course");
    game->*member(StartTag{}) = MatchStart::menu;
    (game->*member(BuildTag{}))(); empty(*game); live(*game);
    for (int step = 0; step < 600; ++step) { tick(); ++practice_steps; }
    pause(); resume(*game); empty(*game); note("timed-to-practice-clears-old-tape", *game);
    require(csv.good(), "Cannot retain actual GameLevel replay cases");
    std::ofstream summary(output / "practice-replay-summary.json");
    summary << "{\"outcome\":\"passed\",\"scope\":\"actual-GameLevel-practice-replay\",\"practice_steps\":" << practice_steps <<
        ",\"state_updates\":" << updates << ",\"timed_cases\":" << timed_cases << ",\"reproduced_steps\":" << replay_steps <<
        ",\"practice_steps_retained\":0,\"practice_gaps_retained\":0,\"GPU_draws\":0,\"GPU_reads\":0,\"physical_pad_claim\":false,\"debug_recorder\":" <<
#if COLOURWARS_DEV_TOOLS
        "true"
#else
        "false"
#endif
        << "}\n";
    require(summary.good(), "Cannot retain actual GameLevel replay summary");
    std::cout << "PASS actual GameLevel practice replay: " << practice_steps << " practice steps retain zero inputs/gaps; " <<
        timed_cases << " timed controls offer Watch Replay and reproduce " << replay_steps << " steps. No raster or physical-pad claim.\n";
}
