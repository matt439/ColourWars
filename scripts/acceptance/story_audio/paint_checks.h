// Included inside the story-audio fixture's anonymous namespace.
// Paint audio checks. Uses choice_plays from movement_checks.h and
// PaintForReveal from story_checks.h, so it is included after both;
// paint_surface and paint_contact are also used by painted_checks.h.
	void check_paint_content(World& world, const ContentVariant& content)
	{
		const auto rejected = [&](auto edit, const std::string& key, bool resources = false)
		{
			bool invalid = false;
			std::string message = "no exception";
			try
			{
				const GameTuning tuning = content.tuning(edit);
				if (resources) { tuning.validate_resources(world.render, world.audio); }
			}
			catch (const std::exception& error)
			{
				message = error.what();
				invalid = message.find(": paint") != std::string::npos && message.find(key) != std::string::npos;
			}
			if (!invalid) { throw std::runtime_error("Invalid paint audio content '" + key + "': " + message); }
		};
		rejected([](rapidjson::Document& json) { json["paint"]["splash_sounds"].Clear(); }, "splash_sounds");
		for (size_t index = 0; index < world.tuning.paint().splash_sounds.size(); ++index)
		{
			rejected([&](rapidjson::Document& json)
			{
				json["paint"]["splash_sounds"][static_cast<rapidjson::SizeType>(index)]["name"].SetString(
					"missing.paint.splash", json.GetAllocator());
			}, "splash_sounds", true);
			for (float gain : { -0.01f, 1.01f })
			{
				rejected([&](rapidjson::Document& json)
				{
					json["paint"]["splash_sounds"][static_cast<rapidjson::SizeType>(index)]["volume"].SetFloat(gain);
				}, "splash_sounds");
			}
		}
		for (const char* key : { "capture_sound", "dud_sound" })
		{
			rejected([&](rapidjson::Document& json)
			{
				json["paint"][key]["name"].SetString("missing.paint.feedback", json.GetAllocator());
			}, key, true);
			for (float gain : { -0.01f, 1.01f })
			{
				rejected([&](rapidjson::Document& json) { json["paint"][key]["volume"].SetFloat(gain); }, key);
			}
		}
		for (const char* tool : { "spray", "jet", "rolling", "mist", "ball" })
		{
			rejected([&](rapidjson::Document& json)
			{
				json["paint"]["impact_sounds"][tool]["name"].SetString("missing.paint.impact", json.GetAllocator());
			}, std::string("impact_sounds.") + tool, true);
			for (float gain : { -0.01f, 1.01f })
			{
				rejected([&](rapidjson::Document& json)
				{
					json["paint"]["impact_sounds"][tool]["volume"].SetFloat(gain);
				}, "impact_sounds");
			}
		}
		for (const char* key : { "min_impact_interval", "min_capture_interval" })
		{
			for (float value : { 0.0f, -1.0f })
			{
				rejected([&](rapidjson::Document& json) { json["paint"][key].SetFloat(value); }, key);
			}
		}
	}

	void check_paint_limits_and_variants(World& world, const ContentVariant& content)
	{
		const GameTuning tuning = content.tuning([](rapidjson::Document& json)
		{
			json["paint"]["min_impact_interval"].SetFloat(0.2f);
			json["paint"]["min_capture_interval"].SetFloat(0.4f);
		});
		const auto& paint = tuning.paint();
		PaintAudio audio(&paint, &world.audio);
		world.set_sfx_volume(35);
		world.begin = recorded_sounds(world.device).size();
		audio.update(weapon_step);
		audio.contact(ProjectileType::spray, 0, true, true);
		require(choice_plays(world, paint.splash_sounds) == 1 && world.wave_plays(paint.capture_sound.name) == 1,
			"Fresh painted contact did not emit one splash and one capture");
		world.wave_volume_is(paint.splash_sounds[0], 0.35f);
		world.wave_volume_is(paint.capture_sound, 0.35f);
		for (int repeat = 0; repeat < 20; ++repeat)
		{
			audio.contact(ProjectileType::spray, 0, true, true);
			audio.contact(ProjectileType::spray, 0, false, false);
		}
		audio.update(0.0f);
		audio.update(-1.0f);
		audio.contact(ProjectileType::spray, 0, true, true);
		require(choice_plays(world, paint.splash_sounds) == 1 && world.wave_plays(paint.capture_sound.name) == 1 &&
			world.wave_plays(paint.dud_sound.name) == 0,
			"Repeated paint/dud contacts or nonpositive updates bypassed the shared rate limit");
		audio.update(0.201f);
		audio.contact(ProjectileType::spray, 0, true, true);
		require(choice_plays(world, paint.splash_sounds) == 2 && world.wave_plays(paint.capture_sound.name) == 1,
			"Impact and capture cooldowns did not remain independent");
		audio.update(0.201f);
		audio.contact(ProjectileType::spray, 0, true, true);
		require(world.wave_plays(paint.capture_sound.name) == 2, "Capture did not recover after its authored interval");
		for (size_t index = 3; index < paint.splash_sounds.size(); ++index)
		{
			audio.update(0.201f);
			audio.contact(ProjectileType::spray, 0, true, false);
		}
		for (const auto& sound : paint.splash_sounds)
		{
			require(world.wave_plays(sound.name) == 1, "Audible splash variants skipped or repeated before cycling");
		}
		audio.update(100.0f);
		audio.contact(ProjectileType::spray, 0, false, false);
		audio.contact(ProjectileType::spray, 0, true, false);
		require(world.wave_plays(paint.dud_sound.name) == 1 && choice_plays(world, paint.splash_sounds) ==
			static_cast<int>(paint.splash_sounds.size()), "Long update created an impact backlog or separate dud budget");

		PaintAudio independent(&paint, &world.audio);
		world.set_sfx_volume(0);
		world.begin = recorded_sounds(world.device).size();
		independent.update(weapon_step);
		for (int owner = 0; owner < 4; ++owner)
		{
			independent.contact(ProjectileType::spray, owner, true, true);
			independent.contact(ProjectileType::spray, owner, true, true);
		}
		require(choice_plays(world, paint.splash_sounds) == 4 && world.wave_plays(paint.capture_sound.name) == 4,
			"Four paint owners shared a cooldown or failed to coalesce repeated contacts");
		for (size_t index = 0; index < 4; ++index)
		{
			require(world.wave_plays(paint.splash_sounds[index].name) == 1, "Paint owners shared their initial splash variant");
			world.wave_volume_is(paint.splash_sounds[index], 0.0f);
		}
		world.wave_volume_is(paint.capture_sound, 0.0f);
		world.set_sfx_volume(65);
		independent.update(0.401f);
		independent.contact(ProjectileType::spray, 0, true, true);
		require(close_to(world.last_wave_volume(paint.splash_sounds[1]), paint.splash_sounds[1].volume * 0.65f),
			"Paint feedback retained the old SFX preference");
		world.begin = recorded_sounds(world.device).size();
		independent.contact(ProjectileType::spray, paint_owners::index(-1, 0), true, false);
		independent.contact(ProjectileType::spray, paint_owners::index(-1, 1), true, false);
		independent.contact(ProjectileType::spray, -1, true, false);
		independent.contact(ProjectileType::spray, 1000, true, false);
		require(choice_plays(world, paint.splash_sounds) == 3,
			"Device-less paint owners shared a budget or invalid owners escaped their bounded fallback");

		AudioResources silent;
		silent.add_sound_bank(paint.sound_bank, SoundBank::silent());
		PaintAudio fallback(&paint, &silent);
		fallback.update(weapon_step);
		fallback.contact(ProjectileType::ball, 0, true, true);
		fallback.update(1.0f);
		fallback.contact(ProjectileType::ball, 0, false, false);
	}

	std::unique_ptr<StructurePaintable> paint_surface(World& world, const mattmath::RectangleF& rectangle,
		const PaintableFaces& faces = { false, true, false, false })
	{
		return std::make_unique<StructurePaintable>("sprite_sheet_1", "square_white_4", rectangle,
			std::make_unique<mattmath::RectangleF>(rectangle), &world.render, &world.audio,
			&world.preferences, &world.tuning.paint(), CollisionObjectType::structure_paintable, TeamColour{}, faces);
	}

	auto paint_projectiles(World& world, ProjectileType type, PlayerTeam team = PlayerTeam::a,
		int owner = 0, mattmath::Vector2F position = { 50.0f, 0.0f },
		mattmath::Vector2F velocity = mattmath::Vector2F::ZERO)
	{
		return ProjectileBuilder(&world.tuning, &world.render).build_projectiles(position, velocity, team, owner,
			Colour::blue, type, &world.render);
	}

	void paint_contact(Projectile& projectile, StructurePaintable& surface, bool projectile_first = false)
	{
		if (projectile_first) { projectile.on_contact(surface, mattmath::Vector2F::ZERO, 0.0f); }
		surface.on_contact(projectile, mattmath::Vector2F::ZERO, 0.0f);
		if (!projectile_first) { projectile.on_contact(surface, mattmath::Vector2F::ZERO, 0.0f); }
	}

	void check_paint_contacts(World& world)
	{
		using namespace mattmath;
		const auto& paint = world.tuning.paint();
		world.set_sfx_volume(50);
		for (const auto type : { ProjectileType::spray, ProjectileType::jet, ProjectileType::rolling,
			ProjectileType::mist, ProjectileType::ball })
		{
			for (bool projectile_first : { false, true })
			{
				PaintAudio audio(&paint, &world.audio);
				audio.update(weapon_step);
				auto surface = paint_surface(world, { 0, 0, 100, 50 });
				auto shots = paint_projectiles(world, type);
				world.begin = recorded_sounds(world.device).size();
				for (auto& shot : shots)
				{
					paint_contact(*shot, *surface, projectile_first);
					shot->flush_paint_audio(audio);
					shot->flush_paint_audio(audio);
				}
				require(surface->paint_total().team_a > 0.0f && choice_plays(world, paint.splash_sounds) == 1 &&
					world.wave_plays(paint.capture_sound.name) == 1 && world.wave_plays(paint.dud_sound.name) == 0,
					"Actual tool contact lost paint, repeated its volley, or depended on callback order");
				const auto& impact = paint.impact_sounds[static_cast<size_t>(type)];
				require(world.wave_plays(impact.name) == 1, "Painted projectile did not use its tool's impact cue");
				world.wave_volume_is(impact, 0.5f);
				const float captured = surface->paint_total().player_areas[0];
				audio.update(std::max(paint.min_impact_interval, paint.min_capture_interval) + 0.001f);
				auto friendly = paint_projectiles(world, type, PlayerTeam::a, 1);
				world.begin = recorded_sounds(world.device).size();
				for (auto& shot : friendly) { paint_contact(*shot, *surface); shot->flush_paint_audio(audio); }
				require(world.wave_plays(paint.capture_sound.name) == 0 &&
					close_to(surface->paint_total().player_areas[0], captured) && surface->paint_total().player_areas[1] == 0.0f,
					"Friendly repaint emitted capture feedback or transferred scoring credit");
				audio.update(std::max(paint.min_impact_interval, paint.min_capture_interval) + 0.001f);
				auto enemy = paint_projectiles(world, type, PlayerTeam::b, 1);
				world.begin = recorded_sounds(world.device).size();
				for (auto& shot : enemy) { paint_contact(*shot, *surface); shot->flush_paint_audio(audio); }
				require(world.wave_plays(paint.capture_sound.name) == 1 && surface->paint_total().team_b > 0.0f &&
					surface->paint_total().player_areas[1] > 0.0f, "Enemy repaint missed capture feedback or scoring transfer");
			}
		}

		PaintAudio audio(&paint, &world.audio);
		audio.update(weapon_step);
		auto first = paint_surface(world, { 0, 0, 100, 50 });
		auto second = paint_surface(world, { 0, 0, 100, 50 });
		auto unpaintable = paint_surface(world, { 0, 0, 100, 50 }, {});
		auto rollers = paint_projectiles(world, ProjectileType::rolling);
		world.begin = recorded_sounds(world.device).size();
		paint_contact(*rollers[0], *unpaintable);
		paint_contact(*rollers[0], *first);
		paint_contact(*rollers[0], *second);
		rollers[0]->flush_paint_audio(audio);
		require(!rollers[0]->for_deletion() && first->paint_total().team_a > 0.0f && second->paint_total().team_a > 0.0f &&
			choice_plays(world, paint.splash_sounds) == 1 && world.wave_plays(paint.capture_sound.name) == 1 &&
			world.wave_plays(paint.dud_sound.name) == 0, "Roller contacts did not aggregate across its full painting volume");
		for (float dt : { 0.0f, -1.0f })
		{
			audio.update(dt);
			auto pending = paint_projectiles(world, ProjectileType::jet);
			paint_contact(*pending[0], *first);
			world.begin = recorded_sounds(world.device).size();
			pending[0]->flush_paint_audio(audio);
			audio.update(10.0f);
			pending[0]->flush_paint_audio(audio);
			require(choice_plays(world, paint.splash_sounds) == 0 && world.wave_plays(paint.capture_sound.name) == 0,
				"Nonpositive paint step emitted or retained a catch-up contact");
		}
	}

	void check_paint_duds_and_launch(World& world)
	{
		using namespace mattmath;
		const auto& paint = world.tuning.paint();
		for (bool launch : { false, true })
		{
			PaintAudio audio(&paint, &world.audio);
			audio.update(weapon_step);
			const RectangleF rectangle(0, -50, 5, 100);
			Structure cover("sprite_sheet_1", "square_white_4", rectangle,
				std::make_unique<RectangleF>(rectangle), &world.render, CollisionObjectType::structure);
			CollisionObject* objects[]{ &cover };
			auto shots = paint_projectiles(world, ProjectileType::jet, PlayerTeam::a, 0,
				launch ? Vector2F(20, 0) : Vector2F(-20, 0), { 1000, 0 });
			world.begin = recorded_sounds(world.device).size();
			if (launch) { shots[0]->resolve_launch_obstruction({ -20, 0 }, objects); }
			else { shots[0]->update(0.05f); shots[0]->resolve_movement_contacts(objects); }
			shots[0]->flush_paint_audio(audio);
			shots[0]->flush_paint_audio(audio);
			require(shots[0]->for_deletion() && world.wave_plays(paint.dud_sound.name) == 1 &&
				choice_plays(world, paint.splash_sounds) == 0 && world.wave_plays(paint.capture_sound.name) == 0,
				"Swept or launch bare-geometry contact missed or duplicated its dud");
		}
		PaintAudio audio(&paint, &world.audio);
		audio.update(weapon_step);
		auto no_faces = paint_surface(world, { 0, 0, 100, 50 }, {});
		auto shot = paint_projectiles(world, ProjectileType::jet);
		world.begin = recorded_sounds(world.device).size();
		paint_contact(*shot[0], *no_faces, true);
		shot[0]->flush_paint_audio(audio);
		require(world.wave_plays(paint.dud_sound.name) == 1 && choice_plays(world, paint.splash_sounds) == 0,
			"Paintable geometry with no touched scoring face did not classify as a dud");
		audio.update(paint.min_impact_interval + 0.001f);
		world.begin = recorded_sounds(world.device).size();
		auto lifetime = paint_projectiles(world, ProjectileType::jet);
		lifetime[0]->update(weapon_step);
		lifetime[0]->update(world.tuning.projectile(ProjectileType::jet).details.delete_timer + 1.0f);
		lifetime[0]->flush_paint_audio(audio);
		require(lifetime[0]->for_deletion() && world.wave_plays(paint.dud_sound.name) == 0,
			"Lifetime retirement invented a geometry dud");

		// The actual Player launch path discards obstructed shots before returning
		// them to Level. Their accumulated audio must be flushed before that erase.
		const RectangleF player_rectangle(Vector2F::ZERO, world.tuning.player().size);
		Player player(player_rectangle, &world.tuning, &world.presentation.movement_feedback(), &world.presentation.combat_feedback(),
			&world.presentation.respawn_redraw(), &world.render, &world.audio, &world.preferences,
			0, PlayerTeam::a, WeaponType::sniper, world.weapon(world.tuning, WeaponType::sniper),
			WeaponType::none, Colour::blue, Viewport{}, Vector2F::ZERO);
		player.set_paint_audio(&audio);
		const RectangleF enclosure(-1000, -1000, 2000, 2000);
		Structure cover("sprite_sheet_1", "square_white_4", enclosure,
			std::make_unique<RectangleF>(enclosure), &world.render, CollisionObjectType::structure);
		CollisionObject* objects[]{ &cover };
		PlayerInputData held;
		held.primary_shoot = true;
		player.set_player_input(held);
		audio.update(paint.min_impact_interval + 0.001f);
		world.begin = recorded_sounds(world.device).size();
		const auto& sniper = world.tuning.weapon(WeaponType::sniper);
		player.update_weapon_and_get_projectiles(sniper.details.shoot_interval + 0.001f, objects);
		const auto launched = player.update_weapon_and_get_projectiles(weapon_step, objects);
		require(launched.empty() && world.wave_plays(sniper.details.shoot_sound_name) == 1 &&
			world.wave_plays(paint.dud_sound.name) == 1, "Player erased an obstructed paid shot before emitting its launch dud");
	}

	void check_level_paint_after_owner_death(World& world)
	{
		const auto& paint = world.tuning.paint();
		world.build(OpeningMode::off, 2, 10.0f);
		world.until([&] { return world.level->state() == LevelState::active; });
		world.level->set_player_cheats(true, true);
		require(world.level->place_player(0, { 2000.0f, 4700.0f }), "Cannot place Level paint audio fixture");
		PaintForReveal fire(0);
		for (int tick = 0; tick < 60 && world.level->projectile_count() == 0; ++tick)
		{
			world.level->update(world.idle, weapon_step, &fire);
		}
		require(world.level->projectile_count() > 0, "Level fixture did not admit a flying paint projectile");
		world.begin = recorded_sounds(world.device).size();
		require(world.level->kill_player(0), "Cannot kill Level paint owner fixture");
		world.level->update(world.idle, 0.0f, &fire);
		world.level->update(world.idle, -1.0f, &fire);
		require(choice_plays(world, paint.splash_sounds) == 0 && world.wave_plays(paint.capture_sound.name) == 0,
			"Nonpositive Level updates emitted pending paint feedback");
		for (int tick = 0; tick < 90; ++tick) { world.level->update(world.idle, weapon_step); }
		require(world.level->player(0).state() == PlayerState::dead &&
			choice_plays(world, paint.splash_sounds) == 1 && world.wave_plays(paint.capture_sound.name) == 1 &&
			world.wave_plays(paint.impact_sounds[static_cast<size_t>(ProjectileType::spray)].name) == 1 &&
			world.level->level_end_info().team_a_area > 0.0f,
			"Level failed to flush a flying paint hit after its owner died");
	}
