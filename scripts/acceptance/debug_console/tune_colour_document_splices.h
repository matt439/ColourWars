// Included by tune_colour_document_checks.h inside its existing private namespace.
// Generic colour splices, conflicts, rebases and literal-key aliases.

	void check_colour_document_splices()
	{
		const std::string local = "movement_feedback.colour";
		const std::string baseline = "{\r\n  \"movement_feedback\":{\"colour\":\"\\u0052\\u0045D\"},\r\n"
			"  \"font\":\"untouched\\\\path\",\r\n  \"number\":1\r\n}\r\n";
		TuneDocument document("presentation", baseline);
		const JsonLeaf* colour = document.find(local);
		require(colour != nullptr && colour->kind == JsonLeaf::Kind::string &&
			document.current().token(*colour) == "\"\\u0052\\u0045D\"" && document.find("font") == nullptr,
			"TuneDocument exposes the audited colour while preserving its escaped baseline spelling");
		const JsonLeaf stale = *colour;
		document.commit(document.edited(*colour, "\"11223344\""), local);
		require(document.edits().size() == 1 && document.edits().at(local).baseline_token == "\"\\u0052\\u0045D\"" &&
			document.edits().at(local).current_token == "\"11223344\"" && document.reverted(local) == baseline,
			"A string edit retains exact quoted baseline/current tokens and resets byte for byte");
		const std::string pending = document.current().text();
		const auto rejected_commit = [&](const std::string& candidate, const std::string& path)
		{
			bool refused = false;
			try { document.commit(candidate, path); } catch (const std::invalid_argument&) { refused = true; }
			require(refused && document.current().text() == pending && document.edits().size() == 1 &&
				document.edits().at(local).current_token == "\"11223344\"",
				"An unsupported, type-changing or multi-field commit leaves the string edit atomic");
		};
		const JsonLeaf* current = document.find(local);
		rejected_commit(document.current().replaced(*current, "false"), local);
		rejected_commit(document.current().replaced(*document.current().find("font"), "\"other\""), "font");
		const JsonSpanDocument other(document.current().replaced(*current, "\"BLUE\""), "multi-field");
		rejected_commit(other.replaced(*other.find("number"), "2"), local);
		for (const JsonLeaf leaf : { stale, *document.current().find("font") })
		{
			bool refused = false;
			try { document.edited(leaf, "\"WHITE\""); } catch (const std::invalid_argument&) { refused = true; }
			require(refused && document.current().text() == pending,
				"A stale string span or arbitrary string cannot be edited through TuneDocument");
		}
		const JsonSpanDocument external_baseline(baseline, "external-colour");
		const std::string external = external_baseline.replaced(*external_baseline.find("number"), "25");
		const TuneSplice splice = document.splice(external, false);
		const JsonSpanDocument external_document(external, "external-colour");
		require(splice.conflicts.empty() && splice.written == 1 && splice.text ==
			external_document.replaced(*external_document.find(local), "\"11223344\"") &&
			count_of(splice.text, "\r\n") == count_of(baseline, "\r\n") &&
			splice.text.find("untouched\\\\path") != std::string::npos,
			"A CRLF colour splice preserves unrelated numeric edits, escape bytes and every newline");
		const std::string conflicting = external_document.replaced(*external_document.find(local), "\"BLUE\"");
		require(document.splice(conflicting, false).conflicts.size() == 1 &&
			document.splice(conflicting, false).written == 0 && document.splice(conflicting, true).written == 1,
			"A changed colour token conflicts unless explicitly forced");
		for (const char* missing : { R"({"movement_feedback":{}})", R"({"movement_feedback":{"colour":null}})",
			R"({"movement_feedback":{"colour":{"value":"RED"}}})" })
		{
			const TuneSplice refused = document.splice(missing, true);
			require(refused.conflicts.size() == 1 && refused.written == 0 && refused.text == missing,
				"Even force cannot invent a missing colour key or replace null/a container");
		}
		document.rebase(splice.text);
		require(document.edits().empty() && document.current().text() == splice.text &&
			document.baseline().text() == splice.text && document.current_hash() == fnv1a(splice.text),
			"Reload/rebase adopts the full string document as its new hash and empty edit baseline");
		for (const char* alias : { R"({"movement_feedback.colour":"WHITE"})",
			R"({"debug_overlay":{"station_label.colour":"WHITE"}})",
			R"({"main_menu":{"backgrounds.home":"WHITE"}})" })
		{
			const TuneDocument literal("presentation", alias);
			require(literal.prefixed_paths().empty(), "Literal path punctuation cannot turn an arbitrary string key into a colour");
		}
		for (const char* text : { R"({"movement_feedback.colour":"BLUE","movement_feedback":{"colour":"RED"}})",
			R"({"movement_feedback":{"colour":"RED"},"movement_feedback.colour":"BLUE"})" })
		{
			const TuneDocument literal("presentation", text);
			const JsonLeaf* leaf = literal.find(local);
			require(leaf == nullptr || (literal.current().token(*leaf) == "\"RED\"" &&
				leaf->begin == std::string(text).find("\"RED\"")),
				"An alias before or beside the real object never exposes the literal-key string as a colour");
		}
		TuneDocument alias_edits("presentation", baseline);
		alias_edits.commit(alias_edits.edited(*alias_edits.find(local), "\"BLUE\""), local);
		const std::string literal_disk = R"({"movement_feedback.colour":"RED"})";
		const TuneSplice alias_disk = alias_edits.splice(literal_disk, true);
		require(alias_disk.written == 0 && alias_disk.conflicts.size() == 1 && alias_disk.text == literal_disk,
			"Even force cannot splice an audited colour edit into a literal path-alias string");
		TuneDocument number("tuning", R"({"number":1})");
		number.commit(number.edited(*number.find("number"), "2"), "number");
		const TuneSplice number_string = number.splice(R"({"number":"1"})", true);
		require(number_string.written == 0 && number_string.conflicts.size() == 1,
			"Scanning general strings does not make an on-disk numeric-to-string change force-writable");
	}
