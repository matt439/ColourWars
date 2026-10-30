#include "game/pch.h"

// Presentation validation for the settings pages: Controls, Sticks, Keyboard
// Jump, Pad Buttons and Profiles. The phase functions in
// presentation_validation.cpp call these in diagnostic order.
#include "game/presentation_validation.h"
#include "game/controls_text.h"
#include "game/drawable_text.h"
#include "game/input_bindings.h"
#include "game/input_slots.h"
#include "game/menu_geometry.h"
#include "game/player_profiles.h"

#include <array>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace labrador;
using namespace mattmath;
using namespace presentation_read;

// A Controls or Sticks action row, clear of the help column and status line.
void Presentation::Validation::check_controls_item(MenuItem item, int row, bool fitted_label) const
{
	const ControlsSettingsPresentation& controls = this->presentation_.controls_settings_;
	const Vector2F position = controls.layout.position +
		Vector2F(0.0f, controls.layout.spacing.y * static_cast<float>(row));
	const bool fitted_action = item == MenuItem::keyboard_jump || item == MenuItem::pad_buttons ||
		item == MenuItem::profiles || item == MenuItem::apply;
	const std::optional<float> label_width = fitted_action ?
		std::optional<float>(menu_geometry::settings_action_width(controls, position.x)) :
		(fitted_label ? std::optional<float>(controls.layout.spacing.x - controls.art.focus_padding.x) : std::nullopt);
	const Vector2F size = check_menu_item(item, controls.item_font, position,
		menu_geometry::footer_paper(controls.box_size, this->presentation_.menu_.footer_y), controls.layout.spacing.y, label_width, controls.art, fitted_action);
	if (position.x + size.x + controls.art.focus_padding.x > controls.controls_position.x)
	{
		throw std::runtime_error(this->presentation_.source_path_ + ": menu_text.items." +
			menu_item_keys[static_cast<size_t>(item)] + " must fit before the controls help column including focus padding.");
	}
	if (position.y + size.y + controls.art.focus_padding.y > controls.status_position.y)
	{
		throw std::runtime_error(this->presentation_.source_path_ + ": menu_text.items." +
			menu_item_keys[static_cast<size_t>(item)] + " must fit above the controls status line including focus padding.");
	}
}

// Phase 1: Controls' eleven rows.
void Presentation::Validation::validate_controls_actions() const
{
	const MenuItem controls_items[]{ MenuItem::pad_sticks, MenuItem::keyboard_jump, MenuItem::pad_buttons, MenuItem::profiles, MenuItem::music_volume,
		MenuItem::sfx_volume, MenuItem::view_shake, MenuItem::aim_marker, MenuItem::reduce_effects, MenuItem::apply, MenuItem::back };
	for (size_t index = 0; index < std::size(controls_items); ++index)
	{
		check_controls_item(controls_items[index], static_cast<int>(index), index >= 4 && index <= 8);
	}
}

// Phase 1: Sticks' option rows, Apply and Back.
void Presentation::Validation::validate_sticks_actions() const
{
	// Pad sticks: seven option rows, then Apply and Back.
	const MenuItem sticks_items[]{ MenuItem::pad, MenuItem::move_deadzone, MenuItem::aim_deadzone,
		MenuItem::rumble, MenuItem::view_shake, MenuItem::aim_marker, MenuItem::reduce_effects,
		MenuItem::apply, MenuItem::back };
	for (size_t index = 0; index < std::size(sticks_items); ++index)
	{
		check_controls_item(sticks_items[index], static_cast<int>(index), index < 7);
	}
}

