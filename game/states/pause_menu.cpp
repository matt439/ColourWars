#include "game/pch.h"
#include "game/states/pause_menu.h"

using namespace DirectX;
using namespace mattmath;
using namespace pause_menu_consts;
using namespace colour_consts;
using namespace artattack;

PauseMenuPage::PauseMenuPage(PauseMenuData* data) :
	MenuPage(data),
	SoundBankObject(pause_menu_consts::SOUND_BANK,
		this->audio_resources()),
	data_(data)
{
	this->direction_sound_ = this->resolve_wave(DIRECTION_SOUND);
	this->confirm_sound_ = this->resolve_wave(CONFIRM_SOUND);
	this->cancel_sound_ = this->resolve_wave(CANCEL_SOUND);
	this->window_open_sound_ = this->resolve_wave(WINDOW_OPEN_SOUND);
}

std::wstring PauseMenuPage::player_number_text(int player_num)
{
	switch (player_num)
	{
	case 0:
		return L"Pause Menu P1";
	case 1:
		return L"Pause Menu P2";
	case 2:
		return L"Pause Menu P3";
	case 3:
		return L"Pause Menu P4";
	default:
		return L"Pause Menu P error";
	};
}

PauseMenuData* PauseMenuPage::pause_menu_data() const
{
	return this->data_;
}

ProcessedMenuInput PauseMenuPage::pausing_player_input(
	const std::vector<ProcessedMenuInput>& inputs) const
{
	const int player_num = this->pause_menu_data()->player_num();

	if (player_num < 0 ||
		static_cast<size_t>(player_num) >= inputs.size())
	{
		return ProcessedMenuInput();
	}
	return inputs[static_cast<size_t>(player_num)];
}

PauseMenuInitial::PauseMenuInitial(PauseMenuData* data) :
	PauseMenuPage(data)
{

}

void PauseMenuInitial::update()
{
	const ProcessedMenuInput input =
		this->pausing_player_input(this->menu_inputs());
	std::string highlighted_element =
		this->highlighted_widget()->name();

	if (input.action == MenuInputAction::back ||
		input.action == MenuInputAction::pause)
	{
		this->play_wave(this->cancel_sound_);
		*this->pause_menu_data()->action() = PauseMenuAction::resume;
	}
	else if (input.action == MenuInputAction::proceed)
	{
		if (highlighted_element == "resume")
		{
			this->play_wave(this->confirm_sound_);
			*this->pause_menu_data()->action() =
				PauseMenuAction::resume;
		}
		else if (highlighted_element == "restart")
		{
			this->play_wave(this->confirm_sound_);
			this->context()->transition_to(std::make_unique<
				PauseMenuConfirmation>(this->pause_menu_data(),
					ConfirmationType::restart));
		}
		else if (highlighted_element == "quit")
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(std::make_unique<
				PauseMenuConfirmation>(this->pause_menu_data(),
					ConfirmationType::quit));
		}
	}
	else if (input.direction == MenuDirection::up)
	{
		this->play_wave(this->direction_sound_);
		if (highlighted_element == "resume")
		{
			this->change_highlight(this->quit_.get());
		}
		else if (highlighted_element == "restart")
		{
			this->change_highlight(this->resume_.get());
		}
		else if (highlighted_element == "quit")
		{
			this->change_highlight(this->restart_.get());
		}
	}
	else if (input.direction == MenuDirection::down)
	{
		this->play_wave(this->direction_sound_);
		if (highlighted_element == "resume")
		{
			this->change_highlight(this->restart_.get());
		}
		else if (highlighted_element == "restart")
		{
			this->change_highlight(this->quit_.get());
		}
		else if (highlighted_element == "quit")
		{
			this->change_highlight(this->resume_.get());
		}
	}
}
void PauseMenuInitial::init()
{
	this->set_highlight_colour(STANDARD_HIGHLIGHT);
	this->set_unhighlight_colour(STANDARD_UNHIGHLIGHT);

	this->box_ = std::make_unique<UiTexture>(
		"box",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, PAUSE_MENU_BOX_SIZE),
		this->render_resources(),
		PAUSE_MENU_BOX_COLOUR);
	this->box_->set_position_at_center(DEFAULT_RESOLUTION / 2.0f);

	this->set_widget_position(PAUSE_MENU_INITIAL_WIDGET_POSITION);
	this->set_widget_size(PAUSE_MENU_INITIAL_WIDGET_SIZE);
	this->set_widget_spacing(PAUSE_MENU_INITIAL_WIDGET_SPACING);

	this->player_num_ = std::make_unique<UiTextDropShadow>(
		"player_num",
		this->player_number_text(
			this->pause_menu_data()->player_num()),
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		PAUSE_MENU_HEADING_TEXT_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->resume_ = std::make_unique<UiTextDropShadow>(
		"resume",
		L"Resume",
		ITEM_FONT,
		this->calculate_widget_position(1, 2),
		this->render_resources(),
		this->highlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->restart_ = std::make_unique<UiTextDropShadow>(
		"restart",
		L"Restart",
		ITEM_FONT,
		this->calculate_widget_position(1, 3),
		this->render_resources(),
		this->unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->quit_ = std::make_unique<UiTextDropShadow>(
		"quit",
		L"Quit",
		ITEM_FONT,
		this->calculate_widget_position(1, 4),
		this->render_resources(),
		this->unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->set_highlighted_widget(this->resume_.get());

	const Vector2F resolution = this->float_resolution();

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->box_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->player_num_.get());
	this->text_container_->add_child(this->resume_.get());
	this->text_container_->add_child(this->restart_.get());
	this->text_container_->add_child(this->quit_.get());

	this->texture_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->text_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);

	this->play_wave(this->window_open_sound_);
}
void PauseMenuInitial::draw()
{
	std::vector<std::pair<UiObject*, ID3D11SamplerState*>> ui_objects;

	ui_objects.push_back(std::make_pair(this->texture_container_.get(),
		this->point_clamp_sampler_state()));

	ui_objects.push_back(std::make_pair(this->text_container_.get(), nullptr));

	this->draw_ui_objects_in_viewports(&ui_objects);
}

PauseMenuConfirmation::PauseMenuConfirmation(PauseMenuData* data,
	ConfirmationType type) :
	PauseMenuPage(data), type_(type)
{

}

void PauseMenuConfirmation::update()
{
	const ProcessedMenuInput input =
		this->pausing_player_input(this->menu_inputs());
	std::string highlighted_element =
		this->highlighted_widget()->name();

	if (input.action == MenuInputAction::back)
	{
		this->play_wave(this->cancel_sound_);
		this->context()->transition_to(std::make_unique<PauseMenuInitial>(
			this->pause_menu_data()));
		return;
	}
	else if (input.action == MenuInputAction::proceed)
	{
		if (highlighted_element == "yes")
		{
			switch (this->type_)
			{
			case ConfirmationType::restart:
				this->play_wave(this->confirm_sound_);
				*this->pause_menu_data()->action() =
					PauseMenuAction::restart;
				break;
			case ConfirmationType::quit:
				this->play_wave(this->cancel_sound_);
				*this->pause_menu_data()->action() =
					PauseMenuAction::quit;
				break;
			}
		}
		else if (highlighted_element == "no")
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(
				std::make_unique<PauseMenuInitial>(
					this->pause_menu_data()));
			return;
		}
	}
	else if (input.direction == MenuDirection::up ||
		input.direction == MenuDirection::down)
	{
		this->play_wave(this->direction_sound_);
		if (highlighted_element == "yes")
		{
			this->change_highlight(this->no_.get());
		}
		else if (highlighted_element == "no")
		{
			this->change_highlight(this->yes_.get());
		}
	}
}

