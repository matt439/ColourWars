#include "game/pch.h"
#include "game/states/controls_settings_menu.h"
#include "game/states/main_menu.h"
#include "game/states/menu_panel.h"
#include "game/drawable_text.h"
#include "game/build_info.h"

using namespace DirectX;
using namespace mattmath;
using namespace menu_consts;
using namespace labrador;

MainMenuPage::MainMenuPage(MenuContext* context) :
	MenuPage(context)
{
	const MainMenuPresentation& look = this->look();

	this->error_sound_ = this->resolve_wave(look.error_sound);
	this->ready_sound_ = this->resolve_wave(look.ready_sound);
	this->music_ = this->resolve_effect(look.music.name);
}

const MainMenuPresentation& MainMenuPage::look() const
{
	return this->presentation().main_menu();
}

int MainMenuPage::player_count() const
{ 
	return this->level_settings()->
		player_count();
}

std::vector<int> MainMenuPage::connected_slots() const
{
	const std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	std::vector<int> result;
	for (int slot = 0; slot < static_cast<int>(inputs.size()); slot++)
	{
		if (inputs[slot].connected)
		{
			result.push_back(slot);
		}
	}
	return result;
}

void MainMenuPage::set_player_prompts(
	const std::vector<PromptBar::Prompt>& prompts)
{
	this->player_prompts_ = prompts;
	this->set_prompts(prompts);
}

bool MainMenuPage::wait_for_players(
	const std::vector<ProcessedMenuInput>& inputs)
{
	// Focus loss can temporarily hide every pad. It does not change the roster.
	if (!this->context()->active())
	{
		return true;
	}
	const bool missing = std::any_of(inputs.begin(), inputs.end(),
		[](const ProcessedMenuInput& input) { return !input.connected; });
	if (missing != this->waiting_for_players_)
	{
		this->waiting_for_players_ = missing;
		if (missing)
		{
			this->set_prompts({
				{ L"", L"Reconnect controller" },
				{ L"B", L"Change players" } });
		}
		else
		{
			this->set_prompts(this->player_prompts_);
		}
	}
	if (missing)
	{
		// Any available device may recover a setup whose owner disconnected.
		for (const ProcessedMenuInput& input : this->menu_inputs())
		{
			if (input.action == MenuInputAction::back)
			{
				this->play_wave(this->cancel_sound_);
				this->context()->transition_to(
					std::make_unique<MainMenuPlayerCount>(this->menu_context()));
				break;
			}
		}
	}
	return missing;
}

#pragma region MainMenuTitle

MainMenuTitle::MainMenuTitle(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuTitle::update(float /*dt*/)
{
	if (this->context()->active())
	{
		this->no_pad_->set_hidden(!this->connected_slots().empty());
	}

	const std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	for (int i = 0; i < inputs.size(); i++)
	{
		if (inputs[i].action == MenuInputAction::proceed)
		{
			this->play_wave(this->confirm_sound_);
			this->context()->transition_to(std::make_unique<MainMenuHome>(
				this->menu_context()));
			return;
		}
	}
}
void MainMenuTitle::init()
{
	this->set_prompts({
		{ L"A", L"Start" } });

	this->background_ = std::make_unique<UiTexture>(
		"background",
		this->look().backdrop_sheet,
		this->look().backdrop_frame,
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		this->look().title_background_colour);

	this->title_ = std::make_unique<UiTextDropShadow>(
		"title",
		L"COLOUR\nWARS",
		this->look().title_font,
		this->look().title_position,
		this->render_resources(),
		this->look().title_text_colour,
		this->menu_style().shadow_colour,
		this->menu_style().title_shadow_offset);

	this->start_ = std::make_unique<UiTextDropShadow>(
		"start",
		L"LET'S PLAY",
		this->menu_style().item_font,
		this->look().title_start_position,
		this->render_resources(),
		this->look().title_start_text_colour,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->no_pad_ = std::make_unique<UiTextDropShadow>(
		"no_pad",
		L"Connect a controller",
		this->menu_style().item_font,
		Vector2F(250.0f, 800.0f),
		this->render_resources(),
		this->look().title_no_pad_text_colour,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset,
		true);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->background_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->title_.get());
	this->text_container_->add_child(this->start_.get());
	this->text_container_->add_child(this->no_pad_.get());
	this->subtitle_ = std::make_unique<UiTextDropShadow>(
		"subtitle", L"MAKE A MESS. MAKE IT YOURS.",
		this->menu_style().detail_font, this->look().title_subtitle_position,
		this->render_resources(), this->menu_style().unhighlight,
		this->menu_style().shadow_colour, this->menu_style().detail_shadow_offset);
	this->text_container_->add_child(this->subtitle_.get());
	this->focus_.set_style({ this->menu_style().highlight, this->menu_style().unhighlight });
	this->focus_.add(this->start_.get());

	this->build_id_ = std::make_unique<UiTextDropShadow>(
		"build_id", build_info::label, this->presentation().prompt_bar().font,
		this->look().build_id_position, this->render_resources(),
		this->look().title_start_text_colour,
		this->menu_style().shadow_colour,
		this->menu_style().detail_shadow_offset);
	this->text_container_->add_child(this->build_id_.get());

	this->play_music(this->music_, this->look().music.volume);
}

#pragma endregion MainMenuTitle

#pragma region MainMenuHome

MainMenuHome::MainMenuHome(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuHome::update(float /*dt*/)
{
	// Read focus for each device so later inputs see earlier navigation
	// from the same frame.
	for (const ProcessedMenuInput& input : this->menu_inputs())
	{
		if (input.action == MenuInputAction::back)
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuTitle>(this->menu_context()));
			return;
		}
		if (input.action == MenuInputAction::proceed)
		{
			// Stop after one activation: every action here leaves the page.
			if (this->focus_.activate(0) == Activation::ran)
			{
				return;
			}
		}
		else if (this->focus_.move(0, input.direction))
		{
			this->play_wave(this->direction_sound_);
		}
	}
}
void MainMenuHome::init()
{
	this->set_prompts({
		{ L"A", L"Select" },
		{ L"B", L"Back" } });

	this->focus_.set_style({ this->menu_style().highlight, this->menu_style().unhighlight });

	this->background_ = std::make_unique<UiTexture>(
		"background",
		this->look().backdrop_sheet,
		this->look().backdrop_frame,
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		this->look().home_background_colour);

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Make your mark.",
		this->menu_style().heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		this->menu_style().heading_colour,
		this->menu_style().shadow_colour,
		this->menu_style().heading_shadow_offset);

	this->play_ = std::make_unique<UiTextDropShadow>(
		"play",
		L"Play",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 2),
		this->render_resources(),
		this->menu_style().highlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->options_ = std::make_unique<UiTextDropShadow>(
		"options",
		L"Options",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 3),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->exit_ = std::make_unique<UiTextDropShadow>(
		"exit",
		L"Exit",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 4),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->background_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->play_.get());
	this->text_container_->add_child(this->options_.get());
	this->text_container_->add_child(this->exit_.get());
	this->subtitle_ = std::make_unique<UiTextDropShadow>(
		"subtitle", L"Paint the arena. Claim the win.",
		this->menu_style().detail_font, this->look().home_subtitle_position,
		this->render_resources(), this->menu_style().unhighlight,
		this->menu_style().shadow_colour, this->menu_style().detail_shadow_offset);
	this->text_container_->add_child(this->subtitle_.get());

	// Navigation uses widget bounds in design space, not declaration order.
	this->focus_.add(this->play_.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuModeSelect>(this->menu_context()));
		});
	this->focus_.add(this->options_.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuOptions>(this->menu_context()));
		});
	this->focus_.add(this->exit_.get(), [this]
		{
			this->data()->application()->quit();
		});

	this->play_music(this->music_, this->look().music.volume);
}