// Phase 3: on and off in the Sticks Rumble row and the Controls toggles,
// whose value limit later settings checks use.
Vector2F Presentation::Validation::validate_settings_toggles() const
{
	const ControlsSettingsPresentation& controls = this->presentation_.controls_settings_;
	const MenuValueText& values = this->presentation_.menu_text_.values;
	const Vector2F controls_value_limit(
		menu_geometry::settings_action_width(controls, controls.layout.position.x + controls.layout.spacing.x),
		controls.layout.spacing.y - controls.art.focus_padding.y * 2.0f);
	// Rumble occupies Stick row 3; Controls toggles occupy rows 6 through 8.
	for (const int row : { 3, 6, 7, 8 })
	{
		const Vector2F position = controls.layout.position +
			Vector2F(controls.layout.spacing.x, controls.layout.spacing.y * static_cast<float>(row));
		check_value("on", values.on, controls.item_font, position, controls_value_limit,
			controls.box_size, controls.art, controls.layout.spacing.y, true);
		check_value("off", values.off, controls.item_font, position, controls_value_limit,
			controls.box_size, controls.art, controls.layout.spacing.y, true);
	}

	return controls_value_limit;
}

// A help block fitted uniformly into an area of the controls help region.
RectangleF Presentation::Validation::check_help_fit(const std::string& key, const std::wstring& text,
	const Vector2F& position, const Vector2F& available, const RectangleF& help_area) const
{
	const ControlsSettingsPresentation& controls = this->presentation_.controls_settings_;
	const Vector2F raw = fixed_menu_size(key, text, controls.help_font);
	if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
		!std::isfinite(available.x) || !std::isfinite(available.y) || available.x <= 0.0f || available.y <= 0.0f ||
		!help_area.contains(RectangleF(position, available)))
	{
		throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
			" must have a finite positive fitting area inside the controls help region.");
	}
	// Preserve the two pages' existing uniform fit. The available region
	// bounds it by construction; supported long blocks need no length cap.
	const float scale = std::min({ 1.0f, available.x / raw.x, available.y / raw.y });
	const Vector2F fitted = raw * scale;
	if (!std::isfinite(scale) || scale <= 0.0f || !std::isfinite(fitted.x) || !std::isfinite(fitted.y) ||
		fitted.x <= 0.0f || fitted.y <= 0.0f)
	{
		throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
			" must retain finite positive text bounds after fitting.");
	}
	return RectangleF(position, fitted);
}

// Phase 3: Controls' help for every jump key and keyboard binding.
RectangleF Presentation::Validation::validate_controls_help(const RectangleF& help_area) const
{
	const ControlsSettingsPresentation& controls = this->presentation_.controls_settings_;
	check_fixed_paper("menu_text.help.controls", help_area, controls.box_size, Vector2F::ZERO);
	RectangleF controls_help;
	bool first_help = true;
	const auto controls_variant = [&](const std::wstring& words)
	{
		const RectangleF fitted = check_help_fit("menu_text.help.controls", words,
			controls.controls_position, controls.controls_size, help_area);
		controls_help = first_help ? fitted : RectangleF::union_of(controls_help, fitted);
		first_help = false;
	};
	for (const Key key : controls_text::jump_keys())
	{
		controls_variant(controls_text::controls_help(this->presentation_.menu_text_.help.controls, key));
	}
	// Each keyboard action's every label, the others at their defaults where
	// valid, so every glyph the Controls help can show is measured.
	for (size_t index = 0; index < keyboard_action_count; ++index)
	{
		const KeyboardAction action = static_cast<KeyboardAction>(index);
		const std::array<MouseButton, 5> buttons = shoot_mouse_buttons();
		std::vector<DesktopControl> choices;
		if (action == KeyboardAction::pause) { choices.push_back({}); }
		for (const Key key : controls_text::jump_keys()) { choices.push_back({ key, MouseButton::none }); }
		if (action == KeyboardAction::shoot) { for (const MouseButton button : buttons) { choices.push_back({ Key::none, button }); } }
		for (const DesktopControl& choice : choices)
		{
			InputBindings bindings;
			set_keyboard_control(bindings, action, choice);
			try { bindings.validate_keyboard(); }
			catch (const std::invalid_argument&) { continue; }
			controls_variant(controls_text::controls_help(this->presentation_.menu_text_.help.controls, bindings));
		}
	}
	return controls_help;
}

