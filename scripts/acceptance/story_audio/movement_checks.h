// Included inside the story-audio fixture's anonymous namespace.
// Movement audio checks and the MovementCourse rig. MovementCourse,
// movement_input and choice_plays are also used by paint_checks.h and
// painted_checks.h.
	void check_movement_content(World& world, const ContentVariant& content)
	{
		const auto rejected = [&](auto edit, const std::string& key, bool resources = false)
		{
			bool invalid = false;
			try
			{
				const GameTuning tuning = content.tuning(edit);
				if (resources) { tuning.validate_resources(world.render, world.audio); }
			}
			catch (const std::exception& error)
			{
				const std::string message = error.what();
				invalid = message.find("movement_sounds") != std::string::npos &&
					message.find(key) != std::string::npos;
			}
			require(invalid, "Invalid movement sound content was accepted or lost its authored key");
		};
		for (const char* key : { "land", "step", "step_painted" })
		{
			rejected([&](rapidjson::Document& json)
			{
				json["player"]["movement_sounds"][key].Clear();
			}, key);
			const auto& sounds = world.tuning.player().movement_sounds;
			const auto& choices = std::string(key) == "land" ? sounds.land :
				(std::string(key) == "step" ? sounds.step : sounds.step_painted);
			for (size_t index = 0; index < choices.size(); ++index)
			{
				rejected([&](rapidjson::Document& json)
				{
					json["player"]["movement_sounds"][key][static_cast<rapidjson::SizeType>(index)]["name"].SetString(
						"missing.player.movement", json.GetAllocator());
				}, std::string(key) + "[" + std::to_string(index) + "]", true);
				for (float gain : { -0.01f, 1.01f })
				{
					rejected([&](rapidjson::Document& json)
					{
						json["player"]["movement_sounds"][key][static_cast<rapidjson::SizeType>(index)]["volume"].SetFloat(gain);
					}, key);
				}
			}
		}
		rejected([](rapidjson::Document& json)
		{
			json["player"]["movement_sounds"]["skid"]["name"].SetString("missing.player.skid", json.GetAllocator());
		}, "skid", true);
		for (float gain : { -0.01f, 1.01f })
		{
			rejected([&](rapidjson::Document& json)
			{
				json["player"]["movement_sounds"]["skid"]["volume"].SetFloat(gain);
			}, "skid");
		}
		for (const char* key : { "stride_distance", "min_step_interval", "skid_min_speed", "skid_repeat_delay", "landing_full_speed" })
		{
			for (float value : { 0.0f, -1.0f })
			{
				rejected([&](rapidjson::Document& json)
				{
					json["player"]["movement_sounds"][key].SetFloat(value);
				}, key);
			}
		}
		for (float value : { -0.01f, 1.01f })
		{
			rejected([&](rapidjson::Document& json)
			{
				json["player"]["movement_sounds"]["landing_min_gain"].SetFloat(value);
			}, "landing_min_gain");
		}
	}

	// Actual players, factory weapons and collision resolution. Audio is observed
	// only after every contact, as in Level's normal simulation phase ordering.
	struct MovementCourse
	{
		World& world;
		const GameTuning& tuning;
		Scene scene{ nullptr, nullptr };
		std::vector<Player*> players;
		std::vector<float> downward_speed;

		MovementCourse(World& resources, const GameTuning& balance) : world(resources), tuning(balance) {}

		Player& add(float x = 0.0f, float bottom = 0.0f, float speed = 0.0f)
		{
			using namespace mattmath;
			const int index = static_cast<int>(this->players.size());
			const auto team = index < 2 ? PlayerTeam::a : PlayerTeam::b;
			const RectangleF rectangle(x, bottom - this->tuning.player().size.y,
				this->tuning.player().size.x, this->tuning.player().size.y);
			Player* player = this->scene.add(std::make_unique<Player>(rectangle, &this->tuning,
				&this->world.presentation.movement_feedback(), &this->world.presentation.combat_feedback(), &this->world.presentation.respawn_redraw(),
				&this->world.render, &this->world.audio, &this->world.preferences, index, team,
				WeaponType::sprayer, this->world.weapon(this->tuning, WeaponType::sprayer, index, team),
				WeaponType::none, Colour::blue, Viewport{}, rectangle.top_left(), Vector2F(speed, 0.0f)));
			this->players.push_back(player);
			this->scene.end_tick();
			return *player;
		}

		void surface(const mattmath::RectangleF& rectangle,
			CollisionObjectType type = CollisionObjectType::structure)
		{
			this->scene.add(std::make_unique<Structure>("sprite_sheet_1", "square_white_4", rectangle,
				std::make_unique<mattmath::RectangleF>(rectangle), &this->world.render,
				type));
			this->scene.end_tick();
		}

		void floor() { this->surface({ -10000.0f, 0.0f, 20000.0f, 100.0f }); }

		void ramp(const mattmath::TriangleRightAxisAligned& shape, CollisionObjectType type)
		{
			this->scene.add(std::make_unique<Structure>("sprite_sheet_1", "square_white_4", shape.bounding_box(),
				std::make_unique<mattmath::TriangleRightAxisAligned>(shape), &this->world.render, type));
			this->scene.end_tick();
		}

		void tick(const PlayerInputData& input = {}, float dt = weapon_step, bool fatal = false)
		{
			for (auto* player : this->players) { player->set_player_input(input); }
			this->scene.update(dt);
			this->downward_speed.clear();
			for (auto* player : this->players) { this->downward_speed.push_back(player->velocity().y); }
			for (auto* player : this->players) { player->update_weapon_and_get_projectiles(dt, {}); }
			this->scene.resolve();
			for (auto* player : this->players)
			{
				if (fatal) { player->kill(); }
				player->end_contacts(this->scene.contacts());
				player->update_prev_rectangle();
			}
			this->scene.end_tick();
		}

		void land()
		{
			for (int tick = 0; tick < 600; ++tick)
			{
				this->tick();
				if (std::all_of(this->players.begin(), this->players.end(), [](const Player* player)
					{ return player->move_state() == PlayerMoveState::on_ground; })) { return; }
			}
			throw std::runtime_error("Movement audio fixture did not reach the floor");
		}
	};

	PlayerInputData movement_input(float x, bool jump = false)
	{
		PlayerInputData input;
		input.x_movement = x;
		input.jump_pressed = jump;
		input.jump_held = jump;
		return input;
	}

	int choice_plays(const World& world, const std::vector<SoundTuning>& choices)
	{
		int count = 0;
		for (const auto& choice : choices) { count += world.wave_plays(choice.name); }
		return count;
	}

	void check_movement_landings(World& world)
	{
		world.set_sfx_volume(40);
		world.begin = recorded_sounds(world.device).size();
		const auto& sounds = world.tuning.player().movement_sounds;
		MovementCourse course(world, world.tuning);
		Player& player = course.add(0.0f, -300.0f);
		course.floor();
		course.land();
		require(choice_plays(world, sounds.land) == 0 && choice_plays(world, sounds.step) == 0 &&
			world.wave_plays(sounds.skid.name) == 0, "Spawn falling/settling played movement audio");
		for (size_t landing = 0; landing < sounds.land.size() + 1; ++landing)
		{
			course.tick(movement_input(0.0f, true));
			require(player.velocity().y < 0.0f, "Movement audio fixture failed to jump");
			course.tick({}, 0.0f);
			course.land();
			const auto& sound = sounds.land[landing % sounds.land.size()];
			const float impact_gain = std::clamp(course.downward_speed.front() / sounds.landing_full_speed,
				sounds.landing_min_gain, 1.0f);
			require(choice_plays(world, sounds.land) == static_cast<int>(landing + 1) &&
				world.wave_plays(sound.name) ==
				static_cast<int>(landing / sounds.land.size() + 1),
				"Completed jump landing missed, repeated or failed to cycle its variants");
			require(close_to(world.last_wave_volume(sound), sound.volume * impact_gain * 0.4f),
				"Landing ignored resolved impact speed or current SFX gain");
			course.tick({}, 0.0f);
			course.tick({}, -weapon_step);
			course.tick();
			require(choice_plays(world, sounds.land) == static_cast<int>(landing + 1),
				"Nonpositive movement ticks or sustained support invented a landing");
		}
		require(choice_plays(world, sounds.step) == 0 && world.wave_plays(sounds.skid.name) == 0,
			"Vertical jumps emitted footsteps or skids");
		player.force_respawn();
		world.begin = recorded_sounds(world.device).size();
		course.land();
		require(choice_plays(world, sounds.land) == 0, "Respawn fall/settling played a landing");
		world.set_sfx_volume(0);
		course.tick(movement_input(0.0f, true));
		course.land();
		require(choice_plays(world, sounds.land) == 1, "Respawn did not rearm its next ordinary landing");
		for (const auto& sound : sounds.land)
		{
			if (world.wave_plays(sound.name) > 0) { world.wave_volume_is(sound, 0.0f); }
		}

		// A fatal hit can arrive after update and ground resolution, before the
		// final state changes to dead on the following update.
		course.tick(movement_input(0.0f, true));
		bool next_contact = false;
		for (int tick = 0; tick < 600; ++tick)
		{
			const float next_speed = std::min(player.velocity().y + world.tuning.player().gravity * weapon_step,
				world.tuning.player().max_velocity.y);
			if (next_speed > 0.0f && player.rectangle().bottom() + next_speed * weapon_step >= 0.0f)
			{
				next_contact = true;
				break;
			}
			course.tick();
		}
		require(next_contact, "Fatal movement audio fixture never approached its landing");
		world.begin = recorded_sounds(world.device).size();
		course.tick({}, weapon_step, true);
		require(player.state() == PlayerState::alive && player.health() <= 0.0f,
			"Movement audio fatal-contact fixture missed its boundary");
		for (int tick = 0; tick < 30; ++tick) { course.tick(movement_input(1.0f)); }
		require(choice_plays(world, sounds.land) == 0 && choice_plays(world, sounds.step) == 0 &&
			world.wave_plays(sounds.skid.name) == 0, "Fatal/dead player emitted movement audio");
	}

	void check_movement_impact_gain(World& world)
	{
		const auto& sounds = world.tuning.player().movement_sounds;
		world.set_sfx_volume(40);
		float short_volume = 0.0f;
		for (float drop : { 20.0f, 1000.0f })
		{
			MovementCourse course(world, world.tuning);
			Player& player = course.add(0.0f, 0.0f, world.tuning.player().max_velocity.x);
			course.surface({ -1000.0f, 0.0f, 1020.0f, 100.0f });
			course.surface({ -10000.0f, drop, 20000.0f, 100.0f });
			course.tick(movement_input(1.0f));
			require(player.move_state() == PlayerMoveState::on_ground, "Impact fixture did not start supported");
			world.begin = recorded_sounds(world.device).size();
			bool airborne = false;
			bool landed = false;
			for (int tick = 0; tick < 600; ++tick)
			{
				course.tick(movement_input(1.0f));
				airborne = airborne || player.move_state() == PlayerMoveState::in_air;
				if (airborne && player.move_state() == PlayerMoveState::on_ground) { landed = true; break; }
			}
			require(landed && choice_plays(world, sounds.land) == 1, "Walked-off drop missed or repeated its landing");
			const float gain = std::clamp(course.downward_speed.front() / sounds.landing_full_speed,
				sounds.landing_min_gain, 1.0f);
			const float volume = world.last_wave_volume(sounds.land.front());
			require(close_to(volume, sounds.land.front().volume * gain * 0.4f),
				"Small/large drop landing ignored authored impact scaling");
			if (drop == 20.0f) { short_volume = volume; }
			else { require(volume > short_volume, "Long fall did not produce a stronger landing than a short drop"); }
		}
	}

	void check_movement_steps_and_walls(World& world)
	{
		world.set_sfx_volume(40);
		world.begin = recorded_sounds(world.device).size();
		const auto& sounds = world.tuning.player().movement_sounds;
		MovementCourse course(world, world.tuning);
		Player& player = course.add();
		// Adjacent floor rectangles exercise the completed support state at seams.
		course.surface({ -10000.0f, 0.0f, 10300.0f, 100.0f });
		course.surface({ 300.0f, 0.0f, 10000.0f, 100.0f });
		course.land();
		float distance = 0.0f;
		float since_step = 1000.0f;
		int steps = 0;
		for (int tick = 0; tick < 240; ++tick)
		{
			const float before = player.rectangle().left();
			course.tick(movement_input(1.0f));
			distance += std::abs(player.rectangle().left() - before);
			since_step += weapon_step;
			const int count = choice_plays(world, sounds.step);
			require(count <= steps + 1 && distance + 0.001f >= count * sounds.stride_distance,
				"Footsteps burst or played without resolved grounded stride distance");
			if (count != steps)
			{
				require(since_step + 0.0001f >= sounds.min_step_interval &&
					world.wave_plays(sounds.step[static_cast<size_t>(steps) % sounds.step.size()].name) ==
					steps / static_cast<int>(sounds.step.size()) + 1,
					"Footstep rate limit or deterministic variant cycle failed");
				since_step = 0.0f;
				steps = count;
			}
		}
		require(steps >= static_cast<int>(sounds.step.size()) + 1 && choice_plays(world, sounds.land) == 0,
			"Ground travel missed footsteps or invented landings at floor seams");
		for (const auto& sound : sounds.step) { world.wave_volume_is(sound, 0.4f); }
		const float wall_x = player.rectangle().right() + 1.0f;
		course.surface({ wall_x, -1000.0f, 100.0f, 1000.0f });
		for (int tick = 0; tick < 120; ++tick) { course.tick(movement_input(1.0f)); }
		const int blocked_steps = choice_plays(world, sounds.step);
		for (int tick = 0; tick < 120; ++tick) { course.tick(movement_input(1.0f)); }
		require(choice_plays(world, sounds.step) == blocked_steps && choice_plays(world, sounds.land) == 0,
			"Pushing against a wall or contact separation invented steps/landings");
		world.begin = recorded_sounds(world.device).size();
		course.tick(movement_input(-1.0f, true));
		for (int tick = 0; tick < 12; ++tick) { course.tick(movement_input(-1.0f)); }
		require(choice_plays(world, sounds.land) == 0 && choice_plays(world, sounds.step) == 0 &&
			world.wave_plays(sounds.skid.name) == 0, "Airborne horizontal movement emitted ground cues");
	}

	void check_movement_contact_kinds(World& world)
	{
		using namespace mattmath;
		const auto& sounds = world.tuning.player().movement_sounds;
		MovementCourse buffered(world, world.tuning);
		Player& player = buffered.add();
		buffered.floor();
		buffered.land();
		world.begin = recorded_sounds(world.device).size();
		buffered.tick(movement_input(0.0f, true));
		bool queued = false;
		for (int tick = 0; tick < 150 && choice_plays(world, sounds.land) == 0; ++tick)
		{
			const bool press = !queued && player.velocity().y > 0.0f && player.rectangle().bottom() > -25.0f;
			buffered.tick(movement_input(0.0f, press));
			queued = queued || press;
		}
		require(queued && player.velocity().y < 0.0f && choice_plays(world, sounds.land) == 1 &&
			choice_plays(world, sounds.step) == 0, "Buffered landing/takeoff missed its landing or invented a step");
		buffered.land();
		require(choice_plays(world, sounds.land) == 2, "Buffered takeoff failed to rearm its next landing");

		MovementCourse platform(world, world.tuning);
		platform.add();
		platform.surface({ -10000.0f, 0.0f, 20000.0f, 20.0f }, CollisionObjectType::structure_jump_through);
		platform.surface({ -10000.0f, 300.0f, 20000.0f, 100.0f });
		platform.tick();
		world.begin = recorded_sounds(world.device).size();
		PlayerInputData drop;
		drop.drop_through = true;
		platform.tick(drop);
		platform.land();
		require(choice_plays(world, sounds.land) == 1 && choice_plays(world, sounds.step) == 0,
			"Drop-through platform missed its lower-floor landing or invented footsteps");

		for (bool uphill_right : { false, true })
		{
			const float gradient = 450.0f / 1400.0f;
			const float height = 1000.0f * gradient;
			MovementCourse slope(world, world.tuning);
			slope.add(0.0f, uphill_right ? -world.tuning.player().size.x * gradient : 0.0f);
			slope.ramp(uphill_right ? TriangleRightAxisAligned(1000, -height, -1000, height, 1000, height) :
				TriangleRightAxisAligned(-1000, -height, -1000, height, 1000, height),
				uphill_right ? CollisionObjectType::structure_ramp_right : CollisionObjectType::structure_ramp_left);
			slope.tick();
			world.begin = recorded_sounds(world.device).size();
			slope.tick(movement_input(0.0f, true));
			for (int tick = 0; tick < 120; ++tick) { slope.tick(); }
			require(choice_plays(world, sounds.land) == 1 && choice_plays(world, sounds.step) == 0,
				"Ramp hop missed or repeated its landing under sustained slope gravity");
			for (int tick = 0; tick < 15; ++tick) { slope.tick(movement_input(uphill_right ? 1.0f : -1.0f)); }
			for (int tick = 0; tick < 60; ++tick) { slope.tick(movement_input(uphill_right ? -1.0f : 1.0f)); }
			require(choice_plays(world, sounds.land) == 1 && choice_plays(world, sounds.step) > 0,
				"Slope travel invented landings or failed to count resolved grounded strides");
		}
	}

	void check_movement_skids_and_players(World& world, const ContentVariant& content)
	{
		// Slow authored deceleration keeps the opposite-input condition true long
		// enough to distinguish an edge cue from a held/repeating cue.
		const GameTuning tuning = content.tuning([](rapidjson::Document& json)
		{
			json["player"]["x_acceleration"].SetFloat(1.0f);
			json["player"]["x_deacceleration"].SetFloat(1.0f);
		});
		const auto& sounds = tuning.player().movement_sounds;
		MovementCourse course(world, tuning);
		for (int index = 0; index < 4; ++index) { course.add(index * 2000.0f, 0.0f, tuning.player().max_velocity.x); }
		course.floor();
		course.tick(movement_input(1.0f));
		world.set_sfx_volume(40);
		world.begin = recorded_sounds(world.device).size();
		course.tick(movement_input(-1.0f));
		require(world.wave_plays(sounds.skid.name) == 4, "Four players shared or missed skid edges");
		for (int tick = 0; tick < 10; ++tick) { course.tick(movement_input(-1.0f), 0.0f); }
		course.tick(movement_input(1.0f), -weapon_step);
		course.tick(movement_input(1.0f));
		course.tick(movement_input(-1.0f));
		require(world.wave_plays(sounds.skid.name) == 4, "Nonpositive ticks or rapid reversal bypassed skid cooldown");
		for (int tick = 0; tick < 60; ++tick) { course.tick(movement_input(-1.0f)); }
		require(world.wave_plays(sounds.skid.name) == 4, "Held opposite input repeated its skid edge");
		course.tick(movement_input(1.0f));
		course.tick(movement_input(-1.0f));
		require(world.wave_plays(sounds.skid.name) == 8, "Fresh reversal after skid delay did not rearm per player");
		world.wave_volume_is(sounds.skid, 0.4f);
		for (const auto& sound : sounds.step)
		{
			require(world.wave_plays(sound.name) > 0, "Four-player travel failed to cycle independent footsteps");
		}
		world.set_sfx_volume(0);
		for (int tick = 0; tick < 60; ++tick) { course.tick(movement_input(1.0f)); }
		world.begin = recorded_sounds(world.device).size();
		course.tick(movement_input(-1.0f));
		for (int tick = 0; tick < 60; ++tick) { course.tick(movement_input(-1.0f)); }
		world.wave_volume_is(sounds.skid, 0.0f);
		for (const auto& sound : sounds.step) { world.wave_volume_is(sound, 0.0f); }
		world.begin = recorded_sounds(world.device).size();
		course.tick(movement_input(0.0f, true));
		for (int tick = 0; tick < 4; ++tick) { course.tick(movement_input(-1.0f)); }
		require(choice_plays(world, sounds.land) == 0 && choice_plays(world, sounds.step) == 0 &&
			world.wave_plays(sounds.skid.name) == 0, "Airborne high-speed reversal emitted ground movement cues");
		course.land();
		for (const auto& sound : sounds.land)
		{
			require(world.wave_plays(sound.name) == 1, "Four-player landings shared a variant or support history");
			world.wave_volume_is(sound, 0.0f);
		}
		MovementCourse slow(world, tuning);
		slow.add(0.0f, 0.0f, sounds.skid_min_speed - 1.0f);
		slow.floor();
		slow.tick(movement_input(1.0f));
		world.begin = recorded_sounds(world.device).size();
		slow.tick(movement_input(-1.0f));
		require(world.wave_plays(sounds.skid.name) == 0, "Below-threshold reversal emitted a skid");
	}
