#include "game/pch.h"

// Presentation validation for the Options page. The phase functions in
// presentation_validation.cpp call these in diagnostic order.
#include "game/presentation_validation.h"
#include "engine/render/resolution_manager.h"
#include "game/drawable_text.h"
#include "game/menu_geometry.h"

#include <initializer_list>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>

using namespace labrador;
using namespace mattmath;
using namespace presentation_read;

// Phase 1: Options' seven actions; the three option rows fit their labels.
void Presentation::Validation::validate_options_actions(const SetupGeometry& setup) const
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	const auto& options_art = setup.options_art;
	const MenuItem options_items[]{ MenuItem::resolution, MenuItem::fullscreen, MenuItem::team_palette,
		MenuItem::controls_audio, MenuItem::restore_defaults, MenuItem::apply, MenuItem::back };
	for (size_t index = 0; index < std::size(options_items); ++index)
	{
		const Vector2F position = main.options_position +
			Vector2F(0.0f, main.options_spacing.y * static_cast<float>(index));
		const std::optional<float> label_width = index < 3 ?
			std::optional<float>(main.options_value_x - position.x - options_art.focus_padding.x) : std::nullopt;
		check_menu_item(options_items[index], this->presentation_.menu_.item_font, position,
			menu_geometry::footer_paper(main.options_box_size, this->presentation_.menu_.footer_y), main.options_spacing.y, label_width, options_art);
	}
}

// Phase 3: the on, off and random values in Options' value column, whose
// limit the resolution values use later.
Vector2F Presentation::Validation::validate_options_values(const SetupGeometry& setup) const
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	const MenuValueText& values = this->presentation_.menu_text_.values;
	const auto& options_art = setup.options_art;
	const Vector2F options_value_limit(menu_geometry::paper_text_right(
		menu_geometry::footer_paper(main.options_box_size, this->presentation_.menu_.footer_y), options_art) -
		main.options_value_x,
		main.options_spacing.y - options_art.focus_padding.y * 2.0f);
	check_value("on", values.on, this->presentation_.menu_.item_font, options_value_position(1), options_value_limit,
		main.options_box_size, options_art, main.options_spacing.y, false);
	check_value("off", values.off, this->presentation_.menu_.item_font, options_value_position(1), options_value_limit,
		main.options_box_size, options_art, main.options_spacing.y, false);
	check_value("random", values.random, this->presentation_.menu_.item_font, options_value_position(2), options_value_limit,
		main.options_box_size, options_art, main.options_spacing.y, false);

	return options_value_limit;
}

// Phase 3: Options' status lines, clear of its Back card.
void Presentation::Validation::validate_options_statuses(const SetupGeometry& setup, const RectangleF& options_back) const
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	const auto& options_art = setup.options_art;
	const MenuStatusText& status_text = this->presentation_.menu_text_.status;
	const StatusMessage options_statuses[]{ { "options_pending", status_text.options_pending },
		{ "options_saved", status_text.options_saved }, { "options_save_failed", status_text.options_save_failed },
		{ "options_reverted", status_text.options_reverted }, { "options_display_failed", status_text.options_display_failed },
		{ "options_rollback_failed", status_text.options_rollback_failed } };
	for (const StatusMessage& message : options_statuses)
	{
		check_status(message, this->presentation_.prompt_bar_.font, main.options_status_position, main.options_box_size,
			options_art, { options_back });
	}
}

// Phase 5: the palette timing note beside the Options heading.
void Presentation::Validation::validate_options_palette_timing(const SetupGeometry& setup) const
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	const auto& options_art = setup.options_art;
	const std::string palette_timing_key = "menu_text.options_palette_timing";
	const Vector2F palette_timing_size = fixed_menu_size(palette_timing_key,
		this->presentation_.menu_text_.options_palette_timing, this->presentation_.menu_.detail_font);
	const RectangleF palette_timing(Vector2F(main.options_value_x, main.options_heading_position.y), palette_timing_size);
	check_fixed_paper(palette_timing_key, palette_timing, main.options_box_size, Vector2F::ZERO);
	const RectangleF options_heading(main.options_heading_position,
		fixed_menu_size("menu_text.headings.options", this->presentation_.menu_text_.heading(MenuHeading::options), this->presentation_.menu_.heading_font));
	if (palette_timing.x < options_heading.right() ||
		palette_timing.bottom() > main.options_position.y - options_art.focus_padding.y)
	{
		throw std::runtime_error(this->presentation_.source_path_ + ": " + palette_timing_key +
			" must fit unscaled beside the Options heading and above the first action's focus padding.");
	}
}

// Phase 8: every generated resolution value in Options' value column.
void Presentation::Validation::validate_options_resolutions(const SetupGeometry& setup,
	const ControlsGeometry& control_geometry) const
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	const MenuValueText& values = this->presentation_.menu_text_.values;
	const auto& options_value_limit = control_geometry.options_value_limit;
	const auto& options_art = setup.options_art;
	// Fullscreen sizes are positive integers, not a closed set of presets.
	// Check every generated glyph and retain the value column's complete fit
	// area: a shorter, narrower value can stay taller than a long fitted one.
	const std::string resolution_key = "menu_text.values.resolution";
	try
	{
		const std::wstring literal = format_label_template(values.resolution,
			{ { L"{width}", L"" }, { L"{height}", L"" } });
		drawable_text::check_font_can_draw(&render, this->presentation_.menu_.item_font,
			literal + L"0123456789", this->presentation_.source_path_ + ": " + resolution_key);
	}
	catch (const std::invalid_argument& error)
	{
		throw std::runtime_error(this->presentation_.source_path_ + ": " + resolution_key + ": " + error.what());
	}
	const auto check_resolution = [&](const Vector2I& size)
	{
		check_value("resolution", this->presentation_.menu_text_.setting_resolution(size.x, size.y), this->presentation_.menu_.item_font,
			options_value_position(0), options_value_limit, main.options_box_size, options_art,
			main.options_spacing.y, false);
	};
	ResolutionManager preset;
	for (int index = 0; index <= static_cast<int>(ScreenResolution::max); ++index)
	{
		preset.set_resolution(static_cast<ScreenResolution>(index));
		check_resolution(preset.resolution_ivec());
	}
	const int largest = std::numeric_limits<int>::max();
	for (const Vector2I size : { Vector2I(1, 1), Vector2I(1600, 900),
		Vector2I(1, largest), Vector2I(largest, 1), Vector2I(largest, largest) })
	{
		check_resolution(size);
	}
	// OptionRow always resets to scale 1, then fits width and height. Each
	// actual finite positive output therefore stays within this rectangle;
	// INT_MAX is a domain endpoint, not an assertion about the widest digits.
	check_fixed_paper(resolution_key, RectangleF(options_value_position(0), options_value_limit),
		main.options_box_size, options_art.focus_padding);
}