void PauseMenuConfirmation::init()
{
	this->set_highlight_colour(STANDARD_HIGHLIGHT);
	this->set_unhighlight_colour(STANDARD_UNHIGHLIGHT);

	this->box_ = std::make_unique<UiTexture>(
		"box",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, PAUSE_MENU_BOX_SIZE),
		this->render_resources(),
		PAUSE_MENU_BOX_COLOUR);
	this->box_->set_position_at_center(DEFAULT_RESOLUTION / 2.0f);

	this->set_widget_position(PAUSE_MENU_INITIAL_WIDGET_POSITION);
	this->set_widget_size(PAUSE_MENU_INITIAL_WIDGET_SIZE);
	this->set_widget_spacing(PAUSE_MENU_INITIAL_WIDGET_SPACING);

	this->player_num_ = std::make_unique<UiTextDropShadow>(
		"player_num",
		this->player_number_text(
			this->pause_menu_data()->player_num()),
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		PAUSE_MENU_HEADING_TEXT_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->question_ = std::make_unique<UiTextDropShadow>(
		"question",
		question_text(this->type_),
		DETAIL_FONT,
		this->calculate_widget_position(0, 2),
		this->render_resources(),
		PAUSE_MENU_HEADING_TEXT_COLOUR,
		SHADOW_COLOUR,
		DETAIL_SHADOW_OFFSET);

	this->yes_ = std::make_unique<UiTextDropShadow>(
		"yes",
		L"Yes",
		ITEM_FONT,
		this->calculate_widget_position(1, 3),
		this->render_resources(),
		this->unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->no_ = std::make_unique<UiTextDropShadow>(
		"no",
		L"No",
		ITEM_FONT,
		this->calculate_widget_position(1, 4),
		this->render_resources(),
		this->highlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->set_highlighted_widget(this->no_.get());

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->box_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->player_num_.get());
	this->text_container_->add_child(this->question_.get());
	this->text_container_->add_child(this->yes_.get());
	this->text_container_->add_child(this->no_.get());

	const Vector2F resolution = this->float_resolution();
	this->texture_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->text_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
}

void PauseMenuConfirmation::draw()
{
	std::vector<std::pair<UiObject*, ID3D11SamplerState*>> ui_objects;

	ui_objects.push_back(std::make_pair(this->texture_container_.get(),
		this->point_clamp_sampler_state()));

	ui_objects.push_back(std::make_pair(this->text_container_.get(), nullptr));

	this->draw_ui_objects_in_viewports(&ui_objects);
}

std::wstring PauseMenuConfirmation::question_text(
	ConfirmationType type)
{
	switch (type)
	{
	case ConfirmationType::restart:
		return L"Are you sure you want to restart?";
	case ConfirmationType::quit:
		return L"Are you sure you want to quit?";
	default:
		return L"Error";
	}
}