#include "game/pch.h"
#include "game/presentation_validation.h"
#include "game/build_info.h"
#include "game/content_value.h"

#include "engine/assets/json.h"
#include "engine/render/text_encoding.h"
#include "engine/render/resolution_manager.h"
#include "game/drawable_text.h"
#include "game/input_slots.h"
#include "game/menu_geometry.h"
#include "game/player_profiles.h"
#include "game/objects/player_badge.h"
#include "game/resource_validation.h"
#include "game/controls_text.h"
#include "game/states/ui_player_badge.h"
#include "game/text_wrap.h"
#include "engine/render/sprite_geometry.h"

#include <cmath>
#include <initializer_list>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

using namespace labrador;
using namespace mattmath;
using namespace content_value;
using namespace presentation_read;
using namespace presentation_validation;

Presentation::Validation::LabelRows Presentation::Validation::validate_roster_labels(const SetupGeometry& setup, const ControlsGeometry& control_geometry,
	const RosterPapers& papers, FontHandle name_font)
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	const ControlsSettingsPresentation& controls = this->presentation_.controls_settings_;
	const PauseMenuPresentation& pause = this->presentation_.pause_menu_;
	const StickPreviewPresentation& preview = controls.stick_preview;
	const WeaponSelectArtworkPresentation& artwork = this->presentation_.main_menu_.weapon_select_art;
	const auto& setup_inside = setup.setup_inside;
	const auto& confirmation_paper = setup.confirmation_paper;
	const auto& help_area = control_geometry.help_area;
	const auto& probe_height = control_geometry.probe_height;
	const auto& team_inside = papers.team_inside;
	const auto& team_heading = papers.team_heading;
	const auto& team_warning = papers.team_warning;
	const auto& weapon_inside = papers.weapon_inside;
	const auto& weapon_heading = papers.weapon_heading;
	// Check these additional roles after the established pages so a shared font
	// or geometry error retains its earlier diagnostic wherever possible.
	for (int player = 1; player < 4; ++player)
	{
		for (const BotDifficulty difficulty : label_difficulties)
		{
			std::ignore = label_size("menu_text.labels.bot at world player identity",
				this->presentation_.menu_text_.bot_roster_label(player, difficulty), this->presentation_.hud_.player_identity_font);
		}
	}
	const MenuConnectionText& connection = this->presentation_.menu_text_.connection;
	const std::string reconnect_key = "menu_text.connection.join_reconnect_suffix";
	const RectangleF join_heading = check_setup_paper(setup_inside, "menu_text.headings.join_players",
		this->presentation_.menu_text_.heading(MenuHeading::join_players), this->presentation_.menu_.heading_font,
		this->presentation_.menu_.layout.position, this->presentation_.menu_.heading_shadow_offset, false, 0.0f);
	float join_previous_bottom = join_heading.bottom();
	std::vector<std::string> join_previous_keys{ "menu_text.headings.join_players" };
	const auto previous_join_context = [&]
	{
		std::string context;
		for (const std::string& key : join_previous_keys)
		{
			if (!context.empty()) { context += " or "; }
			context += key;
		}
		return context;
	};
	for (int player = 0; player < 4; ++player)
	{
		const Vector2F position = this->presentation_.menu_.layout.position +
			Vector2F(0.0f, main.player_count_spacing.y * static_cast<float>(player + 2));
		float row_bottom = join_previous_bottom;
		std::vector<std::string> row_bottom_keys = join_previous_keys;
		for (int slot = -1; slot < (player == 0 ? input_slots::count : label_choices); ++slot)
		{
			const std::wstring label = label_words(player, slot);
			const auto check_join_row = [&](const std::wstring& words, bool reconnect)
			{
				const std::string key = label_key(slot) + " at Join" + (reconnect ? " with " + reconnect_key : "");
				std::ignore = label_size(key, words, this->presentation_.menu_.item_font);
				RectangleF bounds = check_setup_paper(setup_inside, key, words, this->presentation_.menu_.item_font,
					position, this->presentation_.menu_.item_shadow_offset, false, 0.0f);
				// Match Join's retained acknowledgement strip beside the original
				// text bounds, without adding focus-card padding to these rows.
				const RectangleF accent(bounds.x - this->presentation_.menu_.focus_padding.x, bounds.y,
					std::max(0.0f, this->presentation_.menu_.focus_padding.x), bounds.height);
				if (player > 0 && (slot < 0 || slot >= input_slots::count))
				{
					const RectangleF cursor(bounds.x, bounds.bottom() + this->presentation_.menu_.rule_height,
						bounds.width, this->presentation_.menu_.rule_height);
					bounds = RectangleF::union_of(bounds, cursor);
				}
				bounds = RectangleF::union_of(bounds, accent);
				if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
					!std::isfinite(bounds.width) || !std::isfinite(bounds.height) || !setup_inside.contains(bounds))
				{
					throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
						" must fit every composed Join player/device row inside the setup paper including its retained shadow and acknowledgement bounds.");
				}
				if (bounds.y < join_previous_bottom)
				{
					throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
						" requires Join player rows to follow the heading and each other without overlap"
						" (preceding envelope contributions from " + previous_join_context() + ").");
				}
				if (bounds.bottom() > row_bottom)
				{
					row_bottom_keys = { key };
				}
				else if (bounds.bottom() == row_bottom &&
					std::find(row_bottom_keys.begin(), row_bottom_keys.end(), key) == row_bottom_keys.end())
				{
					row_bottom_keys.push_back(key);
				}
				row_bottom = std::max(row_bottom, bounds.bottom());
			};
			check_join_row(label, false);
			// The shared keyboard/mouse input is always connected. Only a
			// reserved pad can actually display the reconnect suffix, and only
			// a connected pad shows the profile it wears, whose name may be
			// the widest one.
			if (input_slots::is_pad(slot))
			{
				check_join_row(label + connection.join_reconnect_suffix, true);
				check_join_row(this->presentation_.menu_text_.worn_profile_label(label,
					std::string(player_profiles::max_name_length, 'W')), false);
			}
		}
		join_previous_bottom = row_bottom;
		join_previous_keys = std::move(row_bottom_keys);
	}
	const auto check_join_status = [&](const char* role, const std::wstring& words)
	{
		const std::string key = std::string("menu_text.connection.") + role;
		std::ignore = fixed_menu_size(key, words, this->presentation_.menu_.detail_font);
		const RectangleF bounds = check_setup_paper(setup_inside, key, words, this->presentation_.menu_.detail_font,
			this->presentation_.menu_.layout.position + Vector2F(0.0f, main.player_count_spacing.y * 7.0f),
			this->presentation_.menu_.detail_shadow_offset, false, 0.0f);
		if (bounds.y < join_previous_bottom)
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				" must follow all four Join player rows without overlap"
				" (preceding envelope contributions from " + previous_join_context() + ").");
		}
	};
	check_join_status("join_ready", connection.join_ready);
	check_join_status("join_waiting", connection.join_waiting);
	const auto check_pause_connection = [&](const char* role, const std::wstring& words, bool heading)
	{
		const std::string key = std::string("menu_text.connection.") + role;
		const Vector2F size = fixed_menu_size(key, words,
			heading ? this->presentation_.menu_.heading_font : this->presentation_.menu_.detail_font);
		// Pause uses plain UiText, centered on the full-design paper. It has
		// neither Main's footer centering nor retained text-shadow bounds.
		const RectangleF bounds(menu_geometry::centred_x(size.x),
			heading ? pause.heading_y : pause.status_y, size.x, size.y);
		const float bottom = heading ? pause.status_y : pause.layout.position.y - pause.art.focus_padding.y;
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) || !std::isfinite(bottom) ||
			!confirmation_paper.contains(bounds) || bounds.bottom() > bottom)
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				" must fit unscaled inside the Pause paper in heading, status, first-action order including the action's focus padding.");
		}
	};
	check_pause_connection("pause_title", connection.pause_title, true);
	for (int number = 1; number <= 4; ++number)
	{
		check_pause_connection("pause_player_title", this->presentation_.menu_text_.pause_title_for_player(number), true);
	}
	check_pause_connection("controller_lost", connection.controller_lost, true);
	check_pause_connection("all_connected", connection.all_connected, false);
	const Font& stick_font = *render.font(render.resolve_sprite_font(controls.help_font));
	const auto stick_numeric_size = [&](const char* role, const std::wstring& text)
	{
		const std::string where = this->presentation_.source_path_ + ": menu_text.stick_readouts." + role;
		drawable_text::check_font_can_draw(&render, controls.help_font, L"-0123456789", where);
		struct Pen
		{
			float x = 0.0f;
			float y = 0.0f;
			Vector2F extent = Vector2F::ZERO;
		};
		const auto walk = [&](Pen& pen, std::wstring_view words)
		{
			// Match Font's pen order, including accumulated bearings, the clamp
			// at zero, whitespace suppression and glyph overhang beyond a line.
			for (const wchar_t character : words)
			{
				if (character == L'\n')
				{
					pen.x = 0.0f;
					pen.y += stick_font.line_spacing();
					continue;
				}
				const Glyph& glyph = stick_font.drawn(static_cast<char32_t>(character));
				if (!std::isfinite(glyph.x_offset) || !std::isfinite(glyph.y_offset) ||
					!std::isfinite(glyph.x_advance))
				{
					throw std::runtime_error(where + " requires finite font glyph metrics.");
				}
				const float width = static_cast<float>(glyph.subrect.width);
				pen.x += glyph.x_offset;
				if (pen.x < 0.0f) { pen.x = 0.0f; }
				if (character != L' ' || glyph.subrect.width > 1 || glyph.subrect.height > 1)
				{
					const float height = character == L' ' ? stick_font.line_spacing() :
						std::max(stick_font.line_spacing(), static_cast<float>(glyph.subrect.height) + glyph.y_offset);
					pen.extent.x = std::max(pen.extent.x, pen.x + width);
					pen.extent.y = std::max(pen.extent.y, pen.y + height);
				}
				pen.x += width + glyph.x_advance;
			}
		};
		if (!std::isfinite(stick_font.line_spacing()) || stick_font.line_spacing() <= 0.0f)
		{
			throw std::runtime_error(where + " requires finite positive font line spacing.");
		}
		Pen pen;
		size_t begin = 0;
		for (;;)
		{
			const size_t token = text.find(L'{', begin);
			const std::wstring_view literal = std::wstring_view(text).substr(begin,
				token == std::wstring::npos ? token : token - begin);
			drawable_text::check_font_can_draw(&render, controls.help_font, literal, where);
			walk(pen, literal);
			if (token == std::wstring::npos) { break; }
			// Every incoming-pen transform is monotone. The maximum reachable
			// pen suffices for the next token, but its greatest ink extent may
			// come from a different value, so retain both maxima independently.
			// Tokens contain no LF; their line position is independent of value.
			float maximum_pen = -std::numeric_limits<float>::infinity();
			Vector2F maximum_extent = pen.extent;
			for (int value = -100; value <= 100; ++value)
			{
				Pen candidate = pen;
				walk(candidate, std::to_wstring(value));
				maximum_pen = std::max(maximum_pen, candidate.x);
				maximum_extent.x = std::max(maximum_extent.x, candidate.extent.x);
				maximum_extent.y = std::max(maximum_extent.y, candidate.extent.y);
			}
			pen.x = maximum_pen;
			pen.extent = maximum_extent;
			// The required template was checked while its source key was read.
			begin = text.find(L'}', token) + 1;
		}
		if (!std::isfinite(pen.x) || !std::isfinite(pen.y) ||
			!std::isfinite(pen.extent.x) || !std::isfinite(pen.extent.y) ||
			pen.extent.x <= 0.0f || pen.extent.y <= 0.0f)
		{
			throw std::runtime_error(where + " must retain finite positive measured text bounds for every rounded percentage.");
		}
		return pen.extent;
	};
	const auto check_stick_region = [&](const char* role, const Vector2F& size,
		const Vector2F& position, float top, float bottom)
	{
		const std::string key = std::string("menu_text.stick_readouts.") + role +
			((std::string_view(role) == "connected" || std::string_view(role) == "disconnected") ?
				" with menu_text.labels.pad" : "");
		const RectangleF bounds(position, size);
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
			!std::isfinite(bounds.width) || !std::isfinite(bounds.height) ||
			!std::isfinite(top) || !std::isfinite(bottom) || !help_area.contains(bounds) ||
			bounds.y < top || bounds.bottom() > bottom)
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				" must fit unscaled inside its controls preview/readout region without overlapping the adjacent preview or help text.");
		}
		return bounds;
	};
	const Vector2F raw_caption_position = preview.raw_movement.position() -
		Vector2F(0.0f, probe_height + controls.art.focus_padding.y);
	const Vector2F response_caption_position = preview.movement_response.position() -
		Vector2F(0.0f, probe_height + controls.art.focus_padding.y);
	const MenuText& stick_text = this->presentation_.menu_text_;
	for (const bool connected : { true, false })
	{
		const char* role = connected ? "connected" : "disconnected";
		for (int slot = 0; slot < input_slots::pad_count; ++slot)
		{
			const Vector2F size = label_size(std::string("menu_text.stick_readouts.") + role + " with menu_text.labels.pad",
				stick_text.stick_connection(slot, connected), controls.help_font);
			check_stick_region(role, size, controls.controls_position, help_area.y, raw_caption_position.y);
		}
	}
	check_stick_region("raw_movement", stick_numeric_size("raw_movement", stick_text.stick_readouts.raw_movement),
		raw_caption_position, help_area.y, preview.raw_movement.y);
	check_stick_region("movement_response", stick_numeric_size("movement_response", stick_text.stick_readouts.movement_response),
		response_caption_position, preview.raw_movement.bottom(), preview.movement_response.y);
	for (const auto& readout : { std::pair("aim_inactive", &stick_text.stick_readouts.aim_inactive),
		std::pair("aim_active", &stick_text.stick_readouts.aim_active) })
	{
		check_stick_region(readout.first, stick_numeric_size(readout.first, *readout.second),
			preview.aim_readout_position, preview.aim.y, preview.association_position.y);
		if (preview.aim_readout_position.x < preview.aim.right())
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": menu_text.stick_readouts." + readout.first +
				" must remain beside the aim preview.");
		}
	}

	// The remaining label consumers have different fonts, shadows and badge
	// geometry. Validate their complete finite outputs, retaining their existing
	// unfitted layouts and the Pad row's existing OptionRow fit.
	LabelRows team_rows;
	LabelRows weapon_rows;
	LabelRows results_rows;
	const SpriteSheet* team_sheet = render.sprite_sheet(main.team_select_sheet);
	for (int player = 0; player < 4; ++player)
	{
		const float row = static_cast<float>(player + 2);
		const Vector2F team_position = this->presentation_.menu_.layout.position + Vector2F(0.0f, main.team_select.spacing.y * row);
		const Vector2F weapon_position = this->presentation_.menu_.layout.position + Vector2F(0.0f, main.weapon_select.spacing.y * row);
		RectangleF team_icons;
		float first_team_x = std::numeric_limits<float>::infinity();
		for (size_t icon = 0; icon < main.team_select_frames.size(); ++icon)
		{
			const Vector2F position = team_position + Vector2F(main.team_select.spacing.x * 2.0f +
				main.team_select.size.x * static_cast<float>(icon), 0.0f);
			const RectangleF canvas(position, main.team_select.size);
			const SpriteFrame& frame = team_sheet->sprite_frame(team_sheet->resolve_sprite_frame(main.team_select_frames[icon]));
			const RectangleF bounds = sprite_world_bounds(canvas, frame.source_rectangle(), 0.0f, frame.origin());
			check_label_bounds("menu_text.labels.roster_row at Team Select icons", bounds, team_inside);
			team_icons = icon == 0 ? bounds : RectangleF::union_of(team_icons, bounds);
			if (icon == 0)
			{
				const SpriteFrame& selected = team_sheet->sprite_frame(team_sheet->resolve_sprite_frame(main.team_select_selected_frames[icon]));
				const RectangleF selected_bounds = sprite_world_bounds(canvas, selected.source_rectangle(), 0.0f, selected.origin());
				check_label_bounds("menu_text.labels.roster_row at Team Select selected icon", selected_bounds, team_inside);
				first_team_x = std::min(bounds.x, selected_bounds.x);
			}
		}
		const Vector2F weapon_canvas_position = weapon_position + Vector2F(main.weapon_select.spacing.x * 2.0f, 0.0f);
		RectangleF weapon_tile(weapon_canvas_position, main.weapon_select.size);
		weapon_tile.inflate(this->presentation_.menu_.icon_padding);
		weapon_tile.inflate(this->presentation_.menu_.rule_height);
		RectangleF weapon_contents = weapon_tile;
		for (size_t choice = 0; choice < artwork.choices.size(); ++choice)
		{
			const Vector2F name_position = weapon_canvas_position + Vector2F(0.0f,
				main.weapon_select.size.y + this->presentation_.menu_.icon_padding.y + this->presentation_.menu_.rule_height);
			const Vector2F name_size = render.measure_text(name_font, artwork.choices[choice].name);
			weapon_contents = RectangleF::union_of(weapon_contents,
				shadow_bounds(name_position, name_size, this->presentation_.menu_.detail_shadow_offset));
			const Vector2F description_position = weapon_canvas_position + Vector2F(main.weapon_description_x_offset, 0.0f);
			const std::wstring wrapped = text_wrap::wrap(render, name_font, main.weapon_descriptions[choice], main.weapon_description_max_width);
			const Vector2F description_size = render.measure_text(name_font, wrapped);
			weapon_contents = RectangleF::union_of(weapon_contents,
				shadow_bounds(description_position, description_size, main.weapon_description_shadow_offset));
		}
		for (int slot = 0; slot < (player == 0 ? input_slots::count : label_choices); ++slot)
		{
			const std::wstring words = label_words(player, slot);
			const std::string key = label_key(slot);
			const RectangleF team_label = shadow_bounds(team_position,
				label_size(key + " at Team Select", words, this->presentation_.menu_.detail_font), this->presentation_.menu_.item_shadow_offset);
			RectangleF team_card = RectangleF::union_of(team_label, UiPlayerBadge::before(team_label, this->presentation_.player_identity_));
			team_card = RectangleF::union_of(team_card, team_icons);
			team_card.inflate(main.setup_focus_padding);
			check_label_bounds(key + " at Team Select", team_card, team_inside);
			if (!std::isfinite(first_team_x) || team_label.right() > first_team_x)
			{
				throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
					" at Team Select must end before the first choice icon in either captured frame.");
			}
			team_rows[static_cast<size_t>(player)][static_cast<size_t>(slot)] = team_card;
			// Weapon passes no offset to UiTextDropShadow: its transparent
			// default (2,2) still contributes to bounds.
			const RectangleF weapon_label = shadow_bounds(weapon_position,
				label_size(key + " at Weapon Select", words, this->presentation_.menu_.detail_font), Vector2F(2.0f, 2.0f));
			RectangleF weapon_row = RectangleF::union_of(weapon_label, UiPlayerBadge::before(weapon_label, this->presentation_.player_identity_));
			weapon_row = RectangleF::union_of(weapon_row, weapon_contents);
			// The row's handicap sits under its label, in the label's column,
			// in the smaller tool-name font so the row keeps its height.
			for (int level = 0; level < handicaps::count; ++level)
			{
				constexpr std::array<const char*, handicaps::count> handicap_roles{
					"handicap_none", "handicap_plus_one", "handicap_plus_two" };
				const std::string handicap_key = std::string("menu_text.labels.") +
					handicap_roles[static_cast<size_t>(level)] + " at Weapon Select";
				const RectangleF handicap = shadow_bounds(Vector2F(weapon_position.x, weapon_label.bottom()),
					label_size(handicap_key, this->presentation_.menu_text_.handicap_label(level), main.weapon_description_font),
					Vector2F(2.0f, 2.0f));
				check_label_bounds(handicap_key, handicap, weapon_inside);
				if (handicap.right() > weapon_tile.x)
				{
					throw std::runtime_error(this->presentation_.source_path_ + ": " + handicap_key +
						" must end before the first weapon tile including its border.");
				}
				weapon_row = RectangleF::union_of(weapon_row, handicap);
			}
			check_label_bounds(key + " at Weapon Select", weapon_row, weapon_inside);
			if (weapon_label.right() > weapon_tile.x)
			{
				throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
					" at Weapon Select must end before the first weapon tile including its border.");
			}
			weapon_rows[static_cast<size_t>(player)][static_cast<size_t>(slot)] = weapon_row;
		}
	}
	check_roster_rows(team_rows, team_heading.bottom(), team_warning.y, "Team Select");
	check_roster_rows(weapon_rows, weapon_heading.bottom(), weapon_inside.bottom(), "Weapon Select");

	// A missing list contains distinct pad slots in roster order; keyboard
	// never goes missing. Skipping a seat or choosing one remaining pad gives
	// all 208 nonempty reachable lists, with the existing two-column joins.
	const std::string missing_key = "menu_text.labels.roster_row with menu_text.labels.pad at Pause";
	const auto missing_lists = [&](const auto& self, int player, unsigned int used,
		int count, const std::wstring& words) -> void
	{
		if (player == 4)
		{
			if (count == 0) { return; }
			const Vector2F size = label_size(missing_key, words, this->presentation_.menu_.detail_font);
			const RectangleF bounds(menu_geometry::centred_x(size.x), pause.status_y, size.x, size.y);
			check_label_bounds(missing_key, bounds, confirmation_paper);
			if (!std::isfinite(pause.layout.position.y) || !std::isfinite(pause.art.focus_padding.y) ||
				bounds.bottom() > pause.layout.position.y - pause.art.focus_padding.y)
			{
				throw std::runtime_error(this->presentation_.source_path_ + ": " + missing_key +
					" must fit before the first Pause action including its focus padding.");
			}
			return;
		}
		self(self, player + 1, used, count, words);
		for (int slot = 0; slot < input_slots::pad_count; ++slot)
		{
			const unsigned int bit = 1u << slot;
			if ((used & bit) != 0u) { continue; }
			std::wstring next = words;
			if (count > 0) { next += count % 2 == 0 ? L"\n" : L"   "; }
			next += this->presentation_.menu_text_.roster_label(player, slot);
			self(self, player + 1, used | bit, count + 1, next);
		}
	};
	missing_lists(missing_lists, 0, 0u, 0, std::wstring());


	return results_rows;
}

