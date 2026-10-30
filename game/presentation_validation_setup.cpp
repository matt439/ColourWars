#include "game/pch.h"

// Presentation validation for the setup pages: Mode, Player Count, Join, Team
// Select and Weapon Select. The phase functions in presentation_validation.cpp
// call these in diagnostic order.
#include "game/presentation_validation.h"
#include "engine/render/sprite_geometry.h"
#include "game/drawable_text.h"
#include "game/input_slots.h"
#include "game/menu_geometry.h"
#include "game/player_profiles.h"
#include "game/text_wrap.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace labrador;
using namespace mattmath;
using namespace presentation_read;

// Phase 1: the Mode and Player Count headings and rows, the Join heading
// and Mode's solo practice hint.
void Presentation::Validation::validate_setup_actions(const SetupGeometry& setup) const
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	const auto& setup_inside = setup.setup_inside;
	float setup_heading_bottom = 0.0f;
	for (const MenuHeading role : { MenuHeading::mode, MenuHeading::players })
	{
		const RectangleF heading = check_setup_paper(setup_inside,
			std::string("menu_text.headings.") + menu_heading_keys[static_cast<size_t>(role)],
			this->presentation_.menu_text_.heading(role), this->presentation_.menu_.heading_font, this->presentation_.menu_.layout.position,
			this->presentation_.menu_.heading_shadow_offset, false, 0.0f);
		setup_heading_bottom = std::max(setup_heading_bottom, heading.bottom());
	}
	check_setup_paper(setup_inside, "menu_text.headings.join_players", this->presentation_.menu_text_.heading(MenuHeading::join_players),
		this->presentation_.menu_.heading_font, this->presentation_.menu_.layout.position, this->presentation_.menu_.heading_shadow_offset, false, 0.0f);
	const auto check_setup_item = [&](MenuItem item, const Vector2F& spacing, int row, bool duration_label)
	{
		const std::string key = std::string("menu_text.items.") + menu_item_keys[static_cast<size_t>(item)];
		const std::string where = this->presentation_.source_path_ + ": " + key;
		const Vector2F shadow = this->presentation_.menu_.item_shadow_offset;
		if (!std::isfinite(spacing.y) || spacing.y <= 0.0f ||
			(duration_label && (!std::isfinite(spacing.x) || spacing.x <= 0.0f)))
		{
			throw std::runtime_error(where + " requires positive finite setup row/column spacing.");
		}
		const Vector2F position = this->presentation_.menu_.layout.position + Vector2F(0.0f, spacing.y * static_cast<float>(row));
		const std::wstring& text = this->presentation_.menu_text_.item(item);
		check_menu_line(key, text, this->presentation_.menu_.item_font, position, this->presentation_.menu_.footer_y, shadow);
		const Vector2F size = render.measure_text(render.resolve_sprite_font(this->presentation_.menu_.item_font), text);
		if (size.y + std::abs(shadow.y) > spacing.y)
		{
			throw std::runtime_error(where + " must fit its setup row including its item shadow.");
		}
		if (duration_label && size.x + std::abs(shadow.x) > spacing.x * 2.0f)
		{
			// The existing, unfitted OptionRow puts its value two columns later.
			throw std::runtime_error(where + " must end before its duration value column including its item shadow.");
		}
		const float value_x = position.x + spacing.x * 2.0f;
		if (duration_label && (!std::isfinite(value_x) ||
			value_x + main.setup_focus_padding.x + main.setup_shadow_offset >= setup_inside.right()))
		{
			throw std::runtime_error(where +
				" must leave positive duration value space inside main_menu.setup_box_size.");
		}
		const RectangleF card = check_setup_paper(setup_inside, key, text, this->presentation_.menu_.item_font, position, shadow, true, spacing.y);
		if (row == 2 && card.y < setup_heading_bottom)
		{
			throw std::runtime_error(where + " must leave its setup heading clear of the first focus card.");
		}
		return card;
	};
	// Mode rows 2-7; Match length and Series stage carry a value column.
	const MenuItem mode_items[]{ MenuItem::single_match, MenuItem::best_of_three, MenuItem::best_of_five,
		MenuItem::match_length, MenuItem::series_stage, MenuItem::back };
	float single_match_right = 0.0f;
	std::vector<RectangleF> mode_cards;
	for (size_t index = 0; index < std::size(mode_items); ++index)
	{
		const RectangleF card = check_setup_item(mode_items[index], main.mode_select_spacing, static_cast<int>(index) + 2,
			mode_items[index] == MenuItem::match_length || mode_items[index] == MenuItem::series_stage);
		if (mode_items[index] == MenuItem::single_match) { single_match_right = card.right(); }
		else { mode_cards.push_back(card); }
	}
	const MenuItem player_items[]{ MenuItem::one_player_practice, MenuItem::two_players,
		MenuItem::three_players, MenuItem::four_players, MenuItem::back };
	for (size_t index = 0; index < std::size(player_items); ++index)
	{
		check_setup_item(player_items[index], main.player_count_spacing, static_cast<int>(index) + 2, false);
	}
	// The nonfocusable hint retains the detail font and the item-shadow offset
	// that its existing widget uses; it has no action-row fit or focus padding.
	// Beside Single Match on row 2, one card padding after its complete card.
	const Vector2F hint_position(single_match_right + main.setup_focus_padding.x,
		this->presentation_.menu_.layout.position.y + main.mode_select_spacing.y * 2.0f);
	check_menu_line("menu_text.solo_practice_hint", this->presentation_.menu_text_.solo_practice_hint, this->presentation_.menu_.detail_font,
		hint_position, this->presentation_.menu_.footer_y, this->presentation_.menu_.item_shadow_offset);
	const RectangleF practice_hint = check_setup_paper(setup_inside,
		"menu_text.solo_practice_hint", this->presentation_.menu_text_.solo_practice_hint, this->presentation_.menu_.detail_font,
		hint_position, this->presentation_.menu_.item_shadow_offset, false, 0.0f);
	// Beside Single Match it must still keep clear of every lower row's card,
	// as it once had to follow Back's.
	if (std::any_of(mode_cards.begin(), mode_cards.end(),
		[&practice_hint](const RectangleF& card) { return card.intersects(practice_hint); }))
	{
		throw std::runtime_error(this->presentation_.source_path_ +
			": menu_text.solo_practice_hint must stay clear of the Mode rows' complete focus cards and shade.");
	}
}