#pragma endregion MainMenuHome

#pragma region MainMenuOptions

MainMenuOptions::MainMenuOptions(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuOptions::update(float /*dt*/)
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

#pragma region MainMenuModeSelect

MainMenuModeSelect::MainMenuModeSelect(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuModeSelect::update(float /*dt*/)
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
		}
		else if (this->focus_.move(0, input.direction))
		{
			this->play_wave(this->direction_sound_);
		}
	}
}
void MainMenuModeSelect::init()
{
	this->set_prompts({
		{ L"A", L"Select" },
		{ L"B", L"Back" } });

	// Every mode uses the same player setup and paint scoring.
	this->focus_.set_style({ this->menu_style().highlight,
		this->menu_style().unhighlight,
		this->menu_style().disabled });

	this->set_widget_spacing(this->look().mode_select_spacing);

	this->background_ = std::make_unique<UiTexture>(
		"background",
		this->look().backdrop_sheet,
		this->look().backdrop_frame,
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		this->look().play_background_colour);

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Mode Select",
		this->menu_style().heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		this->menu_style().heading_colour,
		this->menu_style().shadow_colour,
		this->menu_style().heading_shadow_offset);

	this->standard_ = std::make_unique<UiTextDropShadow>(
		"standard",
		L"Single Match",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 2),
		this->render_resources(),
		this->menu_style().highlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->best_of_three_ = std::make_unique<UiTextDropShadow>(
		"best_of_three",
		L"Best of 3",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 3),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->best_of_five_ = std::make_unique<UiTextDropShadow>(
		"best_of_five",
		L"Best of 5",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 4),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->practice_ = std::make_unique<UiTextDropShadow>(
		"practice",
		L"Solo practice: choose Single Match",
		this->menu_style().detail_font,
		this->calculate_widget_position(0, 6),
		this->render_resources(),
		this->menu_style().heading_colour,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->back_ = std::make_unique<UiTextDropShadow>(
		"back",
		L"Back",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 5),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->background_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->standard_.get());
	this->text_container_->add_child(this->best_of_three_.get());
	this->text_container_->add_child(this->best_of_five_.get());
	this->text_container_->add_child(this->practice_.get());
	this->text_container_->add_child(this->back_.get());

	const auto choose = [this](int best_of)
		{
			this->level_settings()->set_best_of(best_of);
			this->play_wave(this->confirm_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuPlayerCount>(this->menu_context()));
		};
	this->focus_.add(this->standard_.get(), [choose] { choose(1); });
	this->focus_.add(this->best_of_three_.get(), [choose] { choose(3); });
	this->focus_.add(this->best_of_five_.get(), [choose] { choose(5); });

	this->focus_.add(this->back_.get(), [this]
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuHome>(this->menu_context()));
		});

	this->play_music(this->music_, this->look().music.volume);
}

#pragma endregion MainMenuModeSelect

#pragma region MainMenuPlayerCount

MainMenuPlayerCount::MainMenuPlayerCount(MenuContext* context) :
	MainMenuPage(context)
{

}
void MainMenuPlayerCount::update(float /*dt*/)
{
	if (!this->context()->active())
	{
		return;
	}
	const std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	for (int slot = 0; slot < static_cast<int>(inputs.size()); ++slot)
	{
		const ProcessedMenuInput& input = inputs[slot];
		if (input.action == MenuInputAction::back)
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuModeSelect>(this->menu_context()));
			return;
		}
		if (input.action == MenuInputAction::proceed)
		{
			this->activating_slot_ = slot;
			if (this->focus_.activate(0) == Activation::ran)
			{
				return;
			}
		}
		else if (this->focus_.move(0, input.direction))
		{
			this->play_wave(this->direction_sound_);
		}
	}
}
void MainMenuPlayerCount::init()
{
	this->set_prompts({
		{ L"A", L"Select" },
		{ L"B", L"Back" } });

	// Devices can be connected and claimed on the next page.
	this->focus_.set_style({ this->menu_style().highlight,
		this->menu_style().unhighlight,
		this->menu_style().disabled });

	this->set_widget_spacing(this->look().player_count_spacing);

	this->background_ = std::make_unique<UiTexture>(
		"background",
		this->look().backdrop_sheet,
		this->look().backdrop_frame,
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		this->look().play_background_colour);

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Number of Players",
		this->menu_style().heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		this->menu_style().heading_colour,
		this->menu_style().shadow_colour,
		this->menu_style().heading_shadow_offset);

	this->_1_player = std::make_unique<UiTextDropShadow>(
		"1_player",
		L"1 Player (practice)",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 2),
		this->render_resources(),
		this->menu_style().highlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->_2_players = std::make_unique<UiTextDropShadow>(
		"2_players",
		L"2 Players",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 3),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->_3_players = std::make_unique<UiTextDropShadow>(
		"3_players",
		L"3 Players",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 4),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->_4_players = std::make_unique<UiTextDropShadow>(
		"4_players",
		L"4 Players",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 5),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->back_ = std::make_unique<UiTextDropShadow>(
		"back",
		L"Back",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 6),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->background_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->_1_player.get());
	this->text_container_->add_child(this->_2_players.get());
	this->text_container_->add_child(this->_3_players.get());
	this->text_container_->add_child(this->_4_players.get());
	this->text_container_->add_child(this->back_.get());

	this->focus_.add(this->_1_player.get(),
		[this] { this->start_join(1, ScreenLayout::one_player); });
	this->focus_.add(this->_2_players.get(),
		[this] { this->start_join(2, ScreenLayout::two_player); });
	this->focus_.add(this->_3_players.get(),
		[this] { this->start_join(3, ScreenLayout::three_player); });
	this->focus_.add(this->_4_players.get(),
		[this] { this->start_join(4, ScreenLayout::four_player); });
	// A series needs both teams; solo remains available through Single Match.
	this->focus_.set_enabled(this->_1_player.get(),
		this->level_settings()->best_of() == 1);
	if (this->level_settings()->best_of() > 1)
	{
		this->focus_.set_focused(0, this->_2_players.get());
	}
	this->focus_.add(this->back_.get(), [this]
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuModeSelect>(this->menu_context()));
		});

	this->play_music(this->music_, this->look().music.volume);
}

