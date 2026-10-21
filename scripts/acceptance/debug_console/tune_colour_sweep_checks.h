// Included inside tune_checks.cpp's private namespace, after its includes.
// The audited colour-path sweep and the player identity ink/background tunes.

	void check_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"colours");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const std::string source_before = read_text(scratch.source / L"presentation.json");
		const std::string staged_before = read_text(scratch.staged / L"presentation.json");
		const auto initial = colour_values(presentation);
		const MovementFeedbackPresentation* const movement = &presentation.movement_feedback();
		const DebugOverlayPresentation* const overlay = &presentation.debug_overlay();
		const StationLabelPresentation* const station = &overlay->station_label;
		const SelectPresentation* const team = &presentation.main_menu().team_select;
		const SelectPresentation* const weapon = &presentation.main_menu().weapon_select;
		const WeaponSelectArtworkPresentation* const weapon_art = &presentation.main_menu().weapon_select_art;
		const char* const weapon_sheet = weapon_art->sheet.data();
		const char* const weapon_fill = weapon_art->fill_frame.data();
		const char* const weapon_frame = weapon_art->choices.front().frame.data();
		const WeaponFeedbackPresentation* const feedback = &presentation.weapon_feedback();
		const char* const feedback_sheet = feedback->sprite_sheet.data();
		const char* const feedback_fill = feedback->fill_frame.data();
		const CombatFeedbackPresentation* const combat_look = &presentation.combat_feedback();
		const char* const combat_sheet = combat_look->sprite_sheet.data();
		const char* const combat_fill = combat_look->fill_frame.data();
		const auto combat_shape = [&]
		{
			return std::array<float, 9>{ combat_look->hurt_duration, combat_look->hurt_repeat_delay,
				combat_look->hurt_width, combat_look->hurt_reach, combat_look->elimination_duration,
				combat_look->elimination_spread, combat_look->elimination_lift,
				combat_look->particle_size, combat_look->keyline_width };
		};
		const auto combat_geometry = combat_shape();
		const bool combat_enabled = combat_look->enabled;
		const PlayerIdentityPresentation* const identity = &presentation.player_identity();
		const std::string team_sheet = presentation.main_menu().team_select_sheet;
		const auto team_frames = presentation.main_menu().team_select_frames;
		const auto team_selected_frames = presentation.main_menu().team_select_selected_frames;
		const char* const font = overlay->font.data();
		const char* const sheet = movement->sprite_sheet.data();
		std::array<std::string, 44> words{ "RED", "112233", "a1B2c3", "00000080",
			"FFFFFF", "12345678", "654321", "BLUE", "10203040", "RED", "56789ABC", "35BCE1", "2266CC80",
			"123456FF", "DDEEFF", "73A1C280", "19B87000", "C26D3A80", "2A80D500", "93C74D80", "D973A680",
			"A147B980", "27C4A500", "5E91C780", "B86A4D40", "73C2A500", "D47C2A80", "61B2D440", "9C35E700", "3B97C180", "A65E2940", "93C74D80", "4CA7D980", "D6824F80", "A35CDE80", "31C8A640", "8E52CB80", "5EA3D680", "6EC19A80", "B67D2980", "0161C180", "4B8CD180", "5167B980", "C6A25D40" };
		std::vector<labrador::Colour> expected{ { 255, 0, 0 }, { 17, 34, 51 },
			{ 161, 178, 195 }, { 0, 0, 0, 128 }, { 255, 255, 255 }, { 18, 52, 86, 120 }, { 101, 67, 33 },
			{ 0, 0, 255 }, { 16, 32, 48, 64 }, { 255, 0, 0 }, { 86, 120, 154, 188 },
			{ 53, 188, 225 }, { 34, 102, 204, 128 }, { 18, 52, 86 }, { 221, 238, 255 },
			{ 115, 161, 194, 128 }, { 25, 184, 112, 0 }, { 194, 109, 58, 128 }, { 42, 128, 213, 0 },
			{ 147, 199, 77, 128 }, { 217, 115, 166, 128 }, { 161, 71, 185, 128 }, { 39, 196, 165, 0 },
			{ 94, 145, 199, 128 }, { 184, 106, 77, 64 }, { 115, 194, 165, 0 },
			{ 212, 124, 42, 128 }, { 97, 178, 212, 64 }, { 156, 53, 231, 0 }, { 59, 151, 193, 128 }, { 166, 94, 41, 64 },
			{ 147, 199, 77, 128 }, { 76, 167, 217, 128 }, { 214, 130, 79, 128 }, { 163, 92, 222, 128 }, { 49, 200, 166, 64 }, { 142, 82, 203, 128 }, { 94, 163, 214, 128 }, { 110, 193, 154, 128 }, { 182, 125, 41, 128 }, { 1, 97, 193, 128 }, { 75, 140, 209, 128 }, { 81, 103, 185, 128 }, { 198, 162, 93, 64 } };
		require(impl.execute("feedback off").ok, "Colour tuning starts with an independent live F9 override");
		for (std::size_t index = 0; index < colour_paths.size(); ++index)
		{
			const std::string path = colour_paths[index];
			if (impl.tune_token(path) == "\"" + words[index] + "\"")
			{
				// Identity colours have a stricter opaque/distinct contract. Keep
				// their fallbacks opaque and different from one another too.
				words[index] = index == 0 ? "BLUE" : index == identity_ink_colour_index ? "FEDCBAFF" :
					index == identity_background_colour_index ? "0F1E2DFF" : "FEDCBA98";
				expected[index] = index == 0 ? labrador::Colour(0, 0, 255) :
					index == identity_ink_colour_index ? labrador::Colour(254, 220, 186) :
					index == identity_background_colour_index ? labrador::Colour(15, 30, 45) : labrador::Colour(254, 220, 186, 152);
			}
			const std::string token = "\"" + words[index] + "\"";
			require(impl.execute("tune " + path + " " + words[index]).ok &&
				impl.tune_token(path) == token && colour_values(presentation)[index] == expected[index],
				"An audited colour accepts a named/RGB/RGBA word as one JSON string: " + path);
			require(impl.execute("tune " + path).ok &&
				log_contains(impl.log(), path + " = " + token, DebugSeverity::info),
				"A colour getter prints the exact quoted token: " + path);
		}
		for (const std::size_t index : { std::size_t(7), std::size_t(8), std::size_t(9), std::size_t(10),
			std::size_t(11), std::size_t(12), combat_highlight_colour_index, combat_keyline_colour_index,
			blocked_highlight_colour_index, blocked_keyline_colour_index, idle_drip_keyline_colour_index,
			respawn_pen_colour_index, cursor_ink_colour_index, cursor_fill_colour_index,
			silhouette_ink_colour_index, silhouette_rim_colour_index, silhouette_shade_colour_index,
			reveal_ink_colour_index, reveal_paper_colour_index, reveal_shade_colour_index, cursor_shade_colour_index, cursor_shadow_colour_index, desktop_colour_index, power_light_colour_index, program_colour_index, acting_face_colour_index, acting_sweat_colour_index, glass_colour_index, silhouette_body_colour_index, scribble_colour_index, hud_health_colour_index, hud_track_colour_index, crack_colour_index, limb_arm_colour_index, limb_hand_colour_index })
		{
			for (const auto& [word, colour] : std::array<std::pair<const char*, labrador::Colour>, 3>{ {
				{ "RED", { 255, 0, 0 } }, { "a1B2c3", { 161, 178, 195 } }, { "10203040", { 16, 32, 48, 64 } } } })
			{
				const std::string path = colour_paths[index];
				const std::string token = std::string("\"") + word + '"';
				expected[index] = colour;
				require(impl.execute("tune " + path + " " + word).ok && impl.tune_token(path) == token &&
					colour_values(presentation) == expected,
					"Each selection/feedback colour accepts named, RGB and RGBA values without changing the other colours: " + path);
			}
		}
		require(colour_values(presentation) == expected && !presentation.movement_feedback().enabled &&
			&presentation.movement_feedback() == movement && &presentation.debug_overlay() == overlay &&
			&presentation.debug_overlay().station_label == station && overlay->font.data() == font &&
			movement->sprite_sheet.data() == sheet && &presentation.main_menu().team_select == team &&
			presentation.main_menu().team_select_sheet == team_sheet &&
			presentation.main_menu().team_select_frames == team_frames &&
			presentation.main_menu().team_select_selected_frames == team_selected_frames &&
			&presentation.main_menu().weapon_select == weapon && &presentation.main_menu().weapon_select_art == weapon_art &&
			weapon_art->sheet.data() == weapon_sheet && weapon_art->fill_frame.data() == weapon_fill &&
			weapon_art->choices.front().frame.data() == weapon_frame && &presentation.weapon_feedback() == feedback &&
			feedback->sprite_sheet.data() == feedback_sheet && feedback->fill_frame.data() == feedback_fill &&
			&presentation.combat_feedback() == combat_look && combat_look->sprite_sheet.data() == combat_sheet &&
			combat_look->fill_frame.data() == combat_fill && combat_shape() == combat_geometry &&
			combat_look->enabled == combat_enabled &&
			&presentation.player_identity() == identity && identity->background_colour == expected[identity_background_colour_index],
			"Colour adoption keeps the borrowed records, unrelated string buffers and live F9 override");
		require(read_text(scratch.source / L"presentation.json") == source_before &&
			read_text(scratch.staged / L"presentation.json") == staged_before,
			"Live colour edits do not write either content copy");
		require(impl.execute("tune.diff presentation").ok, "Colour edits are listed by tune.diff");
		for (std::size_t index = 0; index < colour_paths.size(); ++index)
		{
			const std::string path = colour_paths[index];
			require(log_contains(impl.log(), "  " + path + "  ", DebugSeverity::info) &&
				tune_scope_of(path) == (index == 6 ? TuneScope::next_screen : TuneScope::live),
				"Each colour diff has its audited live/next-screen scope: " + path);
		}
		const auto reject = [&](const std::string& path, const std::string& value)
		{
			const auto before = colour_values(presentation);
			const std::string text = impl.tune_document("presentation")->current().text();
			const auto edits = impl.tune_document("presentation")->edits().size();
			require(!impl.execute("tune " + path + " " + quote_word(value)).ok &&
				colour_values(presentation) == before &&
				impl.tune_document("presentation")->current().text() == text &&
				impl.tune_document("presentation")->edits().size() == edits,
				"Rejected colour word/path keeps all live values and pending JSON: " + path + "=" + value);
		};
		for (const char* path : { colour_paths.front(), colour_paths[7], colour_paths[8], colour_paths[9], colour_paths[10],
			colour_paths[11], colour_paths[12], colour_paths[identity_ink_colour_index], colour_paths[identity_background_colour_index],
			colour_paths[combat_highlight_colour_index], colour_paths[combat_keyline_colour_index],
			colour_paths[blocked_highlight_colour_index], colour_paths[blocked_keyline_colour_index],
			colour_paths[idle_drip_keyline_colour_index], colour_paths[respawn_pen_colour_index],
			colour_paths[cursor_ink_colour_index], colour_paths[cursor_fill_colour_index],
			colour_paths[silhouette_ink_colour_index], colour_paths[silhouette_rim_colour_index],
			colour_paths[silhouette_shade_colour_index], colour_paths[reveal_ink_colour_index],
			colour_paths[reveal_paper_colour_index], colour_paths[reveal_shade_colour_index], colour_paths[cursor_shade_colour_index], colour_paths[cursor_shadow_colour_index], colour_paths[desktop_colour_index], colour_paths[power_light_colour_index], colour_paths[program_colour_index], colour_paths[acting_face_colour_index], colour_paths[acting_sweat_colour_index], colour_paths[glass_colour_index], colour_paths[silhouette_body_colour_index], colour_paths[scribble_colour_index], colour_paths[hud_health_colour_index], colour_paths[hud_track_colour_index], colour_paths[crack_colour_index], colour_paths[limb_arm_colour_index], colour_paths[limb_hand_colour_index] })
		{
			for (const char* value : { "next", "prev", "toggle", "on", "off", "true", "false", "+=1", "-=1",
				"#112233", "12345", "1234567", "GGGGGG", "not_a_colour", "white", "black", "\"RED\"", "RED\\u0000", "RED\\n",
				"RED\",\"enabled\":true,\"injected\":\"x", "RED;feedback on" })
			{
				reject(path, value);
			}
		}
		for (const char* path : { "presentation.debug_overlay.font", "presentation.movement_feedback.sprite_sheet",
			"presentation.main_menu.backgrounds.title", "presentation.movement_feedback.colour.r",
			"tuning.player.sound_bank", "presentation.main_menu.team_select.sheet",
			"presentation.main_menu.team_select.frames.a", "presentation.main_menu.team_select.selected_frames.a",
			"presentation.main_menu.team_select.selected.r", "presentation.main_menu.team_select.unselected_extra",
			"presentation.main_menu.team_select.selected_colour", "presentation.main_menu.weapon_select.sheet",
			"presentation.main_menu.weapon_select.fill_frame", "presentation.main_menu.weapon_select.choices.sprayer.frame",
			"presentation.main_menu.weapon_select.choices.sprayer.name", "presentation.main_menu.weapon_select.selected.r",
			"presentation.main_menu.weapon_select.selected_colour", "presentation.main_menu.weapon_select.unselected_extra",
			"presentation.weapon_feedback.sprite_sheet", "presentation.weapon_feedback.fill_frame",
			"presentation.weapon_feedback.highlight.r", "presentation.weapon_feedback.highlight_colour",
			"presentation.weapon_feedback.keyline_extra", "presentation.player_identity.background_colour.r",
			"presentation.player_identity.background_colour_extra", "presentation.player_identity.ink_colour.r",
			"presentation.player_identity.ink_colour_extra", "presentation.combat_feedback.sprite_sheet",
			"presentation.combat_feedback.fill_frame", "presentation.combat_feedback.highlight.r",
			"presentation.combat_feedback.highlight_colour", "presentation.combat_feedback.keyline_extra",
			"presentation.blocked_shot.sprite_sheet", "presentation.blocked_shot.fill_frame",
			"presentation.blocked_shot.highlight.r", "presentation.blocked_shot.highlight_colour",
			"presentation.blocked_shot.keyline_extra", "presentation.idle_drip.sprite_sheet",
			"presentation.idle_drip.fill_frame", "presentation.idle_drip.keyline.r",
			"presentation.idle_drip.keyline_extra", "presentation.idle_drip.tools.sprayer.keyline",
			"presentation.idle_drip.tools.sniper.keyline", "presentation.idle_drip.tools.bucket.keyline",
			"presentation.respawn_redraw.sprite_sheet", "presentation.respawn_redraw.pen_frame",
			"presentation.respawn_redraw.pen_colour.r", "presentation.respawn_redraw.pen_colour_extra",
			"presentation.respawn_redraw.strokes[0].part", "presentation.opening.sprite_sheet",
			"presentation.opening.fill_frame", "presentation.opening.cursor_art.ink.r",
			"presentation.opening.cursor_art.ink_colour", "presentation.opening.cursor_art.fill_extra",
			"presentation.opening.cursor_art.shade.r", "presentation.opening.cursor_art.shadow_extra",
			"presentation.opening.cursor_art.shade_extra", "presentation.opening.cursor_art.shadow.r",
			"opening.cursor_art.shade", "opening.cursor_art.shadow",
			"tuning.opening.cursor_art.shade", "tuning.opening.cursor_art.shadow",
			"presentation.opening.silhouette_extra", "presentation.opening.silhouette_art.ink.r",
			"presentation.opening.silhouette_art.rim_extra", "presentation.opening.silhouette_art.shade_colour",
			"presentation.opening.percent_font", "presentation.opening.reveal_art.ink.r",
			"presentation.opening.reveal_art.paper_extra", "presentation.opening.reveal_art.shade_colour",
			"opening.desktop_colour", "tuning.opening.desktop_colour", "presentation.opening.desktop_colour.r",
			"presentation.opening.desktop_colour_extra", "opening.monitor_art.power_light",
			"tuning.opening.monitor_art.power_light", "presentation.opening.monitor_art.power_light.r",
			"presentation.opening.monitor_art.power_light_extra", "presentation.opening.monitor_art.ink",
			"presentation.opening.monitor_art.highlight", "opening.program_colour", "tuning.opening.program_colour",
			"presentation.opening.program_colour.r", "presentation.opening.program_colour_extra",
			"opening.acting.face", "tuning.opening.acting.face", "presentation.opening.acting.face.r",
			"presentation.opening.acting.face_extra", "opening.acting.sweat", "tuning.opening.acting.sweat",
			"presentation.opening.acting.sweat.r", "presentation.opening.acting.sweat_extra",
			"opening.glass_colour", "tuning.opening.glass_colour", "presentation.opening.glass_colour.r",
			"presentation.opening.glass_colour_extra", "opening.silhouette", "tuning.opening.silhouette",
			"presentation.opening.silhouette.r", "opening.scribble", "tuning.opening.scribble",
			"presentation.opening.scribble.r", "presentation.opening.scribble_extra",
			"opening.crack", "tuning.opening.crack", "presentation.opening.crack.r", "presentation.opening.crack_extra",
            "figure.arm_colour", "tuning.figure.arm_colour", "presentation.figure.arm_colour.r", "presentation.figure.arm_colour_extra",
            "figure.hand_colour", "tuning.figure.hand_colour", "presentation.figure.hand_colour.r", "presentation.figure.hand_colour_extra",
            "presentation.figure.sprite_sheet", "presentation.figure.rest_arms_frame", "presentation.figure.rest_arms_paint_frame",
            "presentation.figure.sleeves_frame", "presentation.figure.sleeves_paint_frame", "presentation.figure.quiet.face_colour",
			"presentation.opening.sticky_notes[0]", "presentation.opening.stickers[0]",
			"hud.health_colour", "tuning.hud.health_colour", "presentation.hud.health_colour.r",
			"presentation.hud.health_colour_extra", "hud.meter_track_colour", "tuning.hud.meter_track_colour",
			"presentation.hud.meter_track_colour.r", "presentation.hud.meter_track_colour_extra",
			"presentation.hud.sprite_sheet", "presentation.hud.fill_frame", "presentation.hud.meter_label_font",
			"presentation.hud.timer.font", "presentation.hud.player_identity.font" })
		{
			require(impl.tune_token(path).empty() && !impl.execute(std::string("tune ") + path).ok,
				std::string("An arbitrary string is neither readable nor completed as tunable: ") + path);
			reject(path, "WHITE");
		}
		require(!presentation.movement_feedback().enabled, "A quoted injection cannot run a second console statement");
		require(impl.execute(std::string("tune.reset ") + colour_paths.front()).ok &&
			colour_values(presentation).front() == initial.front() &&
			colour_values(presentation)[identity_ink_colour_index] == expected[identity_ink_colour_index] &&
			colour_values(presentation)[identity_background_colour_index] == expected[identity_background_colour_index],
			"A colour path reset leaves both identity colour edits intact");
		require(impl.execute(std::string("tune.reset ") + colour_paths[7]).ok &&
			colour_values(presentation)[7] == initial[7] && colour_values(presentation)[8] == expected[8] &&
			impl.tune_document("presentation")->edits().count("main_menu.team_select.selected") == 0 &&
			impl.tune_document("presentation")->edits().count("main_menu.team_select.unselected") == 1,
			"Resetting the selected Team tint retains the unselected tint's independent edit");
		for (const std::size_t index : { std::size_t(9), std::size_t(10) })
		{
			const std::string path = colour_paths[index];
			const std::string local = path.substr(std::string("presentation.").size());
			auto restored = colour_values(presentation);
			restored[index] = initial[index];
			require(impl.tune_document("presentation")->edits().count(local) == 1 &&
				impl.execute("tune.reset " + path).ok && colour_values(presentation) == restored &&
				impl.tune_document("presentation")->edits().count(local) == 0 &&
				impl.tune_document("presentation")->edits().count("main_menu.team_select.unselected") == 1 &&
				(index != 9 || impl.tune_document("presentation")->edits().count("main_menu.weapon_select.unselected") == 1),
				"Each Weapon tint resets independently while every other pending colour remains intact: " + path);
		}
		for (const std::size_t index : { std::size_t(11), std::size_t(12),
			combat_highlight_colour_index, combat_keyline_colour_index,
			blocked_highlight_colour_index, blocked_keyline_colour_index, idle_drip_keyline_colour_index,
			respawn_pen_colour_index, cursor_ink_colour_index, cursor_fill_colour_index,
			silhouette_ink_colour_index, silhouette_rim_colour_index, silhouette_shade_colour_index,
			reveal_ink_colour_index, reveal_paper_colour_index, reveal_shade_colour_index, cursor_shade_colour_index, cursor_shadow_colour_index, desktop_colour_index, power_light_colour_index, program_colour_index, acting_face_colour_index, acting_sweat_colour_index, glass_colour_index, silhouette_body_colour_index, scribble_colour_index, hud_health_colour_index, hud_track_colour_index, crack_colour_index, limb_arm_colour_index, limb_hand_colour_index })
		{
			const std::string path = colour_paths[index];
			const std::string local = path.substr(std::string("presentation.").size());
			auto restored = colour_values(presentation);
			restored[index] = initial[index];
			require(impl.tune_document("presentation")->edits().count(local) == 1 &&
				impl.execute("tune.reset " + path).ok && colour_values(presentation) == restored &&
				impl.tune_document("presentation")->edits().count(local) == 0 &&
				impl.tune_document("presentation")->edits().count("main_menu.team_select.unselected") == 1 &&
				(index != 11 || impl.tune_document("presentation")->edits().count("weapon_feedback.keyline") == 1) &&
				(index != combat_highlight_colour_index ||
					impl.tune_document("presentation")->edits().count("combat_feedback.keyline") == 1) &&
				(index != blocked_highlight_colour_index ||
					impl.tune_document("presentation")->edits().count("blocked_shot.keyline") == 1) &&
				(index != cursor_ink_colour_index ||
					impl.tune_document("presentation")->edits().count("opening.cursor_art.fill") == 1) &&
				(index != silhouette_ink_colour_index ||
					(impl.tune_document("presentation")->edits().contains("opening.silhouette_art.rim") &&
						impl.tune_document("presentation")->edits().contains("opening.silhouette_art.shade"))) &&
				(index != silhouette_rim_colour_index ||
					impl.tune_document("presentation")->edits().contains("opening.silhouette_art.shade")) &&
				(index != reveal_ink_colour_index ||
					(impl.tune_document("presentation")->edits().contains("opening.reveal_art.paper") &&
						impl.tune_document("presentation")->edits().contains("opening.reveal_art.shade"))) &&
				(index != reveal_paper_colour_index ||
					impl.tune_document("presentation")->edits().contains("opening.reveal_art.shade")) &&
				&presentation.weapon_feedback() == feedback && feedback->sprite_sheet.data() == feedback_sheet &&
				feedback->fill_frame.data() == feedback_fill && &presentation.combat_feedback() == combat_look &&
				combat_look->sprite_sheet.data() == combat_sheet && combat_look->fill_frame.data() == combat_fill &&
				combat_shape() == combat_geometry && combat_look->enabled == combat_enabled,
				"Each feedback colour resets independently while other edits and captured bindings remain intact: " + path);
		}
		for (const std::size_t index : { identity_ink_colour_index, identity_background_colour_index })
		{
			const std::size_t sibling = index == identity_ink_colour_index ? identity_background_colour_index : identity_ink_colour_index;
			const std::string path = colour_paths[index];
			const std::string local = path.substr(std::string("presentation.").size());
			const std::string sibling_local = std::string(colour_paths[sibling]).substr(std::string("presentation.").size());
			const auto before_reset = colour_values(presentation);
			auto restored = before_reset;
			restored[index] = initial[index];
			require(impl.tune_document("presentation")->edits().count(local) == 1 &&
				impl.execute("tune.reset " + path).ok && colour_values(presentation) == restored &&
				impl.tune_document("presentation")->edits().count(local) == 0 &&
				impl.tune_document("presentation")->edits().count(sibling_local) == 1 &&
				impl.tune_document("presentation")->edits().count("main_menu.team_select.unselected") == 1 &&
				&presentation.player_identity() == identity,
				"Each identity reset preserves its sibling, borrowed record and unrelated pending tints: " + path);
			require(impl.execute("tune " + path + " " + words[index]).ok && colour_values(presentation) == before_reset,
				"An identity reset can be reapplied without changing its sibling or any other colour: " + path);
		}
		require(impl.execute("tune.reset presentation").ok && colour_values(presentation) == initial &&
			impl.tune_document("presentation")->edits().empty(), "A presentation reset restores every colour token");

		// The literal save oracle replaces one exact span in otherwise untouched
		// LF content. A subsequent external edit to another field must survive.
		for (const std::size_t index : { std::size_t(5), std::size_t(7), std::size_t(8), std::size_t(9), std::size_t(10),
			std::size_t(11), std::size_t(12), combat_highlight_colour_index, combat_keyline_colour_index,
			blocked_highlight_colour_index, blocked_keyline_colour_index, idle_drip_keyline_colour_index,
			respawn_pen_colour_index, cursor_ink_colour_index, cursor_fill_colour_index,
			silhouette_ink_colour_index, silhouette_rim_colour_index, silhouette_shade_colour_index,
			reveal_ink_colour_index, reveal_paper_colour_index, reveal_shade_colour_index, cursor_shade_colour_index, cursor_shadow_colour_index, desktop_colour_index, power_light_colour_index, program_colour_index, acting_face_colour_index, acting_sweat_colour_index, glass_colour_index, silhouette_body_colour_index, scribble_colour_index, hud_health_colour_index, hud_track_colour_index, crack_colour_index, limb_arm_colour_index, limb_hand_colour_index })
		{
			const std::string path = colour_paths[index];
			const std::string local = path.substr(std::string("presentation.").size());
			const std::string save_before = read_text(scratch.source / L"presentation.json");
			const JsonSpanDocument original(save_before, "colour-save-reference");
			const JsonLeaf* original_leaf = original.find(local);
			require(original_leaf != nullptr, "The colour-save reference has its audited string token: " + path);
			const std::string original_token(original.token(*original_leaf));
			std::string saved = save_before;
			saved.replace(original_leaf->begin, original_leaf->end - original_leaf->begin, "\"12345678\"");
			require(impl.execute("tune " + path + " 12345678").ok && impl.execute("tune.save presentation").ok &&
				read_text(scratch.source / L"presentation.json") == saved &&
				read_text(scratch.staged / L"presentation.json") == saved && saved.find('\r') == std::string::npos &&
				impl.tune_document("presentation")->edits().empty(),
				"Colour save changes exactly one quoted LF token, refreshes staging and rebases the edits");
			require(colour_values(Presentation((scratch.source / L"presentation.json").string().c_str()))[index] ==
				labrador::Colour(18, 52, 86, 120), "The saved RGBA token loads through the real Presentation reader");
			require(impl.execute("tune " + path + " ABCDEF").ok, "A second colour edit is pending before a disk conflict");
			const JsonSpanDocument disk(saved, "colour-save-reference");
			std::string external = disk.replaced(*disk.find(local), "\"FEDCBA\"");
			const JsonSpanDocument external_document(external, "colour-save-reference");
			const char* unrelated_word = index == 5 ? "102030" : index == 7 ? "203040" :
				index == 8 ? "304050" : index == 9 ? "405060" : index == 10 ? "506070" :
				index == 11 ? "607080" : index == 12 ? "708090" :
				index == combat_highlight_colour_index ? "8192A3" : index == combat_keyline_colour_index ? "92A3B4" :
				index == blocked_highlight_colour_index ? "A3B4C5" :
				index == blocked_keyline_colour_index ? "B4C5D6" :
				index == idle_drip_keyline_colour_index ? "C5D6E7" : index == respawn_pen_colour_index ? "D6E7F8" :
				index == cursor_ink_colour_index ? "E7F809" : index == cursor_fill_colour_index ? "F8091A" :
				index == silhouette_ink_colour_index ? "0A1B2C" :
				index == silhouette_rim_colour_index ? "1B2C3D" : index == silhouette_shade_colour_index ? "2C3D4E" :
				index == reveal_ink_colour_index ? "3D4E5F" : index == reveal_paper_colour_index ? "4E5F60" : index == reveal_shade_colour_index ? "5F6071" : index == cursor_shade_colour_index ? "607182" : index == cursor_shadow_colour_index ? "718293" : index == desktop_colour_index ? "8293A4" : index == power_light_colour_index ? "93A4B5" : index == program_colour_index ? "A4B5C6" : index == acting_face_colour_index ? "B5C6D7" : index == acting_sweat_colour_index ? "C6D7E8" : index == glass_colour_index ? "D7E8F9" : index == silhouette_body_colour_index ? "E8F90A" : index == scribble_colour_index ? "F90A1B" : index == hud_health_colour_index ? "0A1B2C" : index == hud_track_colour_index ? "1B2C3D" : index == limb_arm_colour_index ? "3D4E5F" : index == limb_hand_colour_index ? "4E5F60" : "2C3D4E";
			const labrador::Colour unrelated_colour(unrelated_word);
			require(presentation.debug_overlay().colour != unrelated_colour,
				"Each colour conflict must introduce a fresh unrelated external edit");
			external = external_document.replaced(*external_document.find("debug_overlay.colour"),
				std::string("\"") + unrelated_word + '"');
			write_text(scratch.source / L"presentation.json", external);
			require(!impl.execute("tune.save presentation").ok &&
				read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == saved &&
				impl.tune_token(path) == "\"ABCDEF\"",
				"A colour conflict leaves disk, staged baseline and our pending token intact");
			const JsonSpanDocument theirs(external, "colour-save-reference");
			const std::string forced = theirs.replaced(*theirs.find(local), "\"ABCDEF\"");
			require(impl.execute("tune.save presentation force").ok &&
				read_text(scratch.source / L"presentation.json") == forced &&
				read_text(scratch.staged / L"presentation.json") == forced &&
				presentation.debug_overlay().colour == unrelated_colour,
				"Forced colour save replaces only our conflict and adopts the unrelated external edit");
			require(!original_token.empty() && !std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
				!std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
				"Colour save leaves no temporary files beside either content copy");
		}
		for (const std::size_t index : { std::size_t(9), std::size_t(10), std::size_t(11), std::size_t(12),
			combat_highlight_colour_index, combat_keyline_colour_index,
			blocked_highlight_colour_index, blocked_keyline_colour_index, idle_drip_keyline_colour_index,
			respawn_pen_colour_index, cursor_ink_colour_index, cursor_fill_colour_index,
			silhouette_ink_colour_index, silhouette_rim_colour_index, silhouette_shade_colour_index,
			reveal_ink_colour_index, reveal_paper_colour_index, reveal_shade_colour_index, cursor_shade_colour_index, cursor_shadow_colour_index, desktop_colour_index, power_light_colour_index, program_colour_index, acting_face_colour_index, acting_sweat_colour_index, glass_colour_index, silhouette_body_colour_index, scribble_colour_index, hud_health_colour_index, hud_track_colour_index, crack_colour_index, limb_arm_colour_index, limb_hand_colour_index })
		{
			// Each new path also starts from an independent CRLF document, so
			// external adoption cannot pass by reusing an earlier case's value.
			const wchar_t* const scratch_name = index == 9 ? L"weapon-selected-crlf" :
				index == 10 ? L"weapon-unselected-crlf" : index == 11 ? L"feedback-highlight-crlf" :
				index == 12 ? L"feedback-keyline-crlf" : index == combat_highlight_colour_index ?
				L"combat-highlight-crlf" : index == combat_keyline_colour_index ? L"combat-keyline-crlf" :
				index == blocked_highlight_colour_index ? L"blocked-highlight-crlf" :
				index == blocked_keyline_colour_index ? L"blocked-keyline-crlf" :
				index == idle_drip_keyline_colour_index ? L"idle-drip-keyline-crlf" :
				index == respawn_pen_colour_index ? L"respawn-pen-crlf" :
				index == cursor_ink_colour_index ? L"cursor-ink-crlf" : index == cursor_fill_colour_index ? L"cursor-fill-crlf" :
				index == silhouette_ink_colour_index ? L"silhouette-ink-crlf" :
				index == silhouette_rim_colour_index ? L"silhouette-rim-crlf" :
				index == silhouette_shade_colour_index ? L"silhouette-shade-crlf" :
				index == reveal_ink_colour_index ? L"reveal-ink-crlf" :
				index == reveal_paper_colour_index ? L"reveal-paper-crlf" : index == reveal_shade_colour_index ? L"reveal-shade-crlf" : index == cursor_shade_colour_index ? L"cursor-shade-crlf" : index == cursor_shadow_colour_index ? L"cursor-shadow-crlf" : index == desktop_colour_index ? L"desktop-colour-crlf" : index == power_light_colour_index ? L"power-light-colour-crlf" : index == program_colour_index ? L"program-colour-crlf" : index == acting_face_colour_index ? L"acting-face-colour-crlf" : index == acting_sweat_colour_index ? L"acting-sweat-colour-crlf" : index == glass_colour_index ? L"glass-colour-crlf" : index == silhouette_body_colour_index ? L"silhouette-body-colour-crlf" : index == scribble_colour_index ? L"scribble-colour-crlf" : index == hud_health_colour_index ? L"hud-health-colour-crlf" : index == hud_track_colour_index ? L"hud-track-colour-crlf" : L"crack-colour-crlf";
			ScratchContent crlf_scratch(game_directory, scratch_name);
			std::string crlf;
			for (const char character : read_text(crlf_scratch.source / L"presentation.json"))
			{
				crlf += character == '\n' ? std::string("\r\n") : std::string(1, character);
			}
			for (const auto& directory : { crlf_scratch.source, crlf_scratch.staged })
				{ write_text(directory / L"presentation.json", crlf); }
			GameTuning crlf_tuning((crlf_scratch.staged / L"tuning.json").string().c_str());
			Presentation crlf_presentation((crlf_scratch.staged / L"presentation.json").string().c_str());
			TestDirector crlf_test(crlf_tuning, crlf_presentation);
			DebugDirectorImpl& crlf_impl = crlf_test.impl;
			crlf_impl.set_content_directories(crlf_scratch.source, crlf_scratch.staged);
			const std::string path = colour_paths[index];
			const std::string local = path.substr(std::string("presentation.").size());
			const auto original_values = colour_values(crlf_presentation);
			const JsonSpanDocument original(crlf, "selection-feedback-crlf-reference");
			const JsonLeaf* leaf = original.find(local);
			require(leaf != nullptr && original.token(*leaf) != "\"89ABCDEF\"" && count_of(crlf, "\r\n") > 0,
				"Each selection/feedback CRLF save begins with a different authored token: " + path);
			const std::string saved = original.replaced(*leaf, "\"89ABCDEF\"");
			require(crlf_impl.execute("tune " + path + " 89ABCDEF").ok && crlf_impl.execute("tune.save presentation").ok &&
				read_text(crlf_scratch.source / L"presentation.json") == saved &&
				read_text(crlf_scratch.staged / L"presentation.json") == saved &&
				crlf_impl.tune_document("presentation")->edits().empty() &&
				colour_values(Presentation((crlf_scratch.source / L"presentation.json").string().c_str()))[index] ==
					labrador::Colour(137, 171, 205, 239),
				"Selection/feedback save changes exactly one CRLF string token, loads RGBA and rebases staging: " + path);
			require(crlf_impl.execute("tune " + path + " ABCDEF").ok,
				"A selection/feedback CRLF edit is pending before its independent conflict: " + path);
			const JsonSpanDocument saved_document(saved, "weapon-crlf-reference");
			const std::string theirs = saved_document.replaced(*saved_document.find(local), "\"FEDCBA\"");
			const JsonSpanDocument theirs_document(theirs, "weapon-crlf-reference");
			const char* unrelated_word = index == 9 ? "607080" : index == 10 ? "708090" :
				index == 11 ? "8192A3" : index == 12 ? "92A3B4" :
				index == combat_highlight_colour_index ? "A3B4C5" : index == combat_keyline_colour_index ? "B4C5D6" :
				index == blocked_highlight_colour_index ? "C5D6E7" :
				index == blocked_keyline_colour_index ? "D6E7F8" :
				index == idle_drip_keyline_colour_index ? "E7F809" : index == respawn_pen_colour_index ? "F8091A" :
				index == cursor_ink_colour_index ? "091A2B" : index == cursor_fill_colour_index ? "1A2B3C" :
				index == silhouette_ink_colour_index ? "2B3C4D" :
				index == silhouette_rim_colour_index ? "3C4D5E" : index == silhouette_shade_colour_index ? "4D5E6F" :
				index == reveal_ink_colour_index ? "5E6F70" : index == reveal_paper_colour_index ? "6F7081" : index == reveal_shade_colour_index ? "708192" : index == cursor_shade_colour_index ? "8192A3" : index == cursor_shadow_colour_index ? "92A3B4" : index == desktop_colour_index ? "A3B4C5" : index == power_light_colour_index ? "B4C5D6" : index == program_colour_index ? "C5D6E7" : index == acting_face_colour_index ? "D6E7F8" : index == acting_sweat_colour_index ? "E7F809" : index == glass_colour_index ? "F8091A" : index == silhouette_body_colour_index ? "091A2B" : index == scribble_colour_index ? "1A2B3C" : index == hud_health_colour_index ? "2B3C4D" : index == hud_track_colour_index ? "3C4D5E" : "4D5E6F";
			const labrador::Colour unrelated_colour(unrelated_word);
			require(crlf_presentation.debug_overlay().colour != unrelated_colour &&
				theirs_document.token(*theirs_document.find("debug_overlay.colour")) != std::string("\"") + unrelated_word + '"',
				"Each selection/feedback CRLF conflict introduces a fresh live and disk unrelated colour: " + path);
			const std::string external = theirs_document.replaced(*theirs_document.find("debug_overlay.colour"),
				std::string("\"") + unrelated_word + '"');
			write_text(crlf_scratch.source / L"presentation.json", external);
			require(!crlf_impl.execute("tune.save presentation").ok &&
				read_text(crlf_scratch.source / L"presentation.json") == external &&
				read_text(crlf_scratch.staged / L"presentation.json") == saved && crlf_impl.tune_token(path) == "\"ABCDEF\"",
				"A selection/feedback CRLF conflict preserves disk, staging and the pending token: " + path);
			const JsonSpanDocument external_document(external, "weapon-crlf-reference");
			const std::string forced = external_document.replaced(*external_document.find(local), "\"ABCDEF\"");
			auto adopted = original_values;
			adopted[index] = labrador::Colour(171, 205, 239);
			adopted[3] = unrelated_colour;
			require(crlf_impl.execute("tune.save presentation force").ok &&
				read_text(crlf_scratch.source / L"presentation.json") == forced &&
				read_text(crlf_scratch.staged / L"presentation.json") == forced && colour_values(crlf_presentation) == adopted &&
				crlf_impl.tune_document("presentation")->current().text() == forced &&
				crlf_impl.tune_document("presentation")->edits().empty() &&
				count_of(forced, "\r\n") == count_of(crlf, "\r\n") && count_of(forced, "\n") == count_of(crlf, "\r\n"),
				"Force saves only the selection/feedback conflict, adopts the fresh external colour and preserves every CRLF: " + path);
			require(crlf_impl.execute("tune " + path + " RED").ok && crlf_impl.execute("tune.reset " + path).ok &&
				crlf_impl.tune_document("presentation")->current().text() == forced &&
				colour_values(crlf_presentation) == adopted && crlf_impl.tune_document("presentation")->edits().empty() &&
				!std::filesystem::exists(crlf_scratch.source / L"presentation.json.tmp") &&
				!std::filesystem::exists(crlf_scratch.staged / L"presentation.json.tmp"),
				"A selection/feedback CRLF reset uses the saved baseline and leaves no temporary files: " + path);
		}
	}

	void check_identity_colour_tunes(const std::filesystem::path& game_directory)
	{
		for (const bool background : { false, true })
		{
			const std::size_t index = background ? identity_background_colour_index : identity_ink_colour_index;
			const std::size_t sibling_index = background ? identity_ink_colour_index : identity_background_colour_index;
			const std::string path = colour_paths[index];
			const std::string local = path.substr(std::string("presentation.").size());
			const std::string sibling_path = colour_paths[sibling_index];
			const std::string sibling_local = sibling_path.substr(std::string("presentation.").size());
			const auto value_of = [background](const PlayerIdentityPresentation& look)
				{ return background ? look.background_colour : look.ink_colour; };
			const auto sibling_of = [background](const PlayerIdentityPresentation& look)
				{ return background ? look.ink_colour : look.background_colour; };
			for (const bool crlf : { false, true })
			{
				ScratchContent scratch(game_directory, background ?
					(crlf ? L"identity-background-crlf" : L"identity-background-lf") :
					(crlf ? L"identity-ink-crlf" : L"identity-ink-lf"));
				const auto source = scratch.source / L"presentation.json";
				const auto staged = scratch.staged / L"presentation.json";
				std::string baseline = read_text(source);
				if (crlf)
				{
					std::string converted;
					for (const char character : baseline)
						{ converted += character == '\n' ? std::string("\r\n") : std::string(1, character); }
					baseline = std::move(converted);
					write_text(source, baseline);
					write_text(staged, baseline);
				}
				GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
				Presentation presentation(staged.string().c_str());
				TestDirector test(tuning, presentation);
				DebugDirectorImpl& impl = test.impl;
				impl.set_content_directories(scratch.source, scratch.staged);
				const PlayerIdentityPresentation* const identity = &presentation.player_identity();
				const auto initial = colour_values(presentation);
				const auto state = [&] { return colour_values(presentation); };
				const auto edits = [&]
				{
					std::vector<std::string> values;
					for (const auto& [key, edit] : impl.tune_document("presentation")->edits())
						{ values.push_back(key + "=" + edit.baseline_token + "->" + edit.current_token); }
					return values;
				};
				for (const auto& [word, colour] : std::array<std::pair<const char*, labrador::Colour>, 3>{ {
					{ "RED", { 255, 0, 0 } }, { "a1B2c3", { 161, 178, 195 } }, { "102030FF", { 16, 32, 48 } } } })
				{
					auto expected = initial;
					expected[index] = colour;
					require(impl.execute("tune " + path + " " + word).ok && colour_values(presentation) == expected &&
						&presentation.player_identity() == identity && impl.tune_token(path) == std::string("\"") + word + '"',
						"Identity colour accepts named/RGB/opaque RGBA on its borrowed record without changing other colours: " + path);
				}
				const JsonSpanDocument authored(baseline, "identity-colour-reference");
				const std::string sibling_token(authored.token(*authored.find(sibling_local)));
				for (const std::string& word : { std::string("11223380"), std::string("11223300"),
					sibling_token.substr(1, sibling_token.size() - 2) })
				{
					const auto before_state = state();
					const auto before_edits = edits();
					const std::string before_text = impl.tune_document("presentation")->current().text();
					require(!impl.execute("tune " + path + " " + word).ok && state() == before_state && edits() == before_edits &&
						impl.tune_document("presentation")->current().text() == before_text &&
						read_text(source) == baseline && read_text(staged) == baseline,
						"Translucent, invisible or sibling-matching identity colour preserves live values, pending tokens and both files: " + path);
				}
				require(impl.execute("tune.reset " + path).ok && colour_values(presentation) == initial &&
					impl.tune_document("presentation")->current().text() == baseline && edits().empty(),
					"Identity colour resets exactly to its authored token without a rebuild or file write: " + path);

				// Both identity leaves can be pending at once. Validate against the
				// currently edited sibling, and reset one without resetting the other.
				require(initial[3] != labrador::Colour(32, 48, 64) &&
					impl.execute("tune " + path + " RED").ok && impl.execute("tune " + sibling_path + " BLUE").ok &&
					impl.execute("tune presentation.debug_overlay.colour 203040").ok,
					"Both distinct opaque identity fields and an unrelated colour can be pending together: " + path);
				const auto paired_state = state();
				const auto paired_edits = edits();
				const std::string paired_text = impl.tune_document("presentation")->current().text();
				require(!impl.execute("tune " + path + " BLUE").ok && state() == paired_state && edits() == paired_edits &&
					impl.tune_document("presentation")->current().text() == paired_text &&
					read_text(source) == baseline && read_text(staged) == baseline,
					"Identity equality is checked against the pending sibling and refusal is atomic: " + path);
				auto target_reset = paired_state;
				target_reset[index] = initial[index];
				require(impl.execute("tune.reset " + path).ok && state() == target_reset &&
					impl.tune_document("presentation")->edits().count(local) == 0 &&
					impl.tune_document("presentation")->edits().count(sibling_local) == 1 &&
					impl.tune_document("presentation")->edits().count("debug_overlay.colour") == 1,
					"Target identity reset preserves its pending sibling and unrelated colour: " + path);
				require(impl.execute("tune " + path + " RED").ok && impl.execute("tune.reset " + sibling_path).ok,
					"A pending identity edit permits an independent sibling reset: " + path);
				auto sibling_reset = paired_state;
				sibling_reset[sibling_index] = initial[sibling_index];
				require(state() == sibling_reset && impl.tune_document("presentation")->edits().count(local) == 1 &&
					impl.tune_document("presentation")->edits().count(sibling_local) == 0 &&
					impl.tune_document("presentation")->edits().count("debug_overlay.colour") == 1 &&
					impl.execute("tune.reset presentation").ok && state() == initial && edits().empty() &&
					impl.tune_document("presentation")->current().text() == baseline &&
					read_text(source) == baseline && read_text(staged) == baseline,
					"Sibling reset preserves target and unrelated edits; full reset restores exact authored identity bytes: " + path);

				// Saving merges a fresh unrelated disk edit, splices only our colour,
				// then adopts and rebases the full validated file.
				const std::string external = authored.replaced(*authored.find("debug_overlay.colour"), "\"203040\"");
				require(initial[3] != labrador::Colour(32, 48, 64), "The identity save introduces a fresh unrelated external colour");
				write_text(source, external);
				const JsonSpanDocument external_document(external, "identity-colour-reference");
				const std::string saved = external_document.replaced(*external_document.find(local), "\"123456FF\"");
				auto saved_values = initial;
				saved_values[3] = labrador::Colour(32, 48, 64);
				saved_values[index] = labrador::Colour(18, 52, 86);
				require(impl.execute("tune " + path + " 123456FF").ok && impl.execute("tune.save presentation").ok &&
					read_text(source) == saved && read_text(staged) == saved && colour_values(presentation) == saved_values &&
					impl.tune_document("presentation")->baseline().text() == saved && edits().empty() &&
					colour_values(Presentation(source.string().c_str())) == saved_values &&
					count_of(saved, "\r\n") == count_of(baseline, "\r\n") && count_of(saved, "\n") == count_of(baseline, "\n"),
					"Identity save changes only its quoted LF/CRLF token, preserves the external edit, loads and rebases both copies: " + path);
				require(impl.execute("tune " + path + " a1B2c3").ok && impl.execute("tune.reset " + path).ok &&
					colour_values(presentation) == saved_values && impl.tune_document("presentation")->current().text() == saved && edits().empty(),
					"Identity reset after save restores the saved opaque spelling and adopted external baseline: " + path);

				const JsonSpanDocument saved_document(saved, "identity-colour-conflict");
				const std::string conflicting = saved_document.replaced(*saved_document.find(local), "\"FEDCBA\"");
				write_text(source, conflicting);
				require(impl.execute("tune " + path + " ABCDEF").ok, "Identity colour is pending before an on-disk token conflict: " + path);
				const auto conflict_state = state();
				const auto conflict_edits = edits();
				const std::string conflict_text = impl.tune_document("presentation")->current().text();
				require(!impl.execute("tune.save presentation").ok && read_text(source) == conflicting && read_text(staged) == saved &&
					state() == conflict_state && edits() == conflict_edits && impl.tune_document("presentation")->current().text() == conflict_text,
					"An identity colour token conflict preserves source, staging, live colours and the complete pending edit: " + path);
				const JsonSpanDocument conflicting_document(conflicting, "identity-colour-conflict");
				const std::string forced = conflicting_document.replaced(*conflicting_document.find(local), "\"ABCDEF\"");
				require(impl.execute("tune.save presentation force").ok && read_text(source) == forced && read_text(staged) == forced &&
					value_of(*identity) == labrador::Colour(171, 205, 239) && impl.tune_document("presentation")->baseline().text() == forced && edits().empty(),
					"Force resolves only the identity token conflict and rebases the validated opaque colour: " + path);

				// Each file is valid independently. Pending RED ink + external RED
				// background, and the inverse, must fail only at the merged candidate.
				require(impl.execute("tune " + path + " RED").ok, "A distinct red identity colour is pending before the coupled disk conflict: " + path);
				const JsonSpanDocument forced_document(forced, "identity-colour-merged-conflict");
				const std::string coupled = forced_document.replaced(*forced_document.find(sibling_local), "\"RED\"");
				write_text(source, coupled);
				require(sibling_of(Presentation(source.string().c_str()).player_identity()) == labrador::Colour::red &&
					sibling_of(*identity) != labrador::Colour::red, "The external red sibling is valid before merging and is not yet live: " + path);
				const auto coupled_state = state();
				const auto coupled_edits = edits();
				const std::string coupled_text = impl.tune_document("presentation")->current().text();
				for (const char* command : { "tune.save presentation", "tune.save presentation force" })
				{
					impl.log().clear();
					require(!impl.execute(command).ok && read_text(source) == coupled && read_text(staged) == forced &&
						state() == coupled_state && edits() == coupled_edits && impl.tune_document("presentation")->current().text() == coupled_text &&
						impl.tune_document("presentation")->baseline().text() == forced &&
						log_contains(impl.log(), "the written file did not load", DebugSeverity::error) &&
						log_contains(impl.log(), "; the old bytes are back", DebugSeverity::error),
						"Invalid merged identity colours restore the exact external LF/CRLF source and preserve staged/live/ledger state, even with force: " + path);
				}
				require(impl.execute("tune.reset " + path).ok && value_of(*identity) == labrador::Colour(171, 205, 239) &&
					read_text(source) == coupled && read_text(staged) == forced && edits().empty() &&
					!std::filesystem::exists(source.string() + ".tmp") && !std::filesystem::exists(source.string() + ".bak") &&
					!std::filesystem::exists(staged.string() + ".tmp"),
					"After rejected merged saves, reset still uses the saved baseline without discarding the external source or leaving recovery files: " + path);
			}
		}
	}