// Phase 3: Keyboard Jump's heading, rows, values, Reset and help.
void Presentation::Validation::validate_keyboard_jump_text(const RectangleF& help_area) const
{
	const ControlsSettingsPresentation& controls = this->presentation_.controls_settings_;
	const KeyboardJumpMenuText& jump = this->presentation_.menu_text_.keyboard_jump;
	const auto jump_text = [&](const char* key, const std::wstring& words, const std::string& font, const Vector2F& position, bool row)
	{
		const std::string where = std::string("menu_text.keyboard_jump.") + key;
		Vector2F size = fixed_menu_size(where, words, font);
		if (row)
		{
			// KeyboardJumpMenu fits its action labels uniformly in both axes.
			const Vector2F available(menu_geometry::settings_action_width(controls, position.x),
				controls.layout.spacing.y - controls.art.focus_padding.y * 2.0f);
			const float scale = std::min({ 1.0f, available.x / size.x, available.y / size.y });
			if (!std::isfinite(scale) || scale <= 0.0f)
			{ throw std::runtime_error(this->presentation_.source_path_ + ": " + where + " must retain finite positive fitted action bounds."); }
			size *= scale;
		}
		check_fixed_paper(where, RectangleF(position, size), controls.box_size, row ? controls.art.focus_padding : Vector2F::ZERO);
		if (position.x + size.x + (row ? controls.art.focus_padding.x : 0.0f) > controls.controls_position.x ||
			(row && size.y + controls.art.focus_padding.y * 2.0f > controls.layout.spacing.y))
		{ throw std::runtime_error(this->presentation_.source_path_ + ": " + where + " must fit its controls column and row."); }
		return RectangleF(position, size);
	};
	const RectangleF jump_heading = jump_text("heading", jump.heading, this->presentation_.menu_.heading_font, controls.heading_position, false);
	if (jump_heading.bottom() > controls.layout.position.y - controls.art.focus_padding.y)
	{ throw std::runtime_error(this->presentation_.source_path_ + ": menu_text.keyboard_jump.heading must end before its first focus card."); }
	// Rows: six capture rows (an OptionRow label and its control value), then
	// Reset, Apply and Back.
	const char* keyboard_rows[]{ "move_left", "move_right", "jump", "drop", "shoot", "pause" };
	for (size_t index = 0; index < keyboard_action_count; ++index)
	{
		const Vector2F position = controls.layout.position + Vector2F(0.0f, controls.layout.spacing.y * static_cast<float>(index));
		Vector2F label = fixed_menu_size(std::string("menu_text.keyboard_jump.") + keyboard_rows[index], jump.row(index), controls.item_font);
		const Vector2F label_available(controls.layout.spacing.x - controls.art.focus_padding.x,
			controls.layout.spacing.y - controls.art.focus_padding.y * 2.0f);
		label *= std::min({ 1.0f, label_available.x / label.x, label_available.y / label.y });
		check_fixed_paper(std::string("menu_text.keyboard_jump.") + keyboard_rows[index], RectangleF(position, label),
			controls.box_size, controls.art.focus_padding);
		for (const std::wstring& value : controls_text::keyboard_control_labels(static_cast<KeyboardAction>(index)))
		{
			jump_text(keyboard_rows[index], value, controls.item_font, position + Vector2F(controls.layout.spacing.x, 0.0f), true);
		}
	}
	jump_text("reset", jump.reset, controls.item_font,
		controls.layout.position + Vector2F(0.0f, controls.layout.spacing.y * static_cast<float>(keyboard_action_count)), true);
	std::ignore = fixed_menu_size("menu_text.keyboard_jump.help", jump.help, controls.help_font);
	std::ignore = fixed_menu_size("menu_text.keyboard_jump.debug_collision", jump.debug_collision, controls.help_font);
	for (const bool collision : { false, true })
	{
		std::ignore = check_help_fit("menu_text.keyboard_jump.help", collision ? jump.help + L'\n' + jump.debug_collision : jump.help,
			controls.controls_position, controls.controls_size, help_area);
	}
}

