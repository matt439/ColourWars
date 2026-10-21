// Included inside tune_checks.cpp's private namespace, after its includes.
// The opening drawing's body and scribble colours.

	void check_silhouette_body_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"silhouette-body-colour-records");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const OpeningPresentation* const opening = &presentation.opening();
		const OpeningPresentation original = *opening;
		const SilhouetteArtPresentation* const art = &opening->silhouette_art;
		labrador::RenderResources resources;
		for (const std::string& sheet_name : { opening->sprite_sheet, opening->palette_sheet })
		{
			resources.add_sprite_sheet(sheet_name, labrador::read_sprite_sheet(
				(game_directory / L"textures" / (sheet_name + ".json")).string().c_str(), labrador::TextureHandle{}));
		}
		const Silhouette person(opening, &resources);
		static_cast<void>(person);
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
				opening->cursor_art.shade, opening->cursor_art.shadow, opening->glass_colour,
				opening->silhouette_art.ink, opening->silhouette_art.rim, opening->silhouette_art.shade,
				opening->bezel_shadow, opening->monitor_art.ink, opening->monitor_art.highlight,
				opening->desktop_colour, opening->program_colour, opening->monitor_art.power_light,
				opening->slot_outline, opening->scribble, opening->coffee, opening->crack,
				opening->reveal_art.ink, opening->reveal_art.paper, opening->reveal_art.shade,
				opening->figure_grey, opening->face, opening->sweat };
		};
		const auto rectangles = [&]
		{
			return std::array<mattmath::RectangleF, 4>{ opening->glass, opening->monitor_art.vent_area,
				opening->monitor_art.power_button, opening->monitor_art.power_indicator };
		};
		const auto held_geometry = geometry();
		const auto held_unselected = unselected();
		const auto held_rectangles = rectangles();
		// Silhouette owns no pose or timer. Native acceptance draws a retained
		// consumer; this CPU check observes the borrowed records and unchanged
		// caller-owned inputs without claiming pixels or private draw state.
		const std::array<MonitorPersona, 2> personas = []
		{
			std::array<MonitorPersona, 2> result;
			result[0].mug = true;
			result[0].coffee_ring = { 0.77f, 0.9f };
			result[0].sticky_notes = { { { 0.72f, 0.005f }, 1, -0.1f } };
			result[0].crack = { { 0.004f, 0.6f }, { 0.02f, 0.62f } };
			result[1].child = true;
			result[1].stickers = { { { 0.27f, 0.92f }, 0, 0.0f } };
			return result;
		}();
		const std::array<SilhouettePose, 3> poses{ SilhouettePose{},
			SilhouettePose{ 0.5f, 0.0f, 0.0f }, SilhouettePose{ 0.0f, 1.0f, 0.4f } };
		const mattmath::RectangleF screen(0.0f, 0.0f, 1280.0f, 720.0f);
		const auto inputs = [&]
		{
			std::vector<float> values{ screen.x, screen.y, screen.width, screen.height };
			for (const MonitorPersona& persona : personas)
			{
				values.insert(values.end(), { static_cast<float>(persona.shade), persona.child ? 1.0f : 0.0f,
					persona.mug ? 1.0f : 0.0f, persona.coffee_ring.x, persona.coffee_ring.y });
				for (const auto* decorations : { &persona.sticky_notes, &persona.stickers })
				{
					values.push_back(static_cast<float>(decorations->size()));
					for (const auto& decoration : *decorations)
					{
						values.insert(values.end(), { decoration.at.x, decoration.at.y,
							static_cast<float>(decoration.colour), decoration.tilt });
					}
				}
				values.push_back(static_cast<float>(persona.crack.size()));
				for (const auto& point : persona.crack) { values.insert(values.end(), { point.x, point.y }); }
			}
			for (const SilhouettePose& pose : poses) { values.insert(values.end(), { pose.sigh, pose.stand, pose.leave }); }
			return values;
		};
		const auto held_inputs = inputs();
		const auto unchanged = [&]
		{
			require(&presentation.opening() == opening && &opening->silhouette_art == art &&
				geometry() == held_geometry && unselected() == held_unselected && rectangles() == held_rectangles &&
				inputs() == held_inputs && opening->bezel_shades == original.bezel_shades &&
				opening->sticky_notes == original.sticky_notes && opening->stickers == original.stickers &&
				opening->sprite_sheet == original.sprite_sheet && opening->fill_frame == original.fill_frame &&
				opening->percent_font == original.percent_font && opening->palette_sheet == original.palette_sheet &&
				opening->palette_frames == original.palette_frames,
				"Silhouette body adoption retains the borrowed opening/art addresses, every unselected material/numeric record, resource names, public input rectangles and caller-owned persona/pose inputs");
		};
		// The real retained Silhouette is constructed without GPU resources or RNG.
		// This CPU check observes records and public geometry, not rendered pixels
		// or protected captured frame handles. Native acceptance covers the draw.
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const auto initial_colours = colour_values(presentation);
		std::srand(0xb0d138);
		const int expected_random = std::rand();
		std::srand(0xb0d138);
		for (const auto& [word, expected] : std::array<std::pair<const char*, labrador::Colour>, 4>{ {
			{ "RED", { 255, 0, 0 } }, { "3165A7", { 49, 101, 167 } },
			{ "2060C080", { 32, 96, 192, 128 } }, { "A532D600", { 165, 50, 214, 0 } } } })
		{
			auto expected_colours = initial_colours;
			expected_colours[silhouette_body_colour_index] = expected;
			require(impl.execute(std::string("tune presentation.opening.silhouette ") + word).ok &&
				colour_values(presentation) == expected_colours && read_text(scratch.source / L"presentation.json") == baseline &&
				read_text(scratch.staged / L"presentation.json") == baseline,
				"Silhouette body accepts straight named/RGB/partial/nonblack-zero-alpha channels while retaining every unrelated colour and both content copies");
			unchanged();
		}
		const TuneDocument* const pending = impl.tune_document("presentation");
		require(impl.execute("tune presentation.debug_overlay.colour 789ABC").ok &&
			impl.execute("tune.reset presentation.opening.silhouette").ok && opening->silhouette == original.silhouette &&
			presentation.debug_overlay().colour == labrador::Colour(120, 154, 188) && pending->edits().size() == 1 &&
			pending->edits().contains("debug_overlay.colour"),
			"Silhouette body reset restores its original tint while retaining an unrelated live colour edit");
		unchanged();
		require(impl.execute("tune.reset presentation.debug_overlay.colour").ok && colour_values(presentation) == initial_colours &&
			pending->edits().empty() && pending->current().text() == baseline,
			"Independent reset restores the exact initial document after retaining the constructed Silhouette");
		const JsonSpanDocument baseline_document(baseline, "silhouette-body-colour-save-reference");
		const std::string saved = baseline_document.replaced(*baseline_document.find("opening.silhouette"), "\"2060C080\"");
		require(impl.execute("tune presentation.opening.silhouette 2060C080").ok && impl.execute("tune.save presentation").ok &&
			opening->silhouette == labrador::Colour(32, 96, 192, 128) && read_text(scratch.source / L"presentation.json") == saved &&
			read_text(scratch.staged / L"presentation.json") == saved && pending->edits().empty() &&
			pending->baseline().text() == saved && pending->current().text() == saved,
			"Silhouette body save changes exactly one quoted token and rebases the retained borrowed record");
		unchanged();
		require(impl.execute("tune presentation.opening.silhouette A532D600").ok,
			"A fresh nonblack alpha0 body edit is pending before invalid merged geometry");
		const char* unrelated_word = initial_colours[3] == labrador::Colour(120, 154, 188) ? "654321" : "789ABC";
		require(impl.execute(std::string("tune presentation.debug_overlay.colour ") + unrelated_word).ok && pending->edits().size() == 2,
			"Complete body merge checks retain an unrelated pending colour edit");
		const JsonSpanDocument saved_document(saved, "silhouette-body-colour-merged-reference");
		const JsonLeaf* body_shade_width = saved_document.find("opening.silhouette_art.shade_width");
		require(body_shade_width != nullptr && original.silhouette_art.pen_width == 0.008f && 0.004f < original.silhouette_art.pen_width,
			"Silhouette body merge fixture has the actual shade width at least pen width constraint");
		const std::string external = saved_document.replaced(*body_shade_width, "0.004");
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
				"External silhouette_art.shade_width 0.004 below pen_width 0.008 refuses the complete merged candidate while retaining exact source/staged/live/current/baseline and every ledger path/baseline/current token even with force");
			unchanged();
		}
		const std::string overlay_only = saved_document.replaced(*saved_document.find("debug_overlay.colour"),
			std::string("\"") + unrelated_word + '"');
		auto restored_colours = pending_colours;
		restored_colours[silhouette_body_colour_index] = labrador::Colour(32, 96, 192, 128);
		require(impl.execute("tune.reset presentation.opening.silhouette").ok && colour_values(presentation) == restored_colours &&
			pending->edits().size() == 1 && pending->edits().contains("debug_overlay.colour") &&
			pending->current().text() == overlay_only && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external && read_text(scratch.staged / L"presentation.json") == saved &&
			!std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
			!std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
			!std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
			"Silhouette body resets to its saved partial-alpha token while retaining unrelated pending colour, invalid external source and exact staging without recovery files");
		unchanged();
		restored_colours[3] = initial_colours[3];
		require(impl.execute("tune.reset presentation.debug_overlay.colour").ok && colour_values(presentation) == restored_colours &&
			pending->edits().empty() && pending->current().text() == saved && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external && read_text(scratch.staged / L"presentation.json") == saved,
			"Resetting the unrelated colour keeps the saved body baseline and exact external source/staging");
		unchanged();
		write_text(scratch.source / L"presentation.json", saved);
		require(std::rand() == expected_random,
			"Silhouette body colour commands, independent/saved resets and valid/refused saves consume no gameplay random draw");
	}

	void check_scribble_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"scribble-colour-records");
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
		const std::array<MonitorPersona, 2> personas = []
		{
			std::array<MonitorPersona, 2> result;
			result[0].sticky_notes = { { { 0.015f, 0.47f }, 0, 0.0f } };
			result[1].sticky_notes = { { { 0.015f, 0.47f }, 2, 0.1f } };
			return result;
		}();
		const Monitor first_monitor(opening, &resources, personas[0]);
		const Monitor second_monitor(opening, &resources, personas[1]);
		const std::array<const Monitor*, 2> monitors{ &first_monitor, &second_monitor };
		const std::array<const MonitorPersona*, 2> captured_personas{ &first_monitor.persona(), &second_monitor.persona() };
		const auto* const opening_sheet = resources.sprite_sheet(opening->sprite_sheet);
		const auto* const palette_sheet = resources.sprite_sheet(opening->palette_sheet);
		const mattmath::RectangleF screen(0, 0, 1280, 720);
		require(original.sticky_notes.size() > 2 && original.sticky_notes[0] == labrador::Colour(255, 242, 122) &&
			original.sticky_notes[2] == labrador::Colour(174, 235, 255),
			"The scribble fixture retains the two actual authored yellow/blue paper entries without editing the colour arrays");
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
				opening->slot_outline, opening->glass_colour, opening->coffee, opening->crack,
				opening->reveal_art.ink, opening->reveal_art.paper, opening->reveal_art.shade,
				opening->figure_grey, opening->face, opening->sweat };
		};
		const auto rectangles = [&]
		{
			std::vector<mattmath::RectangleF> values{ opening->glass, opening->monitor_art.vent_area,
				opening->monitor_art.power_button, opening->monitor_art.power_indicator, screen };
			for (const Monitor* monitor : monitors)
			{
				values.insert(values.end(), { monitor->glass(screen), monitor->picture(screen), monitor->swatch(screen, 0),
					monitor->swatch(screen, 1), monitor->empty_slot(screen), monitor->close_button(screen) });
			}
			return values;
		};
		const auto persona_values = [](const MonitorPersona& held)
		{
			std::vector<float> values{ static_cast<float>(held.shade), held.child ? 1.0f : 0.0f,
				held.mug ? 1.0f : 0.0f, held.coffee_ring.x, held.coffee_ring.y };
			for (const auto* decorations : { &held.sticky_notes, &held.stickers })
			{
				values.push_back(static_cast<float>(decorations->size()));
				for (const auto& decoration : *decorations)
				{
					values.insert(values.end(), { decoration.at.x, decoration.at.y,
						static_cast<float>(decoration.colour), decoration.tilt });
				}
			}
			values.push_back(static_cast<float>(held.crack.size()));
			for (const auto& point : held.crack) { values.insert(values.end(), { point.x, point.y }); }
			return values;
		};
		const auto persona_state = [&]
		{
			std::vector<float> values;
			for (std::size_t context = 0; context < monitors.size(); ++context)
			{
				const auto captured = persona_values(monitors[context]->persona());
				const auto caller = persona_values(personas[context]);
				values.insert(values.end(), captured.begin(), captured.end());
				values.insert(values.end(), caller.begin(), caller.end());
			}
			return values;
		};
		for (std::size_t context = 0; context < monitors.size(); ++context)
		{
			require(persona_values(monitors[context]->persona()) == persona_values(personas[context]) &&
				!personas[context].child && !personas[context].mug && personas[context].shade == 0 &&
				personas[context].sticky_notes.size() == 1 && personas[context].stickers.empty() && personas[context].crack.empty() &&
				personas[context].sticky_notes[0].at == mattmath::Vector2F(0.015f, 0.47f) &&
				personas[context].sticky_notes[0].colour == (context == 0 ? 0u : 2u) &&
				personas[context].sticky_notes[0].tilt == (context == 0 ? 0.0f : 0.1f),
				"Both retained CPU Monitors copy the exact paired note position/index/tilt contexts, without persona rolling");
		}
		const auto held_geometry = geometry();
		const auto held_unselected = unselected();
		const auto held_rectangles = rectangles();
		const auto held_persona = persona_state();
		const auto unchanged = [&]
		{
			require(&presentation.opening() == opening && &opening->monitor_art == monitor_art &&
				&first_monitor.persona() == captured_personas[0] && &second_monitor.persona() == captured_personas[1] &&
				resources.sprite_sheet(opening->sprite_sheet) == opening_sheet && resources.sprite_sheet(opening->palette_sheet) == palette_sheet &&
				geometry() == held_geometry && unselected() == held_unselected && rectangles() == held_rectangles &&
				persona_state() == held_persona && opening->bezel_shades == original.bezel_shades &&
				opening->sticky_notes == original.sticky_notes && opening->stickers == original.stickers &&
				opening->sprite_sheet == original.sprite_sheet && opening->fill_frame == original.fill_frame &&
				opening->percent_font == original.percent_font && opening->palette_sheet == original.palette_sheet &&
				opening->palette_frames == original.palette_frames,
				"Scribble adoption retains the borrowed opening/art, both captured note personas and caller inputs, public monitor/swatch rectangles, resource lookups and all unselected material/numeric/array/resource records");
		};
		// Both real Monitors are retained without GPU resources or persona rolling.
		// This CPU check observes records and public geometry, not rendered pixels
		// or protected captured frame handles. Native acceptance covers the draw.
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const auto initial_colours = colour_values(presentation);
		std::srand(0x5cdb39);
		const int expected_random = std::rand();
		std::srand(0x5cdb39);
		for (const auto& [word, expected] : std::array<std::pair<const char*, labrador::Colour>, 4>{ {
			{ "RED", { 255, 0, 0 } }, { "3165A7", { 49, 101, 167 } },
			{ "2060C080", { 32, 96, 192, 128 } }, { "A532D600", { 165, 50, 214, 0 } } } })
		{
			auto expected_colours = initial_colours;
			expected_colours[scribble_colour_index] = expected;
			require(impl.execute(std::string("tune presentation.opening.scribble ") + word).ok &&
				colour_values(presentation) == expected_colours && read_text(scratch.source / L"presentation.json") == baseline &&
				read_text(scratch.staged / L"presentation.json") == baseline,
				"Scribble accepts straight named/RGB/partial/nonblack-zero-alpha channels while retaining every unrelated colour and both content copies");
			unchanged();
		}
		const TuneDocument* const pending = impl.tune_document("presentation");
		require(impl.execute("tune presentation.debug_overlay.colour 789ABC").ok &&
			impl.execute("tune.reset presentation.opening.scribble").ok && opening->scribble == original.scribble &&
			presentation.debug_overlay().colour == labrador::Colour(120, 154, 188) && pending->edits().size() == 1 &&
			pending->edits().contains("debug_overlay.colour"),
			"Scribble reset restores its original tint while retaining an unrelated live colour edit");
		unchanged();
		require(impl.execute("tune.reset presentation.debug_overlay.colour").ok && colour_values(presentation) == initial_colours &&
			pending->edits().empty() && pending->current().text() == baseline,
			"Independent reset restores the exact initial document while retaining both constructed note consumers");
		const JsonSpanDocument baseline_document(baseline, "scribble-colour-save-reference");
		const std::string saved = baseline_document.replaced(*baseline_document.find("opening.scribble"), "\"2060C080\"");
		require(impl.execute("tune presentation.opening.scribble 2060C080").ok && impl.execute("tune.save presentation").ok &&
			opening->scribble == labrador::Colour(32, 96, 192, 128) && read_text(scratch.source / L"presentation.json") == saved &&
			read_text(scratch.staged / L"presentation.json") == saved && pending->edits().empty() &&
			pending->baseline().text() == saved && pending->current().text() == saved,
			"Scribble save changes exactly one quoted token and rebases the retained borrowed record");
		unchanged();
		require(impl.execute("tune presentation.opening.scribble A532D600").ok,
			"A fresh nonblack alpha0 scribble edit is pending before invalid merged geometry");
		const char* unrelated_word = initial_colours[3] == labrador::Colour(120, 154, 188) ? "654321" : "789ABC";
		require(impl.execute(std::string("tune presentation.debug_overlay.colour ") + unrelated_word).ok && pending->edits().size() == 2 &&
			pending->edits().contains("opening.scribble") && pending->edits().contains("debug_overlay.colour") &&
			opening->scribble == labrador::Colour(165, 50, 214, 0),
			"Complete scribble merge checks retain an unrelated pending colour edit");
		const JsonSpanDocument saved_document(saved, "scribble-colour-merged-reference");
		const JsonLeaf* scribble_corner_step = saved_document.find("opening.monitor_art.corner_step");
		require(scribble_corner_step != nullptr && original.monitor_art.pen_width == 0.004f && 0.002f < original.monitor_art.pen_width,
			"Scribble merge fixture has the actual corner step at least pen width constraint");
		const std::string external = saved_document.replaced(*scribble_corner_step, "0.002");
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
				"External monitor_art.corner_step 0.002 below pen_width 0.004 refuses the complete merged candidate while retaining exact source/staged/live/current/baseline and every ledger path/baseline/current token even with force");
			unchanged();
		}
		const std::string overlay_only = saved_document.replaced(*saved_document.find("debug_overlay.colour"),
			std::string("\"") + unrelated_word + '"');
		auto restored_colours = pending_colours;
		restored_colours[scribble_colour_index] = labrador::Colour(32, 96, 192, 128);
		require(impl.execute("tune.reset presentation.opening.scribble").ok && colour_values(presentation) == restored_colours &&
			pending->edits().size() == 1 && pending->edits().contains("debug_overlay.colour") &&
			pending->current().text() == overlay_only && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external && read_text(scratch.staged / L"presentation.json") == saved &&
			!std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
			!std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
			!std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
			"Scribble resets to its saved partial-alpha token while retaining unrelated pending colour, invalid external source and exact staging without recovery files");
		unchanged();
		restored_colours[3] = initial_colours[3];
		require(impl.execute("tune.reset presentation.debug_overlay.colour").ok && colour_values(presentation) == restored_colours &&
			pending->edits().empty() && pending->current().text() == saved && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external && read_text(scratch.staged / L"presentation.json") == saved,
			"Resetting the unrelated colour keeps the saved scribble baseline and exact external source/staging");
		unchanged();
		write_text(scratch.source / L"presentation.json", saved);
		require(std::rand() == expected_random,
			"Scribble colour commands, independent/saved resets and valid/refused saves consume no gameplay random draw");
	}