// Phase 5: Team Select's heading and warning, and its paper for the roster
// labels.
void Presentation::Validation::validate_team_heading(RosterPapers& papers) const
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	// Preserve the existing diagnostics when a shared font or shadow also
	// invalidates an earlier page, then check Team's additional paper bounds.
	const RectangleF team_inside = menu_geometry::paper_interior(
		menu_geometry::footer_paper(main.team_box_size, this->presentation_.menu_.footer_y),
		menu_geometry::setup_art(main));
	const auto check_team_text = [&](const char* key, const std::wstring& words, const std::string& font,
		const Vector2F& position, const Vector2F& shadow)
	{
		drawable_text::check_font_can_draw(&render, font, words, this->presentation_.source_path_ + ": " + key);
		const Vector2F size = render.measure_text(render.resolve_sprite_font(font), words);
		const RectangleF bounds = RectangleF::union_of(RectangleF(position, size), RectangleF(position + shadow, size));
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
			!std::isfinite(bounds.width) || !std::isfinite(bounds.height) ||
			bounds.width <= 0.0f || bounds.height <= 0.0f || !team_inside.contains(bounds))
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				" must fit main_menu.team_box_size including its retained shadow bounds.");
		}
		return bounds;
	};
	const RectangleF team_heading = check_team_text("menu_text.headings.teams",
		this->presentation_.menu_text_.heading(MenuHeading::teams), this->presentation_.menu_.heading_font,
		this->presentation_.menu_.layout.position, this->presentation_.menu_.heading_shadow_offset);
	const RectangleF team_warning = check_team_text(
		"menu_text.connection.teams_required at main_menu.team_select.warning_position",
		this->presentation_.menu_text_.connection.teams_required, this->presentation_.menu_.detail_font,
		main.team_warning_position, this->presentation_.menu_.detail_shadow_offset);
	if (team_warning.y < team_heading.bottom())
	{
		throw std::runtime_error(this->presentation_.source_path_ +
			": menu_text.connection.teams_required at main_menu.team_select.warning_position must follow the Team Select heading.");
	}
	papers.team_inside = team_inside;
	papers.team_heading = team_heading;
	papers.team_warning = team_warning;
}