// Phase 3: Pad Buttons' heading, rows, values, Reset and help.
RectangleF Presentation::Validation::validate_pad_buttons_text(const RectangleF& help_area) const
{
	const ControlsSettingsPresentation& controls = this->presentation_.controls_settings_;
	const PadButtonsMenuText& pad_buttons = this->presentation_.menu_text_.pad_buttons;
	const auto pad_buttons_text = [&](const char* key, const std::wstring& words, const std::string& font,
		const Vector2F& position, bool row, bool option_label = false)
	{
		const std::string where = std::string("menu_text.pad_buttons.") + key;
		Vector2F size = fixed_menu_size(where, words, font);
		if (row)
		{
			const Vector2F available(option_label ? controls.layout.spacing.x - controls.art.focus_padding.x :
				menu_geometry::settings_action_width(controls, position.x),
				controls.layout.spacing.y - controls.art.focus_padding.y * 2.0f);
			const float scale = std::min({ 1.0f, available.x / size.x, available.y / size.y });
			if (!std::isfinite(scale) || scale <= 0.0f)
			{ throw std::runtime_error(this->presentation_.source_path_ + ": " + where + " must retain finite positive fitted bounds."); }
			size *= scale;
		}
		check_fixed_paper(where, RectangleF(position, size), controls.box_size, row ? controls.art.focus_padding : Vector2F::ZERO);
		if (position.x + size.x + (row ? controls.art.focus_padding.x : 0.0f) > controls.controls_position.x ||
			(row && size.y + controls.art.focus_padding.y * 2.0f > controls.layout.spacing.y))
		{ throw std::runtime_error(this->presentation_.source_path_ + ": " + where + " must fit its controls column and row."); }
		return RectangleF(position, size);
	};
	const RectangleF pad_buttons_heading = pad_buttons_text("heading", pad_buttons.heading, this->presentation_.menu_.heading_font,
		controls.heading_position, false);
	if (pad_buttons_heading.bottom() > controls.layout.position.y - controls.art.focus_padding.y)
	{ throw std::runtime_error(this->presentation_.source_path_ + ": menu_text.pad_buttons.heading must end before its first focus card."); }
	// Rows: Pad, six action entries, Reset, Apply, Back.
	const char* pad_button_rows[]{ "jump", "jump_alternate", "shoot", "shoot_alternate", "pause", "pause_alternate" };
	for (size_t index = 0; index < std::size(pad_button_rows); ++index)
	{
		const Vector2F position = controls.layout.position + Vector2F(0.0f, controls.layout.spacing.y * static_cast<float>(index + 1));
		pad_buttons_text(pad_button_rows[index], pad_buttons.row(index), controls.item_font, position, true, true);
		for (const PadControl control : pad_controls())
		{
			pad_buttons_text(pad_button_rows[index], std::wstring(pad_control_label(control)), controls.item_font,
				position + Vector2F(controls.layout.spacing.x, 0.0f), true);
		}
		pad_buttons_text(pad_button_rows[index], std::wstring(pad_control_label(PadControl::none)), controls.item_font,
			position + Vector2F(controls.layout.spacing.x, 0.0f), true);
	}
	pad_buttons_text("reset", pad_buttons.reset, controls.item_font,
		controls.layout.position + Vector2F(0.0f, controls.layout.spacing.y * 7.0f), true);
	const RectangleF pad_buttons_help = check_help_fit("menu_text.pad_buttons.help", pad_buttons.help,
		controls.controls_position, controls.controls_size, help_area);
	return pad_buttons_help;
}

