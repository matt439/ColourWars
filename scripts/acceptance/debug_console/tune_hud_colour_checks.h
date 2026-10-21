// Included inside tune_checks.cpp's private namespace, after its includes.
// HUD meter health and track colours.

	void check_hud_health_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"hud-health-colour-records");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const HudPresentation* const look = &presentation.hud();
		const HudPaperPresentation* const paper = &look->paper;
		const ClosingClockPresentation* const clock = &look->closing_clock;
		const PlayerIdentityPresentation* const identity = &presentation.player_identity();
		const HudPresentation original = *look;
		const PlayerIdentityPresentation original_identity = *identity;
		labrador::RenderResources resources;
		std::vector<std::string> sheets{ original.sprite_sheet };
		if (original.sprite_sheet != "sprite_sheet_1") { sheets.push_back("sprite_sheet_1"); }
		for (const std::string& sheet : sheets)
		{
			resources.add_sprite_sheet(sheet, labrador::read_sprite_sheet(
				(game_directory / L"textures" / (sheet + ".json")).string().c_str(), labrador::TextureHandle{}));
		}
		const std::array<std::string, 3> fonts{ original.meter_label_font,
			original.timer_font, original.player_identity_font };
		for (const std::string& font : fonts)
		{
			const labrador::SpriteFontFile file = labrador::read_sprite_font_file(
				(game_directory / L"fonts" / (font + ".spritefont")).string());
			auto loaded = std::make_unique<labrador::Font>(labrador::TextureHandle{}, file.glyphs, file.line_spacing);
			if (loaded->contains(U'?')) { loaded->set_stand_in(U'?'); }
			resources.add_font(font, std::move(loaded));
		}
		const InterfaceGameplay hud(&resources, look, identity);
		const InterfaceGameplay* const retained_hud = &hud;
		const auto* const sheet = resources.sprite_sheet(original.sprite_sheet);
		const auto fill = sheet->resolve_sprite_frame(original.fill_frame);
		const auto fill_source = sheet->sprite_frame(fill).source_rectangle();
		const auto fill_origin = sheet->sprite_frame(fill).origin();
		const auto font_handles = [&]
		{
			std::vector<labrador::FontHandle> result;
			for (const auto& font : fonts) { result.push_back(resources.resolve_sprite_font(font)); }
			return result;
		};
		const auto font_measurements = [&]
		{
			return std::vector<mattmath::Vector2F>{
				resources.measure_text(resources.resolve_sprite_font(original.meter_label_font), L"HEALTH"),
				resources.measure_text(resources.resolve_sprite_font(original.timer_font), L"02:04"),
				resources.measure_text(resources.resolve_sprite_font(original.player_identity_font), L"P1  /  TEAM A\nKeyboard + mouse") };
		};
		const auto geometry = [&]
		{
			return std::vector<float>{ look->paper.keyline_width, look->paper.shadow_offset,
				look->meter_label_gap, look->reference_height, look->right_margin,
				look->ammo_top_margin, look->health_top_margin, look->box_size.x, look->box_size.y,
				look->box_fill_border, look->timer_offset.x, look->timer_offset.y,
				look->timer_shadow_offset.x, look->timer_shadow_offset.y, look->timer_reference_width,
				look->closing_clock.keyline_width, look->closing_clock.shadow_offset,
				look->closing_clock.tick_reach, look->closing_clock.beat_duration,
				look->player_identity_offset.x, look->player_identity_offset.y,
				look->player_identity_shadow_offset.x, look->player_identity_shadow_offset.y,
				look->player_identity_padding.x, look->player_identity_padding.y,
				look->player_identity_reference_width };
		};
		const auto unselected = [&]
		{
			return std::vector<labrador::Colour>{ look->paper.ink, look->paper.paper,
				look->paper.shade, look->meter_track_colour, look->timer_colour, look->timer_shadow_colour,
				look->closing_clock.ink, look->closing_clock.paper, look->closing_clock.shade,
				look->closing_clock.text, look->closing_clock.accent, look->player_identity_colour,
				look->player_identity_shadow_colour, look->player_identity_background_colour,
				identity->ink_colour, identity->background_colour };
		};
		const auto resource_names = [&]
		{
			return std::vector<std::string>{ look->sprite_sheet, look->fill_frame,
				look->meter_label_font, look->timer_font, look->player_identity_font };
		};
		const auto identity_geometry = [&]
		{
			return std::vector<float>{ identity->enabled ? 1.0f : 0.0f,
				identity->world_size.x, identity->world_size.y, identity->world_offset.x, identity->world_offset.y,
				identity->emphasis_duration, identity->emphasis_scale, identity->ui_size.x,
				identity->ui_size.y, identity->ui_gap, identity->border };
		};
		const auto held_geometry = geometry();
		const auto held_unselected = unselected();
		const auto held_names = resource_names();
		const auto held_identity = identity_geometry();
		const auto held_fonts = font_handles();
		const auto held_measurements = font_measurements();
		require(original.health_colour == labrador::Colour(118, 232, 181) &&
			original.meter_track_colour == labrador::Colour(184, 184, 184) &&
			original.paper.keyline_width <= 4.0f && original_identity.ink_colour.a == 1.0f &&
			held_measurements[0].x > 0.0f && held_measurements[1].x > 0.0f && held_measurements[2].x > 0.0f,
			"The retained actual CPU HUD loads the authored health/track defaults and real label/timer/identity font metadata");
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const std::string tuning_source = read_text(scratch.source / L"tuning.json");
		const std::string tuning_staged = read_text(scratch.staged / L"tuning.json");
		const auto unchanged = [&]
		{
			require(retained_hud == &hud && &presentation.hud() == look && &look->paper == paper &&
				&look->closing_clock == clock && &presentation.player_identity() == identity &&
				geometry() == held_geometry && unselected() == held_unselected &&
				resource_names() == held_names && identity_geometry() == held_identity &&
				resources.sprite_sheet(look->sprite_sheet) == sheet &&
				sheet->resolve_sprite_frame(look->fill_frame) == fill &&
				sheet->sprite_frame(fill).source_rectangle() == fill_source &&
				sheet->sprite_frame(fill).origin() == fill_origin && font_handles() == held_fonts &&
				font_measurements() == held_measurements && !impl.in_match() &&
				!impl.can_apply() && impl.request().empty() &&
				read_text(scratch.source / L"tuning.json") == tuning_source &&
				read_text(scratch.staged / L"tuning.json") == tuning_staged,
				"Health adoption retains the actual HUD, borrowed full HUD/identity records, every unselected material/numeric/resource input and public sheet/frame/font lookups, without match work");
		};
		// The retained real HUD resolves engine sheet/font metadata with null
		// texture handles. CPU observes borrowed records and public lookups,
		// not its private captured handles, pixels, or a Player/Level draw.
		const auto initial_colours = colour_values(presentation);
		std::srand(0x4ea140);
		const int expected_random = std::rand();
		std::srand(0x4ea140);
		for (const auto& [word, expected] : std::array<std::pair<const char*, labrador::Colour>, 4>{ {
			{ "RED", { 255, 0, 0 } }, { "3165A7", { 49, 101, 167 } },
			{ "2060C080", { 32, 96, 192, 128 } }, { "A532D600", { 165, 50, 214, 0 } } } })
		{
			auto expected_colours = initial_colours;
			expected_colours[hud_health_colour_index] = expected;
			require(impl.execute(std::string("tune presentation.hud.health_colour ") + word).ok &&
				colour_values(presentation) == expected_colours && look->health_colour == expected &&
				read_text(scratch.source / L"presentation.json") == baseline &&
				read_text(scratch.staged / L"presentation.json") == baseline,
				"Health accepts straight named/RGB/asymmetric partial and nonblack zero-alpha channels without changing another colour or content copy");
			unchanged();
		}
		const TuneDocument* const pending = impl.tune_document("presentation");
		require(impl.execute("tune presentation.debug_overlay.colour 789ABC").ok &&
			impl.execute("tune.reset presentation.hud.health_colour").ok &&
			look->health_colour == original.health_colour && pending->edits().size() == 1 &&
			pending->edits().contains("debug_overlay.colour") &&
			presentation.debug_overlay().colour == labrador::Colour(120, 154, 188),
			"Health resets independently to the original tint while retaining an unrelated live colour edit");
		unchanged();
		require(impl.execute("tune.reset presentation.debug_overlay.colour").ok &&
			colour_values(presentation) == initial_colours && pending->edits().empty() &&
			pending->current().text() == baseline,
			"Independent reset restores the initial document with the same retained HUD and resource lookups");
		const JsonSpanDocument baseline_document(baseline, "hud-health-colour-save-reference");
		const JsonLeaf* health_leaf = baseline_document.find("hud.health_colour");
		require(health_leaf != nullptr, "The actual authored health token exists");
		const std::string saved = baseline_document.replaced(*health_leaf, "\"2060C080\"");
		require(impl.execute("tune presentation.hud.health_colour 2060C080").ok &&
			impl.execute("tune.save presentation").ok && look->health_colour == labrador::Colour(32, 96, 192, 128) &&
			read_text(scratch.source / L"presentation.json") == saved &&
			read_text(scratch.staged / L"presentation.json") == saved && pending->edits().empty() &&
			pending->baseline().text() == saved && pending->current().text() == saved,
			"Health save replaces exactly one quoted token and rebases the retained borrowed HUD record");
		unchanged();
		require(impl.execute("tune presentation.hud.health_colour A532D600").ok &&
			impl.execute("tune presentation.debug_overlay.colour 789ABC").ok &&
			pending->edits().size() == 2 && pending->edits().contains("hud.health_colour") &&
			pending->edits().contains("debug_overlay.colour"),
			"A fresh health edit and an unrelated supported colour are both pending before merged-reader refusal");
		const JsonSpanDocument saved_document(saved, "hud-health-colour-merged-reference");
		const JsonLeaf* keyline = saved_document.find("hud.paper.keyline_width");
		require(keyline != nullptr && original.paper.keyline_width <= 4.0f,
			"Health merge fixture uses the existing ordinary HUD paper keyline-width bound");
		const std::string external = saved_document.replaced(*keyline, "4.1");
		write_text(scratch.source / L"presentation.json", external);
		const std::string pending_text = pending->current().text();
		const auto pending_colours = colour_values(presentation);
		const auto ledger = [&]
		{
			std::vector<std::array<std::string, 3>> result;
			for (const auto& [path, edit] : pending->edits())
				{ result.push_back({ path, edit.baseline_token, edit.current_token }); }
			return result;
		};
		const auto held_ledger = ledger();
		for (const char* command : { "tune.save presentation", "tune.save presentation force" })
		{
			require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == saved &&
				pending->current().text() == pending_text && pending->baseline().text() == saved &&
				ledger() == held_ledger && pending->edits().size() == 2 &&
				pending->edits().contains("hud.health_colour") && pending->edits().contains("debug_overlay.colour") &&
				colour_values(presentation) == pending_colours,
				"External hud.paper.keyline_width 4.1 refuses the complete ordinary reader even with force, retaining exact source/staged/live/current/baseline and BOTH ledger keys plus every baseline/current token");
			unchanged();
		}
		const std::string overlay_only = saved_document.replaced(*saved_document.find("debug_overlay.colour"), "\"789ABC\"");
		auto restored_colours = pending_colours;
		restored_colours[hud_health_colour_index] = labrador::Colour(32, 96, 192, 128);
		require(impl.execute("tune.reset presentation.hud.health_colour").ok &&
			colour_values(presentation) == restored_colours && pending->edits().size() == 1 &&
			pending->edits().contains("debug_overlay.colour") && pending->current().text() == overlay_only &&
			pending->baseline().text() == saved && read_text(scratch.source / L"presentation.json") == external &&
			read_text(scratch.staged / L"presentation.json") == saved &&
			!std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
			!std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
			!std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
			"Health resets to its saved partial-alpha token while retaining the unrelated pending edit and exact invalid external source/staging without recovery files");
		unchanged();
		restored_colours[3] = initial_colours[3];
		require(impl.execute("tune.reset presentation.debug_overlay.colour").ok &&
			colour_values(presentation) == restored_colours && pending->edits().empty() &&
			pending->current().text() == saved && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external &&
			read_text(scratch.staged / L"presentation.json") == saved,
			"Resetting the unrelated colour retains the saved health baseline and exact external source/staging");
		unchanged();
		write_text(scratch.source / L"presentation.json", saved);
		require(std::rand() == expected_random,
			"Health colour commands, independent/saved resets and valid/refused saves consume no gameplay random draw");
	}

	void check_hud_track_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"hud-track-colour-records");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const HudPresentation* const look = &presentation.hud();
		const HudPaperPresentation* const paper = &look->paper;
		const ClosingClockPresentation* const clock = &look->closing_clock;
		const PlayerIdentityPresentation* const identity = &presentation.player_identity();
		const HudPresentation original = *look;
		const PlayerIdentityPresentation original_identity = *identity;
		labrador::RenderResources resources;
		std::vector<std::string> sheets{ original.sprite_sheet };
		if (original.sprite_sheet != "sprite_sheet_1") { sheets.push_back("sprite_sheet_1"); }
		for (const std::string& sheet : sheets)
		{
			resources.add_sprite_sheet(sheet, labrador::read_sprite_sheet(
				(game_directory / L"textures" / (sheet + ".json")).string().c_str(), labrador::TextureHandle{}));
		}
		const std::array<std::string, 3> fonts{ original.meter_label_font,
			original.timer_font, original.player_identity_font };
		for (const std::string& font : fonts)
		{
			const labrador::SpriteFontFile file = labrador::read_sprite_font_file(
				(game_directory / L"fonts" / (font + ".spritefont")).string());
			auto loaded = std::make_unique<labrador::Font>(labrador::TextureHandle{}, file.glyphs, file.line_spacing);
			if (loaded->contains(U'?')) { loaded->set_stand_in(U'?'); }
			resources.add_font(font, std::move(loaded));
		}
		const InterfaceGameplay hud(&resources, look, identity);
		const InterfaceGameplay* const retained_hud = &hud;
		const auto* const sheet = resources.sprite_sheet(original.sprite_sheet);
		const auto fill = sheet->resolve_sprite_frame(original.fill_frame);
		const auto fill_source = sheet->sprite_frame(fill).source_rectangle();
		const auto fill_origin = sheet->sprite_frame(fill).origin();
		const auto font_handles = [&]
		{
			std::vector<labrador::FontHandle> result;
			for (const auto& font : fonts) { result.push_back(resources.resolve_sprite_font(font)); }
			return result;
		};
		const auto font_measurements = [&]
		{
			return std::vector<mattmath::Vector2F>{
				resources.measure_text(resources.resolve_sprite_font(original.meter_label_font), L"HEALTH"),
				resources.measure_text(resources.resolve_sprite_font(original.timer_font), L"02:04"),
				resources.measure_text(resources.resolve_sprite_font(original.player_identity_font), L"P1  /  TEAM A\nKeyboard + mouse") };
		};
		const auto geometry = [&]
		{
			return std::vector<float>{ look->paper.keyline_width, look->paper.shadow_offset,
				look->meter_label_gap, look->reference_height, look->right_margin,
				look->ammo_top_margin, look->health_top_margin, look->box_size.x, look->box_size.y,
				look->box_fill_border, look->timer_offset.x, look->timer_offset.y,
				look->timer_shadow_offset.x, look->timer_shadow_offset.y, look->timer_reference_width,
				look->closing_clock.keyline_width, look->closing_clock.shadow_offset,
				look->closing_clock.tick_reach, look->closing_clock.beat_duration,
				look->player_identity_offset.x, look->player_identity_offset.y,
				look->player_identity_shadow_offset.x, look->player_identity_shadow_offset.y,
				look->player_identity_padding.x, look->player_identity_padding.y,
				look->player_identity_reference_width };
		};
		const auto unselected = [&]
		{
			return std::vector<labrador::Colour>{ look->paper.ink, look->paper.paper,
				look->paper.shade, look->health_colour, look->timer_colour, look->timer_shadow_colour,
				look->closing_clock.ink, look->closing_clock.paper, look->closing_clock.shade,
				look->closing_clock.text, look->closing_clock.accent, look->player_identity_colour,
				look->player_identity_shadow_colour, look->player_identity_background_colour,
				identity->ink_colour, identity->background_colour };
		};
		const auto resource_names = [&]
		{
			return std::vector<std::string>{ look->sprite_sheet, look->fill_frame,
				look->meter_label_font, look->timer_font, look->player_identity_font };
		};
		const auto identity_geometry = [&]
		{
			return std::vector<float>{ identity->enabled ? 1.0f : 0.0f,
				identity->world_size.x, identity->world_size.y, identity->world_offset.x, identity->world_offset.y,
				identity->emphasis_duration, identity->emphasis_scale, identity->ui_size.x,
				identity->ui_size.y, identity->ui_gap, identity->border };
		};
		const auto held_geometry = geometry();
		const auto held_unselected = unselected();
		const auto held_names = resource_names();
		const auto held_identity = identity_geometry();
		const auto held_fonts = font_handles();
		const auto held_measurements = font_measurements();
		require(original.health_colour == labrador::Colour(118, 232, 181) &&
			original.meter_track_colour == labrador::Colour(184, 184, 184) &&
			original.paper.keyline_width <= 4.0f && original_identity.ink_colour.a == 1.0f &&
			held_measurements[0].x > 0.0f && held_measurements[1].x > 0.0f && held_measurements[2].x > 0.0f,
			"The retained actual CPU HUD loads the authored health/track defaults and real label/timer/identity font metadata");
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const std::string tuning_source = read_text(scratch.source / L"tuning.json");
		const std::string tuning_staged = read_text(scratch.staged / L"tuning.json");
		const auto unchanged = [&]
		{
			require(retained_hud == &hud && &presentation.hud() == look && &look->paper == paper &&
				&look->closing_clock == clock && &presentation.player_identity() == identity &&
				geometry() == held_geometry && unselected() == held_unselected &&
				resource_names() == held_names && identity_geometry() == held_identity &&
				resources.sprite_sheet(look->sprite_sheet) == sheet &&
				sheet->resolve_sprite_frame(look->fill_frame) == fill &&
				sheet->sprite_frame(fill).source_rectangle() == fill_source &&
				sheet->sprite_frame(fill).origin() == fill_origin && font_handles() == held_fonts &&
				font_measurements() == held_measurements && !impl.in_match() &&
				!impl.can_apply() && impl.request().empty() &&
				read_text(scratch.source / L"tuning.json") == tuning_source &&
				read_text(scratch.staged / L"tuning.json") == tuning_staged,
				"Track adoption retains the actual HUD, borrowed full HUD/identity records, every unselected material/numeric/resource input and public sheet/frame/font lookups, without match work");
		};
		// The retained real HUD resolves engine sheet/font metadata with null
		// texture handles. CPU observes borrowed records and public lookups,
		// not its private captured handles, pixels, or a Player/Level draw.
		const auto initial_colours = colour_values(presentation);
		std::srand(0x4ea141);
		const int expected_random = std::rand();
		std::srand(0x4ea141);
		for (const auto& [word, expected] : std::array<std::pair<const char*, labrador::Colour>, 4>{ {
			{ "RED", { 255, 0, 0 } }, { "3165A7", { 49, 101, 167 } },
			{ "2060C080", { 32, 96, 192, 128 } }, { "A532D600", { 165, 50, 214, 0 } } } })
		{
			auto expected_colours = initial_colours;
			expected_colours[hud_track_colour_index] = expected;
			require(impl.execute(std::string("tune presentation.hud.meter_track_colour ") + word).ok &&
				colour_values(presentation) == expected_colours && look->meter_track_colour == expected &&
				read_text(scratch.source / L"presentation.json") == baseline &&
				read_text(scratch.staged / L"presentation.json") == baseline,
				"Track accepts straight named/RGB/asymmetric partial and nonblack zero-alpha channels without changing another colour or content copy");
			unchanged();
		}
		const TuneDocument* const pending = impl.tune_document("presentation");
		require(impl.execute("tune presentation.debug_overlay.colour 789ABC").ok &&
			impl.execute("tune.reset presentation.hud.meter_track_colour").ok &&
			look->meter_track_colour == original.meter_track_colour && pending->edits().size() == 1 &&
			pending->edits().contains("debug_overlay.colour") &&
			presentation.debug_overlay().colour == labrador::Colour(120, 154, 188),
			"Track resets independently to the original tint while retaining an unrelated live colour edit");
		unchanged();
		require(impl.execute("tune.reset presentation.debug_overlay.colour").ok &&
			colour_values(presentation) == initial_colours && pending->edits().empty() &&
			pending->current().text() == baseline,
			"Independent reset restores the initial document with the same retained HUD and resource lookups");
		const JsonSpanDocument baseline_document(baseline, "hud-track-colour-save-reference");
		const JsonLeaf* track_leaf = baseline_document.find("hud.meter_track_colour");
		require(track_leaf != nullptr, "The actual authored track token exists");
		const std::string saved = baseline_document.replaced(*track_leaf, "\"2060C080\"");
		require(impl.execute("tune presentation.hud.meter_track_colour 2060C080").ok &&
			impl.execute("tune.save presentation").ok && look->meter_track_colour == labrador::Colour(32, 96, 192, 128) &&
			read_text(scratch.source / L"presentation.json") == saved &&
			read_text(scratch.staged / L"presentation.json") == saved && pending->edits().empty() &&
			pending->baseline().text() == saved && pending->current().text() == saved,
			"Track save replaces exactly one quoted token and rebases the retained borrowed HUD record");
		unchanged();
		require(impl.execute("tune presentation.hud.meter_track_colour A532D600").ok &&
			impl.execute("tune presentation.debug_overlay.colour 789ABC").ok &&
			pending->edits().size() == 2 && pending->edits().contains("hud.meter_track_colour") &&
			pending->edits().contains("debug_overlay.colour"),
			"A fresh track edit and an unrelated supported colour are both pending before merged-reader refusal");
		const JsonSpanDocument saved_document(saved, "hud-track-colour-merged-reference");
		const JsonLeaf* keyline = saved_document.find("hud.paper.keyline_width");
		require(keyline != nullptr && original.paper.keyline_width <= 4.0f,
			"Track merge fixture uses the existing ordinary HUD paper keyline-width bound");
		const std::string external = saved_document.replaced(*keyline, "4.1");
		write_text(scratch.source / L"presentation.json", external);
		const std::string pending_text = pending->current().text();
		const auto pending_colours = colour_values(presentation);
		const auto ledger = [&]
		{
			std::vector<std::array<std::string, 3>> result;
			for (const auto& [path, edit] : pending->edits())
				{ result.push_back({ path, edit.baseline_token, edit.current_token }); }
			return result;
		};
		const auto held_ledger = ledger();
		for (const char* command : { "tune.save presentation", "tune.save presentation force" })
		{
			require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == saved &&
				pending->current().text() == pending_text && pending->baseline().text() == saved &&
				ledger() == held_ledger && pending->edits().size() == 2 &&
				pending->edits().contains("hud.meter_track_colour") && pending->edits().contains("debug_overlay.colour") &&
				colour_values(presentation) == pending_colours,
				"External hud.paper.keyline_width 4.1 refuses the complete ordinary reader even with force, retaining exact source/staged/live/current/baseline and BOTH ledger keys plus every baseline/current token");
			unchanged();
		}
		const std::string overlay_only = saved_document.replaced(*saved_document.find("debug_overlay.colour"), "\"789ABC\"");
		auto restored_colours = pending_colours;
		restored_colours[hud_track_colour_index] = labrador::Colour(32, 96, 192, 128);
		require(impl.execute("tune.reset presentation.hud.meter_track_colour").ok &&
			colour_values(presentation) == restored_colours && pending->edits().size() == 1 &&
			pending->edits().contains("debug_overlay.colour") && pending->current().text() == overlay_only &&
			pending->baseline().text() == saved && read_text(scratch.source / L"presentation.json") == external &&
			read_text(scratch.staged / L"presentation.json") == saved &&
			!std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
			!std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
			!std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
			"Track resets to its saved partial-alpha token while retaining the unrelated pending edit and exact invalid external source/staging without recovery files");
		unchanged();
		restored_colours[3] = initial_colours[3];
		require(impl.execute("tune.reset presentation.debug_overlay.colour").ok &&
			colour_values(presentation) == restored_colours && pending->edits().empty() &&
			pending->current().text() == saved && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external &&
			read_text(scratch.staged / L"presentation.json") == saved,
			"Resetting the unrelated colour retains the saved track baseline and exact external source/staging");
		unchanged();
		write_text(scratch.source / L"presentation.json", saved);
		require(std::rand() == expected_random,
			"Track colour commands, independent/saved resets and valid/refused saves consume no gameplay random draw");
	}
