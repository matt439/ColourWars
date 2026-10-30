// Included inside the story-audio fixture's anonymous namespace.
void check_resume_audio(World& world)
{
	const int original_volume = world.preferences.sfx_volume;
	const auto& sound = world.presentation.pause_menu().resume_sound;
	const auto voice = world.voices.at(sound.name);
	world.begin = recorded_sounds(world.device).size();
	world.set_sfx_volume(35);
	{
		ResumeAudio audio(&world.audio, world.presentation);
		audio.update(0.0f);
		audio.update(-1.0f);
		require(world.plays(sound.name) == 0, "An inactive resume count-in invented a beat");
		audio.update(3.0f);
		audio.update(2.75f);
		audio.update(2.01f);
		require(world.plays(sound.name) == 1, "A displayed resume number repeated its cue");
		world.volume_is(sound.name, sound.volume * 0.35f);
		world.set_sfx_volume(0);
		audio.update(2.0f);
		require(world.plays(sound.name) == 2 && close_to(world.last_volume(sound.name), 0.0f),
			"The next resume number missed its cue or ignored current SFX mute");
		audio.update(0.125f);
		audio.update(0.125f);
		require(world.plays(sound.name) == 3, "A long resume update queued missed numbers or repeated its beat");
		audio.update(0.0f);
		audio.update(-5.0f);
		require(world.plays(sound.name) == 3 && world.state(sound.name) == SoundState::stopped,
			"Finishing the resume count-in left its voice alive or invented a GO cue");
		world.set_sfx_volume(65);
		audio.update(3.0f);
		require(world.plays(sound.name) == 4 && close_to(world.last_volume(sound.name), sound.volume * 0.65f),
			"A new resume count-in retained its old beat or SFX gain");
		audio.stop();
		audio.stop();
		require(world.state(sound.name) == SoundState::stopped,
			"An interrupted resume count-in did not cancel its owned voice");
		audio.update(3.0f);
		require(world.plays(sound.name) == 5 && world.state(sound.name) == SoundState::playing,
			"Resuming after cancellation did not start a fresh copy of the same displayed beat");
	}
	require(world.state(sound.name) == SoundState::stopped,
		"Destroying resume audio left its count-in voice alive");
	for (size_t index = world.begin; index < recorded_sounds(world.device).size(); ++index)
	{
		const auto& call = recorded_sounds(world.device)[index];
		if (call.voice != voice) { continue; }
		require(call.call != SoundCall::resume_voice,
			"A cancelled resume count-in continued an old phrase");
		if (call.call == SoundCall::stop_voice)
		{
			require(call.immediate, "Resume count-in cancellation left a tail playing");
		}
		if (call.call == SoundCall::play_voice)
		{
			require(!call.loop, "Resume count-in looped its beat");
		}
	}
	// Jumping directly across the remaining numbers emits only the current
	// beat, and crossing zero never catches up a missed beat.
	world.begin = recorded_sounds(world.device).size();
	{
		ResumeAudio audio(&world.audio, world.presentation);
		audio.update(4.0f);
		audio.update(0.25f);
		require(world.plays(sound.name) == 2, "A skipped resume number left a cue backlog");
		audio.update(-2.0f);
		require(world.plays(sound.name) == 2 && world.state(sound.name) == SoundState::stopped,
			"Crossing the end of a resume count-in played skipped numbers");
	}
	AudioResources missing_bank;
	missing_bank.add_sound_bank(world.presentation.menu().sound_bank, SoundBank::silent());
	ResumeAudio silent_audio(&missing_bank, world.presentation);
	silent_audio.update(3.0f);
	silent_audio.update(1.0f);
	silent_audio.stop();
	silent_audio.update(0.0f);
	world.set_sfx_volume(original_volume);
}

void check_resume_weapon_audio(World& world)
{
	world.build(OpeningMode::off, 1, 60.0f);
	world.until([&] { return world.level->state() == LevelState::active; });
	auto firing = world.idle;
	firing.at(static_cast<size_t>(world.level->player(0).player_num())).primary_shoot = true;
	world.level->update(acceptance::slot_seats(*world.level, firing), 1.0f / 60.0f);
	const std::string loop = world.tuning.weapon(WeaponType::sprayer).loop_sounds.sound_name(PlayerTeam::a, 0);
	require(world.state(loop) == SoundState::playing, "Resume fixture did not start its weapon loop");
	world.level->suspend();
	require(world.state(loop) == SoundState::paused, "Count-in entry left a weapon loop playing");
	world.level->prepare_resume(world.idle);
	world.level->resume();
	require(world.state(loop) == SoundState::stopped,
		"Completing count-in resurrected a stale firing loop before a fresh shot");
	world.level->update(world.idle, 1.0f / 60.0f);
	require(world.state(loop) == SoundState::stopped, "Neutral live input resurrected stale weapon audio");
	world.level->update(acceptance::slot_seats(*world.level, firing), 1.0f / 60.0f);
	require(world.state(loop) == SoundState::playing, "Fresh fire after count-in did not start its weapon loop");
	world.level.reset();
}
