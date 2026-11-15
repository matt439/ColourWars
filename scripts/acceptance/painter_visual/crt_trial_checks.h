// Focused real-Level capture; process-separated flag variants share fixed inputs.
void crt_trial_capture(Application& app, LevelBuilder& builder, PlaySettings& preferences,
    const std::filesystem::path& output)
{
    std::ofstream records(output / "crt-trial.csv");
    records << "capture,state,reduce_effects\n";
    for (const int count : { 2, 4 })
    {
        auto level = monitor_story(app, builder, count);
        resize(app, *level, {1280, 720});
        auto inputs = neutral_inputs();
        const auto snap = [&](const char* beat)
        {
            for (const bool reduced : {false, true})
            {
                preferences.reduce_effects = reduced;
                const std::string stem = "crt-" + std::to_string(count) + "p-" + beat +
                    (reduced ? "-reduced" : "-normal");
                const auto before = observation(*level);
#if COLOURWARS_DEV_TOOLS
                const MatchFingerprint fingerprint = MatchFingerprint::of(*level, 0);
#endif
                const Frame first = render(*level, *app.renderer());
                require(first.pixels == render(*level, *app.renderer()).pixels,
                    "Frozen CRT capture is nondeterministic");
                require(before == observation(*level), "CRT capture changed simulation state");
#if COLOURWARS_DEV_TOOLS
                require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(*level, 0)).empty(),
                    "CRT capture changed fingerprint");
#endif
                save(first, output / (stem + ".ppm"));
                records << stem << ',' << static_cast<int>(level->state()) << ',' << reduced << '\n';
            }
            preferences.reduce_effects = false;
        };
        snap("opening");
        for (int tick=0; tick<2400 && level->state()!=LevelState::active; ++tick)
            level->update(acceptance::slot_seats(*level, inputs), dt);
        require(level->state()==LevelState::active, "CRT fixture missed active gameplay");
        snap("active");
        require(level->place_player(0, {1200, 1275}), "CRT fixture could not place painter");
        inputs[3].primary_shoot = true;
        inputs[3].x_movement = 1;
        for (int tick=0; tick<2400 && level->state()!=LevelState::overview; ++tick)
            level->update(acceptance::slot_seats(*level, inputs), dt);
        require(level->state()==LevelState::overview, "CRT fixture missed full reveal");
        // Settled monitor overview, after pullback; same painted-result story in both processes.
        snap("reveal");
        for (int tick=0; tick<280; ++tick)
            level->update(acceptance::slot_seats(*level, inputs), dt);
        require(level->state()==LevelState::overview, "CRT fixture left reveal before result text");
        snap("result");
    }
    require(records.good(), "CRT capture records failed");
}
