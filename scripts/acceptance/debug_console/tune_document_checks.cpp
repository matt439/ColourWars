#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_menu_model.h"
#include "game/debug/debug_tokenizer.h"
#include "game/debug/json_spans.h"
#include "game/debug/tune_document.h"
#include "game/debug/tune_scope.h"
#include "engine/math/scalar.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <process.h>

// Live tuning, the document side: JSON spans, the colour TuneDocument, tune
// scopes and the consumed-leaf check.
namespace debug_console_acceptance
{
	namespace
	{
	std::string last_key(const std::string& path)
	{
		std::string key = path.substr(path.find_last_of('.') + 1);
		const std::size_t bracket = key.find('[');
		return bracket == std::string::npos ? key : key.substr(0, bracket);
	}

	void check_colour_document()
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
				silhouette_document.find("opening.fill_frame") == nullptr && silhouette_document.find("opening.silhouette") == nullptr,
				"Only the three audited silhouette-art colours are eligible; captured texture names and separate body fill stay outside the tree");
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
	}

	void check_json_spans(const std::filesystem::path& game_directory)
	{
		check_colour_document();
		for (const wchar_t* file : { L"tuning.json", L"presentation.json" })
		{
			const std::string text = read_text(game_directory / file);
			const JsonSpanDocument document(text, "content");
			require(document.leaves().size() > 50 && document.string_count() > 10,
				"A content file scans to its numbers, booleans and strings");
			for (const JsonLeaf& leaf : document.leaves())
			{
				require(document.replaced(leaf, document.token(leaf)) == text,
					"Re-splicing " + leaf.path + "'s own token reproduces the file byte for byte");
			}
			const JsonLeaf& first = document.leaves().front();
			const std::string changed = document.replaced(first, "12345");
			require(changed.compare(0, first.begin, text, 0, first.begin) == 0 &&
				changed.substr(first.begin + 5) == text.substr(first.end) &&
				changed.substr(first.begin, 5) == "12345", "A splice changes only that span");
		}

		const std::string synthetic = "{\r\n  \"a\": [1, [2, true]],\r\n  \"s\": \"q\\\"\xc3\xa9\",\r\n"
			"  \"b\": -1.5e3,\r\n  \"a\": 9,\r\n  \"o\": { \"x\": false }\r\n}\r\n";
		const JsonSpanDocument document(synthetic, "synthetic");
		std::vector<std::string> paths;
		std::string joined;
		for (const JsonLeaf& leaf : document.leaves())
		{
			paths.push_back(leaf.path + "=" + std::string(document.token(leaf)));
			joined += " " + paths.back();
		}
		require(paths == std::vector<std::string>{ "a[0]=1", "a[1][0]=2", "a[1][1]=true", "s=\"q\\\"\xc3\xa9\"", "b=-1.5e3",
			"o.x=false" }, "Nested arrays get [i] paths, exponents and signs are one token, and a "
			"repeated key resolves to the first; got" + joined);

		const std::string spliced = document.replaced(*document.find("b"), "2");
		require(count_of(spliced, "\r\n") == count_of(synthetic, "\r\n") &&
			spliced.find("q\\\"\xc3\xa9") != std::string::npos,
			"CRLF and escapes survive a splice");
		for (const char* newline : { "\n", "\r\n" })
		{
			for (const char* token : { R"("")", R"("plain")", R"("slash\\")", R"("quote\"tail")",
				R"("\\\"")", R"("q\"\\\/\b\f\n\r\t\u0041\uD83D\uDE00")" })
			{
				const std::string text = std::string("{") + newline + "  \"s\": " + token + "," + newline +
					"  \"later\": 9" + newline + "}" + newline;
				const JsonSpanDocument strings(text, "escaped-strings");
				const JsonLeaf* leaf = strings.find("s");
				require(leaf != nullptr && leaf->kind == JsonLeaf::Kind::string && strings.token(*leaf) == token &&
					strings.replaced(*leaf, strings.token(*leaf)) == text,
					"A string span includes both quotes and exact empty/escaped/Unicode bytes");
				const std::string changed = strings.replaced(*leaf, "\"11223344\"");
				require(changed.substr(0, leaf->begin) == text.substr(0, leaf->begin) &&
					changed.substr(leaf->begin + 10) == text.substr(leaf->end) &&
					count_of(changed, newline) == count_of(text, newline),
					"Splicing an escaped string preserves all neighbouring tokens and LF/CRLF bytes");
			}
		}
		const JsonSpanDocument duplicates(R"({"a":"first","\u0061":2,"b":null,"b":"later",
			"c":{"x":"kept"},"c":false,"d":[null,"one",{"x":"two"}],"e":true,"e":"later"})",
			"first-key-wins");
		require(duplicates.leaves().size() == 5 && duplicates.token(*duplicates.find("a")) == "\"first\"" &&
			duplicates.find("b") == nullptr && duplicates.find("c") == nullptr &&
			duplicates.token(*duplicates.find("c.x")) == "\"kept\"" &&
			duplicates.token(*duplicates.find("d[1]")) == "\"one\"" &&
			duplicates.token(*duplicates.find("d[2].x")) == "\"two\"" &&
			duplicates.token(*duplicates.find("e")) == "true",
			"First string, null, container and boolean keys suppress later scalar keys, including escaped duplicate names");
		for (const char* bad : { "{\"a\": 1,}", "{ // c\n \"a\": 1 }", "{\"a\": \"x }", "{\"a\": 1}\n2",
			R"({"a":"\q"})", R"({"a":"\u00ZZ"})" })
		{
			std::string message;
			try
			{
				JsonSpanDocument(bad, "bad.json");
			}
			catch (const std::exception& error)
			{
				message = error.what();
			}
			require(message.find("bad.json: line ") != std::string::npos &&
				message.find(", column ") != std::string::npos,
				std::string("Malformed JSON fails with line and column: ") + bad + " gave '" + message + "'");
		}
	}

	void check_tune_scope(const std::filesystem::path& game_directory)
	{
		const TuneDocument tuning("tuning", read_text(game_directory / L"tuning.json"));
		const TuneDocument presentation("presentation", read_text(game_directory / L"presentation.json"));
		std::vector<std::string> paths = tuning.prefixed_paths();
		const std::vector<std::string> more = presentation.prefixed_paths();
		paths.insert(paths.end(), more.begin(), more.end());
		std::size_t colour_count = 0;
		for (const std::string& path : paths)
		{
			const TuneDocument& file = path.starts_with("presentation.") ? presentation : tuning;
			const JsonLeaf* leaf = file.find(path.substr(path.find('.') + 1));
			require(leaf != nullptr, "Every listed tune path is eligible through TuneDocument::find: " + path);
			if (leaf->kind == JsonLeaf::Kind::string)
			{
				++colour_count;
				require(std::find(colour_paths.begin(), colour_paths.end(), path) != colour_paths.end() &&
					tune_colour_path(path), "Only the independently audited thirty-two colours appear as string tune paths: " + path);
			}
		}
		require(colour_count == colour_paths.size(), "Exactly thirty-two authored colour strings are tunable");
		for (std::size_t index = 0; index < colour_paths.size(); ++index)
		{
			const std::string path = colour_paths[index];
			require(tune_colour_path(path) && tune_scope_of(path) ==
				(index == 6 ? TuneScope::next_screen : TuneScope::live),
				"The audited colour allowlist preserves its exact live/next-screen scope: " + path);
		}
		for (const char* path : { "movement_feedback.colour", "tuning.movement_feedback.colour",
			"presentation.movement_feedback.colour.r", "presentation.movement_feedback.colour_extra",
			"presentation.main_menu.backgrounds.title", "presentation.debug_overlay.font",
			"presentation.main_menu.team_select.sheet", "presentation.main_menu.team_select.frames.a",
			"presentation.main_menu.team_select.selected_frames.a", "presentation.main_menu.team_select.spacing.x",
			"presentation.main_menu.team_select.size.y", "presentation.main_menu.team_select.selected_extra",
			"presentation.main_menu.weapon_select.sheet", "presentation.main_menu.weapon_select.fill_frame",
			"presentation.main_menu.weapon_select.choices.sprayer.frame", "presentation.main_menu.weapon_select.spacing.x",
			"presentation.main_menu.weapon_select.size.y", "presentation.main_menu.weapon_select.selected.r",
			"presentation.main_menu.weapon_select.selected_colour", "presentation.main_menu.weapon_select.unselected_extra",
			"weapon_feedback.highlight", "tuning.weapon_feedback.highlight", "presentation.weapon_feedback.sprite_sheet",
			"presentation.weapon_feedback.fill_frame", "presentation.weapon_feedback.highlight.r",
			"presentation.weapon_feedback.keyline_extra", "presentation.weapon_feedback.highlight_colour",
			"presentation.weapon_feedback.muzzle_duration", "presentation.weapon_feedback.enabled",
			"player_identity.ink_colour", "tuning.player_identity.ink_colour", "player_identity.background_colour",
			"tuning.player_identity.background_colour", "presentation.player_identity.ink_colour.r",
			"presentation.player_identity.ink_colour_extra", "presentation.player_identity.background_colour.r",
			"presentation.player_identity.background_colour_extra", "combat_feedback.highlight",
			"tuning.combat_feedback.highlight", "presentation.combat_feedback.sprite_sheet",
			"presentation.combat_feedback.fill_frame", "presentation.combat_feedback.highlight.r",
			"presentation.combat_feedback.keyline_extra", "presentation.combat_feedback.highlight_colour",
			"presentation.combat_feedback.hurt_duration", "presentation.combat_feedback.enabled",
			"blocked_shot.highlight", "tuning.blocked_shot.highlight", "presentation.blocked_shot.sprite_sheet",
			"presentation.blocked_shot.fill_frame", "presentation.blocked_shot.highlight.r",
			"presentation.blocked_shot.keyline_extra", "presentation.blocked_shot.highlight_colour",
			"presentation.blocked_shot.duration", "presentation.blocked_shot.enabled",
			"idle_drip.keyline", "tuning.idle_drip.keyline", "presentation.idle_drip.sprite_sheet",
			"presentation.idle_drip.fill_frame", "presentation.idle_drip.keyline.r",
			"presentation.idle_drip.keyline_extra", "presentation.idle_drip.tools.sprayer.keyline",
			"presentation.idle_drip.tools.sniper.keyline", "presentation.idle_drip.tools.bucket.keyline",
			"presentation.idle_drip.grow_time", "presentation.idle_drip.enabled",
			"respawn_redraw.pen_colour", "tuning.respawn_redraw.pen_colour",
			"presentation.respawn_redraw.sprite_sheet", "presentation.respawn_redraw.pen_frame",
			"presentation.respawn_redraw.pen_colour.r", "presentation.respawn_redraw.pen_colour_extra",
			"presentation.respawn_redraw.strokes[0].part", "presentation.respawn_redraw.pen_width",
			"opening.cursor_art.ink", "tuning.opening.cursor_art.ink", "opening.cursor_art.fill",
			"tuning.opening.cursor_art.fill", "presentation.opening.sprite_sheet", "presentation.opening.fill_frame",
			"presentation.opening.cursor_art.ink.r", "presentation.opening.cursor_art.fill.r",
			"presentation.opening.cursor_art.ink_extra", "presentation.opening.cursor_art.fill_extra",
			"presentation.opening.cursor_art.ink_colour", "presentation.opening.cursor_art.shade.r",
			"presentation.opening.cursor_art.shade_extra", "presentation.opening.cursor_art.shadow.r",
			"opening.cursor_art.shade", "opening.cursor_art.shadow",
			"tuning.opening.cursor_art.shade", "tuning.opening.cursor_art.shadow",
			"presentation.opening.cursor_art.shadow_extra", "presentation.opening.cursor_art.click_duration",
			"opening.silhouette_art.ink", "tuning.opening.silhouette_art.ink", "presentation.opening.silhouette",
			"presentation.opening.silhouette_art.ink.r", "presentation.opening.silhouette_art.rim_extra",
			"presentation.opening.silhouette_art.shade_colour", "presentation.opening.silhouette_art.pen_width",
			"opening.reveal_art.ink", "tuning.opening.reveal_art.ink", "opening.reveal_art.paper",
			"tuning.opening.reveal_art.paper", "opening.reveal_art.shade", "tuning.opening.reveal_art.shade",
			"presentation.opening.percent_font", "presentation.opening.reveal_art.ink.r",
			"presentation.opening.reveal_art.paper_extra", "presentation.opening.reveal_art.shade_colour",
			"presentation.opening.reveal_art.padding", "presentation.opening.reveal_art.beat_duration",
			"opening.desktop_colour", "tuning.opening.desktop_colour", "presentation.opening.desktop_colour.r",
			"presentation.opening.desktop_colour_extra" })
			{ require(!tune_colour_path(path), std::string("No prefix, suffix or nearby arbitrary string enters the colour allowlist: ") + path); }
		for (const std::string& pattern : tune_scope_patterns())
		{
			require(std::any_of(paths.begin(), paths.end(), [&pattern](const std::string& path)
				{ return tune_pattern_matches(pattern, path); }),
				"The scope row '" + pattern + "' matches a path the files have");
		}
		const std::pair<const char*, TuneScope> samples[] = {
			{ "tuning.input.movement_deadzone", TuneScope::live },
			{ "tuning.player.gravity", TuneScope::live },
			{ "tuning.player.size.x", TuneScope::rebuild },
			{ "tuning.player.animations.running.frame_time", TuneScope::live },
			{ "tuning.match.duration", TuneScope::rebuild },
			{ "tuning.match.sounds.countdown.volume", TuneScope::live },
			{ "tuning.paint.thickness", TuneScope::rebuild },
			{ "tuning.weapons.sprayer.offset.x", TuneScope::live },
			{ "tuning.weapons.ammo_regen_rate", TuneScope::live },
			{ "tuning.weapons.roller.aim_tilt", TuneScope::live },
			{ "tuning.projectiles.spray.size.x", TuneScope::new_objects },
			{ "tuning.projectiles.mist.spread_step", TuneScope::new_objects },
			{ "tuning.projectiles.wind_stop_factor", TuneScope::live },
			{ "tuning.projectiles.spray.gravity", TuneScope::live },
			{ "presentation.movement_feedback.minimum_strength", TuneScope::live },
			{ "presentation.weapon_feedback.muzzle_duration", TuneScope::live },
			{ "presentation.weapon_feedback.enabled", TuneScope::live },
			{ "presentation.weapon_feedback.highlight", TuneScope::live },
			{ "presentation.weapon_feedback.keyline", TuneScope::live },
			{ "presentation.combat_feedback.highlight", TuneScope::live },
			{ "presentation.combat_feedback.keyline", TuneScope::live },
			{ "presentation.blocked_shot.highlight", TuneScope::live },
			{ "presentation.blocked_shot.keyline", TuneScope::live },
			{ "presentation.blocked_shot.duration", TuneScope::next_screen },
			{ "presentation.idle_drip.keyline", TuneScope::live },
			{ "presentation.idle_drip.grow_time", TuneScope::next_screen },
			{ "presentation.respawn_redraw.pen_colour", TuneScope::live },
			{ "presentation.opening.cursor_art.ink", TuneScope::live },
			{ "presentation.opening.cursor_art.fill", TuneScope::live },
			{ "presentation.opening.silhouette_art.ink", TuneScope::live },
			{ "presentation.opening.silhouette_art.rim", TuneScope::live },
			{ "presentation.opening.silhouette_art.shade", TuneScope::live },
			{ "presentation.opening.reveal_art.ink", TuneScope::live },
			{ "presentation.opening.reveal_art.paper", TuneScope::live },
			{ "presentation.opening.reveal_art.shade", TuneScope::live },
			{ "presentation.opening.desktop_colour", TuneScope::live },
			{ "presentation.respawn_redraw.strokes[3].points[1].x", TuneScope::live },
			{ "presentation.opening.glass.width", TuneScope::live },
			{ "tuning.match.opening.leave", TuneScope::live },
			{ "tuning.match.reveal.decide", TuneScope::live },
			{ "presentation.camera.reference_height", TuneScope::live },
			{ "presentation.match.countdown_center_y", TuneScope::rebuild },
			{ "presentation.main_menu.title_y", TuneScope::next_screen },
			{ "presentation.main_menu.team_select.spacing.x", TuneScope::next_screen },
			{ "presentation.main_menu.team_select.spacing.y", TuneScope::next_screen },
			{ "presentation.main_menu.team_select.size.x", TuneScope::next_screen },
			{ "presentation.main_menu.team_select.size.y", TuneScope::next_screen },
			{ "presentation.menu_style.layout.position.x", TuneScope::next_screen },
			{ "presentation.menu_style.layout.size.y", TuneScope::next_screen },
			{ "presentation.menu_style.layout.spacing.x", TuneScope::next_screen },
		};
		for (const auto& [path, scope] : samples)
		{
			require(std::find(paths.begin(), paths.end(), path) != paths.end(),
				std::string("The files have ") + path);
			require(tune_scope_of(path) == scope, std::string("Scope of ") + path + " is " +
				tune_scope_name(scope) + ", got " + tune_scope_name(tune_scope_of(path)));
		}
		require(tune_scope_of("tuning.never.seen") == TuneScope::rebuild, "Unmatched paths rebuild");

		require(tune_step(1800.0, false) == 100.0 && std::fabs(tune_step(0.15, false) - 0.01) < 1e-12 &&
			tune_step(0.0, false) == 0.1, "Decimal steps are one unit of the digit below the leading one");
		require(tune_step(4.0, true) == 1.0 && tune_step(0.0, true) == 1.0 && tune_step(12.0, true) == 1.0 &&
			tune_step(250.0, true) == 10.0, "Integer tokens step by whole numbers, at least 1");
		require(tune_format(250.0, "240.0") == "250.0" && tune_format(5.0, "4") == "5" &&
			tune_format(0.16, "0.15") == "0.16" && tune_format(4.5, "4") == "4.5" &&
			tune_format(-650.0, "-600.0") == "-650.0", "Formatting keeps the token's style");

		const std::vector<std::string> tree = { "a.b.c", "a.b.d", "a.e", "f[0].x", "f[1].x", "g[2]" };
		require(tune_children(tree, "") == std::vector<std::string>{ "a.", "f[", "g[" } &&
			tune_children(tree, "a.") == std::vector<std::string>{ "a.b.", "a.e" } &&
			tune_children(tree, "f[") == std::vector<std::string>{ "f[0].", "f[1]." } &&
			tune_children(tree, "g[") == std::vector<std::string>{ "g[2]" },
			"tune_children walks the tree one segment at a time");
	}

	// Every number and boolean is read by the game: replacing one with a
	// string must make the reader throw and name its key. Otherwise the file
	// has a key nothing reads and `tune` on it would do nothing.
	int check_tune_consumed(const std::filesystem::path& game_directory)
	{
		const std::filesystem::path directory = std::filesystem::temp_directory_path() /
			(L"colourwars_consumed_" + std::to_wstring(_getpid()));
		std::filesystem::create_directories(directory);
		int checked = 0;
		std::size_t colours_checked = 0;
		for (const char* name : { "tuning", "presentation" })
		{
			const std::string text = read_text(game_directory / (std::string(name) + ".json"));
			const JsonSpanDocument document(text, name);
			const std::filesystem::path candidate = directory / (std::string(name) + ".json");
			for (const JsonLeaf& leaf : document.leaves())
			{
				if (leaf.kind == JsonLeaf::Kind::string) { continue; }
				bool comment = false;
				for (std::size_t at = 0; at < leaf.path.size(); at = leaf.path.find('.', at) + 1)
				{
					comment = comment || leaf.path[at] == '_';
					if (leaf.path.find('.', at) == std::string::npos) { break; }
				}
				if (comment)
				{
					continue;
				}
				write_text(candidate, document.replaced(leaf, "\"__unread__\""));
				std::string message;
				try
				{
					if (std::string(name) == "tuning")
					{
						GameTuning loaded(candidate.string().c_str());
					}
					else
					{
						Presentation loaded(candidate.string().c_str());
					}
				}
				catch (const std::exception& error)
				{
					message = error.what();
				}
				require(!message.empty(), std::string(name) + "." + leaf.path +
					" is in the file but the game never reads it");
				require(message.find(last_key(leaf.path)) != std::string::npos,
					std::string(name) + "." + leaf.path + ": the reader's message does not name '" +
					last_key(leaf.path) + "': " + message);
				++checked;
			}
			if (std::string(name) == "presentation")
			{
				for (const char* prefixed : colour_paths)
				{
					const std::string path = std::string(prefixed).substr(std::string("presentation.").size());
					const JsonLeaf* leaf = document.find(path);
					require(leaf != nullptr && leaf->kind == JsonLeaf::Kind::string,
						"The audited colour is an authored string: " + path);
					write_text(candidate, document.replaced(*leaf, "false"));
					std::string message;
					try { Presentation loaded(candidate.string().c_str()); }
					catch (const std::exception& error) { message = error.what(); }
					require(message.find(last_key(path)) != std::string::npos,
						"The Presentation reader consumes the audited colour and names a rejected type: " + path);
					++colours_checked;
				}
			}
		}
		require(colours_checked == colour_paths.size(), "All thirty-two colour strings have real Presentation consumers");
		std::error_code ignored;
		std::filesystem::remove_all(directory, ignored);
		return checked;
	}
}
#endif
