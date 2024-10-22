#include "pch.h"
#include "PauseMenu.h"

using namespace DirectX;
using namespace MattMath;
using namespace pause_menu_consts;
using namespace colour_consts;

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
	return this->_data;
}

void PauseMenuInitial::update()
{
	std::vector<ProcessedMenuInput> inputs = this->get_menu_inputs();
	int player_num = this->get_pause_menu_data()->get_player_num();
	std::string highlighted_element =
		this->get_highlighted_widget()->get_name();

	if (inputs[player_num].action == menu_input_action::BACK ||
		inputs[player_num].action == menu_input_action::PAUSE)
	{
		this->play_wave(CANCEL_SOUND);
		*this->get_pause_menu_data()->get_action() = pause_menu_action::RESUME;
	}
	else if (inputs[player_num].action == menu_input_action::PROCEED)
	{
		if (highlighted_element == "resume")
		{
			this->play_wave(CONFIRM_SOUND);
			*this->get_pause_menu_data()->get_action() =
				pause_menu_action::RESUME;
		}
		else if (highlighted_element == "restart")
		{
			this->play_wave(CONFIRM_SOUND);
			this->get_context()->transition_to(std::make_unique<
				PauseMenuConfirmation>(this->get_pause_menu_data(),
					confirmation_type::RESTART));
		}
		else if (highlighted_element == "quit")
		{
			this->play_wave(CANCEL_SOUND);
			this->get_context()->transition_to(std::make_unique<
				PauseMenuConfirmation>(this->get_pause_menu_data(),
					confirmation_type::QUIT));
		}
	}
	else if (inputs[player_num].direction == menu_direction::UP)
	{
		this->play_wave(DIRECTION_SOUND);
		if (highlighted_element == "resume")
		{
			this->change_highlight(this->_quit.get());
		}
		else if (highlighted_element == "restart")
		{
			this->change_highlight(this->_resume.get());
		}
		else if (highlighted_element == "quit")
		{
			this->change_highlight(this->_restart.get());
		}
	}
	else if (inputs[player_num].direction == menu_direction::DOWN)
	{
		this->play_wave(DIRECTION_SOUND);
		if (highlighted_element == "resume")
		{
			this->change_highlight(this->_restart.get());
		}
		else if (highlighted_element == "restart")
		{
			this->change_highlight(this->_quit.get());
		}
		else if (highlighted_element == "quit")
		{
			this->change_highlight(this->_resume.get());
		}
	}
}
void PauseMenuInitial::init()
{
	this->set_highlight_colour(STANDARD_HIGHLIGHT);
	this->set_unhighlight_colour(STANDARD_UNHIGHLIGHT);

	this->_box = std::make_unique<MTexture>(
		"box",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, PAUSE_MENU_BOX_SIZE),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		PAUSE_MENU_BOX_COLOUR);
	this->_box->set_position_at_center(DEFAULT_RESOLUTION / 2.0f);

	this->set_widget_position(PAUSE_MENU_INITIAL_WIDGET_POSITION);
	this->set_widget_size(PAUSE_MENU_INITIAL_WIDGET_SIZE);
	this->set_widget_spacing(PAUSE_MENU_INITIAL_WIDGET_SPACING);

	this->_player_num = std::make_unique<MTextDropShadow>(
		"player_num",
		this->get_player_number_text(
			this->get_pause_menu_data()->get_player_num()),
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		PAUSE_MENU_HEADING_TEXT_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->_resume = std::make_unique<MTextDropShadow>(
		"resume",
		"Resume",
		ITEM_FONT,
		this->calculate_widget_position(1, 2),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_highlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_restart = std::make_unique<MTextDropShadow>(
		"restart",
		"Restart",
		ITEM_FONT,
		this->calculate_widget_position(1, 3),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_quit = std::make_unique<MTextDropShadow>(
		"quit",
		"Quit",
		ITEM_FONT,
		this->calculate_widget_position(1, 4),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->set_highlighted_widget(this->_resume.get());

	const Vector2F resolution = this->get_float_resolution();

	this->_texture_container = std::make_unique<MContainer>(
		"texture_container");
	this->_texture_container->add_child(this->_box.get());

	this->_text_container = std::make_unique<MContainer>(
		"text_container");
	this->_text_container->add_child(this->_player_num.get());
	this->_text_container->add_child(this->_resume.get());
	this->_text_container->add_child(this->_restart.get());
	this->_text_container->add_child(this->_quit.get());

	this->_texture_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->_text_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);

	this->play_wave(WINDOW_OPEN_SOUND);
}
void PauseMenuInitial::draw()
{
	this->draw_mobject_in_viewports(this->_texture_container.get(),
		this->get_point_clamp_sampler_state());

	// draw text separately to use blend state
	this->draw_mobject_in_viewports(this->_text_container.get());
}

void PauseMenuConfirmation::update()
{
	std::vector<ProcessedMenuInput> inputs = this->get_menu_inputs();
	int player_num = this->get_pause_menu_data()->get_player_num();
	std::string highlighted_element =
		this->get_highlighted_widget()->get_name();

	if (inputs[player_num].action == menu_input_action::BACK)
	{
		this->play_wave(CANCEL_SOUND);
		this->get_context()->transition_to(std::make_unique<PauseMenuInitial>(
			this->get_pause_menu_data()));
		return;
	}
	else if (inputs[player_num].action == menu_input_action::PROCEED)
	{
		if (highlighted_element == "yes")
		{
			switch (this->_type)
			{
			case confirmation_type::RESTART:
				this->play_wave(CONFIRM_SOUND);
				*this->get_pause_menu_data()->get_action() =
					pause_menu_action::RESTART;
				break;
			case confirmation_type::QUIT:
				this->play_wave(CANCEL_SOUND);
				*this->get_pause_menu_data()->get_action() =
					pause_menu_action::QUIT;
				break;
			}
		}
		else if (highlighted_element == "no")
		{
			this->play_wave(CANCEL_SOUND);
			this->get_context()->transition_to(
				std::make_unique<PauseMenuInitial>(
					this->get_pause_menu_data()));
			return;
		}
	}
	else if (inputs[player_num].direction == menu_direction::UP ||
		inputs[player_num].direction == menu_direction::DOWN)
	{
		this->play_wave(DIRECTION_SOUND);
		if (highlighted_element == "yes")
		{
			this->change_highlight(this->_no.get());
		}
		else if (highlighted_element == "no")
		{
			this->change_highlight(this->_yes.get());
		}
	}
}

void PauseMenuConfirmation::init()
{
	this->set_highlight_colour(STANDARD_HIGHLIGHT);
	this->set_unhighlight_colour(STANDARD_UNHIGHLIGHT);

	this->_box = std::make_unique<MTexture>(
		"box",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, PAUSE_MENU_BOX_SIZE),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		PAUSE_MENU_BOX_COLOUR);
	this->_box->set_position_at_center(DEFAULT_RESOLUTION / 2.0f);

	this->set_widget_position(PAUSE_MENU_INITIAL_WIDGET_POSITION);
	this->set_widget_size(PAUSE_MENU_INITIAL_WIDGET_SIZE);
	this->set_widget_spacing(PAUSE_MENU_INITIAL_WIDGET_SPACING);

	this->_player_num = std::make_unique<MTextDropShadow>(
		"player_num",
		this->get_player_number_text(
			this->get_pause_menu_data()->get_player_num()),
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		PAUSE_MENU_HEADING_TEXT_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->_question = std::make_unique<MTextDropShadow>(
		"question",
		get_question_text(this->_type),
		DETAIL_FONT,
		this->calculate_widget_position(0, 2),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		PAUSE_MENU_HEADING_TEXT_COLOUR,
		SHADOW_COLOUR,
		DETAIL_SHADOW_OFFSET);

	this->_yes = std::make_unique<MTextDropShadow>(
		"yes",
		"Yes",
		ITEM_FONT,
		this->calculate_widget_position(1, 3),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_no = std::make_unique<MTextDropShadow>(
		"no",
		"No",
		ITEM_FONT,
		this->calculate_widget_position(1, 4),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_highlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->set_highlighted_widget(this->_no.get());

	this->_texture_container = std::make_unique<MContainer>(
		"texture_container");
	this->_texture_container->add_child(this->_box.get());

	this->_text_container = std::make_unique<MContainer>(
		"text_container");
	this->_text_container->add_child(this->_player_num.get());
	this->_text_container->add_child(this->_question.get());
	this->_text_container->add_child(this->_yes.get());
	this->_text_container->add_child(this->_no.get());

	const Vector2F resolution = this->get_float_resolution();
	this->_texture_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->_text_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
}

void PauseMenuConfirmation::draw()
{
	this->draw_mobject_in_viewports(this->_texture_container.get(),
		this->get_point_clamp_sampler_state());

	// draw text separately to use blend state
	this->draw_mobject_in_viewports(this->_text_container.get());
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