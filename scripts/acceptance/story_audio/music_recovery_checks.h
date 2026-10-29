#pragma once

// ROAD#36: Labrador's device recovery (engine 2b3b97b, past the pinned v0.2.0)
// stops persistent voices without an event. Stopping each music effect through
// its own sound bank stands in for that; the recorder has no device to lose.
// Included after music_mix_checks.h and results_music_checks.h.
namespace
{
	int looping_plays(const World& world, const std::string& name)
	{
		const auto voice = world.voices.at(name);
		int count = 0;
		const auto& calls = recorded_sounds(world.device);
		for (size_t index = world.begin; index < calls.size(); ++index)
		{
			const auto& call = calls[index];
			count += call.voice == voice && call.call == SoundCall::play_voice && call.loop ? 1 : 0;
		}
		return count;
	}

	void stop_from_outside(World& world, const std::string& bank, const std::string& name)
	{
		SoundBank* sounds = world.audio.sound_bank(bank);
		sounds->stop_effect(sounds->resolve_effect(name), true);
		require(world.state(name) == SoundState::stopped, "Outside stop fixture left the music voice alive");
	}

	void check_screen_music_recovery(World& world)
	{
		const int original_music = world.preferences.music_volume;
		world.set_music_volume(50);
		const auto& music = world.presentation.main_menu().music;
		const auto& mix = world.presentation.music_mix();
		const auto& bank = world.presentation.menu().sound_bank;
		{
			world.begin = recorded_sounds(world.device).size();
			ScreenMusic audio(bank, &world.audio, &world.preferences, music, mix, true);
			audio.update(mix.fade_in);
			require(close_to(world.last_volume(music.name), music.volume * 0.5f),
				"Recovery fixture did not reach full menu-music gain");
			stop_from_outside(world, bank, music.name);
			audio.update(0.0f);
			audio.resume();
			require(looping_plays(world, music.name) == 1,
				"Zero-time refresh or resume replayed a stopped menu loop");
			audio.update(1.0f / 60.0f);
			require(looping_plays(world, music.name) == 2 && world.state(music.name) == SoundState::playing &&
				close_to(world.last_volume(music.name), 0.0f) && !audio.finished(),
				"An outside stop of the staying menu loop was not replayed once from silence");
			audio.update(mix.fade_in * 0.5f);
			require(close_to(world.last_volume(music.name), music.volume * 0.5f * 0.5f) &&
				looping_plays(world, music.name) == 2,
				"Recovered menu loop did not fade in with music_mix.fade_in");
			audio.suspend();
			stop_from_outside(world, bank, music.name);
			audio.resume();
			require(looping_plays(world, music.name) == 2,
				"Focus return alone replayed a menu loop stopped while suspended");
			audio.update(1.0f / 60.0f);
			require(looping_plays(world, music.name) == 3 && world.state(music.name) == SoundState::playing,
				"The first positive page tick after suspension did not replay the stopped menu loop");
			audio.update(mix.fade_in);
			audio.fade_out();
			audio.update(mix.fade_out * 0.25f);
			stop_from_outside(world, bank, music.name);
			audio.update(1.0f / 60.0f);
			audio.update(mix.fade_out);
			require(looping_plays(world, music.name) == 3 && audio.finished() &&
				world.state(music.name) == SoundState::stopped,
				"A leaving menu loop stopped from outside was replayed or did not finish");
		}
		{
			const auto& coda = world.presentation.results_menu().music;
			world.begin = recorded_sounds(world.device).size();
			ResultsAudio results(&world.audio, &world.preferences, world.presentation);
			results.update(mix.fade_in);
			stop_from_outside(world, bank, coda.name);
			results.update(1.0f / 60.0f);
			results.resume();
			results.update(mix.fade_in);
			require(world.plays(coda.name) == 1 && world.state(coda.name) == SoundState::stopped,
				"A stopped one-shot results coda was replayed as if recovered");
		}
		{
			AudioResources missing;
			missing.add_sound_bank(bank, SoundBank::silent());
			const auto absent = recorded_sounds(world.device).size();
			ScreenMusic silent(bank, &missing, &world.preferences, music, mix, true);
			for (int tick = 0; tick < 3; ++tick) { silent.update(mix.fade_in); }
			require(recorded_sounds(world.device).size() == absent,
				"Silent-bank menu music tried to replay a voice it never had");
		}
		world.set_music_volume(original_music);
	}

