#include "game/pch.h"
#include "game/states/controls_settings_menu.h"
#include "game/states/main_menu.h"

using namespace DirectX;
using namespace mattmath;
using namespace menu_consts;
using namespace labrador;

#pragma region MainMenuOptions

MainMenuOptions::MainMenuOptions(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuOptions::update_page(float /*dt*/)
{
	for (const ProcessedMenuInput& input : this->menu_inputs())
	{
		if (input.action == MenuInputAction::back)
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(std::make_unique<MainMenuHome>(
				this->menu_context()));
			return;
		}
		if (input.action == MenuInputAction::proceed)
		{
			if (this->focus_.activate(0) == Activation::ran)
			{
				return;
			}
			continue;
		}
		if (this->focus_.move(0, input.direction))
		{
			this->play_wave(this->direction_sound_);
			continue;
		}

		// Rows overlap horizontally, so left/right does not move focus.
		// Let the focused value row handle that input instead.
		this->adjust_focused_value(input.direction);
	}
}

void MainMenuOptions::adjust_focused_value(labrador::Direction direction)
{
	if (direction != labrador::Direction::left &&
		direction != labrador::Direction::right)
	{
		return;
	}

	const OptionRow* row =
		dynamic_cast<const OptionRow*>(this->focus_.focused(0));
	if (row == nullptr)
	{
		return;
	}

	// A row disabled while focused must refuse adjustments as well as activation.
	if (!this->focus_.enabled(row))
	{
		return;
	}

	this->play_wave(this->direction_sound_);
	row->adjust(direction);
	this->status_->set_text(L"Apply to save changes.");
	this->status_->set_colour(this->menu_style().highlight);
}

void MainMenuOptions::apply_settings()
{
	this->play_wave(this->confirm_sound_);

	const bool fs = this->full_screen_selection_;

	// Always save the windowed preset, including when entering fullscreen.
	// Apply it only when returning to or staying windowed, before setting
	// fullscreen, so entering fullscreen does not trigger an intermediate resize.
	const bool saved = this->save()->set_display_settings_and_save(
		this->resolution_selection_, fs, this->team_palette_selection_);

	if (!fs)
	{
		this->data()->application()->set_resolution(this->resolution_selection_);
	}

	this->apply_fullscreen_setting(fs);

	// Refresh from the applied display state.
	this->refresh_resolution_row();
	this->status_->set_text(saved ? L"Settings applied and saved." :
		L"Settings active for this run. Could not save for next time.");
	this->status_->set_colour(saved ? this->menu_style().highlight :
		this->menu_style().warning);
}

void MainMenuOptions::apply_fullscreen_setting(bool fullscreen)
{
	this->data()->application()->set_fullscreen(fullscreen);
}

void MainMenuOptions::init()
{
	this->set_widget_spacing(this->look().options_spacing);
	this->set_prompts({
		{ L"Left/Right", L"Change" },
		{ L"A", L"Select" },
		{ L"B", L"Back" } });

	// Disabling a row greys its label; the displayed value keeps its own colour.
	this->focus_.set_style({ this->menu_style().highlight,
		this->menu_style().unhighlight,
		this->menu_style().disabled });
	Vector2F widget_size = this->widget_size();

	// Load the saved windowed preset, not the current fullscreen dimensions.
	this->resolution_selection_ = this->save()->resolution();
	this->full_screen_selection_ = this->save()->fullscreen();
	this->team_palette_selection_ = this->save()->team_palette();

	this->background_ = std::make_unique<UiTexture>(
		"background",
		this->look().backdrop_sheet,
		this->look().backdrop_frame,
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		this->look().options_background_colour);

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Options",
		this->menu_style().heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		this->menu_style().heading_colour,
		this->menu_style().shadow_colour,
		this->menu_style().heading_shadow_offset);

	this->resolution_row_ = std::make_unique<OptionRow>(
		"resolution",
		L"Resolution",
		this->calculate_widget_position(0, 2),
		this->calculate_widget_position(2, 2),
		this->menu_style(),
		this->look().options_value_colour,
		this->render_resources(),
		[this](Direction direction) { this->cycle_resolution(direction); });

	this->update_resolution_selection_text();

	this->full_screen_row_ = std::make_unique<OptionRow>(
		"full_screen",
		L"Fullscreen",
		this->calculate_widget_position(0, 3),
		this->calculate_widget_position(2, 3),
		this->menu_style(),
		this->look().options_value_colour,
		this->render_resources(),
		[this](Direction /*direction*/)
		{
			// Two states, so either direction is the same toggle.
			this->full_screen_selection_ = !this->full_screen_selection_;
			this->update_full_screen_selection_text();
		});

	this->update_full_screen_selection_text();

	this->team_palette_row_ = std::make_unique<OptionRow>(
		"team_palette",
		L"Team palette",
		this->calculate_widget_position(0, 4),
		this->calculate_widget_position(2, 4),
		this->menu_style(),
		this->look().options_value_colour,
		this->render_resources(),
		[this](Direction direction) { this->cycle_team_palette(direction); });
	this->update_team_palette_selection_text();

	this->controls_ = std::make_unique<UiTextDropShadow>("controls", L"Controls / audio",
		this->menu_style().item_font, this->calculate_widget_position(0, 5),
		this->render_resources(), this->menu_style().unhighlight,
		this->menu_style().shadow_colour, this->menu_style().item_shadow_offset);

	this->apply_ = std::make_unique<UiTextDropShadow>(
		"apply",
		L"Apply",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 6),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->back_ = std::make_unique<UiTextDropShadow>(
		"back",
		L"Back",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 7),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->status_ = std::make_unique<UiTextDropShadow>(
		"status", L"", this->presentation().prompt_bar().font,
		this->calculate_widget_position(2, 6), this->render_resources(),
		this->menu_style().highlight, this->menu_style().shadow_colour,
		this->menu_style().detail_shadow_offset);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->background_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->resolution_row_.get());
	this->text_container_->add_child(this->full_screen_row_.get());
	this->text_container_->add_child(this->team_palette_row_.get());
	this->text_container_->add_child(this->controls_.get());
	this->text_container_->add_child(this->apply_.get());
	this->text_container_->add_child(this->back_.get());
	this->text_container_->add_child(this->status_.get());

	// Value rows respond to left/right; confirming them has no action.
	this->focus_.add(this->resolution_row_.get());
	this->focus_.add(this->full_screen_row_.get());
	this->focus_.add(this->team_palette_row_.get());
	this->focus_.add(this->controls_.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			this->context()->push(std::make_unique<ControlsSettingsMenu>(this->menu_context()));
		});
	this->focus_.add(this->apply_.get(), [this] { this->apply_settings(); });
	this->focus_.add(this->back_.get(), [this]
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuHome>(this->menu_context()));
		});

	// Refresh after registration so fullscreen disables the row before first draw.
	this->refresh_resolution_row();

	this->play_music(this->music_, this->look().music.volume);
}