// Phase 5: Weapon Select's heading, and its paper for the roster labels.
void Presentation::Validation::validate_weapon_heading(RosterPapers& papers) const
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	const RectangleF weapon_inside = menu_geometry::paper_interior(
		menu_geometry::footer_paper(main.weapon_box_size, this->presentation_.menu_.footer_y),
		menu_geometry::setup_art(main));
	const Vector2F weapon_heading_size = render.measure_text(render.resolve_sprite_font(this->presentation_.menu_.heading_font),
		this->presentation_.menu_text_.heading(MenuHeading::weapons));
	const RectangleF weapon_heading = RectangleF::union_of(
		RectangleF(this->presentation_.menu_.layout.position, weapon_heading_size),
		RectangleF(this->presentation_.menu_.layout.position + this->presentation_.menu_.heading_shadow_offset, weapon_heading_size));
	if (!std::isfinite(weapon_heading.x) || !std::isfinite(weapon_heading.y) ||
		!std::isfinite(weapon_heading.width) || !std::isfinite(weapon_heading.height) ||
		weapon_heading.width <= 0.0f || weapon_heading.height <= 0.0f || !weapon_inside.contains(weapon_heading))
	{
		throw std::runtime_error(this->presentation_.source_path_ +
			": menu_text.headings.weapons must fit main_menu.weapon_box_size including its retained shadow bounds.");
	}
	papers.weapon_inside = weapon_inside;
	papers.weapon_heading = weapon_heading;
}

// Phase 6: every bot's label at the world player identity.
void Presentation::Validation::validate_bot_labels() const
{
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
}

// Phase 6: Join's player rows for every device and its status lines.
void Presentation::Validation::validate_join_labels(const SetupGeometry& setup) const
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	const auto& setup_inside = setup.setup_inside;
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
}

// Phase 6: the roster labels on Team Select and Weapon Select, a player at a
// time across both pages.
void Presentation::Validation::validate_team_and_weapon_labels(const RosterPapers& papers, FontHandle name_font) const
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	const WeaponSelectArtworkPresentation& artwork = this->presentation_.main_menu_.weapon_select_art;
	const auto& team_inside = papers.team_inside;
	const auto& team_heading = papers.team_heading;
	const auto& team_warning = papers.team_warning;
	const auto& weapon_inside = papers.weapon_inside;
	const auto& weapon_heading = papers.weapon_heading;
	// The remaining label consumers have different fonts, shadows and badge
	// geometry. Validate their complete finite outputs, retaining their existing
	// unfitted layouts and the Pad row's existing OptionRow fit.
	LabelRows team_rows;
	LabelRows weapon_rows;
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
			RectangleF team_card = RectangleF::union_of(team_label, this->presentation_.player_identity_.ui_badge_before(team_label));
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
			RectangleF weapon_row = RectangleF::union_of(weapon_label, this->presentation_.player_identity_.ui_badge_before(weapon_label));
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
}

// Phase 8: Mode's duration value templates.
void Presentation::Validation::validate_mode_duration_templates() const
{
	const MenuValueText& values = this->presentation_.menu_text_.values;
	// No tuning value belongs to Presentation alone. Validate each original
	// duration template and its literal glyphs here; the joint content check
	// measures actual defaults/choices, and Mode checks retained values at refresh.
	for (const auto& field : { std::pair("duration_minutes", &values.duration_minutes),
		std::pair("duration_default", &values.duration_default) })
	{
		const std::string where = this->presentation_.source_path_ + ": menu_text.values." + field.first;
		try
		{
			const std::wstring literal = format_label_template(*field.second, { { L"{value}", L"" } });
			drawable_text::check_font_can_draw(&render, this->presentation_.menu_.item_font, literal, where);
		}
		catch (const std::invalid_argument& error)
		{
			throw std::runtime_error(where + ": " + error.what());
		}
	}
}