void MainMenuPlayerCount::start_join(int players, ScreenLayout layout)
{
	this->play_wave(this->confirm_sound_);
	MenuLevelSettings* settings = this->level_settings();
	settings->set_player_count(players);
	settings->set_screen_layout(layout);
	settings->begin_input_claims(this->activating_slot_);
	if (players == 1)
	{
		settings->set_player_team(0, PlayerTeam::a);
	}
	this->context()->transition_to(
		std::make_unique<MainMenuJoin>(this->menu_context()));
}

#pragma endregion MainMenuPlayerCount

MainMenuJoin::MainMenuJoin(MenuContext* context) : MainMenuPage(context)
{
}

void MainMenuJoin::init()
{
	this->set_prompts({ { L"A", L"Join / P1 continue" },
		{ L"B", L"Leave / P1 back" } });
	this->set_widget_spacing(this->look().player_count_spacing);
	const MenuStyle& style = this->menu_style();
	this->background_ = std::make_unique<UiTexture>("background",
		this->look().backdrop_sheet,
		this->look().backdrop_frame, RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(), this->look().play_background_colour);
	this->texture_container_ = std::make_unique<UiContainer>("texture_container");
	this->texture_container_->add_child(this->background_.get());
	this->text_container_ = std::make_unique<UiContainer>("text_container");
	this->heading_ = std::make_unique<UiTextDropShadow>("heading", L"Join Players",
		style.heading_font, this->calculate_widget_position(0, 0),
		this->render_resources(), style.heading_colour, style.shadow_colour,
		style.heading_shadow_offset);
	this->text_container_->add_child(this->heading_.get());
	for (int player = 0; player < this->player_count(); ++player)
	{
		this->players_.push_back(std::make_unique<UiTextDropShadow>(
			"player_" + std::to_string(player), L"", style.item_font,
			this->calculate_widget_position(0, player + 2), this->render_resources(),
			style.unhighlight, style.shadow_colour, style.item_shadow_offset));
		this->text_container_->add_child(this->players_.back().get());
	}
	this->status_ = std::make_unique<UiTextDropShadow>("status", L"",
		style.detail_font, this->calculate_widget_position(0, 7),
		this->render_resources(), style.warning, style.shadow_colour,
		style.detail_shadow_offset);
	this->text_container_->add_child(this->status_.get());
	this->refresh_players(this->input()->player_inputs(*this->level_settings()));
	this->play_music(this->music_, this->look().music.volume);
}

void MainMenuJoin::refresh_players(const std::vector<ProcessedMenuInput>& inputs)
{
	bool ready = this->level_settings()->input_roster_complete();
	const std::vector<MenuPlayerSettings> players = this->level_settings()->player_settings();
	for (int player = 0; player < this->player_count(); ++player)
	{
		const bool connected = inputs[player].connected;
		ready = ready && connected;
		std::wstring label = this->level_settings()->player_label(player);
		if (!connected && players[player].num >= 0)
		{
			label += L" - Reconnect";
		}
		this->players_[player]->set_text(label);
		this->players_[player]->set_colour(connected ?
			this->menu_style().highlight : this->menu_style().disabled);
	}
	this->status_->set_text(ready ? L"Everyone joined. Player 1 can continue." :
		L"Press A / Enter on each device to join. Reconnect reserved pads.");
}

void MainMenuJoin::update(float /*dt*/)
{
	if (!this->context()->active())
	{
		return;
	}
	MenuLevelSettings* settings = this->level_settings();
	const std::vector<ProcessedMenuInput> players = this->input()->player_inputs(*settings);
	const std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	// Evaluate readiness before accepting joins: the last join cannot also
	// dismiss the page in the same frame as Player 1's confirmation.
	const bool ready = settings->input_roster_complete() &&
		std::all_of(players.begin(), players.end(),
			[](const ProcessedMenuInput& input) { return input.connected; });
	bool continue_requested = false;
	bool roster_changed = false;
	for (int slot = 0; slot < static_cast<int>(inputs.size()); ++slot)
	{
		const int player = settings->player_index_for_slot(slot);
		if (inputs[slot].action == MenuInputAction::back)
		{
			if (player == 0 || !players[0].connected)
			{
				this->play_wave(this->cancel_sound_);
				this->context()->transition_to(
					std::make_unique<MainMenuPlayerCount>(this->menu_context()));
				return;
			}
			if (settings->release_input_slot(slot))
			{
				roster_changed = true;
				this->play_wave(this->cancel_sound_);
			}
		}
		else if (inputs[slot].action == MenuInputAction::proceed)
		{
			if (player == 0 && ready)
			{
				continue_requested = true;
			}
			if (settings->claim_input_slot(slot))
			{
				roster_changed = true;
				this->play_wave(this->ready_sound_);
			}
		}
	}
	if (continue_requested && !roster_changed)
	{
		this->play_wave(this->confirm_sound_);
		if (this->player_count() == 1)
		{
			this->context()->transition_to(
				std::make_unique<MainMenuWeaponSelect>(this->menu_context()));
		}
		else
		{
			this->context()->transition_to(
				std::make_unique<MainMenuTeamSelect>(this->menu_context()));
		}
		return;
	}
	this->refresh_players(this->input()->player_inputs(*settings));
}

#pragma region MainMenuTeamSelect

