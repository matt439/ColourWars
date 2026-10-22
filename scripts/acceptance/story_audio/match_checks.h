#pragma once

// Included after story_audio.cpp's shared recording world and content helpers.
namespace
{
	void step_match_clock(World& world, float dt)
	{
		// These coarse ticks test the clock, not movement. Reset the players to
		// open space before every step so they cannot accumulate a fall through
		// the spawn's narrow ledge between clock boundaries.
		for (int view = 0; view < world.level->player_count(); ++view)
		{
			require(world.level->place_player(view, { 1000.0f + view * 100.0f, 1000.0f }),
				"Cannot position match-audio clock fixture");
		}
		world.level->update(world.idle, dt);
	}

	void enter_match_clock(World& world, float remaining, int players = 2)
	{
		// The tick crossing GO also advances the match clock. Binary-exact steps
		// leave a known clock value without reaching into Level's private timer.
		constexpr float go_step = 0.125f;
		world.build(OpeningMode::off, players, remaining + go_step);
		world.level->update(world.idle, world.tuning.match().countdown - go_step);
		require(world.level->state() == LevelState::start_countdown,
			"Match clock fixture crossed GO before its final countdown step");
		world.level->update(world.idle, 0.0f);
		world.level->update(world.idle, -1.0f);
		require(world.plays(world.tuning.match().last_ten_sound.name) == 0,
			"Countdown or nonpositive updates emitted a last-ten cue");
		step_match_clock(world, go_step);
		require(world.level->state() == LevelState::active, "Match clock fixture did not cross GO");
	}

	void check_last_ten_boundaries(World& world)
	{
		const auto& sound = world.tuning.match().last_ten_sound;
		world.set_sfx_volume(40);
		enter_match_clock(world, 12.25f, 4);
		for (int tick = 0; tick < 8; ++tick) { step_match_clock(world, 0.25f); }
		require(world.plays(sound.name) == 0, "Last-ten cue sounded while the displayed clock exceeded ten");
		step_match_clock(world, 0.25f);
		require(world.plays(sound.name) == 1, "Entering displayed second ten did not sound exactly once");
		world.volume_is(sound.name, sound.volume * 0.4f);
		world.level->suspend();
		require(world.state(sound.name) == SoundState::paused, "Suspension did not pause last-ten audio");
		world.set_sfx_volume(0);
		world.level->resume();
		require(world.state(sound.name) == SoundState::playing && world.plays(sound.name) == 1 &&
			close_to(world.last_volume(sound.name), 0.0f),
			"Last-ten resume restarted its voice or ignored the edited SFX mute");
		for (int second = 9; second >= 1; --second)
		{
			const int before = world.plays(sound.name);
			for (int tick = 0; tick < 3; ++tick) { step_match_clock(world, 0.25f); }
			world.level->update(world.idle, 0.0f);
			world.level->update(world.idle, -1.0f);
			require(world.plays(sound.name) == before, "Last-ten cue repeated within a displayed second");
			step_match_clock(world, 0.25f);
			require(world.plays(sound.name) == 11 - second && close_to(world.last_volume(sound.name), 0.0f),
				"Last-ten audio missed a displayed second, repeated per player, or ignored current mute");
		}
		for (int tick = 0; tick < 4; ++tick) { step_match_clock(world, 0.25f); }
		require(world.level->state() == LevelState::zoom_out && world.plays(sound.name) == 10 &&
			world.state(sound.name) == SoundState::stopped, "Time up repeated or failed to stop last-ten audio");
		world.level->resume();
		world.level->update(world.idle, 0.25f);
		require(world.plays(sound.name) == 10 && world.state(sound.name) == SoundState::stopped,
			"Post-match resume or updates resurrected last-ten audio");
	}

	void check_last_ten_short_and_long_matches(World& world)
	{
		const auto& sound = world.tuning.match().last_ten_sound;
		for (int players : { 1, 2, 4 })
		{
			world.set_sfx_volume(players == 1 ? 0 : 35);
			enter_match_clock(world, 9.75f, players);
			require(world.plays(sound.name) == 1, "A short match missed its first displayed last-ten second");
			world.volume_is(sound.name, sound.volume * world.preferences.sfx_gain());
			world.level->update(world.idle, 0.0f);
			world.level->update(world.idle, -1.0f);
			step_match_clock(world, 0.25f);
			require(world.plays(sound.name) == 1, "A short match repeated its initial last-ten cue");
			world.level.reset();
			require(world.state(sound.name) == SoundState::stopped, "Level destruction left last-ten audio alive");
		}
		enter_match_clock(world, 10.25f, 4);
		step_match_clock(world, 1.5f);
		require(world.plays(sound.name) == 1, "Long match tick queued missed last-ten seconds");
		world.level->update(world.idle, 0.0f);
		world.level->update(world.idle, -1.0f);
		step_match_clock(world, 0.25f);
		require(world.plays(sound.name) == 1, "Long match tick left a catch-up last-ten backlog");
		step_match_clock(world, 0.5f);
		require(world.plays(sound.name) == 2, "Last-ten audio failed to resume at the next displayed second");
		enter_match_clock(world, 1.25f);
		step_match_clock(world, 1.5f);
		require(world.level->state() == LevelState::zoom_out && world.plays(sound.name) == 1 &&
			world.state(sound.name) == SoundState::stopped,
			"A match tick crossing time up queued an unheard last-ten cue");
	}

