// Included by tune_colour_document_checks.h inside its existing private namespace.
// Opening artwork and later crack/acting colour fixtures; called in historical order.

	void check_colour_document_opening_art()
	{
		for (const char* cursor_field : { "ink", "fill" })
		{
			const std::string cursor_path = std::string("opening.cursor_art.") + cursor_field;
			const std::string cursor_sibling = std::string("opening.cursor_art.") +
				(std::string(cursor_field) == "ink" ? "fill" : "ink");
			const std::string cursor_baseline = "{\r\n  \"opening\":{\"sprite_sheet\":\"unchanged\",\"fill_frame\":\"solid\","
				"\"cursor_art\":{\"ink\":\"\\u0042LACK\",\"fill\":\"\\u0057HITE\",\"shade\":\"B4B4B4\",\"shadow\":\"00000066\"}},\r\n"
				"  \"note\":\"keep\\\\bytes\",\r\n  \"number\":1\r\n}\r\n";
			const JsonSpanDocument cursor_reference(cursor_baseline, "cursor-colour-reference");
			const std::string cursor_sibling_token(cursor_reference.token(*cursor_reference.find(cursor_sibling)));
			TuneDocument cursor_document("presentation", cursor_baseline);
			const JsonLeaf* cursor_leaf = cursor_document.find(cursor_path);
			require(cursor_leaf != nullptr && cursor_leaf->kind == JsonLeaf::Kind::string &&
				cursor_document.find(cursor_sibling) != nullptr && cursor_document.find("opening.sprite_sheet") == nullptr &&
				cursor_document.find("opening.fill_frame") == nullptr && cursor_document.find("opening.cursor_art.shade") != nullptr &&
				cursor_document.find("opening.cursor_art.shadow") != nullptr,
				"All four audited cursor colours are eligible while captured texture names stay outside the tree: " + cursor_path);
			cursor_document.commit(cursor_document.edited(*cursor_leaf, "\"11223344\""), cursor_path);
			require(cursor_document.current().token(*cursor_document.find(cursor_sibling)) == cursor_sibling_token &&
				cursor_document.reverted(cursor_path) == cursor_baseline,
				"Each cursor colour reset preserves its sibling and restores its escaped token and every CRLF byte: " + cursor_path);
			const std::string cursor_external_number = cursor_reference.replaced(*cursor_reference.find("number"), "25");
			const JsonSpanDocument cursor_external_reference(cursor_external_number, "cursor-colour-reference");
			const TuneSplice cursor_splice = cursor_document.splice(cursor_external_number, false);
			require(cursor_splice.conflicts.empty() && cursor_splice.written == 1 && cursor_splice.text ==
				cursor_external_reference.replaced(*cursor_external_reference.find(cursor_path), "\"11223344\"") &&
				count_of(cursor_splice.text, "\r\n") == count_of(cursor_baseline, "\r\n"),
				"Each cursor splice preserves its sibling, unselected materials, escaped bytes, an external number and every CRLF: " + cursor_path);
			const std::string cursor_pending = cursor_document.current().text();
			bool cursor_wrong_type = false;
			try { cursor_document.commit(cursor_document.edited(*cursor_document.find(cursor_path), "false"), cursor_path); }
			catch (const std::invalid_argument&) { cursor_wrong_type = true; }
			require(cursor_wrong_type && cursor_document.current().text() == cursor_pending && cursor_document.edits().size() == 1,
				"A wrong-type cursor commit retains its sibling and the pending string edit: " + cursor_path);
			for (const std::string& cursor_invalid : { std::string("{}"),
				cursor_reference.replaced(*cursor_reference.find(cursor_path), "null"),
				cursor_reference.replaced(*cursor_reference.find(cursor_path), "{\"value\":\"RED\"}"),
				"{\"" + cursor_path + "\":\"RED\"}",
				std::string("{\"opening\":{\"cursor_art.") + cursor_field + "\":\"RED\"}}",
				std::string("{\"opening.cursor_art\":{\"") + cursor_field + "\":\"RED\"}}",
				std::string("{\"opening\":{\"cursor_art\":{\"") + cursor_field + ".r\":\"RED\"}}}" })
			{
				const TuneDocument cursor_unsupported("presentation", cursor_invalid);
				const TuneSplice cursor_refused = cursor_document.splice(cursor_invalid, true);
				require(cursor_unsupported.find(cursor_path) == nullptr && cursor_refused.written == 0 &&
					cursor_refused.conflicts.size() == 1 && cursor_refused.text == cursor_invalid,
					"Force cannot insert a cursor colour, replace null/a container or expose literal key aliases: " + cursor_path);
			}
		}
		for (const char* material_field : { "shade", "shadow" })
		{
			const std::string material_path = std::string("opening.cursor_art.") + material_field;
			const std::string material_sibling = std::string("opening.cursor_art.") +
				(std::string(material_field) == "shade" ? "shadow" : "shade");
			const std::string material_baseline = "{\r\n  \"opening\":{\"sprite_sheet\":\"unchanged\",\"fill_frame\":\"solid\","
				"\"cursor_art\":{\"ink\":\"\\u0042LACK\",\"fill\":\"\\u0057HITE\",\"shade\":\"\\u00424B4B4\",\"shadow\":\"\\u00300000066\"}},\r\n"
				"  \"note\":\"keep\\\\bytes\",\r\n  \"number\":1\r\n}\r\n";
			const JsonSpanDocument material_reference(material_baseline, "cursor-material-reference");
			const std::string material_sibling_token(material_reference.token(*material_reference.find(material_sibling)));
			TuneDocument material_document("presentation", material_baseline);
			const JsonLeaf* material_leaf = material_document.find(material_path);
			require(material_leaf != nullptr && material_leaf->kind == JsonLeaf::Kind::string &&
				material_document.find(material_sibling) != nullptr && material_document.find("opening.sprite_sheet") == nullptr &&
				material_document.find("opening.fill_frame") == nullptr && material_document.find("opening.cursor_art.shade") != nullptr &&
				material_document.find("opening.cursor_art.shadow") != nullptr,
				"All four audited cursor colours are eligible while captured texture names stay outside the tree: " + material_path);
			material_document.commit(material_document.edited(*material_leaf, "\"11223344\""), material_path);
			require(material_document.current().token(*material_document.find(material_sibling)) == material_sibling_token &&
				material_document.reverted(material_path) == material_baseline,
				"Each cursor colour reset preserves its sibling and restores its escaped token and every CRLF byte: " + material_path);
			const std::string material_external_number = material_reference.replaced(*material_reference.find("number"), "25");
			const JsonSpanDocument material_external_reference(material_external_number, "cursor-material-reference");
			const TuneSplice material_splice = material_document.splice(material_external_number, false);
			require(material_splice.conflicts.empty() && material_splice.written == 1 && material_splice.text ==
				material_external_reference.replaced(*material_external_reference.find(material_path), "\"11223344\"") &&
				count_of(material_splice.text, "\r\n") == count_of(material_baseline, "\r\n"),
				"Each cursor splice preserves its sibling, unselected materials, escaped bytes, an external number and every CRLF: " + material_path);
			const std::string material_pending = material_document.current().text();
			bool material_wrong_type = false;
			try { material_document.commit(material_document.edited(*material_document.find(material_path), "false"), material_path); }
			catch (const std::invalid_argument&) { material_wrong_type = true; }
			require(material_wrong_type && material_document.current().text() == material_pending && material_document.edits().size() == 1,
				"A wrong-type cursor commit retains its sibling and the pending string edit: " + material_path);
			for (const std::string& material_invalid : { std::string("{}"),
				material_reference.replaced(*material_reference.find(material_path), "null"),
				material_reference.replaced(*material_reference.find(material_path), "{\"value\":\"RED\"}"),
				"{\"" + material_path + "\":\"RED\"}",
				std::string("{\"opening\":{\"cursor_art.") + material_field + "\":\"RED\"}}",
				std::string("{\"opening.cursor_art\":{\"") + material_field + "\":\"RED\"}}",
				std::string("{\"opening\":{\"cursor_art\":{\"") + material_field + ".r\":\"RED\"}}}" })
			{
				const TuneDocument material_unsupported("presentation", material_invalid);
				const TuneSplice material_refused = material_document.splice(material_invalid, true);
				require(material_unsupported.find(material_path) == nullptr && material_refused.written == 0 &&
					material_refused.conflicts.size() == 1 && material_refused.text == material_invalid,
					"Force cannot insert a cursor colour, replace null/a container or expose literal key aliases: " + material_path);
			}
		}
		for (const char* silhouette_field : { "ink", "rim", "shade" })
		{
			const std::string silhouette_path = std::string("opening.silhouette_art.") + silhouette_field;
			const std::string silhouette_baseline = "{\r\n  \"opening\":{\"sprite_sheet\":\"unchanged\",\"fill_frame\":\"solid\","
				"\"silhouette\":\"0E0E0E\",\"silhouette_art\":{\"ink\":\"\\u0042LACK\",\"rim\":\"343434\",\"shade\":\"080808\"}},\r\n"
				"  \"note\":\"keep\\\\bytes\",\r\n  \"number\":1\r\n}\r\n";
			const JsonSpanDocument silhouette_reference(silhouette_baseline, "silhouette-colour-reference");
			TuneDocument silhouette_document("presentation", silhouette_baseline);
			const JsonLeaf* silhouette_leaf = silhouette_document.find(silhouette_path);
			require(silhouette_leaf != nullptr && silhouette_leaf->kind == JsonLeaf::Kind::string &&
				silhouette_document.find("opening.sprite_sheet") == nullptr &&
				silhouette_document.find("opening.fill_frame") == nullptr && silhouette_document.find("opening.silhouette") != nullptr,
				"The three silhouette-art colours and separate body tint are eligible; captured texture names stay outside the tree");
			silhouette_document.commit(silhouette_document.edited(*silhouette_leaf, "\"11223344\""), silhouette_path);
			for (const char* sibling_field : { "ink", "rim", "shade" })
			{
				if (std::string(sibling_field) == silhouette_field) { continue; }
				const std::string silhouette_sibling = std::string("opening.silhouette_art.") + sibling_field;
				require(silhouette_document.current().token(*silhouette_document.find(silhouette_sibling)) ==
					silhouette_reference.token(*silhouette_reference.find(silhouette_sibling)),
					"Each silhouette-art edit preserves both sibling quoted tokens byte for byte");
			}
			require(silhouette_document.reverted(silhouette_path) == silhouette_baseline,
				"Each silhouette-art reset restores its authored spelling and every original CRLF byte");
			const std::string silhouette_external_number = silhouette_reference.replaced(*silhouette_reference.find("number"), "25");
			const JsonSpanDocument silhouette_external_reference(silhouette_external_number, "silhouette-colour-reference");
			const TuneSplice silhouette_splice = silhouette_document.splice(silhouette_external_number, false);
			require(silhouette_splice.conflicts.empty() && silhouette_splice.written == 1 && silhouette_splice.text ==
				silhouette_external_reference.replaced(*silhouette_external_reference.find(silhouette_path), "\"11223344\"") &&
				count_of(silhouette_splice.text, "\r\n") == count_of(silhouette_baseline, "\r\n"),
				"A silhouette-art splice preserves both siblings, separate body fill, escape bytes, external number and every CRLF");
			const std::string silhouette_pending = silhouette_document.current().text();
			bool silhouette_wrong_type = false;
			try { silhouette_document.commit(silhouette_document.edited(*silhouette_document.find(silhouette_path), "false"), silhouette_path); }
			catch (const std::invalid_argument&) { silhouette_wrong_type = true; }
			require(silhouette_wrong_type && silhouette_document.current().text() == silhouette_pending && silhouette_document.edits().size() == 1,
				"A wrong-type silhouette-art commit retains both siblings and the pending string edit");
			for (const std::string& silhouette_invalid : { std::string("{}"),
				silhouette_reference.replaced(*silhouette_reference.find(silhouette_path), "null"),
				silhouette_reference.replaced(*silhouette_reference.find(silhouette_path), "{\"value\":\"RED\"}"),
				"{\"" + silhouette_path + "\":\"RED\"}",
				std::string("{\"opening\":{\"silhouette_art.") + silhouette_field + "\":\"RED\"}}",
				std::string("{\"opening.silhouette_art\":{\"") + silhouette_field + "\":\"RED\"}}",
				std::string("{\"opening\":{\"silhouette_art\":{\"") + silhouette_field + ".r\":\"RED\"}}}" })
			{
				const TuneDocument silhouette_unsupported("presentation", silhouette_invalid);
				const TuneSplice silhouette_refused = silhouette_document.splice(silhouette_invalid, true);
				require(silhouette_unsupported.find(silhouette_path) == nullptr && silhouette_refused.written == 0 &&
					silhouette_refused.conflicts.size() == 1 && silhouette_refused.text == silhouette_invalid,
					"Force cannot insert a silhouette-art colour, replace null/a container or expose literal key aliases");
			}
		}
		for (const char* reveal_field : { "ink", "paper", "shade" })
		{
			const std::string reveal_path = std::string("opening.reveal_art.") + reveal_field;
			const std::string reveal_baseline = "{\r\n  \"opening\":{\"sprite_sheet\":\"unchanged\",\"fill_frame\":\"solid\","
				"\"percent_font\":\"captured\",\"reveal_art\":{\"ink\":\"\\u0042LACK\",\"paper\":\"\\u0057HITE\","
				"\"shade\":\"\\u0030\\u0030000055\"}},\r\n  \"note\":\"keep\\\\bytes\",\r\n  \"number\":1\r\n}\r\n";
			const JsonSpanDocument reveal_reference(reveal_baseline, "reveal-colour-reference");
			TuneDocument reveal_document("presentation", reveal_baseline);
			const JsonLeaf* reveal_leaf = reveal_document.find(reveal_path);
			require(reveal_leaf != nullptr && reveal_leaf->kind == JsonLeaf::Kind::string &&
				reveal_document.find("opening.sprite_sheet") == nullptr && reveal_document.find("opening.fill_frame") == nullptr &&
				reveal_document.find("opening.percent_font") == nullptr,
				"Each audited reveal colour is eligible while the captured font and texture names stay outside the tree: " + reveal_path);
			reveal_document.commit(reveal_document.edited(*reveal_leaf, "\"11223344\""), reveal_path);
			for (const char* reveal_sibling_field : { "ink", "paper", "shade" })
			{
				const std::string reveal_sibling = std::string("opening.reveal_art.") + reveal_sibling_field;
				if (reveal_sibling == reveal_path) { continue; }
				require(reveal_document.find(reveal_sibling) != nullptr &&
					reveal_document.current().token(*reveal_document.find(reveal_sibling)) ==
						reveal_reference.token(*reveal_reference.find(reveal_sibling)),
					"Editing one reveal colour retains both siblings' exact escaped token spellings: " + reveal_path);
			}
			require(reveal_document.reverted(reveal_path) == reveal_baseline,
				"Each reveal reset restores its escaped token, both siblings and every original CRLF byte: " + reveal_path);
			const std::string reveal_external_number = reveal_reference.replaced(*reveal_reference.find("number"), "25");
			const JsonSpanDocument reveal_external_reference(reveal_external_number, "reveal-colour-reference");
			const TuneSplice reveal_splice = reveal_document.splice(reveal_external_number, false);
			require(reveal_splice.conflicts.empty() && reveal_splice.written == 1 && reveal_splice.text ==
				reveal_external_reference.replaced(*reveal_external_reference.find(reveal_path), "\"11223344\"") &&
				count_of(reveal_splice.text, "\r\n") == count_of(reveal_baseline, "\r\n"),
				"Each reveal splice preserves both siblings, captured strings, unrelated escapes, external numbers and every CRLF: " + reveal_path);
			const std::string reveal_pending = reveal_document.current().text();
			bool reveal_wrong_type = false;
			try { reveal_document.commit(reveal_document.edited(*reveal_document.find(reveal_path), "false"), reveal_path); }
			catch (const std::invalid_argument&) { reveal_wrong_type = true; }
			require(reveal_wrong_type && reveal_document.current().text() == reveal_pending && reveal_document.edits().size() == 1,
				"A wrong-type reveal commit retains its pending string and both siblings: " + reveal_path);
			for (const std::string& reveal_invalid : { std::string("{}"),
				reveal_reference.replaced(*reveal_reference.find(reveal_path), "null"),
				reveal_reference.replaced(*reveal_reference.find(reveal_path), "{\"value\":\"RED\"}"),
				"{\"" + reveal_path + "\":\"RED\"}",
				std::string("{\"opening\":{\"reveal_art.") + reveal_field + "\":\"RED\"}}",
				std::string("{\"opening.reveal_art\":{\"") + reveal_field + "\":\"RED\"}}",
				std::string("{\"opening\":{\"reveal_art\":{\"") + reveal_field + ".r\":\"RED\"}}}" })
			{
				const TuneDocument reveal_unsupported("presentation", reveal_invalid);
				const TuneSplice reveal_refused = reveal_document.splice(reveal_invalid, true);
				require(reveal_unsupported.find(reveal_path) == nullptr && reveal_refused.written == 0 &&
					reveal_refused.conflicts.size() == 1 && reveal_refused.text == reveal_invalid,
					"Force cannot insert a reveal colour, replace null/a container or expose literal key aliases: " + reveal_path);
			}
		}
		{
			const std::string desktop_path = "opening.desktop_colour";
			const std::string desktop_baseline = "{\r\n  \"opening\":{\"desktop_colour\":\"\\u0030\\u00308080\","
				"\"sprite_sheet\":\"captured\",\"fill_frame\":\"solid\",\"palette_sheet\":\"unchanged\"},\r\n"
				"  \"note\":\"keep\\\\bytes\",\r\n  \"number\":1\r\n}\r\n";
			const JsonSpanDocument desktop_reference(desktop_baseline, "desktop-colour-reference");
			TuneDocument desktop_document("presentation", desktop_baseline);
			const JsonLeaf* desktop_leaf = desktop_document.find(desktop_path);
			require(desktop_leaf != nullptr && desktop_leaf->kind == JsonLeaf::Kind::string &&
				desktop_document.find("opening.sprite_sheet") == nullptr && desktop_document.find("opening.fill_frame") == nullptr &&
				desktop_document.find("opening.palette_sheet") == nullptr,
				"Desktop colour is eligible while captured texture/palette names remain outside the tree");
			desktop_document.commit(desktop_document.edited(*desktop_leaf, "\"11223344\""), desktop_path);
			require(desktop_document.reverted(desktop_path) == desktop_baseline,
				"Desktop reset restores its original escaped tint token and every CRLF byte");
			const std::string desktop_external_number = desktop_reference.replaced(*desktop_reference.find("number"), "25");
			const JsonSpanDocument desktop_external_reference(desktop_external_number, "desktop-colour-reference");
			const TuneSplice desktop_splice = desktop_document.splice(desktop_external_number, false);
			require(desktop_splice.conflicts.empty() && desktop_splice.written == 1 && desktop_splice.text ==
				desktop_external_reference.replaced(*desktop_external_reference.find(desktop_path), "\"11223344\"") &&
				count_of(desktop_splice.text, "\r\n") == count_of(desktop_baseline, "\r\n"),
				"Desktop splice preserves every unrelated escaped byte, captured string, external number and CRLF");
			const std::string desktop_pending = desktop_document.current().text();
			bool desktop_wrong_type = false;
			try { desktop_document.commit(desktop_document.edited(*desktop_document.find(desktop_path), "false"), desktop_path); }
			catch (const std::invalid_argument&) { desktop_wrong_type = true; }
			require(desktop_wrong_type && desktop_document.current().text() == desktop_pending && desktop_document.edits().size() == 1,
				"A wrong-type desktop commit retains the pending string edit");
			for (const std::string& desktop_invalid : { std::string("{}"),
				desktop_reference.replaced(*desktop_reference.find(desktop_path), "null"),
				desktop_reference.replaced(*desktop_reference.find(desktop_path), "{\"value\":\"RED\"}"),
				std::string("{\"opening.desktop_colour\":\"RED\"}"),
				std::string("{\"opening\":{\"desktop_colour.r\":\"RED\"}}") })
			{
				const TuneDocument desktop_unsupported("presentation", desktop_invalid);
				const TuneSplice desktop_refused = desktop_document.splice(desktop_invalid, true);
				require(desktop_unsupported.find(desktop_path) == nullptr && desktop_refused.written == 0 &&
					desktop_refused.conflicts.size() == 1 && desktop_refused.text == desktop_invalid,
					"Force cannot insert a desktop colour, replace null/a container or expose literal aliases");
			}
		}
		{
			const std::string power_light_path = "opening.monitor_art.power_light";
			const std::string power_light_baseline = "{\r\n  \"opening\":{\"monitor_art\":{\"power_light\":\"\\u00435D6AA\","
				"\"ink\":\"BLACK\",\"highlight\":\"WHITE\"},\"sprite_sheet\":\"captured\",\"fill_frame\":\"solid\",\"palette_sheet\":\"unchanged\"},\r\n"
				"  \"note\":\"keep\\\\bytes\",\r\n  \"number\":1\r\n}\r\n";
			const JsonSpanDocument power_light_reference(power_light_baseline, "power-light-colour-reference");
			TuneDocument power_light_document("presentation", power_light_baseline);
			const JsonLeaf* power_light_leaf = power_light_document.find(power_light_path);
			require(power_light_leaf != nullptr && power_light_leaf->kind == JsonLeaf::Kind::string &&
				power_light_document.find("opening.sprite_sheet") == nullptr && power_light_document.find("opening.fill_frame") == nullptr &&
				power_light_document.find("opening.palette_sheet") == nullptr &&
				power_light_document.find("opening.monitor_art.ink") == nullptr &&
				power_light_document.find("opening.monitor_art.highlight") == nullptr,
				"Power-light colour is eligible while captured texture/palette names remain outside the tree");
			power_light_document.commit(power_light_document.edited(*power_light_leaf, "\"11223344\""), power_light_path);
			require(power_light_document.reverted(power_light_path) == power_light_baseline,
				"Power-light reset restores its original escaped tint token and every CRLF byte");
			const std::string power_light_external_number = power_light_reference.replaced(*power_light_reference.find("number"), "25");
			const JsonSpanDocument power_light_external_reference(power_light_external_number, "power-light-colour-reference");
			const TuneSplice power_light_splice = power_light_document.splice(power_light_external_number, false);
			require(power_light_splice.conflicts.empty() && power_light_splice.written == 1 && power_light_splice.text ==
				power_light_external_reference.replaced(*power_light_external_reference.find(power_light_path), "\"11223344\"") &&
				count_of(power_light_splice.text, "\r\n") == count_of(power_light_baseline, "\r\n"),
				"Power-light splice preserves every unrelated escaped byte, captured string, external number and CRLF");
			const std::string power_light_pending = power_light_document.current().text();
			bool power_light_wrong_type = false;
			try { power_light_document.commit(power_light_document.edited(*power_light_document.find(power_light_path), "false"), power_light_path); }
			catch (const std::invalid_argument&) { power_light_wrong_type = true; }
			require(power_light_wrong_type && power_light_document.current().text() == power_light_pending && power_light_document.edits().size() == 1,
				"A wrong-type power-light commit retains the pending string edit");
			for (const std::string& power_light_invalid : { std::string("{}"),
				power_light_reference.replaced(*power_light_reference.find(power_light_path), "null"),
				power_light_reference.replaced(*power_light_reference.find(power_light_path), "{\"value\":\"RED\"}"),
				std::string("{\"opening.monitor_art.power_light\":\"RED\"}"),
				std::string("{\"opening\":{\"monitor_art\":{\"power_light.r\":\"RED\"}}}") })
			{
				const TuneDocument power_light_unsupported("presentation", power_light_invalid);
				const TuneSplice power_light_refused = power_light_document.splice(power_light_invalid, true);
				require(power_light_unsupported.find(power_light_path) == nullptr && power_light_refused.written == 0 &&
					power_light_refused.conflicts.size() == 1 && power_light_refused.text == power_light_invalid,
					"Force cannot insert a power-light colour, replace null/a container or expose literal aliases");
			}
		}
		{
			const std::string program_path = "opening.program_colour";
			const std::string program_baseline = "{\r\n  \"opening\":{\"program_colour\":\"\\u0032C2C2C\","
				"\"desktop_colour\":\"008080\",\"monitor_art\":{\"ink\":\"BLACK\",\"highlight\":\"WHITE\",\"power_light\":\"C5D6AA\"},"
				"\"sprite_sheet\":\"captured\",\"fill_frame\":\"solid\",\"palette_sheet\":\"unchanged\"},\r\n"
				"  \"note\":\"keep\\\\bytes\",\r\n  \"number\":1\r\n}\r\n";
			const JsonSpanDocument program_reference(program_baseline, "program-colour-reference");
			TuneDocument program_document("presentation", program_baseline);
			const JsonLeaf* program_leaf = program_document.find(program_path);
			require(program_leaf != nullptr && program_leaf->kind == JsonLeaf::Kind::string &&
				program_document.find("opening.sprite_sheet") == nullptr && program_document.find("opening.fill_frame") == nullptr &&
				program_document.find("opening.palette_sheet") == nullptr &&
				program_document.find("opening.monitor_art.ink") == nullptr &&
				program_document.find("opening.monitor_art.highlight") == nullptr,
				"Program colour is eligible while captured texture/palette names remain outside the tree");
			program_document.commit(program_document.edited(*program_leaf, "\"11223344\""), program_path);
			require(program_document.reverted(program_path) == program_baseline,
				"Program reset restores its original escaped tint token and every CRLF byte");
			const std::string program_external_number = program_reference.replaced(*program_reference.find("number"), "25");
			const JsonSpanDocument program_external_reference(program_external_number, "program-colour-reference");
			const TuneSplice program_splice = program_document.splice(program_external_number, false);
			require(program_splice.conflicts.empty() && program_splice.written == 1 && program_splice.text ==
				program_external_reference.replaced(*program_external_reference.find(program_path), "\"11223344\"") &&
				count_of(program_splice.text, "\r\n") == count_of(program_baseline, "\r\n"),
				"Program splice preserves every unrelated escaped byte, captured string, external number and CRLF");
			const std::string program_pending = program_document.current().text();
			bool program_wrong_type = false;
			try { program_document.commit(program_document.edited(*program_document.find(program_path), "false"), program_path); }
			catch (const std::invalid_argument&) { program_wrong_type = true; }
			require(program_wrong_type && program_document.current().text() == program_pending && program_document.edits().size() == 1,
				"A wrong-type program commit retains the pending string edit");
			for (const std::string& program_invalid : { std::string("{}"),
				program_reference.replaced(*program_reference.find(program_path), "null"),
				program_reference.replaced(*program_reference.find(program_path), "{\"value\":\"RED\"}"),
				std::string("{\"opening.program_colour\":\"RED\"}"),
				std::string("{\"opening\":{\"program_colour.r\":\"RED\"}}") })
			{
				const TuneDocument program_unsupported("presentation", program_invalid);
				const TuneSplice program_refused = program_document.splice(program_invalid, true);
				require(program_unsupported.find(program_path) == nullptr && program_refused.written == 0 &&
					program_refused.conflicts.size() == 1 && program_refused.text == program_invalid,
					"Force cannot insert a program colour, replace null/a container or expose literal aliases");
			}
		}
		{
			const std::string glass_path = "opening.glass_colour";
			const std::string glass_baseline = "{\r\n  \"opening\":{\"glass_colour\":\"\\u003161616\","
				"\"desktop_colour\":\"008080\",\"monitor_art\":{\"ink\":\"BLACK\",\"highlight\":\"WHITE\",\"power_light\":\"C5D6AA\"},"
				"\"sprite_sheet\":\"captured\",\"fill_frame\":\"solid\",\"palette_sheet\":\"unchanged\"},\r\n"
				"  \"note\":\"keep\\\\bytes\",\r\n  \"number\":1\r\n}\r\n";
			const JsonSpanDocument glass_reference(glass_baseline, "glass-colour-reference");
			TuneDocument glass_document("presentation", glass_baseline);
			const JsonLeaf* glass_leaf = glass_document.find(glass_path);
			require(glass_leaf != nullptr && glass_leaf->kind == JsonLeaf::Kind::string &&
				glass_document.find("opening.sprite_sheet") == nullptr && glass_document.find("opening.fill_frame") == nullptr &&
				glass_document.find("opening.palette_sheet") == nullptr &&
				glass_document.find("opening.monitor_art.ink") == nullptr &&
				glass_document.find("opening.monitor_art.highlight") == nullptr,
				"Glass colour is eligible while captured texture/palette names remain outside the tree");
			glass_document.commit(glass_document.edited(*glass_leaf, "\"11223344\""), glass_path);
			require(glass_document.reverted(glass_path) == glass_baseline,
				"Glass reset restores its original escaped tint token and every CRLF byte");
			const std::string glass_external_number = glass_reference.replaced(*glass_reference.find("number"), "25");
			const JsonSpanDocument glass_external_reference(glass_external_number, "glass-colour-reference");
			const TuneSplice glass_splice = glass_document.splice(glass_external_number, false);
			require(glass_splice.conflicts.empty() && glass_splice.written == 1 && glass_splice.text ==
				glass_external_reference.replaced(*glass_external_reference.find(glass_path), "\"11223344\"") &&
				count_of(glass_splice.text, "\r\n") == count_of(glass_baseline, "\r\n"),
				"Glass splice preserves every unrelated escaped byte, captured string, external number and CRLF");
			const std::string glass_pending = glass_document.current().text();
			const TuneEdit glass_held_edit = glass_document.edits().at(glass_path);
			bool glass_wrong_type = false;
			try { glass_document.commit(glass_document.edited(*glass_document.find(glass_path), "false"), glass_path); }
			catch (const std::invalid_argument&) { glass_wrong_type = true; }
			require(glass_wrong_type && glass_document.current().text() == glass_pending && glass_document.edits().size() == 1 &&
				glass_document.edits().at(glass_path).baseline_token == glass_held_edit.baseline_token &&
				glass_document.edits().at(glass_path).current_token == glass_held_edit.current_token,
				"A wrong-type glass commit retains the pending string edit");
			for (const std::string& glass_invalid : { std::string("{}"),
				glass_reference.replaced(*glass_reference.find(glass_path), "null"),
				glass_reference.replaced(*glass_reference.find(glass_path), "{\"value\":\"RED\"}"),
				std::string("{\"opening.glass_colour\":\"RED\"}"),
				std::string("{\"opening\":{\"glass_colour.r\":\"RED\"}}") })
			{
				const TuneDocument glass_unsupported("presentation", glass_invalid);
				const TuneSplice glass_refused = glass_document.splice(glass_invalid, true);
				require(glass_unsupported.find(glass_path) == nullptr && glass_refused.written == 0 &&
					glass_refused.conflicts.size() == 1 && glass_refused.text == glass_invalid,
					"Force cannot insert a glass colour, replace null/a container or expose literal aliases");
			}
		}
		{
			const std::string body_path = "opening.silhouette";
			const std::string body_baseline = "{\r\n  \"opening\":{\"silhouette\":\"\\u0030E0E0E\","
				"\"silhouette_art\":{\"ink\":\"000000\",\"rim\":\"343434\",\"shade\":\"080808\"},"
				"\"sprite_sheet\":\"captured\",\"fill_frame\":\"solid\",\"palette_sheet\":\"unchanged\"},\r\n"
				"  \"note\":\"keep\\\\bytes\",\r\n  \"number\":1\r\n}\r\n";
			const JsonSpanDocument body_reference(body_baseline, "silhouette-body-colour-reference");
			TuneDocument body_document("presentation", body_baseline);
			const JsonLeaf* body_leaf = body_document.find(body_path);
			require(body_leaf != nullptr && body_leaf->kind == JsonLeaf::Kind::string &&
				body_document.find("opening.sprite_sheet") == nullptr && body_document.find("opening.fill_frame") == nullptr &&
				body_document.find("opening.palette_sheet") == nullptr &&
				body_document.find("opening.silhouette_art.ink") != nullptr &&
				body_document.find("opening.silhouette_art.rim") != nullptr &&
				body_document.find("opening.silhouette_art.shade") != nullptr,
				"Body and its separate ink/rim/shade siblings are eligible while captured texture/palette names remain outside the tree");
			body_document.commit(body_document.edited(*body_leaf, "\"11223344\""), body_path);
			for (const char* body_sibling : { "opening.silhouette_art.ink", "opening.silhouette_art.rim", "opening.silhouette_art.shade" })
			{
				require(body_document.current().token(*body_document.find(body_sibling)) ==
					body_reference.token(*body_reference.find(body_sibling)),
					"Body commit retains each exact ink/rim/shade sibling token");
			}
			require(body_document.reverted(body_path) == body_baseline,
				"Silhouette body reset restores its original escaped tint token and every CRLF byte");
			const std::string body_external_number = body_reference.replaced(*body_reference.find("number"), "25");
			const JsonSpanDocument body_external_reference(body_external_number, "silhouette-body-colour-reference");
			const TuneSplice body_splice = body_document.splice(body_external_number, false);
			require(body_splice.conflicts.empty() && body_splice.written == 1 && body_splice.text ==
				body_external_reference.replaced(*body_external_reference.find(body_path), "\"11223344\"") &&
				count_of(body_splice.text, "\r\n") == count_of(body_baseline, "\r\n"),
				"Silhouette body splice preserves every unrelated escaped byte, captured string, external number and CRLF");
			const std::string body_pending = body_document.current().text();
			const TuneEdit body_held_edit = body_document.edits().at(body_path);
			bool body_wrong_type = false;
			try { body_document.commit(body_document.edited(*body_document.find(body_path), "false"), body_path); }
			catch (const std::invalid_argument&) { body_wrong_type = true; }
			require(body_wrong_type && body_document.current().text() == body_pending && body_document.edits().size() == 1 &&
				body_document.edits().at(body_path).baseline_token == body_held_edit.baseline_token &&
				body_document.edits().at(body_path).current_token == body_held_edit.current_token,
				"A wrong-type body commit retains the pending string edit");
			for (const std::string& body_invalid : { std::string("{}"),
				body_reference.replaced(*body_reference.find(body_path), "null"),
				body_reference.replaced(*body_reference.find(body_path), "{\"value\":\"RED\"}"),
				std::string("{\"opening.silhouette\":\"RED\"}"),
				std::string("{\"opening\":{\"silhouette.r\":\"RED\"}}") })
			{
				const TuneDocument body_unsupported("presentation", body_invalid);
				const TuneSplice body_refused = body_document.splice(body_invalid, true);
				require(body_unsupported.find(body_path) == nullptr && body_refused.written == 0 &&
					body_refused.conflicts.size() == 1 && body_refused.text == body_invalid,
					"Force cannot insert a body colour, replace null/a container or expose literal aliases");
			}
		}
		{
			const std::string scribble_path = "opening.scribble";
			const std::string scribble_baseline = "{\r\n  \"opening\":{\"scribble\":\"\\u0033050C0\","
				"\"sticky_notes\":[\"FFF27A\",\"FFB6D5\",\"AEEBFF\",\"C8F59A\"],\"stickers\":[\"FF5A5A\"],"
				"\"sprite_sheet\":\"captured\",\"fill_frame\":\"solid\",\"palette_sheet\":\"unchanged\"},\r\n"
				"  \"note\":\"keep\\\\bytes\",\r\n  \"number\":1\r\n}\r\n";
			const JsonSpanDocument scribble_reference(scribble_baseline, "scribble-colour-reference");
			TuneDocument scribble_document("presentation", scribble_baseline);
			const JsonLeaf* scribble_leaf = scribble_document.find(scribble_path);
			require(scribble_leaf != nullptr && scribble_leaf->kind == JsonLeaf::Kind::string &&
				scribble_document.find("opening.sprite_sheet") == nullptr && scribble_document.find("opening.fill_frame") == nullptr &&
				scribble_document.find("opening.palette_sheet") == nullptr &&
				scribble_document.find("opening.sticky_notes[0]") == nullptr &&
				scribble_document.find("opening.stickers[0]") == nullptr,
				"Scribble is eligible while captured texture/palette names and authored note/sticker array colours remain outside the string tree");
			scribble_document.commit(scribble_document.edited(*scribble_leaf, "\"11223344\""), scribble_path);
			require(scribble_document.reverted(scribble_path) == scribble_baseline,
				"Scribble reset restores its original escaped tint token and every CRLF byte");
			const std::string scribble_external_number = scribble_reference.replaced(*scribble_reference.find("number"), "25");
			const JsonSpanDocument scribble_external_reference(scribble_external_number, "scribble-colour-reference");
			const TuneSplice scribble_splice = scribble_document.splice(scribble_external_number, false);
			require(scribble_splice.conflicts.empty() && scribble_splice.written == 1 && scribble_splice.text ==
				scribble_external_reference.replaced(*scribble_external_reference.find(scribble_path), "\"11223344\"") &&
				count_of(scribble_splice.text, "\r\n") == count_of(scribble_baseline, "\r\n"),
				"Scribble splice preserves both note/sticker arrays, every unrelated escaped byte, captured string, external number and CRLF");
			const std::string scribble_pending = scribble_document.current().text();
			const TuneEdit scribble_held_edit = scribble_document.edits().at(scribble_path);
			bool scribble_wrong_type = false;
			try { scribble_document.commit(scribble_document.edited(*scribble_document.find(scribble_path), "false"), scribble_path); }
			catch (const std::invalid_argument&) { scribble_wrong_type = true; }
			require(scribble_wrong_type && scribble_document.current().text() == scribble_pending && scribble_document.edits().size() == 1 &&
				scribble_document.edits().at(scribble_path).baseline_token == scribble_held_edit.baseline_token &&
				scribble_document.edits().at(scribble_path).current_token == scribble_held_edit.current_token,
				"A wrong-type scribble commit retains the pending string edit");
			for (const std::string& scribble_invalid : { std::string("{}"),
				scribble_reference.replaced(*scribble_reference.find(scribble_path), "null"),
				scribble_reference.replaced(*scribble_reference.find(scribble_path), "{\"value\":\"RED\"}"),
				std::string("{\"opening.scribble\":\"RED\"}"),
				std::string("{\"opening\":{\"scribble.r\":\"RED\"}}") })
			{
				const TuneDocument scribble_unsupported("presentation", scribble_invalid);
				const TuneSplice scribble_refused = scribble_document.splice(scribble_invalid, true);
				require(scribble_unsupported.find(scribble_path) == nullptr && scribble_refused.written == 0 &&
					scribble_refused.conflicts.size() == 1 && scribble_refused.text == scribble_invalid,
					"Force cannot insert a scribble colour, replace null/a container or expose literal aliases");
			}
		}
	}

	void check_colour_document_opening_details()
	{
		{
			// Save/conflict/force and LF/CRLF consumption reuse the42-path loops;
			// retain only the new field's escaped-token/type/no-insertion checks here.
			const std::string crack_path = "opening.crack";
			const std::string crack_baseline = "{\r\n\"opening\":{\"crack\":\"\\u0033A3A3A\",\"coffee\":\"keep\"},\r\n\"number\":1\r\n}\r\n";
			TuneDocument crack_document("presentation", crack_baseline);
			const JsonSpanDocument crack_reference(crack_baseline, "crack-document-reference");
			const auto* crack_leaf = crack_document.find(crack_path);
			require(crack_leaf != nullptr && crack_leaf->kind == JsonLeaf::Kind::string && crack_document.find("opening.coffee") == nullptr,
				"The crack string is eligible while the unselected coffee policy stays unsupported");
			crack_document.commit(crack_document.edited(*crack_leaf, "\"2060C080\""), crack_path);
			const auto crack_held = crack_document.edits().at(crack_path);const auto crack_pending = crack_document.current().text();
			bool crack_bad_type = false;
			try { crack_document.commit(crack_document.edited(*crack_document.find(crack_path), "false"), crack_path); }
			catch (const std::invalid_argument&) { crack_bad_type = true; }
			require(crack_bad_type && crack_document.current().text() == crack_pending && crack_document.edits().size() == 1 &&
				crack_document.edits().at(crack_path).baseline_token == crack_held.baseline_token &&
				crack_document.edits().at(crack_path).current_token == crack_held.current_token && crack_document.reverted(crack_path) == crack_baseline,
				"Crack wrong-type commit retains both exact tokens/current text and escaped CRLF reset bytes");
			for (const std::string& missing : { std::string("{}"), crack_reference.replaced(*crack_reference.find(crack_path), "null"),
				crack_reference.replaced(*crack_reference.find(crack_path), "{\"value\":\"RED\"}"),
				std::string("{\"opening.crack\":\"RED\"}"), std::string("{\"opening\":{\"crack.r\":\"RED\"}}") })
			{
				const TuneDocument unsupported("presentation", missing);const TuneSplice refused = crack_document.splice(missing, true);
				require(unsupported.find(crack_path) == nullptr && refused.written == 0 && refused.conflicts.size() == 1 && refused.text == missing,
					"Even force cannot insert crack, replace null/container or expose literal/channel aliases");
			}
		}
		{
			const std::string acting_baseline = "{\r\n  \"opening\":{\"acting\":{\"face\":\"\\u0057HITE\","
				"\"sweat\":\"\\u0038FD3FF\",\"hop\":0.22},\"sprite_sheet\":\"captured\",\"fill_frame\":\"solid\"},\r\n"
				"  \"note\":\"keep\\\\bytes\",\r\n  \"number\":1\r\n}\r\n";
			for (const char* acting_field : { "face", "sweat" })
			{
				const std::string acting_path = std::string("opening.acting.") + acting_field;
				const std::string acting_sibling = std::string("opening.acting.") + (std::string(acting_field) == "face" ? "sweat" : "face");
				const JsonSpanDocument acting_reference(acting_baseline, "acting-colour-reference");
				TuneDocument acting_document("presentation", acting_baseline);
				const JsonLeaf* acting_leaf = acting_document.find(acting_path);
				require(acting_leaf != nullptr && acting_leaf->kind == JsonLeaf::Kind::string &&
					acting_document.find(acting_sibling) != nullptr && acting_document.find("opening.sprite_sheet") == nullptr &&
					acting_document.find("opening.fill_frame") == nullptr,
					"Both nested acting tints are eligible while captured texture names stay outside the string tree");
				acting_document.commit(acting_document.edited(*acting_leaf, "\"11223344\""), acting_path);
				require(acting_document.reverted(acting_path) == acting_baseline,
					"Each acting reset restores its original escaped token, sibling and every CRLF byte");
				const std::string acting_external_number = acting_reference.replaced(*acting_reference.find("number"), "25");
				const JsonSpanDocument acting_external_reference(acting_external_number, "acting-colour-reference");
				const TuneSplice acting_splice = acting_document.splice(acting_external_number, false);
				require(acting_splice.conflicts.empty() && acting_splice.written == 1 && acting_splice.text ==
					acting_external_reference.replaced(*acting_external_reference.find(acting_path), "\"11223344\"") &&
					count_of(acting_splice.text, "\r\n") == count_of(acting_baseline, "\r\n"),
					"Each acting splice preserves its sibling, captured strings, unrelated escaped bytes/external number and CRLF");
				const std::string acting_pending = acting_document.current().text();
				const TuneEdit acting_held_edit = acting_document.edits().at(acting_path);
				bool acting_wrong_type = false;
				try { acting_document.commit(acting_document.edited(*acting_document.find(acting_path), "false"), acting_path); }
				catch (const std::invalid_argument&) { acting_wrong_type = true; }
				require(acting_wrong_type && acting_document.current().text() == acting_pending && acting_document.edits().size() == 1 &&
					acting_document.edits().at(acting_path).baseline_token == acting_held_edit.baseline_token &&
					acting_document.edits().at(acting_path).current_token == acting_held_edit.current_token,
					"A wrong-type acting commit retains the exact pending string and both edit tokens");
				for (const std::string& acting_invalid : { std::string("{}"),
					acting_reference.replaced(*acting_reference.find(acting_path), "null"),
					acting_reference.replaced(*acting_reference.find(acting_path), "{\"value\":\"RED\"}"),
					"{\"" + acting_path + "\":\"RED\"}",
					std::string("{\"opening\":{\"acting.") + acting_field + "\":\"RED\"}}",
					std::string("{\"opening.acting\":{\"") + acting_field + "\":\"RED\"}}",
					std::string("{\"opening\":{\"acting\":{\"") + acting_field + ".r\":\"RED\"}}}" })
				{
					const TuneDocument acting_unsupported("presentation", acting_invalid);
					const TuneSplice acting_refused = acting_document.splice(acting_invalid, true);
					require(acting_unsupported.find(acting_path) == nullptr && acting_refused.written == 0 &&
						acting_refused.conflicts.size() == 1 && acting_refused.text == acting_invalid,
						"Force cannot insert an acting tint, replace null/a container or expose literal parent/member aliases");
				}
			}
		}
	}