MainMenuTeamSelect::MainMenuTeamSelect(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuTeamSelect::update(float /*dt*/)
{
	const std::vector<ProcessedMenuInput> inputs =
		this->input()->player_inputs(*this->level_settings());
	if (this->wait_for_players(inputs))
	{
		return;
	}
	const int num_inputs = static_cast<int>(inputs.size());
	const int player_count = this->player_count();
	// Back leaves only if the frame began with nobody ready. Simultaneous
	// cancellations must not unconfirm one player and then leave the page.
	const bool can_leave = this->all_players_unconfirmed();

	for (int i = 0; i < num_inputs && i < player_count; i++)
	{
		if (inputs[i].action == MenuInputAction::back)
		{
			if (can_leave)
			{
				this->play_wave(this->cancel_sound_);
				this->context()->transition_to(
					std::make_unique<MainMenuPlayerCount>(
						this->menu_context()));
				return;
			}
			if (this->select_states_[i].state == ConfirmationState::confirmed)
			{
				this->play_wave(this->cancel_sound_);
				this->select_states_[i].state =
					ConfirmationState::unconfirmed;
			}
			// Continue so other players can act in the same frame.
		}
		else if (inputs[i].action == MenuInputAction::proceed &&
			this->select_states_[i].team != PlayerTeam::none)
		{
			// Require both teams only on the confirmation that completes a multiplayer
			// roster, allowing earlier players to choose either side.
			if (this->confirm_completes_roster(i) &&
				!this->roster_is_playable())
			{
				this->play_wave(this->error_sound_);
				this->both_teams_needed_->set_hidden(false);
			}
			else
			{
				this->play_wave(this->confirm_sound_);
				this->select_states_[i].state = ConfirmationState::confirmed;
			}
		}
		else if (this->select_states_[i].state ==
			ConfirmationState::unconfirmed)
		{
			if (inputs[i].direction == labrador::Direction::left)
			{
				this->play_wave(this->direction_sound_);
				if (this->select_states_[i].team == PlayerTeam::none)
				{
					this->select_states_[i].team = PlayerTeam::a;
				}
				else if (this->select_states_[i].team == PlayerTeam::b)
				{
					this->select_states_[i].team = PlayerTeam::none;
				}
			}
			else if (inputs[i].direction == labrador::Direction::right)
			{
				this->play_wave(this->direction_sound_);
				if (this->select_states_[i].team == PlayerTeam::none)
				{
					this->select_states_[i].team = PlayerTeam::b;
				}
				else if (this->select_states_[i].team == PlayerTeam::a)
				{
					this->select_states_[i].team = PlayerTeam::none;
				}
			}
		}
	}
	// Clear the warning as soon as the roster becomes playable.
	if (this->roster_is_playable())
	{
		this->both_teams_needed_->set_hidden(true);
	}

	if (this->all_players_confirmed())
	{
		this->set_level_settings();
		this->context()->transition_to(
			std::make_unique<MainMenuWeaponSelect>(
				this->menu_context()));
		return;
	}
	this->update_team_select_visuals();
}
bool MainMenuTeamSelect::confirm_completes_roster(int player) const
{
	for (int i = 0; i < this->player_count(); i++)
	{
		if (i != player &&
			this->select_states_[i].state != ConfirmationState::confirmed)
		{
			return false;
		}
	}
	return true;
}
bool MainMenuTeamSelect::roster_is_playable() const
{
	// Solo matches allow either team; multiplayer requires both.
	if (this->player_count() < 2)
	{
		return true;
	}

	return this->both_teams_chosen();
}
bool MainMenuTeamSelect::both_teams_chosen() const
{
	bool team_a = false;
	bool team_b = false;
	for (int i = 0; i < this->player_count(); i++)
	{
		switch (this->select_states_[i].team)
		{
		case PlayerTeam::a:
			team_a = true;
			break;
		case PlayerTeam::b:
			team_b = true;
			break;
		default:
			break;
		}
	}
	return team_a && team_b;
}
void MainMenuTeamSelect::set_level_settings() const
{
	for (int i = 0; i < this->player_count(); i++)
	{
		this->level_settings()->
			set_player_team(i, this->select_states_[i].team);
	}
}
bool MainMenuTeamSelect::all_players_confirmed() const
{
	for (int i = 0; i < this->player_count(); i++)
	{
		if (this->select_states_[i].state != ConfirmationState::confirmed)
		{
			return false;
		}
	}
	return true;
}
bool MainMenuTeamSelect::all_players_unconfirmed() const
{
	for (int i = 0; i < this->player_count(); i++)
	{
		if (this->select_states_[i].state == ConfirmationState::confirmed)
		{
			return false;
		}
	}
	return true;
}
void MainMenuTeamSelect::update_team_select_visuals()
{
	this->deselect_and_unconfirm_all_widgets();

	for (int i = 0; i < this->player_count(); i++)
	{
		UiWidget* selected_widget = nullptr;
		switch (this->select_states_[i].team)
		{
		case PlayerTeam::a:
			this->player_widgets_[i]->player_a->set_sprite_frame(
				"team_select_a_selected");
			selected_widget = this->player_widgets_[i]->player_a.get();
			break;
		case PlayerTeam::none:
			this->player_widgets_[i]->player_center->set_sprite_frame(
				"team_select_center_selected");
			selected_widget = this->player_widgets_[i]->player_center.get();
			break;
		case PlayerTeam::b:
			this->player_widgets_[i]->player_b->set_sprite_frame(
				"team_select_b_selected");
			selected_widget = this->player_widgets_[i]->player_b.get();
			break;
		}
		if (this->select_states_[i].state ==
			ConfirmationState::confirmed)
		{
			selected_widget->set_colour(
				this->look().team_select.selected);
		}
	}
}
void MainMenuTeamSelect::deselect_and_unconfirm_all_widgets()
{
	for (int i = 0; i < this->player_count(); i++)
	{
		this->player_widgets_[i]->player_a->set_sprite_frame(
			"team_select_a");
		this->player_widgets_[i]->player_center->set_sprite_frame(
			"team_select_center");
		this->player_widgets_[i]->player_b->set_sprite_frame(
			"team_select_b");

		this->player_widgets_[i]->player_a->set_colour(
			this->look().team_select.unselected);
		this->player_widgets_[i]->player_center->set_colour(
			this->look().team_select.unselected);
		this->player_widgets_[i]->player_b->set_colour(
			this->look().team_select.unselected);
	}
}
void MainMenuTeamSelect::init()
{
	this->set_player_prompts({
		{ L"Left/Right", L"Pick a team" },
		{ L"A", L"Confirm" },
		{ L"B", L"Cancel" } });

	this->set_widget_spacing(this->look().team_select.spacing);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	
	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");

	this->background_ = std::make_unique<UiTexture>(
		"background",
		this->look().backdrop_sheet,
		this->look().backdrop_frame,
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		this->look().play_background_colour);

	this->texture_container_->add_child(this->background_.get());

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Team Select",
		this->menu_style().heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		this->menu_style().heading_colour,
		this->menu_style().shadow_colour,
		this->menu_style().heading_shadow_offset);

	this->text_container_->add_child(this->heading_.get());

	// Keep the warning below the four player rows and inside the design bounds.
	this->both_teams_needed_ = std::make_unique<UiTextDropShadow>(
		"both_teams_needed",
		L"Both teams need a player",
		this->menu_style().detail_font,
		this->calculate_widget_position(0, 6),
		this->render_resources(),
		this->menu_style().warning,
		this->menu_style().shadow_colour,
		this->menu_style().detail_shadow_offset,
		true);

	this->text_container_->add_child(this->both_teams_needed_.get());


	for (int i = 0; i < this->player_count(); i++)
	{
		auto state = TeamSelectState();
		state.state = ConfirmationState::unconfirmed;
		state.team = this->level_settings()->
			player_team(i);
		this->select_states_.push_back(state);

		auto widgets = std::make_unique<PlayerWidgets>();

		std::string name = "player_" + std::to_string(i + 1);
		std::wstring label_text = this->level_settings()->player_label(i);

		widgets->player = std::make_unique<UiTextDropShadow>(
			name,
			label_text,
			this->menu_style().detail_font,
			this->calculate_widget_position(0, i + 2),
			this->render_resources(),
			this->look().team_select.unselected,
			this->menu_style().shadow_colour,
			this->menu_style().item_shadow_offset);

		widgets->player_a = std::make_unique<UiTexture>(
			name + "_a",
			"sprite_sheet_1",
			"team_select_a",
			RectangleF(this->calculate_widget_position(2, i + 2), this->look().team_select.size),
			this->render_resources());

		auto move = Vector2F(this->look().team_select.size.x, 0.0f);

		widgets->player_center = std::make_unique<UiTexture>(
			name + "_center",
			"sprite_sheet_1",
			"team_select_center",
			RectangleF(widgets->player_a->rectangle().position() + move,
				this->look().team_select.size),
			this->render_resources());

		widgets->player_b = std::make_unique<UiTexture>(
			name + "_b",
			"sprite_sheet_1",
			"team_select_b",
			RectangleF(widgets->player_center->rectangle().position() + move,
				this->look().team_select.size),
			this->render_resources());

		this->texture_container_->add_child(widgets->player_a.get());
		this->texture_container_->add_child(widgets->player_center.get());
		this->texture_container_->add_child(widgets->player_b.get());

		this->text_container_->add_child(widgets->player.get());

		this->player_widgets_.push_back(std::move(widgets));
	}

	this->play_music(this->music_, this->look().music.volume);
}

