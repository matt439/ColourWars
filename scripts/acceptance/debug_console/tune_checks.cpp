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
#include "game/objects/figure_attachments.h"
#include "game/objects/idle_drip.h"
#include "game/objects/interface_gameplay.h"
#include "game/objects/monitor.h"
#include "game/objects/respawn_redraw.h"
#include "game/objects/silhouette.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/math/scalar.h"
#include "engine/render/render_resources.h"
#include "engine/render/sprite_font_file.h"
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
#include "scripts/acceptance/debug_console/tune_colour_sweep_checks.h"
#include "scripts/acceptance/debug_console/tune_feedback_colour_checks.h"
#include "scripts/acceptance/debug_console/tune_opening_art_colour_checks.h"
#include "scripts/acceptance/debug_console/tune_opening_scene_colour_checks.h"
#include "scripts/acceptance/debug_console/tune_opening_drawing_colour_checks.h"
#include "scripts/acceptance/debug_console/tune_hud_colour_checks.h"
#include "scripts/acceptance/debug_console/tune_crack_and_figure_colour_checks.h"
#include "scripts/acceptance/debug_console/tune_quiet_face_colour_checks.h"

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
		check_glass_colour_tunes(game_directory);
		check_silhouette_body_colour_tunes(game_directory);
		check_scribble_colour_tunes(game_directory);
		check_hud_health_colour_tunes(game_directory);
		check_hud_track_colour_tunes(game_directory);
		check_crack_colour_tunes(game_directory);
		check_limb_colour_tunes(game_directory);
		check_quiet_face_colour_tunes(game_directory);
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