// Phase 3: Profiles' heading, rows, values and help.
RectangleF Presentation::Validation::validate_profiles_text(const RectangleF& help_area) const
{
	const ControlsSettingsPresentation& controls = this->presentation_.controls_settings_;
	// Profiles (ROAD#21): four option rows, then New, Delete and Back.
	const ProfilesMenuText& profiles = this->presentation_.menu_text_.profiles;
	const auto profiles_text = [&](const char* key, const std::wstring& words, const std::string& font,
		const Vector2F& position, bool row, bool option_label = false)
	{
		const std::string where = std::string("menu_text.profiles.") + key;
		Vector2F size = fixed_menu_size(where, words, font);
		if (row)
		{
			const Vector2F available(option_label ? controls.layout.spacing.x - controls.art.focus_padding.x :
				menu_geometry::settings_action_width(controls, position.x),
				controls.layout.spacing.y - controls.art.focus_padding.y * 2.0f);
			const float scale = std::min({ 1.0f, available.x / size.x, available.y / size.y });
			if (!std::isfinite(scale) || scale <= 0.0f)
			{ throw std::runtime_error(this->presentation_.source_path_ + ": " + where + " must retain finite positive fitted bounds."); }
			size *= scale;
		}
		check_fixed_paper(where, RectangleF(position, size), controls.box_size, row ? controls.art.focus_padding : Vector2F::ZERO);
		if (position.x + size.x + (row ? controls.art.focus_padding.x : 0.0f) > controls.controls_position.x ||
			(row && size.y + controls.art.focus_padding.y * 2.0f > controls.layout.spacing.y))
		{ throw std::runtime_error(this->presentation_.source_path_ + ": " + where + " must fit its controls column and row."); }
		return RectangleF(position, size);
	};
	const RectangleF profiles_heading = profiles_text("heading", profiles.heading, this->presentation_.menu_.heading_font,
		controls.heading_position, false);
	if (profiles_heading.bottom() > controls.layout.position.y - controls.art.focus_padding.y)
	{ throw std::runtime_error(this->presentation_.source_path_ + ": menu_text.profiles.heading must end before its first focus card."); }
	const auto profile_row = [&](int row) { return controls.layout.position + Vector2F(0.0f, controls.layout.spacing.y * static_cast<float>(row)); };
	const auto profile_value = [&](const char* key, const std::wstring& words, int row)
	{ std::ignore = profiles_text(key, words, controls.item_font, profile_row(row) + Vector2F(controls.layout.spacing.x, 0.0f), true); };
	profiles_text("profile", profiles.profile, controls.item_font, profile_row(0), true, true);
	profiles_text("name", profiles.name, controls.item_font, profile_row(1), true, true);
	profiles_text("boost", profiles.boost, controls.item_font, profile_row(2), true, true);
	profiles_text("worn_by", profiles.worn_by, controls.item_font, profile_row(3), true, true);
	// The widest name, as shown and while its first letter is being edited.
	const std::wstring widest_name(player_profiles::max_name_length, L'W');
	profile_value("none", profiles.none, 0);
	profile_value("none", widest_name, 0);
	profile_value("none", L"[W]" + widest_name.substr(1), 1);
	profile_value("none", L"[_]", 1);
	// Boost and pad labels are checked where they first appear; OptionRow
	// fits every value to its column, so here only this page's own words.
	profile_value("nobody", profiles.nobody, 3);
	profiles_text("new_profile", profiles.new_profile, controls.item_font, profile_row(4), true);
	profiles_text("delete_profile", profiles.delete_profile, controls.item_font, profile_row(5), true);
	const RectangleF profiles_help = check_help_fit("menu_text.profiles.help", profiles.help,
		controls.controls_position, controls.controls_size, help_area);
	return profiles_help;
}

// Phase 3: Sticks' pad association help and aim cutoff caption.
Presentation::Validation::SticksHelp Presentation::Validation::validate_sticks_help(const RectangleF& help_area) const
{
	const ControlsSettingsPresentation& controls = this->presentation_.controls_settings_;
	const StickPreviewPresentation& preview = controls.stick_preview;
	const RectangleF pad_association = check_help_fit("menu_text.help.pad_association", this->presentation_.menu_text_.help.pad_association,
		preview.association_position, help_area.bottom_right() - preview.association_position, help_area);
	if (preview.association_position.y < preview.aim.bottom())
	{
		throw std::runtime_error(this->presentation_.source_path_ +
			": menu_text.help.pad_association must begin below the aim preview.");
	}
	const Vector2F caption_size = fixed_menu_size("menu_text.help.aim_cutoff",
		this->presentation_.menu_text_.help.aim_cutoff, controls.help_font);
	const float probe_height = render.measure_text(render.resolve_sprite_font(controls.help_font), L"Move").y;
	const RectangleF caption(preview.aim.position() -
		Vector2F(0.0f, probe_height + controls.art.focus_padding.y), caption_size);
	if (!std::isfinite(probe_height) || probe_height <= 0.0f || !help_area.contains(caption) ||
		caption.y < preview.movement_response.bottom() || caption.bottom() > preview.aim.y)
	{
		throw std::runtime_error(this->presentation_.source_path_ +
			": menu_text.help.aim_cutoff must fit unscaled inside the controls help region between the movement and aim previews.");
	}
	return { pad_association, caption, probe_height };
}

