// Included inside tune_checks.cpp's private namespace, after its includes.
// The opening cursor_art, silhouette_art and reveal_art colour records.

	void check_cursor_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"cursor-colour-records");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const OpeningPresentation* const opening = &presentation.opening();
		const CursorArtPresentation* const art = &opening->cursor_art;
		const OpeningPresentation original = *opening;
		const auto geometry = [&]
		{
			return std::array<float, 8>{ art->shadow_offset, art->click_duration, art->click_reach,
				opening->cursor_size, opening->adult_height, opening->child_height,
				opening->preview_strength, opening->percent_height };
		};
		const auto held_geometry = geometry();
		const auto unchanged = [&]
		{
			require(&presentation.opening() == opening && &opening->cursor_art == art &&
				art->shade == original.cursor_art.shade && art->shadow == original.cursor_art.shadow &&
				geometry() == held_geometry && opening->glass == original.glass &&
				opening->sprite_sheet == original.sprite_sheet && opening->fill_frame == original.fill_frame &&
				opening->palette_sheet == original.palette_sheet && opening->palette_frames == original.palette_frames,
				"Cursor colour adoption retains both borrowed records, unselected shade/shadow, geometry, timings and captured resource names");
		};
		// ScreenCursor has no retained click timer or mutable geometry: tip,
		// screen height, click age and Reduce effects are supplied to each draw.
		// Native acceptance covers those draws; this check stays on the records.
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const auto initial_colours = colour_values(presentation);
		auto expected_colours = initial_colours;
		std::srand(0xc015017);
		const int expected_random = std::rand();
		std::srand(0xc015017);
		for (const std::size_t index : { cursor_ink_colour_index, cursor_fill_colour_index })
		{
			for (const auto& [word, expected] : std::array<std::pair<const char*, labrador::Colour>, 4>{ {
				{ "RED", { 255, 0, 0 } }, { "3165A7", { 49, 101, 167 } },
				{ "2060C080", { 32, 96, 192, 128 } }, { "A532D600", { 165, 50, 214, 0 } } } })
			{
				expected_colours[index] = expected;
				require(impl.execute(std::string("tune ") + colour_paths[index] + ' ' + word).ok &&
					colour_values(presentation) == expected_colours &&
					read_text(scratch.source / L"presentation.json") == baseline &&
					read_text(scratch.staged / L"presentation.json") == baseline,
					"Each cursor colour accepts straight named/RGB/partial/nonblack-zero-alpha channels while retaining every other colour and both content copies");
				unchanged();
			}
		}
		const TuneDocument* const pending = impl.tune_document("presentation");
		for (const std::size_t index : { cursor_ink_colour_index, cursor_fill_colour_index })
		{
			expected_colours[index] = initial_colours[index];
			require(impl.execute(std::string("tune.reset ") + colour_paths[index]).ok &&
				colour_values(presentation) == expected_colours &&
				pending->edits().size() == (index == cursor_ink_colour_index ? 1u : 0u) &&
				(index != cursor_ink_colour_index || pending->edits().contains("opening.cursor_art.fill")),
				"Each cursor reset restores its own authored colour while retaining the sibling pending edit");
			unchanged();
		}
		require(pending->current().text() == baseline && pending->edits().empty(),
			"Resetting both cursor colours restores the exact original document");
		const JsonSpanDocument baseline_document(baseline, "cursor-colour-save-reference");
		const JsonSpanDocument saved_ink(baseline_document.replaced(
			*baseline_document.find("opening.cursor_art.ink"), "\"2060C080\""), "cursor-colour-save-reference");
		const std::string saved = saved_ink.replaced(*saved_ink.find("opening.cursor_art.fill"), "\"D973A6C0\"");
		expected_colours[cursor_ink_colour_index] = labrador::Colour(32, 96, 192, 128);
		expected_colours[cursor_fill_colour_index] = labrador::Colour(217, 115, 166, 192);
		require(impl.execute("tune presentation.opening.cursor_art.ink 2060C080").ok &&
			impl.execute("tune presentation.opening.cursor_art.fill D973A6C0").ok &&
			impl.execute("tune.save presentation").ok && colour_values(presentation) == expected_colours &&
			read_text(scratch.source / L"presentation.json") == saved && read_text(scratch.staged / L"presentation.json") == saved &&
			pending->edits().empty() && pending->baseline().text() == saved && pending->current().text() == saved,
			"Saving both cursor colours changes only their two quoted tokens and rebases the retained borrowed records");
		unchanged();
		require(impl.execute("tune presentation.opening.cursor_art.ink A532D600").ok &&
			impl.execute("tune presentation.opening.cursor_art.fill RED").ok,
			"Independent cursor colour edits are pending before an unrelated invalid merged candidate");
		expected_colours[cursor_ink_colour_index] = labrador::Colour(165, 50, 214, 0);
		expected_colours[cursor_fill_colour_index] = labrador::Colour(255, 0, 0);
		const JsonSpanDocument saved_document(saved, "cursor-colour-merged-reference");
		const JsonLeaf* click_duration = saved_document.find("opening.cursor_art.click_duration");
		require(click_duration != nullptr && original.cursor_art.click_duration <= 0.5f,
			"The cursor merged candidate has its real unrelated click-duration upper bound");
		const std::string external = saved_document.replaced(*click_duration, "0.501");
		write_text(scratch.source / L"presentation.json", external);
		const std::string pending_text = pending->current().text();
		for (const char* command : { "tune.save presentation", "tune.save presentation force" })
		{
			require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == saved && pending->current().text() == pending_text &&
				pending->baseline().text() == saved && pending->edits().size() == 2 &&
				pending->edits().contains("opening.cursor_art.ink") && pending->edits().contains("opening.cursor_art.fill") &&
				colour_values(presentation) == expected_colours,
				"An unrelated click_duration 0.501 rejects the complete merged candidate and retains source/staged/live/ledger, even with force");
			unchanged();
		}
		expected_colours[cursor_ink_colour_index] = labrador::Colour(32, 96, 192, 128);
		require(impl.execute("tune.reset presentation.opening.cursor_art.ink").ok &&
			colour_values(presentation) == expected_colours && pending->edits().size() == 1 &&
			pending->edits().contains("opening.cursor_art.fill"),
			"After rejected saves cursor ink resets to the saved partial-alpha baseline while retaining the fill edit");
		unchanged();
		expected_colours[cursor_fill_colour_index] = labrador::Colour(217, 115, 166, 192);
		require(impl.execute("tune.reset presentation.opening.cursor_art.fill").ok &&
			colour_values(presentation) == expected_colours && pending->edits().empty() &&
			pending->current().text() == saved && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external && read_text(scratch.staged / L"presentation.json") == saved &&
			!std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
			!std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
			!std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
			"Cursor fill resets to its saved baseline without discarding the invalid external source or leaving recovery files");
		unchanged();
		write_text(scratch.source / L"presentation.json", saved);
		require(std::rand() == expected_random,
			"Cursor colour edits, independent resets and valid/refused saves consume no gameplay random draw");
	}

	void check_silhouette_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"silhouette-colour-records");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const OpeningPresentation* const opening = &presentation.opening();
		const SilhouetteArtPresentation* const art = &opening->silhouette_art;
		const OpeningPresentation original = *opening;
		const auto geometry = [&]
		{
			return std::vector<float>{ art->pen_width, art->shade_width, opening->palette_height,
				opening->adult_height, opening->child_height, opening->cursor_size, opening->preview_strength,
				opening->percent_height, opening->hop, opening->shake, opening->slump,
				opening->cursor_art.shadow_offset, opening->cursor_art.click_duration, opening->cursor_art.click_reach,
				opening->monitor_art.pen_width, opening->monitor_art.corner_step, opening->monitor_art.shadow_width,
				static_cast<float>(opening->monitor_art.vent_count), opening->reveal_art.keyline_width,
				opening->reveal_art.padding, opening->reveal_art.shadow_offset,
				opening->reveal_art.tick_length, opening->reveal_art.beat_duration };
		};
		const auto unselected_colours = [&]
		{
			return std::vector<labrador::Colour>{ opening->silhouette, opening->bezel_shadow,
				opening->monitor_art.ink, opening->monitor_art.highlight, opening->monitor_art.power_light,
				opening->glass_colour, opening->program_colour, opening->desktop_colour, opening->slot_outline,
				opening->scribble, opening->coffee, opening->crack, opening->cursor_art.ink, opening->cursor_art.fill,
				opening->cursor_art.shade, opening->cursor_art.shadow, opening->reveal_art.ink,
				opening->reveal_art.paper, opening->reveal_art.shade, opening->figure_grey, opening->face, opening->sweat };
		};
		const auto rectangles = [&]
		{
			return std::array<mattmath::RectangleF, 4>{ opening->glass, opening->monitor_art.vent_area,
				opening->monitor_art.power_button, opening->monitor_art.power_indicator };
		};
		const auto held_geometry = geometry();
		const auto held_unselected = unselected_colours();
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
				geometry() == held_geometry && unselected_colours() == held_unselected && rectangles() == held_rectangles &&
				opening->sprite_sheet == original.sprite_sheet && opening->fill_frame == original.fill_frame &&
				opening->palette_sheet == original.palette_sheet && opening->palette_frames == original.palette_frames &&
				opening->percent_font == original.percent_font && opening->bezel_shades == original.bezel_shades &&
				opening->sticky_notes == original.sticky_notes && opening->stickers == original.stickers && inputs() == held_inputs,
				"Silhouette colour adoption retains both borrowed records, separate body fill, unselected opening colours/resources/geometry and caller-owned persona/pose inputs");
		};
		const std::array<std::size_t, 3> indices{ silhouette_ink_colour_index,
			silhouette_rim_colour_index, silhouette_shade_colour_index };
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const auto initial_colours = colour_values(presentation);
		auto expected_colours = initial_colours;
		std::srand(0x511026);
		const int expected_random = std::rand();
		std::srand(0x511026);
		for (const std::size_t index : indices)
		{
			for (const auto& [word, expected] : std::array<std::pair<const char*, labrador::Colour>, 4>{ {
				{ "RED", { 255, 0, 0 } }, { "3165A7", { 49, 101, 167 } },
				{ "2060C080", { 32, 96, 192, 128 } }, { "A532D600", { 165, 50, 214, 0 } } } })
			{
				expected_colours[index] = expected;
				require(impl.execute(std::string("tune ") + colour_paths[index] + ' ' + word).ok &&
					colour_values(presentation) == expected_colours && read_text(scratch.source / L"presentation.json") == baseline &&
					read_text(scratch.staged / L"presentation.json") == baseline,
					"Each silhouette-art colour accepts straight named/RGB/partial/nonblack-zero-alpha channels while retaining all other colours and both content copies");
				unchanged();
			}
		}
		const TuneDocument* const pending = impl.tune_document("presentation");
		const char* unrelated_word = initial_colours[3] == labrador::Colour(120, 154, 188) ? "654321" : "789ABC";
		expected_colours[3] = labrador::Colour(unrelated_word);
		require(impl.execute(std::string("tune presentation.debug_overlay.colour ") + unrelated_word).ok && pending->edits().size() == 4,
			"A fresh unrelated colour edit is pending beside all three silhouette-art edits");
		for (const std::size_t index : indices)
		{
			const std::string local = std::string(colour_paths[index]).substr(std::string("presentation.").size());
			expected_colours[index] = initial_colours[index];
			require(impl.execute(std::string("tune.reset ") + colour_paths[index]).ok && colour_values(presentation) == expected_colours &&
				pending->edits().size() == 3 && !pending->edits().contains(local) && pending->edits().contains("debug_overlay.colour"),
				"Every silhouette-art reset restores only itself while both sibling edits and unrelated colour remain pending");
			for (const std::size_t sibling : indices)
			{
				if (sibling == index) { continue; }
				require(pending->edits().contains(std::string(colour_paths[sibling]).substr(std::string("presentation.").size())),
					"Each silhouette-art reset retains both independently pending siblings");
			}
			unchanged();
			expected_colours[index] = labrador::Colour(165, 50, 214, 0);
			require(impl.execute(std::string("tune ") + colour_paths[index] + " A532D600").ok &&
				colour_values(presentation) == expected_colours && pending->edits().size() == 4,
				"Reapplying a reset silhouette-art edit restores the independent three-sibling reset fixture");
		}
		require(impl.execute("tune.reset presentation").ok && colour_values(presentation) == initial_colours &&
			pending->edits().empty() && pending->current().text() == baseline,
			"Resetting the presentation restores the exact original document and all three silhouette-art defaults");
		unchanged();
		expected_colours = initial_colours;
		const std::array<const char*, 3> saved_words{ "2060C080", "D973A6C0", "8A43C140" };
		std::string saved = baseline;
		for (std::size_t position = 0; position < indices.size(); ++position)
		{
			const std::size_t index = indices[position];
			const std::string local = std::string(colour_paths[index]).substr(std::string("presentation.").size());
			const JsonSpanDocument save_reference(saved, "silhouette-colour-save-reference");
			saved = save_reference.replaced(*save_reference.find(local), std::string("\"") + saved_words[position] + '"');
			expected_colours[index] = labrador::Colour(saved_words[position]);
			require(impl.execute(std::string("tune ") + colour_paths[index] + ' ' + saved_words[position]).ok &&
				colour_values(presentation) == expected_colours, "Each saved silhouette-art colour is independently validated before the private three-token save");
		}
		require(impl.execute("tune.save presentation").ok && read_text(scratch.source / L"presentation.json") == saved &&
			read_text(scratch.staged / L"presentation.json") == saved && pending->edits().empty() &&
			pending->baseline().text() == saved && pending->current().text() == saved,
			"Saving silhouette art changes exactly three quoted tokens and rebases the retained borrowed records");
		unchanged();
		for (const std::size_t index : indices)
		{
			expected_colours[index] = labrador::Colour(165, 50, 214, 0);
			require(impl.execute(std::string("tune ") + colour_paths[index] + " A532D600").ok &&
				colour_values(presentation) == expected_colours, "All three independent silhouette-art edits are pending before invalid merged-width saves");
		}
		const JsonSpanDocument saved_document(saved, "silhouette-colour-merged-reference");
		const JsonLeaf* shade_width = saved_document.find("opening.silhouette_art.shade_width");
		require(shade_width != nullptr && original.silhouette_art.pen_width > 0.0f,
			"The silhouette merged candidate has the actual unrelated pen/shade width constraint");
		const std::string external = saved_document.replaced(*shade_width,
			tune_format(original.silhouette_art.pen_width * 0.5f, saved_document.token(*shade_width)));
		write_text(scratch.source / L"presentation.json", external);
		const std::string pending_text = pending->current().text();
		for (const char* command : { "tune.save presentation", "tune.save presentation force" })
		{
			require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == saved && pending->current().text() == pending_text &&
				pending->baseline().text() == saved && pending->edits().size() == 3 &&
				pending->edits().contains("opening.silhouette_art.ink") && pending->edits().contains("opening.silhouette_art.rim") &&
				pending->edits().contains("opening.silhouette_art.shade") && colour_values(presentation) == expected_colours,
				"Shade width below unchanged pen width rejects the complete merged candidate and retains source/staged/live/ledger, even with force");
			unchanged();
		}
		for (std::size_t position = 0; position < indices.size(); ++position)
		{
			const std::size_t index = indices[position];
			expected_colours[index] = labrador::Colour(saved_words[position]);
			require(impl.execute(std::string("tune.reset ") + colour_paths[index]).ok &&
				colour_values(presentation) == expected_colours && pending->edits().size() == indices.size() - position - 1,
				"After refused saves every silhouette-art colour resets independently to its saved partial-alpha baseline");
			unchanged();
		}
		require(pending->current().text() == saved && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external && read_text(scratch.staged / L"presentation.json") == saved &&
			!std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
			!std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
			!std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
			"Saved silhouette-art resets retain the invalid external source and exact staging without recovery files");
		write_text(scratch.source / L"presentation.json", saved);
		require(std::rand() == expected_random,
			"Silhouette-art colour commands, sibling resets and valid/refused saves consume no gameplay random draw");
	}

	void check_reveal_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"reveal-colour-records");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const OpeningPresentation* const opening = &presentation.opening();
		const RevealArtPresentation* const art = &opening->reveal_art;
		const MenuResultsText* const wrappers = &presentation.menu_text().results;
		const OpeningPresentation original = *opening;
		const auto geometry = [&]
		{
			return std::array<float, 8>{ art->keyline_width, art->padding, art->shadow_offset,
				art->tick_length, art->beat_duration, opening->percent_height,
				opening->cursor_size, opening->palette_height };
		};
		const auto unselected = [&]
		{
			return std::array<labrador::Colour, 10>{ opening->cursor_art.ink, opening->cursor_art.fill,
				opening->cursor_art.shade, opening->cursor_art.shadow, opening->silhouette,
				opening->silhouette_art.ink, opening->silhouette_art.rim, opening->silhouette_art.shade,
				opening->figure_grey, opening->face };
		};
		const auto percentage_labels = [&]
		{
			return std::array<std::wstring, 2>{ wrappers->team_a_percentage_line(L"100.0%"),
				wrappers->team_b_percentage_line(L"0.0%") };
		};
		const auto team_tints = [&]
		{
			std::vector<labrador::Colour> values;
			for (const auto pair : { TeamColourPair::blue_orange, TeamColourPair::pink_green, TeamColourPair::blue_yellow })
			{
				const TeamColour colours = TeamColourTools::team_colours(pair, presentation.team_palette());
				values.insert(values.end(), { colours.team_a, colours.team_b });
			}
			return values;
		};
		const auto held_geometry = geometry();
		const auto held_unselected = unselected();
		const auto held_labels = percentage_labels();
		const auto held_team_tints = team_tints();
		const auto unchanged = [&]
		{
			require(&presentation.opening() == opening && &opening->reveal_art == art &&
				&presentation.menu_text().results == wrappers && geometry() == held_geometry && unselected() == held_unselected &&
				percentage_labels() == held_labels && team_tints() == held_team_tints && opening->glass == original.glass &&
				opening->sprite_sheet == original.sprite_sheet && opening->fill_frame == original.fill_frame &&
				opening->percent_font == original.percent_font && opening->palette_sheet == original.palette_sheet &&
				opening->palette_frames == original.palette_frames,
				"Reveal colour adoption retains borrowed records, unselected material, card geometry/timings, team tints, percentage wrappers and resource names");
		};
		// RevealArt owns no click clock or geometry. Native acceptance retains
		// the consumer and supplies fixed bounds/text/team/age/reduced inputs;
		// this CPU check observes their source records without claiming pixels.
		const std::array<std::size_t, 3> indices{ reveal_ink_colour_index, reveal_paper_colour_index, reveal_shade_colour_index };
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const auto initial_colours = colour_values(presentation);
		auto expected_colours = initial_colours;
		std::srand(0x7e0a29);
		const int expected_random = std::rand();
		std::srand(0x7e0a29);
		for (const std::size_t index : indices)
		{
			for (const auto& [word, expected] : std::array<std::pair<const char*, labrador::Colour>, 4>{ {
				{ "RED", { 255, 0, 0 } }, { "3165A7", { 49, 101, 167 } },
				{ "2060C080", { 32, 96, 192, 128 } }, { "A532D600", { 165, 50, 214, 0 } } } })
			{
				expected_colours[index] = expected;
				require(impl.execute(std::string("tune ") + colour_paths[index] + ' ' + word).ok &&
					colour_values(presentation) == expected_colours && read_text(scratch.source / L"presentation.json") == baseline &&
					read_text(scratch.staged / L"presentation.json") == baseline,
					"Each reveal colour accepts straight named/RGB/partial/nonblack-zero-alpha channels while preserving all other colours and both content copies");
				unchanged();
			}
		}
		const TuneDocument* const pending = impl.tune_document("presentation");
		const std::array<const char*, 3> reset_words{ "2060C080", "D973A6C0", "A532D600" };
		const std::array<labrador::Colour, 3> reset_colours{ labrador::Colour(32, 96, 192, 128),
			labrador::Colour(217, 115, 166, 192), labrador::Colour(165, 50, 214, 0) };
		require(impl.execute("tune presentation.debug_overlay.colour 789ABC").ok,
			"An unrelated live colour edit is pending for each reveal sibling-reset case");
		for (const std::size_t reset_index : indices)
		{
			expected_colours = initial_colours;
			expected_colours[3] = labrador::Colour(120, 154, 188);
			for (std::size_t slot = 0; slot < indices.size(); ++slot)
			{
				require(impl.execute(std::string("tune ") + colour_paths[indices[slot]] + ' ' + reset_words[slot]).ok,
					"Each reveal reset case begins with three distinct pending colour values");
				expected_colours[indices[slot]] = reset_colours[slot];
			}
			expected_colours[reset_index] = initial_colours[reset_index];
			require(impl.execute(std::string("tune.reset ") + colour_paths[reset_index]).ok &&
				colour_values(presentation) == expected_colours && pending->edits().size() == 3 &&
				pending->edits().contains("debug_overlay.colour"),
				"Resetting each reveal field preserves both independently edited siblings and an unrelated colour edit");
			for (const std::size_t sibling : indices)
			{
				const std::string local = std::string(colour_paths[sibling]).substr(std::string("presentation.").size());
				require(pending->edits().contains(local) == (sibling != reset_index),
					"Only the selected reveal sibling leaves the pending ledger during its reset");
			}
			unchanged();
		}
		require(impl.execute("tune.reset presentation").ok && colour_values(presentation) == initial_colours &&
			pending->edits().empty() && pending->current().text() == baseline,
			"Resetting the file restores all reveal fields and the unrelated edit to the exact original baseline");
		unchanged();
		const std::array<const char*, 3> saved_words{ "2060C080", "D973A6C0", "4BA26080" };
		const std::array<labrador::Colour, 3> saved_colours{ labrador::Colour(32, 96, 192, 128),
			labrador::Colour(217, 115, 166, 192), labrador::Colour(75, 162, 96, 128) };
		std::string saved = baseline;
		expected_colours = initial_colours;
		for (std::size_t slot = 0; slot < indices.size(); ++slot)
		{
			const std::string path = colour_paths[indices[slot]];
			const std::string local = path.substr(std::string("presentation.").size());
			const JsonSpanDocument saved_reference(saved, "reveal-colour-save-reference");
			saved = saved_reference.replaced(*saved_reference.find(local), std::string("\"") + saved_words[slot] + '"');
			require(impl.execute("tune " + path + ' ' + saved_words[slot]).ok,
				"A distinct partial-alpha reveal palette is ready for a three-token save");
			expected_colours[indices[slot]] = saved_colours[slot];
		}
		require(impl.execute("tune.save presentation").ok && colour_values(presentation) == expected_colours &&
			read_text(scratch.source / L"presentation.json") == saved && read_text(scratch.staged / L"presentation.json") == saved &&
			pending->edits().empty() && pending->baseline().text() == saved && pending->current().text() == saved,
			"Saving the reveal palette changes exactly three authored tokens and rebases the retained borrowed records");
		unchanged();
		const std::array<const char*, 3> conflict_words{ "A532D600", "RED", "3165A7" };
		const std::array<labrador::Colour, 3> conflict_colours{ labrador::Colour(165, 50, 214, 0),
			labrador::Colour(255, 0, 0), labrador::Colour(49, 101, 167) };
		for (std::size_t slot = 0; slot < indices.size(); ++slot)
		{
			require(impl.execute(std::string("tune ") + colour_paths[indices[slot]] + ' ' + conflict_words[slot]).ok,
				"Three independent reveal edits are pending before complete merged candidate validation");
			expected_colours[indices[slot]] = conflict_colours[slot];
		}
		const JsonSpanDocument saved_document(saved, "reveal-colour-merged-reference");
		const JsonLeaf* padding = saved_document.find("opening.reveal_art.padding");
		require(padding != nullptr && original.reveal_art.padding <= 0.18f,
			"The reveal merged candidate has its real unrelated padding upper bound");
		const std::string external = saved_document.replaced(*padding, "0.181");
		write_text(scratch.source / L"presentation.json", external);
		const std::string pending_text = pending->current().text();
		for (const char* command : { "tune.save presentation", "tune.save presentation force" })
		{
			require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == saved && pending->current().text() == pending_text &&
				pending->baseline().text() == saved && pending->edits().size() == 3 &&
				pending->edits().contains("opening.reveal_art.ink") && pending->edits().contains("opening.reveal_art.paper") &&
				pending->edits().contains("opening.reveal_art.shade") && colour_values(presentation) == expected_colours,
				"An unrelated padding 0.181 rejects the complete merged candidate and retains source/staged/live/current/baseline/ledger, even with force");
			unchanged();
		}
		for (std::size_t slot = 0; slot < indices.size(); ++slot)
		{
			expected_colours[indices[slot]] = saved_colours[slot];
			require(impl.execute(std::string("tune.reset ") + colour_paths[indices[slot]]).ok &&
				colour_values(presentation) == expected_colours && pending->edits().size() == indices.size() - slot - 1,
				"After rejected merged saves each reveal field resets to its saved partial-alpha baseline while retaining pending siblings");
			unchanged();
		}
		require(pending->edits().empty() && pending->current().text() == saved && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external && read_text(scratch.staged / L"presentation.json") == saved &&
			!std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
			!std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
			!std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
			"Reveal resets retain the exact saved baseline and invalid external source without leaving recovery files");
		write_text(scratch.source / L"presentation.json", saved);
		require(std::rand() == expected_random,
			"Reveal colour edits, sibling resets and valid/refused saves consume no gameplay random draw");
	}

	void check_cursor_material_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"cursor-material-colour-records");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const OpeningPresentation* const opening = &presentation.opening();
		const CursorArtPresentation* const art = &opening->cursor_art;
		const OpeningPresentation original = *opening;
		const auto geometry = [&]
		{
			return std::vector<float>{ art->shadow_offset, art->click_duration, art->click_reach,
				opening->silhouette_art.pen_width, opening->silhouette_art.shade_width, opening->palette_height,
				opening->adult_height, opening->child_height, opening->cursor_size, opening->preview_strength,
				opening->percent_height, opening->hop, opening->shake, opening->slump,
				opening->monitor_art.pen_width, opening->monitor_art.corner_step, opening->monitor_art.shadow_width,
				static_cast<float>(opening->monitor_art.vent_count), opening->reveal_art.keyline_width,
				opening->reveal_art.padding, opening->reveal_art.shadow_offset,
				opening->reveal_art.tick_length, opening->reveal_art.beat_duration };
		};
		const auto unselected = [&]
		{
			return std::vector<labrador::Colour>{ art->ink, art->fill, opening->silhouette,
				opening->silhouette_art.ink, opening->silhouette_art.rim, opening->silhouette_art.shade,
				opening->bezel_shadow, opening->monitor_art.ink, opening->monitor_art.highlight,
				opening->monitor_art.power_light, opening->glass_colour, opening->program_colour,
				opening->desktop_colour, opening->slot_outline, opening->scribble, opening->coffee, opening->crack,
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
		const auto unchanged = [&]
		{
			require(&presentation.opening() == opening && &opening->cursor_art == art &&
				geometry() == held_geometry && unselected() == held_unselected && rectangles() == held_rectangles &&
				opening->bezel_shades == original.bezel_shades && opening->sticky_notes == original.sticky_notes &&
				opening->stickers == original.stickers && opening->sprite_sheet == original.sprite_sheet &&
				opening->fill_frame == original.fill_frame && opening->percent_font == original.percent_font &&
				opening->palette_sheet == original.palette_sheet && opening->palette_frames == original.palette_frames,
				"Cursor material adoption retains borrowed records, every unselected opening colour including ink/fill, geometry/timings and captured resource names");
		};
		// ScreenCursor owns no click clock or geometry. Native acceptance retains
		// the consumer and supplies fixed tip/height/age/reduced inputs;
		// this CPU check observes their source records without claiming pixels.
		const std::array<std::size_t, 2> indices{ cursor_shade_colour_index, cursor_shadow_colour_index };
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const auto initial_colours = colour_values(presentation);
		auto expected_colours = initial_colours;
		std::srand(0xc053031);
		const int expected_random = std::rand();
		std::srand(0xc053031);
		for (const std::size_t index : indices)
		{
			for (const auto& [word, expected] : std::array<std::pair<const char*, labrador::Colour>, 4>{ {
				{ "RED", { 255, 0, 0 } }, { "3165A7", { 49, 101, 167 } },
				{ "2060C080", { 32, 96, 192, 128 } }, { "A532D600", { 165, 50, 214, 0 } } } })
			{
				expected_colours[index] = expected;
				require(impl.execute(std::string("tune ") + colour_paths[index] + ' ' + word).ok &&
					colour_values(presentation) == expected_colours && read_text(scratch.source / L"presentation.json") == baseline &&
					read_text(scratch.staged / L"presentation.json") == baseline,
					"Each cursor-material colour accepts straight named/RGB/partial/nonblack-zero-alpha channels while preserving all other colours and both content copies");
				unchanged();
			}
		}
		const TuneDocument* const pending = impl.tune_document("presentation");
		const std::array<const char*, 2> reset_words{ "2060C080", "A532D600" };
		const std::array<labrador::Colour, 2> reset_colours{ labrador::Colour(32, 96, 192, 128),
			labrador::Colour(165, 50, 214, 0) };
		require(impl.execute("tune presentation.debug_overlay.colour 789ABC").ok,
			"An unrelated live colour edit is pending for each cursor-material sibling-reset case");
		for (const std::size_t reset_index : indices)
		{
			expected_colours = initial_colours;
			expected_colours[3] = labrador::Colour(120, 154, 188);
			for (std::size_t slot = 0; slot < indices.size(); ++slot)
			{
				require(impl.execute(std::string("tune ") + colour_paths[indices[slot]] + ' ' + reset_words[slot]).ok,
					"Each cursor-material reset case begins with two distinct pending colour values");
				expected_colours[indices[slot]] = reset_colours[slot];
			}
			expected_colours[reset_index] = initial_colours[reset_index];
			require(impl.execute(std::string("tune.reset ") + colour_paths[reset_index]).ok &&
				colour_values(presentation) == expected_colours && pending->edits().size() == indices.size() &&
				pending->edits().contains("debug_overlay.colour"),
				"Resetting each cursor-material field preserves the independently edited sibling and an unrelated colour edit");
			for (const std::size_t sibling : indices)
			{
				const std::string local = std::string(colour_paths[sibling]).substr(std::string("presentation.").size());
				require(pending->edits().contains(local) == (sibling != reset_index),
					"Only the selected cursor-material sibling leaves the pending ledger during its reset");
			}
			unchanged();
		}
		require(impl.execute("tune.reset presentation").ok && colour_values(presentation) == initial_colours &&
			pending->edits().empty() && pending->current().text() == baseline,
			"Resetting the file restores all cursor-material fields and the unrelated edit to the exact original baseline");
		unchanged();
		const std::array<const char*, 2> saved_words{ "2060C080", "4BA26080" };
		const std::array<labrador::Colour, 2> saved_colours{ labrador::Colour(32, 96, 192, 128),
			labrador::Colour(75, 162, 96, 128) };
		std::string saved = baseline;
		expected_colours = initial_colours;
		for (std::size_t slot = 0; slot < indices.size(); ++slot)
		{
			const std::string path = colour_paths[indices[slot]];
			const std::string local = path.substr(std::string("presentation.").size());
			const JsonSpanDocument saved_reference(saved, "cursor-material-colour-save-reference");
			saved = saved_reference.replaced(*saved_reference.find(local), std::string("\"") + saved_words[slot] + '"');
			require(impl.execute("tune " + path + ' ' + saved_words[slot]).ok,
				"A distinct partial-alpha cursor-material palette is ready for a two-token save");
			expected_colours[indices[slot]] = saved_colours[slot];
		}
		require(impl.execute("tune.save presentation").ok && colour_values(presentation) == expected_colours &&
			read_text(scratch.source / L"presentation.json") == saved && read_text(scratch.staged / L"presentation.json") == saved &&
			pending->edits().empty() && pending->baseline().text() == saved && pending->current().text() == saved,
			"Saving the cursor-material palette changes exactly two authored tokens and rebases the retained borrowed records");
		unchanged();
		const std::array<const char*, 2> conflict_words{ "A532D600", "RED" };
		const std::array<labrador::Colour, 2> conflict_colours{ labrador::Colour(165, 50, 214, 0),
			labrador::Colour(255, 0, 0) };
		for (std::size_t slot = 0; slot < indices.size(); ++slot)
		{
			require(impl.execute(std::string("tune ") + colour_paths[indices[slot]] + ' ' + conflict_words[slot]).ok,
				"Two independent cursor-material edits are pending before complete merged candidate validation");
			expected_colours[indices[slot]] = conflict_colours[slot];
		}
		const JsonSpanDocument saved_document(saved, "cursor-material-colour-merged-reference");
		const JsonLeaf* shadow_offset = saved_document.find("opening.cursor_art.shadow_offset");
		require(shadow_offset != nullptr && original.cursor_art.shadow_offset <= 0.2f,
			"The cursor-material merged candidate has its real unrelated shadow-offset upper bound");
		const std::string external = saved_document.replaced(*shadow_offset, "0.201");
		write_text(scratch.source / L"presentation.json", external);
		const std::string pending_text = pending->current().text();
		const auto ledger = [&]
		{
			std::vector<std::string> result;
			for (const auto& [path, edit] : pending->edits())
				{ result.push_back(path + "=" + edit.baseline_token + "->" + edit.current_token); }
			return result;
		};
		const auto pending_edits = ledger();
		for (const char* command : { "tune.save presentation", "tune.save presentation force" })
		{
			require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == saved && pending->current().text() == pending_text &&
				pending->baseline().text() == saved && ledger() == pending_edits && pending->edits().size() == indices.size() &&
				pending->edits().contains("opening.cursor_art.shade") && pending->edits().contains("opening.cursor_art.shadow") && colour_values(presentation) == expected_colours,
				"An unrelated shadow_offset 0.201 rejects the complete merged candidate and retains source/staged/live/current/baseline/ledger, even with force");
			unchanged();
		}
		for (std::size_t slot = 0; slot < indices.size(); ++slot)
		{
			expected_colours[indices[slot]] = saved_colours[slot];
			require(impl.execute(std::string("tune.reset ") + colour_paths[indices[slot]]).ok &&
				colour_values(presentation) == expected_colours && pending->edits().size() == indices.size() - slot - 1,
				"After rejected merged saves each cursor-material field resets to its saved partial-alpha baseline while retaining pending siblings");
			unchanged();
		}
		require(pending->edits().empty() && pending->current().text() == saved && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external && read_text(scratch.staged / L"presentation.json") == saved &&
			!std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
			!std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
			!std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
			"Cursor-material resets retain the exact saved baseline and invalid external source without leaving recovery files");
		write_text(scratch.source / L"presentation.json", saved);
		require(std::rand() == expected_random,
			"Cursor-material colour edits, sibling resets and valid/refused saves consume no gameplay random draw");
	}
