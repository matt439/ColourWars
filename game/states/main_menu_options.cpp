#include "game/pch.h"
#include "game/states/controls_settings_menu.h"
#include "game/states/display_confirmation.h"
#include "game/states/main_menu.h"
#include "game/input_slots.h"

#include <stdexcept>

using namespace DirectX;
using namespace mattmath;
using namespace menu_consts;
using namespace labrador;

#pragma region MainMenuOptions

MainMenuOptions::MainMenuOptions(MenuContext* context, std::optional<int> pausing_slot) :
	MainMenuPage(context), pausing_slot_(pausing_slot),
	options_art_(context->data()->presentation()->main_menu().art)
{
	if (pausing_slot && (*pausing_slot < 0 || *pausing_slot >= input_slots::count ||
		this->level_settings()->player_index_for_slot(*pausing_slot) < 0))
	{
		throw std::invalid_argument("Paused Options requires a device in the match roster.");
	}
	const MainMenuPresentation& look = this->look();
	this->options_art_.focus_padding = look.setup_focus_padding;
	this->options_art_.shadow_offset = look.setup_shadow_offset;
}

void MainMenuOptions::update_page(float /*dt*/)
{
	if (!this->context()->active()) { return; }
	std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	if (this->pausing_slot_)
	{
		inputs = { menu_input_for_owner(inputs, *this->pausing_slot_, *this->level_settings()) };
	}
	for (const ProcessedMenuInput& input : inputs)
	{
		if (input.action == MenuInputAction::back)
		{
			this->close();
			return;
		}
		if (input.action == MenuInputAction::proceed)
		{
			if (this->activate_focus() == Activation::ran)
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

void MainMenuOptions::close()
{
	this->play_wave(this->cancel_sound_);
	this->input()->reset_actions();
	this->reset_menu_motion();
	if (this->pausing_slot_)
	{
		this->context()->pop();
	}
	else
	{
		this->context()->transition_to(std::make_unique<MainMenuHome>(
			this->menu_context(), MainMenuHome::InitialFocus::options));
	}
}

bool MainMenuOptions::debug_start_match(const MenuLevelSettings& plan)
{
	return !this->pausing_slot_ && MainMenuPage::debug_start_match(plan);
}

bool MainMenuOptions::debug_dismiss(bool leaving_match)
{
	if (!this->pausing_slot_) { return MainMenuPage::debug_dismiss(leaving_match); }
	this->close();
	return true;
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
	this->press_focus();
	this->set_status(this->text().status.options_pending, this->look().art.focus_ink);
}

void MainMenuOptions::set_status(const std::wstring& text, const Colour& colour)
{
	this->status_->set_text(text);
	this->status_->set_colour(colour);
	this->status_->set_scale(1.0f);
	const float right = this->paper_->rectangle().right() - this->options_art_.shadow_offset -
		this->options_art_.focus_padding.x;
	const float available = right - this->look().options_status_position.x;
	if (available > 0.0f && this->status_->bounds().width > available)
	{
		this->status_->set_scale(available / this->status_->bounds().width);
	}
}

void MainMenuOptions::apply_settings()
{
	this->play_wave(this->confirm_sound_);

	const bool fs = this->full_screen_selection_;
	if (this->resolution_selection_ != this->save()->resolution() ||
		fs != this->save()->fullscreen() || fs != this->data()->application()->fullscreen())
	{
		this->input()->reset_actions();
		this->context()->push<DisplayTrialResult>(
			std::make_unique<MainMenuDisplayConfirmation>(this->menu_context(),
				DisplaySettingsChoice{ this->resolution_selection_, fs, this->team_palette_selection_ },
				this->pausing_slot_),
			[this](const DisplayTrialResult& result) { this->finish_display_trial(result); });
		return;
	}

	// Palette-only and unchanged Apply do not resize a manually sized window.
	const bool saved = this->save()->set_display_settings_and_save(
		this->resolution_selection_, fs, this->team_palette_selection_);
	this->refresh_resolution_row();
	this->set_status(saved ? this->text().status.options_saved :
		this->text().status.options_save_failed,
		saved ? this->look().art.focus_ink : this->look().options_warning_colour);
}

void MainMenuOptions::finish_display_trial(DisplayTrialResult result)
{
	this->input()->reset_actions();
	this->reset_menu_motion();
	this->refresh_resolution_row();
	const MenuStatusText& status = this->text().status;
	const std::wstring* message = &status.options_reverted;
	bool warning = false;
	switch (result)
	{
	case DisplayTrialResult::kept: message = &status.options_saved; break;
	case DisplayTrialResult::save_failed: message = &status.options_save_failed; warning = true; break;
	case DisplayTrialResult::apply_failed: message = &status.options_display_failed; warning = true; break;
	case DisplayTrialResult::rollback_failed: message = &status.options_rollback_failed; warning = true; break;
	case DisplayTrialResult::reverted: break;
	}
	this->set_status(*message, warning ? this->look().options_warning_colour : this->look().art.focus_ink);
}

void MainMenuOptions::restore_defaults()
{
	// Reset this page's pending choices. Apply owns all active/save mutations;
	// controls and per-pad preferences remain outside this page's transaction.
	const SaveData defaults{};
	this->resolution_selection_ = defaults.resolution;
	this->full_screen_selection_ = defaults.fullscreen;
	this->team_palette_selection_ = defaults.team_palette;
	this->update_full_screen_selection_text();
	this->update_team_palette_selection_text();
	this->refresh_resolution_row();
	this->play_wave(this->confirm_sound_);
	this->set_status(this->text().status.options_pending, this->look().art.focus_ink);
}

void MainMenuOptions::init()
{
	this->input()->reset_actions();
	const MainMenuPresentation& look = this->look();
	const MenuStyle& style = this->menu_style();
	this->set_widget_position(look.options_position);
	this->set_widget_spacing(look.options_spacing);
	this->set_prompts(MenuPrompt::options);

	// Disabling a row greys its label; the displayed value keeps its own colour.
	this->focus_.set_style({ look.art.focus_ink, look.art.ink, look.art.disabled_ink });
	const RectangleF paper_bounds(
		(Vector2F(DESIGN_RESOLUTION.x, style.footer_y) - look.options_box_size) / 2.0f,
		look.options_box_size);
	const Vector2F value_limit(
		paper_bounds.right() - this->options_art_.shadow_offset - this->options_art_.focus_padding.x - look.options_value_x,
		look.options_spacing.y - this->options_art_.focus_padding.y * 2.0f);
	const auto value_position = [&](int row)
	{
		return Vector2F(look.options_value_x, this->calculate_widget_position(0, row).y);
	};

	// Load the saved windowed preset, not the current fullscreen dimensions.
	this->resolution_selection_ = this->save()->resolution();
	this->full_screen_selection_ = this->save()->fullscreen();
	this->team_palette_selection_ = this->save()->team_palette();

	this->background_ = std::make_unique<UiTexture>(
		"background",
		style.sprite_sheet,
		style.fill_frame,
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		look.home_background_colour);

	this->heading_ = std::make_unique<UiText>(
		"heading",
		this->text().heading(MenuHeading::options),
		this->menu_style().heading_font,
		look.options_heading_position,
		this->render_resources(),
		look.art.ink);
	if (this->pausing_slot_)
	{
		this->palette_timing_ = std::make_unique<UiText>("palette_timing",
			this->text().options_palette_timing, style.detail_font,
			Vector2F(look.options_value_x, look.options_heading_position.y),
			this->render_resources(), look.art.ink);
	}

	this->resolution_row_ = std::make_unique<OptionRow>(
		"resolution",
		this->text().item(MenuItem::resolution),
		this->calculate_widget_position(0, 0),
		value_position(0),
		this->menu_style(),
		look.art.ink,
		this->render_resources(),
		[this](Direction direction) { this->cycle_resolution(direction); },
		&this->options_art_, value_limit);

	this->update_resolution_selection_text();

	this->full_screen_row_ = std::make_unique<OptionRow>(
		"full_screen",
		this->text().item(MenuItem::fullscreen),
		this->calculate_widget_position(0, 1),
		value_position(1),
		this->menu_style(),
		look.art.ink,
		this->render_resources(),
		[this](Direction /*direction*/)
		{
			// Two states, so either direction is the same toggle.
			this->full_screen_selection_ = !this->full_screen_selection_;
			this->update_full_screen_selection_text();
		}, &this->options_art_, value_limit);

	this->update_full_screen_selection_text();

	this->team_palette_row_ = std::make_unique<OptionRow>(
		"team_palette",
		this->text().item(MenuItem::team_palette),
		this->calculate_widget_position(0, 2),
		value_position(2),
		this->menu_style(),
		look.art.ink,
		this->render_resources(),
		[this](Direction direction) { this->cycle_team_palette(direction); },
		&this->options_art_, value_limit);
	this->update_team_palette_selection_text();

	this->controls_ = std::make_unique<UiText>("controls", this->text().item(MenuItem::controls_audio),
		style.item_font, this->calculate_widget_position(0, 3),
		this->render_resources(), look.art.ink);
	this->restore_defaults_ = std::make_unique<UiText>("restore_defaults",
		this->text().item(MenuItem::restore_defaults), style.item_font,
		this->calculate_widget_position(0, 4), this->render_resources(), look.art.ink);

	this->apply_ = std::make_unique<UiText>(
		"apply",
		this->text().item(MenuItem::apply),
		this->menu_style().item_font,
		this->calculate_widget_position(0, 5),
		this->render_resources(),
		look.art.ink);

	this->back_ = std::make_unique<UiText>(
		"back",
		this->text().item(MenuItem::back),
		this->menu_style().item_font,
		this->calculate_widget_position(0, 6),
		this->render_resources(),
		look.art.ink);

	this->status_ = std::make_unique<UiText>(
		"status", L"", this->presentation().prompt_bar().font,
		look.options_status_position, this->render_resources(), look.art.focus_ink);

	this->paper_ = std::make_unique<MenuPaperArtwork>(paper_bounds,
		this->render_resources(), &style, &look.art, &this->focus_, nullptr,
		&this->focus_motion_, &this->save()->play_settings(), &this->options_art_);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->background_.get());
	this->texture_container_->add_child(this->paper_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->heading_.get());
	if (this->palette_timing_) { this->text_container_->add_child(this->palette_timing_.get()); }
	this->text_container_->add_child(this->resolution_row_.get());
	this->text_container_->add_child(this->full_screen_row_.get());
	this->text_container_->add_child(this->team_palette_row_.get());
	this->text_container_->add_child(this->controls_.get());
	this->text_container_->add_child(this->restore_defaults_.get());
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
			this->input()->reset_actions();
			this->context()->push(std::make_unique<ControlsSettingsMenu>(
				this->menu_context(), this->pausing_slot_.value_or(-1)));
		});
	this->focus_.add(this->restore_defaults_.get(), [this] { this->restore_defaults(); });
	this->focus_.add(this->apply_.get(), [this] { this->apply_settings(); });
	this->focus_.add(this->back_.get(), [this] { this->close(); });

	// Refresh after registration so fullscreen disables the row before first draw.
	this->refresh_resolution_row();

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
		fs_text = this->text().values.on;
	}
	else
	{
		fs_text = this->text().values.off;
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
	const std::wstring label = this->team_palette_selection_ == TeamColourPair::random
		? this->text().values.random
		: this->presentation().team_palette().fixed_pair(this->team_palette_selection_).label;
	this->team_palette_row_->set_value(label);
}

#pragma endregion MainMenuOptions
