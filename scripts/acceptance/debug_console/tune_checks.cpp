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

// Live tuning: JSON spans, tune scopes, the consumed-leaf check, tune commands
// and save, tune completion and the menu tree.
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

	std::size_t paged_row_index(DebugMenuModel& model, const std::string& label)
	{
		const std::vector<std::string> first_page = labels(model.rows());
		do
		{
			const std::vector<DebugMenuRow> rows = model.rows();
			const auto found = std::find_if(rows.begin(), rows.end(), [&label](const DebugMenuRow& row)
				{ return row.label == label; });
			if (found != rows.end())
			{
				return static_cast<std::size_t>(found - rows.begin());
			}
			const auto more = std::find_if(rows.begin(), rows.end(), [](const DebugMenuRow& row)
				{ return row.kind == DebugMenuRow::Kind::more; });
			if (more == rows.end()) { break; }
			model.confirm(static_cast<std::size_t>(more - rows.begin()));
		} while (labels(model.rows()) != first_page);
		throw std::runtime_error("The debug menu has no row '" + label + "' on any page");
	}

	const std::array<const char*, 13> colour_paths{
		"presentation.movement_feedback.colour", "presentation.movement_feedback.highlight",
		"presentation.movement_feedback.keyline", "presentation.debug_overlay.colour",
		"presentation.debug_overlay.shadow_colour", "presentation.debug_overlay.station_label.colour",
		"presentation.main_menu.backgrounds.home", "presentation.main_menu.team_select.selected",
		"presentation.main_menu.team_select.unselected", "presentation.main_menu.weapon_select.selected",
		"presentation.main_menu.weapon_select.unselected", "presentation.weapon_feedback.highlight",
		"presentation.weapon_feedback.keyline" };

	std::vector<labrador::Colour> colour_values(const Presentation& presentation)
	{
		return { presentation.movement_feedback().colour, presentation.movement_feedback().highlight,
			presentation.movement_feedback().keyline, presentation.debug_overlay().colour,
			presentation.debug_overlay().shadow_colour, presentation.debug_overlay().station_label.colour,
			presentation.main_menu().home_background_colour, presentation.main_menu().team_select.selected,
			presentation.main_menu().team_select.unselected, presentation.main_menu().weapon_select.selected,
			presentation.main_menu().weapon_select.unselected, presentation.weapon_feedback().highlight,
			presentation.weapon_feedback().keyline };
	}

	void check_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"colours");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const std::string source_before = read_text(scratch.source / L"presentation.json");
		const std::string staged_before = read_text(scratch.staged / L"presentation.json");
		const auto initial = colour_values(presentation);
		const MovementFeedbackPresentation* const movement = &presentation.movement_feedback();
		const DebugOverlayPresentation* const overlay = &presentation.debug_overlay();
		const StationLabelPresentation* const station = &overlay->station_label;
		const SelectPresentation* const team = &presentation.main_menu().team_select;
		const SelectPresentation* const weapon = &presentation.main_menu().weapon_select;
		const WeaponSelectArtworkPresentation* const weapon_art = &presentation.main_menu().weapon_select_art;
		const char* const weapon_sheet = weapon_art->sheet.data();
		const char* const weapon_fill = weapon_art->fill_frame.data();
		const char* const weapon_frame = weapon_art->choices.front().frame.data();
		const WeaponFeedbackPresentation* const feedback = &presentation.weapon_feedback();
		const char* const feedback_sheet = feedback->sprite_sheet.data();
		const char* const feedback_fill = feedback->fill_frame.data();
		const std::string team_sheet = presentation.main_menu().team_select_sheet;
		const auto team_frames = presentation.main_menu().team_select_frames;
		const auto team_selected_frames = presentation.main_menu().team_select_selected_frames;
		const char* const font = overlay->font.data();
		const char* const sheet = movement->sprite_sheet.data();
		std::array<std::string, 13> words{ "RED", "112233", "a1B2c3", "00000080",
			"FFFFFF", "12345678", "654321", "BLUE", "10203040", "RED", "56789ABC", "35BCE1", "2266CC80" };
		std::vector<labrador::Colour> expected{ { 255, 0, 0 }, { 17, 34, 51 },
			{ 161, 178, 195 }, { 0, 0, 0, 128 }, { 255, 255, 255 }, { 18, 52, 86, 120 }, { 101, 67, 33 },
			{ 0, 0, 255 }, { 16, 32, 48, 64 }, { 255, 0, 0 }, { 86, 120, 154, 188 },
			{ 53, 188, 225 }, { 34, 102, 204, 128 } };
		require(impl.execute("feedback off").ok, "Colour tuning starts with an independent live F9 override");
		for (std::size_t index = 0; index < colour_paths.size(); ++index)
		{
			const std::string path = colour_paths[index];
			if (impl.tune_token(path) == "\"" + words[index] + "\"")
			{
				words[index] = index == 0 ? "BLUE" : "FEDCBA98";
				expected[index] = index == 0 ? labrador::Colour(0, 0, 255) : labrador::Colour(254, 220, 186, 152);
			}
			const std::string token = "\"" + words[index] + "\"";
			require(impl.execute("tune " + path + " " + words[index]).ok &&
				impl.tune_token(path) == token && colour_values(presentation)[index] == expected[index],
				"An audited colour accepts a named/RGB/RGBA word as one JSON string: " + path);
			require(impl.execute("tune " + path).ok &&
				log_contains(impl.log(), path + " = " + token, DebugSeverity::info),
				"A colour getter prints the exact quoted token: " + path);
		}
		for (const std::size_t index : { std::size_t(7), std::size_t(8), std::size_t(9), std::size_t(10),
			std::size_t(11), std::size_t(12) })
		{
			for (const auto& [word, colour] : std::array<std::pair<const char*, labrador::Colour>, 3>{ {
				{ "RED", { 255, 0, 0 } }, { "a1B2c3", { 161, 178, 195 } }, { "10203040", { 16, 32, 48, 64 } } } })
			{
				const std::string path = colour_paths[index];
				const std::string token = std::string("\"") + word + '"';
				expected[index] = colour;
				require(impl.execute("tune " + path + " " + word).ok && impl.tune_token(path) == token &&
					colour_values(presentation) == expected,
					"Each selection/feedback colour accepts named, RGB and RGBA values without changing the other colours: " + path);
			}
		}
		require(colour_values(presentation) == expected && !presentation.movement_feedback().enabled &&
			&presentation.movement_feedback() == movement && &presentation.debug_overlay() == overlay &&
			&presentation.debug_overlay().station_label == station && overlay->font.data() == font &&
			movement->sprite_sheet.data() == sheet && &presentation.main_menu().team_select == team &&
			presentation.main_menu().team_select_sheet == team_sheet &&
			presentation.main_menu().team_select_frames == team_frames &&
			presentation.main_menu().team_select_selected_frames == team_selected_frames &&
			&presentation.main_menu().weapon_select == weapon && &presentation.main_menu().weapon_select_art == weapon_art &&
			weapon_art->sheet.data() == weapon_sheet && weapon_art->fill_frame.data() == weapon_fill &&
			weapon_art->choices.front().frame.data() == weapon_frame && &presentation.weapon_feedback() == feedback &&
			feedback->sprite_sheet.data() == feedback_sheet && feedback->fill_frame.data() == feedback_fill,
			"Colour adoption keeps the borrowed records, unrelated string buffers and live F9 override");
		require(read_text(scratch.source / L"presentation.json") == source_before &&
			read_text(scratch.staged / L"presentation.json") == staged_before,
			"Live colour edits do not write either content copy");
		require(impl.execute("tune.diff presentation").ok, "Colour edits are listed by tune.diff");
		for (std::size_t index = 0; index < colour_paths.size(); ++index)
		{
			const std::string path = colour_paths[index];
			require(log_contains(impl.log(), "  " + path + "  ", DebugSeverity::info) &&
				tune_scope_of(path) == (index == 6 ? TuneScope::next_screen : TuneScope::live),
				"Each colour diff has its audited live/next-screen scope: " + path);
		}
		const auto reject = [&](const std::string& path, const std::string& value)
		{
			const auto before = colour_values(presentation);
			const std::string text = impl.tune_document("presentation")->current().text();
			const auto edits = impl.tune_document("presentation")->edits().size();
			require(!impl.execute("tune " + path + " " + quote_word(value)).ok &&
				colour_values(presentation) == before &&
				impl.tune_document("presentation")->current().text() == text &&
				impl.tune_document("presentation")->edits().size() == edits,
				"Rejected colour word/path keeps all live values and pending JSON: " + path + "=" + value);
		};
		for (const char* path : { colour_paths.front(), colour_paths[7], colour_paths[8], colour_paths[9], colour_paths[10],
			colour_paths[11], colour_paths[12] })
		{
			for (const char* value : { "next", "prev", "toggle", "on", "off", "true", "false", "+=1", "-=1",
				"#112233", "12345", "1234567", "GGGGGG", "not_a_colour", "white", "black", "\"RED\"", "RED\\u0000", "RED\\n",
				"RED\",\"enabled\":true,\"injected\":\"x", "RED;feedback on" })
			{
				reject(path, value);
			}
		}
		for (const char* path : { "presentation.debug_overlay.font", "presentation.movement_feedback.sprite_sheet",
			"presentation.main_menu.backgrounds.title", "presentation.movement_feedback.colour.r",
			"tuning.player.sound_bank", "presentation.main_menu.team_select.sheet",
			"presentation.main_menu.team_select.frames.a", "presentation.main_menu.team_select.selected_frames.a",
			"presentation.main_menu.team_select.selected.r", "presentation.main_menu.team_select.unselected_extra",
			"presentation.main_menu.team_select.selected_colour", "presentation.main_menu.weapon_select.sheet",
			"presentation.main_menu.weapon_select.fill_frame", "presentation.main_menu.weapon_select.choices.sprayer.frame",
			"presentation.main_menu.weapon_select.choices.sprayer.name", "presentation.main_menu.weapon_select.selected.r",
			"presentation.main_menu.weapon_select.selected_colour", "presentation.main_menu.weapon_select.unselected_extra",
			"presentation.weapon_feedback.sprite_sheet", "presentation.weapon_feedback.fill_frame",
			"presentation.weapon_feedback.highlight.r", "presentation.weapon_feedback.highlight_colour",
			"presentation.weapon_feedback.keyline_extra" })
		{
			require(impl.tune_token(path).empty() && !impl.execute(std::string("tune ") + path).ok,
				std::string("An arbitrary string is neither readable nor completed as tunable: ") + path);
			reject(path, "WHITE");
		}
		require(!presentation.movement_feedback().enabled, "A quoted injection cannot run a second console statement");
		require(impl.execute(std::string("tune.reset ") + colour_paths.front()).ok &&
			colour_values(presentation).front() == initial.front() &&
			colour_values(presentation).back() == expected.back(), "A colour path reset leaves the other colour edits intact");
		require(impl.execute(std::string("tune.reset ") + colour_paths[7]).ok &&
			colour_values(presentation)[7] == initial[7] && colour_values(presentation)[8] == expected[8] &&
			impl.tune_document("presentation")->edits().count("main_menu.team_select.selected") == 0 &&
			impl.tune_document("presentation")->edits().count("main_menu.team_select.unselected") == 1,
			"Resetting the selected Team tint retains the unselected tint's independent edit");
		for (const std::size_t index : { std::size_t(9), std::size_t(10) })
		{
			const std::string path = colour_paths[index];
			const std::string local = path.substr(std::string("presentation.").size());
			auto restored = colour_values(presentation);
			restored[index] = initial[index];
			require(impl.tune_document("presentation")->edits().count(local) == 1 &&
				impl.execute("tune.reset " + path).ok && colour_values(presentation) == restored &&
				impl.tune_document("presentation")->edits().count(local) == 0 &&
				impl.tune_document("presentation")->edits().count("main_menu.team_select.unselected") == 1 &&
				(index != 9 || impl.tune_document("presentation")->edits().count("main_menu.weapon_select.unselected") == 1),
				"Each Weapon tint resets independently while every other pending colour remains intact: " + path);
		}
		for (const std::size_t index : { std::size_t(11), std::size_t(12) })
		{
			const std::string path = colour_paths[index];
			const std::string local = path.substr(std::string("presentation.").size());
			auto restored = colour_values(presentation);
			restored[index] = initial[index];
			require(impl.tune_document("presentation")->edits().count(local) == 1 &&
				impl.execute("tune.reset " + path).ok && colour_values(presentation) == restored &&
				impl.tune_document("presentation")->edits().count(local) == 0 &&
				impl.tune_document("presentation")->edits().count("main_menu.team_select.unselected") == 1 &&
				(index != 11 || impl.tune_document("presentation")->edits().count("weapon_feedback.keyline") == 1) &&
				&presentation.weapon_feedback() == feedback && feedback->sprite_sheet.data() == feedback_sheet &&
				feedback->fill_frame.data() == feedback_fill,
				"Each feedback colour resets independently while other edits and captured bindings remain intact: " + path);
		}
		require(impl.execute("tune.reset presentation").ok && colour_values(presentation) == initial &&
			impl.tune_document("presentation")->edits().empty(), "A presentation reset restores every colour token");

		// The literal save oracle replaces one exact span in otherwise untouched
		// LF content. A subsequent external edit to another field must survive.
		for (const std::size_t index : { std::size_t(5), std::size_t(7), std::size_t(8), std::size_t(9), std::size_t(10),
			std::size_t(11), std::size_t(12) })
		{
			const std::string path = colour_paths[index];
			const std::string local = path.substr(std::string("presentation.").size());
			const std::string save_before = read_text(scratch.source / L"presentation.json");
			const JsonSpanDocument original(save_before, "colour-save-reference");
			const JsonLeaf* original_leaf = original.find(local);
			require(original_leaf != nullptr, "The colour-save reference has its audited string token: " + path);
			const std::string original_token(original.token(*original_leaf));
			std::string saved = save_before;
			saved.replace(original_leaf->begin, original_leaf->end - original_leaf->begin, "\"12345678\"");
			require(impl.execute("tune " + path + " 12345678").ok && impl.execute("tune.save presentation").ok &&
				read_text(scratch.source / L"presentation.json") == saved &&
				read_text(scratch.staged / L"presentation.json") == saved && saved.find('\r') == std::string::npos &&
				impl.tune_document("presentation")->edits().empty(),
				"Colour save changes exactly one quoted LF token, refreshes staging and rebases the edits");
			require(colour_values(Presentation((scratch.source / L"presentation.json").string().c_str()))[index] ==
				labrador::Colour(18, 52, 86, 120), "The saved RGBA token loads through the real Presentation reader");
			require(impl.execute("tune " + path + " ABCDEF").ok, "A second colour edit is pending before a disk conflict");
			const JsonSpanDocument disk(saved, "colour-save-reference");
			std::string external = disk.replaced(*disk.find(local), "\"FEDCBA\"");
			const JsonSpanDocument external_document(external, "colour-save-reference");
			const char* unrelated_word = index == 5 ? "102030" : index == 7 ? "203040" :
				index == 8 ? "304050" : index == 9 ? "405060" : index == 10 ? "506070" :
				index == 11 ? "607080" : "708090";
			const labrador::Colour unrelated_colour(unrelated_word);
			require(presentation.debug_overlay().colour != unrelated_colour,
				"Each colour conflict must introduce a fresh unrelated external edit");
			external = external_document.replaced(*external_document.find("debug_overlay.colour"),
				std::string("\"") + unrelated_word + '"');
			write_text(scratch.source / L"presentation.json", external);
			require(!impl.execute("tune.save presentation").ok &&
				read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == saved &&
				impl.tune_token(path) == "\"ABCDEF\"",
				"A colour conflict leaves disk, staged baseline and our pending token intact");
			const JsonSpanDocument theirs(external, "colour-save-reference");
			const std::string forced = theirs.replaced(*theirs.find(local), "\"ABCDEF\"");
			require(impl.execute("tune.save presentation force").ok &&
				read_text(scratch.source / L"presentation.json") == forced &&
				read_text(scratch.staged / L"presentation.json") == forced &&
				presentation.debug_overlay().colour == unrelated_colour,
				"Forced colour save replaces only our conflict and adopts the unrelated external edit");
			require(!original_token.empty() && !std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
				!std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
				"Colour save leaves no temporary files beside either content copy");
		}
		for (const std::size_t index : { std::size_t(9), std::size_t(10), std::size_t(11), std::size_t(12) })
		{
			// Each new path also starts from an independent CRLF document, so
			// external adoption cannot pass by reusing an earlier case's value.
			const wchar_t* const scratch_name = index == 9 ? L"weapon-selected-crlf" :
				index == 10 ? L"weapon-unselected-crlf" : index == 11 ? L"feedback-highlight-crlf" : L"feedback-keyline-crlf";
			ScratchContent crlf_scratch(game_directory, scratch_name);
			std::string crlf;
			for (const char character : read_text(crlf_scratch.source / L"presentation.json"))
			{
				crlf += character == '\n' ? std::string("\r\n") : std::string(1, character);
			}
			for (const auto& directory : { crlf_scratch.source, crlf_scratch.staged })
				{ write_text(directory / L"presentation.json", crlf); }
			GameTuning crlf_tuning((crlf_scratch.staged / L"tuning.json").string().c_str());
			Presentation crlf_presentation((crlf_scratch.staged / L"presentation.json").string().c_str());
			TestDirector crlf_test(crlf_tuning, crlf_presentation);
			DebugDirectorImpl& crlf_impl = crlf_test.impl;
			crlf_impl.set_content_directories(crlf_scratch.source, crlf_scratch.staged);
			const std::string path = colour_paths[index];
			const std::string local = path.substr(std::string("presentation.").size());
			const auto original_values = colour_values(crlf_presentation);
			const JsonSpanDocument original(crlf, "selection-feedback-crlf-reference");
			const JsonLeaf* leaf = original.find(local);
			require(leaf != nullptr && original.token(*leaf) != "\"89ABCDEF\"" && count_of(crlf, "\r\n") > 0,
				"Each selection/feedback CRLF save begins with a different authored token: " + path);
			const std::string saved = original.replaced(*leaf, "\"89ABCDEF\"");
			require(crlf_impl.execute("tune " + path + " 89ABCDEF").ok && crlf_impl.execute("tune.save presentation").ok &&
				read_text(crlf_scratch.source / L"presentation.json") == saved &&
				read_text(crlf_scratch.staged / L"presentation.json") == saved &&
				crlf_impl.tune_document("presentation")->edits().empty() &&
				colour_values(Presentation((crlf_scratch.source / L"presentation.json").string().c_str()))[index] ==
					labrador::Colour(137, 171, 205, 239),
				"Selection/feedback save changes exactly one CRLF string token, loads RGBA and rebases staging: " + path);
			require(crlf_impl.execute("tune " + path + " ABCDEF").ok,
				"A selection/feedback CRLF edit is pending before its independent conflict: " + path);
			const JsonSpanDocument saved_document(saved, "weapon-crlf-reference");
			const std::string theirs = saved_document.replaced(*saved_document.find(local), "\"FEDCBA\"");
			const JsonSpanDocument theirs_document(theirs, "weapon-crlf-reference");
			const char* unrelated_word = index == 9 ? "607080" : "708090";
			const labrador::Colour unrelated_colour(unrelated_word);
			require(crlf_presentation.debug_overlay().colour != unrelated_colour &&
				theirs_document.token(*theirs_document.find("debug_overlay.colour")) != std::string("\"") + unrelated_word + '"',
				"Each selection/feedback CRLF conflict introduces a fresh live and disk unrelated colour: " + path);
			const std::string external = theirs_document.replaced(*theirs_document.find("debug_overlay.colour"),
				std::string("\"") + unrelated_word + '"');
			write_text(crlf_scratch.source / L"presentation.json", external);
			require(!crlf_impl.execute("tune.save presentation").ok &&
				read_text(crlf_scratch.source / L"presentation.json") == external &&
				read_text(crlf_scratch.staged / L"presentation.json") == saved && crlf_impl.tune_token(path) == "\"ABCDEF\"",
				"A selection/feedback CRLF conflict preserves disk, staging and the pending token: " + path);
			const JsonSpanDocument external_document(external, "weapon-crlf-reference");
			const std::string forced = external_document.replaced(*external_document.find(local), "\"ABCDEF\"");
			auto adopted = original_values;
			adopted[index] = labrador::Colour(171, 205, 239);
			adopted[3] = unrelated_colour;
			require(crlf_impl.execute("tune.save presentation force").ok &&
				read_text(crlf_scratch.source / L"presentation.json") == forced &&
				read_text(crlf_scratch.staged / L"presentation.json") == forced && colour_values(crlf_presentation) == adopted &&
				crlf_impl.tune_document("presentation")->current().text() == forced &&
				crlf_impl.tune_document("presentation")->edits().empty() &&
				count_of(forced, "\r\n") == count_of(crlf, "\r\n") && count_of(forced, "\n") == count_of(crlf, "\r\n"),
				"Force saves only the selection/feedback conflict, adopts the fresh external colour and preserves every CRLF: " + path);
			require(crlf_impl.execute("tune " + path + " RED").ok && crlf_impl.execute("tune.reset " + path).ok &&
				crlf_impl.tune_document("presentation")->current().text() == forced &&
				colour_values(crlf_presentation) == adopted && crlf_impl.tune_document("presentation")->edits().empty() &&
				!std::filesystem::exists(crlf_scratch.source / L"presentation.json.tmp") &&
				!std::filesystem::exists(crlf_scratch.staged / L"presentation.json.tmp"),
				"A selection/feedback CRLF reset uses the saved baseline and leaves no temporary files: " + path);
		}
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
		const std::string feedback_baseline = "{\r\n  \"weapon_feedback\":{\"highlight\":\"\\u0057HITE\","
			"\"keyline\":\"\\u0042LACK\",\"sprite_sheet\":\"unchanged\",\"fill_frame\":\"solid\"},\r\n"
			"  \"note\":\"keep\\\\bytes\",\r\n  \"number\":1\r\n}\r\n";
		for (const char* field : { "highlight", "keyline" })
		{
			const std::string path = std::string("weapon_feedback.") + field;
			TuneDocument feedback_document("presentation", feedback_baseline);
			const JsonLeaf* leaf = feedback_document.find(path);
			require(leaf != nullptr && leaf->kind == JsonLeaf::Kind::string &&
				feedback_document.find("weapon_feedback.sprite_sheet") == nullptr &&
				feedback_document.find("weapon_feedback.fill_frame") == nullptr,
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
				std::string("{\"weapon_feedback\":{\"") + field + ".r\":\"RED\"}}" })
			{
				const TuneDocument literal("presentation", alias);
				const TuneSplice refused = feedback_document.splice(alias, true);
				require(literal.prefixed_paths().empty() && literal.find(path) == nullptr &&
					refused.written == 0 && refused.conflicts.size() == 1 && refused.text == alias,
					"A literal alias cannot expose or receive a feedback colour, even with force: " + path);
			}
		}
	}

	void check_motion_tunes(DebugDirectorImpl& impl, GameTuning& tuning, const ScratchContent& scratch)
	{
		const std::string spread_path = "tuning.projectiles.mist.spread_step";
		const std::string tilt_path = "tuning.weapons.roller.aim_tilt";
		const std::string wind_path = "tuning.projectiles.wind_stop_factor";
		const std::string spread_base = impl.tune_token(spread_path);
		const std::string tilt_base = impl.tune_token(tilt_path);
		const std::string wind_base = impl.tune_token(wind_path);
		require(!spread_base.empty() && !tilt_base.empty() && !wind_base.empty(),
			"Motion tuning paths are present in the actual scratch content");
		const ProjectileTuning* const mist = &tuning.projectile(ProjectileType::mist);
		const WeaponTuning* const roller = &tuning.weapon(WeaponType::roller);
		const ProjectileType kinds[] = { ProjectileType::spray, ProjectileType::jet,
			ProjectileType::rolling, ProjectileType::mist, ProjectileType::ball };
		const auto state = [&]
		{
			std::vector<float> values{ mist->spread_step, roller->aim_tilt };
			for (const ProjectileType kind : kinds)
			{
				const auto& projectile = tuning.projectile(kind);
				values.push_back(projectile.wind_stop_factor);
				values.push_back(projectile.details.wind_resistance);
			}
			return values;
		};
		const auto edits = [&]
		{
			std::vector<std::string> values;
			for (const auto& [path, edit] : impl.tune_document("tuning")->edits())
				{ values.push_back(path + "=" + edit.baseline_token + "->" + edit.current_token); }
			return values;
		};
		const auto reject = [&](const std::string& path, const std::string& value)
		{
			const auto before = state();
			const auto before_edits = edits();
			const std::string before_text = impl.tune_document("tuning")->current().text();
			require(!impl.execute("tune " + path + " " + value).ok && state() == before &&
				edits() == before_edits && impl.tune_document("tuning")->current().text() == before_text,
				"Rejected " + path + "=" + value + " preserves every motion record, token and pending edit");
		};
		const auto set = [&](const std::string& path, const std::string& value)
			{ require(impl.execute("tune " + path + " " + value).ok, "Valid motion tune " + path + "=" + value); };
		const std::string source_before = read_text(scratch.source / L"tuning.json");
		const std::string staged_before = read_text(scratch.staged / L"tuning.json");
		const auto initial = state();
		const std::string spread_limit = tune_format(mattmath::PI / 4.0f, spread_base);
		const std::string tilt_limit = tune_format(mattmath::PI / 2.0f, tilt_base);
		set(spread_path, "0");
		require(mist->spread_step == 0.0f, "Mist spread accepts its inclusive zero boundary");
		set(spread_path, spread_limit);
		require(mist->spread_step == mattmath::PI / 4.0f, "Mist spread accepts its inclusive PI/4 boundary");
		set(tilt_path, "0");
		require(roller->aim_tilt == 0.0f, "Roller tilt accepts its inclusive zero boundary");
		const float below_vertical = std::nextafter(mattmath::PI / 2.0f, 0.0f);
		set(tilt_path, tune_format(below_vertical, tilt_base));
		require(roller->aim_tilt == below_vertical, "Roller tilt accepts the float immediately below PI/2");
		reject(spread_path, tune_format(std::nextafter(mattmath::PI / 4.0f, mattmath::PI), spread_base));
		reject(tilt_path, tilt_limit);
		for (const std::string& path : { spread_path, tilt_path, wind_path })
		{
			for (const char* invalid : { "-0.01", "1e100", "nan", "inf" }) { reject(path, invalid); }
		}
		const float spread_live = std::stof(spread_base) == 0.4f ? 0.3f : 0.4f;
		const float tilt_live = std::stof(tilt_base) == 0.2f ? 0.3f : 0.2f;
		set(spread_path, tune_format(spread_live, spread_base));
		set(tilt_path, tune_format(tilt_live, tilt_base));
		set(wind_path, "2");
		require(mist->spread_step == spread_live && roller->aim_tilt == tilt_live &&
			&tuning.projectile(ProjectileType::mist) == mist && &tuning.weapon(WeaponType::roller) == roller,
			"Accepted motion tunes update the borrowed records without moving them");
		for (const ProjectileType kind : kinds)
			{ require(tuning.projectile(kind).wind_stop_factor == 2.0f, "Shared wind factor reaches every projectile record"); }
		require(impl.execute("tune " + spread_path).ok && impl.execute("tune " + tilt_path).ok &&
			impl.execute("tune " + wind_path).ok &&
			log_contains(impl.log(), "tune: " + spread_path + " ", DebugSeverity::info) &&
			log_contains(impl.log(), "(applies to new shots)", DebugSeverity::info) &&
			log_contains(impl.log(), spread_path + " = " + impl.tune_token(spread_path) +
				"  (file: " + spread_base + ")  [new shots]", DebugSeverity::info) &&
			log_contains(impl.log(), tilt_path + " = " + impl.tune_token(tilt_path) +
				"  (file: " + tilt_base + ")  [live]", DebugSeverity::info) &&
			log_contains(impl.log(), wind_path + " = " + impl.tune_token(wind_path) +
				"  (file: " + wind_base + ")  [live]", DebugSeverity::info),
			"Motion tune getters report new-shot spread and live tilt/wind scopes");
		reject("tuning.projectiles.spray.wind_resistance", "-0.01");
		reject("tuning.projectiles.spray.wind_resistance", "3e38");
		set(wind_path, "0");
		set("tuning.projectiles.spray.wind_resistance", "3e38");
		reject(wind_path, "2");
		require(impl.execute("tune.reset tuning.projectiles.spray.wind_resistance").ok,
			"A path reset restores resistance after a rejected wind product");
		require(impl.execute("tune.reset " + spread_path).ok &&
			mist->spread_step == std::stof(spread_base) && roller->aim_tilt == tilt_live &&
			mist->wind_stop_factor == 0.0f && impl.tune_token(spread_path) == spread_base,
			"Resetting spread restores its file token without undoing live tilt/wind edits");
		require(impl.execute("tune.reset tuning").ok && state() == initial &&
			impl.tune_document("tuning")->edits().empty() &&
			impl.tune_token(spread_path) == spread_base && impl.tune_token(tilt_path) == tilt_base &&
			impl.tune_token(wind_path) == wind_base &&
			read_text(scratch.source / L"tuning.json") == source_before &&
			read_text(scratch.staged / L"tuning.json") == staged_before,
			"File reset restores all motion values and leaves source/staged JSON untouched");
	}

	void check_menu_layout_tunes(DebugDirectorImpl& impl, Presentation& presentation,
		const ScratchContent& scratch)
	{
		const std::string root = "presentation.menu_style.layout.";
		const std::vector<std::string> paths{ root + "position.x", root + "position.y",
			root + "size.x", root + "size.y", root + "spacing.x", root + "spacing.y" };
		std::vector<std::string> baseline;
		for (const std::string& path : paths)
		{
			baseline.push_back(impl.tune_token(path));
			require(!baseline.back().empty(), "The shared menu layout has a tunable path " + path);
		}
		const MenuStyle* const style = &presentation.menu();
		const WidgetLayout* const layout = &style->layout;
		const char* const sheet = style->sprite_sheet.data();
		const char* const fill = style->fill_frame.data();
		const auto state = [&]
		{
			return std::vector<float>{ layout->position.x, layout->position.y, layout->size.x,
				layout->size.y, layout->spacing.x, layout->spacing.y };
		};
		const auto edits = [&]
		{
			std::vector<std::string> values;
			for (const auto& [path, edit] : impl.tune_document("presentation")->edits())
				{ values.push_back(path + "=" + edit.baseline_token + "->" + edit.current_token); }
			return values;
		};
		const auto reject = [&](const std::string& path, const std::string& value)
		{
			const auto before = state();
			const auto before_edits = edits();
			const std::string before_text = impl.tune_document("presentation")->current().text();
			require(!impl.execute("tune " + path + " " + value).ok && state() == before &&
				edits() == before_edits &&
				impl.tune_document("presentation")->current().text() == before_text,
				"Rejected " + path + "=" + value + " preserves layout, tokens and pending edits");
		};
		const auto set = [&](std::size_t index, const char* value)
			{ require(impl.execute("tune " + paths[index] + " " + value).ok,
				"Valid shared menu layout tune " + paths[index] + "=" + value); };
		const auto initial = state();
		const std::string source_before = read_text(scratch.source / L"presentation.json");
		const std::string staged_before = read_text(scratch.staged / L"presentation.json");
		set(0, "0");
		set(1, "0");
		set(2, "1920");
		set(3, "1080");
		set(4, "0");
		set(5, "0");
		require(state() == std::vector<float>{ 0.0f, 0.0f, 1920.0f, 1080.0f, 0.0f, 0.0f },
			"A shared menu widget may reach the design edges, and spacing may be zero");
		set(4, "1920");
		set(5, "1080");
		reject(paths[0], "1");
		reject(paths[1], "1");
		reject(paths[2], "1921");
		reject(paths[3], "1081");
		reject(paths[4], "1921");
		reject(paths[5], "1081");
		reject(paths[2], "0");
		reject(paths[3], "0");
		for (const std::string& path : paths)
		{
			for (const char* invalid : { "-0.01", "1e100", "nan", "inf" }) { reject(path, invalid); }
		}
		set(2, "200");
		set(3, "100");
		set(0, "25");
		set(1, "30");
		set(4, "230");
		set(5, "130");
		require(state() == std::vector<float>{ 25.0f, 30.0f, 200.0f, 100.0f, 230.0f, 130.0f } &&
			&presentation.menu() == style && &presentation.menu().layout == layout &&
			style->sprite_sheet.data() == sheet && style->fill_frame.data() == fill,
			"Layout tunes retain the shared record, layout address and named-resource buffers");
		require(impl.execute("tune " + paths[0]).ok &&
			log_contains(impl.log(), "(applies when the screen is next opened)", DebugSeverity::info) &&
			log_contains(impl.log(), paths[0] + " = " + impl.tune_token(paths[0]) +
				"  (file: " + baseline[0] + ")  [next screen]", DebugSeverity::info),
			"Shared menu layout tune setters and getters report next-screen scope");
		for (const char* path : { "presentation.menu_style.sprite_sheet", "presentation.menu_style.fill_frame",
			"presentation.main_menu.team_select.sheet", "presentation.main_menu.team_select.frames.a",
			"presentation.main_menu.team_select.selected_frames.a" })
			{ reject(path, "sprite_sheet_1"); }
		require(impl.execute("tune.reset " + paths[0]).ok && layout->position.x == initial[0] &&
			layout->position.y == 30.0f && impl.tune_token(paths[0]) == baseline[0],
			"A layout path reset restores its baseline without undoing other layout edits");
		require(impl.execute("tune.reset presentation").ok && state() == initial &&
			impl.tune_document("presentation")->edits().empty() &&
			read_text(scratch.source / L"presentation.json") == source_before &&
			read_text(scratch.staged / L"presentation.json") == staged_before,
			"Presentation reset restores the shared menu layout without writing source or staged JSON");
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
					tune_colour_path(path), "Only the independently audited thirteen colours appear as string tune paths: " + path);
			}
		}
		require(colour_count == colour_paths.size(), "Exactly thirteen authored colour strings are tunable");
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
			"presentation.weapon_feedback.muzzle_duration", "presentation.weapon_feedback.enabled" })
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
		require(colours_checked == colour_paths.size(), "All thirteen colour strings have real Presentation consumers");
		std::error_code ignored;
		std::filesystem::remove_all(directory, ignored);
		return checked;
	}

	void check_tune_commands(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"tune");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const DebugLog& log = impl.log();

		// The D2 guard: string buffers the game points at stay put.
		const char* const bank = tuning.player().sound_bank.data();
		const char* const meter_font = presentation.hud().meter_label_font.data();
		const WeaponTuning* const sprayer = &tuning.weapon(WeaponType::sprayer);

		// Every value below is relative to the file's, so a balance pass on
		// tuning.json never fails this check.
		const std::string gravity = impl.tune_token("tuning.player.gravity");
		require(!gravity.empty(), "The scratch file has a player gravity");
		const double base = std::stod(gravity);
		const auto token = [&gravity](double value) { return tune_format(value, gravity); };
		const std::string up = token(base + 100.0);
		require(impl.execute("tune tuning.player.gravity " + up).ok &&
			tuning.player().gravity == static_cast<float>(base + 100.0), "tune sets a live number");
		require(log_contains(log, "tune: tuning.player.gravity " + gravity + " -> " + up + " (live)",
			DebugSeverity::info), "tune reports old, new and the scope");
		require(impl.execute("tune tuning.player.gravity").ok &&
			log_contains(log, "tuning.player.gravity = " + up + "  (file: " + gravity + ")  [live]",
				DebugSeverity::info), "tune without a value prints the value, the file's and the scope");

		require(!impl.execute("tune tuning.player.jump_buffer_time 2").ok &&
			log_contains(log, "outside [0, 1]", DebugSeverity::error) &&
			tuning.player().jump_buffer_time != 2.0f, "The reader's own range rule rejects a value");
		require(!log_contains(log, "ColourWars-tune-", DebugSeverity::error),
			"A rejection names the content file, not the temporary candidate");
		require(!impl.execute("tune tuning.player.respawn_delay -1").ok &&
			log_contains(log, "'respawn_delay' must be finite and not negative", DebugSeverity::error) &&
			tuning.player().respawn_delay >= 0.0f, "A shared content-value bound rejects a value");
		require(!impl.execute("tune presentation.menu_style.sounds.confirm.volume 1.5").ok &&
			log_contains(log, "'volume' is 1.5, which is outside [0, 1]", DebugSeverity::error) &&
			presentation.menu().confirm_sound.volume <= 1.0f, "A menu sound's volume is bounded like the others");
		require(!impl.execute("tune presentation.movement_feedback.minimum_strength 1.5").ok &&
			log_contains(log, "strength must be at most one", DebugSeverity::error),
			"A cross-field rule rejects a value");
		require(!impl.execute("tune tuning.player.sound_bank x").ok &&
			!impl.execute("tune tuning.nowhere 1").ok && !impl.execute("tune player.gravity 1").ok,
			"Strings, unknown paths and unprefixed paths are refused");

		const double stepped = base + 100.0 + tune_step(base + 100.0, tune_integer_token(up));
		require(impl.execute("tune tuning.player.gravity next").ok &&
			tuning.player().gravity == static_cast<float>(stepped) &&
			impl.tune_token("tuning.player.gravity") == token(stepped), "next steps by tune_step");
		require(impl.execute("tune tuning.player.gravity -=150").ok &&
			tuning.player().gravity == static_cast<float>(stepped - 150.0), "-= subtracts");

		// Booleans, and the F9 switch beside them.
		require(impl.execute("tune presentation.movement_feedback.enabled off").ok &&
			!presentation.movement_feedback().enabled, "A boolean tunes with on/off words");
		require(impl.execute("feedback on").ok && presentation.movement_feedback().enabled,
			"F9 still toggles the live switch");
		require(impl.execute("tune presentation.movement_feedback.full_impact_speed next").ok &&
			presentation.movement_feedback().enabled,
			"Tuning another presentation value keeps the live F9 state");
		impl.execute("tune.diff");
		require(log_contains(log, "live toggle (F9) is on, not a file edit", DebugSeverity::info),
			"tune.diff says when F9 disagrees with the file");
		require(log_contains(log, "  tuning.player.gravity  " + gravity + " -> " + token(stepped - 150.0) +
			"  [live]", DebugSeverity::info) &&
			log_contains(log, "  presentation.movement_feedback.enabled  true -> false  [live]",
				DebugSeverity::info) && log_contains(log, "tune.diff: 4 line(s)", DebugSeverity::info),
			"tune.diff lists exactly the edits");
		require(tuning.player().sound_bank.data() == bank &&
			presentation.hud().meter_label_font.data() == meter_font &&
			&tuning.weapon(WeaponType::sprayer) == sprayer,
			"Copy-assigning a candidate keeps every string buffer and record address");

		require(impl.execute("tune.reset presentation").ok && presentation.movement_feedback().enabled &&
			impl.tune_document("presentation")->edits().empty(),
			"tune.reset of a file reverts its edits, the toggle edit included");
		require(impl.execute("tune.reset tuning.player.gravity").ok &&
			tuning.player().gravity == static_cast<float>(base) &&
			impl.tune_document("tuning")->edits().empty(), "tune.reset of a path reverts it");

		// A rebuild-scoped value with restart, in a match.
		FakeMatch match(test.director);
		match.settings = planned(1);
		test.director.attach_match(match);
		const float thickness = tuning.paint().thickness;
		const std::string thicker = tune_format(static_cast<double>(thickness) + 1.0,
			impl.tune_token("tuning.paint.thickness"));
		const DebugStatus restart = impl.execute("tune tuning.paint.thickness " + thicker + " restart");
		require(restart.ok && restart.close_console && impl.request().restart &&
			tuning.paint().thickness == thickness + 1.0f, "restart after a rebuild-scoped tune queues a rebuild");
		require(test.director.before_match_update(match, 1.0f / 60.0f) && match.rebuilds == 1,
			"and the match rebuilds");
		test.director.detach_match(match);
		require(impl.execute("tune.reset").ok && tuning.paint().thickness == thickness, "tune.reset all");

		// Keep these edits after the existing exact tune.diff count and restore
		// them before the save/conflict fixtures below.
		check_motion_tunes(impl, tuning, scratch);
		check_menu_layout_tunes(impl, presentation, scratch);
		check_colour_tunes(game_directory);

		// tune.save, CRLF tuning: exactly that token changes.
		const std::string tuning_before = read_text(scratch.source / L"tuning.json");
		require(impl.execute("tune tuning.player.gravity " + up).ok, "tune before save");
		require(impl.execute("tune.save tuning").ok, "tune.save writes the source file");
		// The player's gravity is the file's first "gravity" key.
		std::string expected = tuning_before;
		const std::string old_key = "\"gravity\": " + gravity;
		const std::size_t at = expected.find(old_key);
		require(at != std::string::npos && at == expected.find("\"gravity\": "),
			"The source's first gravity is the player's " + gravity);
		expected.replace(at, old_key.size(), "\"gravity\": " + up);
		const std::string tuning_after = read_text(scratch.source / L"tuning.json");
		require(tuning_after == expected, "tune.save changes only the edited token (CRLF file)");
		require(read_text(scratch.staged / L"tuning.json") == tuning_after,
			"tune.save refreshes the staged copy");
		require(!std::filesystem::exists(scratch.source / L"tuning.json.tmp") &&
			!std::filesystem::exists(scratch.staged / L"tuning.json.tmp"),
			"tune.save leaves no temporary beside either copy");
		require(GameTuning((scratch.source / L"tuning.json").string().c_str()).player().gravity ==
			static_cast<float>(base + 100.0), "The saved file loads with the new value");
		require(impl.tune_document("tuning")->edits().empty() &&
			impl.tune_document("tuning")->baseline().text() == tuning_after,
			"After a save the baseline is the file and tune.diff is empty");

		// LF presentation.
		const std::string presentation_before = read_text(scratch.source / L"presentation.json");
		const std::string strength_was = impl.tune_token("presentation.movement_feedback.minimum_strength");
		require(impl.execute("tune presentation.movement_feedback.minimum_strength prev").ok &&
			impl.execute("tune.save").ok, "tune.save all");
		const std::string strength_now = impl.tune_token("presentation.movement_feedback.minimum_strength");
		expected = presentation_before;
		const std::size_t strength = expected.find("\"minimum_strength\": " + strength_was);
		require(strength != std::string::npos && strength_now != strength_was,
			"The source has minimum_strength " + strength_was);
		expected.replace(strength, std::string("\"minimum_strength\": " + strength_was).size(),
			"\"minimum_strength\": " + strength_now);
		require(read_text(scratch.source / L"presentation.json") == expected &&
			read_text(scratch.source / L"presentation.json").find('\r') == std::string::npos,
			"tune.save changes only the edited token (LF file)");

		// A token changed on disk since it was loaded is a conflict.
		const std::string ours = token(base + 200.0);
		const std::string theirs = token(base + 50.0);
		require(impl.execute("tune tuning.player.gravity " + ours).ok, "tune before a conflict");
		std::string external = tuning_after;
		external.replace(external.find("\"gravity\": " + up), std::string("\"gravity\": " + up).size(),
			"\"gravity\": " + theirs);
		write_text(scratch.source / L"tuning.json", external);
		require(!impl.execute("tune.save tuning").ok &&
			log_contains(log, "conflict: tuning.player.gravity: changed on disk to " + theirs,
				DebugSeverity::error) && read_text(scratch.source / L"tuning.json") == external,
			"A conflict names the path and writes nothing");
		require(impl.execute("tune.save tuning force").ok &&
			read_text(scratch.source / L"tuning.json").find("\"gravity\": " + ours) != std::string::npos,
			"force overwrites the value changed on disk");
		require(impl.execute("tune.save").ok && log_contains(log, "tune.save: no edits to save",
			DebugSeverity::info), "Nothing to save says so");

		// No source tree: tune.save says what it needs.
		impl.set_content_directories(scratch.root / L"missing", scratch.staged);
		require(impl.execute("tune tuning.player.gravity " + token(base + 300.0)).ok &&
			!impl.execute("tune.save").ok &&
			log_contains(log, "tune.save needs a Debug build made from a checkout", DebugSeverity::error),
			"Without a source tree tune.save fails and says why");
		impl.execute("tune.reset");
		const std::wstring candidate = L"ColourWars-tune-" + std::to_wstring(_getpid());
		require(!std::filesystem::exists(std::filesystem::temp_directory_path() / candidate) &&
			!std::filesystem::exists(std::filesystem::temp_directory_path() / (candidate + L"-tuning.json")) &&
			!std::filesystem::exists(std::filesystem::temp_directory_path() / (candidate + L"-presentation.json")),
			"Validation leaves no candidate file or directory in %TEMP%");
	}

	void check_tune_completion_and_menu(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"menu");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const DebugRegistry& registry = impl.registry();

		require(registry.complete("tune tuning.pl").line == "tune tuning.player.",
			"A path completes one segment, with no space after it");
		require(registry.complete("tune tuning.player.jump_b").line ==
			"tune tuning.player.jump_buffer_time ", "A leaf completes with a space");
		const std::vector<std::string> groups = registry.complete("tune presentation.").candidates;
		require(std::find(groups.begin(), groups.end(), "presentation.hud.") != groups.end() &&
			std::find(groups.begin(), groups.end(), "presentation.camera.") != groups.end(),
			"presentation. lists its groups");
		require(registry.complete("tune tuning.player.gravity ").candidates ==
			std::vector<std::string>{ "next", "prev" } &&
			registry.complete("tune presentation.movement_feedback.enabled ").candidates ==
			std::vector<std::string>{ "off", "on", "toggle" }, "The value completes by kind");
		require(registry.complete("tune presentation.movement_feedback.co").line ==
			"tune presentation.movement_feedback.colour " &&
			registry.complete("tune presentation.debug_overlay.font ").candidates.empty(),
			"Colour leaves complete as paths while arbitrary font strings stay outside the tree");
		const auto colour_choices = registry.complete("tune presentation.movement_feedback.colour ").candidates;
		for (const char* path : { colour_paths[7], colour_paths[8], colour_paths[9], colour_paths[10],
			colour_paths[11] })
		{
			require(registry.complete(std::string("tune ") + path).line == std::string("tune ") + path + " " &&
				registry.complete(std::string("tune ") + path + " ").candidates == colour_choices,
				std::string("The selection/feedback colour completes as an exact leaf with literal colour choices: ") + path);
		}
		// The colour leaf shares its prefix with the existing numeric width.
		// Keep both path choices until a space explicitly selects the colour.
		const std::string feedback_keyline_prefix = colour_paths[12];
		const DebugCompletion feedback_keyline_matches = registry.complete("tune " + feedback_keyline_prefix);
		require(feedback_keyline_matches.line == "tune " + feedback_keyline_prefix &&
			feedback_keyline_matches.candidates == std::vector<std::string>{
				feedback_keyline_prefix, feedback_keyline_prefix + "_width" } &&
			registry.complete("tune " + feedback_keyline_prefix + " ").candidates == colour_choices,
			"Feedback keyline completion preserves the numeric sibling and explicit colour choices");
		require(colour_choices.size() >= 2 &&
			std::find(colour_choices.begin(), colour_choices.end(), "next") == colour_choices.end() &&
			std::find(colour_choices.begin(), colour_choices.end(), "prev") == colour_choices.end() &&
			std::find(colour_choices.begin(), colour_choices.end(), "toggle") == colour_choices.end(),
			"Colour value completion offers literal examples instead of numeric or boolean operations");
		for (const std::string& choice : colour_choices)
		{
			require(impl.execute("tune presentation.movement_feedback.colour " + choice).ok,
				"A completed colour example is accepted by the actual command: " + choice);
		}
		impl.execute("tune.reset");

		DebugMenuModel model(impl);
		const std::vector<std::string> roots = labels(model.rows());
		require(std::find(roots.begin(), roots.end(), "tuning") != roots.end() &&
			std::find(roots.begin(), roots.end(), "replay") != roots.end(),
			"The root lists the tuning and replay categories");
		model.confirm(row_index(model.rows(), "tuning"));
		require(labels(model.rows()) == std::vector<std::string>{ "tuning", "presentation", "tune.diff",
			"tune.reset", "tune.save" }, "Tuning opens on the two files and the save rows");
		model.confirm(row_index(model.rows(), "tuning"));
		require(model.title() == "DEBUG / tuning", "A file node's title");
		model.confirm(row_index(model.rows(), "player"));
		require(model.title() == "DEBUG / tuning / player", "The node's title is its path");
		std::vector<DebugMenuRow> rows = model.rows();
		require(rows.size() == DebugMenuModel::max_rows && rows.back().kind == DebugMenuRow::Kind::more &&
			rows.back().label.rfind("more (1/", 0) == 0, "A long node is paged with a more row");
		const std::size_t gravity = row_index(rows, "gravity");
		const std::string gravity_token = impl.tune_token("tuning.player.gravity");
		const float gravity_value = tuning.player().gravity;
		require(rows[gravity].kind == DebugMenuRow::Kind::variable && rows[gravity].value == gravity_token,
			"A leaf shows its token");
		require(rows[row_index(rows, "size")].kind == DebugMenuRow::Kind::category,
			"An inner node is a category row");
		model.step(gravity, 1);
		require(tuning.player().gravity == static_cast<float>(gravity_value +
			tune_step(gravity_value, tune_integer_token(gravity_token))) &&
			log_contains(impl.log(), "[menu] tune tuning.player.gravity next", DebugSeverity::echo),
			"Right on a leaf runs tune <path> next, echoed");
		model.step(gravity, -1);
		require(tuning.player().gravity == gravity_value, "Left runs prev");
		const std::string first_page = labels(model.rows()).front();
		model.confirm(model.rows().size() - 1);
		require(labels(model.rows()).front() != first_page &&
			model.rows().back().label.rfind("more (2/", 0) == 0, "more advances the page");
		model.back();
		require(model.title() == "DEBUG / tuning" &&
			model.rows()[model.focus_hint()].label == "player", "Back pops one segment onto the node it left");
		model.confirm(row_index(model.rows(), "player"));
		model.confirm(row_index(model.rows(), "size"));
		require(model.title() == "DEBUG / player / size" &&
			model.rows()[row_index(model.rows(), "y")].value.find("*rebuild") != std::string::npos,
			"A value that is not live is marked with its scope");
		model.back();
		model.back();
		model.back();
		require(model.title() == "DEBUG / tuning", "Back reaches the category");
		model.back();
		require(model.title() == "DEBUG", "and then the root");

		// The presentation boolean steps by toggle.
		model.confirm(row_index(model.rows(), "tuning"));
		model.confirm(row_index(model.rows(), "presentation"));
		// Presentation gains authored sections over time; follow its actual
		// pagination rather than assuming a particular section stays on page one.
		model.confirm(paged_row_index(model, "movement_feedback"));
		const bool enabled = presentation.movement_feedback().enabled;
		model.step(row_index(model.rows(), "enabled"), -1);
		require(presentation.movement_feedback().enabled != enabled,
			"Left or Right on a boolean toggles it");
		impl.execute("tune.reset");
		model.back();
		require(model.rows()[model.focus_hint()].label == "movement_feedback",
			"Back restores the presentation page and row it left");
		model.confirm(paged_row_index(model, "weapon_feedback"));
		const bool weapon_enabled = presentation.weapon_feedback().enabled;
		model.step(row_index(model.rows(), "enabled"), 1);
		require(presentation.weapon_feedback().enabled != weapon_enabled,
			"The weapon feedback group remains reachable and its boolean is tunable");
		impl.execute("tune.reset");
		model.back();
		model.confirm(paged_row_index(model, "movement_feedback"));
		const std::size_t colour_row = paged_row_index(model, "colour");
		const std::string colour_token = impl.tune_token("presentation.movement_feedback.colour");
		rows = model.rows();
		require(rows[colour_row].kind == DebugMenuRow::Kind::inspect && rows[colour_row].enabled &&
			rows[colour_row].value.find(colour_token) != std::string::npos &&
			rows[colour_row].value.find("(console)") != std::string::npos,
			"An audited colour has an enabled inspect row with its quoted token and console hint");
		const std::string colour_text = impl.tune_document("presentation")->current().text();
		const auto before_colours = colour_values(presentation);
		const std::string colour_title = model.title();
		impl.log().clear();
		model.step(colour_row, 1); model.step(colour_row, -1);
		require(impl.log().lines().empty() && colour_values(presentation) == before_colours &&
			impl.tune_document("presentation")->current().text() == colour_text,
			"Left/Right on a colour inspect row emits no step and changes no values or pending edits");
		require(model.confirm(colour_row) == DebugMenuModel::Result::stay && model.title() == colour_title &&
			log_contains(impl.log(), "[menu] tune presentation.movement_feedback.colour", DebugSeverity::echo) &&
			log_contains(impl.log(), "presentation.movement_feedback.colour = " + colour_token, DebugSeverity::info) &&
			colour_values(presentation) == before_colours,
			"Confirm inspects the colour through a read-only tune getter and stays on the same menu node");
	}
}
#endif
