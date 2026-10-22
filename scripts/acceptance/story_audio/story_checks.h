// Included inside the story-audio fixture's anonymous namespace.
// Story, opening, match-content and reveal audio checks. PaintForReveal is
// also used by paint_checks.h.
	void check_story(World& world)
	{
		const MatchTuning& match = world.tuning.match();
		const OpeningTuning& opening = match.opening;
		const RevealTuning& reveal = match.reveal;
		const std::string& music = world.levels.get("close_quarters")->music_name();
		const float leave = opening.settle + opening.preview * 2.0f + opening.undo;
		const float chair = leave + opening.leave * 0.35f;
		const float length = leave + opening.leave;
		world.set_sfx_volume(40);
		world.build(OpeningMode::full);
		world.level->update(world.idle, leave - 0.001f);
		require(world.plays(match.sigh_sound.name) == 0 && world.plays(match.chair_sound.name) == 0 &&
			world.plays(music) == 0, "Room cues or picture music started while the person was still choosing");
		world.level->update(world.idle, 0.002f);
		require(world.plays(match.sigh_sound.name) == 1 && world.plays(match.chair_sound.name) == 0,
			"The undo/leave edge must sigh once, before the chair moves");
		world.level->update(world.idle, 0.0f);
		require(world.plays(match.sigh_sound.name) == 1, "A stationary opening repeated its sigh");
		world.level->update(world.idle, chair - leave);
		require(world.plays(match.chair_sound.name) == 1, "Standing did not creak the chair once");
		world.level->suspend();
		require(world.state(match.sigh_sound.name) == SoundState::paused &&
			world.state(match.chair_sound.name) == SoundState::paused, "Suspension did not pause room voices");
		world.set_sfx_volume(25);
		world.level->resume();
		require(world.state(match.chair_sound.name) == SoundState::playing &&
			world.plays(match.chair_sound.name) == 1, "Resume restarted a room cue instead of continuing it");
		require(close_to(world.last_volume(match.sigh_sound.name), match.sigh_sound.volume * 0.25f) &&
			close_to(world.last_volume(match.chair_sound.name), match.chair_sound.volume * 0.25f),
			"Resume did not apply edited SFX gain to the paused room cues");
		world.set_sfx_volume(40);
		world.level->resume();
		world.volume_is(match.sigh_sound.name, match.sigh_sound.volume * 0.4f);
		world.volume_is(match.chair_sound.name, match.chair_sound.volume * 0.4f);
		world.level->update(world.idle, length - chair);
		require(world.level->state() == LevelState::start_countdown && world.plays(music) == 0,
			"Picture music must wait until the person has left");
		world.level->update(world.idle, 0.0f);
		world.level->update(world.idle, 0.0f);
		require(world.plays(match.countdown_sound.name) == 0 && world.plays(music) == 0,
			"Frozen countdown started its beat or picture music");
		world.level->update(world.idle, 0.001f);
		require(world.plays(match.countdown_sound.name) == 1 && world.plays(music) == 1,
			"Zero-dt countdown updates repeated the initial beat or music start");
		require(close_to(world.last_volume(music), 0.0f), "Arena music did not start at the silent fade boundary");
		world.level->update_music(world.presentation.music_mix().fade_in);
		require(close_to(world.last_volume(music), world.levels.get("close_quarters")->music_volume() *
			world.preferences.music_gain()), "Raw audio time did not complete the opening music fade");
		world.until([&] { return world.level->state() == LevelState::active; });
		require(world.plays(match.countdown_sound.name) == static_cast<int>(std::ceil(match.countdown)) &&
			world.plays(match.go_sound.name) == 1, "Countdown must play each number once and GO once");
		world.volume_is(match.countdown_sound.name, match.countdown_sound.volume * 0.4f);
		world.volume_is(match.go_sound.name, match.go_sound.volume * 0.4f);
		world.until([&] { return world.level->state() == LevelState::zoom_out; });
		require(world.plays(match.time_up_sound.name) == 1 && world.state(music) == SoundState::playing,
			"Time up must sound once and let the arena loop fade under its ending sting");
		world.level->update_music(world.presentation.music_mix().fade_out);
		require(world.state(music) == SoundState::stopped,
			"Raw audio time did not finish the time-up music fade");
		world.until([&] { return world.level->state() == LevelState::overview; });
		world.level->update(world.idle, reveal.walk_in * 0.6f - 0.001f);
		require(world.plays(match.chair_sound.name) == 1, "Reveal chair sounded before sitting");
		world.level->update(world.idle, 0.002f);
		require(world.plays(match.chair_sound.name) == 2, "Sitting must creak the chair once");
		const float reveal_leave = reveal.walk_in + reveal.hover * 2.0f + reveal.decide + reveal.show;
		world.level->update(world.idle, reveal_leave - reveal.walk_in * 0.6f);
		require(world.plays(match.chair_sound.name) == 3 && world.plays(match.sigh_sound.name) == 1,
			"Reveal leaving must creak once without another sigh");
		world.until([&] { return world.level->state() == LevelState::finished; });
		world.level->update(world.idle, 1.0f);
		require(world.plays(match.chair_sound.name) == 3 && world.plays(match.time_up_sound.name) == 1 &&
			world.state(music) == SoundState::stopped, "Finished story repeated a cue or resumed picture music");
		world.level.reset();
		require(world.state(match.sigh_sound.name) == SoundState::stopped &&
			world.state(match.chair_sound.name) == SoundState::stopped &&
			world.state(match.go_sound.name) == SoundState::stopped &&
			world.state(match.time_up_sound.name) == SoundState::stopped, "Level destruction left an owned cue alive");
	}

	void check_skip_quick_and_solo(World& world)
	{
		const MatchTuning& match = world.tuning.match();
		const auto& timing = match.opening;
		const float leave = timing.settle + timing.preview * 2.0f + timing.undo;
		const float length = leave + timing.leave;
		world.build(OpeningMode::full);
		world.skip(length + 1.0f);
		require(world.level->state() == LevelState::start_countdown &&
			world.plays(match.sigh_sound.name) == 0 && world.plays(match.chair_sound.name) == 0,
			"Skipping over the whole opening burst its unheard room cues");
		world.build(OpeningMode::full);
		world.level->update(world.idle, leave + 0.001f);
		world.skip(length);
		require(world.plays(match.sigh_sound.name) == 1 && world.plays(match.chair_sound.name) == 0 &&
			world.state(match.sigh_sound.name) == SoundState::stopped,
			"Skipping an in-progress sigh must stop it without starting the chair");
		world.build(OpeningMode::quick);
		world.level->update(world.idle, length - timing.settle + 0.001f);
		world.level->update(world.idle, 0.0f);
		require(world.plays(match.sigh_sound.name) == 1 && world.plays(match.chair_sound.name) == 1,
			"A quick opening or long tick missed or repeated a crossed room cue");
		world.set_sfx_volume(0);
		world.build(OpeningMode::off, 1);
		world.until([&] { return world.level->state() == LevelState::finished; });
		require(!world.level->revealed() && world.plays(match.sigh_sound.name) == 0 &&
			world.plays(match.chair_sound.name) == 0, "Solo practice played the person's room cues");
		world.volume_is(match.countdown_sound.name, 0.0f);
		world.volume_is(match.go_sound.name, 0.0f);
		world.volume_is(match.time_up_sound.name, 0.0f);
	}

	void check_match_content(World& world, const ContentVariant& content)
	{
		for (const char* key : { "glass_in", "close", "celebrate", "slump", "colour_in", "skip", "undo",
			"last_ten", "room_tone" })
		{
			bool rejected = false;
			try
			{
				const GameTuning bad = content.tuning([&](rapidjson::Document& json)
				{
					json["match"]["sounds"][key]["name"].SetString("missing.match.cue", json.GetAllocator());
				});
				bad.validate_match_sounds(world.render, world.audio, "paint_exe");
			}
			catch (const std::exception& error)
			{
				rejected = std::string(error.what()).find(std::string("match.sounds.") + key + ".name") !=
					std::string::npos;
			}
			require(rejected, "Missing match cue was accepted or lost its authored key");
			for (float volume : { -0.01f, 1.01f })
			{
				rejected = false;
				try
				{
					content.tuning([&](rapidjson::Document& json)
					{
						json["match"]["sounds"][key]["volume"].SetFloat(volume);
					});
				}
				catch (const std::exception& error)
				{
					const std::string message = error.what();
					rejected = message.find(std::string("match.sounds.") + key) != std::string::npos &&
						message.find("'volume'") != std::string::npos;
				}
				require(rejected, "Out-of-range match cue volume was accepted or lost its authored key");
			}
		}
	}

	void check_opening_details(World& world)
	{
		const auto& match = world.tuning.match();
		const auto& timing = match.opening;
		const float first = timing.settle + timing.preview * 0.4f;
		const float second = timing.settle + timing.preview * 1.4f;
		const float undo = timing.settle + timing.preview * 2.0f;
		const float length = undo + timing.undo + timing.leave;
		world.set_sfx_volume(40);
		world.build(OpeningMode::full);
		world.level->update(world.idle, first - 0.001f);
		require(world.wave_plays(match.hover_sound.name) == 0 &&
			world.plays(match.undo_sound.name) == 0 && world.plays(match.colour_in_sound.name) == 0,
			"An opening detail sounded before its visual edge");
		world.level->update(world.idle, 0.0f);
		world.level->update(world.idle, -1.0f);
		world.level->update(world.idle, 0.002f);
		require(world.wave_plays(match.hover_sound.name) == 1,
			"Nonpositive opening updates rewound or consumed the first swatch arrival");
		world.level->update(world.idle, second - first);
		require(world.wave_plays(match.hover_sound.name) == 2 && world.plays(match.undo_sound.name) == 0,
			"Second opening swatch arrival missed, repeated or undid early");
		world.level->update(world.idle, undo - second - 0.002f);
		require(world.plays(match.undo_sound.name) == 0, "Undo sounded before its return-to-grey edge");
		world.level->update(world.idle, 0.002f);
		require(world.plays(match.undo_sound.name) == 1 && world.plays(match.colour_in_sound.name) == 0,
			"Opening undo missed its edge or coloured the figures early");
		world.volume_is(match.undo_sound.name, match.undo_sound.volume * 0.4f);
		world.level->suspend();
		require(world.state(match.undo_sound.name) == SoundState::paused, "Suspension did not pause opening undo");
		world.set_sfx_volume(25);
		world.level->resume();
		require(world.state(match.undo_sound.name) == SoundState::playing && world.plays(match.undo_sound.name) == 1 &&
			close_to(world.last_volume(match.undo_sound.name), match.undo_sound.volume * 0.25f),
			"Opening undo resume restarted its voice or ignored edited SFX gain");
		world.level->update(world.idle, length - undo);
		require(world.level->state() == LevelState::start_countdown && world.plays(match.colour_in_sound.name) == 1,
			"Figures did not receive colour once at the countdown entrance");
		world.volume_is(match.colour_in_sound.name, match.colour_in_sound.volume * 0.25f);
		world.wave_volume_is(match.hover_sound, 0.4f);
		world.level->update(world.idle, 0.0f);
		world.level->update(world.idle, -1.0f);
		require(world.plays(match.glass_in_sound.name) == 0 && world.plays(match.colour_in_sound.name) == 1,
			"Frozen countdown invented glass motion or repeated colour arrival");
		world.level->update(world.idle, match.countdown - timing.push - 0.001f);
		require(world.plays(match.glass_in_sound.name) == 0, "Glass entry sounded before the push began");
		world.level->update(world.idle, 0.002f);
		require(world.plays(match.glass_in_sound.name) == 1, "Countdown missed its glass push edge");
		world.volume_is(match.glass_in_sound.name, match.glass_in_sound.volume * 0.25f);
		world.level->suspend();
		require(world.state(match.colour_in_sound.name) == SoundState::paused &&
			world.state(match.glass_in_sound.name) == SoundState::paused, "Suspension did not pause owned opening cues");
		world.set_sfx_volume(0);
		world.level->resume();
		require(world.plays(match.glass_in_sound.name) == 1 &&
			close_to(world.last_volume(match.glass_in_sound.name), 0.0f) &&
			close_to(world.last_volume(match.colour_in_sound.name), 0.0f),
			"Opening cue resume retriggered glass or ignored mute");
		world.skip(0.001f);
		require(world.state(match.glass_in_sound.name) == SoundState::stopped &&
			world.state(match.colour_in_sound.name) == SoundState::stopped &&
			world.state(match.undo_sound.name) == SoundState::stopped && world.plays(match.skip_sound.name) == 1,
			"Skipping the glass push failed to stop the story voices or emit one skip cue");
		world.volume_is(match.skip_sound.name, 0.0f);
		world.skip(0.001f);
		require(world.plays(match.skip_sound.name) == 1 && world.plays(match.glass_in_sound.name) == 1,
			"Split countdown repeated a skip or glass entry");
		world.level->suspend();
		require(world.state(match.skip_sound.name) == SoundState::paused, "Suspension did not pause skip audio");
		world.set_sfx_volume(40);
		world.level->resume();
		require(world.plays(match.skip_sound.name) == 1 &&
			close_to(world.last_volume(match.skip_sound.name), match.skip_sound.volume * 0.4f),
			"Skip audio resume restarted or ignored the current gain");
		world.level.reset();
		for (const auto* sound : { &match.undo_sound, &match.colour_in_sound, &match.glass_in_sound, &match.skip_sound })
		{
			require(world.state(sound->name) == SoundState::stopped, "Teardown left an opening detail alive");
		}
	}

	void check_opening_modes_and_long_ticks(World& world)
	{
		const auto& match = world.tuning.match();
		const auto& timing = match.opening;
		const float length = timing.settle + timing.preview * 2.0f + timing.undo + timing.leave;
		world.set_sfx_volume(0);
		for (const auto mode : { OpeningMode::full, OpeningMode::quick })
		{
			world.build(mode);
			world.level->update(world.idle, length + 1.0f);
			require(world.wave_plays(match.hover_sound.name) == 2 && world.plays(match.undo_sound.name) == 1 &&
				world.plays(match.colour_in_sound.name) == 1 && world.plays(match.glass_in_sound.name) == 0,
				"A long full/quick opening update lost, repeated or advanced beyond its crossed edges");
			world.wave_volume_is(match.hover_sound, 0.0f);
			world.volume_is(match.undo_sound.name, 0.0f);
			world.volume_is(match.colour_in_sound.name, 0.0f);
			// Level's established countdown path gives the GO tick to physics
			// at its full delta. Keep this audio-only long step inside the arena.
			for (int view = 0; view < world.level->player_count(); ++view)
			{
				require(world.level->place_player(view, { 1000.0f + view * 100.0f, 4000.0f }),
					"Cannot position long-countdown audio fixture");
			}
			world.level->update(world.idle, match.countdown - timing.push - 0.001f);
			world.level->update(world.idle, timing.push + 0.002f);
			require(world.plays(match.glass_in_sound.name) == 1 && world.plays(match.go_sound.name) == 1,
				"Long countdown crossing GO missed or repeated the glass entry");
			world.volume_is(match.glass_in_sound.name, 0.0f);
		}
		world.build(OpeningMode::full);
		world.skip(0.0f);
		world.skip(-1.0f);
		require(world.level->state() == LevelState::opening && world.plays(match.skip_sound.name) == 0,
			"Nonpositive updates consumed intro skip input");
		world.skip(length + 1.0f);
		require(world.plays(match.skip_sound.name) == 1 && world.wave_plays(match.hover_sound.name) == 0 &&
			world.plays(match.undo_sound.name) == 0 && world.plays(match.colour_in_sound.name) == 0 &&
			world.plays(match.glass_in_sound.name) == 0,
			"Skipping the whole opening played unheard story edges");
		for (int players : { 1, 2, 4 })
		{
			world.build(OpeningMode::off, players);
			world.until([&] { return world.level->state() == LevelState::finished; });
			require(!world.level->revealed() && world.wave_plays(match.hover_sound.name) == 0 &&
				world.wave_plays(match.click_sound.name) == 0 && world.wave_plays(match.close_sound.name) == 0,
				"An opening-off/solo match played the monitor's reveal cues");
			for (const auto* sound : { &match.undo_sound, &match.colour_in_sound, &match.glass_in_sound,
				&match.skip_sound, &match.celebrate_sound, &match.slump_sound })
			{
				require(world.plays(sound->name) == 0, "An opening-off/solo match played a story-only voice");
			}
		}
	}

	// Shoot at a real exposed scoring face through Level's normal input and
	// projectile phases. Four figures then prove reactions belong to teams,
	// rather than retriggering once per figure or player view.
	class PaintForReveal final : public SeatInputFilter
	{
	public:
		explicit PaintForReveal(size_t view) : view_(view) {}
		void filter_seats(std::vector<PlayerInputData>& seats, const Level&) override
		{
			auto& input = seats.at(this->view_);
			input.primary_shoot = true;
			input.shoot_angle = 1.57079632679f;
			input.shoot_direction = { 0.0f, 1.0f };
			input.shoot_direction_requested = true;
		}
	private:
		size_t view_;
	};

	void prepare_reveal(World& world, PlayerTeam winner)
	{
		const auto& timing = world.tuning.match().opening;
		world.build(OpeningMode::full, 4, 1.0f);
		world.level->update(world.idle, timing.settle + timing.preview * 2.0f +
			timing.undo + timing.leave + 0.001f);
		world.until([&] { return world.level->state() == LevelState::active; });
		if (winner != PlayerTeam::none)
		{
			const int view = winner == PlayerTeam::a ? 0 : 2;
			require(world.level->place_player(view, { 2000.0f, 4790.0f }), "Cannot place reveal painting fixture");
			PaintForReveal paint(static_cast<size_t>(view));
			for (int tick = 0; tick < 100 && world.level->state() == LevelState::active; ++tick)
			{
				world.level->update(world.idle, weapon_step, &paint);
			}
		}
		world.until([&] { return world.level->state() == LevelState::overview; });
		require(world.level->level_end_info().winning_team() == winner,
			"Real paint fixture did not produce the intended reveal winner/draw");
		world.begin = recorded_sounds(world.device).size();
	}

	void check_reveal_details(World& world)
	{
		const auto& match = world.tuning.match();
		const auto& timing = match.reveal;
		const float first = timing.walk_in + timing.hover * 0.4f;
		const float second = timing.walk_in + timing.hover * 1.4f;
		const float click = timing.walk_in + timing.hover * 2.0f + timing.decide;
		const float alone = click + timing.show + timing.leave;
		for (const auto winner : { PlayerTeam::none, PlayerTeam::a, PlayerTeam::b })
		{
			world.set_sfx_volume(40);
			prepare_reveal(world, winner);
			world.level->update(world.idle, first - 0.001f);
			require(world.wave_plays(match.hover_sound.name) == 0 && world.wave_plays(match.click_sound.name) == 0 &&
				world.wave_plays(match.close_sound.name) == 0, "Reveal cue played before its visual edge");
			world.level->update(world.idle, 0.0f);
			world.level->update(world.idle, -1.0f);
			world.level->update(world.idle, 0.002f);
			require(world.wave_plays(match.hover_sound.name) == (winner == PlayerTeam::none ? 0 : 1),
				"Reveal swatch cue ignored winner/draw or nonpositive updates");
			world.level->update(world.idle, second - first);
			require(world.wave_plays(match.hover_sound.name) == (winner == PlayerTeam::none ? 0 : 2),
				"Second reveal swatch arrival missed, repeated or played during a draw");
			world.set_sfx_volume(0);
			world.level->update(world.idle, click - second - 0.002f);
			require(world.wave_plays(match.click_sound.name) == 0 && world.wave_plays(match.close_sound.name) == 0,
				"Reveal click/close sounded before the cursor committed");
			world.level->update(world.idle, 0.002f);
			require(world.wave_plays(match.click_sound.name) == (winner == PlayerTeam::none ? 0 : 1) &&
				world.wave_plays(match.close_sound.name) == (winner == PlayerTeam::none ? 1 : 0),
				"Reveal failed to distinguish saving a winner from closing a draw");
			world.wave_volume_is(winner == PlayerTeam::none ? match.close_sound : match.click_sound, 0.0f);
			if (winner != PlayerTeam::none) { world.wave_volume_is(match.hover_sound, 0.4f); }
			world.level->update(world.idle, alone - click - 0.002f);
			require(world.plays(match.celebrate_sound.name) == 0 && world.plays(match.slump_sound.name) == 0,
				"Figures reacted while the person was still in the room");
			world.set_sfx_volume(25);
			world.level->update(world.idle, 0.002f);
			const int reactions = winner == PlayerTeam::none ? 0 : 1;
			require(world.plays(match.celebrate_sound.name) == reactions && world.plays(match.slump_sound.name) == reactions,
				"Winner/loser reactions missed, repeated per figure or played for a draw");
			if (reactions > 0)
			{
				world.volume_is(match.celebrate_sound.name, match.celebrate_sound.volume * 0.25f);
				world.volume_is(match.slump_sound.name, match.slump_sound.volume * 0.25f);
				world.level->suspend();
				require(world.state(match.celebrate_sound.name) == SoundState::paused &&
					world.state(match.slump_sound.name) == SoundState::paused, "Suspension did not pause reaction voices");
				world.set_sfx_volume(0);
				world.level->resume();
				require(world.plays(match.celebrate_sound.name) == 1 && world.plays(match.slump_sound.name) == 1 &&
					close_to(world.last_volume(match.celebrate_sound.name), 0.0f) &&
					close_to(world.last_volume(match.slump_sound.name), 0.0f),
					"Reaction resume restarted its voices or ignored current mute");
			}
			world.level->update(world.idle, 0.0f);
			world.level->update(world.idle, -1.0f);
			world.until([&] { return world.level->state() == LevelState::finished; });
			world.level->update(world.idle, timing.react + 1.0f);
			require(world.plays(match.celebrate_sound.name) == reactions && world.plays(match.slump_sound.name) == reactions &&
				world.wave_plays(match.hover_sound.name) == reactions * 2,
				"Finished reveal or nonpositive ticks repeated a detail");
			world.level.reset();
			require(world.state(match.celebrate_sound.name) == SoundState::stopped &&
				world.state(match.slump_sound.name) == SoundState::stopped, "Teardown left a reaction voice alive");
		}
	}

	void check_long_reveal(World& world)
	{
		const auto& match = world.tuning.match();
		const auto& timing = match.reveal;
		for (const auto winner : { PlayerTeam::none, PlayerTeam::a })
		{
			prepare_reveal(world, winner);
			world.set_sfx_volume(40);
			world.level->update(world.idle, timing.walk_in + timing.hover * 2.0f + timing.decide +
				timing.show + timing.leave + timing.react + 0.001f);
			require(world.level->state() == LevelState::finished, "Long reveal did not finish");
			std::vector<std::string> events;
			const auto& calls = recorded_sounds(world.device);
			for (size_t index = world.begin; index < calls.size(); ++index)
			{
				const auto& call = calls[index];
				for (const auto* sound : { &match.chair_sound, &match.celebrate_sound, &match.slump_sound })
				{
					if (call.call == SoundCall::play_voice && call.voice == world.voices.at(sound->name))
					{
						events.push_back(sound->name);
					}
				}
				for (const auto* sound : { &match.hover_sound, &match.click_sound, &match.close_sound })
				{
					if (call.call == SoundCall::play_wave && call.wave == world.waves.at(sound->name))
					{
						events.push_back(sound->name);
					}
				}
			}
			const std::vector<std::string> expected = winner == PlayerTeam::none ?
				std::vector<std::string>{ match.chair_sound.name, match.close_sound.name, match.chair_sound.name } :
				std::vector<std::string>{ match.chair_sound.name, match.hover_sound.name, match.hover_sound.name,
					match.click_sound.name, match.chair_sound.name, match.celebrate_sound.name, match.slump_sound.name };
			require(events == expected, "Long reveal lost, duplicated or reordered its crossed winner/draw edges");
			world.wave_volume_is(winner == PlayerTeam::none ? match.close_sound : match.click_sound, 0.4f);
			if (winner != PlayerTeam::none)
			{
				world.wave_volume_is(match.hover_sound, 0.4f);
				world.volume_is(match.celebrate_sound.name, match.celebrate_sound.volume * 0.4f);
				world.volume_is(match.slump_sound.name, match.slump_sound.volume * 0.4f);
			}
		}
	}
