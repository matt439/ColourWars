#include "pch.h"
#include "MainMenu.h"

using namespace DirectX;
using namespace MattMath;
using namespace main_menu_consts;
using namespace colour_consts;

MainMenuData* MainMenuPage::get_main_menu_data() const
{
	return this->_data;
}
int MainMenuPage::get_player_count() const
{ 
	return this->get_main_menu_data()->get_level_settings()->
		get_player_count();
}

#pragma region MainMenuTitle

void MainMenuTitle::draw()
{
	this->draw_mobject_in_viewports(this->_texture_container.get(),
		this->get_point_clamp_sampler_state());

	// draw text separately to use blend state
	this->draw_mobject_in_viewports(this->_text_container.get());
}
void MainMenuTitle::update()
{
	const std::vector<ProcessedMenuInput> inputs = this->get_menu_inputs();
	for (int i = 0; i < inputs.size(); i++)
	{
		if (inputs[i].action == menu_input_action::PROCEED)
		{
			this->play_wave(CONFIRM_SOUND);
			//this->_music->Stop();
			this->get_context()->transition_to(std::make_unique<MainMenuHome>(
				this->get_main_menu_data()));
			return;
		}
	}
}
void MainMenuTitle::init()
{
	this->_background = std::make_unique<MTexture>(
		"background",
		"sprite_sheet_1",
		"square_white_4",
		RectangleF(Vector2F::ZERO, DEFAULT_RESOLUTION),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		TITLE_BACKGROUND_COLOUR);

	this->_title = std::make_unique<MTextDropShadow>(
		"title",
		"Colour Wars",
		TITLE_FONT,
		Vector2F(200.0f, 300.0f),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		TITLE_TEXT_COLOUR,
		SHADOW_COLOUR,
		TITLE_SHADOW_OFFSET);

	this->_start = std::make_unique<MTextDropShadow>(
		"start",
		"Start",
		ITEM_FONT,
		Vector2F(250.0f, 700.0f),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		TITLE_START_TEXT_COLOUR,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_texture_container = std::make_unique<MContainer>(
		"texture_container");
	this->_texture_container->add_child(this->_background.get());

	this->_text_container = std::make_unique<MContainer>(
		"text_container");
	this->_text_container->add_child(this->_title.get());
	this->_text_container->add_child(this->_start.get());

	Vector2F resolution = this->get_float_resolution();
	this->_texture_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->_text_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);

	this->play_effect(MUSIC, true, MUSIC_VOLUME);
}

#pragma endregion MainMenuTitle

#pragma region MainMenuHome