	void check_arena_music_recovery(World& world)
	{
		const int original_music = world.preferences.music_volume;
		world.set_music_volume(40);
		const auto& mix = world.presentation.music_mix();
		const LevelLoadedInfo* stage = world.levels.get("close_quarters");
		const std::string music = stage->music_name();
		const std::string bank = "paint_exe";
		const float full = stage->music_volume() * 0.4f;

		// Countdown: the loop has just started at zero gain.
		world.build(OpeningMode::off, 2, 60.0f);
		world.level->update(world.idle, 0.001f);
		require(world.level->state() == LevelState::start_countdown && looping_plays(world, music) == 1,
			"Arena recovery fixture did not start its loop in the countdown");
		world.level->update_music(mix.fade_in * 0.5f);
		stop_from_outside(world, bank, music);
		world.level->resume_music();
		world.level->update_music(0.0f);
		require(looping_plays(world, music) == 1, "Resume or zero time replayed a stopped arena loop");
		world.level->update_music(1.0f / 60.0f);
		require(looping_plays(world, music) == 2 && world.state(music) == SoundState::playing &&
			close_to(world.last_volume(music), 0.0f),
			"A countdown arena loop stopped from outside was not replayed once from silence");
		world.level->update_music(mix.fade_in * 0.5f);
		require(close_to(world.last_volume(music), full * 0.5f),
			"Recovered arena loop did not fade in with music_mix.fade_in");

		// Live play.
		world.until([&] { return world.level->state() == LevelState::active; });
		for (int tick = 0; tick < 120; ++tick) { world.level->update(world.idle, weapon_step); }
		world.level->update_music(mix.fade_in);
		stop_from_outside(world, bank, music);
		world.level->update_music(1.0f / 60.0f);
		require(world.level->state() == LevelState::active && looping_plays(world, music) == 3 &&
			world.state(music) == SoundState::playing,
			"A live-play arena loop stopped from outside was not replayed once");

		// Under a pause page: ducked, then stopped; the duck target survives.
		world.level->update_music(mix.fade_in);
		world.level->suspend();
		world.level->set_music_ducked(true);
		world.level->resume_music();
		world.level->update_music(mix.duck_time);
		require(close_to(world.last_volume(music), full * mix.pause_gain),
			"Pause-page fixture did not duck the arena loop");
		stop_from_outside(world, bank, music);
		world.level->set_music_ducked(true);
		world.level->resume_music();
		require(looping_plays(world, music) == 3, "Pause-page resume replayed a stopped arena loop");
		world.level->update_music(1.0f / 60.0f);
		require(looping_plays(world, music) == 4 && world.state(music) == SoundState::playing,
			"An arena loop stopped under a pause page was not replayed once");
		world.level->update_music(mix.fade_in);
		require(close_to(world.last_volume(music), full * mix.pause_gain),
			"Recovered arena loop under a pause page lost its duck");
		world.level->resume();
		world.level->update_music(mix.restore_time);
		require(close_to(world.last_volume(music), full) && looping_plays(world, music) == 4,
			"Recovered arena loop did not restore after the pause page");

		// Ending: the time-up fade owns the stop.
		world.level.reset();
		world.build(OpeningMode::off, 2, 0.25f);
		world.level->update(world.idle, 0.001f);
		world.level->update_music(mix.fade_in);
		world.until([&] { return world.level->state() == LevelState::zoom_out; });
		world.level->update_music(mix.fade_out * 0.25f);
		stop_from_outside(world, bank, music);
		world.level->resume_music();
		world.level->update_music(1.0f / 60.0f);
		world.level->update_music(mix.fade_out);
		require(looping_plays(world, music) == 1 && world.state(music) == SoundState::stopped,
			"An ending arena loop stopped from outside was replayed");
		world.level.reset();

		// Movement Course has no music; the silent bank has no voice.
		world.build(OpeningMode::off, 4, 0.25f, "movement_course");
		world.level->update(world.idle, 0.001f);
		const auto quiet = recorded_sounds(world.device).size();
		for (int tick = 0; tick < 3; ++tick) { world.level->update_music(mix.fade_in); }
		require(recorded_sounds(world.device).size() == quiet,
			"Silent Movement Course acquired music calls through recovery");
		world.level.reset();
		AudioResources missing;
		missing.add_sound_bank(bank, SoundBank::silent());
		LevelBuilder builder(&world.viewports, &world.render, &missing, &world.preferences, &world.levels,
			&world.tuning, &world.presentation, &world.resolution, nullptr, &world.partitioner, &world.mouse);
		const auto silent_calls = recorded_sounds(world.device).size();
		{
			auto silent = builder.build_level(music_mix_settings(), TeamColourPair::blue_orange, OpeningMode::off);
			silent->update(world.idle, 0.001f);
			for (int tick = 0; tick < 3; ++tick) { silent->update_music(mix.fade_in); }
		}
		require(recorded_sounds(world.device).size() == silent_calls,
			"Silent-bank arena music tried to replay a voice it never had");
		world.set_music_volume(original_music);
	}
}