void MainMenuOptions::cycle_resolution(
	labrador::Direction direction)
{
	int enum_max = static_cast<int>(ScreenResolution::max);
	int enum_pos =
		static_cast<int>(this->resolution_selection_);
	if (direction == labrador::Direction::left)
	{
		if (enum_pos == 0)
		{
			enum_pos = enum_max;
		}
		else
		{
			enum_pos--;
		}
	}
	else if (direction == labrador::Direction::right)
	{
		if (enum_pos == enum_max)
		{
			enum_pos = 0;
		}
		else
		{
			enum_pos++;
		}
	}
	this->resolution_selection_ =
		static_cast<ScreenResolution>(enum_pos);
	this->update_resolution_selection_text();
}

void MainMenuOptions::update_resolution_selection_text()
{
	const std::string res_text = this->resolution_manager()->
		convert_resolution_to_string(this->resolution_selection_);
	this->resolution_row_->set_value(widen(res_text));
}

// While fullscreen, show the live client dimensions and disable the row.
// Keep resolution_selection_ as the windowed preset to restore on exit.
void MainMenuOptions::refresh_resolution_row()
{
	const bool fullscreen = this->save()->fullscreen();

	if (fullscreen)
	{
		const Vector2I size = this->resolution_manager()->resolution_ivec();
		this->resolution_row_->set_value(
			widen(std::to_string(size.x) + "x" + std::to_string(size.y)));
	}
	else
	{
		this->update_resolution_selection_text();
	}

	this->focus_.set_enabled(this->resolution_row_.get(), !fullscreen);

	// FocusGroup retains disabled focus. Move it to Fullscreen so opening
	// Options in fullscreen starts on an available row.
	if (fullscreen &&
		this->focus_.focused(0) == this->resolution_row_.get())
	{
		this->focus_.set_focused(0, this->full_screen_row_.get());
	}
}

void MainMenuOptions::update_full_screen_selection_text() const
{
	bool fs = this->full_screen_selection_;
	std::wstring fs_text = L"";
	if (fs)
	{
		fs_text = L"On";
	}
	else
	{
		fs_text = L"Off";
	}
	this->full_screen_row_->set_value(fs_text);
}

void MainMenuOptions::cycle_team_palette(labrador::Direction direction)
{
	const int first = static_cast<int>(TeamColourPair::random);
	const int end = static_cast<int>(TeamColourPair::count);
	int value = static_cast<int>(this->team_palette_selection_) +
		(direction == Direction::left ? -1 : 1);
	if (value < first)
	{
		value = end - 1;
	}
	else if (value >= end)
	{
		value = first;
	}
	this->team_palette_selection_ = static_cast<TeamColourPair>(value);
	this->update_team_palette_selection_text();
}

void MainMenuOptions::update_team_palette_selection_text()
{
	// State which colour belongs to which team before starting a match.
	const wchar_t* label = L"Random";
	switch (this->team_palette_selection_)
	{
	case TeamColourPair::blue_orange: label = L"A: Blue / B: Orange"; break;
	case TeamColourPair::pink_green: label = L"A: Pink / B: Green"; break;
	case TeamColourPair::blue_yellow: label = L"A: Blue / B: Yellow"; break;
	case TeamColourPair::random: break;
	default: throw std::invalid_argument("Invalid team palette");
	}
	this->team_palette_row_->set_value(label);
}

#pragma endregion MainMenuOptions
