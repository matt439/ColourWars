#include "game/pch.h"
#include "game/states/pause_menu.h"

using namespace DirectX;
using namespace MattMath;
using namespace pause_menu_consts;
using namespace colour_consts;

PauseMenuPage::PauseMenuPage(PauseMenuData* data) :
	MenuPage(data),
	SoundBankObject(pause_menu_consts::SOUND_BANK,
		this->get_audio_resources()),
	data_(data)
{
	this->direction_sound_ = this->resolve_wave(DIRECTION_SOUND);
	this->confirm_sound_ = this->resolve_wave(CONFIRM_SOUND);
	this->cancel_sound_ = this->resolve_wave(CANCEL_SOUND);
	this->window_open_sound_ = this->resolve_wave(WINDOW_OPEN_SOUND);
}

std::string PauseMenuPage::get_player_number_text(int player_num)
{
	switch (player_num)
	{
	case 0:
		return "Pause Menu P1";
	case 1:
		return "Pause Menu P2";
	case 2:
		return "Pause Menu P3";
	case 3:
		return "Pause Menu P4";
	default:
		return "Pause Menu P error";
	};
}

PauseMenuData* PauseMenuPage::get_pause_menu_data() const
{
	return this->data_;
}

ProcessedMenuInput PauseMenuPage::get_pausing_player_input(
	const std::vector<ProcessedMenuInput>& inputs) const
{
	const int player_num = this->get_pause_menu_data()->get_player_num();

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
		this->get_pausing_player_input(this->get_menu_inputs());
	std::string highlighted_element =
		this->get_highlighted_widget()->get_name();

	if (input.action == menu_input_action::BACK ||
		input.action == menu_input_action::PAUSE)
	{
		this->play_wave(this->cancel_sound_);
		*this->get_pause_menu_data()->get_action() = pause_menu_action::RESUME;
	}
	else if (input.action == menu_input_action::PROCEED)
	{
		if (highlighted_element == "resume")
		{
			this->play_wave(this->confirm_sound_);
			*this->get_pause_menu_data()->get_action() =
				pause_menu_action::RESUME;
		}
		else if (highlighted_element == "restart")
		{
			this->play_wave(this->confirm_sound_);
			this->get_context()->transition_to(std::make_unique<
				PauseMenuConfirmation>(this->get_pause_menu_data(),
					confirmation_type::RESTART));
		}
		else if (highlighted_element == "quit")
		{
			this->play_wave(this->cancel_sound_);
			this->get_context()->transition_to(std::make_unique<
				PauseMenuConfirmation>(this->get_pause_menu_data(),
					confirmation_type::QUIT));
		}
	}
	else if (input.direction == menu_direction::UP)
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
	else if (input.direction == menu_direction::DOWN)
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

	this->box_ = std::make_unique<MTexture>(
		"box",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, PAUSE_MENU_BOX_SIZE),
		this->get_render_resources(),
		PAUSE_MENU_BOX_COLOUR);
	this->box_->set_position_at_center(DEFAULT_RESOLUTION / 2.0f);

	this->set_widget_position(PAUSE_MENU_INITIAL_WIDGET_POSITION);
	this->set_widget_size(PAUSE_MENU_INITIAL_WIDGET_SIZE);
	this->set_widget_spacing(PAUSE_MENU_INITIAL_WIDGET_SPACING);

	this->player_num_ = std::make_unique<MTextDropShadow>(
		"player_num",
		this->get_player_number_text(
			this->get_pause_menu_data()->get_player_num()),
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->get_render_resources(),
		PAUSE_MENU_HEADING_TEXT_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->resume_ = std::make_unique<MTextDropShadow>(
		"resume",
		"Resume",
		ITEM_FONT,
		this->calculate_widget_position(1, 2),
		this->get_render_resources(),
		this->get_highlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->restart_ = std::make_unique<MTextDropShadow>(
		"restart",
		"Restart",
		ITEM_FONT,
		this->calculate_widget_position(1, 3),
		this->get_render_resources(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->quit_ = std::make_unique<MTextDropShadow>(
		"quit",
		"Quit",
		ITEM_FONT,
		this->calculate_widget_position(1, 4),
		this->get_render_resources(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->set_highlighted_widget(this->resume_.get());

	const Vector2F resolution = this->get_float_resolution();

	this->texture_container_ = std::make_unique<MContainer>(
		"texture_container");
	this->texture_container_->add_child(this->box_.get());

	this->text_container_ = std::make_unique<MContainer>(
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
	std::vector<std::pair<MObject*, ID3D11SamplerState*>> mobjects;

	mobjects.push_back(std::make_pair(this->texture_container_.get(),
		this->get_point_clamp_sampler_state()));

	mobjects.push_back(std::make_pair(this->text_container_.get(), nullptr));

	this->draw_mobjects_in_viewports(&mobjects);
}

PauseMenuConfirmation::PauseMenuConfirmation(PauseMenuData* data,
	confirmation_type type) :
	PauseMenuPage(data), type_(type)
{

}

void PauseMenuConfirmation::update()
{
	const ProcessedMenuInput input =
		this->get_pausing_player_input(this->get_menu_inputs());
	std::string highlighted_element =
		this->get_highlighted_widget()->get_name();

	if (input.action == menu_input_action::BACK)
	{
		this->play_wave(this->cancel_sound_);
		this->get_context()->transition_to(std::make_unique<PauseMenuInitial>(
			this->get_pause_menu_data()));
		return;
	}
	else if (input.action == menu_input_action::PROCEED)
	{
		if (highlighted_element == "yes")
		{
			switch (this->type_)
			{
			case confirmation_type::RESTART:
				this->play_wave(this->confirm_sound_);
				*this->get_pause_menu_data()->get_action() =
					pause_menu_action::RESTART;
				break;
			case confirmation_type::QUIT:
				this->play_wave(this->cancel_sound_);
				*this->get_pause_menu_data()->get_action() =
					pause_menu_action::QUIT;
				break;
			}
		}
		else if (highlighted_element == "no")
		{
			this->play_wave(this->cancel_sound_);
			this->get_context()->transition_to(
				std::make_unique<PauseMenuInitial>(
					this->get_pause_menu_data()));
			return;
		}
	}
	else if (input.direction == menu_direction::UP ||
		input.direction == menu_direction::DOWN)
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

	this->box_ = std::make_unique<MTexture>(
		"box",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, PAUSE_MENU_BOX_SIZE),
		this->get_render_resources(),
		PAUSE_MENU_BOX_COLOUR);
	this->box_->set_position_at_center(DEFAULT_RESOLUTION / 2.0f);

	this->set_widget_position(PAUSE_MENU_INITIAL_WIDGET_POSITION);
	this->set_widget_size(PAUSE_MENU_INITIAL_WIDGET_SIZE);
	this->set_widget_spacing(PAUSE_MENU_INITIAL_WIDGET_SPACING);

	this->player_num_ = std::make_unique<MTextDropShadow>(
		"player_num",
		this->get_player_number_text(
			this->get_pause_menu_data()->get_player_num()),
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->get_render_resources(),
		PAUSE_MENU_HEADING_TEXT_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->question_ = std::make_unique<MTextDropShadow>(
		"question",
		get_question_text(this->type_),
		DETAIL_FONT,
		this->calculate_widget_position(0, 2),
		this->get_render_resources(),
		PAUSE_MENU_HEADING_TEXT_COLOUR,
		SHADOW_COLOUR,
		DETAIL_SHADOW_OFFSET);

	this->yes_ = std::make_unique<MTextDropShadow>(
		"yes",
		"Yes",
		ITEM_FONT,
		this->calculate_widget_position(1, 3),
		this->get_render_resources(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->no_ = std::make_unique<MTextDropShadow>(
		"no",
		"No",
		ITEM_FONT,
		this->calculate_widget_position(1, 4),
		this->get_render_resources(),
		this->get_highlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->set_highlighted_widget(this->no_.get());

	this->texture_container_ = std::make_unique<MContainer>(
		"texture_container");
	this->texture_container_->add_child(this->box_.get());

	this->text_container_ = std::make_unique<MContainer>(
		"text_container");
	this->text_container_->add_child(this->player_num_.get());
	this->text_container_->add_child(this->question_.get());
	this->text_container_->add_child(this->yes_.get());
	this->text_container_->add_child(this->no_.get());

	const Vector2F resolution = this->get_float_resolution();
	this->texture_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->text_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
}

void PauseMenuConfirmation::draw()
{
	std::vector<std::pair<MObject*, ID3D11SamplerState*>> mobjects;

	mobjects.push_back(std::make_pair(this->texture_container_.get(),
		this->get_point_clamp_sampler_state()));

	mobjects.push_back(std::make_pair(this->text_container_.get(), nullptr));

	this->draw_mobjects_in_viewports(&mobjects);
}

std::string PauseMenuConfirmation::get_question_text(
	confirmation_type type)
{
	switch (type)
	{
	case confirmation_type::RESTART:
		return "Are you sure you want to restart?";
	case confirmation_type::QUIT:
		return "Are you sure you want to quit?";
	default:
		return "Error";
	}
}