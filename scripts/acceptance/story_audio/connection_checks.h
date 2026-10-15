// Included inside the story-audio fixture's anonymous namespace.
void check_connection_audio(World& world)
{
	ConnectionAudio audio(&world.audio, &world.preferences, world.presentation);
	const auto& look = world.presentation.pause_menu();
	const SoundTuning lost{ look.pad_lost_sound.name, look.pad_lost_sound.volume };
	const SoundTuning back{ look.pad_back_sound.name, look.pad_back_sound.volume };
	ConnectionAudio::Pads eligible{ false, true, false, true };
	ConnectionAudio::Pads connected{ false, true, false, true };
	world.preferences.sfx_volume = 35;
	world.begin = recorded_sounds(world.device).size();
	audio.update(eligible, connected);
	require(world.wave_plays(lost.name) == 0 && world.wave_plays(back.name) == 0,
		"Connected roster or unclaimed pads invented a connection cue");
	connected[1] = connected[3] = false;
	audio.update(eligible, connected);
	audio.update(eligible, connected);
	require(world.wave_plays(lost.name) == 1 && world.wave_plays(back.name) == 0,
		"Simultaneous missing pads or repeated page updates duplicated the loss cue");
	world.wave_volume_is(lost, world.preferences.sfx_gain());
	connected[1] = true;
	audio.update(eligible, connected);
	audio.update(eligible, connected);
	require(world.wave_plays(back.name) == 1, "Partial reconnect was missed or repeated");
	world.wave_volume_is(back, world.preferences.sfx_gain());
	// A different participant can disconnect while another is returning.
	world.begin = recorded_sounds(world.device).size();
	connected[1] = false;
	connected[3] = true;
	world.preferences.sfx_volume = 0;
	audio.update(eligible, connected);
	require(world.wave_plays(lost.name) == 1 && world.wave_plays(back.name) == 1,
		"Opposite pad edges in one sample did not stay independent");
	world.wave_volume_is(lost, 0.0f);
	world.wave_volume_is(back, 0.0f);
	world.begin = recorded_sounds(world.device).size();
	eligible[1] = false; // Bot/replay takes the absent seat; no physical return.
	audio.update(eligible, connected);
	connected[1] = true;
	audio.update(eligible, connected);
	require(world.wave_plays(lost.name) == 0 && world.wave_plays(back.name) == 0,
		"A debug-driven or unclaimed seat invented a pad-back cue");
	eligible[1] = true;
	audio.update(eligible, connected);
	require(world.wave_plays(back.name) == 0, "A newly eligible connected seat invented a return");
	// A new match can begin with an already absent claimed pad (no engine edge).
	ConnectionAudio fresh(&world.audio, &world.preferences, world.presentation);
	connected[3] = false;
	world.preferences.sfx_volume = 100;
	fresh.update(eligible, connected);
	require(world.wave_plays(lost.name) == 1, "Already missing participant was not reported");
	world.wave_volume_is(lost, 1.0f);
	AudioResources missing_bank;
	missing_bank.add_sound_bank(world.presentation.menu().sound_bank,
		SoundBank::silent());
	ConnectionAudio silent_audio(&missing_bank, &world.preferences, world.presentation);
	silent_audio.update(eligible, connected);
	connected[3] = true;
	silent_audio.update(eligible, connected);
}
