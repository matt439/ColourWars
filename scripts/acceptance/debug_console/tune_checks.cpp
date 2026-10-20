#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include "scripts/acceptance/debug_console/cpu_level.h"
#include "game/debug/debug_menu_model.h"
#include "game/debug/debug_tokenizer.h"
#include "game/debug/json_spans.h"
#include "game/debug/match_fingerprint.h"
#include "game/debug/tune_document.h"
#include "game/debug/tune_scope.h"
#include "game/objects/blocked_shot_feedback.h"
#include "game/objects/combat_feedback.h"
#include "game/objects/figure_acting.h"
#include "game/objects/idle_drip.h"
#include "game/objects/monitor.h"
#include "game/objects/respawn_redraw.h"
#include "game/objects/silhouette.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/math/scalar.h"
#include "engine/render/render_resources.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <memory>
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
		std::array<std::string, 36> words{ "RED", "112233", "a1B2c3", "00000080",
			"FFFFFF", "12345678", "654321", "BLUE", "10203040", "RED", "56789ABC", "35BCE1", "2266CC80",
			"123456FF", "DDEEFF", "73A1C280", "19B87000", "C26D3A80", "2A80D500", "93C74D80", "D973A680",
			"A147B980", "27C4A500", "5E91C780", "B86A4D40", "73C2A500", "D47C2A80", "61B2D440", "9C35E700", "3B97C180", "A65E2940", "93C74D80", "4CA7D980", "D6824F80", "A35CDE80", "31C8A640" };
		std::vector<labrador::Colour> expected{ { 255, 0, 0 }, { 17, 34, 51 },
			{ 161, 178, 195 }, { 0, 0, 0, 128 }, { 255, 255, 255 }, { 18, 52, 86, 120 }, { 101, 67, 33 },
			{ 0, 0, 255 }, { 16, 32, 48, 64 }, { 255, 0, 0 }, { 86, 120, 154, 188 },
			{ 53, 188, 225 }, { 34, 102, 204, 128 }, { 18, 52, 86 }, { 221, 238, 255 },
			{ 115, 161, 194, 128 }, { 25, 184, 112, 0 }, { 194, 109, 58, 128 }, { 42, 128, 213, 0 },
			{ 147, 199, 77, 128 }, { 217, 115, 166, 128 }, { 161, 71, 185, 128 }, { 39, 196, 165, 0 },
			{ 94, 145, 199, 128 }, { 184, 106, 77, 64 }, { 115, 194, 165, 0 },
			{ 212, 124, 42, 128 }, { 97, 178, 212, 64 }, { 156, 53, 231, 0 }, { 59, 151, 193, 128 }, { 166, 94, 41, 64 },
			{ 147, 199, 77, 128 }, { 76, 167, 217, 128 }, { 214, 130, 79, 128 }, { 163, 92, 222, 128 }, { 49, 200, 166, 64 } };
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
			std::size_t(11), std::size_t(12), combat_highlight_colour_index, combat_keyline_colour_index,
			blocked_highlight_colour_index, blocked_keyline_colour_index, idle_drip_keyline_colour_index,
			respawn_pen_colour_index, cursor_ink_colour_index, cursor_fill_colour_index,
			silhouette_ink_colour_index, silhouette_rim_colour_index, silhouette_shade_colour_index,
			reveal_ink_colour_index, reveal_paper_colour_index, reveal_shade_colour_index, cursor_shade_colour_index, cursor_shadow_colour_index, desktop_colour_index, power_light_colour_index, program_colour_index, acting_face_colour_index, acting_sweat_colour_index })
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
			colour_paths[combat_highlight_colour_index], colour_paths[combat_keyline_colour_index],
			colour_paths[blocked_highlight_colour_index], colour_paths[blocked_keyline_colour_index],
			colour_paths[idle_drip_keyline_colour_index], colour_paths[respawn_pen_colour_index],
			colour_paths[cursor_ink_colour_index], colour_paths[cursor_fill_colour_index],
			colour_paths[silhouette_ink_colour_index], colour_paths[silhouette_rim_colour_index],
			colour_paths[silhouette_shade_colour_index], colour_paths[reveal_ink_colour_index],
			colour_paths[reveal_paper_colour_index], colour_paths[reveal_shade_colour_index], colour_paths[cursor_shade_colour_index], colour_paths[cursor_shadow_colour_index], colour_paths[desktop_colour_index], colour_paths[power_light_colour_index], colour_paths[program_colour_index], colour_paths[acting_face_colour_index], colour_paths[acting_sweat_colour_index] })
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
			"presentation.combat_feedback.highlight_colour", "presentation.combat_feedback.keyline_extra",
			"presentation.blocked_shot.sprite_sheet", "presentation.blocked_shot.fill_frame",
			"presentation.blocked_shot.highlight.r", "presentation.blocked_shot.highlight_colour",
			"presentation.blocked_shot.keyline_extra", "presentation.idle_drip.sprite_sheet",
			"presentation.idle_drip.fill_frame", "presentation.idle_drip.keyline.r",
			"presentation.idle_drip.keyline_extra", "presentation.idle_drip.tools.sprayer.keyline",
			"presentation.idle_drip.tools.sniper.keyline", "presentation.idle_drip.tools.bucket.keyline",
			"presentation.respawn_redraw.sprite_sheet", "presentation.respawn_redraw.pen_frame",
			"presentation.respawn_redraw.pen_colour.r", "presentation.respawn_redraw.pen_colour_extra",
			"presentation.respawn_redraw.strokes[0].part", "presentation.opening.sprite_sheet",
			"presentation.opening.fill_frame", "presentation.opening.cursor_art.ink.r",
			"presentation.opening.cursor_art.ink_colour", "presentation.opening.cursor_art.fill_extra",
			"presentation.opening.cursor_art.shade.r", "presentation.opening.cursor_art.shadow_extra",
			"presentation.opening.cursor_art.shade_extra", "presentation.opening.cursor_art.shadow.r",
			"opening.cursor_art.shade", "opening.cursor_art.shadow",
			"tuning.opening.cursor_art.shade", "tuning.opening.cursor_art.shadow",
			"presentation.opening.silhouette", "presentation.opening.silhouette_art.ink.r",
			"presentation.opening.silhouette_art.rim_extra", "presentation.opening.silhouette_art.shade_colour",
			"presentation.opening.percent_font", "presentation.opening.reveal_art.ink.r",
			"presentation.opening.reveal_art.paper_extra", "presentation.opening.reveal_art.shade_colour",
			"opening.desktop_colour", "tuning.opening.desktop_colour", "presentation.opening.desktop_colour.r",
			"presentation.opening.desktop_colour_extra", "opening.monitor_art.power_light",
			"tuning.opening.monitor_art.power_light", "presentation.opening.monitor_art.power_light.r",
			"presentation.opening.monitor_art.power_light_extra", "presentation.opening.monitor_art.ink",
			"presentation.opening.monitor_art.highlight", "opening.program_colour", "tuning.opening.program_colour",
			"presentation.opening.program_colour.r", "presentation.opening.program_colour_extra",
			"opening.acting.face", "tuning.opening.acting.face", "presentation.opening.acting.face.r",
			"presentation.opening.acting.face_extra", "opening.acting.sweat", "tuning.opening.acting.sweat",
			"presentation.opening.acting.sweat.r", "presentation.opening.acting.sweat_extra" })
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
			combat_highlight_colour_index, combat_keyline_colour_index,
			blocked_highlight_colour_index, blocked_keyline_colour_index, idle_drip_keyline_colour_index,
			respawn_pen_colour_index, cursor_ink_colour_index, cursor_fill_colour_index,
			silhouette_ink_colour_index, silhouette_rim_colour_index, silhouette_shade_colour_index,
			reveal_ink_colour_index, reveal_paper_colour_index, reveal_shade_colour_index, cursor_shade_colour_index, cursor_shadow_colour_index, desktop_colour_index, power_light_colour_index, program_colour_index, acting_face_colour_index, acting_sweat_colour_index })
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
				(index != blocked_highlight_colour_index ||
					impl.tune_document("presentation")->edits().count("blocked_shot.keyline") == 1) &&
				(index != cursor_ink_colour_index ||
					impl.tune_document("presentation")->edits().count("opening.cursor_art.fill") == 1) &&
				(index != silhouette_ink_colour_index ||
					(impl.tune_document("presentation")->edits().contains("opening.silhouette_art.rim") &&
						impl.tune_document("presentation")->edits().contains("opening.silhouette_art.shade"))) &&
				(index != silhouette_rim_colour_index ||
					impl.tune_document("presentation")->edits().contains("opening.silhouette_art.shade")) &&
				(index != reveal_ink_colour_index ||
					(impl.tune_document("presentation")->edits().contains("opening.reveal_art.paper") &&
						impl.tune_document("presentation")->edits().contains("opening.reveal_art.shade"))) &&
				(index != reveal_paper_colour_index ||
					impl.tune_document("presentation")->edits().contains("opening.reveal_art.shade")) &&
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
			std::size_t(11), std::size_t(12), combat_highlight_colour_index, combat_keyline_colour_index,
			blocked_highlight_colour_index, blocked_keyline_colour_index, idle_drip_keyline_colour_index,
			respawn_pen_colour_index, cursor_ink_colour_index, cursor_fill_colour_index,
			silhouette_ink_colour_index, silhouette_rim_colour_index, silhouette_shade_colour_index,
			reveal_ink_colour_index, reveal_paper_colour_index, reveal_shade_colour_index, cursor_shade_colour_index, cursor_shadow_colour_index, desktop_colour_index, power_light_colour_index, program_colour_index, acting_face_colour_index, acting_sweat_colour_index })
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
				index == combat_highlight_colour_index ? "8192A3" : index == combat_keyline_colour_index ? "92A3B4" :
				index == blocked_highlight_colour_index ? "A3B4C5" :
				index == blocked_keyline_colour_index ? "B4C5D6" :
				index == idle_drip_keyline_colour_index ? "C5D6E7" : index == respawn_pen_colour_index ? "D6E7F8" :
				index == cursor_ink_colour_index ? "E7F809" : index == cursor_fill_colour_index ? "F8091A" :
				index == silhouette_ink_colour_index ? "0A1B2C" :
				index == silhouette_rim_colour_index ? "1B2C3D" : index == silhouette_shade_colour_index ? "2C3D4E" :
				index == reveal_ink_colour_index ? "3D4E5F" : index == reveal_paper_colour_index ? "4E5F60" : index == reveal_shade_colour_index ? "5F6071" : index == cursor_shade_colour_index ? "607182" : index == cursor_shadow_colour_index ? "718293" : index == desktop_colour_index ? "8293A4" : index == power_light_colour_index ? "93A4B5" : index == program_colour_index ? "A4B5C6" : index == acting_face_colour_index ? "B5C6D7" : "C6D7E8";
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
			combat_highlight_colour_index, combat_keyline_colour_index,
			blocked_highlight_colour_index, blocked_keyline_colour_index, idle_drip_keyline_colour_index,
			respawn_pen_colour_index, cursor_ink_colour_index, cursor_fill_colour_index,
			silhouette_ink_colour_index, silhouette_rim_colour_index, silhouette_shade_colour_index,
			reveal_ink_colour_index, reveal_paper_colour_index, reveal_shade_colour_index, cursor_shade_colour_index, cursor_shadow_colour_index, desktop_colour_index, power_light_colour_index, program_colour_index, acting_face_colour_index, acting_sweat_colour_index })
		{
			// Each new path also starts from an independent CRLF document, so
			// external adoption cannot pass by reusing an earlier case's value.
			const wchar_t* const scratch_name = index == 9 ? L"weapon-selected-crlf" :
				index == 10 ? L"weapon-unselected-crlf" : index == 11 ? L"feedback-highlight-crlf" :
				index == 12 ? L"feedback-keyline-crlf" : index == combat_highlight_colour_index ?
				L"combat-highlight-crlf" : index == combat_keyline_colour_index ? L"combat-keyline-crlf" :
				index == blocked_highlight_colour_index ? L"blocked-highlight-crlf" :
				index == blocked_keyline_colour_index ? L"blocked-keyline-crlf" :
				index == idle_drip_keyline_colour_index ? L"idle-drip-keyline-crlf" :
				index == respawn_pen_colour_index ? L"respawn-pen-crlf" :
				index == cursor_ink_colour_index ? L"cursor-ink-crlf" : index == cursor_fill_colour_index ? L"cursor-fill-crlf" :
				index == silhouette_ink_colour_index ? L"silhouette-ink-crlf" :
				index == silhouette_rim_colour_index ? L"silhouette-rim-crlf" :
				index == silhouette_shade_colour_index ? L"silhouette-shade-crlf" :
				index == reveal_ink_colour_index ? L"reveal-ink-crlf" :
				index == reveal_paper_colour_index ? L"reveal-paper-crlf" : index == reveal_shade_colour_index ? L"reveal-shade-crlf" : index == cursor_shade_colour_index ? L"cursor-shade-crlf" : index == cursor_shadow_colour_index ? L"cursor-shadow-crlf" : index == desktop_colour_index ? L"desktop-colour-crlf" : index == power_light_colour_index ? L"power-light-colour-crlf" : index == program_colour_index ? L"program-colour-crlf" : index == acting_face_colour_index ? L"acting-face-colour-crlf" : L"acting-sweat-colour-crlf";
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
				index == combat_highlight_colour_index ? "A3B4C5" : index == combat_keyline_colour_index ? "B4C5D6" :
				index == blocked_highlight_colour_index ? "C5D6E7" :
				index == blocked_keyline_colour_index ? "D6E7F8" :
				index == idle_drip_keyline_colour_index ? "E7F809" : index == respawn_pen_colour_index ? "F8091A" :
				index == cursor_ink_colour_index ? "091A2B" : index == cursor_fill_colour_index ? "1A2B3C" :
				index == silhouette_ink_colour_index ? "2B3C4D" :
				index == silhouette_rim_colour_index ? "3C4D5E" : index == silhouette_shade_colour_index ? "4D5E6F" :
				index == reveal_ink_colour_index ? "5E6F70" : index == reveal_paper_colour_index ? "6F7081" : index == reveal_shade_colour_index ? "708192" : index == cursor_shade_colour_index ? "8192A3" : index == cursor_shadow_colour_index ? "92A3B4" : index == desktop_colour_index ? "A3B4C5" : index == power_light_colour_index ? "B4C5D6" : index == program_colour_index ? "C5D6E7" : index == acting_face_colour_index ? "D6E7F8" : "E7F809";
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

	void check_blocked_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"blocked-colour-cues");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const BlockedShotPresentation* const look = &presentation.blocked_shot();
		const std::string sheet = look->sprite_sheet, fill = look->fill_frame;
		const auto initial_colours = colour_values(presentation);
		const bool enabled = look->enabled;
		const auto shape = [&]
		{
			return std::array<float, 7>{ look->duration, look->repeat_delay, look->width,
				look->reach, look->bead_size, look->stroke_width, look->keyline_width };
		};
		const auto held_shape = shape();
		labrador::RenderResources resources;
		resources.add_sprite_sheet(sheet, labrador::read_sprite_sheet(
			(game_directory / L"textures" / (sheet + ".json")).string().c_str(), labrador::TextureHandle{}));
		BlockedShotFeedback feedback(look, &resources);
		const std::array<mattmath::Vector2F, 4> normals{ mattmath::Vector2F(2.0f, 0.0f),
			mattmath::Vector2F(0.0f, -2.0f), mattmath::Vector2F(-2.0f, 0.0f), mattmath::Vector2F(0.0f, 2.0f) };
		for (int index = 0; index <= paint_owners::count; ++index)
		{
			feedback.emit({ mattmath::Vector2F(120.0f + static_cast<float>(index) * 40.0f, 200.0f),
				normals[static_cast<std::size_t>(index) % normals.size()],
				labrador::Colour(32 + index * 12, 192 - index * 9, 80 + index * 7, 160),
				index == paint_owners::count ? -1 : index });
		}
		// Keep every repeat budget positive as well as every cue age nonzero;
		// a reset to zero must not pass this retained-state check vacuously.
		feedback.update(std::min(look->duration, look->repeat_delay) * 0.5f);
		require(feedback.cues().size() == static_cast<std::size_t>(paint_owners::count + 1) &&
			feedback.bounds() != mattmath::RectangleF::ZERO &&
			std::all_of(feedback.cues().begin(), feedback.cues().end(), [&](const BlockedShotFeedback::Cue& cue)
				{ return cue.age > 0.0f && cue.age < look->duration; }),
			"Blocked colour fixture retains an actual aged cue for every paint owner and the invalid-owner bucket");
		for (int owner = -1; owner < paint_owners::count; ++owner)
		{
			require(feedback.repeat_remaining(owner) > 0.0f,
				"Every blocked-colour fixture owner starts with an active repeat budget");
		}
		const auto state = [&]
		{
			std::vector<float> values;
			for (const BlockedShotFeedback::Cue& cue : feedback.cues())
			{
				const auto& contact = cue.contact;
				const auto bounds = feedback.cue_bounds(cue);
				values.insert(values.end(), { contact.position.x, contact.position.y,
					contact.normal.x, contact.normal.y, contact.team_colour.r, contact.team_colour.g,
					contact.team_colour.b, contact.team_colour.a, static_cast<float>(contact.player_num),
					cue.age, bounds.x, bounds.y, bounds.width, bounds.height });
			}
			for (int owner = -1; owner < paint_owners::count; ++owner)
				{ values.push_back(feedback.repeat_remaining(owner)); }
			values.push_back(feedback.repeat_remaining(paint_owners::count));
			return values;
		};
		const auto held_state = state();
		const auto held_bounds = feedback.bounds();
		const auto unchanged = [&]
		{
			require(&presentation.blocked_shot() == look && look->sprite_sheet == sheet &&
				look->fill_frame == fill && look->enabled == enabled && shape() == held_shape &&
				state() == held_state && feedback.bounds() == held_bounds &&
				feedback.repeat_remaining(-1) == feedback.repeat_remaining(paint_owners::count),
				"Blocked colour adoption retains the borrowed look, assets, geometry, stored contacts/team tints, ages, bounds and every owner repeat budget");
		};
		std::srand(0x6b117);
		const int expected_random = std::rand();
		std::srand(0x6b117);
		for (const std::size_t index : { blocked_highlight_colour_index, blocked_keyline_colour_index })
		{
			for (const auto& [word, expected] : std::array<std::pair<const char*, labrador::Colour>, 3>{ {
				{ "RED", { 255, 0, 0 } }, { "2060C080", { 32, 96, 192, 128 } },
				{ "A532D600", { 165, 50, 214, 0 } } } })
			{
				require(impl.execute(std::string("tune ") + colour_paths[index] + ' ' + word).ok &&
					colour_values(presentation)[index] == expected,
					"A preconstructed blocked cue accepts straight named/partial/zero-alpha authored channels");
				unchanged();
			}
		}
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const JsonSpanDocument baseline_document(baseline, "blocked-colour-merged-reference");
		const JsonLeaf* repeat_delay = baseline_document.find("blocked_shot.repeat_delay");
		require(repeat_delay != nullptr, "Blocked merged candidate has its unrelated timing constraint");
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
				pending->edits().contains("blocked_shot.highlight") && pending->edits().contains("blocked_shot.keyline") &&
				colour_values(presentation) == pending_colours,
				"An invalid blocked timing constraint rejects the complete merged candidate without changing disk/staging/live/ledger, even with force");
			unchanged();
		}
		require(impl.execute("tune.reset presentation.blocked_shot.highlight").ok &&
			look->highlight == initial_colours[blocked_highlight_colour_index] &&
			look->keyline == labrador::Colour(165, 50, 214, 0) && pending->edits().size() == 1 &&
			pending->edits().contains("blocked_shot.keyline"),
			"Resetting blocked highlight retains the independent nonblack zero-alpha keyline edit");
		unchanged();
		require(impl.execute("tune.reset presentation.blocked_shot.keyline").ok &&
			colour_values(presentation) == initial_colours && pending->edits().empty() && pending->current().text() == baseline,
			"Resetting blocked keyline restores both original authored tokens after rejected saves");
		unchanged();
		write_text(scratch.source / L"presentation.json", baseline);
		const JsonSpanDocument saved_highlight(baseline_document.replaced(
			*baseline_document.find("blocked_shot.highlight"), "\"2060C080\""), "blocked-colour-save-reference");
		const std::string saved = saved_highlight.replaced(*saved_highlight.find("blocked_shot.keyline"), "\"A532D600\"");
		require(impl.execute("tune presentation.blocked_shot.highlight 2060C080").ok &&
			impl.execute("tune presentation.blocked_shot.keyline A532D600").ok &&
			impl.execute("tune.save presentation").ok && read_text(scratch.source / L"presentation.json") == saved &&
			read_text(scratch.staged / L"presentation.json") == saved && pending->edits().empty() &&
			pending->baseline().text() == saved && pending->current().text() == saved &&
			look->highlight == labrador::Colour(32, 96, 192, 128) && look->keyline == labrador::Colour(165, 50, 214, 0),
			"Saving both blocked colours changes only the two authored tokens and rebases the retained cue's borrowed look");
		unchanged();
		require(std::rand() == expected_random,
			"Blocked colour edits, resets and valid/refused saves consume no gameplay random draw");
	}

	void check_idle_drip_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"idle-drip-colour-cues");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const std::array<WeaponType, 5> types{ WeaponType::sprayer, WeaponType::sniper,
			WeaponType::roller, WeaponType::mister, WeaponType::bucket };
		std::array<const IdleDripPresentation*, 5> looks{};
		std::array<IdleDripPresentation, 5> original{};
		for (std::size_t index = 0; index < types.size(); ++index)
		{
			looks[index] = &presentation.idle_drip(types[index]);
			original[index] = *looks[index];
			require(looks[index]->enabled == (index == 0 || index == 1 || index == 4),
				"Idle-drip colour fixtures retain the three eligible tools and disabled Roller/Mister profiles");
		}
		const auto geometry = [](const IdleDripPresentation& look)
		{
			return std::array<float, 11>{ look.interval_min, look.interval_max, look.grow_time,
				look.stretch_time, look.lifetime, look.gravity, look.size, look.stretch,
				look.nozzle_inset, look.nozzle_underside, look.keyline_width };
		};
		labrador::RenderResources resources;
		resources.add_sprite_sheet(original.front().sprite_sheet, labrador::read_sprite_sheet(
			(game_directory / L"textures" / (original.front().sprite_sheet + ".json")).string().c_str(), labrador::TextureHandle{}));
		// Separate consumers are necessary: the next quiet interval exceeds a
		// falling drop's lifetime. Do not pretend both states coexist by default.
		std::array<std::unique_ptr<IdleDrip>, 6> cues;
		const std::array<WeaponType, 3> eligible{ WeaponType::sprayer, WeaponType::sniper, WeaponType::bucket };
		for (std::size_t index = 0; index < cues.size(); ++index)
		{
			const IdleDripPresentation& look = presentation.idle_drip(eligible[index / 2]);
			cues[index] = std::make_unique<IdleDrip>(&look, &resources,
				index % 2 == 0 ? labrador::Colour(224, 48, 80) : labrador::Colour(48, 160, 224),
				static_cast<uint32_t>(index));
			IdleDrip& cue = *cues[index];
			const mattmath::Vector2F nozzle(160.0f + static_cast<float>(index) * 48.0f, 160.0f);
			cue.advance(true, nozzle, 0.0f, cue.interval_remaining() + 0.001f, {});
			cue.advance(true, nozzle, 0.0f, look.grow_time + look.stretch_time *
				(index % 2 == 0 ? 0.5f : 1.1f), {});
			if (index % 2 != 0) { cue.advance(false, nozzle, 0.0f, look.lifetime * 0.5f, {}); }
			require(cue.bounds() != mattmath::RectangleF::ZERO &&
				(index % 2 == 0 ? cue.attached() && cue.bead_age() > 0.0f && cue.drops().empty() :
					!cue.attached() && cue.drops().size() == 1 && cue.drops().front().age > 0.0f &&
					cue.drops().front().age < look.lifetime),
				"Each eligible tool has a real aged attached bead and a separate aged falling drop before colour adoption");
		}
		const auto state = [&]
		{
			std::vector<float> values;
			const auto append_rectangle = [&](const mattmath::RectangleF& rectangle)
				{ values.insert(values.end(), { rectangle.x, rectangle.y, rectangle.width, rectangle.height }); };
			for (const auto& cue : cues)
			{
				values.insert(values.end(), { cue->attached() ? 1.0f : 0.0f, cue->bead_age(),
					cue->interval_remaining(), static_cast<float>(cue->drops().size()) });
				append_rectangle(cue->bead_rectangle());
				append_rectangle(cue->bounds());
				for (const IdleDrip::Drop& drop : cue->drops())
					{ values.insert(values.end(), { drop.position.x, drop.position.y, drop.age }); }
			}
			return values;
		};
		const auto held_state = state();
		const auto unchanged = [&](const labrador::Colour& keyline)
		{
			for (std::size_t index = 0; index < types.size(); ++index)
			{
				const auto& look = presentation.idle_drip(types[index]);
				require(&look == looks[index] && look.keyline == keyline && look.enabled == original[index].enabled &&
					look.sprite_sheet == original[index].sprite_sheet && look.fill_frame == original[index].fill_frame &&
					geometry(look) == geometry(original[index]),
					"The one common keyline reaches all five stable borrowed records without changing eligibility, assets or tool geometry");
			}
			require(state() == held_state,
				"Idle-drip colour adoption retains every attached/falling age, interval, position, bead rectangle and cull bound");
		};
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const auto initial_colours = colour_values(presentation);
		std::srand(0x1d1e17);
		const int expected_random = std::rand();
		std::srand(0x1d1e17);
		for (const auto& [word, keyline] : std::array<std::pair<const char*, labrador::Colour>, 4>{ {
			{ "RED", { 255, 0, 0 } }, { "3165A7", { 49, 101, 167 } },
			{ "2060C080", { 32, 96, 192, 128 } }, { "A532D600", { 165, 50, 214, 0 } } } })
		{
			auto expected = initial_colours;
			expected[idle_drip_keyline_colour_index] = keyline;
			require(impl.execute(std::string("tune presentation.idle_drip.keyline ") + word).ok &&
				colour_values(presentation) == expected && read_text(scratch.source / L"presentation.json") == baseline &&
				read_text(scratch.staged / L"presentation.json") == baseline,
				"A preconstructed idle drip adopts straight named/RGB/partial/nonblack-zero-alpha keyline channels without writing content");
			unchanged(keyline);
		}
		const TuneDocument* const pending = impl.tune_document("presentation");
		require(impl.execute("tune presentation.debug_overlay.colour 789ABC").ok &&
			impl.execute("tune.reset presentation.idle_drip.keyline").ok && pending->edits().size() == 1 &&
			pending->edits().contains("debug_overlay.colour") && presentation.debug_overlay().colour == labrador::Colour(120, 154, 188),
			"The common idle-drip reset preserves an unrelated live colour edit");
		unchanged(original.front().keyline);
		require(impl.execute("tune.reset presentation.debug_overlay.colour").ok && pending->edits().empty() &&
			pending->current().text() == baseline && colour_values(presentation) == initial_colours,
			"Independent reset restores the exact original idle-drip document after retaining its active cues");
		const JsonSpanDocument baseline_document(baseline, "idle-drip-colour-save-reference");
		const std::string saved = baseline_document.replaced(*baseline_document.find("idle_drip.keyline"), "\"2060C080\"");
		require(impl.execute("tune presentation.idle_drip.keyline 2060C080").ok &&
			impl.execute("tune.save presentation").ok && read_text(scratch.source / L"presentation.json") == saved &&
			read_text(scratch.staged / L"presentation.json") == saved && pending->edits().empty() &&
			pending->baseline().text() == saved && pending->current().text() == saved,
			"Saving the common keyline changes one authored token and rebases all five retained tool profiles");
		unchanged(labrador::Colour(32, 96, 192, 128));
		require(impl.execute("tune presentation.idle_drip.keyline A532D600").ok,
			"An independent nonblack zero-alpha keyline is pending before invalid merged candidate saves");
		const JsonSpanDocument saved_document(saved, "idle-drip-colour-merged-reference");
		const JsonLeaf* interval_max = saved_document.find("idle_drip.tools.sniper.interval_max");
		require(interval_max != nullptr && original[1].interval_min > 0.001f,
			"The merged candidate has an unrelated real Sniper interval ordering constraint");
		const std::string external = saved_document.replaced(*interval_max, "0.001");
		write_text(scratch.source / L"presentation.json", external);
		const std::string pending_text = pending->current().text();
		const auto pending_colours = colour_values(presentation);
		for (const char* command : { "tune.save presentation", "tune.save presentation force" })
		{
			require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == saved && pending->current().text() == pending_text &&
				pending->baseline().text() == saved && pending->edits().size() == 1 &&
				pending->edits().contains("idle_drip.keyline") && colour_values(presentation) == pending_colours,
				"An invalid unrelated tool interval rejects the complete merged candidate and retains disk/staging/live/ledger, even with force");
			unchanged(labrador::Colour(165, 50, 214, 0));
		}
		require(impl.execute("tune.reset presentation.idle_drip.keyline").ok && pending->edits().empty() &&
			pending->current().text() == saved && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external && read_text(scratch.staged / L"presentation.json") == saved &&
			!std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
			!std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
			!std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
			"After rejected saves the common keyline resets to the saved baseline without discarding the external source or leaving recovery files");
		unchanged(labrador::Colour(32, 96, 192, 128));
		write_text(scratch.source / L"presentation.json", saved);
		require(std::rand() == expected_random,
			"Idle-drip colour edits, resets and valid/refused saves consume no gameplay random draw");
	}

	void check_respawn_pen_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"respawn-pen-colour");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		CpuWorld world(game_directory, tuning, presentation);
		TestDirector test(tuning, presentation);
		test.data.set_stages(&world.stages);
		test.data.set_level_infos(&world.levels);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const RespawnRedrawPresentation* const look = &presentation.respawn_redraw();
		const RespawnRedrawPresentation original = *look;
		RespawnRedraw pen(look, &world.render);
		const float middle = (look->draw_from + look->fill_at) * 0.5f;
		const std::array<float, 6> progress{ std::nextafter(look->draw_from, -1.0f),
			look->draw_from, middle, std::nextafter(look->fill_at, -1.0f),
			look->fill_at, std::nextafter(look->fill_at, 2.0f) };
		const auto queries = [&]
		{
			std::vector<bool> values;
			for (const float at : progress)
				{ values.push_back(pen.filled(at)); values.push_back(pen.drawing(at)); }
			return values;
		};
		const std::vector<bool> held_queries{ false, false, false, true, false, true,
			false, true, true, false, true, false };
		require(queries() == held_queries && pen.camera_hold() == original.camera_hold,
			"The preconstructed respawn pen pins drawing/fill queries immediately around both authored boundaries");
		// Reuse the existing real CPU Level fixture: prepare an actual dead
		// player once, then never tick it during colour commands or saves.
		MatchRequest roster;
		roster.stage = "close_quarters";
		roster.players = 2;
		const MatchPlan plan = MatchPlanner(&world.stages).resolve(nullptr, roster,
			MatchPlanner::ConnectedPads{}, [] { return 0u; });
		require(plan.ok, "The respawn pen's real CPU roster resolves");
		CpuMatch match(world, test.director);
		match.settings = plan.settings;
		match.settings.set_duration(60.0f);
		test.director.attach_match(match);
		match.build();
		const std::vector<PlayerInputData> idle(input_slots::count);
		const float dt = 1.0f / 60.0f;
		int preparation_updates = 0;
		while (preparation_updates < 1200 && match.level->state() != LevelState::active)
		{
			match.level->update(idle, dt);
			++preparation_updates;
		}
		require(match.level->state() == LevelState::active && match.level->kill_player(0),
			"Respawn pen fixture enters active play and requests an actual elimination");
		const Player& player = match.level->player(0);
		while (preparation_updates < 2400 &&
			(player.state() != PlayerState::dead || player.respawn_progress() < middle))
		{
			match.level->update(idle, dt);
			++preparation_updates;
		}
		require(player.state() == PlayerState::dead && player.respawn_timer() > 0.0f &&
			player.respawn_progress() >= middle && player.respawn_progress() < look->fill_at &&
			player.animation_state() != PlayerAnimationState::elimination && pen.drawing(player.respawn_progress()),
			"Respawn pen fixture freezes a real dead player in the outline phase after elimination");
		const auto player_state = [&]
		{
			std::vector<float> values{ player.health(), player.weapon_ammo(), player.respawn_timer(),
				player.respawn_progress(), player.velocity().x, player.velocity().y,
				static_cast<float>(player.state()), static_cast<float>(player.move_state()),
				static_cast<float>(player.animation_state()), player.action_age(),
				static_cast<float>(player.action_frame()), player.facing_right() ? 1.0f : 0.0f,
				player.drawn_whole() ? 1.0f : 0.0f, player.camera_focus().x, player.camera_focus().y };
			for (const auto& rectangle : { player.rectangle(), player.spawn_rectangle(),
				player.posed_rectangle(), player.bounds(), player.cull_bounds(1.0f) })
			{
				values.insert(values.end(), { rectangle.x, rectangle.y, rectangle.width, rectangle.height });
			}
			return values;
		};
		const auto held_player = player_state();
		const auto held_camera = player.camera();
		const MatchFingerprint held_match = MatchFingerprint::of(*match.level, preparation_updates);
		const auto unchanged = [&](const labrador::Colour& colour)
		{
			require(&presentation.respawn_redraw() == look && look->pen_colour == colour &&
				look->sprite_sheet == original.sprite_sheet && look->pen_frame == original.pen_frame &&
				look->canvas_size == original.canvas_size && look->pen_width == original.pen_width &&
				look->draw_from == original.draw_from && look->fill_at == original.fill_at &&
				look->camera_hold == original.camera_hold && look->strokes.size() == original.strokes.size() &&
				queries() == held_queries && pen.camera_hold() == original.camera_hold,
				"Respawn pen colour adoption retains the stable borrowed record, captured assets, geometry and boundary queries");
			for (std::size_t index = 0; index < original.strokes.size(); ++index)
			{
				require(look->strokes[index].part == original.strokes[index].part &&
					look->strokes[index].points == original.strokes[index].points,
					"Respawn pen colour adoption preserves every ordered stroke part and point value");
			}
			require(player_state() == held_player && player.camera() == held_camera && match.builds == 1 &&
				!impl.can_apply() && MatchFingerprint::difference(held_match,
					MatchFingerprint::of(*match.level, preparation_updates)).empty(),
				"Respawn pen commands preserve the frozen real player's timers, state, camera, figure/cull bounds and match fingerprint without queueing a rebuild");
		};
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const auto initial_colours = colour_values(presentation);
		std::srand(0x2e7a21);
		const int expected_random = std::rand();
		std::srand(0x2e7a21);
		for (const auto& [word, colour] : std::array<std::pair<const char*, labrador::Colour>, 4>{ {
			{ "RED", { 255, 0, 0 } }, { "3165A7", { 49, 101, 167 } },
			{ "2060C080", { 32, 96, 192, 128 } }, { "A532D600", { 165, 50, 214, 0 } } } })
		{
			auto expected = initial_colours;
			expected[respawn_pen_colour_index] = colour;
			require(impl.execute(std::string("tune presentation.respawn_redraw.pen_colour ") + word).ok &&
				colour_values(presentation) == expected && read_text(scratch.source / L"presentation.json") == baseline &&
				read_text(scratch.staged / L"presentation.json") == baseline,
				"A retained respawn pen accepts straight named/RGB/partial/nonblack-zero-alpha channels without writing content");
			unchanged(colour);
		}
		const TuneDocument* const pending = impl.tune_document("presentation");
		require(impl.execute("tune presentation.debug_overlay.colour 789ABC").ok &&
			impl.execute("tune.reset presentation.respawn_redraw.pen_colour").ok && pending->edits().size() == 1 &&
			pending->edits().contains("debug_overlay.colour") && presentation.debug_overlay().colour == labrador::Colour(120, 154, 188),
			"Resetting respawn pen colour retains an unrelated live colour edit");
		unchanged(original.pen_colour);
		require(impl.execute("tune.reset presentation.debug_overlay.colour").ok && pending->edits().empty() &&
			pending->current().text() == baseline && colour_values(presentation) == initial_colours,
			"Independent reset restores the exact original pen document while its actual dead player remains frozen");
		const JsonSpanDocument baseline_document(baseline, "respawn-pen-colour-save-reference");
		const std::string saved = baseline_document.replaced(*baseline_document.find("respawn_redraw.pen_colour"), "\"2060C080\"");
		require(impl.execute("tune presentation.respawn_redraw.pen_colour 2060C080").ok &&
			impl.execute("tune.save presentation").ok && read_text(scratch.source / L"presentation.json") == saved &&
			read_text(scratch.staged / L"presentation.json") == saved && pending->edits().empty() &&
			pending->baseline().text() == saved && pending->current().text() == saved,
			"Respawn pen save changes only its one authored token and rebases the retained consumer");
		unchanged(labrador::Colour(32, 96, 192, 128));
		require(impl.execute("tune presentation.respawn_redraw.pen_colour A532D600").ok,
			"A nonblack zero-alpha pen edit is pending before complete merged candidate refusal");
		const JsonSpanDocument saved_document(saved, "respawn-pen-colour-merged-reference");
		const JsonLeaf* draw_from = saved_document.find("respawn_redraw.draw_from");
		const JsonLeaf* fill_at = saved_document.find("respawn_redraw.fill_at");
		require(draw_from != nullptr && fill_at != nullptr && original.draw_from < original.fill_at,
			"The pen merged candidate has the actual unrelated draw/fill ordering constraint");
		const std::string external = saved_document.replaced(*draw_from, saved_document.token(*fill_at));
		write_text(scratch.source / L"presentation.json", external);
		const std::string pending_text = pending->current().text();
		const auto pending_colours = colour_values(presentation);
		for (const char* command : { "tune.save presentation", "tune.save presentation force" })
		{
			require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == saved && pending->current().text() == pending_text &&
				pending->baseline().text() == saved && pending->edits().size() == 1 &&
				pending->edits().contains("respawn_redraw.pen_colour") && colour_values(presentation) == pending_colours,
				"draw_from equal to fill_at rejects the complete merged pen save and retains disk/staging/live/ledger even with force");
			unchanged(labrador::Colour(165, 50, 214, 0));
		}
		require(impl.execute("tune.reset presentation.respawn_redraw.pen_colour").ok && pending->edits().empty() &&
			pending->current().text() == saved && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external && read_text(scratch.staged / L"presentation.json") == saved &&
			!std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
			!std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
			!std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
			"After refused saves the pen resets to its saved baseline without overwriting external source or leaving recovery files");
		unchanged(labrador::Colour(32, 96, 192, 128));
		write_text(scratch.source / L"presentation.json", saved);
		require(std::rand() == expected_random,
			"Respawn pen commands, resets and valid/refused saves consume no gameplay random draw");
		test.director.detach_match(match);
	}

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

	void check_desktop_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"desktop-colour-records");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_content_directories(scratch.source, scratch.staged);
		const OpeningPresentation* const opening = &presentation.opening();
		const OpeningPresentation original = *opening;
		labrador::RenderResources resources;
		for (const std::string& sheet_name : { opening->sprite_sheet, opening->palette_sheet })
		{
			resources.add_sprite_sheet(sheet_name, labrador::read_sprite_sheet(
				(game_directory / L"textures" / (sheet_name + ".json")).string().c_str(), labrador::TextureHandle{}));
		}
		MonitorPersona persona;
		persona.mug = true;
		persona.coffee_ring = mattmath::Vector2F(0.85f, 0.9f);
		persona.sticky_notes.push_back({ mattmath::Vector2F(0.015f, 0.12f), 0, 0.1f });
		persona.stickers.push_back({ mattmath::Vector2F(0.2f, 0.9f), 1, 0.0f });
		persona.crack = { mattmath::Vector2F(0.004f, 0.6f), mattmath::Vector2F(0.02f, 0.62f) };
		const Monitor monitor(opening, &resources, persona);
		const MonitorPersona* const captured_persona = &monitor.persona();
		const mattmath::RectangleF screen(0, 0, 1280, 720);
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
				opening->monitor_art.power_light, opening->glass_colour, opening->program_colour,
				opening->slot_outline, opening->scribble, opening->coffee, opening->crack,
				opening->reveal_art.ink, opening->reveal_art.paper, opening->reveal_art.shade,
				opening->figure_grey, opening->face, opening->sweat };
		};
		const auto rectangles = [&]
		{
			return std::array<mattmath::RectangleF, 10>{ opening->glass, opening->monitor_art.vent_area,
				opening->monitor_art.power_button, opening->monitor_art.power_indicator,
				monitor.glass(screen), monitor.picture(screen), monitor.swatch(screen, 0), monitor.swatch(screen, 1),
				monitor.empty_slot(screen), monitor.close_button(screen) };
		};
		const auto persona_state = [&]
		{
			const auto& held = monitor.persona();
			std::vector<float> values{ static_cast<float>(held.shade), held.child ? 1.0f : 0.0f,
				held.mug ? 1.0f : 0.0f, held.coffee_ring.x, held.coffee_ring.y };
			for (const auto& decorations : { held.sticky_notes, held.stickers })
			{
				values.push_back(static_cast<float>(decorations.size()));
				for (const auto& decoration : decorations)
					{ values.insert(values.end(), { decoration.at.x, decoration.at.y, static_cast<float>(decoration.colour), decoration.tilt }); }
			}
			values.push_back(static_cast<float>(held.crack.size()));
			for (const auto& point : held.crack) { values.insert(values.end(), { point.x, point.y }); }
			return values;
		};
		const auto held_geometry = geometry();
		const auto held_unselected = unselected();
		const auto held_rectangles = rectangles();
		const auto held_persona = persona_state();
		const auto unchanged = [&]
		{
			require(&presentation.opening() == opening && &monitor.persona() == captured_persona &&
				geometry() == held_geometry && unselected() == held_unselected && rectangles() == held_rectangles &&
				persona_state() == held_persona && opening->bezel_shades == original.bezel_shades &&
				opening->sticky_notes == original.sticky_notes && opening->stickers == original.stickers &&
				opening->sprite_sheet == original.sprite_sheet && opening->fill_frame == original.fill_frame &&
				opening->percent_font == original.percent_font && opening->palette_sheet == original.palette_sheet &&
				opening->palette_frames == original.palette_frames,
				"Desktop adoption retains the borrowed opening, captured persona, public monitor/swatch geometry, all unselected material/numeric records and resource names");
		};
		// The real retained Monitor is constructed without GPU resources or RNG.
		// This CPU check observes records and public geometry, not rendered pixels
		// or protected captured frame handles. Native acceptance covers the draw.
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const auto initial_colours = colour_values(presentation);
		std::srand(0xde5c32);
		const int expected_random = std::rand();
		std::srand(0xde5c32);
		for (const auto& [word, expected] : std::array<std::pair<const char*, labrador::Colour>, 4>{ {
			{ "RED", { 255, 0, 0 } }, { "3165A7", { 49, 101, 167 } },
			{ "2060C080", { 32, 96, 192, 128 } }, { "A532D600", { 165, 50, 214, 0 } } } })
		{
			auto expected_colours = initial_colours;
			expected_colours[desktop_colour_index] = expected;
			require(impl.execute(std::string("tune presentation.opening.desktop_colour ") + word).ok &&
				colour_values(presentation) == expected_colours && read_text(scratch.source / L"presentation.json") == baseline &&
				read_text(scratch.staged / L"presentation.json") == baseline,
				"Desktop accepts straight named/RGB/partial/nonblack-zero-alpha channels while retaining every unrelated colour and both content copies");
			unchanged();
		}
		const TuneDocument* const pending = impl.tune_document("presentation");
		require(impl.execute("tune presentation.debug_overlay.colour 789ABC").ok &&
			impl.execute("tune.reset presentation.opening.desktop_colour").ok && opening->desktop_colour == original.desktop_colour &&
			presentation.debug_overlay().colour == labrador::Colour(120, 154, 188) && pending->edits().size() == 1 &&
			pending->edits().contains("debug_overlay.colour"),
			"Desktop reset restores its original tint while retaining an unrelated live colour edit");
		unchanged();
		require(impl.execute("tune.reset presentation.debug_overlay.colour").ok && colour_values(presentation) == initial_colours &&
			pending->edits().empty() && pending->current().text() == baseline,
			"Independent reset restores the exact initial document after retaining the constructed Monitor");
		const JsonSpanDocument baseline_document(baseline, "desktop-colour-save-reference");
		const std::string saved = baseline_document.replaced(*baseline_document.find("opening.desktop_colour"), "\"2060C080\"");
		require(impl.execute("tune presentation.opening.desktop_colour 2060C080").ok && impl.execute("tune.save presentation").ok &&
			opening->desktop_colour == labrador::Colour(32, 96, 192, 128) && read_text(scratch.source / L"presentation.json") == saved &&
			read_text(scratch.staged / L"presentation.json") == saved && pending->edits().empty() &&
			pending->baseline().text() == saved && pending->current().text() == saved,
			"Desktop save changes exactly one quoted token and rebases the retained borrowed record");
		unchanged();
		require(impl.execute("tune presentation.opening.desktop_colour A532D600").ok,
			"A fresh nonblack alpha0 desktop edit is pending before invalid merged geometry");
		const JsonSpanDocument saved_document(saved, "desktop-colour-merged-reference");
		const JsonLeaf* glass_width = saved_document.find("opening.glass.width");
		require(glass_width != nullptr && original.glass.x == 0.1f && original.glass.x + 0.95f > 1.0f,
			"Desktop merge fixture has the real cross-field glass-on-screen constraint");
		const std::string external = saved_document.replaced(*glass_width, "0.95");
		write_text(scratch.source / L"presentation.json", external);
		const std::string pending_text = pending->current().text();
		const auto pending_colours = colour_values(presentation);
		for (const char* command : { "tune.save presentation", "tune.save presentation force" })
		{
			require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == saved && pending->current().text() == pending_text &&
				pending->baseline().text() == saved && pending->edits().size() == 1 &&
				pending->edits().contains("opening.desktop_colour") && colour_values(presentation) == pending_colours,
				"External glass.width 0.95 with x 0.1 rejects the complete merged candidate and retains exact source/staged/live/current/baseline/ledger, even with force");
			unchanged();
		}
		require(impl.execute("tune.reset presentation.opening.desktop_colour").ok &&
			opening->desktop_colour == labrador::Colour(32, 96, 192, 128) && pending->edits().empty() &&
			pending->current().text() == saved && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external && read_text(scratch.staged / L"presentation.json") == saved &&
			!std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
			!std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
			!std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
			"Desktop resets to its saved partial-alpha baseline without discarding the external source or leaving recovery files");
		unchanged();
		write_text(scratch.source / L"presentation.json", saved);
		require(std::rand() == expected_random,
			"Desktop edits, independent resets and valid/refused saves consume no gameplay random draw");
	}

	void check_power_light_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"power-light-colour-records");
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
		MonitorPersona persona;
		persona.mug = true;
		persona.coffee_ring = mattmath::Vector2F(0.85f, 0.9f);
		persona.sticky_notes.push_back({ mattmath::Vector2F(0.015f, 0.12f), 0, 0.1f });
		persona.stickers.push_back({ mattmath::Vector2F(0.2f, 0.9f), 1, 0.0f });
		persona.crack = { mattmath::Vector2F(0.004f, 0.6f), mattmath::Vector2F(0.02f, 0.62f) };
		const Monitor monitor(opening, &resources, persona);
		const MonitorPersona* const captured_persona = &monitor.persona();
		const mattmath::RectangleF screen(0, 0, 1280, 720);
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
				opening->desktop_colour, opening->glass_colour, opening->program_colour,
				opening->slot_outline, opening->scribble, opening->coffee, opening->crack,
				opening->reveal_art.ink, opening->reveal_art.paper, opening->reveal_art.shade,
				opening->figure_grey, opening->face, opening->sweat };
		};
		const auto rectangles = [&]
		{
			return std::array<mattmath::RectangleF, 10>{ opening->glass, opening->monitor_art.vent_area,
				opening->monitor_art.power_button, opening->monitor_art.power_indicator,
				monitor.glass(screen), monitor.picture(screen), monitor.swatch(screen, 0), monitor.swatch(screen, 1),
				monitor.empty_slot(screen), monitor.close_button(screen) };
		};
		const auto persona_state = [&]
		{
			const auto& held = monitor.persona();
			std::vector<float> values{ static_cast<float>(held.shade), held.child ? 1.0f : 0.0f,
				held.mug ? 1.0f : 0.0f, held.coffee_ring.x, held.coffee_ring.y };
			for (const auto& decorations : { held.sticky_notes, held.stickers })
			{
				values.push_back(static_cast<float>(decorations.size()));
				for (const auto& decoration : decorations)
					{ values.insert(values.end(), { decoration.at.x, decoration.at.y, static_cast<float>(decoration.colour), decoration.tilt }); }
			}
			values.push_back(static_cast<float>(held.crack.size()));
			for (const auto& point : held.crack) { values.insert(values.end(), { point.x, point.y }); }
			return values;
		};
		const auto held_geometry = geometry();
		const auto held_unselected = unselected();
		const auto held_rectangles = rectangles();
		const auto held_persona = persona_state();
		const auto unchanged = [&]
		{
			require(&presentation.opening() == opening && &opening->monitor_art == monitor_art && &monitor.persona() == captured_persona &&
				geometry() == held_geometry && unselected() == held_unselected && rectangles() == held_rectangles &&
				persona_state() == held_persona && opening->bezel_shades == original.bezel_shades &&
				opening->sticky_notes == original.sticky_notes && opening->stickers == original.stickers &&
				opening->sprite_sheet == original.sprite_sheet && opening->fill_frame == original.fill_frame &&
				opening->percent_font == original.percent_font && opening->palette_sheet == original.palette_sheet &&
				opening->palette_frames == original.palette_frames,
				"Power-light adoption retains the borrowed opening/monitor-art addresses, captured persona, public monitor/swatch geometry, all unselected material/numeric records and resource names");
		};
		// The real retained Monitor is constructed without GPU resources or RNG.
		// This CPU check observes records and public geometry, not rendered pixels
		// or protected captured frame handles. Native acceptance covers the draw.
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const auto initial_colours = colour_values(presentation);
		std::srand(0x90a133);
		const int expected_random = std::rand();
		std::srand(0x90a133);
		for (const auto& [word, expected] : std::array<std::pair<const char*, labrador::Colour>, 4>{ {
			{ "RED", { 255, 0, 0 } }, { "3165A7", { 49, 101, 167 } },
			{ "2060C080", { 32, 96, 192, 128 } }, { "A532D600", { 165, 50, 214, 0 } } } })
		{
			auto expected_colours = initial_colours;
			expected_colours[power_light_colour_index] = expected;
			require(impl.execute(std::string("tune presentation.opening.monitor_art.power_light ") + word).ok &&
				colour_values(presentation) == expected_colours && read_text(scratch.source / L"presentation.json") == baseline &&
				read_text(scratch.staged / L"presentation.json") == baseline,
				"Power-light accepts straight named/RGB/partial/nonblack-zero-alpha channels while retaining every unrelated colour and both content copies");
			unchanged();
		}
		const TuneDocument* const pending = impl.tune_document("presentation");
		require(impl.execute("tune presentation.debug_overlay.colour 789ABC").ok &&
			impl.execute("tune.reset presentation.opening.monitor_art.power_light").ok && opening->monitor_art.power_light == original.monitor_art.power_light &&
			presentation.debug_overlay().colour == labrador::Colour(120, 154, 188) && pending->edits().size() == 1 &&
			pending->edits().contains("debug_overlay.colour"),
			"Power-light reset restores its original tint while retaining an unrelated live colour edit");
		unchanged();
		require(impl.execute("tune.reset presentation.debug_overlay.colour").ok && colour_values(presentation) == initial_colours &&
			pending->edits().empty() && pending->current().text() == baseline,
			"Independent reset restores the exact initial document after retaining the constructed Monitor");
		const JsonSpanDocument baseline_document(baseline, "power-light-colour-save-reference");
		const std::string saved = baseline_document.replaced(*baseline_document.find("opening.monitor_art.power_light"), "\"2060C080\"");
		require(impl.execute("tune presentation.opening.monitor_art.power_light 2060C080").ok && impl.execute("tune.save presentation").ok &&
			opening->monitor_art.power_light == labrador::Colour(32, 96, 192, 128) && read_text(scratch.source / L"presentation.json") == saved &&
			read_text(scratch.staged / L"presentation.json") == saved && pending->edits().empty() &&
			pending->baseline().text() == saved && pending->current().text() == saved,
			"Power-light save changes exactly one quoted token and rebases the retained borrowed record");
		unchanged();
		require(impl.execute("tune presentation.opening.monitor_art.power_light A532D600").ok,
			"A fresh nonblack alpha0 power-light edit is pending before invalid merged geometry");
		const JsonSpanDocument saved_document(saved, "power-light-colour-merged-reference");
		const JsonLeaf* shadow_width = saved_document.find("opening.monitor_art.shadow_width");
		require(shadow_width != nullptr && original.monitor_art.shadow_width > 0.0f && original.monitor_art.shadow_width <= 0.1f,
			"Power-light merge fixture starts inside the real monitor shadow-width bound");
		const std::string external = saved_document.replaced(*shadow_width, "0.101");
		write_text(scratch.source / L"presentation.json", external);
		const std::string pending_text = pending->current().text();
		const auto pending_colours = colour_values(presentation);
		const TuneEdit held_edit = pending->edits().at("opening.monitor_art.power_light");
		for (const char* command : { "tune.save presentation", "tune.save presentation force" })
		{
			require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == saved && pending->current().text() == pending_text &&
				pending->baseline().text() == saved && pending->edits().size() == 1 &&
				pending->edits().contains("opening.monitor_art.power_light") &&
				pending->edits().at("opening.monitor_art.power_light").baseline_token == held_edit.baseline_token &&
				pending->edits().at("opening.monitor_art.power_light").current_token == held_edit.current_token &&
				colour_values(presentation) == pending_colours,
				"External monitor_art.shadow_width 0.101 exceeds 0.1; complete merged rejection retains exact source/staged/live/current/baseline/ledger, even with force");
			unchanged();
		}
		require(impl.execute("tune.reset presentation.opening.monitor_art.power_light").ok &&
			opening->monitor_art.power_light == labrador::Colour(32, 96, 192, 128) && pending->edits().empty() &&
			pending->current().text() == saved && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external && read_text(scratch.staged / L"presentation.json") == saved &&
			!std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
			!std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
			!std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
			"Power-light resets to its saved partial-alpha baseline without discarding the external source or leaving recovery files");
		unchanged();
		write_text(scratch.source / L"presentation.json", saved);
		require(std::rand() == expected_random,
			"Power-light edits, independent resets and valid/refused saves consume no gameplay random draw");
	}

	void check_program_colour_tunes(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"program-colour-records");
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
		MonitorPersona persona;
		persona.mug = true;
		persona.coffee_ring = mattmath::Vector2F(0.85f, 0.9f);
		persona.sticky_notes.push_back({ mattmath::Vector2F(0.015f, 0.12f), 0, 0.1f });
		persona.stickers.push_back({ mattmath::Vector2F(0.2f, 0.9f), 1, 0.0f });
		persona.crack = { mattmath::Vector2F(0.004f, 0.6f), mattmath::Vector2F(0.02f, 0.62f) };
		const Monitor monitor(opening, &resources, persona);
		const MonitorPersona* const captured_persona = &monitor.persona();
		const mattmath::RectangleF screen(0, 0, 1280, 720);
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
				opening->desktop_colour, opening->glass_colour, opening->monitor_art.power_light,
				opening->slot_outline, opening->scribble, opening->coffee, opening->crack,
				opening->reveal_art.ink, opening->reveal_art.paper, opening->reveal_art.shade,
				opening->figure_grey, opening->face, opening->sweat };
		};
		const auto rectangles = [&]
		{
			return std::array<mattmath::RectangleF, 10>{ opening->glass, opening->monitor_art.vent_area,
				opening->monitor_art.power_button, opening->monitor_art.power_indicator,
				monitor.glass(screen), monitor.picture(screen), monitor.swatch(screen, 0), monitor.swatch(screen, 1),
				monitor.empty_slot(screen), monitor.close_button(screen) };
		};
		const auto persona_state = [&]
		{
			const auto& held = monitor.persona();
			std::vector<float> values{ static_cast<float>(held.shade), held.child ? 1.0f : 0.0f,
				held.mug ? 1.0f : 0.0f, held.coffee_ring.x, held.coffee_ring.y };
			for (const auto& decorations : { held.sticky_notes, held.stickers })
			{
				values.push_back(static_cast<float>(decorations.size()));
				for (const auto& decoration : decorations)
					{ values.insert(values.end(), { decoration.at.x, decoration.at.y, static_cast<float>(decoration.colour), decoration.tilt }); }
			}
			values.push_back(static_cast<float>(held.crack.size()));
			for (const auto& point : held.crack) { values.insert(values.end(), { point.x, point.y }); }
			return values;
		};
		const auto held_geometry = geometry();
		const auto held_unselected = unselected();
		const auto held_rectangles = rectangles();
		const auto held_persona = persona_state();
		const auto unchanged = [&]
		{
			require(&presentation.opening() == opening && &opening->monitor_art == monitor_art && &monitor.persona() == captured_persona &&
				geometry() == held_geometry && unselected() == held_unselected && rectangles() == held_rectangles &&
				persona_state() == held_persona && opening->bezel_shades == original.bezel_shades &&
				opening->sticky_notes == original.sticky_notes && opening->stickers == original.stickers &&
				opening->sprite_sheet == original.sprite_sheet && opening->fill_frame == original.fill_frame &&
				opening->percent_font == original.percent_font && opening->palette_sheet == original.palette_sheet &&
				opening->palette_frames == original.palette_frames,
				"Program adoption retains the borrowed opening/monitor-art addresses, captured persona, public monitor/swatch geometry, all unselected material/numeric records and resource names");
		};
		// The real retained Monitor is constructed without GPU resources or RNG.
		// This CPU check observes records and public geometry, not rendered pixels
		// or protected captured frame handles. Native acceptance covers the draw.
		const std::string baseline = read_text(scratch.source / L"presentation.json");
		const auto initial_colours = colour_values(presentation);
		std::srand(0x90a134);
		const int expected_random = std::rand();
		std::srand(0x90a134);
		for (const auto& [word, expected] : std::array<std::pair<const char*, labrador::Colour>, 4>{ {
			{ "RED", { 255, 0, 0 } }, { "3165A7", { 49, 101, 167 } },
			{ "2060C080", { 32, 96, 192, 128 } }, { "A532D600", { 165, 50, 214, 0 } } } })
		{
			auto expected_colours = initial_colours;
			expected_colours[program_colour_index] = expected;
			require(impl.execute(std::string("tune presentation.opening.program_colour ") + word).ok &&
				colour_values(presentation) == expected_colours && read_text(scratch.source / L"presentation.json") == baseline &&
				read_text(scratch.staged / L"presentation.json") == baseline,
				"Program accepts straight named/RGB/partial/nonblack-zero-alpha channels while retaining every unrelated colour and both content copies");
			unchanged();
		}
		const TuneDocument* const pending = impl.tune_document("presentation");
		require(impl.execute("tune presentation.debug_overlay.colour 789ABC").ok &&
			impl.execute("tune.reset presentation.opening.program_colour").ok && opening->program_colour == original.program_colour &&
			presentation.debug_overlay().colour == labrador::Colour(120, 154, 188) && pending->edits().size() == 1 &&
			pending->edits().contains("debug_overlay.colour"),
			"Program reset restores its original tint while retaining an unrelated live colour edit");
		unchanged();
		require(impl.execute("tune.reset presentation.debug_overlay.colour").ok && colour_values(presentation) == initial_colours &&
			pending->edits().empty() && pending->current().text() == baseline,
			"Independent reset restores the exact initial document after retaining the constructed Monitor");
		const JsonSpanDocument baseline_document(baseline, "program-colour-save-reference");
		const std::string saved = baseline_document.replaced(*baseline_document.find("opening.program_colour"), "\"2060C080\"");
		require(impl.execute("tune presentation.opening.program_colour 2060C080").ok && impl.execute("tune.save presentation").ok &&
			opening->program_colour == labrador::Colour(32, 96, 192, 128) && read_text(scratch.source / L"presentation.json") == saved &&
			read_text(scratch.staged / L"presentation.json") == saved && pending->edits().empty() &&
			pending->baseline().text() == saved && pending->current().text() == saved,
			"Program save changes exactly one quoted token and rebases the retained borrowed record");
		unchanged();
		require(impl.execute("tune presentation.opening.program_colour A532D600").ok,
			"A fresh nonblack alpha0 program edit is pending before invalid merged geometry");
		const JsonSpanDocument saved_document(saved, "program-colour-merged-reference");
		const JsonLeaf* palette_height = saved_document.find("opening.palette_height");
		require(palette_height != nullptr && original.palette_height > 0.0f,
			"Program merge fixture starts inside the real positive palette-height constraint");
		const std::string external = saved_document.replaced(*palette_height, "0");
		write_text(scratch.source / L"presentation.json", external);
		const std::string pending_text = pending->current().text();
		const auto pending_colours = colour_values(presentation);
		const TuneEdit held_edit = pending->edits().at("opening.program_colour");
		for (const char* command : { "tune.save presentation", "tune.save presentation force" })
		{
			require(!impl.execute(command).ok && read_text(scratch.source / L"presentation.json") == external &&
				read_text(scratch.staged / L"presentation.json") == saved && pending->current().text() == pending_text &&
				pending->baseline().text() == saved && pending->edits().size() == 1 &&
				pending->edits().contains("opening.program_colour") &&
				pending->edits().at("opening.program_colour").baseline_token == held_edit.baseline_token &&
				pending->edits().at("opening.program_colour").current_token == held_edit.current_token &&
				colour_values(presentation) == pending_colours,
				"External palette_height 0 fails the positive reader constraint; complete merged rejection retains exact source/staged/live/current/baseline/ledger, even with force");
			unchanged();
		}
		require(impl.execute("tune.reset presentation.opening.program_colour").ok &&
			opening->program_colour == labrador::Colour(32, 96, 192, 128) && pending->edits().empty() &&
			pending->current().text() == saved && pending->baseline().text() == saved &&
			read_text(scratch.source / L"presentation.json") == external && read_text(scratch.staged / L"presentation.json") == saved &&
			!std::filesystem::exists(scratch.source / L"presentation.json.tmp") &&
			!std::filesystem::exists(scratch.source / L"presentation.json.bak") &&
			!std::filesystem::exists(scratch.staged / L"presentation.json.tmp"),
			"Program resets to its saved partial-alpha baseline without discarding the external source or leaving recovery files");
		unchanged();
		write_text(scratch.source / L"presentation.json", saved);
		require(std::rand() == expected_random,
			"Program edits, independent resets and valid/refused saves consume no gameplay random draw");
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
		check_blocked_colour_tunes(game_directory);
		check_idle_drip_colour_tunes(game_directory);
		check_respawn_pen_colour_tunes(game_directory);
		check_cursor_colour_tunes(game_directory);
		check_silhouette_colour_tunes(game_directory);
		check_reveal_colour_tunes(game_directory);
		check_cursor_material_colour_tunes(game_directory);
		check_desktop_colour_tunes(game_directory);
		check_power_light_colour_tunes(game_directory);
		check_program_colour_tunes(game_directory);
		check_acting_colour_tunes(game_directory);
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
