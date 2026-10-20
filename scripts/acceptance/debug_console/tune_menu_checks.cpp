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

// Live tuning in the menus: tune completion and the debug menu's tuning tree.
namespace debug_console_acceptance
{
	namespace
	{
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
			colour_paths[11], colour_paths[identity_ink_colour_index], colour_paths[identity_background_colour_index] })
		{
			require(registry.complete(std::string("tune ") + path).line == std::string("tune ") + path + " " &&
				registry.complete(std::string("tune ") + path + " ").candidates == colour_choices,
				std::string("The selection/feedback/identity colour completes as an exact leaf with literal colour choices: ") + path);
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
		model.back();
		model.confirm(paged_row_index(model, "player_identity"));
		const std::size_t background_row = paged_row_index(model, "background_colour");
		const std::string background_path = colour_paths[identity_background_colour_index];
		const std::string background_token = impl.tune_token(background_path);
		rows = model.rows();
		require(rows[background_row].kind == DebugMenuRow::Kind::inspect && rows[background_row].enabled &&
			rows[background_row].value.find(background_token) != std::string::npos &&
			rows[background_row].value.find("(console)") != std::string::npos,
			"Identity background has an enabled inspect row with its exact quoted token and console hint");
		const auto identity_before = colour_values(presentation);
		const std::string identity_text = impl.tune_document("presentation")->current().text();
		const std::string identity_title = model.title();
		impl.log().clear();
		model.step(background_row, 1); model.step(background_row, -1);
		require(impl.log().lines().empty() && colour_values(presentation) == identity_before &&
			impl.tune_document("presentation")->current().text() == identity_text,
			"Left/Right on identity background cannot invent numeric stepping or alter either badge colour");
		require(model.confirm(background_row) == DebugMenuModel::Result::stay && model.title() == identity_title &&
			log_contains(impl.log(), "[menu] tune " + background_path, DebugSeverity::echo) &&
			log_contains(impl.log(), background_path + " = " + background_token, DebugSeverity::info) &&
			colour_values(presentation) == identity_before &&
			impl.tune_document("presentation")->current().text() == identity_text,
			"Confirm inspects the exact background leaf without changing its sibling, pending edits or menu node");
	}
}
#endif
