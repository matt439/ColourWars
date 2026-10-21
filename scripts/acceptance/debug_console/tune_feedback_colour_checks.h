// Included inside tune_checks.cpp's private namespace, after its includes.
// Gameplay feedback cue colours: combat, blocked, idle drip and respawn pen.

	void check_combat_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"combat-colour-cues");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const CombatFeedbackPresentation* const look = &presentation.combat_feedback();
		labrador::RenderResources resources;
		resources.add_sprite_sheet(look->sprite_sheet, labrador::read_sprite_sheet(
			(game_directory / L"textures" / (look->sprite_sheet + ".json")).string().c_str(), labrador::TextureHandle{}));
		CombatFeedback hurt(look, &resources), burst(look, &resources);
		const mattmath::RectangleF body(80, 120, 56, 96), detached(240, 120, 56, 96);
		require(hurt.hurt(), "Combat colour fixture starts an actual hurt cue before editing");
		hurt.update(look->hurt_duration * 0.5f);
		burst.eliminate(detached);
		burst.update(look->elimination_duration * 0.5f);
		const auto timers = [&]
		{
			return std::array<float, 4>{ hurt.hurt_remaining(), hurt.elimination_remaining(),
				burst.hurt_remaining(), burst.elimination_remaining() };
		};
		const auto held_timers = timers();
		const auto hurt_bounds = hurt.bounds(body), burst_bounds = burst.bounds(body);
		const auto unchanged = [&]
		{
			require(&presentation.combat_feedback() == look && timers() == held_timers &&
				hurt.death_rectangle() == mattmath::RectangleF::ZERO && burst.death_rectangle() == detached &&
				hurt.bounds(body) == hurt_bounds && burst.bounds(body) == burst_bounds,
				"Combat colour adoption retains the borrowed record, active cue timers, detached body and cull bounds");
		};
		for (const std::size_t index : { combat_highlight_colour_index, combat_keyline_colour_index })
		{
			for (const auto& [word, expected] : std::array<std::pair<const char*, labrador::Colour>, 3>{ {
				{ "RED", { 255, 0, 0 } }, { "2060C080", { 32, 96, 192, 128 } },
				{ "A532D600", { 165, 50, 214, 0 } } } })
			{
				require(impl.execute(std::string("tune ") + colour_paths[index] + ' ' + word).ok &&
					colour_values(presentation)[index] == expected,
					"A preconstructed combat cue accepts straight named/partial/zero-alpha authored channels");
				unchanged();
			}
		}
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const JsonSpanDocument baseline_document(baseline, "combat-colour-merged-reference");
		const JsonLeaf* repeat_delay = baseline_document.find("combat_feedback.hurt_repeat_delay");
		require(repeat_delay != nullptr, "Combat merged candidate has its unrelated timing constraint");
		const std::string external = baseline_document.replaced(*repeat_delay, "0.001");
		write_text(scratch.source / L"presentation.json", external);
		const TuneDocument* const pending = impl.tune_document("presentation");
		const std::string pending_text = pending->current().text();
		const auto pending_colours = colour_values(presentation);
		for (const char* command : { "tune.save presentation", "tune.save presentation force" })
		{
			require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == baseline && pending->current().text() == pending_text &&
				pending->baseline().text() == baseline && pending->edits().size() == 2 &&
				colour_values(presentation) == pending_colours,
				"An unrelated invalid combat constraint rejects the complete merged candidate and restores disk/staging/live/ledger, even with force");
			unchanged();
		}
		require(impl.execute("tune.reset presentation.combat_feedback.highlight").ok &&
			look->highlight == labrador::Colour(255, 255, 255) && look->keyline == labrador::Colour(165, 50, 214, 0) &&
			pending->edits().size() == 1 && pending->edits().contains("combat_feedback.keyline"),
			"Resetting combat highlight retains the independent nonblack zero-alpha keyline edit");
		unchanged();
		require(impl.execute("tune.reset presentation.combat_feedback.keyline").ok &&
			look->keyline == labrador::Colour(0, 0, 0) && pending->edits().empty() && pending->current().text() == baseline,
			"Resetting combat keyline restores its original authored token after rejected saves");
		unchanged();
		write_text(scratch.source / L"presentation.json", baseline);
	}

	void check_blocked_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"blocked-colour-cues");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const BlockedShotPresentation* const look = &presentation.blocked_shot();
		const std::string sheet = look->sprite_sheet, fill = look->fill_frame;
		const auto initial_colours = colour_values(presentation);
		const bool enabled = look->enabled;
		const auto shape = [&]
		{
			return std::array<float, 7>{ look->duration, look->repeat_delay, look->width,
				look->reach, look->bead_size, look->stroke_width, look->keyline_width };
		};
		const auto held_shape = shape();
		labrador::RenderResources resources;
		resources.add_sprite_sheet(sheet, labrador::read_sprite_sheet(
			(game_directory / L"textures" / (sheet + ".json")).string().c_str(), labrador::TextureHandle{}));
		BlockedShotFeedback feedback(look, &resources);
		const std::array<mattmath::Vector2F, 4> normals{ mattmath::Vector2F(2.0f, 0.0f),
			mattmath::Vector2F(0.0f, -2.0f), mattmath::Vector2F(-2.0f, 0.0f), mattmath::Vector2F(0.0f, 2.0f) };
		for (int index = 0; index <= paint_owners::count; ++index)
		{
			feedback.emit({ mattmath::Vector2F(120.0f + static_cast<float>(index) * 40.0f, 200.0f),
				normals[static_cast<std::size_t>(index) % normals.size()],
				labrador::Colour(32 + index * 12, 192 - index * 9, 80 + index * 7, 160),
				index == paint_owners::count ? -1 : index });
		}
		// Keep every repeat budget positive as well as every cue age nonzero;
		// a reset to zero must not pass this retained-state check vacuously.
		feedback.update(std::min(look->duration, look->repeat_delay) * 0.5f);
		require(feedback.cues().size() == static_cast<std::size_t>(paint_owners::count + 1) &&
			feedback.bounds() != mattmath::RectangleF::ZERO &&
			std::all_of(feedback.cues().begin(), feedback.cues().end(), [&](const BlockedShotFeedback::Cue& cue)
				{ return cue.age > 0.0f && cue.age < look->duration; }),
			"Blocked colour fixture retains an actual aged cue for every paint owner and the invalid-owner bucket");
		for (int owner = -1; owner < paint_owners::count; ++owner)
		{
			require(feedback.repeat_remaining(owner) > 0.0f,
				"Every blocked-colour fixture owner starts with an active repeat budget");
		}
		const auto state = [&]
		{
			std::vector<float> values;
			for (const BlockedShotFeedback::Cue& cue : feedback.cues())
			{
				const auto& contact = cue.contact;
				const auto bounds = feedback.cue_bounds(cue);
				values.insert(values.end(), { contact.position.x, contact.position.y,
					contact.normal.x, contact.normal.y, contact.team_colour.r, contact.team_colour.g,
					contact.team_colour.b, contact.team_colour.a, static_cast<float>(contact.player_num),
					cue.age, bounds.x, bounds.y, bounds.width, bounds.height });
			}
			for (int owner = -1; owner < paint_owners::count; ++owner)
				{ values.push_back(feedback.repeat_remaining(owner)); }
			values.push_back(feedback.repeat_remaining(paint_owners::count));
			return values;
		};
		const auto held_state = state();
		const auto held_bounds = feedback.bounds();
		const auto unchanged = [&]
		{
			require(&presentation.blocked_shot() == look && look->sprite_sheet == sheet &&
				look->fill_frame == fill && look->enabled == enabled && shape() == held_shape &&
				state() == held_state && feedback.bounds() == held_bounds &&
				feedback.repeat_remaining(-1) == feedback.repeat_remaining(paint_owners::count),
				"Blocked colour adoption retains the borrowed look, assets, geometry, stored contacts/team tints, ages, bounds and every owner repeat budget");
		};
		std::srand(0x6b117);
		const int expected_random = std::rand();
		std::srand(0x6b117);
		for (const std::size_t index : { blocked_highlight_colour_index, blocked_keyline_colour_index })
		{
			for (const auto& [word, expected] : std::array<std::pair<const char*, labrador::Colour>, 3>{ {
				{ "RED", { 255, 0, 0 } }, { "2060C080", { 32, 96, 192, 128 } },
				{ "A532D600", { 165, 50, 214, 0 } } } })
			{
				require(impl.execute(std::string("tune ") + colour_paths[index] + ' ' + word).ok &&
					colour_values(presentation)[index] == expected,
					"A preconstructed blocked cue accepts straight named/partial/zero-alpha authored channels");
				unchanged();
			}
		}
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const JsonSpanDocument baseline_document(baseline, "blocked-colour-merged-reference");
		const JsonLeaf* repeat_delay = baseline_document.find("blocked_shot.repeat_delay");
		require(repeat_delay != nullptr, "Blocked merged candidate has its unrelated timing constraint");
		const std::string external = baseline_document.replaced(*repeat_delay, "0.001");
		write_text(scratch.source / L"presentation.json", external);
		const TuneDocument* const pending = impl.tune_document("presentation");
		const std::string pending_text = pending->current().text();
		const auto pending_colours = colour_values(presentation);
		for (const char* command : { "tune.save presentation", "tune.save presentation force" })
		{
			require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == baseline && pending->current().text() == pending_text &&
				pending->baseline().text() == baseline && pending->edits().size() == 2 &&
				pending->edits().contains("blocked_shot.highlight") && pending->edits().contains("blocked_shot.keyline") &&
				colour_values(presentation) == pending_colours,
				"An invalid blocked timing constraint rejects the complete merged candidate without changing disk/staging/live/ledger, even with force");
			unchanged();
		}
		require(impl.execute("tune.reset presentation.blocked_shot.highlight").ok &&
			look->highlight == initial_colours[blocked_highlight_colour_index] &&
			look->keyline == labrador::Colour(165, 50, 214, 0) && pending->edits().size() == 1 &&
			pending->edits().contains("blocked_shot.keyline"),
			"Resetting blocked highlight retains the independent nonblack zero-alpha keyline edit");
		unchanged();
		require(impl.execute("tune.reset presentation.blocked_shot.keyline").ok &&
			colour_values(presentation) == initial_colours && pending->edits().empty() && pending->current().text() == baseline,
			"Resetting blocked keyline restores both original authored tokens after rejected saves");
		unchanged();
		write_text(scratch.source / L"presentation.json", baseline);
		const JsonSpanDocument saved_highlight(baseline_document.replaced(
			*baseline_document.find("blocked_shot.highlight"), "\"2060C080\""), "blocked-colour-save-reference");
		const std::string saved = saved_highlight.replaced(*saved_highlight.find("blocked_shot.keyline"), "\"A532D600\"");
		require(impl.execute("tune presentation.blocked_shot.highlight 2060C080").ok &&
			impl.execute("tune presentation.blocked_shot.keyline A532D600").ok &&
			impl.execute("tune.save presentation").ok && read_text(scratch.source / L"presentation.json") == saved &&
			read_text(scratch.staged / L"presentation.json") == saved && pending->edits().empty() &&
			pending->baseline().text() == saved && pending->current().text() == saved &&
			look->highlight == labrador::Colour(32, 96, 192, 128) && look->keyline == labrador::Colour(165, 50, 214, 0),
			"Saving both blocked colours changes only the two authored tokens and rebases the retained cue's borrowed look");
		unchanged();
		require(std::rand() == expected_random,
			"Blocked colour edits, resets and valid/refused saves consume no gameplay random draw");
	}

	void check_idle_drip_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"idle-drip-colour-cues");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const std::array<WeaponType, 5> types{ WeaponType::sprayer, WeaponType::sniper,
			WeaponType::roller, WeaponType::mister, WeaponType::bucket };
		std::array<const IdleDripPresentation*, 5> looks{};
		std::array<IdleDripPresentation, 5> original{};
		for (std::size_t index = 0; index < types.size(); ++index)
		{
			looks[index] = &presentation.idle_drip(types[index]);
			original[index] = *looks[index];
			require(looks[index]->enabled == (index == 0 || index == 1 || index == 4),
				"Idle-drip colour fixtures retain the three eligible tools and disabled Roller/Mister profiles");
		}
		const auto geometry = [](const IdleDripPresentation& look)
		{
			return std::array<float, 11>{ look.interval_min, look.interval_max, look.grow_time,
				look.stretch_time, look.lifetime, look.gravity, look.size, look.stretch,
				look.nozzle_inset, look.nozzle_underside, look.keyline_width };
		};
		labrador::RenderResources resources;
		resources.add_sprite_sheet(original.front().sprite_sheet, labrador::read_sprite_sheet(
			(game_directory / L"textures" / (original.front().sprite_sheet + ".json")).string().c_str(), labrador::TextureHandle{}));
		// Separate consumers are necessary: the next quiet interval exceeds a
		// falling drop's lifetime. Do not pretend both states coexist by default.
		std::array<std::unique_ptr<IdleDrip>, 6> cues;
		const std::array<WeaponType, 3> eligible{ WeaponType::sprayer, WeaponType::sniper, WeaponType::bucket };
		for (std::size_t index = 0; index < cues.size(); ++index)
		{
			const IdleDripPresentation& look = presentation.idle_drip(eligible[index / 2]);
			cues[index] = std::make_unique<IdleDrip>(&look, &resources,
				index % 2 == 0 ? labrador::Colour(224, 48, 80) : labrador::Colour(48, 160, 224),
				static_cast<uint32_t>(index));
			IdleDrip& cue = *cues[index];
			const mattmath::Vector2F nozzle(160.0f + static_cast<float>(index) * 48.0f, 160.0f);
			cue.advance(true, nozzle, 0.0f, cue.interval_remaining() + 0.001f, {});
			cue.advance(true, nozzle, 0.0f, look.grow_time + look.stretch_time *
				(index % 2 == 0 ? 0.5f : 1.1f), {});
			if (index % 2 != 0) { cue.advance(false, nozzle, 0.0f, look.lifetime * 0.5f, {}); }
			require(cue.bounds() != mattmath::RectangleF::ZERO &&
				(index % 2 == 0 ? cue.attached() && cue.bead_age() > 0.0f && cue.drops().empty() :
					!cue.attached() && cue.drops().size() == 1 && cue.drops().front().age > 0.0f &&
					cue.drops().front().age < look.lifetime),
				"Each eligible tool has a real aged attached bead and a separate aged falling drop before colour adoption");
		}
		const auto state = [&]
		{
			std::vector<float> values;
			const auto append_rectangle = [&](const mattmath::RectangleF& rectangle)
				{ values.insert(values.end(), { rectangle.x, rectangle.y, rectangle.width, rectangle.height }); };
			for (const auto& cue : cues)
			{
				values.insert(values.end(), { cue->attached() ? 1.0f : 0.0f, cue->bead_age(),
					cue->interval_remaining(), static_cast<float>(cue->drops().size()) });
				append_rectangle(cue->bead_rectangle());
				append_rectangle(cue->bounds());
				for (const IdleDrip::Drop& drop : cue->drops())
					{ values.insert(values.end(), { drop.position.x, drop.position.y, drop.age }); }
			}
			return values;
		};
		const auto held_state = state();
		const auto unchanged = [&](const labrador::Colour& keyline)
		{
			for (std::size_t index = 0; index < types.size(); ++index)
			{
				const auto& look = presentation.idle_drip(types[index]);
				require(&look == looks[index] && look.keyline == keyline && look.enabled == original[index].enabled &&
					look.sprite_sheet == original[index].sprite_sheet && look.fill_frame == original[index].fill_frame &&
					geometry(look) == geometry(original[index]),
					"The one common keyline reaches all five stable borrowed records without changing eligibility, assets or tool geometry");
			}
			require(state() == held_state,
				"Idle-drip colour adoption retains every attached/falling age, interval, position, bead rectangle and cull bound");
		};
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const auto initial_colours = colour_values(presentation);
		std::srand(0x1d1e17);
		const int expected_random = std::rand();
		std::srand(0x1d1e17);
		for (const auto& [word, keyline] : std::array<std::pair<const char*, labrador::Colour>, 4>{ {
			{ "RED", { 255, 0, 0 } }, { "3165A7", { 49, 101, 167 } },
			{ "2060C080", { 32, 96, 192, 128 } }, { "A532D600", { 165, 50, 214, 0 } } } })
		{
			auto expected = initial_colours;
			expected[idle_drip_keyline_colour_index] = keyline;
			require(impl.execute(std::string("tune presentation.idle_drip.keyline ") + word).ok &&
				colour_values(presentation) == expected && read_text(scratch.source / L"presentation.json") == baseline &&
				read_text(scratch.staged / L"presentation.json") == baseline,
				"A preconstructed idle drip adopts straight named/RGB/partial/nonblack-zero-alpha keyline channels without writing content");
			unchanged(keyline);
		}
		const TuneDocument* const pending = impl.tune_document("presentation");
		require(impl.execute("tune presentation.debug_overlay.colour 789ABC").ok &&
			impl.execute("tune.reset presentation.idle_drip.keyline").ok && pending->edits().size() == 1 &&
			pending->edits().contains("debug_overlay.colour") && presentation.debug_overlay().colour == labrador::Colour(120, 154, 188),
			"The common idle-drip reset preserves an unrelated live colour edit");
		unchanged(original.front().keyline);
		require(impl.execute("tune.reset presentation.debug_overlay.colour").ok && pending->edits().empty() &&
			pending->current().text() == baseline && colour_values(presentation) == initial_colours,
			"Independent reset restores the exact original idle-drip document after retaining its active cues");
		const JsonSpanDocument baseline_document(baseline, "idle-drip-colour-save-reference");
		const std::string saved = baseline_document.replaced(*baseline_document.find("idle_drip.keyline"), "\"2060C080\"");
		require(impl.execute("tune presentation.idle_drip.keyline 2060C080").ok &&
			impl.execute("tune.save presentation").ok && read_text(scratch.source / L"presentation.json") == saved &&
			read_text(scratch.staged / L"presentation.json") == saved && pending->edits().empty() &&
			pending->baseline().text() == saved && pending->current().text() == saved,
			"Saving the common keyline changes one authored token and rebases all five retained tool profiles");
		unchanged(labrador::Colour(32, 96, 192, 128));
		require(impl.execute("tune presentation.idle_drip.keyline A532D600").ok,
			"An independent nonblack zero-alpha keyline is pending before invalid merged candidate saves");
		const JsonSpanDocument saved_document(saved, "idle-drip-colour-merged-reference");
		const JsonLeaf* interval_max = saved_document.find("idle_drip.tools.sniper.interval_max");
		require(interval_max != nullptr && original[1].interval_min > 0.001f,
			"The merged candidate has an unrelated real Sniper interval ordering constraint");
		const std::string external = saved_document.replaced(*interval_max, "0.001");
		write_text(scratch.source / L"presentation.json", external);
		const std::string pending_text = pending->current().text();
		const auto pending_colours = colour_values(presentation);
		for (const char* command : { "tune.save presentation", "tune.save presentation force" })
		{
			require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == saved && pending->current().text() == pending_text &&
				pending->baseline().text() == saved && pending->edits().size() == 1 &&
				pending->edits().contains("idle_drip.keyline") && colour_values(presentation) == pending_colours,
				"An invalid unrelated tool interval rejects the complete merged candidate and retains disk/staging/live/ledger, even with force");
			unchanged(labrador::Colour(165, 50, 214, 0));
		}
		require(impl.execute("tune.reset presentation.idle_drip.keyline").ok && pending->edits().empty() &&
			pending->current().text() == saved && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external && read_text(scratch.staged / L"presentation.json") == saved &&
			!std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
			!std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
			!std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
			"After rejected saves the common keyline resets to the saved baseline without discarding the external source or leaving recovery files");
		unchanged(labrador::Colour(32, 96, 192, 128));
		write_text(scratch.source / L"presentation.json", saved);
		require(std::rand() == expected_random,
			"Idle-drip colour edits, resets and valid/refused saves consume no gameplay random draw");
	}

	void check_respawn_pen_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"respawn-pen-colour");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		CpuWorld world(game_directory, tuning, presentation);
		TestDirector test(tuning, presentation);
		test.data.set_stages(&world.stages);
		test.data.set_level_infos(&world.levels);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const RespawnRedrawPresentation* const look = &presentation.respawn_redraw();
		const RespawnRedrawPresentation original = *look;
		RespawnRedraw pen(look, &world.render);
		const float middle = (look->draw_from + look->fill_at) * 0.5f;
		const std::array<float, 6> progress{ std::nextafter(look->draw_from, -1.0f),
			look->draw_from, middle, std::nextafter(look->fill_at, -1.0f),
			look->fill_at, std::nextafter(look->fill_at, 2.0f) };
		const auto queries = [&]
		{
			std::vector<bool> values;
			for (const float at : progress)
				{ values.push_back(pen.filled(at)); values.push_back(pen.drawing(at)); }
			return values;
		};
		const std::vector<bool> held_queries{ false, false, false, true, false, true,
			false, true, true, false, true, false };
		require(queries() == held_queries && pen.camera_hold() == original.camera_hold,
			"The preconstructed respawn pen pins drawing/fill queries immediately around both authored boundaries");
		// Reuse the existing real CPU Level fixture: prepare an actual dead
		// player once, then never tick it during colour commands or saves.
		MatchRequest roster;
		roster.stage = "close_quarters";
		roster.players = 2;
		const MatchPlan plan = MatchPlanner(&world.stages).resolve(nullptr, roster,
			MatchPlanner::ConnectedPads{}, [] { return 0u; });
		require(plan.ok, "The respawn pen's real CPU roster resolves");
		CpuMatch match(world, test.director);
		match.settings = plan.settings;
		match.settings.set_duration(60.0f);
		test.director.attach_match(match);
		match.build();
		const std::vector<PlayerInputData> idle(input_slots::count);
		const float dt = 1.0f / 60.0f;
		int preparation_updates = 0;
		while (preparation_updates < 1200 && match.level->state() != LevelState::active)
		{
			match.level->update(idle, dt);
			++preparation_updates;
		}
		require(match.level->state() == LevelState::active && match.level->kill_player(0),
			"Respawn pen fixture enters active play and requests an actual elimination");
		const Player& player = match.level->player(0);
		while (preparation_updates < 2400 &&
			(player.state() != PlayerState::dead || player.respawn_progress() < middle))
		{
			match.level->update(idle, dt);
			++preparation_updates;
		}
		require(player.state() == PlayerState::dead && player.respawn_timer() > 0.0f &&
			player.respawn_progress() >= middle && player.respawn_progress() < look->fill_at &&
			player.animation_state() != PlayerAnimationState::elimination && pen.drawing(player.respawn_progress()),
			"Respawn pen fixture freezes a real dead player in the outline phase after elimination");
		const auto player_state = [&]
		{
			std::vector<float> values{ player.health(), player.weapon_ammo(), player.respawn_timer(),
				player.respawn_progress(), player.velocity().x, player.velocity().y,
				static_cast<float>(player.state()), static_cast<float>(player.move_state()),
				static_cast<float>(player.animation_state()), player.action_age(),
				static_cast<float>(player.action_frame()), player.facing_right() ? 1.0f : 0.0f,
				player.drawn_whole() ? 1.0f : 0.0f, player.camera_focus().x, player.camera_focus().y };
			for (const auto& rectangle : { player.rectangle(), player.spawn_rectangle(),
				player.posed_rectangle(), player.bounds(), player.cull_bounds(1.0f) })
			{
				values.insert(values.end(), { rectangle.x, rectangle.y, rectangle.width, rectangle.height });
			}
			return values;
		};
		const auto held_player = player_state();
		const auto held_camera = player.camera();
		const MatchFingerprint held_match = MatchFingerprint::of(*match.level, preparation_updates);
		const auto unchanged = [&](const labrador::Colour& colour)
		{
			require(&presentation.respawn_redraw() == look && look->pen_colour == colour &&
				look->sprite_sheet == original.sprite_sheet && look->pen_frame == original.pen_frame &&
				look->canvas_size == original.canvas_size && look->pen_width == original.pen_width &&
				look->draw_from == original.draw_from && look->fill_at == original.fill_at &&
				look->camera_hold == original.camera_hold && look->strokes.size() == original.strokes.size() &&
				queries() == held_queries && pen.camera_hold() == original.camera_hold,
				"Respawn pen colour adoption retains the stable borrowed record, captured assets, geometry and boundary queries");
			for (std::size_t index = 0; index < original.strokes.size(); ++index)
			{
				require(look->strokes[index].part == original.strokes[index].part &&
					look->strokes[index].points == original.strokes[index].points,
					"Respawn pen colour adoption preserves every ordered stroke part and point value");
			}
			require(player_state() == held_player && player.camera() == held_camera && match.builds == 1 &&
				!impl.can_apply() && MatchFingerprint::difference(held_match,
					MatchFingerprint::of(*match.level, preparation_updates)).empty(),
				"Respawn pen commands preserve the frozen real player's timers, state, camera, figure/cull bounds and match fingerprint without queueing a rebuild");
		};
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const auto initial_colours = colour_values(presentation);
		std::srand(0x2e7a21);
		const int expected_random = std::rand();
		std::srand(0x2e7a21);
		for (const auto& [word, colour] : std::array<std::pair<const char*, labrador::Colour>, 4>{ {
			{ "RED", { 255, 0, 0 } }, { "3165A7", { 49, 101, 167 } },
			{ "2060C080", { 32, 96, 192, 128 } }, { "A532D600", { 165, 50, 214, 0 } } } })
		{
			auto expected = initial_colours;
			expected[respawn_pen_colour_index] = colour;
			require(impl.execute(std::string("tune presentation.respawn_redraw.pen_colour ") + word).ok &&
				colour_values(presentation) == expected && read_text(scratch.source / L"presentation.json") == baseline &&
				read_text(scratch.staged / L"presentation.json") == baseline,
				"A retained respawn pen accepts straight named/RGB/partial/nonblack-zero-alpha channels without writing content");
			unchanged(colour);
		}
		const TuneDocument* const pending = impl.tune_document("presentation");
		require(impl.execute("tune presentation.debug_overlay.colour 789ABC").ok &&
			impl.execute("tune.reset presentation.respawn_redraw.pen_colour").ok && pending->edits().size() == 1 &&
			pending->edits().contains("debug_overlay.colour") && presentation.debug_overlay().colour == labrador::Colour(120, 154, 188),
			"Resetting respawn pen colour retains an unrelated live colour edit");
		unchanged(original.pen_colour);
		require(impl.execute("tune.reset presentation.debug_overlay.colour").ok && pending->edits().empty() &&
			pending->current().text() == baseline && colour_values(presentation) == initial_colours,
			"Independent reset restores the exact original pen document while its actual dead player remains frozen");
		const JsonSpanDocument baseline_document(baseline, "respawn-pen-colour-save-reference");
		const std::string saved = baseline_document.replaced(*baseline_document.find("respawn_redraw.pen_colour"), "\"2060C080\"");
		require(impl.execute("tune presentation.respawn_redraw.pen_colour 2060C080").ok &&
			impl.execute("tune.save presentation").ok && read_text(scratch.source / L"presentation.json") == saved &&
			read_text(scratch.staged / L"presentation.json") == saved && pending->edits().empty() &&
			pending->baseline().text() == saved && pending->current().text() == saved,
			"Respawn pen save changes only its one authored token and rebases the retained consumer");
		unchanged(labrador::Colour(32, 96, 192, 128));
		require(impl.execute("tune presentation.respawn_redraw.pen_colour A532D600").ok,
			"A nonblack zero-alpha pen edit is pending before complete merged candidate refusal");
		const JsonSpanDocument saved_document(saved, "respawn-pen-colour-merged-reference");
		const JsonLeaf* draw_from = saved_document.find("respawn_redraw.draw_from");
		const JsonLeaf* fill_at = saved_document.find("respawn_redraw.fill_at");
		require(draw_from != nullptr && fill_at != nullptr && original.draw_from < original.fill_at,
			"The pen merged candidate has the actual unrelated draw/fill ordering constraint");
		const std::string external = saved_document.replaced(*draw_from, saved_document.token(*fill_at));
		write_text(scratch.source / L"presentation.json", external);
		const std::string pending_text = pending->current().text();
		const auto pending_colours = colour_values(presentation);
		for (const char* command : { "tune.save presentation", "tune.save presentation force" })
		{
			require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == saved && pending->current().text() == pending_text &&
				pending->baseline().text() == saved && pending->edits().size() == 1 &&
				pending->edits().contains("respawn_redraw.pen_colour") && colour_values(presentation) == pending_colours,
				"draw_from equal to fill_at rejects the complete merged pen save and retains disk/staging/live/ledger even with force");
			unchanged(labrador::Colour(165, 50, 214, 0));
		}
		require(impl.execute("tune.reset presentation.respawn_redraw.pen_colour").ok && pending->edits().empty() &&
			pending->current().text() == saved && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external && read_text(scratch.staged / L"presentation.json") == saved &&
			!std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
			!std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
			!std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
			"After refused saves the pen resets to its saved baseline without overwriting external source or leaving recovery files");
		unchanged(labrador::Colour(32, 96, 192, 128));
		write_text(scratch.source / L"presentation.json", saved);
		require(std::rand() == expected_random,
			"Respawn pen commands, resets and valid/refused saves consume no gameplay random draw");
		test.director.detach_match(match);
	}