#pragma endregion MainMenuTeamSelect

#pragma region MainMenuWeaponSelect

MainMenuWeaponSelect::MainMenuWeaponSelect(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuWeaponSelect::update(float /*dt*/)
{
	const std::vector<ProcessedMenuInput> inputs =
		this->input()->player_inputs(*this->level_settings());
	if (this->wait_for_players(inputs))
	{
		return;
	}
	const int player_count = this->player_count();
	const int num_inputs = static_cast<int>(inputs.size());
	// Each player may cancel independently, but leaving is a separate press
	// after the page has shown that every player is unconfirmed.
	const bool can_leave = this->all_players_unconfirmed();

	for (int i = 0; i < num_inputs && i < player_count; i++)
	{
		if (inputs[i].action == MenuInputAction::back)
		{
			if (can_leave)
			{
				this->play_wave(this->cancel_sound_);
				if (this->player_count() == 1)
				{
					this->context()->transition_to(
						std::make_unique<MainMenuModeSelect>(
							this->menu_context()));
				}
				else
				{
					this->context()->transition_to(
						std::make_unique<MainMenuTeamSelect>(
							this->menu_context()));
				}
				return;
			}
			if (this->select_states_[i].state ==
				ConfirmationState::confirmed)
			{
				this->play_wave(this->cancel_sound_);
				this->select_states_[i].state =
					ConfirmationState::unconfirmed;
			}
			// Continue so other players can act in the same frame.
		}
		else if (inputs[i].action == MenuInputAction::proceed)
		{
			this->play_wave(this->confirm_sound_);
			this->select_states_[i].state = ConfirmationState::confirmed;
		}
		else if (this->select_states_[i].state ==
			ConfirmationState::unconfirmed)
		{
			if (inputs[i].direction == labrador::Direction::left)
			{
				this->play_wave(this->direction_sound_);
				this->cycle_weapons(labrador::Direction::left, i);
			}
			else if (inputs[i].direction == labrador::Direction::right)
			{
				this->play_wave(this->direction_sound_);
				this->cycle_weapons(labrador::Direction::right, i);
			}
		}
	}
	if (this->all_players_confirmed())
	{
		for (int i = 0; i < this->player_count(); i++)
		{
			if (this->select_states_[i].type == WeaponType::random_primary)
			{
				this->select_states_[i].type = random_weapon();
			}
		}
		this->set_level_settings();
		this->context()->transition_to(
			std::make_unique<MainMenuStageSelect>(this->menu_context()));
		return;
	}
	this->update_weapon_select_visuals();
}
void MainMenuWeaponSelect::set_level_settings() const
{
	for (int i = 0; i < this->player_count(); i++)
	{
		this->level_settings()->
			set_player_weapon(i, this->select_states_[i].type);
	}
}
WeaponType MainMenuWeaponSelect::random_weapon()
{
	int random = rand() % static_cast<int>(WeaponType::max_prim_wep);
	return static_cast<WeaponType>(random);
}
void MainMenuWeaponSelect::cycle_weapons(
	labrador::Direction direction, int player_index)
{
	int enum_max = static_cast<int>(WeaponType::max_prim_wep);
	int enum_pos =
		static_cast<int>(this->select_states_[player_index].type);
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
	this->select_states_[player_index].type =
		static_cast<WeaponType>(enum_pos);
}
bool MainMenuWeaponSelect::all_players_confirmed() const
{
	for (int i = 0; i < this->player_count(); i++)
	{
		if (this->select_states_[i].state != ConfirmationState::confirmed)
		{
			return false;
		}
	}
	return true;
}
bool MainMenuWeaponSelect::all_players_unconfirmed() const
{
	for (int i = 0; i < this->player_count(); i++)
	{
		if (this->select_states_[i].state == ConfirmationState::confirmed)
		{
			return false;
		}
	}
	return true;
}
void MainMenuWeaponSelect::update_weapon_select_visuals()
{
	this->unconfirm_all_widgets();

	for (int i = 0; i < this->player_count(); i++)
	{
		switch (this->select_states_[i].type)
		{
		case WeaponType::sprayer:
			this->player_widgets_[i]->weapon_icon->set_sprite_frame("sprayer", "sprayer_paint");
			this->player_widgets_[i]->weapon_name->set_text(L"Sprayer");
			break;
		case WeaponType::sniper:
			this->player_widgets_[i]->weapon_icon->set_sprite_frame("sniper", "sniper_paint");
			this->player_widgets_[i]->weapon_name->set_text(L"Sniper");
			break;
		case WeaponType::roller:
			this->player_widgets_[i]->weapon_icon->set_sprite_frame("roller", "roller_paint");
			this->player_widgets_[i]->weapon_name->set_text(L"Roller");
			break;
		case WeaponType::mister:
			this->player_widgets_[i]->weapon_icon->set_sprite_frame("mister", "mister_paint");
			this->player_widgets_[i]->weapon_name->set_text(L"Mister");
			break;
		case WeaponType::bucket:
			this->player_widgets_[i]->weapon_icon->set_sprite_frame("bucket", "bucket_paint");
			this->player_widgets_[i]->weapon_name->set_text(L"Bucket");
			break;
		case WeaponType::random_primary:
			this->player_widgets_[i]->weapon_icon->set_sprite_frame("random");
			this->player_widgets_[i]->weapon_name->set_text(L"Random");
			break;
		default:
			this->player_widgets_[i]->weapon_icon->set_sprite_frame("error");
			this->player_widgets_[i]->weapon_name->set_text(L"ERROR");
			break;
		}

		if (this->select_states_[i].state == ConfirmationState::confirmed)
		{
			this->player_widgets_[i]->weapon_icon->
				set_colour(this->look().weapon_select.selected);
			this->player_widgets_[i]->weapon_name->
				set_colour(this->look().weapon_select.selected);
		}

		this->player_widgets_[i]->weapon_description->set_text(
			this->weapon_description(this->select_states_[i].type));
	}
}
void MainMenuWeaponSelect::unconfirm_all_widgets()
{
	for (int i = 0; i < this->player_count(); i++)
	{
		this->player_widgets_[i]->weapon_icon->set_colour(
			this->look().weapon_select.unselected);
		this->player_widgets_[i]->weapon_name->set_colour(
			this->look().weapon_select.unselected);
	}
}
void MainMenuWeaponSelect::init()
{
	this->set_player_prompts({
		{ L"Left/Right", L"Change weapon" },
		{ L"A", L"Confirm" },
		{ L"B", L"Cancel" } });

	this->set_widget_spacing(this->look().weapon_select.spacing);
	// A random palette is not chosen until the level is built. Keep that
	// preview neutral instead of consuming a different random draw here.
	const TeamColourPair palette = this->save()->team_palette();
	const TeamColour preview_colours = palette == TeamColourPair::random
		? TeamColour(Colour::white, Colour::white)
		: TeamColourTools::team_colours(palette);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	
	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");

	this->background_ = std::make_unique<UiTexture>(
		"background",
		this->look().backdrop_sheet,
		this->look().backdrop_frame,
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		this->look().play_background_colour);

	this->texture_container_->add_child(this->background_.get());

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Weapon Select",
		this->menu_style().heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		this->menu_style().heading_colour,
		this->menu_style().shadow_colour,
		this->menu_style().heading_shadow_offset);

	this->text_container_->add_child(this->heading_.get());

	for (int i = 0; i < this->player_count(); i++)
	{
		SelectState state;
		state.state = ConfirmationState::unconfirmed;
		state.type = this->level_settings()
			->player_weapon(i);
		this->select_states_.push_back(state);

		auto widgets = std::make_unique<Widgets>();

		std::string name = "player_" + std::to_string(i + 1);
		std::wstring label_text = this->level_settings()->player_label(i);

		widgets->player = std::make_unique<UiTextDropShadow>(
			name,
			label_text,
			this->menu_style().detail_font,
			this->calculate_widget_position(0, i + 2),
			this->render_resources(),
			this->look().weapon_select.unselected);

		widgets->weapon_icon = std::make_unique<MenuIcon>(
			name + "_wep_icon",
			"sprayer",
			RectangleF(this->calculate_widget_position(2, i + 2), this->look().weapon_select.size),
			this->render_resources(),
			this->look().weapon_select.unselected, &this->menu_style(), "weapon_paint");
		widgets->weapon_icon->set_paint_colour(preview_colours.team_colour(
			this->level_settings()->player_team(i)));

		widgets->weapon_name = std::make_unique<UiTextDropShadow>(
			name + "_wep_name",
			L"Sprayer",
			this->look().weapon_description_font,
			widgets->weapon_icon->rectangle().position() +
				Vector2F(0.0f, this->look().weapon_select.size.y +
					this->menu_style().icon_padding.y + this->menu_style().rule_height),
			this->render_resources(),
			this->look().weapon_select.unselected,
			this->menu_style().shadow_colour,
			this->menu_style().detail_shadow_offset);

		widgets->weapon_description = std::make_unique<UiTextDropShadow>(
			name + "_wep_desc",
			this->weapon_description(WeaponType::sprayer),
			this->look().weapon_description_font,
			widgets->weapon_icon->rectangle().position() +
				Vector2F(this->look().weapon_description_x_offset, 0.0f),
			this->render_resources(),
			this->look().weapon_description_colour,
			this->menu_style().shadow_colour,
			this->look().weapon_description_shadow_offset);

		this->texture_container_->add_child(widgets->weapon_icon.get());
		this->text_container_->add_child(widgets->weapon_name.get());
		this->text_container_->add_child(widgets->player.get());
		this->text_container_->add_child(widgets->weapon_description.get());

		this->player_widgets_.push_back(std::move(widgets));
	}
	this->update_weapon_select_visuals();
	this->play_music(this->music_, this->look().music.volume);
}
std::wstring MainMenuWeaponSelect::weapon_description(WeaponType type) const
{
	const size_t index = static_cast<size_t>(type);
	const auto& descriptions = this->look().weapon_descriptions;
	if (index >= descriptions.size())
	{
		return std::wstring();
	}
	return descriptions[index];
}