void Presentation::Validation::validate_contributions(const StageGeometry& stage_geometry, const ControlsGeometry& control_geometry,
	LabelRows& results_rows)
{
	const ResultsMenuPresentation& results = this->presentation_.results_menu_;
	const ControlsSettingsPresentation& controls = this->presentation_.controls_settings_;
	const auto& results_panel = stage_geometry.results_panel;
	const auto& results_fill_right = stage_geometry.results_fill_right;
	const auto& contribution_heading = stage_geometry.contribution_heading;
	const auto& controls_value_limit = control_geometry.controls_value_limit;
	const std::string results_key = "menu_text.labels.roster_row at Results";
	const Font& contribution_font = *render.font(render.resolve_sprite_font(results.contribution_font));
	std::ignore = label_size(results_key, L"0123456789.%", results.contribution_font);
	struct PercentageSuffix
	{
		std::wstring words;
		std::array<const Glyph*, 6> glyphs = {};
	};
	std::array<PercentageSuffix, 1001> percentages;
	for (size_t tenth = 0; tenth < percentages.size(); ++tenth)
	{
		PercentageSuffix& suffix = percentages[tenth];
		// These are the legal one-decimal outputs, not rounded float samples:
		// LevelEndInfo's formatter truncates std::to_wstring at one decimal.
		suffix.words = std::to_wstring(tenth / 10) + L"." + std::to_wstring(tenth % 10) + L"%";
		for (size_t character = 0; character < suffix.words.size(); ++character)
		{
			suffix.glyphs[character] = &contribution_font.drawn(static_cast<char32_t>(suffix.words[character]));
		}
	}
	for (int player = 0; player < 4; ++player)
	{
		const Vector2F position = results.contribution_position + Vector2F(0.0f,
			results.contribution_spacing * static_cast<float>(player + 1));
		for (int slot = 0; slot < (player == 0 ? input_slots::count : label_choices); ++slot)
		{
			const std::string key = label_key(slot) + " at Results";
			RectangleF envelope;
			bool first = true;
			for (const std::wstring* team : { &this->presentation_.menu_text_.results.contribution_team_a,
				&this->presentation_.menu_text_.results.contribution_team_b })
			{
				const std::string team_key = key + " with menu_text.results." +
					(team == &this->presentation_.menu_text_.results.contribution_team_a ? "contribution_team_a" : "contribution_team_b");
				const std::string where = this->presentation_.source_path_ + ": " + team_key;
				const std::wstring prefix = label_words(player, slot) + *team;
				std::ignore = label_size(team_key, prefix, results.contribution_font);
				LabelPen prefix_pen;
				append_label_text(prefix_pen, contribution_font, prefix, where);
				for (const PercentageSuffix& suffix : percentages)
				{
					// Walk each fixed prefix once, then append each cached suffix in
					// Font's sequential FLOAT order. Regrouped sums would change
					// rounding for some finite bearings at an inclusive fit edge.
					LabelPen pen = prefix_pen;
					for (size_t character = 0; character < suffix.words.size(); ++character)
					{
						append_label_glyph(pen, *suffix.glyphs[character], suffix.words[character],
							contribution_font.line_spacing(), where);
					}
					// Results explicitly uses zero shadow. Include each actual badge
					// position: shorter lines can put its top above a taller line's.
					const RectangleF text(position, pen.extent);
					const RectangleF row = RectangleF::union_of(text, UiPlayerBadge::before(text, this->presentation_.player_identity_));
					check_label_bounds(team_key, row, results_panel);
					envelope = first ? row : RectangleF::union_of(envelope, row);
					first = false;
				}
			}
			results_rows[static_cast<size_t>(player)][static_cast<size_t>(slot)] = envelope;
		}
	}
	check_roster_rows(results_rows, contribution_heading.bottom(), results.proceed_y,
		"Results with menu_text.results.contribution_team_a and menu_text.results.contribution_team_b");
	// An award sits at the right of a contribution row, right-aligned to the
	// score bar. The page leaves out one that would meet its own row's text,
	// so the rows above stay the only bound on how long a label may be.
	for (const auto& [award, key] : { std::pair{ MatchAward::paint_laid, "award_paint_laid" },
		std::pair{ MatchAward::recaptured, "award_recaptured" },
		std::pair{ MatchAward::airtime, "award_airtime" },
		std::pair{ MatchAward::splats, "award_splats" } })
	{
		const std::string award_key = std::string("menu_text.results.") + key;
		const Vector2F size = fixed_menu_size(award_key,
			this->presentation_.menu_text_.results.award_line(award), results.contribution_font);
		for (int player = 0; player < 4; ++player)
		{
			const RectangleF bounds(Vector2F(results_fill_right - size.x,
				results.contribution_position.y +
					results.contribution_spacing * static_cast<float>(player + 1)), size);
			if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
				!results_panel.contains(bounds) || bounds.bottom() > results.proceed_y)
			{
				throw std::runtime_error(this->presentation_.source_path_ + ": " + award_key +
					" must fit the Results panel at the end of every contribution row, above the prompt.");
			}
		}
	}
	for (int slot = 0; slot < input_slots::pad_count; ++slot)
	{
		const std::string key = "menu_text.labels.pad at Pad Sticks value";
		const Vector2F raw = label_size(key, this->presentation_.menu_text_.device_label(slot), controls.item_font);
		if (!std::isfinite(controls_value_limit.x) || !std::isfinite(controls_value_limit.y) ||
			controls_value_limit.x <= 0.0f || controls_value_limit.y <= 0.0f)
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key + " requires finite positive value limits.");
		}
		float scale = 1.0f;
		if (raw.x > controls_value_limit.x) { scale = controls_value_limit.x / raw.x; }
		if (raw.y * scale > controls_value_limit.y) { scale = controls_value_limit.y / raw.y; }
		const Vector2F fitted = raw * scale;
		const RectangleF bounds(controls.layout.position + Vector2F(controls.layout.spacing.x, 0.0f), fitted);
		if (!std::isfinite(scale) || scale <= 0.0f || !finite_stage_rectangle(bounds))
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key + " must retain finite positive fitted bounds.");
		}
		check_fixed_paper(key, bounds, controls.box_size, controls.art.focus_padding);
		if (fitted.y + controls.art.focus_padding.y * 2.0f > controls.layout.spacing.y ||
			bounds.right() + controls.art.focus_padding.x > controls.controls_position.x ||
			bounds.bottom() + controls.art.focus_padding.y > controls.status_position.y)
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				" must fit its row before the controls help and status including focus padding.");
		}
	}
}
