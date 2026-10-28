// Included by tune_colour_document_checks.h inside its existing private namespace.
// HUD, figure and identity colour fixtures; called around the opening details.

	void check_colour_document_hud()
	{
		{
			const std::string health_path = "hud.health_colour";
			const std::string health_baseline = "{\r\n  \"hud\":{\"health_colour\":\"\\u00376E8B5\","
				"\"meter_track_colour\":\"B8B8B8\",\"sprite_sheet\":\"captured\",\"fill_frame\":\"solid\","
				"\"meter_label_font\":\"label\",\"timer\":{\"font\":\"clock\"},"
				"\"player_identity\":{\"font\":\"identity\"}},\r\n"
				"  \"note\":\"keep\\\\bytes\",\r\n  \"number\":1\r\n}\r\n";
			const JsonSpanDocument health_reference(health_baseline, "hud-health-colour-reference");
			TuneDocument health_document("presentation", health_baseline);
			const JsonLeaf* health_leaf = health_document.find(health_path);
			require(health_leaf != nullptr && health_leaf->kind == JsonLeaf::Kind::string &&
				health_document.find("hud.meter_track_colour") != nullptr &&
				health_document.find("hud.sprite_sheet") == nullptr && health_document.find("hud.fill_frame") == nullptr &&
				health_document.find("hud.meter_label_font") == nullptr && health_document.find("hud.timer.font") == nullptr &&
				health_document.find("hud.player_identity.font") == nullptr,
				"Health and track are eligible while captured HUD texture/font names remain outside the string tree");
			health_document.commit(health_document.edited(*health_leaf, "\"11223344\""), health_path);
			require(health_document.reverted(health_path) == health_baseline,
				"Health reset restores its original escaped tint token and every CRLF byte");
			const std::string health_external_number = health_reference.replaced(*health_reference.find("number"), "25");
			const JsonSpanDocument health_external_reference(health_external_number, "hud-health-colour-reference");
			const TuneSplice health_splice = health_document.splice(health_external_number, false);
			require(health_splice.conflicts.empty() && health_splice.written == 1 && health_splice.text ==
				health_external_reference.replaced(*health_external_reference.find(health_path), "\"11223344\"") &&
				count_of(health_splice.text, "\r\n") == count_of(health_baseline, "\r\n"),
				"Health splice preserves unselected track, captured font/texture strings, escaped bytes, external number and CRLF");
			const std::string health_pending = health_document.current().text();
			const TuneEdit health_held_edit = health_document.edits().at(health_path);
			bool health_wrong_type = false;
			try { health_document.commit(health_document.edited(*health_document.find(health_path), "false"), health_path); }
			catch (const std::invalid_argument&) { health_wrong_type = true; }
			require(health_wrong_type && health_document.current().text() == health_pending && health_document.edits().size() == 1 &&
				health_document.edits().at(health_path).baseline_token == health_held_edit.baseline_token &&
				health_document.edits().at(health_path).current_token == health_held_edit.current_token,
				"A wrong-type health commit retains the pending string edit and both token spellings");
			for (const std::string& health_invalid : { std::string("{}"),
				health_reference.replaced(*health_reference.find(health_path), "null"),
				health_reference.replaced(*health_reference.find(health_path), "{\"value\":\"RED\"}"),
				std::string("{\"hud.health_colour\":\"RED\"}"),
				std::string("{\"hud\":{\"health_colour.r\":\"RED\"}}") })
			{
				const TuneDocument health_unsupported("presentation", health_invalid);
				const TuneSplice health_refused = health_document.splice(health_invalid, true);
				require(health_unsupported.find(health_path) == nullptr && health_refused.written == 0 &&
					health_refused.conflicts.size() == 1 && health_refused.text == health_invalid,
					"Force cannot insert a health colour, replace null/a container or expose literal aliases");
			}
		}
		{
			const std::string track_path = "hud.meter_track_colour";
			const std::string track_baseline = "{\r\n  \"hud\":{\"meter_track_colour\":\"\\u00428B8B8\","
				"\"health_colour\":\"76E8B5\",\"sprite_sheet\":\"captured\",\"fill_frame\":\"solid\","
				"\"meter_label_font\":\"label\",\"timer\":{\"font\":\"clock\"},"
				"\"player_identity\":{\"font\":\"identity\"}},\r\n"
				"  \"note\":\"keep\\\\bytes\",\r\n  \"number\":1\r\n}\r\n";
			const JsonSpanDocument track_reference(track_baseline, "hud-track-colour-reference");
			TuneDocument track_document("presentation", track_baseline);
			const JsonLeaf* track_leaf = track_document.find(track_path);
			require(track_leaf != nullptr && track_leaf->kind == JsonLeaf::Kind::string &&
				track_document.find("hud.health_colour") != nullptr &&
				track_document.find("hud.sprite_sheet") == nullptr && track_document.find("hud.fill_frame") == nullptr &&
				track_document.find("hud.meter_label_font") == nullptr && track_document.find("hud.timer.font") == nullptr &&
				track_document.find("hud.player_identity.font") == nullptr,
				"Track and health are eligible while captured HUD texture/font names remain outside the string tree");
			track_document.commit(track_document.edited(*track_leaf, "\"11223344\""), track_path);
			require(track_document.reverted(track_path) == track_baseline,
				"Track reset restores its original escaped tint token and every CRLF byte");
			const std::string track_external_number = track_reference.replaced(*track_reference.find("number"), "25");
			const JsonSpanDocument track_external_reference(track_external_number, "hud-track-colour-reference");
			const TuneSplice track_splice = track_document.splice(track_external_number, false);
			require(track_splice.conflicts.empty() && track_splice.written == 1 && track_splice.text ==
				track_external_reference.replaced(*track_external_reference.find(track_path), "\"11223344\"") &&
				count_of(track_splice.text, "\r\n") == count_of(track_baseline, "\r\n"),
				"Track splice preserves unselected health, captured font/texture strings, escaped bytes, external number and CRLF");
			const std::string track_pending = track_document.current().text();
			const TuneEdit track_held_edit = track_document.edits().at(track_path);
			bool track_wrong_type = false;
			try { track_document.commit(track_document.edited(*track_document.find(track_path), "false"), track_path); }
			catch (const std::invalid_argument&) { track_wrong_type = true; }
			require(track_wrong_type && track_document.current().text() == track_pending && track_document.edits().size() == 1 &&
				track_document.edits().at(track_path).baseline_token == track_held_edit.baseline_token &&
				track_document.edits().at(track_path).current_token == track_held_edit.current_token,
				"A wrong-type track commit retains the pending string edit and both token spellings");
			for (const std::string& track_invalid : { std::string("{}"),
				track_reference.replaced(*track_reference.find(track_path), "null"),
				track_reference.replaced(*track_reference.find(track_path), "{\"value\":\"RED\"}"),
				std::string("{\"hud.meter_track_colour\":\"RED\"}"),
				std::string("{\"hud\":{\"meter_track_colour.r\":\"RED\"}}") })
			{
				const TuneDocument track_unsupported("presentation", track_invalid);
				const TuneSplice track_refused = track_document.splice(track_invalid, true);
				require(track_unsupported.find(track_path) == nullptr && track_refused.written == 0 &&
					track_refused.conflicts.size() == 1 && track_refused.text == track_invalid,
					"Force cannot insert a track colour, replace null/a container or expose literal aliases");
			}
		}
	}

	void check_colour_document_figure_identity()
	{
		{
			// Reuse45-path generic LF/CRLF saves, conflicts, force and reader
			// consumption; this block covers only the pair's escaped/type/alias gap.
			const std::string limb_baseline = "{\r\n\"figure\":{\"arm_colour\":\"\\u0042LACK\",\"hand_colour\":\"\\u0057HITE\","
				"\"sprite_sheet\":\"captured\",\"rest_arms_frame\":\"rest\",\"sleeves_frame\":\"sleeves\",\"quiet\":{\"face_colour\":\"WHITE\"}},\r\n\"number\":1\r\n}\r\n";
			for (const char* limb_field : { "arm_colour", "hand_colour" })
			{
				const std::string limb_path = std::string("figure.") + limb_field;
				const std::string limb_sibling = std::string("figure.") + (std::string(limb_field) == "arm_colour" ? "hand_colour" : "arm_colour");
				const JsonSpanDocument limb_reference(limb_baseline, "limb-document-reference");
				TuneDocument limb_document("presentation", limb_baseline);
				const auto* limb_leaf = limb_document.find(limb_path);
				require(limb_leaf != nullptr && limb_leaf->kind == JsonLeaf::Kind::string && limb_document.find(limb_sibling) != nullptr &&
					limb_document.find("figure.sprite_sheet") == nullptr && limb_document.find("figure.rest_arms_frame") == nullptr &&
					limb_document.find("figure.sleeves_frame") == nullptr && limb_document.find("figure.quiet.face_colour") != nullptr,
					"The exact limb pair and separately audited quiet face are editable; captured names remain unavailable");
				limb_document.commit(limb_document.edited(*limb_leaf, "\"2060C080\""), limb_path);
				const auto limb_edit = limb_document.edits().at(limb_path);const auto limb_pending = limb_document.current().text();
				bool limb_bad_type = false;
				try { limb_document.commit(limb_document.edited(*limb_document.find(limb_path), "false"), limb_path); }
				catch (const std::invalid_argument&) { limb_bad_type = true; }
				require(limb_bad_type && limb_document.current().text() == limb_pending && limb_document.baseline().text() == limb_baseline &&
					limb_document.edits().size() == 1 && limb_document.edits().at(limb_path).baseline_token == limb_edit.baseline_token &&
					limb_document.edits().at(limb_path).current_token == limb_edit.current_token && limb_document.reverted(limb_path) == limb_baseline,
					"Each wrong-type limb commit preserves current/baseline, both ledger tokens, its sibling and escaped CRLF reset bytes");
				for (const std::string& limb_missing : { std::string("{}"), limb_reference.replaced(*limb_reference.find(limb_path), "null"),
					limb_reference.replaced(*limb_reference.find(limb_path), "{\"value\":\"RED\"}"),
					std::string("{\"") + limb_path + "\":\"RED\"}", std::string("{\"figure\":{\"") + limb_field + ".r\":\"RED\"}}" })
				{
					const TuneDocument limb_unsupported("presentation", limb_missing);const TuneSplice limb_refused = limb_document.splice(limb_missing, true);
					require(limb_unsupported.find(limb_path) == nullptr && limb_refused.written == 0 && limb_refused.conflicts.size() == 1 && limb_refused.text == limb_missing,
						"Even force cannot insert a limb key, replace null/container or expose literal/channel aliases");
				}
			}
		}
		const std::string identity_baseline = "{\r\n  \"player_identity\":{\"ink_colour\":\"\\u0042LACK\","
			"\"background_colour\":\"\\u0057HITE\"},\r\n  \"note\":\"keep\\\\bytes\",\r\n  \"number\":1\r\n}\r\n";
		const JsonSpanDocument identity_reference(identity_baseline, "identity-colour-reference");
		for (const char* field : { "ink_colour", "background_colour" })
		{
			const std::string path = std::string("player_identity.") + field;
			const std::string sibling = std::string("player_identity.") +
				(std::string(field) == "ink_colour" ? "background_colour" : "ink_colour");
			const std::string replacement = std::string(field) == "ink_colour" ? "\"112233FF\"" : "\"EECCAAFF\"";
			const std::string sibling_token(identity_reference.token(*identity_reference.find(sibling)));
			TuneDocument identity_document("presentation", identity_baseline);
			const JsonLeaf* leaf = identity_document.find(path);
			const JsonLeaf* sibling_leaf = identity_document.find(sibling);
			require(leaf != nullptr && leaf->kind == JsonLeaf::Kind::string &&
				sibling_leaf != nullptr && sibling_leaf->kind == JsonLeaf::Kind::string &&
				identity_document.current().token(*sibling_leaf) == sibling_token,
				"Both audited identity colours are eligible and retain their exact escaped tokens: " + path);
			identity_document.commit(identity_document.edited(*leaf, replacement), path);
			require(identity_document.edits().size() == 1 && identity_document.edits().contains(path) &&
				identity_document.current().token(*identity_document.find(sibling)) == sibling_token &&
				identity_document.reverted(path) == identity_baseline,
				"Each identity reset preserves its sibling and restores its escaped token and every CRLF byte: " + path);
			const std::string identity_number_external = identity_reference.replaced(*identity_reference.find("number"), "25");
			const JsonSpanDocument identity_number_document(identity_number_external, "identity-colour-reference");
			const TuneSplice identity_number_splice = identity_document.splice(identity_number_external, false);
			require(identity_number_splice.conflicts.empty() && identity_number_splice.written == 1 && identity_number_splice.text ==
				identity_number_document.replaced(*identity_number_document.find(path), replacement) &&
				count_of(identity_number_splice.text, "\r\n") == count_of(identity_baseline, "\r\n"),
				"Each identity splice preserves its sibling, unrelated external number, escapes and CRLF: " + path);
			const std::string identity_pending_text = identity_document.current().text();
			bool wrong_type = false;
			try { identity_document.commit(identity_document.edited(*identity_document.find(path), "false"), path); }
			catch (const std::invalid_argument&) { wrong_type = true; }
			require(wrong_type && identity_document.current().text() == identity_pending_text &&
				identity_document.edits().size() == 1 && identity_document.edits().contains(path),
				"A wrong-type identity commit preserves the pending string and its sibling: " + path);
			for (const std::string& invalid : { std::string("{}"),
				identity_reference.replaced(*identity_reference.find(path), "null"),
				identity_reference.replaced(*identity_reference.find(path), "{\"value\":\"RED\"}"),
				"{\"" + path + "\":\"BLACK\"}",
				std::string("{\"player_identity\":{\"") + field + ".r\":\"BLACK\"}}" })
			{
				const TuneDocument unsupported("presentation", invalid);
				const TuneSplice refused = identity_document.splice(invalid, true);
				require(unsupported.find(path) == nullptr && refused.written == 0 &&
					refused.conflicts.size() == 1 && refused.text == invalid,
					"Force cannot insert an identity colour, replace a non-string/container, or splice a literal alias: " + path);
			}
		}
		for (const char* reset_field : { "ink_colour", "background_colour" })
		{
			TuneDocument paired("presentation", identity_baseline);
			paired.commit(paired.edited(*paired.find("player_identity.ink_colour"), "\"112233FF\""), "player_identity.ink_colour");
			paired.commit(paired.edited(*paired.find("player_identity.background_colour"), "\"EECCAAFF\""), "player_identity.background_colour");
			require(paired.edits().size() == 2, "Both identity colours retain independent pending edits");
			const std::string path = std::string("player_identity.") + reset_field;
			const bool reset_ink = std::string(reset_field) == "ink_colour";
			const std::string sibling = reset_ink ? "player_identity.background_colour" : "player_identity.ink_colour";
			const std::string sibling_token = reset_ink ? "\"EECCAAFF\"" : "\"112233FF\"";
			paired.commit(paired.reverted(path), path);
			require(paired.edits().size() == 1 && !paired.edits().contains(path) && paired.edits().contains(sibling) &&
				paired.current().text() == identity_reference.replaced(*identity_reference.find(sibling), sibling_token),
				"Resetting either identity colour restores only its own escaped baseline: " + path);
			const std::string identity_sibling_external = identity_reference.replaced(*identity_reference.find(path), "\"102030FF\"");
			const JsonSpanDocument identity_sibling_document(identity_sibling_external, "identity-sibling-external");
			const TuneSplice identity_sibling_splice = paired.splice(identity_sibling_external, false);
			require(identity_sibling_splice.conflicts.empty() && identity_sibling_splice.written == 1 &&
				identity_sibling_splice.text == identity_sibling_document.replaced(*identity_sibling_document.find(sibling), sibling_token),
				"Saving the remaining identity edit preserves an external sibling edit byte for byte: " + sibling);
			paired.commit(paired.reverted(sibling), sibling);
			require(paired.edits().empty() && paired.current().text() == identity_baseline,
				"Resetting the remaining identity edit restores both escaped tokens and exact CRLF bytes");
		}
	}
