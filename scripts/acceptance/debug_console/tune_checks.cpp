#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_menu_model.h"
#include "game/debug/debug_tokenizer.h"
#include "game/debug/json_spans.h"
#include "game/debug/tune_document.h"
#include "game/debug/tune_scope.h"
#include "game/objects/combat_feedback.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/math/scalar.h"
#include "engine/render/render_resources.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <process.h>

// Live tuning commands: tune, tune.diff, tune.reset and tune.save on scratch
// copies, and the colour, motion and menu-layout tunes.
namespace debug_console_acceptance
{
	namespace
	{
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
		const CombatFeedbackPresentation* const combat_look = &presentation.combat_feedback();
		const char* const combat_sheet = combat_look->sprite_sheet.data();
		const char* const combat_fill = combat_look->fill_frame.data();
		const auto combat_shape = [&]
		{
			return std::array<float, 9>{ combat_look->hurt_duration, combat_look->hurt_repeat_delay,
				combat_look->hurt_width, combat_look->hurt_reach, combat_look->elimination_duration,
				combat_look->elimination_spread, combat_look->elimination_lift,
				combat_look->particle_size, combat_look->keyline_width };
		};
		const auto combat_geometry = combat_shape();
		const bool combat_enabled = combat_look->enabled;
		const PlayerIdentityPresentation* const identity = &presentation.player_identity();
		const std::string team_sheet = presentation.main_menu().team_select_sheet;
		const auto team_frames = presentation.main_menu().team_select_frames;
		const auto team_selected_frames = presentation.main_menu().team_select_selected_frames;
		const char* const font = overlay->font.data();
		const char* const sheet = movement->sprite_sheet.data();
		std::array<std::string, 17> words{ "RED", "112233", "a1B2c3", "00000080",
			"FFFFFF", "12345678", "654321", "BLUE", "10203040", "RED", "56789ABC", "35BCE1", "2266CC80",
			"123456FF", "DDEEFF", "73A1C280", "19B87000" };
		std::vector<labrador::Colour> expected{ { 255, 0, 0 }, { 17, 34, 51 },
			{ 161, 178, 195 }, { 0, 0, 0, 128 }, { 255, 255, 255 }, { 18, 52, 86, 120 }, { 101, 67, 33 },
			{ 0, 0, 255 }, { 16, 32, 48, 64 }, { 255, 0, 0 }, { 86, 120, 154, 188 },
			{ 53, 188, 225 }, { 34, 102, 204, 128 }, { 18, 52, 86 }, { 221, 238, 255 },
			{ 115, 161, 194, 128 }, { 25, 184, 112, 0 } };
		require(impl.execute("feedback off").ok, "Colour tuning starts with an independent live F9 override");
		for (std::size_t index = 0; index < colour_paths.size(); ++index)
		{
			const std::string path = colour_paths[index];
			if (impl.tune_token(path) == "\"" + words[index] + "\"")
			{
				// Identity colours have a stricter opaque/distinct contract. Keep
				// their fallbacks opaque and different from one another too.
				words[index] = index == 0 ? "BLUE" : index == identity_ink_colour_index ? "FEDCBAFF" :
					index == identity_background_colour_index ? "0F1E2DFF" : "FEDCBA98";
				expected[index] = index == 0 ? labrador::Colour(0, 0, 255) :
					index == identity_ink_colour_index ? labrador::Colour(254, 220, 186) :
					index == identity_background_colour_index ? labrador::Colour(15, 30, 45) : labrador::Colour(254, 220, 186, 152);
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
			std::size_t(11), std::size_t(12), combat_highlight_colour_index, combat_keyline_colour_index })
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
			feedback->sprite_sheet.data() == feedback_sheet && feedback->fill_frame.data() == feedback_fill &&
			&presentation.combat_feedback() == combat_look && combat_look->sprite_sheet.data() == combat_sheet &&
			combat_look->fill_frame.data() == combat_fill && combat_shape() == combat_geometry &&
			combat_look->enabled == combat_enabled &&
			&presentation.player_identity() == identity && identity->background_colour == expected[identity_background_colour_index],
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
			colour_paths[11], colour_paths[12], colour_paths[identity_ink_colour_index], colour_paths[identity_background_colour_index],
			colour_paths[combat_highlight_colour_index], colour_paths[combat_keyline_colour_index] })
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
			"presentation.weapon_feedback.keyline_extra", "presentation.player_identity.background_colour.r",
			"presentation.player_identity.background_colour_extra", "presentation.player_identity.ink_colour.r",
			"presentation.player_identity.ink_colour_extra", "presentation.combat_feedback.sprite_sheet",
			"presentation.combat_feedback.fill_frame", "presentation.combat_feedback.highlight.r",
			"presentation.combat_feedback.highlight_colour", "presentation.combat_feedback.keyline_extra" })
		{
			require(impl.tune_token(path).empty() && !impl.execute(std::string("tune ") + path).ok,
				std::string("An arbitrary string is neither readable nor completed as tunable: ") + path);
			reject(path, "WHITE");
		}
		require(!presentation.movement_feedback().enabled, "A quoted injection cannot run a second console statement");
		require(impl.execute(std::string("tune.reset ") + colour_paths.front()).ok &&
			colour_values(presentation).front() == initial.front() &&
			colour_values(presentation)[identity_ink_colour_index] == expected[identity_ink_colour_index] &&
			colour_values(presentation)[identity_background_colour_index] == expected[identity_background_colour_index],
			"A colour path reset leaves both identity colour edits intact");
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
		for (const std::size_t index : { std::size_t(11), std::size_t(12),
			combat_highlight_colour_index, combat_keyline_colour_index })
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
				(index != combat_highlight_colour_index ||
					impl.tune_document("presentation")->edits().count("combat_feedback.keyline") == 1) &&
				&presentation.weapon_feedback() == feedback && feedback->sprite_sheet.data() == feedback_sheet &&
				feedback->fill_frame.data() == feedback_fill && &presentation.combat_feedback() == combat_look &&
				combat_look->sprite_sheet.data() == combat_sheet && combat_look->fill_frame.data() == combat_fill &&
				combat_shape() == combat_geometry && combat_look->enabled == combat_enabled,
				"Each feedback colour resets independently while other edits and captured bindings remain intact: " + path);
		}
		for (const std::size_t index : { identity_ink_colour_index, identity_background_colour_index })
		{
			const std::size_t sibling = index == identity_ink_colour_index ? identity_background_colour_index : identity_ink_colour_index;
			const std::string path = colour_paths[index];
			const std::string local = path.substr(std::string("presentation.").size());
			const std::string sibling_local = std::string(colour_paths[sibling]).substr(std::string("presentation.").size());
			const auto before_reset = colour_values(presentation);
			auto restored = before_reset;
			restored[index] = initial[index];
			require(impl.tune_document("presentation")->edits().count(local) == 1 &&
				impl.execute("tune.reset " + path).ok && colour_values(presentation) == restored &&
				impl.tune_document("presentation")->edits().count(local) == 0 &&
				impl.tune_document("presentation")->edits().count(sibling_local) == 1 &&
				impl.tune_document("presentation")->edits().count("main_menu.team_select.unselected") == 1 &&
				&presentation.player_identity() == identity,
				"Each identity reset preserves its sibling, borrowed record and unrelated pending tints: " + path);
			require(impl.execute("tune " + path + " " + words[index]).ok && colour_values(presentation) == before_reset,
				"An identity reset can be reapplied without changing its sibling or any other colour: " + path);
		}
		require(impl.execute("tune.reset presentation").ok && colour_values(presentation) == initial &&
			impl.tune_document("presentation")->edits().empty(), "A presentation reset restores every colour token");

		// The literal save oracle replaces one exact span in otherwise untouched
		// LF content. A subsequent external edit to another field must survive.
		for (const std::size_t index : { std::size_t(5), std::size_t(7), std::size_t(8), std::size_t(9), std::size_t(10),
			std::size_t(11), std::size_t(12), combat_highlight_colour_index, combat_keyline_colour_index })
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
				index == 11 ? "607080" : index == 12 ? "708090" :
				index == combat_highlight_colour_index ? "8192A3" : "92A3B4";
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
		for (const std::size_t index : { std::size_t(9), std::size_t(10), std::size_t(11), std::size_t(12),
			combat_highlight_colour_index, combat_keyline_colour_index })
		{
			// Each new path also starts from an independent CRLF document, so
			// external adoption cannot pass by reusing an earlier case's value.
			const wchar_t* const scratch_name = index == 9 ? L"weapon-selected-crlf" :
				index == 10 ? L"weapon-unselected-crlf" : index == 11 ? L"feedback-highlight-crlf" :
				index == 12 ? L"feedback-keyline-crlf" : index == combat_highlight_colour_index ?
				L"combat-highlight-crlf" : L"combat-keyline-crlf";
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
			const char* unrelated_word = index == 9 ? "607080" : index == 10 ? "708090" :
				index == 11 ? "8192A3" : index == 12 ? "92A3B4" :
				index == combat_highlight_colour_index ? "A3B4C5" : "B4C5D6";
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

	void check_identity_colour_tunes(const std::filesystem::path& game_directory)
	{
		for (const bool background : { false, true })
		{
			const std::size_t index = background ? identity_background_colour_index : identity_ink_colour_index;
			const std::size_t sibling_index = background ? identity_ink_colour_index : identity_background_colour_index;
			const std::string path = colour_paths[index];
			const std::string local = path.substr(std::string("presentation.").size());
			const std::string sibling_path = colour_paths[sibling_index];
			const std::string sibling_local = sibling_path.substr(std::string("presentation.").size());
			const auto value_of = [background](const PlayerIdentityPresentation& look)
				{ return background ? look.background_colour : look.ink_colour; };
			const auto sibling_of = [background](const PlayerIdentityPresentation& look)
				{ return background ? look.ink_colour : look.background_colour; };
			for (const bool crlf : { false, true })
			{
				ScratchContent scratch(game_directory, background ?
					(crlf ? L"identity-background-crlf" : L"identity-background-lf") :
					(crlf ? L"identity-ink-crlf" : L"identity-ink-lf"));
				const auto source = scratch.source / L"presentation.json";
				const auto staged = scratch.staged / L"presentation.json";
				std::string baseline = read_text(source);
				if (crlf)
				{
					std::string converted;
					for (const char character : baseline)
						{ converted += character == '\n' ? std::string("\r\n") : std::string(1, character); }
					baseline = std::move(converted);
					write_text(source, baseline);
					write_text(staged, baseline);
				}
				GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
				Presentation presentation(staged.string().c_str());
				TestDirector test(tuning, presentation);
				DebugDirectorImpl& impl = test.impl;
				impl.set_content_directories(scratch.source, scratch.staged);
				const PlayerIdentityPresentation* const identity = &presentation.player_identity();
				const auto initial = colour_values(presentation);
				const auto state = [&] { return colour_values(presentation); };
				const auto edits = [&]
				{
					std::vector<std::string> values;
					for (const auto& [key, edit] : impl.tune_document("presentation")->edits())
						{ values.push_back(key + "=" + edit.baseline_token + "->" + edit.current_token); }
					return values;
				};
				for (const auto& [word, colour] : std::array<std::pair<const char*, labrador::Colour>, 3>{ {
					{ "RED", { 255, 0, 0 } }, { "a1B2c3", { 161, 178, 195 } }, { "102030FF", { 16, 32, 48 } } } })
				{
					auto expected = initial;
					expected[index] = colour;
					require(impl.execute("tune " + path + " " + word).ok && colour_values(presentation) == expected &&
						&presentation.player_identity() == identity && impl.tune_token(path) == std::string("\"") + word + '"',
						"Identity colour accepts named/RGB/opaque RGBA on its borrowed record without changing other colours: " + path);
				}
				const JsonSpanDocument authored(baseline, "identity-colour-reference");
				const std::string sibling_token(authored.token(*authored.find(sibling_local)));
				for (const std::string& word : { std::string("11223380"), std::string("11223300"),
					sibling_token.substr(1, sibling_token.size() - 2) })
				{
					const auto before_state = state();
					const auto before_edits = edits();
					const std::string before_text = impl.tune_document("presentation")->current().text();
					require(!impl.execute("tune " + path + " " + word).ok && state() == before_state && edits() == before_edits &&
						impl.tune_document("presentation")->current().text() == before_text &&
						read_text(source) == baseline && read_text(staged) == baseline,
						"Translucent, invisible or sibling-matching identity colour preserves live values, pending tokens and both files: " + path);
				}
				require(impl.execute("tune.reset " + path).ok && colour_values(presentation) == initial &&
					impl.tune_document("presentation")->current().text() == baseline && edits().empty(),
					"Identity colour resets exactly to its authored token without a rebuild or file write: " + path);

				// Both identity leaves can be pending at once. Validate against the
				// currently edited sibling, and reset one without resetting the other.
				require(initial[3] != labrador::Colour(32, 48, 64) &&
					impl.execute("tune " + path + " RED").ok && impl.execute("tune " + sibling_path + " BLUE").ok &&
					impl.execute("tune presentation.debug_overlay.colour 203040").ok,
					"Both distinct opaque identity fields and an unrelated colour can be pending together: " + path);
				const auto paired_state = state();
				const auto paired_edits = edits();
				const std::string paired_text = impl.tune_document("presentation")->current().text();
				require(!impl.execute("tune " + path + " BLUE").ok && state() == paired_state && edits() == paired_edits &&
					impl.tune_document("presentation")->current().text() == paired_text &&
					read_text(source) == baseline && read_text(staged) == baseline,
					"Identity equality is checked against the pending sibling and refusal is atomic: " + path);
				auto target_reset = paired_state;
				target_reset[index] = initial[index];
				require(impl.execute("tune.reset " + path).ok && state() == target_reset &&
					impl.tune_document("presentation")->edits().count(local) == 0 &&
					impl.tune_document("presentation")->edits().count(sibling_local) == 1 &&
					impl.tune_document("presentation")->edits().count("debug_overlay.colour") == 1,
					"Target identity reset preserves its pending sibling and unrelated colour: " + path);
				require(impl.execute("tune " + path + " RED").ok && impl.execute("tune.reset " + sibling_path).ok,
					"A pending identity edit permits an independent sibling reset: " + path);
				auto sibling_reset = paired_state;
				sibling_reset[sibling_index] = initial[sibling_index];
				require(state() == sibling_reset && impl.tune_document("presentation")->edits().count(local) == 1 &&
					impl.tune_document("presentation")->edits().count(sibling_local) == 0 &&
					impl.tune_document("presentation")->edits().count("debug_overlay.colour") == 1 &&
					impl.execute("tune.reset presentation").ok && state() == initial && edits().empty() &&
					impl.tune_document("presentation")->current().text() == baseline &&
					read_text(source) == baseline && read_text(staged) == baseline,
					"Sibling reset preserves target and unrelated edits; full reset restores exact authored identity bytes: " + path);

				// Saving merges a fresh unrelated disk edit, splices only our colour,
				// then adopts and rebases the full validated file.
				const std::string external = authored.replaced(*authored.find("debug_overlay.colour"), "\"203040\"");
				require(initial[3] != labrador::Colour(32, 48, 64), "The identity save introduces a fresh unrelated external colour");
				write_text(source, external);
				const JsonSpanDocument external_document(external, "identity-colour-reference");
				const std::string saved = external_document.replaced(*external_document.find(local), "\"123456FF\"");
				auto saved_values = initial;
				saved_values[3] = labrador::Colour(32, 48, 64);
				saved_values[index] = labrador::Colour(18, 52, 86);
				require(impl.execute("tune " + path + " 123456FF").ok && impl.execute("tune.save presentation").ok &&
					read_text(source) == saved && read_text(staged) == saved && colour_values(presentation) == saved_values &&
					impl.tune_document("presentation")->baseline().text() == saved && edits().empty() &&
					colour_values(Presentation(source.string().c_str())) == saved_values &&
					count_of(saved, "\r\n") == count_of(baseline, "\r\n") && count_of(saved, "\n") == count_of(baseline, "\n"),
					"Identity save changes only its quoted LF/CRLF token, preserves the external edit, loads and rebases both copies: " + path);
				require(impl.execute("tune " + path + " a1B2c3").ok && impl.execute("tune.reset " + path).ok &&
					colour_values(presentation) == saved_values && impl.tune_document("presentation")->current().text() == saved && edits().empty(),
					"Identity reset after save restores the saved opaque spelling and adopted external baseline: " + path);

				const JsonSpanDocument saved_document(saved, "identity-colour-conflict");
				const std::string conflicting = saved_document.replaced(*saved_document.find(local), "\"FEDCBA\"");
				write_text(source, conflicting);
				require(impl.execute("tune " + path + " ABCDEF").ok, "Identity colour is pending before an on-disk token conflict: " + path);
				const auto conflict_state = state();
				const auto conflict_edits = edits();
				const std::string conflict_text = impl.tune_document("presentation")->current().text();
				require(!impl.execute("tune.save presentation").ok && read_text(source) == conflicting && read_text(staged) == saved &&
					state() == conflict_state && edits() == conflict_edits && impl.tune_document("presentation")->current().text() == conflict_text,
					"An identity colour token conflict preserves source, staging, live colours and the complete pending edit: " + path);
				const JsonSpanDocument conflicting_document(conflicting, "identity-colour-conflict");
				const std::string forced = conflicting_document.replaced(*conflicting_document.find(local), "\"ABCDEF\"");
				require(impl.execute("tune.save presentation force").ok && read_text(source) == forced && read_text(staged) == forced &&
					value_of(*identity) == labrador::Colour(171, 205, 239) && impl.tune_document("presentation")->baseline().text() == forced && edits().empty(),
					"Force resolves only the identity token conflict and rebases the validated opaque colour: " + path);

				// Each file is valid independently. Pending RED ink + external RED
				// background, and the inverse, must fail only at the merged candidate.
				require(impl.execute("tune " + path + " RED").ok, "A distinct red identity colour is pending before the coupled disk conflict: " + path);
				const JsonSpanDocument forced_document(forced, "identity-colour-merged-conflict");
				const std::string coupled = forced_document.replaced(*forced_document.find(sibling_local), "\"RED\"");
				write_text(source, coupled);
				require(sibling_of(Presentation(source.string().c_str()).player_identity()) == labrador::Colour::red &&
					sibling_of(*identity) != labrador::Colour::red, "The external red sibling is valid before merging and is not yet live: " + path);
				const auto coupled_state = state();
				const auto coupled_edits = edits();
				const std::string coupled_text = impl.tune_document("presentation")->current().text();
				for (const char* command : { "tune.save presentation", "tune.save presentation force" })
				{
					impl.log().clear();
					require(!impl.execute(command).ok && read_text(source) == coupled && read_text(staged) == forced &&
						state() == coupled_state && edits() == coupled_edits && impl.tune_document("presentation")->current().text() == coupled_text &&
						impl.tune_document("presentation")->baseline().text() == forced &&
						log_contains(impl.log(), "the written file did not load", DebugSeverity::error) &&
						log_contains(impl.log(), "; the old bytes are back", DebugSeverity::error),
						"Invalid merged identity colours restore the exact external LF/CRLF source and preserve staged/live/ledger state, even with force: " + path);
				}
				require(impl.execute("tune.reset " + path).ok && value_of(*identity) == labrador::Colour(171, 205, 239) &&
					read_text(source) == coupled && read_text(staged) == forced && edits().empty() &&
					!std::filesystem::exists(source.string() + ".tmp") && !std::filesystem::exists(source.string() + ".bak") &&
					!std::filesystem::exists(staged.string() + ".tmp"),
					"After rejected merged saves, reset still uses the saved baseline without discarding the external source or leaving recovery files: " + path);
			}
		}
	}

	void check_combat_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"combat-colour-cues");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const CombatFeedbackPresentation* const look = &presentation.combat_feedback();
		labrador::RenderResources resources;
		resources.add_sprite_sheet(look->sprite_sheet, labrador::read_sprite_sheet(
			(game_directory / L"textures" / (look->sprite_sheet + ".json")).string().c_str(), labrador::TextureHandle{}));
		CombatFeedback hurt(look, &resources), burst(look, &resources);
		const mattmath::RectangleF body(80, 120, 56, 96), detached(240, 120, 56, 96);
		require(hurt.hurt(), "Combat colour fixture starts an actual hurt cue before editing");
		hurt.update(look->hurt_duration * 0.5f);
		burst.eliminate(detached);
		burst.update(look->elimination_duration * 0.5f);
		const auto timers = [&]
		{
			return std::array<float, 4>{ hurt.hurt_remaining(), hurt.elimination_remaining(),
				burst.hurt_remaining(), burst.elimination_remaining() };
		};
		const auto held_timers = timers();
		const auto hurt_bounds = hurt.bounds(body), burst_bounds = burst.bounds(body);
		const auto unchanged = [&]
		{
			require(&presentation.combat_feedback() == look && timers() == held_timers &&
				hurt.death_rectangle() == mattmath::RectangleF::ZERO && burst.death_rectangle() == detached &&
				hurt.bounds(body) == hurt_bounds && burst.bounds(body) == burst_bounds,
				"Combat colour adoption retains the borrowed record, active cue timers, detached body and cull bounds");
		};
		for (const std::size_t index : { combat_highlight_colour_index, combat_keyline_colour_index })
		{
			for (const auto& [word, expected] : std::array<std::pair<const char*, labrador::Colour>, 3>{ {
				{ "RED", { 255, 0, 0 } }, { "2060C080", { 32, 96, 192, 128 } },
				{ "A532D600", { 165, 50, 214, 0 } } } })
			{
				require(impl.execute(std::string("tune ") + colour_paths[index] + ' ' + word).ok &&
					colour_values(presentation)[index] == expected,
					"A preconstructed combat cue accepts straight named/partial/zero-alpha authored channels");
				unchanged();
			}
		}
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const JsonSpanDocument baseline_document(baseline, "combat-colour-merged-reference");
		const JsonLeaf* repeat_delay = baseline_document.find("combat_feedback.hurt_repeat_delay");
		require(repeat_delay != nullptr, "Combat merged candidate has its unrelated timing constraint");
		const std::string external = baseline_document.replaced(*repeat_delay, "0.001");
		write_text(scratch.source / L"presentation.json", external);
		const TuneDocument* const pending = impl.tune_document("presentation");
		const std::string pending_text = pending->current().text();
		const auto pending_colours = colour_values(presentation);
		for (const char* command : { "tune.save presentation", "tune.save presentation force" })
		{
			require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == baseline && pending->current().text() == pending_text &&
				pending->baseline().text() == baseline && pending->edits().size() == 2 &&
				colour_values(presentation) == pending_colours,
				"An unrelated invalid combat constraint rejects the complete merged candidate and restores disk/staging/live/ledger, even with force");
			unchanged();
		}
		require(impl.execute("tune.reset presentation.combat_feedback.highlight").ok &&
			look->highlight == labrador::Colour(255, 255, 255) && look->keyline == labrador::Colour(165, 50, 214, 0) &&
			pending->edits().size() == 1 && pending->edits().contains("combat_feedback.keyline"),
			"Resetting combat highlight retains the independent nonblack zero-alpha keyline edit");
		unchanged();
		require(impl.execute("tune.reset presentation.combat_feedback.keyline").ok &&
			look->keyline == labrador::Colour(0, 0, 0) && pending->edits().empty() && pending->current().text() == baseline,
			"Resetting combat keyline restores its original authored token after rejected saves");
		unchanged();
		write_text(scratch.source / L"presentation.json", baseline);
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
		check_combat_colour_tunes(game_directory);
		check_identity_colour_tunes(game_directory);

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
}
#endif
