// Included inside tune_checks.cpp's private namespace, after its includes.
// The opening monitor and desktop scene colours.

	void check_desktop_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"desktop-colour-records");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const OpeningPresentation* const opening = &presentation.opening();
		const OpeningPresentation original = *opening;
		labrador::RenderResources resources;
		for (const std::string& sheet_name : { opening->sprite_sheet, opening->palette_sheet })
		{
			resources.add_sprite_sheet(sheet_name, labrador::read_sprite_sheet(
				(game_directory / L"textures" / (sheet_name + ".json")).string().c_str(), labrador::TextureHandle{}));
		}
		MonitorPersona persona;
		persona.mug = true;
		persona.coffee_ring = mattmath::Vector2F(0.85f, 0.9f);
		persona.sticky_notes.push_back({ mattmath::Vector2F(0.015f, 0.12f), 0, 0.1f });
		persona.stickers.push_back({ mattmath::Vector2F(0.2f, 0.9f), 1, 0.0f });
		persona.crack = { mattmath::Vector2F(0.004f, 0.6f), mattmath::Vector2F(0.02f, 0.62f) };
		const Monitor monitor(opening, &resources, persona);
		const MonitorPersona* const captured_persona = &monitor.persona();
		const mattmath::RectangleF screen(0, 0, 1280, 720);
		const auto geometry = [&]
		{
			return std::vector<float>{ opening->cursor_art.shadow_offset, opening->cursor_art.click_duration,
				opening->cursor_art.click_reach, opening->silhouette_art.pen_width, opening->silhouette_art.shade_width,
				opening->palette_height, opening->adult_height, opening->child_height, opening->cursor_size,
				opening->preview_strength, opening->percent_height, opening->hop, opening->shake, opening->slump,
				opening->monitor_art.pen_width, opening->monitor_art.corner_step, opening->monitor_art.shadow_width,
				static_cast<float>(opening->monitor_art.vent_count), opening->reveal_art.keyline_width,
				opening->reveal_art.padding, opening->reveal_art.shadow_offset,
				opening->reveal_art.tick_length, opening->reveal_art.beat_duration };
		};
		const auto unselected = [&]
		{
			return std::vector<labrador::Colour>{ opening->cursor_art.ink, opening->cursor_art.fill,
				opening->cursor_art.shade, opening->cursor_art.shadow, opening->silhouette,
				opening->silhouette_art.ink, opening->silhouette_art.rim, opening->silhouette_art.shade,
				opening->bezel_shadow, opening->monitor_art.ink, opening->monitor_art.highlight,
				opening->monitor_art.power_light, opening->glass_colour, opening->program_colour,
				opening->slot_outline, opening->scribble, opening->coffee, opening->crack,
				opening->reveal_art.ink, opening->reveal_art.paper, opening->reveal_art.shade,
				opening->figure_grey, opening->face, opening->sweat };
		};
		const auto rectangles = [&]
		{
			return std::array<mattmath::RectangleF, 10>{ opening->glass, opening->monitor_art.vent_area,
				opening->monitor_art.power_button, opening->monitor_art.power_indicator,
				monitor.glass(screen), monitor.picture(screen), monitor.swatch(screen, 0), monitor.swatch(screen, 1),
				monitor.empty_slot(screen), monitor.close_button(screen) };
		};
		const auto persona_state = [&]
		{
			const auto& held = monitor.persona();
			std::vector<float> values{ static_cast<float>(held.shade), held.child ? 1.0f : 0.0f,
				held.mug ? 1.0f : 0.0f, held.coffee_ring.x, held.coffee_ring.y };
			for (const auto& decorations : { held.sticky_notes, held.stickers })
			{
				values.push_back(static_cast<float>(decorations.size()));
				for (const auto& decoration : decorations)
					{ values.insert(values.end(), { decoration.at.x, decoration.at.y, static_cast<float>(decoration.colour), decoration.tilt }); }
			}
			values.push_back(static_cast<float>(held.crack.size()));
			for (const auto& point : held.crack) { values.insert(values.end(), { point.x, point.y }); }
			return values;
		};
		const auto held_geometry = geometry();
		const auto held_unselected = unselected();
		const auto held_rectangles = rectangles();
		const auto held_persona = persona_state();
		const auto unchanged = [&]
		{
			require(&presentation.opening() == opening && &monitor.persona() == captured_persona &&
				geometry() == held_geometry && unselected() == held_unselected && rectangles() == held_rectangles &&
				persona_state() == held_persona && opening->bezel_shades == original.bezel_shades &&
				opening->sticky_notes == original.sticky_notes && opening->stickers == original.stickers &&
				opening->sprite_sheet == original.sprite_sheet && opening->fill_frame == original.fill_frame &&
				opening->percent_font == original.percent_font && opening->palette_sheet == original.palette_sheet &&
				opening->palette_frames == original.palette_frames,
				"Desktop adoption retains the borrowed opening, captured persona, public monitor/swatch geometry, all unselected material/numeric records and resource names");
		};
		// The real retained Monitor is constructed without GPU resources or RNG.
		// This CPU check observes records and public geometry, not rendered pixels
		// or protected captured frame handles. Native acceptance covers the draw.
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const auto initial_colours = colour_values(presentation);
		std::srand(0xde5c32);
		const int expected_random = std::rand();
		std::srand(0xde5c32);
		for (const auto& [word, expected] : std::array<std::pair<const char*, labrador::Colour>, 4>{ {
			{ "RED", { 255, 0, 0 } }, { "3165A7", { 49, 101, 167 } },
			{ "2060C080", { 32, 96, 192, 128 } }, { "A532D600", { 165, 50, 214, 0 } } } })
		{
			auto expected_colours = initial_colours;
			expected_colours[desktop_colour_index] = expected;
			require(impl.execute(std::string("tune presentation.opening.desktop_colour ") + word).ok &&
				colour_values(presentation) == expected_colours && read_text(scratch.source / L"presentation.json") == baseline &&
				read_text(scratch.staged / L"presentation.json") == baseline,
				"Desktop accepts straight named/RGB/partial/nonblack-zero-alpha channels while retaining every unrelated colour and both content copies");
			unchanged();
		}
		const TuneDocument* const pending = impl.tune_document("presentation");
		require(impl.execute("tune presentation.debug_overlay.colour 789ABC").ok &&
			impl.execute("tune.reset presentation.opening.desktop_colour").ok && opening->desktop_colour == original.desktop_colour &&
			presentation.debug_overlay().colour == labrador::Colour(120, 154, 188) && pending->edits().size() == 1 &&
			pending->edits().contains("debug_overlay.colour"),
			"Desktop reset restores its original tint while retaining an unrelated live colour edit");
		unchanged();
		require(impl.execute("tune.reset presentation.debug_overlay.colour").ok && colour_values(presentation) == initial_colours &&
			pending->edits().empty() && pending->current().text() == baseline,
			"Independent reset restores the exact initial document after retaining the constructed Monitor");
		const JsonSpanDocument baseline_document(baseline, "desktop-colour-save-reference");
		const std::string saved = baseline_document.replaced(*baseline_document.find("opening.desktop_colour"), "\"2060C080\"");
		require(impl.execute("tune presentation.opening.desktop_colour 2060C080").ok && impl.execute("tune.save presentation").ok &&
			opening->desktop_colour == labrador::Colour(32, 96, 192, 128) && read_text(scratch.source / L"presentation.json") == saved &&
			read_text(scratch.staged / L"presentation.json") == saved && pending->edits().empty() &&
			pending->baseline().text() == saved && pending->current().text() == saved,
			"Desktop save changes exactly one quoted token and rebases the retained borrowed record");
		unchanged();
		require(impl.execute("tune presentation.opening.desktop_colour A532D600").ok,
			"A fresh nonblack alpha0 desktop edit is pending before invalid merged geometry");
		const JsonSpanDocument saved_document(saved, "desktop-colour-merged-reference");
		const JsonLeaf* glass_width = saved_document.find("opening.glass.width");
		require(glass_width != nullptr && original.glass.x == 0.1f && original.glass.x + 0.95f > 1.0f,
			"Desktop merge fixture has the real cross-field glass-on-screen constraint");
		const std::string external = saved_document.replaced(*glass_width, "0.95");
		write_text(scratch.source / L"presentation.json", external);
		const std::string pending_text = pending->current().text();
		const auto pending_colours = colour_values(presentation);
		for (const char* command : { "tune.save presentation", "tune.save presentation force" })
		{
			require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == saved && pending->current().text() == pending_text &&
				pending->baseline().text() == saved && pending->edits().size() == 1 &&
				pending->edits().contains("opening.desktop_colour") && colour_values(presentation) == pending_colours,
				"External glass.width 0.95 with x 0.1 rejects the complete merged candidate and retains exact source/staged/live/current/baseline/ledger, even with force");
			unchanged();
		}
		require(impl.execute("tune.reset presentation.opening.desktop_colour").ok &&
			opening->desktop_colour == labrador::Colour(32, 96, 192, 128) && pending->edits().empty() &&
			pending->current().text() == saved && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external && read_text(scratch.staged / L"presentation.json") == saved &&
			!std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
			!std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
			!std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
			"Desktop resets to its saved partial-alpha baseline without discarding the external source or leaving recovery files");
		unchanged();
		write_text(scratch.source / L"presentation.json", saved);
		require(std::rand() == expected_random,
			"Desktop edits, independent resets and valid/refused saves consume no gameplay random draw");
	}

	void check_power_light_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"power-light-colour-records");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const OpeningPresentation* const opening = &presentation.opening();
		const OpeningPresentation original = *opening;
		const MonitorArtPresentation* const monitor_art = &opening->monitor_art;
		labrador::RenderResources resources;
		for (const std::string& sheet_name : { opening->sprite_sheet, opening->palette_sheet })
		{
			resources.add_sprite_sheet(sheet_name, labrador::read_sprite_sheet(
				(game_directory / L"textures" / (sheet_name + ".json")).string().c_str(), labrador::TextureHandle{}));
		}
		MonitorPersona persona;
		persona.mug = true;
		persona.coffee_ring = mattmath::Vector2F(0.85f, 0.9f);
		persona.sticky_notes.push_back({ mattmath::Vector2F(0.015f, 0.12f), 0, 0.1f });
		persona.stickers.push_back({ mattmath::Vector2F(0.2f, 0.9f), 1, 0.0f });
		persona.crack = { mattmath::Vector2F(0.004f, 0.6f), mattmath::Vector2F(0.02f, 0.62f) };
		const Monitor monitor(opening, &resources, persona);
		const MonitorPersona* const captured_persona = &monitor.persona();
		const mattmath::RectangleF screen(0, 0, 1280, 720);
		const auto geometry = [&]
		{
			return std::vector<float>{ opening->cursor_art.shadow_offset, opening->cursor_art.click_duration,
				opening->cursor_art.click_reach, opening->silhouette_art.pen_width, opening->silhouette_art.shade_width,
				opening->palette_height, opening->adult_height, opening->child_height, opening->cursor_size,
				opening->preview_strength, opening->percent_height, opening->hop, opening->shake, opening->slump,
				opening->monitor_art.pen_width, opening->monitor_art.corner_step, opening->monitor_art.shadow_width,
				static_cast<float>(opening->monitor_art.vent_count), opening->reveal_art.keyline_width,
				opening->reveal_art.padding, opening->reveal_art.shadow_offset,
				opening->reveal_art.tick_length, opening->reveal_art.beat_duration };
		};
		const auto unselected = [&]
		{
			return std::vector<labrador::Colour>{ opening->cursor_art.ink, opening->cursor_art.fill,
				opening->cursor_art.shade, opening->cursor_art.shadow, opening->silhouette,
				opening->silhouette_art.ink, opening->silhouette_art.rim, opening->silhouette_art.shade,
				opening->bezel_shadow, opening->monitor_art.ink, opening->monitor_art.highlight,
				opening->desktop_colour, opening->glass_colour, opening->program_colour,
				opening->slot_outline, opening->scribble, opening->coffee, opening->crack,
				opening->reveal_art.ink, opening->reveal_art.paper, opening->reveal_art.shade,
				opening->figure_grey, opening->face, opening->sweat };
		};
		const auto rectangles = [&]
		{
			return std::array<mattmath::RectangleF, 10>{ opening->glass, opening->monitor_art.vent_area,
				opening->monitor_art.power_button, opening->monitor_art.power_indicator,
				monitor.glass(screen), monitor.picture(screen), monitor.swatch(screen, 0), monitor.swatch(screen, 1),
				monitor.empty_slot(screen), monitor.close_button(screen) };
		};
		const auto persona_state = [&]
		{
			const auto& held = monitor.persona();
			std::vector<float> values{ static_cast<float>(held.shade), held.child ? 1.0f : 0.0f,
				held.mug ? 1.0f : 0.0f, held.coffee_ring.x, held.coffee_ring.y };
			for (const auto& decorations : { held.sticky_notes, held.stickers })
			{
				values.push_back(static_cast<float>(decorations.size()));
				for (const auto& decoration : decorations)
					{ values.insert(values.end(), { decoration.at.x, decoration.at.y, static_cast<float>(decoration.colour), decoration.tilt }); }
			}
			values.push_back(static_cast<float>(held.crack.size()));
			for (const auto& point : held.crack) { values.insert(values.end(), { point.x, point.y }); }
			return values;
		};
		const auto held_geometry = geometry();
		const auto held_unselected = unselected();
		const auto held_rectangles = rectangles();
		const auto held_persona = persona_state();
		const auto unchanged = [&]
		{
			require(&presentation.opening() == opening && &opening->monitor_art == monitor_art && &monitor.persona() == captured_persona &&
				geometry() == held_geometry && unselected() == held_unselected && rectangles() == held_rectangles &&
				persona_state() == held_persona && opening->bezel_shades == original.bezel_shades &&
				opening->sticky_notes == original.sticky_notes && opening->stickers == original.stickers &&
				opening->sprite_sheet == original.sprite_sheet && opening->fill_frame == original.fill_frame &&
				opening->percent_font == original.percent_font && opening->palette_sheet == original.palette_sheet &&
				opening->palette_frames == original.palette_frames,
				"Power-light adoption retains the borrowed opening/monitor-art addresses, captured persona, public monitor/swatch geometry, all unselected material/numeric records and resource names");
		};
		// The real retained Monitor is constructed without GPU resources or RNG.
		// This CPU check observes records and public geometry, not rendered pixels
		// or protected captured frame handles. Native acceptance covers the draw.
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const auto initial_colours = colour_values(presentation);
		std::srand(0x90a133);
		const int expected_random = std::rand();
		std::srand(0x90a133);
		for (const auto& [word, expected] : std::array<std::pair<const char*, labrador::Colour>, 4>{ {
			{ "RED", { 255, 0, 0 } }, { "3165A7", { 49, 101, 167 } },
			{ "2060C080", { 32, 96, 192, 128 } }, { "A532D600", { 165, 50, 214, 0 } } } })
		{
			auto expected_colours = initial_colours;
			expected_colours[power_light_colour_index] = expected;
			require(impl.execute(std::string("tune presentation.opening.monitor_art.power_light ") + word).ok &&
				colour_values(presentation) == expected_colours && read_text(scratch.source / L"presentation.json") == baseline &&
				read_text(scratch.staged / L"presentation.json") == baseline,
				"Power-light accepts straight named/RGB/partial/nonblack-zero-alpha channels while retaining every unrelated colour and both content copies");
			unchanged();
		}
		const TuneDocument* const pending = impl.tune_document("presentation");
		require(impl.execute("tune presentation.debug_overlay.colour 789ABC").ok &&
			impl.execute("tune.reset presentation.opening.monitor_art.power_light").ok && opening->monitor_art.power_light == original.monitor_art.power_light &&
			presentation.debug_overlay().colour == labrador::Colour(120, 154, 188) && pending->edits().size() == 1 &&
			pending->edits().contains("debug_overlay.colour"),
			"Power-light reset restores its original tint while retaining an unrelated live colour edit");
		unchanged();
		require(impl.execute("tune.reset presentation.debug_overlay.colour").ok && colour_values(presentation) == initial_colours &&
			pending->edits().empty() && pending->current().text() == baseline,
			"Independent reset restores the exact initial document after retaining the constructed Monitor");
		const JsonSpanDocument baseline_document(baseline, "power-light-colour-save-reference");
		const std::string saved = baseline_document.replaced(*baseline_document.find("opening.monitor_art.power_light"), "\"2060C080\"");
		require(impl.execute("tune presentation.opening.monitor_art.power_light 2060C080").ok && impl.execute("tune.save presentation").ok &&
			opening->monitor_art.power_light == labrador::Colour(32, 96, 192, 128) && read_text(scratch.source / L"presentation.json") == saved &&
			read_text(scratch.staged / L"presentation.json") == saved && pending->edits().empty() &&
			pending->baseline().text() == saved && pending->current().text() == saved,
			"Power-light save changes exactly one quoted token and rebases the retained borrowed record");
		unchanged();
		require(impl.execute("tune presentation.opening.monitor_art.power_light A532D600").ok,
			"A fresh nonblack alpha0 power-light edit is pending before invalid merged geometry");
		const JsonSpanDocument saved_document(saved, "power-light-colour-merged-reference");
		const JsonLeaf* shadow_width = saved_document.find("opening.monitor_art.shadow_width");
		require(shadow_width != nullptr && original.monitor_art.shadow_width > 0.0f && original.monitor_art.shadow_width <= 0.1f,
			"Power-light merge fixture starts inside the real monitor shadow-width bound");
		const std::string external = saved_document.replaced(*shadow_width, "0.101");
		write_text(scratch.source / L"presentation.json", external);
		const std::string pending_text = pending->current().text();
		const auto pending_colours = colour_values(presentation);
		const TuneEdit held_edit = pending->edits().at("opening.monitor_art.power_light");
		for (const char* command : { "tune.save presentation", "tune.save presentation force" })
		{
			require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == saved && pending->current().text() == pending_text &&
				pending->baseline().text() == saved && pending->edits().size() == 1 &&
				pending->edits().contains("opening.monitor_art.power_light") &&
				pending->edits().at("opening.monitor_art.power_light").baseline_token == held_edit.baseline_token &&
				pending->edits().at("opening.monitor_art.power_light").current_token == held_edit.current_token &&
				colour_values(presentation) == pending_colours,
				"External monitor_art.shadow_width 0.101 exceeds 0.1; complete merged rejection retains exact source/staged/live/current/baseline/ledger, even with force");
			unchanged();
		}
		require(impl.execute("tune.reset presentation.opening.monitor_art.power_light").ok &&
			opening->monitor_art.power_light == labrador::Colour(32, 96, 192, 128) && pending->edits().empty() &&
			pending->current().text() == saved && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external && read_text(scratch.staged / L"presentation.json") == saved &&
			!std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
			!std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
			!std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
			"Power-light resets to its saved partial-alpha baseline without discarding the external source or leaving recovery files");
		unchanged();
		write_text(scratch.source / L"presentation.json", saved);
		require(std::rand() == expected_random,
			"Power-light edits, independent resets and valid/refused saves consume no gameplay random draw");
	}

	void check_program_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"program-colour-records");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const OpeningPresentation* const opening = &presentation.opening();
		const OpeningPresentation original = *opening;
		const MonitorArtPresentation* const monitor_art = &opening->monitor_art;
		labrador::RenderResources resources;
		for (const std::string& sheet_name : { opening->sprite_sheet, opening->palette_sheet })
		{
			resources.add_sprite_sheet(sheet_name, labrador::read_sprite_sheet(
				(game_directory / L"textures" / (sheet_name + ".json")).string().c_str(), labrador::TextureHandle{}));
		}
		MonitorPersona persona;
		persona.mug = true;
		persona.coffee_ring = mattmath::Vector2F(0.85f, 0.9f);
		persona.sticky_notes.push_back({ mattmath::Vector2F(0.015f, 0.12f), 0, 0.1f });
		persona.stickers.push_back({ mattmath::Vector2F(0.2f, 0.9f), 1, 0.0f });
		persona.crack = { mattmath::Vector2F(0.004f, 0.6f), mattmath::Vector2F(0.02f, 0.62f) };
		const Monitor monitor(opening, &resources, persona);
		const MonitorPersona* const captured_persona = &monitor.persona();
		const mattmath::RectangleF screen(0, 0, 1280, 720);
		const auto geometry = [&]
		{
			return std::vector<float>{ opening->cursor_art.shadow_offset, opening->cursor_art.click_duration,
				opening->cursor_art.click_reach, opening->silhouette_art.pen_width, opening->silhouette_art.shade_width,
				opening->palette_height, opening->adult_height, opening->child_height, opening->cursor_size,
				opening->preview_strength, opening->percent_height, opening->hop, opening->shake, opening->slump,
				opening->monitor_art.pen_width, opening->monitor_art.corner_step, opening->monitor_art.shadow_width,
				static_cast<float>(opening->monitor_art.vent_count), opening->reveal_art.keyline_width,
				opening->reveal_art.padding, opening->reveal_art.shadow_offset,
				opening->reveal_art.tick_length, opening->reveal_art.beat_duration };
		};
		const auto unselected = [&]
		{
			return std::vector<labrador::Colour>{ opening->cursor_art.ink, opening->cursor_art.fill,
				opening->cursor_art.shade, opening->cursor_art.shadow, opening->silhouette,
				opening->silhouette_art.ink, opening->silhouette_art.rim, opening->silhouette_art.shade,
				opening->bezel_shadow, opening->monitor_art.ink, opening->monitor_art.highlight,
				opening->desktop_colour, opening->glass_colour, opening->monitor_art.power_light,
				opening->slot_outline, opening->scribble, opening->coffee, opening->crack,
				opening->reveal_art.ink, opening->reveal_art.paper, opening->reveal_art.shade,
				opening->figure_grey, opening->face, opening->sweat };
		};
		const auto rectangles = [&]
		{
			return std::array<mattmath::RectangleF, 10>{ opening->glass, opening->monitor_art.vent_area,
				opening->monitor_art.power_button, opening->monitor_art.power_indicator,
				monitor.glass(screen), monitor.picture(screen), monitor.swatch(screen, 0), monitor.swatch(screen, 1),
				monitor.empty_slot(screen), monitor.close_button(screen) };
		};
		const auto persona_state = [&]
		{
			const auto& held = monitor.persona();
			std::vector<float> values{ static_cast<float>(held.shade), held.child ? 1.0f : 0.0f,
				held.mug ? 1.0f : 0.0f, held.coffee_ring.x, held.coffee_ring.y };
			for (const auto& decorations : { held.sticky_notes, held.stickers })
			{
				values.push_back(static_cast<float>(decorations.size()));
				for (const auto& decoration : decorations)
					{ values.insert(values.end(), { decoration.at.x, decoration.at.y, static_cast<float>(decoration.colour), decoration.tilt }); }
			}
			values.push_back(static_cast<float>(held.crack.size()));
			for (const auto& point : held.crack) { values.insert(values.end(), { point.x, point.y }); }
			return values;
		};
		const auto held_geometry = geometry();
		const auto held_unselected = unselected();
		const auto held_rectangles = rectangles();
		const auto held_persona = persona_state();
		const auto unchanged = [&]
		{
			require(&presentation.opening() == opening && &opening->monitor_art == monitor_art && &monitor.persona() == captured_persona &&
				geometry() == held_geometry && unselected() == held_unselected && rectangles() == held_rectangles &&
				persona_state() == held_persona && opening->bezel_shades == original.bezel_shades &&
				opening->sticky_notes == original.sticky_notes && opening->stickers == original.stickers &&
				opening->sprite_sheet == original.sprite_sheet && opening->fill_frame == original.fill_frame &&
				opening->percent_font == original.percent_font && opening->palette_sheet == original.palette_sheet &&
				opening->palette_frames == original.palette_frames,
				"Program adoption retains the borrowed opening/monitor-art addresses, captured persona, public monitor/swatch geometry, all unselected material/numeric records and resource names");
		};
		// The real retained Monitor is constructed without GPU resources or RNG.
		// This CPU check observes records and public geometry, not rendered pixels
		// or protected captured frame handles. Native acceptance covers the draw.
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const auto initial_colours = colour_values(presentation);
		std::srand(0x90a134);
		const int expected_random = std::rand();
		std::srand(0x90a134);
		for (const auto& [word, expected] : std::array<std::pair<const char*, labrador::Colour>, 4>{ {
			{ "RED", { 255, 0, 0 } }, { "3165A7", { 49, 101, 167 } },
			{ "2060C080", { 32, 96, 192, 128 } }, { "A532D600", { 165, 50, 214, 0 } } } })
		{
			auto expected_colours = initial_colours;
			expected_colours[program_colour_index] = expected;
			require(impl.execute(std::string("tune presentation.opening.program_colour ") + word).ok &&
				colour_values(presentation) == expected_colours && read_text(scratch.source / L"presentation.json") == baseline &&
				read_text(scratch.staged / L"presentation.json") == baseline,
				"Program accepts straight named/RGB/partial/nonblack-zero-alpha channels while retaining every unrelated colour and both content copies");
			unchanged();
		}
		const TuneDocument* const pending = impl.tune_document("presentation");
		require(impl.execute("tune presentation.debug_overlay.colour 789ABC").ok &&
			impl.execute("tune.reset presentation.opening.program_colour").ok && opening->program_colour == original.program_colour &&
			presentation.debug_overlay().colour == labrador::Colour(120, 154, 188) && pending->edits().size() == 1 &&
			pending->edits().contains("debug_overlay.colour"),
			"Program reset restores its original tint while retaining an unrelated live colour edit");
		unchanged();
		require(impl.execute("tune.reset presentation.debug_overlay.colour").ok && colour_values(presentation) == initial_colours &&
			pending->edits().empty() && pending->current().text() == baseline,
			"Independent reset restores the exact initial document after retaining the constructed Monitor");
		const JsonSpanDocument baseline_document(baseline, "program-colour-save-reference");
		const std::string saved = baseline_document.replaced(*baseline_document.find("opening.program_colour"), "\"2060C080\"");
		require(impl.execute("tune presentation.opening.program_colour 2060C080").ok && impl.execute("tune.save presentation").ok &&
			opening->program_colour == labrador::Colour(32, 96, 192, 128) && read_text(scratch.source / L"presentation.json") == saved &&
			read_text(scratch.staged / L"presentation.json") == saved && pending->edits().empty() &&
			pending->baseline().text() == saved && pending->current().text() == saved,
			"Program save changes exactly one quoted token and rebases the retained borrowed record");
		unchanged();
		require(impl.execute("tune presentation.opening.program_colour A532D600").ok,
			"A fresh nonblack alpha0 program edit is pending before invalid merged geometry");
		const JsonSpanDocument saved_document(saved, "program-colour-merged-reference");
		const JsonLeaf* palette_height = saved_document.find("opening.palette_height");
		require(palette_height != nullptr && original.palette_height > 0.0f,
			"Program merge fixture starts inside the real positive palette-height constraint");
		const std::string external = saved_document.replaced(*palette_height, "0");
		write_text(scratch.source / L"presentation.json", external);
		const std::string pending_text = pending->current().text();
		const auto pending_colours = colour_values(presentation);
		const TuneEdit held_edit = pending->edits().at("opening.program_colour");
		for (const char* command : { "tune.save presentation", "tune.save presentation force" })
		{
			require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == saved && pending->current().text() == pending_text &&
				pending->baseline().text() == saved && pending->edits().size() == 1 &&
				pending->edits().contains("opening.program_colour") &&
				pending->edits().at("opening.program_colour").baseline_token == held_edit.baseline_token &&
				pending->edits().at("opening.program_colour").current_token == held_edit.current_token &&
				colour_values(presentation) == pending_colours,
				"External palette_height 0 fails the positive reader constraint; complete merged rejection retains exact source/staged/live/current/baseline/ledger, even with force");
			unchanged();
		}
		require(impl.execute("tune.reset presentation.opening.program_colour").ok &&
			opening->program_colour == labrador::Colour(32, 96, 192, 128) && pending->edits().empty() &&
			pending->current().text() == saved && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external && read_text(scratch.staged / L"presentation.json") == saved &&
			!std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
			!std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
			!std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
			"Program resets to its saved partial-alpha baseline without discarding the external source or leaving recovery files");
		unchanged();
		write_text(scratch.source / L"presentation.json", saved);
		require(std::rand() == expected_random,
			"Program edits, independent resets and valid/refused saves consume no gameplay random draw");
	}

	void check_glass_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"glass-colour-records");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const OpeningPresentation* const opening = &presentation.opening();
		const OpeningPresentation original = *opening;
		const MonitorArtPresentation* const monitor_art = &opening->monitor_art;
		labrador::RenderResources resources;
		for (const std::string& sheet_name : { opening->sprite_sheet, opening->palette_sheet })
		{
			resources.add_sprite_sheet(sheet_name, labrador::read_sprite_sheet(
				(game_directory / L"textures" / (sheet_name + ".json")).string().c_str(), labrador::TextureHandle{}));
		}
		MonitorPersona persona;
		persona.mug = true;
		persona.coffee_ring = mattmath::Vector2F(0.85f, 0.9f);
		persona.sticky_notes.push_back({ mattmath::Vector2F(0.015f, 0.12f), 0, 0.1f });
		persona.stickers.push_back({ mattmath::Vector2F(0.2f, 0.9f), 1, 0.0f });
		persona.crack = { mattmath::Vector2F(0.004f, 0.6f), mattmath::Vector2F(0.02f, 0.62f) };
		const Monitor monitor(opening, &resources, persona);
		const MonitorPersona* const captured_persona = &monitor.persona();
		const mattmath::RectangleF screen(0, 0, 1280, 720);
		const auto geometry = [&]
		{
			return std::vector<float>{ opening->cursor_art.shadow_offset, opening->cursor_art.click_duration,
				opening->cursor_art.click_reach, opening->silhouette_art.pen_width, opening->silhouette_art.shade_width,
				opening->palette_height, opening->adult_height, opening->child_height, opening->cursor_size,
				opening->preview_strength, opening->percent_height, opening->hop, opening->shake, opening->slump,
				opening->monitor_art.pen_width, opening->monitor_art.corner_step, opening->monitor_art.shadow_width,
				static_cast<float>(opening->monitor_art.vent_count), opening->reveal_art.keyline_width,
				opening->reveal_art.padding, opening->reveal_art.shadow_offset,
				opening->reveal_art.tick_length, opening->reveal_art.beat_duration };
		};
		const auto unselected = [&]
		{
			return std::vector<labrador::Colour>{ opening->cursor_art.ink, opening->cursor_art.fill,
				opening->cursor_art.shade, opening->cursor_art.shadow, opening->silhouette,
				opening->silhouette_art.ink, opening->silhouette_art.rim, opening->silhouette_art.shade,
				opening->bezel_shadow, opening->monitor_art.ink, opening->monitor_art.highlight,
				opening->desktop_colour, opening->program_colour, opening->monitor_art.power_light,
				opening->slot_outline, opening->scribble, opening->coffee, opening->crack,
				opening->reveal_art.ink, opening->reveal_art.paper, opening->reveal_art.shade,
				opening->figure_grey, opening->face, opening->sweat };
		};
		const auto rectangles = [&]
		{
			return std::array<mattmath::RectangleF, 10>{ opening->glass, opening->monitor_art.vent_area,
				opening->monitor_art.power_button, opening->monitor_art.power_indicator,
				monitor.glass(screen), monitor.picture(screen), monitor.swatch(screen, 0), monitor.swatch(screen, 1),
				monitor.empty_slot(screen), monitor.close_button(screen) };
		};
		const auto persona_state = [&]
		{
			const auto& held = monitor.persona();
			std::vector<float> values{ static_cast<float>(held.shade), held.child ? 1.0f : 0.0f,
				held.mug ? 1.0f : 0.0f, held.coffee_ring.x, held.coffee_ring.y };
			for (const auto& decorations : { held.sticky_notes, held.stickers })
			{
				values.push_back(static_cast<float>(decorations.size()));
				for (const auto& decoration : decorations)
					{ values.insert(values.end(), { decoration.at.x, decoration.at.y, static_cast<float>(decoration.colour), decoration.tilt }); }
			}
			values.push_back(static_cast<float>(held.crack.size()));
			for (const auto& point : held.crack) { values.insert(values.end(), { point.x, point.y }); }
			return values;
		};
		const auto held_geometry = geometry();
		const auto held_unselected = unselected();
		const auto held_rectangles = rectangles();
		const auto held_persona = persona_state();
		const auto unchanged = [&]
		{
			require(&presentation.opening() == opening && &opening->monitor_art == monitor_art && &monitor.persona() == captured_persona &&
				geometry() == held_geometry && unselected() == held_unselected && rectangles() == held_rectangles &&
				persona_state() == held_persona && opening->bezel_shades == original.bezel_shades &&
				opening->sticky_notes == original.sticky_notes && opening->stickers == original.stickers &&
				opening->sprite_sheet == original.sprite_sheet && opening->fill_frame == original.fill_frame &&
				opening->percent_font == original.percent_font && opening->palette_sheet == original.palette_sheet &&
				opening->palette_frames == original.palette_frames,
				"Glass adoption retains the borrowed opening/monitor-art addresses, captured persona, public monitor/swatch geometry, all unselected material/numeric records and resource names");
		};
		// The real retained Monitor is constructed without GPU resources or RNG.
		// This CPU check observes records and public geometry, not rendered pixels
		// or protected captured frame handles. Native acceptance covers the draw.
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const auto initial_colours = colour_values(presentation);
		std::srand(0x91a137);
		const int expected_random = std::rand();
		std::srand(0x91a137);
		for (const auto& [word, expected] : std::array<std::pair<const char*, labrador::Colour>, 4>{ {
			{ "RED", { 255, 0, 0 } }, { "3165A7", { 49, 101, 167 } },
			{ "2060C080", { 32, 96, 192, 128 } }, { "A532D600", { 165, 50, 214, 0 } } } })
		{
			auto expected_colours = initial_colours;
			expected_colours[glass_colour_index] = expected;
			require(impl.execute(std::string("tune presentation.opening.glass_colour ") + word).ok &&
				colour_values(presentation) == expected_colours && read_text(scratch.source / L"presentation.json") == baseline &&
				read_text(scratch.staged / L"presentation.json") == baseline,
				"Glass accepts straight named/RGB/partial/nonblack-zero-alpha channels while retaining every unrelated colour and both content copies");
			unchanged();
		}
		const TuneDocument* const pending = impl.tune_document("presentation");
		require(impl.execute("tune presentation.debug_overlay.colour 789ABC").ok &&
			impl.execute("tune.reset presentation.opening.glass_colour").ok && opening->glass_colour == original.glass_colour &&
			presentation.debug_overlay().colour == labrador::Colour(120, 154, 188) && pending->edits().size() == 1 &&
			pending->edits().contains("debug_overlay.colour"),
			"Glass reset restores its original tint while retaining an unrelated live colour edit");
		unchanged();
		require(impl.execute("tune.reset presentation.debug_overlay.colour").ok && colour_values(presentation) == initial_colours &&
			pending->edits().empty() && pending->current().text() == baseline,
			"Independent reset restores the exact initial document after retaining the constructed Monitor");
		const JsonSpanDocument baseline_document(baseline, "glass-colour-save-reference");
		const std::string saved = baseline_document.replaced(*baseline_document.find("opening.glass_colour"), "\"2060C080\"");
		require(impl.execute("tune presentation.opening.glass_colour 2060C080").ok && impl.execute("tune.save presentation").ok &&
			opening->glass_colour == labrador::Colour(32, 96, 192, 128) && read_text(scratch.source / L"presentation.json") == saved &&
			read_text(scratch.staged / L"presentation.json") == saved && pending->edits().empty() &&
			pending->baseline().text() == saved && pending->current().text() == saved,
			"Glass save changes exactly one quoted token and rebases the retained borrowed record");
		unchanged();
		require(impl.execute("tune presentation.opening.glass_colour A532D600").ok,
			"A fresh nonblack alpha0 glass edit is pending before invalid merged geometry");
		const char* unrelated_word = initial_colours[3] == labrador::Colour(120, 154, 188) ? "654321" : "789ABC";
		require(impl.execute(std::string("tune presentation.debug_overlay.colour ") + unrelated_word).ok && pending->edits().size() == 2,
			"Complete glass merge checks retain an unrelated pending colour edit");
		const JsonSpanDocument saved_document(saved, "glass-colour-merged-reference");
		const JsonLeaf* glass_width = saved_document.find("opening.glass.width");
		require(glass_width != nullptr && original.glass.x == 0.1f && original.glass.x + 0.95f > 1.0f,
			"Glass merge fixture has the actual cross-field on-screen rectangle constraint");
		const std::string external = saved_document.replaced(*glass_width, "0.95");
		write_text(scratch.source / L"presentation.json", external);
		const std::string pending_text = pending->current().text();
		const auto pending_colours = colour_values(presentation);
		const auto ledger = [&]
		{
			std::vector<std::string> result;
			for (const auto& [path, edit] : pending->edits())
				{ result.push_back(path + "=" + edit.baseline_token + "->" + edit.current_token); }
			return result;
		};
		const auto held_ledger = ledger();
		for (const char* command : { "tune.save presentation", "tune.save presentation force" })
		{
			require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == saved && pending->current().text() == pending_text &&
				pending->baseline().text() == saved && ledger() == held_ledger && pending->edits().size() == 2 &&
				colour_values(presentation) == pending_colours,
				"External glass.width 0.95 with x 0.1 refuses the complete merged candidate while retaining exact source/staged/live/current/baseline and every ledger path/baseline/current token even with force");
			unchanged();
		}
		const std::string overlay_only = saved_document.replaced(*saved_document.find("debug_overlay.colour"),
			std::string("\"") + unrelated_word + '"');
		auto restored_colours = pending_colours;
		restored_colours[glass_colour_index] = labrador::Colour(32, 96, 192, 128);
		require(impl.execute("tune.reset presentation.opening.glass_colour").ok && colour_values(presentation) == restored_colours &&
			pending->edits().size() == 1 && pending->edits().contains("debug_overlay.colour") &&
			pending->current().text() == overlay_only && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external && read_text(scratch.staged / L"presentation.json") == saved &&
			!std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
			!std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
			!std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
			"Glass resets to its saved partial-alpha token while retaining unrelated pending colour, invalid external source and exact staging without recovery files");
		unchanged();
		restored_colours[3] = initial_colours[3];
		require(impl.execute("tune.reset presentation.debug_overlay.colour").ok && colour_values(presentation) == restored_colours &&
			pending->edits().empty() && pending->current().text() == saved && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external && read_text(scratch.staged / L"presentation.json") == saved,
			"Resetting the unrelated colour keeps the saved glass baseline and exact external source/staging");
		unchanged();
		write_text(scratch.source / L"presentation.json", saved);
		require(std::rand() == expected_random,
			"Glass colour commands, independent/saved resets and valid/refused saves consume no gameplay random draw");
	}