void MainMenuHome::draw()
{
	this->draw_mobject_in_viewports(this->_texture_container.get(),
		this->get_point_clamp_sampler_state());

	// draw text separately to use blend state
	this->draw_mobject_in_viewports(this->_text_container.get());
}
void MainMenuHome::update()
{
	std::vector<ProcessedMenuInput> inputs = this->get_menu_inputs();
	int num_inputs = static_cast<int>(inputs.size());
	std::string highlighted_element =
		this->get_highlighted_widget()->get_name();

	for (int i = 0; i < num_inputs; i++)
	{		
		if (inputs[i].action == menu_input_action::BACK)
		{
			this->play_wave(CANCEL_SOUND);
			this->get_context()->transition_to(
				std::make_unique<MainMenuTitle>(this->get_main_menu_data()));
			return;
		}
		if (inputs[i].action == menu_input_action::PROCEED)
		{
			if (highlighted_element == "play")
			{
				this->play_wave(CONFIRM_SOUND);
				this->get_context()->transition_to(
					std::make_unique<MainMenuModeSelect>(
						this->get_main_menu_data()));
				return;
			}
			if (highlighted_element == "options")
			{
				this->play_wave(CONFIRM_SOUND);
				this->get_context()->transition_to(
					std::make_unique<MainMenuOptions>(
						this->get_main_menu_data()));
				return;
			}
			if (highlighted_element == "exit")
			{
				DestroyWindow(this->get_main_menu_data()->get_window());
			}
		}
		else if (inputs[i].direction == menu_direction::UP)
		{
			this->play_wave(DIRECTION_SOUND);
			if (highlighted_element == "play")
			{
				this->change_highlight(this->_exit.get());
			}
			else if (highlighted_element == "options")
			{
				this->change_highlight(this->_play.get());
			}
			else if (highlighted_element == "exit")
			{
				this->change_highlight(this->_options.get());
			}
		}
		else if (inputs[i].direction == menu_direction::DOWN)
		{
			this->play_wave(DIRECTION_SOUND);
			if (highlighted_element == "play")
			{
				this->change_highlight(this->_options.get());
			}
			else if (highlighted_element == "options")
			{
				this->change_highlight(this->_exit.get());
			}
			else if (highlighted_element == "exit")
			{
				this->change_highlight(this->_play.get());
			}
		}
	}
}
void MainMenuHome::init()
{
	this->set_highlight_colour(STANDARD_HIGHLIGHT);
	this->set_unhighlight_colour(STANDARD_UNHIGHLIGHT);

	this->_background = std::make_unique<MTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DEFAULT_RESOLUTION),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		HOME_BACKGROUND_COLOUR);

	this->_heading = std::make_unique<MTextDropShadow>(
		"heading",
		"Main Menu",
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		HEADING_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->_play = std::make_unique<MTextDropShadow>(
		"play",
		"Play",
		ITEM_FONT,
		this->calculate_widget_position(0, 2),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_highlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_options = std::make_unique<MTextDropShadow>(
		"options",
		"Options",
		ITEM_FONT,
		this->calculate_widget_position(0, 3),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_exit = std::make_unique<MTextDropShadow>(
		"exit",
		"Exit",
		ITEM_FONT,
		this->calculate_widget_position(0, 4),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_texture_container = std::make_unique<MContainer>(
		"texture_container");
	this->_texture_container->add_child(this->_background.get());

	this->_text_container = std::make_unique<MContainer>(
		"text_container");
	this->_text_container->add_child(this->_heading.get());
	this->_text_container->add_child(this->_play.get());
	this->_text_container->add_child(this->_options.get());
	this->_text_container->add_child(this->_exit.get());

	Vector2F resolution = this->get_float_resolution();
	this->_texture_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->_text_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);

	this->set_highlighted_widget(this->_play.get());

	this->play_effect(MUSIC, true, MUSIC_VOLUME);
}

#pragma endregion MainMenuHome

#pragma region MainMenuOptions

void MainMenuOptions::draw()
{
	this->draw_mobject_in_viewports(this->_texture_container.get(),
		this->get_point_clamp_sampler_state());

	// draw text separately to use blend state
	this->draw_mobject_in_viewports(this->_text_container.get());
}
void MainMenuOptions::update()
{
	std::vector<ProcessedMenuInput> inputs = this->get_menu_inputs();
	int num_inputs = static_cast<int>(inputs.size());
	std::string highlighted_element =
		this->get_highlighted_widget()->get_name();

	for (int i = 0; i < num_inputs; i++)
	{
		if (inputs[i].action == menu_input_action::BACK)
		{
			this->play_wave(CANCEL_SOUND);
			this->get_context()->transition_to(std::make_unique<MainMenuHome>(
				this->get_main_menu_data()));
			return;
		}
		else if (inputs[i].action == menu_input_action::PROCEED)
		{
			if (highlighted_element == "apply")
			{
				this->play_wave(CONFIRM_SOUND);
				
				auto prev_resolution = this->get_resolution_manager()->get_resolution_vec();
				
				this->get_resolution_manager()->set_resolution(
					this->_resolution_selection);

				//std::string res_string = this->get_resolution_manager()->
				//	get_resolution_string();
				this->get_save()->set_resolution_and_save(this->_resolution_selection);

				auto res_f = this->get_float_resolution();
				auto res_i = this->get_resolution_manager()->get_resolution_ivec();

				SetWindowPos(this->get_data()->get_window(),
										HWND_TOP,
										0,
										0,
										res_i.x,
										res_i.y,
										SWP_NOMOVE | SWP_NOZORDER);

				bool fs = this->_full_screen_selection;
				this->get_save()->set_full_screen_and_save(fs);
				this->apply_fullscreen_setting(fs);

				//update window and its children's sizes and positions
				this->_texture_container->scale_objects_to_new_resolution(
					prev_resolution, res_f);
				this->_text_container->scale_objects_to_new_resolution(
					prev_resolution, res_f);
				
				//this->_background->update_resolution(prev_resolution, res_f);
			}
			else if (highlighted_element == "back")
			{
				this->play_wave(CANCEL_SOUND);
				this->get_context()->transition_to(
					std::make_unique<MainMenuHome>(
						this->get_main_menu_data()));
				return;
			}
		}
		else if (inputs[i].direction == menu_direction::DOWN)
		{
			this->play_wave(DIRECTION_SOUND);
			if (highlighted_element == "resolution_element")
			{
				this->change_highlight(this->_full_screen.get());
			}
			else if (highlighted_element == "full_screen")
			{
				this->change_highlight(this->_apply.get());
			}
			else if (highlighted_element == "apply")
			{
				this->change_highlight(this->_back.get());
			}
			else if (highlighted_element == "back")
			{
				this->change_highlight(this->_resolution_element.get());
			}
		}
		else if (inputs[i].direction == menu_direction::UP)
		{
			this->play_wave(DIRECTION_SOUND);
			if (highlighted_element == "resolution_element")
			{
				this->change_highlight(this->_back.get());
			}
			else if (highlighted_element == "full_screen")
			{
				this->change_highlight(this->_resolution_element.get());
			}
			else if (highlighted_element == "apply")
			{
				this->change_highlight(this->_full_screen.get());
			}
			else if (highlighted_element == "back")
			{
				this->change_highlight(this->_apply.get());
			}
		}
		else if (highlighted_element == "resolution_element")
		{
			if (inputs[i].direction == menu_direction::LEFT)
			{
				this->play_wave(DIRECTION_SOUND);
				this->cycle_resolution(menu_direction::LEFT);
			}
			else if (inputs[i].direction == menu_direction::RIGHT)
			{
				this->play_wave(DIRECTION_SOUND);
				this->cycle_resolution(menu_direction::RIGHT);
			}
		}
		else if (highlighted_element == "full_screen" &&
			(inputs[i].direction == menu_direction::LEFT ||
			inputs[i].direction == menu_direction::RIGHT))
		{
			this->play_wave(DIRECTION_SOUND);
			this->_full_screen_selection = !this->_full_screen_selection;
			this->update_full_screen_selection_text();
		}
	}
}

void MainMenuOptions::apply_fullscreen_setting(bool fullscreen)
{
	HWND hWnd = this->get_data()->get_window();
	if (fullscreen)
	{
		SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP);
		SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);

		SetWindowPos(hWnd,
			HWND_TOP, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
		ShowWindow(hWnd, SW_SHOWMAXIMIZED);
	}
	else // windowed
	{
		SetWindowLongPtr(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
		SetWindowLongPtr(hWnd, GWL_EXSTYLE, 0);

		Vector2I res_i = this->get_resolution_manager()->get_resolution_ivec();

		ShowWindow(hWnd, SW_SHOWNORMAL);
		SetWindowPos(hWnd, HWND_TOP, 0, 0, res_i.x, res_i.y,
			SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}
}

void MainMenuOptions::init()
{
	this->set_highlight_colour(STANDARD_HIGHLIGHT);
	this->set_unhighlight_colour(STANDARD_UNHIGHLIGHT);
	Vector2F widget_size = this->get_widget_size();

	this->_resolution_selection = this->get_resolution_manager()->get_resolution();
	this->_full_screen_selection = this->get_save()->get_fullscreen();

	this->_background = std::make_unique<MTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DEFAULT_RESOLUTION),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		OPTIONS_BACKGROUND_COLOUR);

	this->_heading = std::make_unique<MTextDropShadow>(
		"heading",
		"Options",
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		HEADING_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->_resolution_element = std::make_unique<MTextDropShadow>(
		"resolution_element",
		"Resolution",
		ITEM_FONT,
		this->calculate_widget_position(0, 2),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_highlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_resolution_value = std::make_unique<MTextDropShadow>(
		"resolution_value",
		"null",
		ITEM_FONT,
		this->calculate_widget_position(2, 2),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		OPTIONS_VALUE_COLOUR,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->update_resolution_selection_text();

	this->_full_screen = std::make_unique<MTextDropShadow>(
		"full_screen",
		"Fullscreen",
		ITEM_FONT,
		this->calculate_widget_position(0, 3),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_full_screen_value = std::make_unique<MTextDropShadow>(
		"full_screen_value",
		"null",
		ITEM_FONT,
		this->calculate_widget_position(2, 3),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		OPTIONS_VALUE_COLOUR,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->update_full_screen_selection_text();

	this->_apply = std::make_unique<MTextDropShadow>(
		"apply",
		"Apply",
		ITEM_FONT,
		this->calculate_widget_position(0, 4),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_back = std::make_unique<MTextDropShadow>(
		"back",
		"Back",
		ITEM_FONT,
		this->calculate_widget_position(0, 5),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_texture_container = std::make_unique<MContainer>(
		"texture_container");
	this->_texture_container->add_child(this->_background.get());

	this->_text_container = std::make_unique<MContainer>(
		"text_container");
	this->_text_container->add_child(this->_heading.get());
	this->_text_container->add_child(this->_resolution_element.get());
	this->_text_container->add_child(this->_resolution_value.get());
	this->_text_container->add_child(this->_full_screen.get());
	this->_text_container->add_child(this->_full_screen_value.get());
	this->_text_container->add_child(this->_apply.get());
	this->_text_container->add_child(this->_back.get());

	Vector2F resolution = this->get_float_resolution();
	this->_texture_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->_text_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);

	this->set_highlighted_widget(this->_resolution_element.get());

	this->play_effect(MUSIC, true, MUSIC_VOLUME);
}

void MainMenuOptions::cycle_resolution(
	menu_direction direction)
{
	int enum_max = static_cast<int>(screen_resolution::MAX);
	int enum_pos =
		static_cast<int>(this->_resolution_selection);
	if (direction == menu_direction::LEFT)
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
	else if (direction == menu_direction::RIGHT)
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
	this->_resolution_selection =
		static_cast<screen_resolution>(enum_pos);
	this->update_resolution_selection_text();
}

void MainMenuOptions::update_resolution_selection_text()
{
	std::string res_text = this->get_resolution_manager()->
		convert_resolution_to_string(this->_resolution_selection);
	this->_resolution_value->set_text(res_text);
}

void MainMenuOptions::update_full_screen_selection_text() const
{
	bool fs = this->_full_screen_selection; //this->get_context()->get_data()->get_save()->get_full_screen();
	std::string fs_text = "";
	if (fs)
	{
		fs_text = "On";
	}
	else
	{
		fs_text = "Off";
	}
	this->_full_screen_value->set_text(fs_text);
}

#pragma endregion MainMenuOptions

#pragma region MainMenuModeSelect

void MainMenuModeSelect::draw()
{
	this->draw_mobject_in_viewports(this->_texture_container.get(),
		this->get_point_clamp_sampler_state());

	// draw text separately to use blend state
	this->draw_mobject_in_viewports(this->_text_container.get());
}
void MainMenuModeSelect::update()
{
	std::vector<ProcessedMenuInput> inputs = this->get_menu_inputs();
	int num_inputs = static_cast<int>(inputs.size());
	std::string highlighted_element =
		this->get_highlighted_widget()->get_name();

	for (int i = 0; i < num_inputs; i++)
	{
		if (inputs[i].action == menu_input_action::BACK)
		{
			this->play_wave(CANCEL_SOUND);
			this->get_context()->transition_to(std::make_unique<MainMenuHome>(
				this->get_main_menu_data()));
			return;
		}
		if (inputs[i].action == menu_input_action::PROCEED)
		{
			if (highlighted_element == "standard")
			{
				this->play_wave(CONFIRM_SOUND);
				this->get_main_menu_data()->get_level_settings()->
					set_game_mode(level_mode::STANDARD_MODE);
				this->get_context()->transition_to(
					std::make_unique<MainMenuPlayerCount>(
						this->get_main_menu_data()));
				return;
			}
			if (highlighted_element == "tdm")
			{
				this->play_wave(ERROR_SOUND);
			}
			else if (highlighted_element == "dm")
			{
				this->play_wave(ERROR_SOUND);
			}
			else if (highlighted_element == "practice")
			{
				this->play_wave(ERROR_SOUND);
			}
			else if (highlighted_element == "back")
			{
				this->play_wave(CANCEL_SOUND);
				this->get_context()->transition_to(
					std::make_unique<MainMenuHome>(
						this->get_main_menu_data()));
				return;
			}
		}
		else if (inputs[i].direction == menu_direction::UP)
		{
			this->play_wave(DIRECTION_SOUND);
			if (highlighted_element == "standard")
			{
				this->change_highlight(this->_back.get());
			}
			else if (highlighted_element == "tdm")
			{
				this->change_highlight(this->_standard.get());
			}
			else if (highlighted_element == "dm")
			{
				this->change_highlight(this->_tdm.get());
			}
			else if (highlighted_element == "practice")
			{
				this->change_highlight(this->_dm.get());
			}
			else if (highlighted_element == "back")
			{
				this->change_highlight(this->_practice.get());
			}
		}
		else if (inputs[i].direction == menu_direction::DOWN)
		{
			this->play_wave(DIRECTION_SOUND);
			if (highlighted_element == "standard")
			{
				this->change_highlight(this->_tdm.get());
			}
			else if (highlighted_element == "tdm")
			{
				this->change_highlight(this->_dm.get());
			}
			else if (highlighted_element == "dm")
			{
				this->change_highlight(this->_practice.get());
			}
			else if (highlighted_element == "practice")
			{
				this->change_highlight(this->_back.get());
			}
			else if (highlighted_element == "back")
			{
				this->change_highlight(this->_standard.get());
			}
		}
	}
}
menu_element MainMenuModeSelect::convert_mode_to_element(level_mode mode)
{
	switch (mode)
	{
	case level_mode::STANDARD_MODE:
		return menu_element::MAIN_MENU_STANDARD_MODE;
	case level_mode::TEAM_DEATHMATCH:
		return menu_element::MAIN_MENU_TEAM_DEATHMATCH;
	case level_mode::DEATHMATCH:
		return menu_element::MAIN_MENU_DEATHMATCH;
	case level_mode::PRACTICE:
		return menu_element::MAIN_MENU_PRACTICE;
	default:
		return menu_element::MAIN_MENU_STANDARD_MODE;
	}
}
void MainMenuModeSelect::init()
{
	this->set_highlight_colour(STANDARD_HIGHLIGHT);
	this->set_unhighlight_colour(STANDARD_UNHIGHLIGHT);

	//this->set_widget_position(MODE_SELECT_WIDGET_POSITION);
	this->set_widget_spacing(MODE_SELECT_WIDGET_SPACING);

	this->_background = std::make_unique<MTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DEFAULT_RESOLUTION),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		PLAY_BACKGROUND_COLOUR);

	this->_heading = std::make_unique<MTextDropShadow>(
		"heading",
		"Mode Select",
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		HEADING_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->_standard = std::make_unique<MTextDropShadow>(
		"standard",
		"Standard",
		ITEM_FONT,
		this->calculate_widget_position(0, 2),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_highlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_tdm = std::make_unique<MTextDropShadow>(
		"tdm",
		"Team Deathmatch",
		ITEM_FONT,
		this->calculate_widget_position(0, 3),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_dm = std::make_unique<MTextDropShadow>(
		"dm",
		"Deathmatch",
		ITEM_FONT,
		this->calculate_widget_position(0, 4),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_practice = std::make_unique<MTextDropShadow>(
		"practice",
		"Practice",
		ITEM_FONT,
		this->calculate_widget_position(0, 5),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_back = std::make_unique<MTextDropShadow>(
		"back",
		"Back",
		ITEM_FONT,
		this->calculate_widget_position(0, 6),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_texture_container = std::make_unique<MContainer>(
		"texture_container");
	this->_texture_container->add_child(this->_background.get());

	this->_text_container = std::make_unique<MContainer>(
		"text_container");
	this->_text_container->add_child(this->_heading.get());
	this->_text_container->add_child(this->_standard.get());
	this->_text_container->add_child(this->_tdm.get());
	this->_text_container->add_child(this->_dm.get());
	this->_text_container->add_child(this->_practice.get());
	this->_text_container->add_child(this->_back.get());

	Vector2F resolution = this->get_float_resolution();
	this->_texture_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->_text_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);

	this->set_highlighted_widget(this->_standard.get());

	this->play_effect(MUSIC, true, MUSIC_VOLUME);
}

#pragma endregion MainMenuModeSelect

#pragma region MainMenuPlayerCount

void MainMenuPlayerCount::draw()
{
	this->draw_mobject_in_viewports(this->_texture_container.get(),
		this->get_point_clamp_sampler_state());

	// draw text separately to use blend state
	this->draw_mobject_in_viewports(this->_text_container.get());
}
void MainMenuPlayerCount::update()
{
	std::vector<ProcessedMenuInput> inputs = this->get_menu_inputs();
	int num_inputs = static_cast<int>(inputs.size());
	std::string highlighted_element =
		this->get_highlighted_widget()->get_name();

	for (int i = 0; i < num_inputs; i++)
	{
		if (inputs[i].action == menu_input_action::BACK)
		{
			this->play_wave(CANCEL_SOUND);
			this->get_context()->transition_to(
				std::make_unique<MainMenuModeSelect>(
					this->get_main_menu_data()));
			return;
		}
		if (inputs[i].action == menu_input_action::PROCEED)
		{
			if (highlighted_element == "1_player")
			{
				this->play_wave(CONFIRM_SOUND);
				this->get_main_menu_data()->get_level_settings()->
					set_player_count(1);
				this->get_main_menu_data()->get_level_settings()->
					set_player_team(0, player_team::A);
				this->get_main_menu_data()->get_level_settings()->
					set_screen_layout(screen_layout::ONE_PLAYER);
				this->get_main_menu_data()->get_level_settings()->
					set_player_num(0, 0);
				this->get_context()->transition_to(
					std::make_unique<MainMenuWeaponSelect>(
						get_main_menu_data()));
				return;
			}
			if (highlighted_element == "2_players")
			{
				this->play_wave(CONFIRM_SOUND);
				this->get_main_menu_data()->get_level_settings()->
					set_player_count(2);
				this->get_main_menu_data()->get_level_settings()->
					set_screen_layout(screen_layout::TWO_PLAYER);
				this->get_context()->transition_to(
					std::make_unique<MainMenuTeamSelect>(
						get_main_menu_data()));
				return;
			}
			if (highlighted_element == "3_players")
			{
				this->play_wave(CONFIRM_SOUND);
				this->get_main_menu_data()->get_level_settings()->
					set_player_count(3);
				this->get_main_menu_data()->get_level_settings()->
					set_screen_layout(screen_layout::THREE_PLAYER);
				this->get_context()->transition_to(
					std::make_unique<MainMenuTeamSelect>(
						get_main_menu_data()));
				return;
			}
			if (highlighted_element == "4_players")
			{
				this->play_wave(CONFIRM_SOUND);
				this->get_main_menu_data()->get_level_settings()->
					set_player_count(4);
				this->get_main_menu_data()->get_level_settings()->
					set_screen_layout(screen_layout::FOUR_PLAYER);
				this->get_context()->transition_to(
					std::make_unique<MainMenuTeamSelect>(
					get_main_menu_data()));
				return;
			}
			if (highlighted_element == "back")
			{
				this->play_wave(CANCEL_SOUND);
				this->get_context()->transition_to(
					std::make_unique<MainMenuModeSelect>(
						this->get_main_menu_data()));
				return;
			}
		}
		else if (inputs[i].direction == menu_direction::UP)
		{
			this->play_wave(DIRECTION_SOUND);
			if (highlighted_element == "1_player")
			{
				this->change_highlight(this->_back.get());
			}
			else if (highlighted_element == "2_players")
			{
				this->change_highlight(this->_1_player.get());
			}
			else if (highlighted_element == "3_players")
			{
				this->change_highlight(this->_2_players.get());
			}
			else if (highlighted_element == "4_players")
			{
				this->change_highlight(this->_3_players.get());
			}
			else if (highlighted_element == "back")
			{
				this->change_highlight(this->_4_players.get());
			}
		}
		else if (inputs[i].direction == menu_direction::DOWN)
		{
			this->play_wave(DIRECTION_SOUND);
			if (highlighted_element == "1_player")
			{
				this->change_highlight(this->_2_players.get());
			}
			else if (highlighted_element == "2_players")
			{
				this->change_highlight(this->_3_players.get());
			}
			else if (highlighted_element == "3_players")
			{
				this->change_highlight(this->_4_players.get());
			}
			else if (highlighted_element == "4_players")
			{
				this->change_highlight(this->_back.get());
			}
			else if (highlighted_element == "back")
			{
				this->change_highlight(this->_1_player.get());
			}
		}
	}
}
menu_element MainMenuPlayerCount::convert_player_count_to_element(
	int player_count)
{
	switch (player_count)
	{
	case 1:
		return menu_element::MAIN_MENU_ONE_PLAYER;
	case 2:
		return menu_element::MAIN_MENU_TWO_PLAYERS;
	case 3:
		return menu_element::MAIN_MENU_THREE_PLAYERS;
	case 4:
		return menu_element::MAIN_MENU_FOUR_PLAYERS;
	default:
		return menu_element::MAIN_MENU_ONE_PLAYER;
	}
}
void MainMenuPlayerCount::init()
{
	this->set_highlight_colour(STANDARD_HIGHLIGHT);
	this->set_unhighlight_colour(STANDARD_UNHIGHLIGHT);

	//this->set_widget_position(PLAYER_COUNT_WIDGET_POSITION);
	this->set_widget_spacing(PLAYER_COUNT_WIDGET_SPACING);

	this->_background = std::make_unique<MTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DEFAULT_RESOLUTION),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		PLAY_BACKGROUND_COLOUR);

	this->_heading = std::make_unique<MTextDropShadow>(
		"heading",
		"Number of Players",
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		HEADING_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->_1_player = std::make_unique<MTextDropShadow>(
		"1_player",
		"1 Player",
		ITEM_FONT,
		this->calculate_widget_position(0, 2),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_highlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_2_players = std::make_unique<MTextDropShadow>(
		"2_players",
		"2 Players",
		ITEM_FONT,
		this->calculate_widget_position(0, 3),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_3_players = std::make_unique<MTextDropShadow>(
		"3_players",
		"3 Players",
		ITEM_FONT,
		this->calculate_widget_position(0, 4),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_4_players = std::make_unique<MTextDropShadow>(
		"4_players",
		"4 Players",
		ITEM_FONT,
		this->calculate_widget_position(0, 5),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_back = std::make_unique<MTextDropShadow>(
		"back",
		"Back",
		ITEM_FONT,
		this->calculate_widget_position(0, 6),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_texture_container = std::make_unique<MContainer>(
		"texture_container");
	this->_texture_container->add_child(this->_background.get());

	this->_text_container = std::make_unique<MContainer>(
		"text_container");
	this->_text_container->add_child(this->_heading.get());
	this->_text_container->add_child(this->_1_player.get());
	this->_text_container->add_child(this->_2_players.get());
	this->_text_container->add_child(this->_3_players.get());
	this->_text_container->add_child(this->_4_players.get());
	this->_text_container->add_child(this->_back.get());

	Vector2F resolution = this->get_float_resolution();
	this->_texture_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->_text_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);

	this->set_highlighted_widget(this->_1_player.get());

	this->play_effect(MUSIC, true, MUSIC_VOLUME);
}

#pragma endregion MainMenuPlayerCount

#pragma region MainMenuTeamSelect

MainMenuTeamSelect::MainMenuTeamSelect(MainMenuData* data,
	MenuLevelSettings* settings) :
	MainMenuPage(data)
{
	this->get_main_menu_data()->set_level_settings(settings);
}
void MainMenuTeamSelect::draw()
{
	this->draw_mobject_in_viewports(this->_texture_container.get(),
		this->get_point_clamp_sampler_state());

	// draw text separately to use blend state
	this->draw_mobject_in_viewports(this->_text_container.get());
}
void MainMenuTeamSelect::update()
{
	std::vector<ProcessedMenuInput> inputs = this->get_menu_inputs();
	int num_inputs = static_cast<int>(inputs.size());
	int player_count = this->get_player_count();

	for (int i = 0; i < num_inputs && i < player_count; i++)
	{
		if (inputs[i].action == menu_input_action::BACK)
		{
			if (this->all_players_unconfirmed())
			{
				this->play_wave(CANCEL_SOUND);
				this->get_context()->transition_to(
					std::make_unique<MainMenuPlayerCount>(
						this->get_main_menu_data()));
				return;
			}
			if (this->_select_states[i].state == confirmation_state::CONFIRMED)
			{
				this->play_wave(CANCEL_SOUND);
				this->_select_states[i].state =
					confirmation_state::UNCONFIRMED;
			}
			//no break here as I want simultaneous input from players to be possible
		}
		else if (inputs[i].action == menu_input_action::PROCEED &&
			this->_select_states[i].team != player_team::NONE)
		{
			this->play_wave(CONFIRM_SOUND);
			this->_select_states[i].state = confirmation_state::CONFIRMED;
		}
		else if (this->_select_states[i].state ==
			confirmation_state::UNCONFIRMED)
		{
			if (inputs[i].direction == menu_direction::LEFT)
			{
				this->play_wave(DIRECTION_SOUND);
				if (this->_select_states[i].team == player_team::NONE)
				{
					this->_select_states[i].team = player_team::A;
				}
				else if (this->_select_states[i].team == player_team::B)
				{
					this->_select_states[i].team = player_team::NONE;
				}
			}
			else if (inputs[i].direction == menu_direction::RIGHT)
			{
				this->play_wave(DIRECTION_SOUND);
				if (this->_select_states[i].team == player_team::NONE)
				{
					this->_select_states[i].team = player_team::B;
				}
				else if (this->_select_states[i].team == player_team::A)
				{
					this->_select_states[i].team = player_team::NONE;
				}
			}
		}
	}
	if (this->all_players_confirmed())
	{
		this->set_level_settings();
		this->get_context()->transition_to(
			std::make_unique<MainMenuWeaponSelect>(
				this->get_main_menu_data()));
		return;
	}
	this->update_team_select_visuals();
}
void MainMenuTeamSelect::set_level_settings() const
{
	for (int i = 0; i < this->get_player_count(); i++)
	{
		this->get_main_menu_data()->get_level_settings()->
			set_player_team(i, this->_select_states[i].team);

		this->get_main_menu_data()->get_level_settings()->
			set_player_num(i, i);
	}
}
bool MainMenuTeamSelect::all_players_confirmed() const
{
	for (int i = 0; i < this->get_player_count(); i++)
	{
		if (this->_select_states[i].state != confirmation_state::CONFIRMED)
		{
			return false;
		}
	}
	return true;
}
bool MainMenuTeamSelect::all_players_unconfirmed() const
{
	for (int i = 0; i < this->get_player_count(); i++)
	{
		if (this->_select_states[i].state == confirmation_state::CONFIRMED)
		{
			return false;
		}
	}
	return true;
}
void MainMenuTeamSelect::update_team_select_visuals()
{
	this->deselect_and_unconfirm_all_widgets();

	for (int i = 0; i < this->get_player_count(); i++)
	{
		MWidget* selected_widget = nullptr;
		switch (this->_select_states[i].team)
		{
		case player_team::A:
			this->_player_widgets[i]->player_a->set_sprite_frame(
				"team_select_a_selected");
			selected_widget = this->_player_widgets[i]->player_a.get();
			break;
		case player_team::NONE:
			this->_player_widgets[i]->player_center->set_sprite_frame(
				"team_select_center_selected");
			selected_widget = this->_player_widgets[i]->player_center.get();
			break;
		case player_team::B:
			this->_player_widgets[i]->player_b->set_sprite_frame(
				"team_select_b_selected");
			selected_widget = this->_player_widgets[i]->player_b.get();
			break;
		}
		if (this->_select_states[i].state ==
			confirmation_state::CONFIRMED)
		{
			selected_widget->set_colour(
				main_menu_consts::TEAM_SELECT_SELECTED_COLOUR);
		}
	}
}
void MainMenuTeamSelect::deselect_and_unconfirm_all_widgets()
{
	for (int i = 0; i < this->get_player_count(); i++)
	{
		this->_player_widgets[i]->player_a->set_sprite_frame(
			"team_select_a");
		this->_player_widgets[i]->player_center->set_sprite_frame(
			"team_select_center");
		this->_player_widgets[i]->player_b->set_sprite_frame(
			"team_select_b");

		this->_player_widgets[i]->player_a->set_colour(
			main_menu_consts::TEAM_SELECT_UNSELECTED_COLOUR);
		this->_player_widgets[i]->player_center->set_colour(
			main_menu_consts::TEAM_SELECT_UNSELECTED_COLOUR);
		this->_player_widgets[i]->player_b->set_colour(
			main_menu_consts::TEAM_SELECT_UNSELECTED_COLOUR);
	}
}
void MainMenuTeamSelect::init()
{
	this->set_widget_spacing(TEAM_SELECT_WIDGET_SPACING);

	const Vector2F resolution = this->get_float_resolution();

	this->_texture_container = std::make_unique<MContainer>(
		"texture_container");
	
	this->_text_container = std::make_unique<MContainer>(
		"text_container");

	this->_background = std::make_unique<MTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DEFAULT_RESOLUTION),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		PLAY_BACKGROUND_COLOUR);

	this->_texture_container->add_child(this->_background.get());

	this->_heading = std::make_unique<MTextDropShadow>(
		"heading",
		"Team Select",
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		HEADING_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->_text_container->add_child(this->_heading.get());

	
	for (int i = 0; i < this->get_player_count(); i++)
	{
		auto state = TeamSelectState();
		state.state = confirmation_state::UNCONFIRMED;
		state.team = this->get_main_menu_data()->get_level_settings()->
			get_player_team(i);
		this->_select_states.push_back(state);

		auto widgets = std::make_unique<PlayerWidgets>();

		std::string name = "player_" + std::to_string(i + 1);
		std::string label_text = "Player " + std::to_string(i + 1);

		widgets->player = std::make_unique<MTextDropShadow>(
			name,
			label_text,
			ITEM_FONT,
			this->calculate_widget_position(0, i + 2),
			this->get_sprite_batch(),
			this->get_resource_manager(),
			TEAM_SELECT_UNSELECTED_COLOUR,
			SHADOW_COLOUR,
			ITEM_SHADOW_OFFSET);

		widgets->player_a = std::make_unique<MTexture>(
			name + "_a",
			"sprite_sheet_1",
			"team_select_a",
			RectangleF(this->calculate_widget_position(2, i + 2), TEAM_SELECT_TEAM_WIDGET_SIZE),
			this->get_sprite_batch(),
			this->get_resource_manager());

		auto move = Vector2F(TEAM_SELECT_TEAM_WIDGET_SIZE.x, 0.0f);

		widgets->player_center = std::make_unique<MTexture>(
			name + "_center",
			"sprite_sheet_1",
			"team_select_center",
			RectangleF(widgets->player_a->get_rectangle().get_position() + move,
				TEAM_SELECT_TEAM_WIDGET_SIZE),
			this->get_sprite_batch(),
			this->get_resource_manager());

		widgets->player_b = std::make_unique<MTexture>(
			name + "_b",
			"sprite_sheet_1",
			"team_select_b",
			RectangleF(widgets->player_center->get_rectangle().get_position() + move,
				TEAM_SELECT_TEAM_WIDGET_SIZE),
			this->get_sprite_batch(),
			this->get_resource_manager());

		this->_texture_container->add_child(widgets->player_a.get());
		this->_texture_container->add_child(widgets->player_center.get());
		this->_texture_container->add_child(widgets->player_b.get());

		this->_text_container->add_child(widgets->player.get());

		this->_player_widgets.push_back(std::move(widgets));
	}

	this->_texture_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->_text_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);

	this->play_effect(MUSIC, true, MUSIC_VOLUME);
}

#pragma endregion MainMenuTeamSelect

#pragma region MainMenuWeaponSelect

MainMenuWeaponSelect::MainMenuWeaponSelect(MainMenuData* data,
	MenuLevelSettings* settings) :
	MainMenuPage(data)
{
	this->get_main_menu_data()->set_level_settings(settings);
}
void MainMenuWeaponSelect::draw()
{
	this->draw_mobject_in_viewports(this->_texture_container.get(),
		this->get_point_clamp_sampler_state());

	// draw text separately to use blend state
	this->draw_mobject_in_viewports(this->_text_container.get());
}
void MainMenuWeaponSelect::update()
{
	std::vector<ProcessedMenuInput> inputs = this->get_menu_inputs();
	int player_count = this->get_player_count();
	int num_inputs = static_cast<int>(inputs.size());

	for (int i = 0; i < num_inputs && i < player_count; i++)
	{
		if (inputs[i].action == menu_input_action::BACK)
		{
			if (this->all_players_unconfirmed())
			{
				this->play_wave(CANCEL_SOUND);
				if (this->get_player_count() == 1)
				{
					this->get_context()->transition_to(
						std::make_unique<MainMenuModeSelect>(
							this->get_main_menu_data()));
				}
				else
				{
					this->get_context()->transition_to(
						std::make_unique<MainMenuTeamSelect>(
							this->get_main_menu_data()));
				}
				return;
			}
			if (this->_select_states[i].state ==
				confirmation_state::CONFIRMED)
			{
				this->play_wave(CANCEL_SOUND);
				this->_select_states[i].state =
					confirmation_state::UNCONFIRMED;
			}
			//no break here as I want simultaneous input
			// from players to be possible
		}
		else if (inputs[i].action == menu_input_action::PROCEED)
		{
			this->play_wave(CONFIRM_SOUND);
			this->_select_states[i].state = confirmation_state::CONFIRMED;
		}
		else if (this->_select_states[i].state ==
			confirmation_state::UNCONFIRMED)
		{
			if (inputs[i].direction == menu_direction::LEFT)
			{
				this->play_wave(DIRECTION_SOUND);
				this->cycle_weapons(menu_direction::LEFT, i);
			}
			else if (inputs[i].direction == menu_direction::RIGHT)
			{
				this->play_wave(DIRECTION_SOUND);
				this->cycle_weapons(menu_direction::RIGHT, i);
			}
		}
	}
	if (this->all_players_confirmed())
	{
		//check if any players have selected random then pick a random weapon.
		for (int i = 0; i < this->get_player_count(); i++)
		{
			if (this->_select_states[i].type == wep_type::RANDOM_PRIMARY)
			{
				this->_select_states[i].type = get_random_weapon();
			}
		}
		this->set_level_settings();
		this->get_context()->transition_to(
			std::make_unique<MainMenuStageSelect>(this->get_main_menu_data()));
		return;
	}
	this->update_weapon_select_visuals();
}
void MainMenuWeaponSelect::set_level_settings() const
{
	for (int i = 0; i < this->get_player_count(); i++)
	{
		this->get_main_menu_data()->get_level_settings()->
			set_player_weapon(i, this->_select_states[i].type);
	}
}
wep_type MainMenuWeaponSelect::get_random_weapon()
{
	int random = rand() % static_cast<int>(wep_type::MAX_PRIM_WEP);
	return static_cast<wep_type>(random);
}
void MainMenuWeaponSelect::cycle_weapons(
	menu_direction direction, int player_index)
{
	int enum_max = static_cast<int>(wep_type::MAX_PRIM_WEP);
	int enum_pos =
		static_cast<int>(this->_select_states[player_index].type);
	if (direction == menu_direction::LEFT)
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
	else if (direction == menu_direction::RIGHT)
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
	this->_select_states[player_index].type =
		static_cast<wep_type>(enum_pos);
}
bool MainMenuWeaponSelect::all_players_confirmed() const
{
	for (int i = 0; i < this->get_player_count(); i++)
	{
		if (this->_select_states[i].state != confirmation_state::CONFIRMED)
		{
			return false;
		}
	}
	return true;
}
bool MainMenuWeaponSelect::all_players_unconfirmed() const
{
	for (int i = 0; i < this->get_player_count(); i++)
	{
		if (this->_select_states[i].state == confirmation_state::CONFIRMED)
		{
			return false;
		}
	}
	return true;
}
void MainMenuWeaponSelect::update_weapon_select_visuals()
{
	this->unconfirm_all_widgets();

	for (int i = 0; i < this->get_player_count(); i++)
	{
		switch (this->_select_states[i].type)
		{
		case wep_type::SPRAYER:
			this->_player_widgets[i]->weapon_icon->set_sprite_frame("sprayer");
			this->_player_widgets[i]->weapon_name->set_text("Sprayer");
			break;
		case wep_type::SNIPER:
			this->_player_widgets[i]->weapon_icon->set_sprite_frame("sniper");
			this->_player_widgets[i]->weapon_name->set_text("Sniper");
			break;
		case wep_type::ROLLER:
			this->_player_widgets[i]->weapon_icon->set_sprite_frame("roller");
			this->_player_widgets[i]->weapon_name->set_text("Roller");
			break;
		case wep_type::MISTER:
			this->_player_widgets[i]->weapon_icon->set_sprite_frame("mister");
			this->_player_widgets[i]->weapon_name->set_text("Mister");
			break;
		case wep_type::BUCKET:
			this->_player_widgets[i]->weapon_icon->set_sprite_frame("bucket");
			this->_player_widgets[i]->weapon_name->set_text("Bucket");
			break;
		case wep_type::RANDOM_PRIMARY:
			this->_player_widgets[i]->weapon_icon->set_sprite_frame("random");
			this->_player_widgets[i]->weapon_name->set_text("Random");
			break;
		default:
			this->_player_widgets[i]->weapon_icon->set_sprite_frame("error");
			this->_player_widgets[i]->weapon_name->set_text("ERROR");
			break;
		}

		if (this->_select_states[i].state == confirmation_state::CONFIRMED)
		{
			this->_player_widgets[i]->weapon_icon->
				set_colour(WEAPON_SELECT_SELECTED_COLOUR);
			this->_player_widgets[i]->weapon_name->
				set_colour(WEAPON_SELECT_SELECTED_COLOUR);
		}

		this->_player_widgets[i]->weapon_description->set_text(
			this->weapon_description(this->_select_states[i].type));
	}
}
void MainMenuWeaponSelect::unconfirm_all_widgets()
{
	for (int i = 0; i < this->get_player_count(); i++)
	{
		this->_player_widgets[i]->weapon_icon->set_colour(
			WEAPON_SELECT_UNSELECTED_COLOUR);
		this->_player_widgets[i]->weapon_name->set_colour(
			WEAPON_SELECT_UNSELECTED_COLOUR);
	}
}
void MainMenuWeaponSelect::init()
{
	this->set_widget_spacing(WEAPON_SELECT_WIDGET_SPACING);

	const Vector2F resolution = this->get_float_resolution();

	this->_texture_container = std::make_unique<MContainer>(
		"texture_container");
	
	this->_text_container = std::make_unique<MContainer>(
		"text_container");

	this->_background = std::make_unique<MTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DEFAULT_RESOLUTION),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		PLAY_BACKGROUND_COLOUR);

	this->_texture_container->add_child(this->_background.get());

	this->_heading = std::make_unique<MTextDropShadow>(
		"heading",
		"Weapon Select",
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		HEADING_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->_text_container->add_child(this->_heading.get());

	for (int i = 0; i < this->get_player_count(); i++)
	{
		SelectState state;
		state.state = confirmation_state::UNCONFIRMED;
		state.type = this->get_main_menu_data()->get_level_settings()
			->get_player_weapon(i);
		this->_select_states.push_back(state);

		auto widgets = std::make_unique<Widgets>();

		std::string name = "player_" + std::to_string(i + 1);
		std::string label_text = "Player " + std::to_string(i + 1);

		widgets->player = std::make_unique<MTextDropShadow>(
			name,
			label_text,
			ITEM_FONT,
			this->calculate_widget_position(0, i + 2),
			this->get_sprite_batch(),
			this->get_resource_manager(),
			WEAPON_SELECT_UNSELECTED_COLOUR);

		widgets->weapon_icon = std::make_unique<MTexture>(
			name + "_wep_icon",
			"sprite_sheet_1",
			"sprayer",
			RectangleF(this->calculate_widget_position(2, i + 2), WEAPON_SELECT_WEAPON_WIDGET_SIZE),
			this->get_sprite_batch(),
			this->get_resource_manager(),
			PLAY_BACKGROUND_COLOUR);

		widgets->weapon_name = std::make_unique<MTextDropShadow>(
			name + "_wep_name",
			"Sprayer",
			DETAIL_FONT,
			widgets->weapon_icon->get_rectangle().get_position() +
				Vector2F(0.0f, WEAPON_SELECT_WEAPON_WIDGET_SIZE.y),
			this->get_sprite_batch(),
			this->get_resource_manager(),
			WEAPON_SELECT_UNSELECTED_COLOUR,
			SHADOW_COLOUR,
			DETAIL_SHADOW_OFFSET);

		widgets->weapon_description = std::make_unique<MTextDropShadow>(
			name + "_wep_desc",
			this->weapon_description(wep_type::SPRAYER),
			WEAPON_DESCRIPTION_FONT,
			widgets->weapon_icon->get_rectangle().get_position() +
				Vector2F(WEAPON_DESC_X_OFFSET, 0.0f),
			this->get_sprite_batch(),
			this->get_resource_manager(),
			WEAPON_DESCRIPTION_FONT_COLOUR,
			SHADOW_COLOUR,
			WEAPON_DESCRIPTION_SHADOW_OFFSET);

		this->_texture_container->add_child(widgets->weapon_icon.get());
		this->_text_container->add_child(widgets->weapon_name.get());
		this->_text_container->add_child(widgets->player.get());
		this->_text_container->add_child(widgets->weapon_description.get());

		this->_player_widgets.push_back(std::move(widgets));
	}
	this->_texture_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->_text_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);

	this->play_effect(MUSIC, true, MUSIC_VOLUME);
}
std::string MainMenuWeaponSelect::weapon_description(wep_type type)
{
	switch (type)
	{
	case wep_type::SPRAYER:
		return SPRAYER_DESC;
	case wep_type::SNIPER:
		return SNIPER_DESC;
	case wep_type::ROLLER:
		return ROLLER_DESC;
	case wep_type::MISTER:
		return MISTER_DESC;
	case wep_type::BUCKET:
		return BUCKET_DESC;
	case wep_type::RANDOM_PRIMARY:
		return RANDOM_DESC;
	default:
		return "ERROR";
	}
}

#pragma endregion MainMenuWeaponSelect

#pragma region MainMenuStageSelect

MainMenuStageSelect::MainMenuStageSelect(MainMenuData* data,
	MenuLevelSettings* settings) :
	MainMenuPage(data)
{
	this->get_main_menu_data()->set_level_settings(settings);
}
void MainMenuStageSelect::draw()
{
	this->draw_mobject_in_viewports(this->_texture_container.get(),
		this->get_point_clamp_sampler_state());

	// draw text separately to use blend state
	this->draw_mobject_in_viewports(this->_text_container.get());
}
void MainMenuStageSelect::update()
{
	std::vector<ProcessedMenuInput> inputs = this->get_menu_inputs();
	int num_inputs = static_cast<int>(inputs.size());
	int player_count = this->get_player_count();

	for (int i = 0; i < num_inputs && i < player_count; i++)
	{
		if (inputs[i].action == menu_input_action::BACK)
		{
			if (this->_select_state.state == confirmation_state::UNCONFIRMED)
			{
				this->play_wave(CANCEL_SOUND);
				this->get_context()->transition_to(
					std::make_unique<MainMenuWeaponSelect>(
						this->get_main_menu_data()));
				return;
			}
			if (this->_select_state.state == confirmation_state::CONFIRMED)
			{
				this->play_wave(CANCEL_SOUND);
				this->_select_state.state =
					confirmation_state::UNCONFIRMED;
			}
			//no break here as I want simultaneous input from players to be possible
		}
		else if (inputs[i].action == menu_input_action::PROCEED)
		{
			if (this->_select_state.state == confirmation_state::CONFIRMED)
			{
				this->play_wave(CONFIRM_SOUND);
				this->stop_effect(MUSIC, true);
				if (this->_select_state.stage == level_stage::RANDOM)
				{
					this->_select_state.stage = this->get_random_stage();
				}
				this->set_level_settings();
				*this->get_main_menu_data()->
					get_is_ready_to_load_level() = true;
				//no need to transition here as we should be going to load level
			}
			else
			{
				this->play_wave(READY_SOUND);
				this->_select_state.state = confirmation_state::CONFIRMED;
			}
		}
		else if (this->_select_state.state == confirmation_state::UNCONFIRMED)
		{
			if (inputs[i].direction == menu_direction::LEFT)
			{
				this->play_wave(DIRECTION_SOUND);
				this->cycle_stages(menu_direction::LEFT);
			}
			else if (inputs[i].direction == menu_direction::RIGHT)
			{
				this->play_wave(DIRECTION_SOUND);
				this->cycle_stages(menu_direction::RIGHT);
			}
		}
	}
	this->update_stage_select_visuals();
}
void MainMenuStageSelect::set_level_settings() const
{
	this->get_main_menu_data()->get_level_settings()->set_stage(
		this->_select_state.stage);
}
void MainMenuStageSelect::update_stage_select_visuals()
{
	this->unconfirm_all_widgets();

	switch (this->_select_state.stage)
	{
	case level_stage::KING_OF_THE_HILL:
		this->_stage_icon->set_sprite_frame("stage_king_of_the_hill");
		this->_stage_name->set_text("King of the Hill");
		break;
	case level_stage::TURBULENCE:
		this->_stage_icon->set_sprite_frame("stage_turbulence");
		this->_stage_name->set_text("Turbulence");
		break;
	case level_stage::CLOSE_QUARTERS:
		this->_stage_icon->set_sprite_frame("stage_close_quarters");
		this->_stage_name->set_text("Close Quarters");
		break;
	case level_stage::RANDOM:
		this->_stage_icon->set_sprite_frame("random");
		this->_stage_name->set_text("Random");
		break;
	default:
		this->_stage_icon->set_sprite_frame("error");
		this->_stage_name->set_text("ERROR");
		break;
	}
	if (this->_select_state.state == confirmation_state::CONFIRMED)
	{
		this->_ready->set_hidden(false);
	}
}
void MainMenuStageSelect::unconfirm_all_widgets()
{
	this->_stage_icon->set_colour(
		main_menu_consts::STAGE_SELECT_UNSELECTED_COLOUR);
	this->_stage_name->set_colour(
		main_menu_consts::STAGE_SELECT_UNSELECTED_COLOUR);
	this->_ready->set_hidden(true);
}
level_stage MainMenuStageSelect::get_random_stage()
{
	int random = static_cast<int>(rand() %
		static_cast<int>(level_stage::MAX_STAGE));
	return static_cast<level_stage>(random);
}
void MainMenuStageSelect::init()
{
	this->_select_state.stage = this->get_main_menu_data()->
		get_level_settings()->get_stage();

	this->set_widget_spacing(STAGE_SELECT_WIDGET_SPACING);

	this->_background = std::make_unique<MTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DEFAULT_RESOLUTION),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		PLAY_BACKGROUND_COLOUR);

	this->_heading = std::make_unique<MTextDropShadow>(
		"heading",
		"Stage Select",
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		HEADING_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);


	this->_stage_icon = std::make_unique<MTexture>(
		"stage_icon",
		"sprite_sheet_1",
		"stage_test_1",
		RectangleF(this->calculate_widget_position(0, 1), STAGE_SELECT_ICON_SIZE),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		PLAY_BACKGROUND_COLOUR);

	this->_stage_name = std::make_unique<MTextDropShadow>(
		"stage_name",
		"Test 1",
		ITEM_FONT,
		this->calculate_widget_position(0, 5),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		STAGE_SELECT_UNSELECTED_COLOUR,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_ready = std::make_unique<MTextDropShadow>(
		"ready",
		"READY?",
		ANNOUNCEMENT_FONT,
		this->_background->get_rectangle().get_center() - Vector2F(400.0f, 100.0f),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		STAGE_SELECT_SELECTED_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET,
		true);

	const Vector2F resolution = this->get_float_resolution();

	this->_texture_container = std::make_unique<MContainer>("texture_container");
	this->_texture_container->add_child(this->_background.get());
	this->_texture_container->add_child(this->_stage_icon.get());

	this->_text_container = std::make_unique<MContainer>("text_container");
	this->_text_container->add_child(this->_heading.get());
	this->_text_container->add_child(this->_stage_name.get());
	this->_text_container->add_child(this->_ready.get());

	this->_texture_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->_text_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);

	this->play_effect(MUSIC, true, MUSIC_VOLUME);
}
void MainMenuStageSelect::cycle_stages(menu_direction direction)
{
	int enum_max = static_cast<int>(level_stage::MAX_STAGE);
	int enum_pos = static_cast<int>(this->_select_state.stage);
	if (direction == menu_direction::LEFT)
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
	else if (direction == menu_direction::RIGHT)
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
	this->_select_state.stage = static_cast<level_stage>(enum_pos);
}

#pragma endregion MainMenuStageSelect