#pragma endregion MainMenuWeaponSelect

#pragma region MainMenuStageSelect

MainMenuStageSelect::MainMenuStageSelect(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuStageSelect::update(float /*dt*/)
{
	const std::vector<ProcessedMenuInput> inputs =
		this->input()->player_inputs(*this->level_settings());
	if (this->wait_for_players(inputs))
	{
		return;
	}
	const int num_inputs = static_cast<int>(inputs.size());
	const int player_count = this->player_count();

	for (int i = 0; i < num_inputs && i < player_count; i++)
	{
		if (inputs[i].action == MenuInputAction::back)
		{
			if (this->select_state_.state == ConfirmationState::unconfirmed)
			{
				this->play_wave(this->cancel_sound_);
				this->context()->transition_to(
					std::make_unique<MainMenuWeaponSelect>(
						this->menu_context()));
				return;
			}
			if (this->select_state_.state == ConfirmationState::confirmed)
			{
				this->play_wave(this->cancel_sound_);
				this->select_state_.state =
					ConfirmationState::unconfirmed;
			}
			// This selection is shared. Show its new state before another
			// action can leave the page, even if two players pressed together.
			break;
		}
		else if (inputs[i].action == MenuInputAction::proceed)
		{
			if (this->select_state_.state == ConfirmationState::confirmed)
			{
				this->play_wave(this->confirm_sound_);
				this->stop_effect(this->music_, true);
				if (this->select_state_.slot == this->random_slot())
				{
					this->select_state_.slot = this->pick_random_stage();
				}
				this->set_level_settings();

				// Settings are already in GameMenu's context. Pop once and return so
				// simultaneous inputs cannot also pop the state underneath.
				this->context()->pop();
				return;
			}
			else
			{
				this->play_wave(this->ready_sound_);
				this->select_state_.state = ConfirmationState::confirmed;
			}
			// READY must be visible before a later press starts the match.
			break;
		}
		else if (this->select_state_.state == ConfirmationState::unconfirmed)
		{
			if (inputs[i].direction == labrador::Direction::left)
			{
				this->play_wave(this->direction_sound_);
				this->cycle_stages(labrador::Direction::left);
			}
			else if (inputs[i].direction == labrador::Direction::right)
			{
				this->play_wave(this->direction_sound_);
				this->cycle_stages(labrador::Direction::right);
			}
		}
	}
	this->update_stage_select_visuals();
}
void MainMenuStageSelect::set_level_settings() const
{
	this->level_settings()->set_stage(
		this->data()->stages()->at(
			static_cast<size_t>(this->select_state_.slot)).name);
}
void MainMenuStageSelect::update_stage_select_visuals()
{
	const int slot = this->select_state_.slot;
	const ConfirmationState state = this->select_state_.state;
	if (slot == this->displayed_slot_ && state == this->displayed_state_)
	{
		return;
	}
	const bool random = slot == this->random_slot();
	const bool confirmed = state == ConfirmationState::confirmed;
	const StageSelectPresentation& look = this->look().stage_select;
	this->stage_preview_->set_stage(random ? std::string() :
		this->data()->stages()->at(static_cast<size_t>(slot)).name);
	this->stage_name_->set_text(this->slot_display_name(slot));
	// User-authored stage names share the preview's width with the briefing.
	const RectangleF name_bounds = RectangleF::union_of(
		this->stage_name_->text_bounds_at(Vector2F::ZERO, 1.0f),
		this->stage_name_->text_bounds_at(this->menu_style().item_shadow_offset, 1.0f));
	const float name_scale = std::min(1.0f, look.preview_size.x / name_bounds.width);
	this->stage_name_->set_scale(name_scale);
	this->stage_name_->set_shadow_scale(name_scale);
	this->stage_name_->set_shadow_offset(this->menu_style().item_shadow_offset * name_scale);
	this->stage_name_->set_colour(confirmed ? look.selected : look.unselected);
	this->random_mark_->set_hidden(!random);
	this->navigation_->set_text(random ? L"<   SURPRISE ME   >" :
		L"<   ARENA " + std::to_wstring(slot + 1) + L" / " +
		std::to_wstring(this->random_slot()) + L"   >");
	this->preview_edge_->set_colour(confirmed ? look.selected :
		this->menu_style().panel_edge_colour);
	this->selection_status_->set_text(confirmed ? L"ARENA LOCKED" : L"FIND YOUR ARENA");
	this->selection_status_->set_colour(confirmed ? look.selected :
		this->menu_style().heading_colour);
	this->selection_hint_->set_text(confirmed ?
		L"Confirm again to start.\nBack to change your choice." : random ?
		L"Random: " + std::to_wstring(random_stage_indices(*this->data()->stages()).size()) +
		L" available arenas.\nConfirm to lock your choice." :
		L"Left/Right to browse.\nConfirm to lock your choice.");
	this->set_player_prompts(confirmed ? std::vector<PromptBar::Prompt>{
		{ L"A", L"Start match" }, { L"B", L"Unlock arena" } } :
		std::vector<PromptBar::Prompt>{ { L"Left/Right", L"Browse arenas" },
			{ L"A", L"Lock arena" }, { L"B", L"Back" } });
	this->displayed_slot_ = slot;
	this->displayed_state_ = state;
}
int MainMenuStageSelect::slot_of(const std::string& stage_name) const
{
	const StageList* stages = this->data()->stages();
	for (size_t i = 0; i < stages->size(); i++)
	{
		if (stages->at(i).name == stage_name)
		{
			return static_cast<int>(i);
		}
	}
	return 0;
}
int MainMenuStageSelect::random_slot() const
{
	return static_cast<int>(this->data()->stages()->size());
}
std::wstring MainMenuStageSelect::slot_display_name(int slot) const
{
	if (slot == this->random_slot())
	{
		return L"Random";
	}
	const StageList* stages = this->data()->stages();
	if (slot >= 0 && slot < static_cast<int>(stages->size()))
	{
		// Startup validates stage names with this same text conversion.
		return drawable_text::widen_drawable(
			stages->at(static_cast<size_t>(slot)).display_name,
			"the stage list");
	}
	return L"ERROR";
}
int MainMenuStageSelect::pick_random_stage() const
{
	return static_cast<int>(random_stage_index(*this->data()->stages(),
		static_cast<size_t>(rand())));
}
void MainMenuStageSelect::init()
{
	this->select_state_.slot = this->slot_of(this->level_settings()->stage());
	const MenuStyle& style = this->menu_style();
	const StageSelectPresentation& look = this->look().stage_select;
	RenderResources* resources = this->render_resources();

	this->background_ = std::make_unique<UiTexture>("background",
		this->look().backdrop_sheet, this->look().backdrop_frame,
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION), resources,
		this->look().play_background_colour);
	this->heading_ = std::make_unique<UiTextDropShadow>("heading",
		L"Choose your arena", style.heading_font,
		this->calculate_widget_position(0, 0), resources,
		style.heading_colour, style.shadow_colour, style.heading_shadow_offset);

	const RectangleF preview(look.preview_position, look.preview_size);
	RectangleF edge = preview;
	edge.inflate(style.rule_height);
	this->preview_edge_ = std::make_unique<UiTexture>("preview_edge",
		"sprite_sheet_1", "pixel", edge, resources, style.panel_edge_colour);
	this->preview_background_ = std::make_unique<UiTexture>("preview_background",
		"sprite_sheet_1", "pixel", preview, resources,
		this->presentation().hud().player_identity_background_colour);
	this->stage_preview_ = std::make_unique<StagePreview>("stage_preview",
		preview, this->data());
	this->stage_name_ = std::make_unique<UiTextDropShadow>("stage_name",
		L"", style.item_font, look.name_position, resources,
		look.unselected, style.shadow_colour, style.item_shadow_offset);
	this->navigation_ = std::make_unique<UiTextDropShadow>("navigation",
		L"", style.detail_font, look.navigation_position, resources,
		style.highlight, style.shadow_colour, style.detail_shadow_offset);
	this->random_mark_ = std::make_unique<UiTextDropShadow>("random_mark",
		L"?", this->look().announcement_font, Vector2F::ZERO, resources,
		style.highlight, style.shadow_colour, style.title_shadow_offset);
	this->random_mark_->set_position(preview.center() -
		this->random_mark_->bounds().center());

	const RectangleF briefing(look.briefing.position, look.briefing.size);
	this->briefing_background_ = std::make_unique<UiTexture>("briefing_background",
		"sprite_sheet_1", "pixel", briefing, resources,
		this->presentation().hud().player_identity_background_colour);
	this->briefing_accent_ = std::make_unique<UiTexture>("briefing_accent",
		"sprite_sheet_1", "pixel", RectangleF(briefing.position(),
			Vector2F(briefing.width, style.accent_width)), resources, style.highlight);
	const Vector2F info = briefing.position() + look.briefing_padding;
	const Vector2F row = look.briefing.spacing;

	this->texture_container_ = std::make_unique<UiContainer>("texture_container");
	for (UiObject* widget : std::initializer_list<UiObject*>{ this->background_.get(),
		this->preview_edge_.get(), this->preview_background_.get(),
		this->stage_preview_.get(), this->briefing_background_.get(),
		this->briefing_accent_.get() })
	{
		this->texture_container_->add_child(widget);
	}
	this->text_container_ = std::make_unique<UiContainer>("text_container");
	for (UiObject* widget : std::initializer_list<UiObject*>{ this->heading_.get(),
		this->stage_name_.get(), this->navigation_.get(), this->random_mark_.get() })
	{
		this->text_container_->add_child(widget);
	}

	const auto add_briefing = [this, resources, &style, &info, &row](
		const std::string& name, const std::wstring& text,
		const std::string& font, float line, const Colour& colour)
	{
		auto label = std::make_unique<UiTextDropShadow>(name, text, font,
			info + row * line, resources, colour, style.shadow_colour,
			style.detail_shadow_offset);
		this->text_container_->add_child(label.get());
		this->briefing_text_.push_back(std::move(label));
	};
	const bool solo = this->player_count() == 1;
	const int best_of = this->level_settings()->best_of();
	add_briefing("briefing_label", L"MATCH SETUP", style.brand_font, 0.0f,
		style.highlight);
	add_briefing("mode", solo ? L"SOLO PRACTICE" : best_of == 1 ?
		L"SINGLE MATCH" : L"BEST OF " + std::to_wstring(best_of),
		style.detail_font, 1.0f, style.heading_colour);
	add_briefing("roster", std::to_wstring(this->player_count()) +
		(solo ? L" PLAYER" : L" PLAYERS / 2 TEAMS"),
		style.brand_font, 2.0f, style.unhighlight);
	add_briefing("goal", L"PAINT TO WIN", style.brand_font, 3.0f, style.highlight);
	add_briefing("objective", solo ?
		L"Make the arena your canvas.\nTry weapons. Cover more ground." :
		L"Cover the arena in your colour.\nMost painted surface wins.",
		style.brand_font, 4.0f, style.unhighlight);

	this->selection_status_ = std::make_unique<UiTextDropShadow>("selection_status",
		L"", style.detail_font, info + row * 6.0f, resources,
		style.heading_colour, style.shadow_colour, style.detail_shadow_offset);
	this->selection_hint_ = std::make_unique<UiTextDropShadow>("selection_hint",
		L"", style.brand_font, info + row * 7.0f, resources,
		style.unhighlight, style.shadow_colour, style.detail_shadow_offset);
	this->text_container_->add_child(this->selection_status_.get());
	this->text_container_->add_child(this->selection_hint_.get());
	this->update_stage_select_visuals();
	this->play_music(this->music_, this->look().music.volume);
}
void MainMenuStageSelect::cycle_stages(labrador::Direction direction)
{
	// Slots run 0..n-1 for the stages and n for Random, and the cursor wraps.
	const int last = random_stage_indices(*this->data()->stages()).empty() ?
		this->random_slot() - 1 : this->random_slot();
	int slot = this->select_state_.slot;
	if (direction == labrador::Direction::left)
	{
		slot = (slot == 0) ? last : slot - 1;
	}
	else if (direction == labrador::Direction::right)
	{
		slot = (slot == last) ? 0 : slot + 1;
	}
	this->select_state_.slot = slot;
}

#pragma endregion MainMenuStageSelect