// Phase 3: Controls' status lines.
void Presentation::Validation::validate_controls_statuses(const RectangleF& controls_back,
	const RectangleF& controls_help) const
{
	const ControlsSettingsPresentation& controls = this->presentation_.controls_settings_;
	const MenuStatusText& status_text = this->presentation_.menu_text_.status;
	const StatusMessage controls_statuses[]{ { "controls_pending", status_text.controls_pending },
		{ "controls_saved", status_text.controls_saved }, { "active_save_failed", status_text.active_save_failed } };
	for (const StatusMessage& message : controls_statuses)
	{
		check_status(message, controls.help_font, controls.status_position, controls.box_size,
			controls.art, { controls_back, controls_help });
	}
}

// Phase 3: Keyboard Jump's status lines, clear of its Back card.
void Presentation::Validation::validate_keyboard_jump_statuses(const RectangleF& help_area) const
{
	const ControlsSettingsPresentation& controls = this->presentation_.controls_settings_;
	const KeyboardJumpMenuText& jump = this->presentation_.menu_text_.keyboard_jump;
	const RectangleF jump_back = padded_back(controls.layout.position + Vector2F(0.0f,
		controls.layout.spacing.y * static_cast<float>(keyboard_action_count + 2)), controls.item_font, controls.art);
	const StatusMessage jump_statuses[]{ { "keyboard_jump.pending_status", jump.pending_status }, { "keyboard_jump.saved_status", jump.saved_status },
		{ "keyboard_jump.capture_wait_neutral", jump.capture_wait_neutral }, { "keyboard_jump.capture_ready", jump.capture_ready },
		{ "keyboard_jump.capture_ready_mouse", jump.capture_ready_mouse },
		{ "keyboard_jump.capture_rejected", jump.capture_rejected }, { "keyboard_jump.capture_conflict", jump.capture_conflict },
		{ "keyboard_jump.capture_cancelled", jump.capture_cancelled }, { "keyboard_jump.debug_collision", jump.debug_collision } };
	for (const StatusMessage& message : jump_statuses)
	{ check_status(message, controls.help_font, controls.status_position, controls.box_size, controls.art, { jump_back, help_area }); }
	for (const std::wstring& label : controls_text::keyboard_control_labels(KeyboardAction::shoot))
	{
		const std::wstring words = jump.release_key(label);
		check_status({ "keyboard_jump.capture_release", words }, controls.help_font, controls.status_position,
			controls.box_size, controls.art, { jump_back, help_area });
	}
}

// Phase 3: Pad Buttons' status lines.
void Presentation::Validation::validate_pad_buttons_statuses(const RectangleF& pad_buttons_back,
	const RectangleF& pad_buttons_help) const
{
	const ControlsSettingsPresentation& controls = this->presentation_.controls_settings_;
	const MenuStatusText& status_text = this->presentation_.menu_text_.status;
	const PadButtonsMenuText& pad_buttons = this->presentation_.menu_text_.pad_buttons;
	const StatusMessage pad_buttons_statuses[]{ { "pad_buttons.pending_status", pad_buttons.pending_status },
		{ "pad_buttons.saved_status", pad_buttons.saved_status }, { "pad_buttons.conflict_status", pad_buttons.conflict_status },
		{ "active_save_failed", status_text.active_save_failed } };
	for (const StatusMessage& message : pad_buttons_statuses)
	{
		check_status(message, controls.help_font, controls.status_position, controls.box_size,
			controls.art, { pad_buttons_back, pad_buttons_help });
	}
}

