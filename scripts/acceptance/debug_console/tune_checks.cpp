#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_menu_model.h"
#include "game/debug/json_spans.h"
#include "game/debug/tune_document.h"
#include "game/debug/tune_scope.h"
#include "engine/math/scalar.h"
#include <algorithm>
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
		require(paths == std::vector<std::string>{ "a[0]=1", "a[1][0]=2", "a[1][1]=true", "b=-1.5e3",
			"o.x=false" }, "Nested arrays get [i] paths, exponents and signs are one token, and a "
			"repeated key resolves to the first; got" + joined);

		const std::string spliced = document.replaced(*document.find("b"), "2");
		require(count_of(spliced, "\r\n") == count_of(synthetic, "\r\n") &&
			spliced.find("q\\\"\xc3\xa9") != std::string::npos,
			"CRLF and escapes survive a splice");
		for (const char* bad : { "{\"a\": 1,}", "{ // c\n \"a\": 1 }", "{\"a\": \"x }", "{\"a\": 1}\n2" })
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
			{ "presentation.respawn_redraw.strokes[3].points[1].x", TuneScope::live },
			{ "presentation.opening.glass.width", TuneScope::live },
			{ "tuning.match.opening.leave", TuneScope::live },
			{ "tuning.match.reveal.decide", TuneScope::live },
			{ "presentation.camera.reference_height", TuneScope::live },
			{ "presentation.match.countdown_center_y", TuneScope::rebuild },
			{ "presentation.main_menu.title_y", TuneScope::next_screen },
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
		for (const char* name : { "tuning", "presentation" })
		{
			const std::string text = read_text(game_directory / (std::string(name) + ".json"));
			const JsonSpanDocument document(text, name);
			const std::filesystem::path candidate = directory / (std::string(name) + ".json");
			for (const JsonLeaf& leaf : document.leaves())
			{
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
		}
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
	}
}
#endif
