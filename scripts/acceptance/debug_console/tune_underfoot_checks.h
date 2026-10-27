// Included inside debug_console_acceptance's anonymous namespace.
void check_underfoot_tunes(DebugDirectorImpl& impl, GameTuning& tuning, const ScratchContent& scratch)
{
    const std::string root = "tuning.player.paint_underfoot.";
    const std::string source_before = read_text(scratch.source / L"tuning.json");
    const std::string staged_before = read_text(scratch.staged / L"tuning.json");
    const auto before = tuning.player().paint_underfoot;
    const PlayerTuning* const borrowed = &tuning.player();
    for (const char* field : { "enabled", "own_speed_multiplier", "own_ammo_regen_multiplier", "enemy_speed_multiplier", "enemy_ammo_regen_multiplier",
        "enemy_blocks_health_regen" })
    {
        require(!impl.tune_token(root + field).empty() && impl.execute("tune " + root + field).ok,
            "Underfoot typed getter is present in actual scratch tuning");
        require(tune_scope_of(root + field) == TuneScope::live, "Underfoot leaves retain generic player live scope");
    }
    require(impl.execute("tune " + root + "own_speed_multiplier 1.2").ok &&
        impl.execute("tune " + root + "own_ammo_regen_multiplier 1.5").ok &&
        impl.execute("tune " + root + "enemy_speed_multiplier 0.75").ok &&
        impl.execute("tune " + root + "enemy_ammo_regen_multiplier 0").ok &&
        impl.execute("tune " + root + "enemy_blocks_health_regen on").ok &&
        tuning.player().paint_underfoot.enemy_blocks_health_regen &&
        impl.execute("tune " + root + "enabled on").ok && tuning.player().paint_underfoot.enabled &&
        &tuning.player() == borrowed, "Valid live underfoot edits do not move borrowed tuning records");
    const std::string text = impl.tune_document("tuning")->current().text();
    const auto ledger = impl.tune_document("tuning")->edits().size();
    for (const std::string& command : { "enabled 2", "own_speed_multiplier 0.99", "own_speed_multiplier 3e38", "enemy_speed_multiplier 0", "enemy_ammo_regen_multiplier 1.01" })
    {
        require(!impl.execute("tune " + root + command).ok &&
            impl.tune_document("tuning")->current().text() == text &&
            impl.tune_document("tuning")->edits().size() == ledger &&
            tuning.player().paint_underfoot.own_speed_multiplier == 1.2f,
            "Rejected schema/derived-rate tune changed live values, document or ledger");
    }
    require(impl.execute("tune.reset " + root + "enabled").ok &&
        tuning.player().paint_underfoot.enabled == before.enabled &&
        tuning.player().paint_underfoot.own_speed_multiplier == 1.2f,
        "Path reset did not isolate the enabled flag from other edited multipliers");
    require(impl.execute("tune.reset tuning").ok &&
        tuning.player().paint_underfoot.enabled == before.enabled &&
        tuning.player().paint_underfoot.own_speed_multiplier == before.own_speed_multiplier &&
        tuning.player().paint_underfoot.enemy_speed_multiplier == before.enemy_speed_multiplier &&
        tuning.player().paint_underfoot.own_ammo_regen_multiplier == before.own_ammo_regen_multiplier &&
        tuning.player().paint_underfoot.enemy_ammo_regen_multiplier == before.enemy_ammo_regen_multiplier &&
        tuning.player().paint_underfoot.enemy_blocks_health_regen == before.enemy_blocks_health_regen &&
        read_text(scratch.source / L"tuning.json") == source_before &&
        read_text(scratch.staged / L"tuning.json") == staged_before,
        "Underfoot reset did not restore exact data without writing source/staged files");
    // Null-Application CPU context does not exercise production whole-stage
    // live validation; root qualifies that branch in an isolated real app.
}
