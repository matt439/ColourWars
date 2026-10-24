#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_menu_model.h"
#include "game/debug/debug_tokenizer.h"
#include "game/debug/json_spans.h"
#include "game/debug/tune_document.h"
#include "game/debug/tune_scope.h"
#include "engine/assets/json.h"
#include "engine/math/scalar.h"
#include <algorithm>
#include <array>
#include <cmath>

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

#include "scripts/acceptance/debug_console/tune_colour_document_checks.h"
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
					tune_colour_path(path), "Only the independently audited fifty-one colours appear as string tune paths: " + path);
			}
		}
		require(colour_count == colour_paths.size(), "Exactly fifty-one authored colour strings are tunable");
		for (std::size_t index = 0; index < colour_paths.size(); ++index)
		{
			const std::string path = colour_paths[index];
			require(tune_colour_path(path) && tune_scope_of(path) ==
				(index == 6 || index == stage_preview_background_colour_index ? TuneScope::next_screen : TuneScope::live),
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
			"opening.silhouette_art.ink", "tuning.opening.silhouette_art.ink", "presentation.opening.silhouette_extra",
			"presentation.opening.silhouette_art.ink.r", "presentation.opening.silhouette_art.rim_extra",
			"presentation.opening.silhouette_art.shade_colour", "presentation.opening.silhouette_art.pen_width",
			"opening.reveal_art.ink", "tuning.opening.reveal_art.ink", "opening.reveal_art.paper",
			"tuning.opening.reveal_art.paper", "opening.reveal_art.shade", "tuning.opening.reveal_art.shade",
			"presentation.opening.percent_font", "presentation.opening.reveal_art.ink.r",
			"presentation.opening.reveal_art.paper_extra", "presentation.opening.reveal_art.shade_colour",
			"presentation.opening.reveal_art.padding", "presentation.opening.reveal_art.beat_duration",
			"opening.desktop_colour", "tuning.opening.desktop_colour", "presentation.opening.desktop_colour.r",
			"presentation.opening.desktop_colour_extra", "opening.monitor_art.power_light",
			"tuning.opening.monitor_art.power_light", "presentation.opening.monitor_art.power_light.r",
			"presentation.opening.monitor_art.power_light_extra", "presentation.opening.monitor_art.ink",
			"presentation.opening.monitor_art.highlight", "opening.program_colour", "tuning.opening.program_colour",
			"presentation.opening.program_colour.r", "presentation.opening.program_colour_extra",
			"opening.acting.face", "tuning.opening.acting.face", "presentation.opening.acting.face.r",
			"presentation.opening.acting.face_extra", "opening.acting.sweat", "tuning.opening.acting.sweat",
			"presentation.opening.acting.sweat.r", "presentation.opening.acting.sweat_extra",
			"opening.glass_colour", "tuning.opening.glass_colour", "presentation.opening.glass_colour.r",
			"presentation.opening.glass_colour_extra", "opening.silhouette", "tuning.opening.silhouette",
			"presentation.opening.silhouette.r", "opening.scribble", "tuning.opening.scribble",
			"presentation.opening.scribble.r", "presentation.opening.scribble_extra",
			"opening.crack", "tuning.opening.crack", "presentation.opening.crack.r", "presentation.opening.crack_extra",
            "figure.arm_colour", "tuning.figure.arm_colour", "presentation.figure.arm_colour.r", "presentation.figure.arm_colour_extra",
            "figure.hand_colour", "tuning.figure.hand_colour", "presentation.figure.hand_colour.r", "presentation.figure.hand_colour_extra",
            "presentation.figure.sprite_sheet", "presentation.figure.rest_arms_frame", "presentation.figure.rest_arms_paint_frame",
            "presentation.figure.sleeves_frame", "presentation.figure.sleeves_paint_frame", "figure.quiet.face_colour", "tuning.figure.quiet.face_colour", "presentation.figure.quiet.face_colour.r", "presentation.figure.quiet.face_colour_extra", "presentation.figure.quiet.face_colour.value",
			"presentation.opening.sticky_notes[0]", "presentation.opening.stickers[0]",
			"hud.health_colour", "tuning.hud.health_colour", "presentation.hud.health_colour.r",
			"presentation.hud.health_colour_extra", "hud.meter_track_colour", "tuning.hud.meter_track_colour",
			"presentation.hud.meter_track_colour.r", "presentation.hud.meter_track_colour_extra",
			"presentation.hud.sprite_sheet", "presentation.hud.fill_frame", "presentation.hud.meter_label_font",
			"hud.timer.colour", "tuning.hud.timer.colour", "presentation.hud.timer.colour.r", "presentation.hud.timer.colour_extra", "presentation.hud.timer.colour.value", "presentation.hud.timer_colour", "hud.timer.shadow_colour", "tuning.hud.timer.shadow_colour", "presentation.hud.timer.shadow_colour.r", "presentation.hud.timer.shadow_colour_extra", "presentation.hud.timer.shadow_colour.value", "presentation.hud.timer_shadow_colour", "hud.closing_clock.text", "tuning.hud.closing_clock.text", "presentation.hud.closing_clock.text.r", "presentation.hud.closing_clock.text_extra", "presentation.hud.closing_clock.text.value", "presentation.hud.closing_clock_text", "presentation.hud.closing_clock.ink", "presentation.hud.closing_clock.paper", "presentation.hud.closing_clock.shade", "presentation.hud.closing_clock.accent", "presentation.hud.timer.font", "hud.player_identity.colour", "tuning.hud.player_identity.colour", "presentation.hud.player_identity.colour.r", "presentation.hud.player_identity.colour_extra", "presentation.hud.player_identity.colour.value", "presentation.hud.player_identity_colour", "hud.player_identity.shadow_colour", "tuning.hud.player_identity.shadow_colour", "presentation.hud.player_identity.shadow_colour.r", "presentation.hud.player_identity.shadow_colour_extra", "presentation.hud.player_identity.shadow_colour.value", "presentation.hud.player_identity_shadow_colour", "hud.player_identity.background_colour", "tuning.hud.player_identity.background_colour", "presentation.hud.player_identity.background_colour.r", "presentation.hud.player_identity.background_colour_extra", "presentation.hud.player_identity.background_colour.value", "presentation.hud.player_identity_background_colour", "presentation.hud.player_identity.font" })
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
			{ "presentation.opening.monitor_art.power_light", TuneScope::live },
			{ "presentation.opening.program_colour", TuneScope::live },
			{ "presentation.opening.acting.face", TuneScope::live },
			{ "presentation.opening.acting.sweat", TuneScope::live },
			{ "presentation.opening.glass_colour", TuneScope::live },
			{ "presentation.opening.silhouette", TuneScope::live },
			{ "presentation.opening.scribble", TuneScope::live },
			{ "presentation.hud.health_colour", TuneScope::live },
			{ "presentation.hud.meter_track_colour", TuneScope::live },
			{ "presentation.opening.crack", TuneScope::live },
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
		int checked = 0;
		std::size_t colours_checked = 0;
		for (const char* name : { "tuning", "presentation" })
		{
			const std::string text = read_text(game_directory / (std::string(name) + ".json"));
			const JsonSpanDocument document(text, name);
			const std::string source = std::string(name) + ".json";
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
				std::string message;
				try
				{
					const labrador::JsonDocument candidate = labrador::parse_json(
						document.replaced(leaf, "\"__unread__\""), source);
					if (std::string(name) == "tuning")
					{
						GameTuning loaded(candidate);
					}
					else
					{
						Presentation loaded(candidate);
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
					std::string message;
					try { Presentation loaded(labrador::parse_json(document.replaced(*leaf, "false"), source)); }
					catch (const std::exception& error) { message = error.what(); }
					require(message.find(last_key(path)) != std::string::npos,
						"The Presentation reader consumes the audited colour and names a rejected type: " + path);
					++colours_checked;
				}
			}
		}
		require(colours_checked == colour_paths.size(), "All fifty-one colour strings have real Presentation consumers");
		return checked;
	}
}
#endif
