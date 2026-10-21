// Included inside tune_checks.cpp's private namespace, after its includes.
// Opening crack, figure arm/hand and acting face/sweat colours.

	void check_crack_colour_tunes(const std::filesystem::path& game_directory)
	{
		// Named/RGB/RGBA, strict aliases, LF/CRLF saves/conflicts and type
		// consumption reuse check_colour_tunes/check_tune_consumed's42-path loops.
		ScratchContent scratch(game_directory, L"crack-colour-records");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);auto& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const OpeningPresentation* const opening = &presentation.opening();
		const OpeningPresentation original = *opening;
		const auto* const art = &opening->monitor_art;
		const auto* const cursor = &opening->cursor_art;
		const auto* const silhouette = &opening->silhouette_art;
		const auto* const reveal = &opening->reveal_art;
		labrador::RenderResources resources;
		for (const auto& sheet : { opening->sprite_sheet, opening->palette_sheet })
			{ resources.add_sprite_sheet(sheet, labrador::read_sprite_sheet((game_directory / L"textures" / (sheet + ".json")).string().c_str(), labrador::TextureHandle{})); }
		MonitorPersona persona;persona.shade = 0;persona.mug = true;
		persona.coffee_ring = { .2f, .7f };
		persona.sticky_notes = { { { .015f, .47f }, 0, 0.0f } };
		persona.stickers = { { { .8f, .8f }, 0, .1f } };
		persona.crack = { { .004f, .70f }, { .024f, .68f }, { .048f, .66f }, { .070f, .68f }, { .090f, .66f } };
		const Monitor monitor(opening, &resources, persona);
		const auto* const held_persona = &monitor.persona();
		const auto* const sheet = resources.sprite_sheet(opening->sprite_sheet);
		const auto* const palette = resources.sprite_sheet(opening->palette_sheet);
		const auto fill = sheet->resolve_sprite_frame(opening->fill_frame);
		const auto fill_source = sheet->sprite_frame(fill).source_rectangle();
		const auto fill_origin = sheet->sprite_frame(fill).origin();
		const auto palette_frames = [&]
		{
			std::vector<labrador::SpriteSheet::frame_handle> result;
			for (const auto& name : opening->palette_frames) { result.push_back(palette->resolve_sprite_frame(name)); }
			return result;
		};
		const auto held_frames = palette_frames();
		const auto geometry = [&]
		{
			return std::vector<float>{ cursor->shadow_offset, cursor->click_duration, cursor->click_reach,
				silhouette->pen_width, silhouette->shade_width, opening->palette_height, opening->adult_height,
				opening->child_height, opening->cursor_size, opening->preview_strength, opening->percent_height,
				opening->hop, opening->shake, opening->slump, art->pen_width, art->corner_step, art->shadow_width,
				static_cast<float>(art->vent_count), reveal->keyline_width, reveal->padding,
				reveal->shadow_offset, reveal->tick_length, reveal->beat_duration };
		};
		const auto unselected = [&]
		{
			return std::vector<labrador::Colour>{ cursor->ink, cursor->fill, cursor->shade, cursor->shadow,
				opening->silhouette, silhouette->ink, silhouette->rim, silhouette->shade, opening->bezel_shadow,
				art->ink, art->highlight, art->power_light, opening->desktop_colour, opening->program_colour,
				opening->slot_outline, opening->glass_colour, opening->coffee, opening->scribble,
				reveal->ink, reveal->paper, reveal->shade, opening->figure_grey, opening->face, opening->sweat };
		};
		const mattmath::RectangleF screen(0, 0, 1280, 720);
		const auto rectangles = [&]
		{
			return std::vector<mattmath::RectangleF>{ opening->glass, art->vent_area, art->power_button,
				art->power_indicator, screen, monitor.glass(screen), monitor.picture(screen), monitor.swatch(screen, 0),
				monitor.swatch(screen, 1), monitor.empty_slot(screen), monitor.close_button(screen), monitor.screen_for_glass(screen) };
		};
		const auto persona_values = [](const MonitorPersona& p)
		{
			std::vector<float> values{ static_cast<float>(p.shade), p.child ? 1.0f : 0.0f,
				p.mug ? 1.0f : 0.0f, p.coffee_ring.x, p.coffee_ring.y };
			for (const auto* decorations : { &p.sticky_notes, &p.stickers })
			{
				values.push_back(static_cast<float>(decorations->size()));
				for (const auto& d : *decorations) { values.insert(values.end(), { d.at.x, d.at.y, static_cast<float>(d.colour), d.tilt }); }
			}
			values.push_back(static_cast<float>(p.crack.size()));
			for (const auto& point : p.crack) { values.insert(values.end(), { point.x, point.y }); }
			return values;
		};
		const auto held_geometry = geometry(), held_persona_values = persona_values(persona);
		const auto held_unselected = unselected();const auto held_rectangles = rectangles();
		require(original.crack == labrador::Colour(58, 58, 58) && art->pen_width == .004f &&
			persona_values(*held_persona) == held_persona_values && held_persona->crack.size() == 5,
			"Crack CPU fixture retains the authored tint and an explicit copied four-segment path without persona rolling");
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const std::string tuning_bytes = read_text(scratch.source / L"tuning.json");
		const auto initial = colour_values(presentation);
		const auto unchanged = [&]
		{
			require(&presentation.opening() == opening && &opening->monitor_art == art && &opening->cursor_art == cursor &&
				&opening->silhouette_art == silhouette && &opening->reveal_art == reveal && &monitor.persona() == held_persona &&
				geometry() == held_geometry && unselected() == held_unselected && rectangles() == held_rectangles &&
				persona_values(persona) == held_persona_values && persona_values(*held_persona) == held_persona_values &&
				opening->bezel_shades == original.bezel_shades && opening->sticky_notes == original.sticky_notes &&
				opening->stickers == original.stickers && opening->sprite_sheet == original.sprite_sheet &&
				opening->fill_frame == original.fill_frame && opening->percent_font == original.percent_font &&
				opening->palette_sheet == original.palette_sheet && opening->palette_frames == original.palette_frames &&
				resources.sprite_sheet(opening->sprite_sheet) == sheet && resources.sprite_sheet(opening->palette_sheet) == palette &&
				sheet->resolve_sprite_frame(opening->fill_frame) == fill && sheet->sprite_frame(fill).source_rectangle() == fill_source &&
				sheet->sprite_frame(fill).origin() == fill_origin && palette_frames() == held_frames &&
				read_text(scratch.source / L"tuning.json") == tuning_bytes && read_text(scratch.staged / L"tuning.json") == tuning_bytes &&
				!impl.in_match() && !impl.can_apply() && impl.request().empty(),
				"Crack tuning preserves the retained Monitor, full unselected borrowed opening/nested records, arrays/resources/public frames/geometry and captured plus caller persona values");
		};
		// Real retained Monitor and null-texture metadata only: no private captured
		// handle inspection, draw, pixel, repeated-dab, Level or human claim.
		std::srand(0x4c4242);const int next_random = std::rand();std::srand(0x4c4242);
		const std::string path = colour_paths[crack_colour_index];
		require(impl.execute("tune " + path + " 2060C080").ok && impl.execute("tune presentation.debug_overlay.colour 789ABC").ok &&
			impl.execute("tune.reset " + path).ok && opening->crack == original.crack &&
			impl.tune_document("presentation")->edits().size() == 1 && impl.tune_document("presentation")->edits().contains("debug_overlay.colour"),
			"Crack original reset retains the unrelated pending edit");unchanged();
		require(impl.execute("tune.reset presentation.debug_overlay.colour").ok && colour_values(presentation) == initial,
			"Independent reset retains all41 prior colours");
		const JsonSpanDocument original_json(baseline, "crack-save-reference");
		const std::string saved = original_json.replaced(*original_json.find("opening.crack"), "\"2060C080\"");
		const auto* const pending = impl.tune_document("presentation");
		require(impl.execute("tune " + path + " 2060C080").ok && impl.execute("tune.save presentation").ok &&
			opening->crack == labrador::Colour(32, 96, 192, 128) && pending->edits().empty() &&
			pending->baseline().text() == saved && pending->current().text() == saved &&
			read_text(scratch.source / L"presentation.json") == saved && read_text(scratch.staged / L"presentation.json") == saved,
			"Crack Save replaces exactly one token and rebases the same borrowed record");unchanged();
		require(impl.execute("tune " + path + " A532D600").ok && impl.execute("tune presentation.debug_overlay.colour 789ABC").ok,
			"Crack and unrelated colour edits are pending before complete-reader refusal");
		const JsonSpanDocument saved_json(saved, "crack-merge-reference");
		const std::string external = saved_json.replaced(*saved_json.find("opening.monitor_art.corner_step"), "0.002");
		write_text(scratch.source / L"presentation.json", external);
		const auto live = colour_values(presentation);const std::string current = pending->current().text();
		const auto ledger = [&]
		{
			std::vector<std::array<std::string, 3>> values;
			for (const auto& [key, edit] : pending->edits()) { values.push_back({ key, edit.baseline_token, edit.current_token }); }
			return values;
		};
		const auto held_ledger = ledger();
		for (const char* command : { "tune.save presentation", "tune.save presentation force" })
		{
			require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == saved && pending->baseline().text() == saved &&
				pending->current().text() == current && colour_values(presentation) == live && ledger() == held_ledger &&
				pending->edits().size() == 2 && pending->edits().contains("opening.crack") && pending->edits().contains("debug_overlay.colour"),
				"Corner_step0.002 below pen_width0.004 refuses ordinary/forced complete merges with exact source/staged/live/current/baseline and BOTH ledger keys/baseline/current tokens");unchanged();
		}
		auto expected = live;expected[crack_colour_index] = labrador::Colour(32, 96, 192, 128);
		const std::string overlay_only = saved_json.replaced(*saved_json.find("debug_overlay.colour"), "\"789ABC\"");
		require(impl.execute("tune.reset " + path).ok && colour_values(presentation) == expected &&
			pending->edits().size() == 1 && pending->edits().contains("debug_overlay.colour") &&
			pending->current().text() == overlay_only && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external && read_text(scratch.staged / L"presentation.json") == saved,
			"Crack saved reset retains unrelated edit and the invalid external source plus exact staged bytes");unchanged();
		require(!std::filesystem::exists(scratch.source / L"presentation.json.tmp") && !std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
			!std::filesystem::exists(scratch.staged / L"presentation.json.tmp") && std::rand() == next_random,
			"Crack commands/refused saves retain RNG and leave no recovery files");
	}

	void check_limb_colour_tunes(const std::filesystem::path& game_directory)
	{
		// Generic named/RGB/RGBA/strict words, exact LF/CRLF saves/conflicts,
		// force and actual reader consumption already cover all44 paths.
		ScratchContent scratch(game_directory, L"limb-colour-records");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);auto& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const FigurePresentation* const look = &presentation.figure();
		const FigurePresentation original = *look;
		const auto* const quiet = &look->quiet;
		const MovementFeedbackPresentation* const fill = &presentation.movement_feedback();
		const std::string fill_sheet_name = fill->sprite_sheet, fill_frame_name = fill->fill_frame;
		labrador::RenderResources resources;
		for (const auto& sheet_name : { fill_sheet_name, original.sprite_sheet })
			{ resources.add_sprite_sheet(sheet_name, labrador::read_sprite_sheet((game_directory / L"textures" / (sheet_name + ".json")).string().c_str(), labrador::TextureHandle{})); }
		const FigureAttachments attachments(fill, look, &resources);
		const auto* const fill_sheet = resources.sprite_sheet(fill_sheet_name);
		const auto* const art_sheet = resources.sprite_sheet(original.sprite_sheet);
		const auto fill_handle = fill_sheet->resolve_sprite_frame(fill_frame_name);
		const auto fill_source = fill_sheet->sprite_frame(fill_handle).source_rectangle();
		const auto fill_origin = fill_sheet->sprite_frame(fill_handle).origin();
		const std::array<std::string, 4> art_names{ original.rest_arms_frame, original.rest_arms_paint_frame,
			original.sleeves_frame, original.sleeves_paint_frame };
		const auto art_handles = [&]
		{
			std::vector<labrador::SpriteSheet::frame_handle> result;
			for (const auto& frame_name : art_names) { result.push_back(art_sheet->resolve_sprite_frame(frame_name)); }
			return result;
		};
		const auto art_metadata = [&]
		{
			std::vector<float> result;
			for (const auto& handle : art_handles())
			{
				const auto& frame = art_sheet->sprite_frame(handle);const auto& box = frame.source_rectangle();const auto& origin = frame.origin();
				result.insert(result.end(), { static_cast<float>(box.x), static_cast<float>(box.y), static_cast<float>(box.width), static_cast<float>(box.height), origin.x, origin.y });
			}
			return result;
		};
		const auto geometry = [&]
		{
			std::vector<float> result{ look->canvas_size.x, look->canvas_size.y, look->body_inset,
				look->rear_shoulder.x, look->rear_shoulder.y, look->front_shoulder.x, look->front_shoulder.y,
				look->arm_width, look->elbow_drop, look->hand_size, look->hand_edge,
				quiet->settle_time, quiet->breath_period, quiet->breath_height, quiet->shift_period, quiet->shift_distance,
				quiet->blink_min_interval, quiet->blink_max_interval, quiet->blink_duration, quiet->lean_distance,
				quiet->lean_acceleration, quiet->lean_settle_time, quiet->eyelid_height };
			for (const auto& eye : quiet->eyes) { result.insert(result.end(), { eye.x, eye.y, eye.width, eye.height }); }
			for (const auto& grip : look->grips) { result.insert(result.end(), { grip.rear.x, grip.rear.y, grip.front.x, grip.front.y }); }
			return result;
		};
		const mattmath::RectangleF body(100, 120, 104, 240);
		const auto public_geometry = [&]
		{
			const auto canvas = look->canvas_rectangle(body);
			return std::array<mattmath::RectangleF, 3>{ canvas, attachments.bounds(canvas, true, nullptr), attachments.bounds(canvas, false, nullptr) };
		};
		const auto held_geometry = geometry(), held_art_metadata = art_metadata();
		const auto held_handles = art_handles();const auto held_public_geometry = public_geometry();
		const auto initial_colours = colour_values(presentation);auto expected_colours = initial_colours;
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const std::string tuning_bytes = read_text(scratch.source / L"tuning.json");
		const auto unchanged = [&]
		{
			require(&presentation.figure() == look && &look->quiet == quiet && &presentation.movement_feedback() == fill &&
				geometry() == held_geometry && public_geometry() == held_public_geometry &&
				look->sprite_sheet == original.sprite_sheet && look->rest_arms_frame == original.rest_arms_frame &&
				look->rest_arms_paint_frame == original.rest_arms_paint_frame && look->sleeves_frame == original.sleeves_frame &&
				look->sleeves_paint_frame == original.sleeves_paint_frame && quiet->enabled == original.quiet.enabled &&
				quiet->face_colour == original.quiet.face_colour && fill->sprite_sheet == fill_sheet_name && fill->fill_frame == fill_frame_name &&
				resources.sprite_sheet(fill_sheet_name) == fill_sheet && resources.sprite_sheet(original.sprite_sheet) == art_sheet &&
				fill_sheet->resolve_sprite_frame(fill_frame_name) == fill_handle && fill_sheet->sprite_frame(fill_handle).source_rectangle() == fill_source &&
				fill_sheet->sprite_frame(fill_handle).origin() == fill_origin && art_handles() == held_handles && art_metadata() == held_art_metadata &&
				colour_values(presentation) == expected_colours && read_text(scratch.source / L"tuning.json") == tuning_bytes &&
				read_text(scratch.staged / L"tuning.json") == tuning_bytes && !impl.in_match() && !impl.can_apply() && impl.request().empty(),
				"Limb commands retain actual FigureAttachments and borrowed Figure/quiet records, every unselected geometry/grip/quiet/name value, public null-tool bounds, external atlas/frame metadata and unrelated colours/queues");
		};
		// Retained real consumer plus null-texture public metadata only. Private
		// captured handles, held-tool bounds, draw/alpha/blink pixels and Level
		// behavior are deliberately not asserted by this CPU fixture.
		require(original.arm_colour == labrador::Colour::black && original.hand_colour == labrador::Colour::white &&
			held_handles.size() == 4 && quiet->eyes.size() == 2 && look->grips.size() == 5,
			"Limb CPU fixture uses authored BLACK/WHITE and all four rest/sleeve frames, two eyes and five tool grips");
		std::srand(0x4c494d42);const int next_random = std::rand();std::srand(0x4c494d42);
		const std::array<std::size_t, 2> indices{ limb_arm_colour_index, limb_hand_colour_index };
		const std::array<const char*, 2> words{ "2060C080", "A532D600" };
		const auto* const pending = impl.tune_document("presentation");
		require(pending != nullptr, "The limb fixture loads its valid presentation document");
		const char* const unrelated_word = initial_colours[3] == labrador::Colour(120, 154, 188) ? "654321" : "789ABC";
		for (std::size_t position = 0; position < indices.size(); ++position)
		{
			const auto index = indices[position];expected_colours[index] = labrador::Colour(words[position]);
			require(impl.execute(std::string("tune ") + colour_paths[index] + ' ' + words[position]).ok,
				"Both selected limb edits are pending independently");unchanged();
		}
		expected_colours[3] = labrador::Colour(unrelated_word);
		require(impl.execute(std::string("tune presentation.debug_overlay.colour ") + unrelated_word).ok && pending->edits().size() == 3,
			"Both limb edits coexist with an unrelated pending colour");unchanged();
		for (std::size_t position = 0; position < indices.size(); ++position)
		{
			const auto index = indices[position], sibling = indices[1 - position];
			const std::string local = std::string(colour_paths[index]).substr(std::string("presentation.").size());
			const std::string sibling_local = std::string(colour_paths[sibling]).substr(std::string("presentation.").size());
			expected_colours[index] = initial_colours[index];
			require(impl.execute(std::string("tune.reset ") + colour_paths[index]).ok && pending->edits().size() == 2 &&
				!pending->edits().contains(local) && pending->edits().contains(sibling_local) && pending->edits().contains("debug_overlay.colour") &&
				read_text(scratch.source / L"presentation.json") == baseline && read_text(scratch.staged / L"presentation.json") == baseline,
				"Each original limb reset retains its sibling and unrelated edit without writing source/staging");unchanged();
			expected_colours[index] = labrador::Colour(words[position]);
			require(impl.execute(std::string("tune ") + colour_paths[index] + ' ' + words[position]).ok && pending->edits().size() == 3,
				"Independent limb reset fixture is restored");unchanged();
		}
		expected_colours = initial_colours;
		require(impl.execute("tune.reset presentation").ok && pending->edits().empty(), "Limb reset-all restores original tokens before exact one-token saves");unchanged();
		std::string saved = baseline;
		const std::array<const char*, 2> saved_words{ "3165A780", "2060C080" };
		for (std::size_t position = 0; position < indices.size(); ++position)
		{
			const auto index = indices[position];const std::string local = std::string(colour_paths[index]).substr(std::string("presentation.").size());
			const JsonSpanDocument save_reference(saved, "limb-one-token-reference");
			saved = save_reference.replaced(*save_reference.find(local), std::string("\"") + saved_words[position] + '"');
			expected_colours[index] = labrador::Colour(saved_words[position]);
			require(impl.execute(std::string("tune ") + colour_paths[index] + ' ' + saved_words[position]).ok && pending->edits().size() == 1 &&
				pending->edits().contains(local) && impl.execute("tune.save presentation").ok && pending->edits().empty() &&
				pending->baseline().text() == saved && pending->current().text() == saved &&
				read_text(scratch.source / L"presentation.json") == saved && read_text(scratch.staged / L"presentation.json") == saved,
				"Each limb Save changes exactly its quoted token, retains its sibling, and rebases exact source/staged/current/baseline bytes");unchanged();
		}
		for (const auto index : indices)
		{
			expected_colours[index] = labrador::Colour(165, 50, 214, 0);
			require(impl.execute(std::string("tune ") + colour_paths[index] + " A532D600").ok, "Both saved limb baselines now have independent pending edits");unchanged();
		}
		expected_colours[3] = labrador::Colour(unrelated_word);
		require(impl.execute(std::string("tune presentation.debug_overlay.colour ") + unrelated_word).ok && pending->edits().size() == 3,
			"Both selected fields and unrelated colour are retained across complete-reader failures");unchanged();
		const std::string current = pending->current().text();
		const auto ledger = [&]
		{
			std::vector<std::array<std::string, 3>> values;
			for (const auto& [key, edit] : pending->edits()) { values.push_back({ key, edit.baseline_token, edit.current_token }); }
			return values;
		};
		const auto held_ledger = ledger();const JsonSpanDocument merged_reference(saved, "limb-invalid-merge-reference");
		std::string last_external;
		for (const auto& [field, token] : std::array<std::pair<const char*, std::string>, 2>{ {
			{ "figure.hand_edge", std::string(merged_reference.token(*merged_reference.find("figure.hand_size"))) },
			{ "figure.quiet.eyelid_height", std::string(merged_reference.token(*merged_reference.find("figure.quiet.eyes[0].height"))) } } })
		{
			last_external = merged_reference.replaced(*merged_reference.find(field), token);
			write_text(scratch.source / L"presentation.json", last_external);
			for (const char* command : { "tune.save presentation", "tune.save presentation force" })
			{
				require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == last_external &&
					read_text(scratch.staged / L"presentation.json") == saved && pending->current().text() == current && pending->baseline().text() == saved &&
					ledger() == held_ledger && pending->edits().size() == 3 && pending->edits().contains("figure.arm_colour") &&
					pending->edits().contains("figure.hand_colour") && pending->edits().contains("debug_overlay.colour"),
					"Each positive scalar but invalid hand-interior/eye-lid relationship rejects ordinary and forced full-reader merges, retaining source/staging/live/current/baseline and every selected/sibling/unrelated ledger key plus both token strings");unchanged();
			}
		}
		for (std::size_t position = 0; position < indices.size(); ++position)
		{
			const auto index = indices[position];expected_colours[index] = labrador::Colour(saved_words[position]);
			require(impl.execute(std::string("tune.reset ") + colour_paths[index]).ok && pending->edits().size() == indices.size() - position &&
				pending->edits().contains("debug_overlay.colour") && pending->baseline().text() == saved &&
				read_text(scratch.source / L"presentation.json") == last_external && read_text(scratch.staged / L"presentation.json") == saved,
				"Each limb resets to its own saved partial-alpha token while sibling/unrelated edits and invalid external source remain");unchanged();
		}
		expected_colours[3] = initial_colours[3];
		require(impl.execute("tune.reset presentation.debug_overlay.colour").ok && pending->edits().empty() &&
			pending->current().text() == saved && pending->baseline().text() == saved,
			"Final unrelated reset retains both independently saved limb baselines");unchanged();
		require(!std::filesystem::exists(scratch.source / L"presentation.json.tmp") && !std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
			!std::filesystem::exists(scratch.staged / L"presentation.json.tmp") && std::rand() == next_random,
			"Limb command/save/reset/refusal sequence leaves no recovery files or gameplay RNG consumption");
	}

	void check_acting_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"acting-colour-records");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const OpeningPresentation* const opening = &presentation.opening();
		const RespawnRedrawPresentation* const redraw = &presentation.respawn_redraw();
		const OpeningPresentation original = *opening;
		const RespawnRedrawPresentation original_redraw = *redraw;
		labrador::RenderResources resources;
		resources.add_sprite_sheet(opening->sprite_sheet, labrador::read_sprite_sheet(
			(game_directory / L"textures" / (opening->sprite_sheet + ".json")).string().c_str(), labrador::TextureHandle{}));
		const FigureActing acting(opening, redraw, &resources);
		static_cast<void>(acting);
		// Keep the real consumer alive across every adoption. CPU observations
		// cover its borrowed records and caller inputs; native checks observe
		// rendered adoption. Protected captured handles and pixels are not read.
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
				opening->desktop_colour, opening->glass_colour, opening->program_colour, opening->monitor_art.power_light,
				opening->slot_outline, opening->scribble, opening->coffee, opening->crack,
				opening->reveal_art.ink, opening->reveal_art.paper, opening->reveal_art.shade, opening->figure_grey };
		};
		const auto rectangles = [&]
		{
			return std::array<mattmath::RectangleF, 4>{ opening->glass, opening->monitor_art.vent_area,
				opening->monitor_art.power_button, opening->monitor_art.power_indicator };
		};
		const auto stroke_parts = [&]
		{
			std::vector<std::string> result;
			for (const RespawnStroke& stroke : redraw->strokes) { result.push_back(stroke.part); }
			return result;
		};
		const auto stroke_geometry = [&]
		{
			std::vector<float> result{ redraw->canvas_size.x, redraw->canvas_size.y, redraw->pen_width,
				redraw->draw_from, redraw->fill_at, redraw->camera_hold, static_cast<float>(redraw->strokes.size()) };
			for (const RespawnStroke& stroke : redraw->strokes)
			{
				result.push_back(static_cast<float>(stroke.points.size()));
				for (const auto& point : stroke.points) { result.insert(result.end(), { point.x, point.y }); }
			}
			return result;
		};
		const std::array<mattmath::RectangleF, 2> figures{
			mattmath::RectangleF(100, 100, 340, 600), mattmath::RectangleF(500, 80, 170, 300) };
		const std::array<mattmath::Vector2F, 2> glances{ mattmath::Vector2F(0.6f, 1.0f), mattmath::Vector2F(-0.6f, 1.0f) };
		const std::array<float, 2> times{ 0.2f, 0.6f };
		const std::array<bool, 2> facings{ true, false };
		const auto inputs = [&]
		{
			std::vector<float> result;
			for (std::size_t index = 0; index < figures.size(); ++index)
			{
				const auto& figure = figures[index];
				result.insert(result.end(), { figure.x, figure.y, figure.width, figure.height,
					glances[index].x, glances[index].y, times[index], facings[index] ? 1.0f : 0.0f });
			}
			return result;
		};
		const auto held_geometry = geometry();
		const auto held_unselected = unselected();
		const auto held_rectangles = rectangles();
		const auto held_parts = stroke_parts();
		const auto held_stroke_geometry = stroke_geometry();
		const auto held_inputs = inputs();
		const auto unchanged = [&]
		{
			require(&presentation.opening() == opening && &presentation.respawn_redraw() == redraw &&
				geometry() == held_geometry && unselected() == held_unselected && rectangles() == held_rectangles &&
				stroke_parts() == held_parts && stroke_geometry() == held_stroke_geometry && inputs() == held_inputs &&
				redraw->pen_colour == original_redraw.pen_colour && redraw->sprite_sheet == original_redraw.sprite_sheet &&
				redraw->pen_frame == original_redraw.pen_frame && opening->bezel_shades == original.bezel_shades &&
				opening->sticky_notes == original.sticky_notes && opening->stickers == original.stickers &&
				opening->sprite_sheet == original.sprite_sheet && opening->fill_frame == original.fill_frame &&
				opening->percent_font == original.percent_font && opening->palette_sheet == original.palette_sheet &&
				opening->palette_frames == original.palette_frames,
				"Acting colour adoption retains both borrowed record addresses, all ordered stroke parts/points/pen/canvas, unrelated opening style/resources/geometry and caller-owned draw inputs");
		};
		const auto ledger = [&]
		{
			std::vector<std::string> result;
			for (const auto& [path, edit] : impl.tune_document("presentation")->edits())
				{ result.push_back(path + "=" + edit.baseline_token + "->" + edit.current_token); }
			return result;
		};
		const std::array<std::size_t, 2> indices{ acting_face_colour_index, acting_sweat_colour_index };
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const auto initial_colours = colour_values(presentation);
		auto expected_colours = initial_colours;
		std::srand(0xac7136);
		const int expected_random = std::rand();
		std::srand(0xac7136);
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
					"Each acting tint accepts straight named/RGB/partial/nonblack-zero-alpha channels without changing its sibling, other colours or content bytes");
				unchanged();
			}
		}
		const TuneDocument* const pending = impl.tune_document("presentation");
		const char* unrelated_word = initial_colours[3] == labrador::Colour(120, 154, 188) ? "654321" : "789ABC";
		expected_colours[3] = labrador::Colour(unrelated_word);
		require(impl.execute(std::string("tune presentation.debug_overlay.colour ") + unrelated_word).ok && pending->edits().size() == 3,
			"An unrelated live colour remains pending beside both acting edits");
		for (const std::size_t index : indices)
		{
			const std::size_t sibling = index == acting_face_colour_index ? acting_sweat_colour_index : acting_face_colour_index;
			const std::string local = std::string(colour_paths[index]).substr(std::string("presentation.").size());
			const std::string sibling_local = std::string(colour_paths[sibling]).substr(std::string("presentation.").size());
			expected_colours[index] = initial_colours[index];
			require(impl.execute(std::string("tune.reset ") + colour_paths[index]).ok && colour_values(presentation) == expected_colours &&
				pending->edits().size() == 2 && !pending->edits().contains(local) && pending->edits().contains(sibling_local) &&
				pending->edits().contains("debug_overlay.colour"),
				"Each acting reset restores only itself while its sibling and unrelated colour remain pending");
			unchanged();
			expected_colours[index] = labrador::Colour(165, 50, 214, 0);
			require(impl.execute(std::string("tune ") + colour_paths[index] + " A532D600").ok &&
				colour_values(presentation) == expected_colours && pending->edits().size() == 3,
				"Reapplying one acting edit restores the independent sibling reset fixture");
		}
		require(impl.execute("tune.reset presentation").ok && colour_values(presentation) == initial_colours &&
			pending->edits().empty() && pending->current().text() == baseline,
			"Resetting presentation restores both acting defaults and the exact initial document");
		unchanged();
		expected_colours = initial_colours;
		const std::array<const char*, 2> saved_words{ "2060C080", "D973A6C0" };
		std::string saved = baseline;
		for (std::size_t position = 0; position < indices.size(); ++position)
		{
			const std::size_t index = indices[position];
			const std::string local = std::string(colour_paths[index]).substr(std::string("presentation.").size());
			const JsonSpanDocument save_reference(saved, "acting-colour-save-reference");
			saved = save_reference.replaced(*save_reference.find(local), std::string("\"") + saved_words[position] + '"');
			expected_colours[index] = labrador::Colour(saved_words[position]);
			require(impl.execute(std::string("tune ") + colour_paths[index] + ' ' + saved_words[position]).ok &&
				pending->edits().size() == 1 && pending->edits().contains(local) && impl.execute("tune.save presentation").ok &&
				colour_values(presentation) == expected_colours && read_text(scratch.source / L"presentation.json") == saved &&
				read_text(scratch.staged / L"presentation.json") == saved && pending->edits().empty() &&
				pending->baseline().text() == saved && pending->current().text() == saved,
				"Each acting tint saves exactly one quoted token and rebases while the other saved/default tint remains unchanged");
			unchanged();
		}
		for (const std::size_t index : indices)
		{
			expected_colours[index] = labrador::Colour(165, 50, 214, 0);
			require(impl.execute(std::string("tune ") + colour_paths[index] + " A532D600").ok &&
				colour_values(presentation) == expected_colours, "Both acting edits are pending before complete merged-candidate refusal");
		}
		expected_colours[3] = labrador::Colour(unrelated_word);
		require(impl.execute(std::string("tune presentation.debug_overlay.colour ") + unrelated_word).ok && pending->edits().size() == 3,
			"Invalid merged-candidate checks retain an unrelated pending edit as well as both acting edits");
		const JsonSpanDocument saved_document(saved, "acting-colour-merged-reference");
		const JsonLeaf* hop = saved_document.find("opening.acting.hop");
		require(hop != nullptr && original.hop >= 0.0f && original.hop <= 1.0f,
			"The acting merge fixture starts inside the real unit-range hop constraint");
		const std::string external = saved_document.replaced(*hop, "1.001");
		write_text(scratch.source / L"presentation.json", external);
		const std::string pending_text = pending->current().text();
		const auto pending_ledger = ledger();
		for (const char* command : { "tune.save presentation", "tune.save presentation force" })
		{
			require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == saved && pending->current().text() == pending_text &&
				pending->baseline().text() == saved && ledger() == pending_ledger && pending->edits().size() == 3 &&
				colour_values(presentation) == expected_colours,
				"External acting.hop 1.001 refuses the complete merged candidate, preserving exact source/staged/live/current/baseline and every ledger path/baseline/current token even with force");
			unchanged();
		}
		for (std::size_t position = 0; position < indices.size(); ++position)
		{
			const std::size_t index = indices[position];
			expected_colours[index] = labrador::Colour(saved_words[position]);
			require(impl.execute(std::string("tune.reset ") + colour_paths[index]).ok &&
				colour_values(presentation) == expected_colours && pending->edits().size() == indices.size() - position &&
				pending->edits().contains("debug_overlay.colour"),
				"After refused saves each acting tint resets to its saved partial-alpha token while sibling/unrelated edits survive");
			unchanged();
		}
		expected_colours[3] = initial_colours[3];
		require(impl.execute("tune.reset presentation.debug_overlay.colour").ok && colour_values(presentation) == expected_colours &&
			pending->edits().empty() && pending->current().text() == saved && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external && read_text(scratch.staged / L"presentation.json") == saved &&
			!std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
			!std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
			!std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
			"Saved acting resets retain the invalid external source and exact staging without losing the unrelated reset or leaving recovery files");
		unchanged();
		write_text(scratch.source / L"presentation.json", saved);
		require(std::rand() == expected_random,
			"Acting colour commands, independent/saved resets and valid/refused saves consume no gameplay random draw");
	}
