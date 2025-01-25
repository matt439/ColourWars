#include "pch.h"
#include "EndMenu.h"

using namespace MattMath;
using namespace colour_consts;
using namespace end_menu_consts;

EndMenuPage::EndMenuPage(EndMenuData* data) :
	MenuPage(data),
	SoundBankObject(SOUND_BANK, this->get_resource_manager()),
	_data(data)
{

}

EndMenuData* EndMenuPage::get_end_menu_data() const
{
	return this->_data;
}

EndMenuInitial::EndMenuInitial(EndMenuData* data) : EndMenuPage(data)
{

}

void EndMenuInitial::update()
{
	std::vector<ProcessedMenuInput> inputs = this->get_menu_inputs();
	std::string highlighted_element =
		this->get_highlighted_widget()->get_name();

	for (int i = 0; i < inputs.size(); i++)
	{
		if (inputs[i].action == menu_input_action::PROCEED)
		{
			if (highlighted_element == "change_teams")
			{
				this->play_wave(CONFIRM_SOUND);
				*this->get_end_menu_data()->get_action() =
					end_menu_action::CHANGE_TEAMS;
				return;
			}
			else if (highlighted_element == "change_weapons")
			{
				this->play_wave(CONFIRM_SOUND);
				*this->get_end_menu_data()->get_action() =
					end_menu_action::CHANGE_WEAPONS;
				return;
			}
			else if (highlighted_element == "change_level")
			{
				this->play_wave(CONFIRM_SOUND);
				*this->get_end_menu_data()->get_action() =
					end_menu_action::CHANGE_LEVEL;
				return;
			}
			else if (highlighted_element == "restart")
			{
				this->play_wave(CONFIRM_SOUND);
				*this->get_end_menu_data()->get_action() =
					end_menu_action::RESTART;
				return;
			}
			else if (highlighted_element == "exit")
			{
				this->play_wave(CANCEL_SOUND);
				*this->get_end_menu_data()->get_action() =
					end_menu_action::EXIT;
				return;
			}
		}
		else if (inputs[i].direction == menu_direction::UP)
		{
			this->play_wave(DIRECTION_SOUND);
			if (highlighted_element == "change_teams")
			{
				this->change_highlight(this->_exit.get());
				return;
			}
			else if (highlighted_element == "change_weapons")
			{
				this->change_highlight(this->_change_teams.get());
				return;
			}
			else if (highlighted_element == "change_level")
			{
				this->change_highlight(this->_change_weapons.get());
				return;
			}
			else if (highlighted_element == "restart")
			{
				this->change_highlight(this->_change_level.get());
				return;
			}
			else if (highlighted_element == "exit")
			{
				this->change_highlight(this->_restart.get());
				return;
			}
		}
		else if (inputs[i].direction == menu_direction::DOWN)
		{
			this->play_wave(DIRECTION_SOUND);
			if (highlighted_element == "change_teams")
			{
				this->change_highlight(this->_change_weapons.get());
				return;
			}
			else if (highlighted_element == "change_weapons")
			{
				this->change_highlight(this->_change_level.get());
				return;
			}
			else if (highlighted_element == "change_level")
			{
				this->change_highlight(this->_restart.get());
				return;
			}
			else if (highlighted_element == "restart")
			{
				this->change_highlight(this->_exit.get());
				return;
			}
			else if (highlighted_element == "exit")
			{
				this->change_highlight(this->_change_teams.get());
				return;
			}
		}
	}
}
void EndMenuInitial::init()
{
	this->set_highlight_colour(STANDARD_HIGHLIGHT);
	this->set_unhighlight_colour(STANDARD_UNHIGHLIGHT);

	this->_box = std::make_unique<MTexture>(
		"box",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, END_MENU_BOX_SIZE),
		this->get_resource_manager(),
		END_MENU_BOX_COLOUR);
	this->_box->set_position_at_center(DEFAULT_RESOLUTION / 2.0f);

	this->set_widget_position(END_MENU_INITIAL_WIDGET_POSITION);
	this->set_widget_size(END_MENU_INITIAL_WIDGET_SIZE);
	this->set_widget_spacing(END_MENU_INITIAL_WIDGET_SPACING);

	this->_heading = std::make_unique<MTextDropShadow>(
		"heading",
		"Level End",
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->get_resource_manager(),
		END_MENU_HEADING_TEXT_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->_change_teams = std::make_unique<MTextDropShadow>(
		"change_teams",
		"Change Teams",
		ITEM_FONT,
		this->calculate_widget_position(0, 2),
		this->get_resource_manager(),
		this->get_highlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_change_weapons = std::make_unique<MTextDropShadow>(
		"change_weapons",
		"Change Weapons",
		ITEM_FONT,
		this->calculate_widget_position(0, 3),
		this->get_resource_manager(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_change_level = std::make_unique<MTextDropShadow>(
		"change_level",
		"Change Level",
		ITEM_FONT,
		this->calculate_widget_position(0, 4),
		this->get_resource_manager(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_restart = std::make_unique<MTextDropShadow>(
		"restart",
		"Restart",
		ITEM_FONT,
		this->calculate_widget_position(0, 5),
		this->get_resource_manager(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_exit = std::make_unique<MTextDropShadow>(
		"exit",
		"Exit to Main Menu",
		ITEM_FONT,
		this->calculate_widget_position(0, 6),
		this->get_resource_manager(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->set_highlighted_widget(this->_change_teams.get());

	this->_texture_container = std::make_unique<MContainer>(
		"texture_container");
	this->_texture_container->add_child(this->_box.get());

	this->_text_container = std::make_unique<MContainer>(
		"text_container");
	this->_text_container->add_child(this->_heading.get());
	this->_text_container->add_child(this->_change_teams.get());
	this->_text_container->add_child(this->_change_weapons.get());
	this->_text_container->add_child(this->_change_level.get());
	this->_text_container->add_child(this->_restart.get());
	this->_text_container->add_child(this->_exit.get());

	const Vector2F resolution = this->get_float_resolution();

	this->_texture_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->_text_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
}
void EndMenuInitial::draw()
{
	std::vector<std::pair<MObject*, ID3D11SamplerState*>> mobjects;

	mobjects.push_back(std::make_pair(this->_texture_container.get(),
		this->get_point_clamp_sampler_state()));

	mobjects.push_back(std::make_pair(this->_text_container.get(), nullptr));

	this->draw_mobjects_in_viewports(&mobjects);
}