// Included by tune_colour_document_checks.h inside its existing private namespace.
// Selector, weapon/combat/blocked-shot feedback, idle-drip and respawn colours.

	void check_colour_document_selectors_feedback()
	{
		for (const char* section : { "team_select", "weapon_select" })
		{
			const std::string section_path = std::string("main_menu.") + section;
			const std::string selector_baseline = "{\r\n  \"main_menu\":{\"" + std::string(section) + "\":{"
				"\"selected\":\"\\u0052ED\",\"unselected\":\"\\u0057HITE\",\"sheet\":\"unchanged\"}},\r\n"
				"  \"note\":\"keep\\\\bytes\",\r\n  \"number\":1\r\n}\r\n";
			for (const char* field : { "selected", "unselected" })
			{
				const std::string path = section_path + '.' + field;
				TuneDocument selector_document("presentation", selector_baseline);
				const JsonLeaf* leaf = selector_document.find(path);
				require(leaf != nullptr && leaf->kind == JsonLeaf::Kind::string && selector_document.find(section_path + ".sheet") == nullptr,
					"The selector colour is eligible while its neighbouring atlas is not: " + path);
				selector_document.commit(selector_document.edited(*leaf, "\"11223344\""), path);
				require(selector_document.reverted(path) == selector_baseline,
					"A selector colour reset restores its original escaped token and CRLF bytes: " + path);
				const JsonSpanDocument baseline_document(selector_baseline, "selector-colour-reference");
				const std::string external_number = baseline_document.replaced(*baseline_document.find("number"), "25");
				const JsonSpanDocument selector_external(external_number, "selector-colour-reference");
				const TuneSplice selector_splice = selector_document.splice(external_number, false);
				require(selector_splice.conflicts.empty() && selector_splice.written == 1 && selector_splice.text ==
					selector_external.replaced(*selector_external.find(path), "\"11223344\"") &&
					count_of(selector_splice.text, "\r\n") == count_of(selector_baseline, "\r\n"),
					"A selector colour splice preserves every unrelated byte, external number and CRLF: " + path);
				const std::string selector_pending = selector_document.current().text();
				bool wrong_type = false;
				try { selector_document.commit(selector_document.edited(*selector_document.find(path), "false"), path); }
				catch (const std::invalid_argument&) { wrong_type = true; }
				require(wrong_type && selector_document.current().text() == selector_pending && selector_document.edits().size() == 1,
					"A wrong-type selector colour commit preserves the pending string edit: " + path);
				for (const std::string& missing : { std::string("{}"),
					baseline_document.replaced(*baseline_document.find(path), "null"),
					baseline_document.replaced(*baseline_document.find(path), "{\"value\":\"RED\"}") })
				{
					const TuneSplice refused = selector_document.splice(missing, true);
					require(refused.written == 0 && refused.conflicts.size() == 1 && refused.text == missing,
						"Force cannot insert a missing selector colour or replace null/a container: " + path);
				}
				for (const std::string& alias : { "{\"" + path + "\":\"RED\"}",
					std::string("{\"main_menu\":{\"") + section + '.' + field + "\":\"RED\"}}",
					"{\"" + section_path + "\":{\"" + field + "\":\"RED\"}}" })
				{
					const TuneDocument literal("presentation", alias);
					const TuneSplice refused = selector_document.splice(alias, true);
					require(literal.prefixed_paths().empty() && literal.find(path) == nullptr &&
						refused.written == 0 && refused.conflicts.size() == 1 && refused.text == alias,
						"Literal key aliases cannot expose or receive a selector colour, even with force: " + path);
				}
				const std::string alias_member = "\"" + path + "\":\"BLUE\"";
				for (const std::string& text : { "{" + alias_member + "," + selector_baseline.substr(1),
					selector_baseline.substr(0, selector_baseline.find_last_of('}')) + "," + alias_member + "}" })
				{
					const TuneDocument literal("presentation", text);
					const JsonLeaf* actual = literal.find(path);
					require(actual == nullptr || (actual->plain_path && literal.current().token(*actual) ==
						baseline_document.token(*baseline_document.find(path))),
						"An alias before or after the nested selector colour never exposes the literal-key token: " + path);
				}
			}
		}
		for (const std::string section : { "weapon_feedback", "combat_feedback", "blocked_shot" })
		{
			const std::string feedback_baseline = "{\r\n  \"" + section + "\":{\"highlight\":\"\\u0057HITE\","
				"\"keyline\":\"\\u0042LACK\",\"sprite_sheet\":\"unchanged\",\"fill_frame\":\"solid\"},\r\n"
				"  \"note\":\"keep\\\\bytes\",\r\n  \"number\":1\r\n}\r\n";
			for (const char* field : { "highlight", "keyline" })
			{
				const std::string path = section + '.' + field;
				TuneDocument feedback_document("presentation", feedback_baseline);
				const JsonLeaf* leaf = feedback_document.find(path);
				require(leaf != nullptr && leaf->kind == JsonLeaf::Kind::string &&
					feedback_document.find(section + ".sprite_sheet") == nullptr &&
					feedback_document.find(section + ".fill_frame") == nullptr,
					"A feedback colour is eligible while both captured texture strings stay outside the tree: " + path);
				feedback_document.commit(feedback_document.edited(*leaf, "\"11223344\""), path);
				require(feedback_document.reverted(path) == feedback_baseline,
					"A feedback reset restores its escaped token and every original CRLF byte: " + path);
				const JsonSpanDocument baseline_document(feedback_baseline, "feedback-colour-reference");
				const std::string external_number = baseline_document.replaced(*baseline_document.find("number"), "25");
				const JsonSpanDocument feedback_external(external_number, "feedback-colour-reference");
				const TuneSplice feedback_splice = feedback_document.splice(external_number, false);
				require(feedback_splice.conflicts.empty() && feedback_splice.written == 1 && feedback_splice.text ==
					feedback_external.replaced(*feedback_external.find(path), "\"11223344\"") &&
					count_of(feedback_splice.text, "\r\n") == count_of(feedback_baseline, "\r\n"),
					"A feedback splice preserves unrelated fields, escaped bytes, external edits and CRLF: " + path);
				const std::string feedback_pending = feedback_document.current().text();
				bool wrong_type = false;
				try { feedback_document.commit(feedback_document.edited(*feedback_document.find(path), "false"), path); }
				catch (const std::invalid_argument&) { wrong_type = true; }
				require(wrong_type && feedback_document.current().text() == feedback_pending && feedback_document.edits().size() == 1,
					"A wrong-type feedback commit preserves the pending string edit: " + path);
				for (const std::string& missing : { std::string("{}"),
					baseline_document.replaced(*baseline_document.find(path), "null"),
					baseline_document.replaced(*baseline_document.find(path), "{\"value\":\"RED\"}") })
				{
					const TuneSplice refused = feedback_document.splice(missing, true);
					require(refused.written == 0 && refused.conflicts.size() == 1 && refused.text == missing,
						"Force cannot insert a missing feedback colour or replace null/a container: " + path);
				}
				for (const std::string& alias : { "{\"" + path + "\":\"RED\"}",
					"{\"" + section + "\":{\"" + field + ".r\":\"RED\"}}" })
				{
					const TuneDocument literal("presentation", alias);
					const TuneSplice refused = feedback_document.splice(alias, true);
					require(literal.prefixed_paths().empty() && literal.find(path) == nullptr &&
						refused.written == 0 && refused.conflicts.size() == 1 && refused.text == alias,
						"A literal alias cannot expose or receive a feedback colour, even with force: " + path);
				}
			}
		}
		{
			const std::string path = "idle_drip.keyline";
			const std::string drip_baseline = "{\r\n  \"idle_drip\":{\"keyline\":\"\\u0042LACK\","
				"\"sprite_sheet\":\"unchanged\",\"fill_frame\":\"solid\"},\r\n"
				"  \"note\":\"keep\\\\bytes\",\r\n  \"number\":1\r\n}\r\n";
			TuneDocument drip_document("presentation", drip_baseline);
			const JsonLeaf* leaf = drip_document.find(path);
			require(leaf != nullptr && leaf->kind == JsonLeaf::Kind::string &&
				drip_document.find("idle_drip.sprite_sheet") == nullptr && drip_document.find("idle_drip.fill_frame") == nullptr,
				"Only the common idle-drip keyline is eligible while captured texture strings remain outside the tree");
			drip_document.commit(drip_document.edited(*leaf, "\"11223344\""), path);
			require(drip_document.reverted(path) == drip_baseline,
				"An idle-drip keyline reset restores its escaped token and every original CRLF byte");
			const JsonSpanDocument baseline_document(drip_baseline, "idle-drip-colour-reference");
			const std::string external_number = baseline_document.replaced(*baseline_document.find("number"), "25");
			const JsonSpanDocument idle_external_document(external_number, "idle-drip-colour-reference");
			const TuneSplice idle_splice = drip_document.splice(external_number, false);
			require(idle_splice.conflicts.empty() && idle_splice.written == 1 && idle_splice.text ==
				idle_external_document.replaced(*idle_external_document.find(path), "\"11223344\"") &&
				count_of(idle_splice.text, "\r\n") == count_of(drip_baseline, "\r\n"),
				"An idle-drip splice preserves unrelated fields, escaped bytes, an external number and every CRLF");
			const std::string idle_pending = drip_document.current().text();
			bool wrong_type = false;
			try { drip_document.commit(drip_document.edited(*drip_document.find(path), "false"), path); }
			catch (const std::invalid_argument&) { wrong_type = true; }
			require(wrong_type && drip_document.current().text() == idle_pending && drip_document.edits().size() == 1,
				"A wrong-type idle-drip commit preserves the pending keyline edit");
			for (const std::string& invalid : { std::string("{}"),
				baseline_document.replaced(*baseline_document.find(path), "null"),
				baseline_document.replaced(*baseline_document.find(path), "{\"value\":\"RED\"}"),
				std::string("{\"idle_drip.keyline\":\"RED\"}"),
				std::string("{\"idle_drip\":{\"keyline.r\":\"RED\"}}"),
				std::string("{\"idle_drip\":{\"tools\":{\"sprayer\":{\"keyline\":\"RED\"}}}}") })
			{
				const TuneDocument unsupported("presentation", invalid);
				const TuneSplice refused = drip_document.splice(invalid, true);
				require(unsupported.find(path) == nullptr &&
					refused.written == 0 && refused.conflicts.size() == 1 && refused.text == invalid,
					"Force cannot insert an idle-drip keyline, replace null/a container, or expose literal/per-tool aliases");
			}
		}
		{
			const std::string path = "respawn_redraw.pen_colour";
			const std::string pen_baseline = "{\r\n  \"respawn_redraw\":{\"pen_colour\":\"\\u0042LACK\","
				"\"sprite_sheet\":\"unchanged\",\"pen_frame\":\"solid\"},\r\n"
				"  \"note\":\"keep\\\\bytes\",\r\n  \"number\":1\r\n}\r\n";
			TuneDocument pen_document("presentation", pen_baseline);
			const JsonLeaf* leaf = pen_document.find(path);
			require(leaf != nullptr && leaf->kind == JsonLeaf::Kind::string &&
				pen_document.find("respawn_redraw.sprite_sheet") == nullptr &&
				pen_document.find("respawn_redraw.pen_frame") == nullptr,
				"Only the respawn pen colour is eligible while its captured texture strings remain outside the tree");
			pen_document.commit(pen_document.edited(*leaf, "\"11223344\""), path);
			require(pen_document.reverted(path) == pen_baseline,
				"A respawn pen reset restores its escaped token and every original CRLF byte");
			const JsonSpanDocument pen_reference(pen_baseline, "respawn-pen-colour-reference");
			const std::string pen_external_number = pen_reference.replaced(*pen_reference.find("number"), "25");
			const JsonSpanDocument pen_external_document(pen_external_number, "respawn-pen-colour-reference");
			const TuneSplice pen_splice = pen_document.splice(pen_external_number, false);
			require(pen_splice.conflicts.empty() && pen_splice.written == 1 && pen_splice.text ==
				pen_external_document.replaced(*pen_external_document.find(path), "\"11223344\"") &&
				count_of(pen_splice.text, "\r\n") == count_of(pen_baseline, "\r\n"),
				"A respawn pen splice preserves unrelated fields, escape bytes, an external number and every CRLF");
			const std::string pen_pending = pen_document.current().text();
			bool wrong_type = false;
			try { pen_document.commit(pen_document.edited(*pen_document.find(path), "false"), path); }
			catch (const std::invalid_argument&) { wrong_type = true; }
			require(wrong_type && pen_document.current().text() == pen_pending && pen_document.edits().size() == 1,
				"A wrong-type respawn pen commit preserves the pending colour edit");
			for (const std::string& invalid : { std::string("{}"),
				pen_reference.replaced(*pen_reference.find(path), "null"),
				pen_reference.replaced(*pen_reference.find(path), "{\"value\":\"RED\"}"),
				std::string("{\"respawn_redraw.pen_colour\":\"RED\"}"),
				std::string("{\"respawn_redraw\":{\"pen_colour.r\":\"RED\"}}") })
			{
				const TuneDocument unsupported("presentation", invalid);
				const TuneSplice refused = pen_document.splice(invalid, true);
				require(unsupported.find(path) == nullptr &&
					refused.written == 0 && refused.conflicts.size() == 1 && refused.text == invalid,
					"Force cannot insert a respawn pen colour, replace null/a container, or expose literal aliases");
			}
		}
	}