// Phase 3: Profiles' status lines, clear of its Back card.
void Presentation::Validation::validate_profiles_statuses(const RectangleF& profiles_help) const
{
	const ControlsSettingsPresentation& controls = this->presentation_.controls_settings_;
	const MenuStatusText& status_text = this->presentation_.menu_text_.status;
	const ProfilesMenuText& profiles = this->presentation_.menu_text_.profiles;
	const auto profile_row = [&](int row) { return controls.layout.position + Vector2F(0.0f, controls.layout.spacing.y * static_cast<float>(row)); };
	const RectangleF profiles_back = padded_back(profile_row(6), controls.item_font, controls.art);
	const StatusMessage profiles_statuses[]{ { "profiles.idle_status", profiles.idle_status },
		{ "profiles.saved_status", profiles.saved_status }, { "profiles.editing_status", profiles.editing_status },
		{ "profiles.name_taken_status", profiles.name_taken_status }, { "profiles.full_status", profiles.full_status },
		{ "profiles.delete_confirm_status", profiles.delete_confirm_status },
		{ "profiles.deleted_status", profiles.deleted_status }, { "active_save_failed", status_text.active_save_failed } };
	for (const StatusMessage& message : profiles_statuses)
	{
		check_status(message, controls.help_font, controls.status_position, controls.box_size,
			controls.art, { profiles_back, profiles_help });
	}
}

// Phase 3: Sticks' status lines, clear of Back, its help and the previews.
void Presentation::Validation::validate_sticks_statuses(const RectangleF& sticks_back,
	const SticksHelp& sticks_help) const
{
	const ControlsSettingsPresentation& controls = this->presentation_.controls_settings_;
	const MenuStatusText& status_text = this->presentation_.menu_text_.status;
	const StickPreviewPresentation& preview = controls.stick_preview;
	const RectangleF& pad_association = sticks_help.pad_association;
	const RectangleF& caption = sticks_help.aim_cutoff;
	const StatusMessage sticks_statuses[]{ { "sticks_pending", status_text.sticks_pending },
		{ "sticks_saved", status_text.sticks_saved }, { "active_save_failed", status_text.active_save_failed } };
	for (const StatusMessage& message : sticks_statuses)
	{
		check_status(message, controls.help_font, controls.status_position, controls.box_size,
			controls.art, { sticks_back, pad_association, preview.raw_movement, preview.movement_response, preview.aim, caption });
	}
}

// Phase 6: Sticks' connection lines and numeric readouts beside the previews.
void Presentation::Validation::validate_stick_readouts(const ControlsGeometry& control_geometry) const
{
	const ControlsSettingsPresentation& controls = this->presentation_.controls_settings_;
	const StickPreviewPresentation& preview = controls.stick_preview;
	const auto& help_area = control_geometry.help_area;
	const auto& probe_height = control_geometry.probe_height;
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
}

// Phase 7: every pad label in Sticks' Pad value.
void Presentation::Validation::validate_sticks_pad_values(const ControlsGeometry& control_geometry) const
{
	const ControlsSettingsPresentation& controls = this->presentation_.controls_settings_;
	const auto& controls_value_limit = control_geometry.controls_value_limit;
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

// Phase 8: every percentage in the Controls volumes and Sticks deadzones.
void Presentation::Validation::validate_settings_percentages(const ControlsGeometry& control_geometry) const
{
	const ControlsSettingsPresentation& controls = this->presentation_.controls_settings_;
	const auto& controls_value_limit = control_geometry.controls_value_limit;
	// Controls volumes occupy rows two and three; Sticks effective deadzones
	// occupy rows one and two in the same captured font/value column. Validate every
	// integer output, including saved non-step volumes and inherited 100%.
	for (int value = 0; value <= 100; ++value)
	{
		const std::wstring text = this->presentation_.menu_text_.setting_percentage(value);
		for (const int row : { 1, 2, 3, 4 })
		{
			const Vector2F position = controls.layout.position +
				Vector2F(controls.layout.spacing.x, controls.layout.spacing.y * static_cast<float>(row));
			check_value("percentage", text, controls.item_font, position, controls_value_limit,
				controls.box_size, controls.art, controls.layout.spacing.y, true);
		}
	}
}

// Phase 8: the Sticks seat switch's value template.
void Presentation::Validation::validate_sticks_shared_template() const
{
	const MenuValueText& values = this->presentation_.menu_text_.values;
	// The Sticks seat switch's template and its literal glyphs; Sticks fills
	// it with on or off, checked with the toggles.
	for (const auto& field : { std::pair("shared", &values.shared) })
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