	void room_tone_volume_is(const World& world, float gain)
	{
		const auto& sound = world.tuning.match().room_tone_sound;
		const auto voice = world.voices.at(sound.name);
		const auto& calls = recorded_sounds(world.device);
		bool found = false;
		for (size_t index = world.begin; index < calls.size(); ++index)
		{
			const auto& call = calls[index];
			if (call.call == SoundCall::play_voice && call.voice == voice)
			{
				require(call.loop, "Room tone was played as a one-shot");
				found = true;
			}
		}
		require(found && close_to(world.last_volume(sound.name), sound.volume * gain),
			"Room tone did not use its authored volume and current SFX gain");
	}

	void check_room_tone_boundaries(World& world)
	{
		const auto& match = world.tuning.match();
		const auto& sound = match.room_tone_sound;
		const auto& opening = match.opening;
		const float length = opening.settle + opening.preview * 2.0f + opening.undo + opening.leave;
		for (const auto mode : { OpeningMode::full, OpeningMode::quick })
		{
			for (int players : { 2, 4 })
			{
				world.set_sfx_volume(40);
				world.build(mode, players);
				world.level->update(world.idle, 0.0f);
				world.level->update(world.idle, -1.0f);
				require(world.plays(sound.name) == 0, "Construction or a frozen opening started room tone");
				world.level->update(world.idle, 0.001f);
				world.level->update(world.idle, 0.001f);
				require(world.plays(sound.name) == 1 && world.state(sound.name) == SoundState::playing,
					"Opening did not start one room-tone loop on its first positive update");
				room_tone_volume_is(world, 0.4f);
				world.level->suspend();
				require(world.state(sound.name) == SoundState::paused, "Suspension did not pause room tone");
				world.set_sfx_volume(25);
				world.level->resume();
				require(world.state(sound.name) == SoundState::playing && world.plays(sound.name) == 1,
					"Room-tone resume restarted the loop");
				room_tone_volume_is(world, 0.25f);
				world.level->update(world.idle,
					length - (mode == OpeningMode::quick ? opening.settle : 0.0f));
				require(world.level->state() == LevelState::start_countdown && world.plays(sound.name) == 1 &&
					world.state(sound.name) == SoundState::playing, "Room tone did not survive the countdown entrance");
				world.level->update(world.idle, match.countdown - opening.push - 0.001f);
				require(world.state(sound.name) == SoundState::playing, "Room tone stopped before the glass push");
				world.level->update(world.idle, 0.0f);
				world.level->update(world.idle, -1.0f);
				world.level->update(world.idle, 0.002f);
				require(world.state(sound.name) == SoundState::stopped && world.plays(sound.name) == 1,
					"Glass push failed to stop room tone or restarted it");
				world.level->resume();
				require(world.state(sound.name) == SoundState::stopped, "Resume resurrected room tone inside the picture");
				world.until([&] { return world.level->state() == LevelState::active; });
				require(world.plays(sound.name) == 1, "Active play restarted room tone");
				world.until([&] { return world.level->state() == LevelState::zoom_out; });
				require(world.plays(sound.name) == 2 && world.state(sound.name) == SoundState::playing,
					"Time up did not restore room tone for the monitor reveal");
				room_tone_volume_is(world, 0.25f);
				world.level->update(world.idle, 0.0f);
				world.level->update(world.idle, -1.0f);
				world.until([&] { return world.level->state() == LevelState::overview; });
				require(world.plays(sound.name) == 2 && world.state(sound.name) == SoundState::playing,
					"Pullback or frozen reveal repeated or stopped room tone");
				world.until([&] { return world.level->state() == LevelState::finished; });
				world.level->resume();
				world.level->update(world.idle, 1.0f);
				require(world.plays(sound.name) == 2 && world.state(sound.name) == SoundState::stopped,
					"Finished reveal left room tone alive or let resume restart it");
			}
		}
	}

	void check_room_tone_skip_off_and_teardown(World& world)
	{
		const auto& match = world.tuning.match();
		const auto& sound = match.room_tone_sound;
		const auto& opening = match.opening;
		const float length = opening.settle + opening.preview * 2.0f + opening.undo + opening.leave;
		for (const auto mode : { OpeningMode::full, OpeningMode::quick })
		{
			world.build(mode);
			world.skip(length + 1.0f);
			require(world.plays(sound.name) == 0, "Immediate intro skip briefly started unheard room tone");
			for (bool countdown : { false, true })
			{
				world.build(mode);
				world.level->update(world.idle, countdown ? length : 0.001f);
				require(world.plays(sound.name) == 1, "Long opening tick missed or repeated room tone");
				world.skip(0.001f);
				world.level->resume();
				require(world.plays(sound.name) == 1 && world.state(sound.name) == SoundState::stopped,
					"Opening/countdown skip or subsequent resume left room tone alive");
			}
		}
		for (int players : { 1, 2, 4 })
		{
			world.build(OpeningMode::off, players);
			world.until([&] { return world.level->state() == LevelState::finished; });
			require(world.plays(sound.name) == 0, "Intro-off or solo practice played room tone");
		}
		world.set_sfx_volume(0);
		world.build(OpeningMode::full);
		world.level->update(world.idle, 0.001f);
		room_tone_volume_is(world, 0.0f);
		world.level.reset();
		require(world.state(sound.name) == SoundState::stopped, "Opening teardown left room tone alive");
		world.build(OpeningMode::full);
		world.skip(0.001f);
		world.until([&] { return world.level->state() == LevelState::zoom_out; });
		require(world.plays(sound.name) == 1 && world.state(sound.name) == SoundState::playing,
			"Skipped opening did not retain room tone for the later reveal");
		world.level->suspend();
		world.set_sfx_volume(60);
		world.level->resume();
		room_tone_volume_is(world, 0.6f);
		world.level.reset();
		require(world.state(sound.name) == SoundState::stopped, "Reveal teardown left room tone alive");
	}
}
