#include "game/pch.h"
#include "game/states/main_menu.h"

using namespace DirectX;
using namespace mattmath;
using namespace main_menu_consts;
using namespace colour_consts;

MainMenuPage::MainMenuPage(MainMenuData* data) :
	MenuPage(data),
	SoundBankObject(main_menu_consts::SOUND_BANK,
		this->get_audio_resources()),
	data_(data)
{
	this->direction_sound_ = this->resolve_wave(DIRECTION_SOUND);
	this->confirm_sound_ = this->resolve_wave(CONFIRM_SOUND);
	this->cancel_sound_ = this->resolve_wave(CANCEL_SOUND);
	this->error_sound_ = this->resolve_wave(ERROR_SOUND);
	this->ready_sound_ = this->resolve_wave(READY_SOUND);
	this->music_ = this->resolve_effect(MUSIC);
}

MainMenuData* MainMenuPage::get_main_menu_data() const
{
	return this->data_;
}
int MainMenuPage::get_player_count() const
{ 
	return this->get_main_menu_data()->get_level_settings()->
		get_player_count();
}

#pragma region MainMenuTitle

MainMenuTitle::MainMenuTitle(MainMenuData* data) :
	MainMenuPage(data)
{

}

void MainMenuTitle::draw()
{
	std::vector<std::pair<MObject*, ID3D11SamplerState*>> mobjects;

	mobjects.push_back(std::make_pair(this->texture_container_.get(),
		this->get_point_clamp_sampler_state()));

	mobjects.push_back(std::make_pair(this->text_container_.get(), nullptr));

	this->draw_mobjects_in_viewports(&mobjects);
}
void MainMenuTitle::update()
{
	const std::vector<ProcessedMenuInput> inputs = this->get_menu_inputs();
	for (int i = 0; i < inputs.size(); i++)
	{
		if (inputs[i].action == menu_input_action::PROCEED)
		{
			this->play_wave(this->confirm_sound_);
			this->get_context()->transition_to(std::make_unique<MainMenuHome>(
				this->get_main_menu_data()));
			return;
		}
	}
}
void MainMenuTitle::init()
{
	this->background_ = std::make_unique<MTexture>(
		"background",
		"sprite_sheet_1",
		"square_white_4",
		RectangleF(Vector2F::ZERO, DEFAULT_RESOLUTION),
		this->get_render_resources(),
		TITLE_BACKGROUND_COLOUR);

	this->title_ = std::make_unique<MTextDropShadow>(
		"title",
		"Colour Wars",
		TITLE_FONT,
		Vector2F(200.0f, 300.0f),
		this->get_render_resources(),
		TITLE_TEXT_COLOUR,
		SHADOW_COLOUR,
		TITLE_SHADOW_OFFSET);

	this->start_ = std::make_unique<MTextDropShadow>(
		"start",
		"Start",
		ITEM_FONT,
		Vector2F(250.0f, 700.0f),
		this->get_render_resources(),
		TITLE_START_TEXT_COLOUR,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->texture_container_ = std::make_unique<MContainer>(
		"texture_container");
	this->texture_container_->add_child(this->background_.get());

	this->text_container_ = std::make_unique<MContainer>(
		"text_container");
	this->text_container_->add_child(this->title_.get());
	this->text_container_->add_child(this->start_.get());

	Vector2F resolution = this->get_float_resolution();
	this->texture_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->text_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);

	this->play_effect(this->music_, true, MUSIC_VOLUME);
}

#pragma endregion MainMenuTitle

#pragma region MainMenuHome

MainMenuHome::MainMenuHome(MainMenuData* data) :
	MainMenuPage(data)
{

}

void MainMenuHome::draw()
{
	std::vector<std::pair<MObject*, ID3D11SamplerState*>> mobjects;

	mobjects.push_back(std::make_pair(this->texture_container_.get(),
		this->get_point_clamp_sampler_state()));

	mobjects.push_back(std::make_pair(this->text_container_.get(), nullptr));

	this->draw_mobjects_in_viewports(&mobjects);
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
			this->play_wave(this->cancel_sound_);
			this->get_context()->transition_to(
				std::make_unique<MainMenuTitle>(this->get_main_menu_data()));
			return;
		}
		if (inputs[i].action == menu_input_action::PROCEED)
		{
			if (highlighted_element == "play")
			{
				this->play_wave(this->confirm_sound_);
				this->get_context()->transition_to(
					std::make_unique<MainMenuModeSelect>(
						this->get_main_menu_data()));
				return;
			}
			if (highlighted_element == "options")
			{
				this->play_wave(this->confirm_sound_);
				this->get_context()->transition_to(
					std::make_unique<MainMenuOptions>(
						this->get_main_menu_data()));
				return;
			}
			if (highlighted_element == "exit")
			{
				this->get_main_menu_data()->get_application()->quit();
			}
		}
		else if (inputs[i].direction == menu_direction::UP)
		{
			this->play_wave(this->direction_sound_);
			if (highlighted_element == "play")
			{
				this->change_highlight(this->exit_.get());
			}
			else if (highlighted_element == "options")
			{
				this->change_highlight(this->play_.get());
			}
			else if (highlighted_element == "exit")
			{
				this->change_highlight(this->options_.get());
			}
		}
		else if (inputs[i].direction == menu_direction::DOWN)
		{
			this->play_wave(this->direction_sound_);
			if (highlighted_element == "play")
			{
				this->change_highlight(this->options_.get());
			}
			else if (highlighted_element == "options")
			{
				this->change_highlight(this->exit_.get());
			}
			else if (highlighted_element == "exit")
			{
				this->change_highlight(this->play_.get());
			}
		}
	}
}
void MainMenuHome::init()
{
	this->set_highlight_colour(STANDARD_HIGHLIGHT);
	this->set_unhighlight_colour(STANDARD_UNHIGHLIGHT);

	this->background_ = std::make_unique<MTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DEFAULT_RESOLUTION),
		this->get_render_resources(),
		HOME_BACKGROUND_COLOUR);

	this->heading_ = std::make_unique<MTextDropShadow>(
		"heading",
		"Main Menu",
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->get_render_resources(),
		HEADING_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->play_ = std::make_unique<MTextDropShadow>(
		"play",
		"Play",
		ITEM_FONT,
		this->calculate_widget_position(0, 2),
		this->get_render_resources(),
		this->get_highlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->options_ = std::make_unique<MTextDropShadow>(
		"options",
		"Options",
		ITEM_FONT,
		this->calculate_widget_position(0, 3),
		this->get_render_resources(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->exit_ = std::make_unique<MTextDropShadow>(
		"exit",
		"Exit",
		ITEM_FONT,
		this->calculate_widget_position(0, 4),
		this->get_render_resources(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->texture_container_ = std::make_unique<MContainer>(
		"texture_container");
	this->texture_container_->add_child(this->background_.get());

	this->text_container_ = std::make_unique<MContainer>(
		"text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->play_.get());
	this->text_container_->add_child(this->options_.get());
	this->text_container_->add_child(this->exit_.get());

	Vector2F resolution = this->get_float_resolution();
	this->texture_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->text_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);

	this->set_highlighted_widget(this->play_.get());

	this->play_effect(this->music_, true, MUSIC_VOLUME);
}

#pragma endregion MainMenuHome

#pragma region MainMenuOptions

MainMenuOptions::MainMenuOptions(MainMenuData* data) :
	MainMenuPage(data)
{

}

void MainMenuOptions::draw()
{
	std::vector<std::pair<MObject*, ID3D11SamplerState*>> mobjects;

	mobjects.push_back(std::make_pair(this->texture_container_.get(),
		this->get_point_clamp_sampler_state()));

	mobjects.push_back(std::make_pair(this->text_container_.get(), nullptr));

	this->draw_mobjects_in_viewports(&mobjects);
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
			this->play_wave(this->cancel_sound_);
			this->get_context()->transition_to(std::make_unique<MainMenuHome>(
				this->get_main_menu_data()));
			return;
		}
		else if (inputs[i].action == menu_input_action::PROCEED)
		{
			if (highlighted_element == "apply")
			{
				this->play_wave(this->confirm_sound_);
				
				auto prev_resolution = this->get_resolution_manager()->get_resolution_vec();
				
				this->get_resolution_manager()->set_resolution(
					this->resolution_selection_);

				//std::string res_string = this->get_resolution_manager()->
				//	get_resolution_string();
				this->get_save()->set_resolution_and_save(this->resolution_selection_);

				auto res_f = this->get_float_resolution();

				this->get_data()->get_application()->set_resolution(
					this->resolution_selection_);

				bool fs = this->full_screen_selection_;
				this->get_save()->set_full_screen_and_save(fs);
				this->apply_fullscreen_setting(fs);

				//update window and its children's sizes and positions
				this->texture_container_->scale_objects_to_new_resolution(
					prev_resolution, res_f);
				this->text_container_->scale_objects_to_new_resolution(
					prev_resolution, res_f);
				
				//this->background_->update_resolution(prev_resolution, res_f);
			}
			else if (highlighted_element == "back")
			{
				this->play_wave(this->cancel_sound_);
				this->get_context()->transition_to(
					std::make_unique<MainMenuHome>(
						this->get_main_menu_data()));
				return;
			}
		}
		else if (inputs[i].direction == menu_direction::DOWN)
		{
			this->play_wave(this->direction_sound_);
			if (highlighted_element == "resolution_element")
			{
				this->change_highlight(this->full_screen_.get());
			}
			else if (highlighted_element == "full_screen")
			{
				this->change_highlight(this->apply_.get());
			}
			else if (highlighted_element == "apply")
			{
				this->change_highlight(this->back_.get());
			}
			else if (highlighted_element == "back")
			{
				this->change_highlight(this->resolution_element_.get());
			}
		}
		else if (inputs[i].direction == menu_direction::UP)
		{
			this->play_wave(this->direction_sound_);
			if (highlighted_element == "resolution_element")
			{
				this->change_highlight(this->back_.get());
			}
			else if (highlighted_element == "full_screen")
			{
				this->change_highlight(this->resolution_element_.get());
			}
			else if (highlighted_element == "apply")
			{
				this->change_highlight(this->full_screen_.get());
			}
			else if (highlighted_element == "back")
			{
				this->change_highlight(this->apply_.get());
			}
		}
		else if (highlighted_element == "resolution_element")
		{
			if (inputs[i].direction == menu_direction::LEFT)
			{
				this->play_wave(this->direction_sound_);
				this->cycle_resolution(menu_direction::LEFT);
			}
			else if (inputs[i].direction == menu_direction::RIGHT)
			{
				this->play_wave(this->direction_sound_);
				this->cycle_resolution(menu_direction::RIGHT);
			}
		}
		else if (highlighted_element == "full_screen" &&
			(inputs[i].direction == menu_direction::LEFT ||
			inputs[i].direction == menu_direction::RIGHT))
		{
			this->play_wave(this->direction_sound_);
			this->full_screen_selection_ = !this->full_screen_selection_;
			this->update_full_screen_selection_text();
		}
	}
}

void MainMenuOptions::apply_fullscreen_setting(bool fullscreen)
{
	this->get_data()->get_application()->set_fullscreen(fullscreen);
}

void MainMenuOptions::init()
{
	this->set_highlight_colour(STANDARD_HIGHLIGHT);
	this->set_unhighlight_colour(STANDARD_UNHIGHLIGHT);
	Vector2F widget_size = this->get_widget_size();

	this->resolution_selection_ = this->get_resolution_manager()->get_resolution();
	this->full_screen_selection_ = this->get_save()->get_fullscreen();

	this->background_ = std::make_unique<MTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DEFAULT_RESOLUTION),
		this->get_render_resources(),
		OPTIONS_BACKGROUND_COLOUR);

	this->heading_ = std::make_unique<MTextDropShadow>(
		"heading",
		"Options",
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->get_render_resources(),
		HEADING_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->resolution_element_ = std::make_unique<MTextDropShadow>(
		"resolution_element",
		"Resolution",
		ITEM_FONT,
		this->calculate_widget_position(0, 2),
		this->get_render_resources(),
		this->get_highlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->resolution_value_ = std::make_unique<MTextDropShadow>(
		"resolution_value",
		"null",
		ITEM_FONT,
		this->calculate_widget_position(2, 2),
		this->get_render_resources(),
		OPTIONS_VALUE_COLOUR,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->update_resolution_selection_text();

	this->full_screen_ = std::make_unique<MTextDropShadow>(
		"full_screen",
		"Fullscreen",
		ITEM_FONT,
		this->calculate_widget_position(0, 3),
		this->get_render_resources(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->full_screen_value_ = std::make_unique<MTextDropShadow>(
		"full_screen_value",
		"null",
		ITEM_FONT,
		this->calculate_widget_position(2, 3),
		this->get_render_resources(),
		OPTIONS_VALUE_COLOUR,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->update_full_screen_selection_text();

	this->apply_ = std::make_unique<MTextDropShadow>(
		"apply",
		"Apply",
		ITEM_FONT,
		this->calculate_widget_position(0, 4),
		this->get_render_resources(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->back_ = std::make_unique<MTextDropShadow>(
		"back",
		"Back",
		ITEM_FONT,
		this->calculate_widget_position(0, 5),
		this->get_render_resources(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->texture_container_ = std::make_unique<MContainer>(
		"texture_container");
	this->texture_container_->add_child(this->background_.get());

	this->text_container_ = std::make_unique<MContainer>(
		"text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->resolution_element_.get());
	this->text_container_->add_child(this->resolution_value_.get());
	this->text_container_->add_child(this->full_screen_.get());
	this->text_container_->add_child(this->full_screen_value_.get());
	this->text_container_->add_child(this->apply_.get());
	this->text_container_->add_child(this->back_.get());

	Vector2F resolution = this->get_float_resolution();
	this->texture_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->text_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);

	this->set_highlighted_widget(this->resolution_element_.get());

	this->play_effect(this->music_, true, MUSIC_VOLUME);
}

void MainMenuOptions::cycle_resolution(
	menu_direction direction)
{
	int enum_max = static_cast<int>(screen_resolution::MAX);
	int enum_pos =
		static_cast<int>(this->resolution_selection_);
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
	this->resolution_selection_ =
		static_cast<screen_resolution>(enum_pos);
	this->update_resolution_selection_text();
}

void MainMenuOptions::update_resolution_selection_text()
{
	std::string res_text = this->get_resolution_manager()->
		convert_resolution_to_string(this->resolution_selection_);
	this->resolution_value_->set_text(res_text);
}

void MainMenuOptions::update_full_screen_selection_text() const
{
	bool fs = this->full_screen_selection_; //this->get_context()->get_data()->get_save()->get_full_screen();
	std::string fs_text = "";
	if (fs)
	{
		fs_text = "On";
	}
	else
	{
		fs_text = "Off";
	}
	this->full_screen_value_->set_text(fs_text);
}

#pragma endregion MainMenuOptions

#pragma region MainMenuModeSelect

MainMenuModeSelect::MainMenuModeSelect(MainMenuData* data) :
	MainMenuPage(data)
{

}

void MainMenuModeSelect::draw()
{
	std::vector<std::pair<MObject*, ID3D11SamplerState*>> mobjects;

	mobjects.push_back(std::make_pair(this->texture_container_.get(),
		this->get_point_clamp_sampler_state()));

	mobjects.push_back(std::make_pair(this->text_container_.get(), nullptr));

	this->draw_mobjects_in_viewports(&mobjects);
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
			this->play_wave(this->cancel_sound_);
			this->get_context()->transition_to(std::make_unique<MainMenuHome>(
				this->get_main_menu_data()));
			return;
		}
		if (inputs[i].action == menu_input_action::PROCEED)
		{
			if (highlighted_element == "standard")
			{
				this->play_wave(this->confirm_sound_);
				this->get_main_menu_data()->get_level_settings()->
					set_game_mode(level_mode::STANDARD_MODE);
				this->get_context()->transition_to(
					std::make_unique<MainMenuPlayerCount>(
						this->get_main_menu_data()));
				return;
			}
			if (highlighted_element == "tdm")
			{
				this->play_wave(this->error_sound_);
			}
			else if (highlighted_element == "dm")
			{
				this->play_wave(this->error_sound_);
			}
			else if (highlighted_element == "practice")
			{
				this->play_wave(this->error_sound_);
			}
			else if (highlighted_element == "back")
			{
				this->play_wave(this->cancel_sound_);
				this->get_context()->transition_to(
					std::make_unique<MainMenuHome>(
						this->get_main_menu_data()));
				return;
			}
		}
		else if (inputs[i].direction == menu_direction::UP)
		{
			this->play_wave(this->direction_sound_);
			if (highlighted_element == "standard")
			{
				this->change_highlight(this->back_.get());
			}
			else if (highlighted_element == "tdm")
			{
				this->change_highlight(this->standard_.get());
			}
			else if (highlighted_element == "dm")
			{
				this->change_highlight(this->tdm_.get());
			}
			else if (highlighted_element == "practice")
			{
				this->change_highlight(this->dm_.get());
			}
			else if (highlighted_element == "back")
			{
				this->change_highlight(this->practice_.get());
			}
		}
		else if (inputs[i].direction == menu_direction::DOWN)
		{
			this->play_wave(this->direction_sound_);
			if (highlighted_element == "standard")
			{
				this->change_highlight(this->tdm_.get());
			}
			else if (highlighted_element == "tdm")
			{
				this->change_highlight(this->dm_.get());
			}
			else if (highlighted_element == "dm")
			{
				this->change_highlight(this->practice_.get());
			}
			else if (highlighted_element == "practice")
			{
				this->change_highlight(this->back_.get());
			}
			else if (highlighted_element == "back")
			{
				this->change_highlight(this->standard_.get());
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

	this->background_ = std::make_unique<MTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DEFAULT_RESOLUTION),
		this->get_render_resources(),
		PLAY_BACKGROUND_COLOUR);

	this->heading_ = std::make_unique<MTextDropShadow>(
		"heading",
		"Mode Select",
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->get_render_resources(),
		HEADING_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->standard_ = std::make_unique<MTextDropShadow>(
		"standard",
		"Standard",
		ITEM_FONT,
		this->calculate_widget_position(0, 2),
		this->get_render_resources(),
		this->get_highlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->tdm_ = std::make_unique<MTextDropShadow>(
		"tdm",
		"Team Deathmatch",
		ITEM_FONT,
		this->calculate_widget_position(0, 3),
		this->get_render_resources(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->dm_ = std::make_unique<MTextDropShadow>(
		"dm",
		"Deathmatch",
		ITEM_FONT,
		this->calculate_widget_position(0, 4),
		this->get_render_resources(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->practice_ = std::make_unique<MTextDropShadow>(
		"practice",
		"Practice",
		ITEM_FONT,
		this->calculate_widget_position(0, 5),
		this->get_render_resources(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->back_ = std::make_unique<MTextDropShadow>(
		"back",
		"Back",
		ITEM_FONT,
		this->calculate_widget_position(0, 6),
		this->get_render_resources(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->texture_container_ = std::make_unique<MContainer>(
		"texture_container");
	this->texture_container_->add_child(this->background_.get());

	this->text_container_ = std::make_unique<MContainer>(
		"text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->standard_.get());
	this->text_container_->add_child(this->tdm_.get());
	this->text_container_->add_child(this->dm_.get());
	this->text_container_->add_child(this->practice_.get());
	this->text_container_->add_child(this->back_.get());

	Vector2F resolution = this->get_float_resolution();
	this->texture_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->text_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);

	this->set_highlighted_widget(this->standard_.get());

	this->play_effect(this->music_, true, MUSIC_VOLUME);
}

#pragma endregion MainMenuModeSelect

#pragma region MainMenuPlayerCount

MainMenuPlayerCount::MainMenuPlayerCount(MainMenuData* data) :
	MainMenuPage(data)
{

}
void MainMenuPlayerCount::draw()
{
	std::vector<std::pair<MObject*, ID3D11SamplerState*>> mobjects;

	mobjects.push_back(std::make_pair(this->texture_container_.get(),
		this->get_point_clamp_sampler_state()));

	mobjects.push_back(std::make_pair(this->text_container_.get(), nullptr));

	this->draw_mobjects_in_viewports(&mobjects);
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
			this->play_wave(this->cancel_sound_);
			this->get_context()->transition_to(
				std::make_unique<MainMenuModeSelect>(
					this->get_main_menu_data()));
			return;
		}
		if (inputs[i].action == menu_input_action::PROCEED)
		{
			if (highlighted_element == "1_player")
			{
				this->play_wave(this->confirm_sound_);
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
				this->play_wave(this->confirm_sound_);
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
				this->play_wave(this->confirm_sound_);
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
				this->play_wave(this->confirm_sound_);
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
				this->play_wave(this->cancel_sound_);
				this->get_context()->transition_to(
					std::make_unique<MainMenuModeSelect>(
						this->get_main_menu_data()));
				return;
			}
		}
		else if (inputs[i].direction == menu_direction::UP)
		{
			this->play_wave(this->direction_sound_);
			if (highlighted_element == "1_player")
			{
				this->change_highlight(this->back_.get());
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
			this->play_wave(this->direction_sound_);
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
				this->change_highlight(this->back_.get());
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

	this->background_ = std::make_unique<MTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DEFAULT_RESOLUTION),
		this->get_render_resources(),
		PLAY_BACKGROUND_COLOUR);

	this->heading_ = std::make_unique<MTextDropShadow>(
		"heading",
		"Number of Players",
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->get_render_resources(),
		HEADING_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->_1_player = std::make_unique<MTextDropShadow>(
		"1_player",
		"1 Player",
		ITEM_FONT,
		this->calculate_widget_position(0, 2),
		this->get_render_resources(),
		this->get_highlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_2_players = std::make_unique<MTextDropShadow>(
		"2_players",
		"2 Players",
		ITEM_FONT,
		this->calculate_widget_position(0, 3),
		this->get_render_resources(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_3_players = std::make_unique<MTextDropShadow>(
		"3_players",
		"3 Players",
		ITEM_FONT,
		this->calculate_widget_position(0, 4),
		this->get_render_resources(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->_4_players = std::make_unique<MTextDropShadow>(
		"4_players",
		"4 Players",
		ITEM_FONT,
		this->calculate_widget_position(0, 5),
		this->get_render_resources(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->back_ = std::make_unique<MTextDropShadow>(
		"back",
		"Back",
		ITEM_FONT,
		this->calculate_widget_position(0, 6),
		this->get_render_resources(),
		this->get_unhighlight_colour(),
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->texture_container_ = std::make_unique<MContainer>(
		"texture_container");
	this->texture_container_->add_child(this->background_.get());

	this->text_container_ = std::make_unique<MContainer>(
		"text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->_1_player.get());
	this->text_container_->add_child(this->_2_players.get());
	this->text_container_->add_child(this->_3_players.get());
	this->text_container_->add_child(this->_4_players.get());
	this->text_container_->add_child(this->back_.get());

	Vector2F resolution = this->get_float_resolution();
	this->texture_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->text_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);

	this->set_highlighted_widget(this->_1_player.get());

	this->play_effect(this->music_, true, MUSIC_VOLUME);
}

#pragma endregion MainMenuPlayerCount

#pragma region MainMenuTeamSelect

MainMenuTeamSelect::MainMenuTeamSelect(MainMenuData* data) :
	MainMenuPage(data)
{

}

MainMenuTeamSelect::MainMenuTeamSelect(MainMenuData* data,
	MenuLevelSettings* settings) :
	MainMenuPage(data)
{
	this->get_main_menu_data()->set_level_settings(settings);
}
void MainMenuTeamSelect::draw()
{
	std::vector<std::pair<MObject*, ID3D11SamplerState*>> mobjects;

	mobjects.push_back(std::make_pair(this->texture_container_.get(),
		this->get_point_clamp_sampler_state()));

	mobjects.push_back(std::make_pair(this->text_container_.get(), nullptr));

	this->draw_mobjects_in_viewports(&mobjects);
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
				this->play_wave(this->cancel_sound_);
				this->get_context()->transition_to(
					std::make_unique<MainMenuPlayerCount>(
						this->get_main_menu_data()));
				return;
			}
			if (this->select_states_[i].state == confirmation_state::CONFIRMED)
			{
				this->play_wave(this->cancel_sound_);
				this->select_states_[i].state =
					confirmation_state::UNCONFIRMED;
			}
			//no break here as I want simultaneous input from players to be possible
		}
		else if (inputs[i].action == menu_input_action::PROCEED &&
			this->select_states_[i].team != player_team::NONE)
		{
			this->play_wave(this->confirm_sound_);
			this->select_states_[i].state = confirmation_state::CONFIRMED;
		}
		else if (this->select_states_[i].state ==
			confirmation_state::UNCONFIRMED)
		{
			if (inputs[i].direction == menu_direction::LEFT)
			{
				this->play_wave(this->direction_sound_);
				if (this->select_states_[i].team == player_team::NONE)
				{
					this->select_states_[i].team = player_team::A;
				}
				else if (this->select_states_[i].team == player_team::B)
				{
					this->select_states_[i].team = player_team::NONE;
				}
			}
			else if (inputs[i].direction == menu_direction::RIGHT)
			{
				this->play_wave(this->direction_sound_);
				if (this->select_states_[i].team == player_team::NONE)
				{
					this->select_states_[i].team = player_team::B;
				}
				else if (this->select_states_[i].team == player_team::A)
				{
					this->select_states_[i].team = player_team::NONE;
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
			set_player_team(i, this->select_states_[i].team);

		this->get_main_menu_data()->get_level_settings()->
			set_player_num(i, i);
	}
}
bool MainMenuTeamSelect::all_players_confirmed() const
{
	for (int i = 0; i < this->get_player_count(); i++)
	{
		if (this->select_states_[i].state != confirmation_state::CONFIRMED)
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
		if (this->select_states_[i].state == confirmation_state::CONFIRMED)
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
		switch (this->select_states_[i].team)
		{
		case player_team::A:
			this->player_widgets_[i]->player_a->set_sprite_frame(
				"team_select_a_selected");
			selected_widget = this->player_widgets_[i]->player_a.get();
			break;
		case player_team::NONE:
			this->player_widgets_[i]->player_center->set_sprite_frame(
				"team_select_center_selected");
			selected_widget = this->player_widgets_[i]->player_center.get();
			break;
		case player_team::B:
			this->player_widgets_[i]->player_b->set_sprite_frame(
				"team_select_b_selected");
			selected_widget = this->player_widgets_[i]->player_b.get();
			break;
		}
		if (this->select_states_[i].state ==
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
		this->player_widgets_[i]->player_a->set_sprite_frame(
			"team_select_a");
		this->player_widgets_[i]->player_center->set_sprite_frame(
			"team_select_center");
		this->player_widgets_[i]->player_b->set_sprite_frame(
			"team_select_b");

		this->player_widgets_[i]->player_a->set_colour(
			main_menu_consts::TEAM_SELECT_UNSELECTED_COLOUR);
		this->player_widgets_[i]->player_center->set_colour(
			main_menu_consts::TEAM_SELECT_UNSELECTED_COLOUR);
		this->player_widgets_[i]->player_b->set_colour(
			main_menu_consts::TEAM_SELECT_UNSELECTED_COLOUR);
	}
}
void MainMenuTeamSelect::init()
{
	this->set_widget_spacing(TEAM_SELECT_WIDGET_SPACING);

	const Vector2F resolution = this->get_float_resolution();

	this->texture_container_ = std::make_unique<MContainer>(
		"texture_container");
	
	this->text_container_ = std::make_unique<MContainer>(
		"text_container");

	this->background_ = std::make_unique<MTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DEFAULT_RESOLUTION),
		this->get_render_resources(),
		PLAY_BACKGROUND_COLOUR);

	this->texture_container_->add_child(this->background_.get());

	this->heading_ = std::make_unique<MTextDropShadow>(
		"heading",
		"Team Select",
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->get_render_resources(),
		HEADING_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->text_container_->add_child(this->heading_.get());

	
	for (int i = 0; i < this->get_player_count(); i++)
	{
		auto state = TeamSelectState();
		state.state = confirmation_state::UNCONFIRMED;
		state.team = this->get_main_menu_data()->get_level_settings()->
			get_player_team(i);
		this->select_states_.push_back(state);

		auto widgets = std::make_unique<PlayerWidgets>();

		std::string name = "player_" + std::to_string(i + 1);
		std::string label_text = "Player " + std::to_string(i + 1);

		widgets->player = std::make_unique<MTextDropShadow>(
			name,
			label_text,
			ITEM_FONT,
			this->calculate_widget_position(0, i + 2),
			this->get_render_resources(),
			TEAM_SELECT_UNSELECTED_COLOUR,
			SHADOW_COLOUR,
			ITEM_SHADOW_OFFSET);

		widgets->player_a = std::make_unique<MTexture>(
			name + "_a",
			"sprite_sheet_1",
			"team_select_a",
			RectangleF(this->calculate_widget_position(2, i + 2), TEAM_SELECT_TEAM_WIDGET_SIZE),
			this->get_render_resources());

		auto move = Vector2F(TEAM_SELECT_TEAM_WIDGET_SIZE.x, 0.0f);

		widgets->player_center = std::make_unique<MTexture>(
			name + "_center",
			"sprite_sheet_1",
			"team_select_center",
			RectangleF(widgets->player_a->get_rectangle().get_position() + move,
				TEAM_SELECT_TEAM_WIDGET_SIZE),
			this->get_render_resources());

		widgets->player_b = std::make_unique<MTexture>(
			name + "_b",
			"sprite_sheet_1",
			"team_select_b",
			RectangleF(widgets->player_center->get_rectangle().get_position() + move,
				TEAM_SELECT_TEAM_WIDGET_SIZE),
			this->get_render_resources());

		this->texture_container_->add_child(widgets->player_a.get());
		this->texture_container_->add_child(widgets->player_center.get());
		this->texture_container_->add_child(widgets->player_b.get());

		this->text_container_->add_child(widgets->player.get());

		this->player_widgets_.push_back(std::move(widgets));
	}

	this->texture_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->text_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);

	this->play_effect(this->music_, true, MUSIC_VOLUME);
}

#pragma endregion MainMenuTeamSelect

#pragma region MainMenuWeaponSelect

MainMenuWeaponSelect::MainMenuWeaponSelect(MainMenuData* data) :
	MainMenuPage(data)
{

}

MainMenuWeaponSelect::MainMenuWeaponSelect(MainMenuData* data,
	MenuLevelSettings* settings) :
	MainMenuPage(data)
{
	this->get_main_menu_data()->set_level_settings(settings);
}
void MainMenuWeaponSelect::draw()
{
	std::vector<std::pair<MObject*, ID3D11SamplerState*>> mobjects;

	mobjects.push_back(std::make_pair(this->texture_container_.get(),
		this->get_point_clamp_sampler_state()));

	mobjects.push_back(std::make_pair(this->text_container_.get(), nullptr));

	this->draw_mobjects_in_viewports(&mobjects);
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
				this->play_wave(this->cancel_sound_);
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
			if (this->select_states_[i].state ==
				confirmation_state::CONFIRMED)
			{
				this->play_wave(this->cancel_sound_);
				this->select_states_[i].state =
					confirmation_state::UNCONFIRMED;
			}
			//no break here as I want simultaneous input
			// from players to be possible
		}
		else if (inputs[i].action == menu_input_action::PROCEED)
		{
			this->play_wave(this->confirm_sound_);
			this->select_states_[i].state = confirmation_state::CONFIRMED;
		}
		else if (this->select_states_[i].state ==
			confirmation_state::UNCONFIRMED)
		{
			if (inputs[i].direction == menu_direction::LEFT)
			{
				this->play_wave(this->direction_sound_);
				this->cycle_weapons(menu_direction::LEFT, i);
			}
			else if (inputs[i].direction == menu_direction::RIGHT)
			{
				this->play_wave(this->direction_sound_);
				this->cycle_weapons(menu_direction::RIGHT, i);
			}
		}
	}
	if (this->all_players_confirmed())
	{
		//check if any players have selected random then pick a random weapon.
		for (int i = 0; i < this->get_player_count(); i++)
		{
			if (this->select_states_[i].type == wep_type::RANDOM_PRIMARY)
			{
				this->select_states_[i].type = get_random_weapon();
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
			set_player_weapon(i, this->select_states_[i].type);
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
		static_cast<int>(this->select_states_[player_index].type);
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
	this->select_states_[player_index].type =
		static_cast<wep_type>(enum_pos);
}
bool MainMenuWeaponSelect::all_players_confirmed() const
{
	for (int i = 0; i < this->get_player_count(); i++)
	{
		if (this->select_states_[i].state != confirmation_state::CONFIRMED)
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
		if (this->select_states_[i].state == confirmation_state::CONFIRMED)
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
		switch (this->select_states_[i].type)
		{
		case wep_type::SPRAYER:
			this->player_widgets_[i]->weapon_icon->set_sprite_frame("sprayer");
			this->player_widgets_[i]->weapon_name->set_text("Sprayer");
			break;
		case wep_type::SNIPER:
			this->player_widgets_[i]->weapon_icon->set_sprite_frame("sniper");
			this->player_widgets_[i]->weapon_name->set_text("Sniper");
			break;
		case wep_type::ROLLER:
			this->player_widgets_[i]->weapon_icon->set_sprite_frame("roller");
			this->player_widgets_[i]->weapon_name->set_text("Roller");
			break;
		case wep_type::MISTER:
			this->player_widgets_[i]->weapon_icon->set_sprite_frame("mister");
			this->player_widgets_[i]->weapon_name->set_text("Mister");
			break;
		case wep_type::BUCKET:
			this->player_widgets_[i]->weapon_icon->set_sprite_frame("bucket");
			this->player_widgets_[i]->weapon_name->set_text("Bucket");
			break;
		case wep_type::RANDOM_PRIMARY:
			this->player_widgets_[i]->weapon_icon->set_sprite_frame("random");
			this->player_widgets_[i]->weapon_name->set_text("Random");
			break;
		default:
			this->player_widgets_[i]->weapon_icon->set_sprite_frame("error");
			this->player_widgets_[i]->weapon_name->set_text("ERROR");
			break;
		}

		if (this->select_states_[i].state == confirmation_state::CONFIRMED)
		{
			this->player_widgets_[i]->weapon_icon->
				set_colour(WEAPON_SELECT_SELECTED_COLOUR);
			this->player_widgets_[i]->weapon_name->
				set_colour(WEAPON_SELECT_SELECTED_COLOUR);
		}

		this->player_widgets_[i]->weapon_description->set_text(
			this->weapon_description(this->select_states_[i].type));
	}
}
void MainMenuWeaponSelect::unconfirm_all_widgets()
{
	for (int i = 0; i < this->get_player_count(); i++)
	{
		this->player_widgets_[i]->weapon_icon->set_colour(
			WEAPON_SELECT_UNSELECTED_COLOUR);
		this->player_widgets_[i]->weapon_name->set_colour(
			WEAPON_SELECT_UNSELECTED_COLOUR);
	}
}
void MainMenuWeaponSelect::init()
{
	this->set_widget_spacing(WEAPON_SELECT_WIDGET_SPACING);

	const Vector2F resolution = this->get_float_resolution();

	this->texture_container_ = std::make_unique<MContainer>(
		"texture_container");
	
	this->text_container_ = std::make_unique<MContainer>(
		"text_container");

	this->background_ = std::make_unique<MTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DEFAULT_RESOLUTION),
		this->get_render_resources(),
		PLAY_BACKGROUND_COLOUR);

	this->texture_container_->add_child(this->background_.get());

	this->heading_ = std::make_unique<MTextDropShadow>(
		"heading",
		"Weapon Select",
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->get_render_resources(),
		HEADING_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->text_container_->add_child(this->heading_.get());

	for (int i = 0; i < this->get_player_count(); i++)
	{
		SelectState state;
		state.state = confirmation_state::UNCONFIRMED;
		state.type = this->get_main_menu_data()->get_level_settings()
			->get_player_weapon(i);
		this->select_states_.push_back(state);

		auto widgets = std::make_unique<Widgets>();

		std::string name = "player_" + std::to_string(i + 1);
		std::string label_text = "Player " + std::to_string(i + 1);

		widgets->player = std::make_unique<MTextDropShadow>(
			name,
			label_text,
			ITEM_FONT,
			this->calculate_widget_position(0, i + 2),
			this->get_render_resources(),
			WEAPON_SELECT_UNSELECTED_COLOUR);

		widgets->weapon_icon = std::make_unique<MTexture>(
			name + "_wep_icon",
			"sprite_sheet_1",
			"sprayer",
			RectangleF(this->calculate_widget_position(2, i + 2), WEAPON_SELECT_WEAPON_WIDGET_SIZE),
			this->get_render_resources(),
			PLAY_BACKGROUND_COLOUR);

		widgets->weapon_name = std::make_unique<MTextDropShadow>(
			name + "_wep_name",
			"Sprayer",
			DETAIL_FONT,
			widgets->weapon_icon->get_rectangle().get_position() +
				Vector2F(0.0f, WEAPON_SELECT_WEAPON_WIDGET_SIZE.y),
			this->get_render_resources(),
			WEAPON_SELECT_UNSELECTED_COLOUR,
			SHADOW_COLOUR,
			DETAIL_SHADOW_OFFSET);

		widgets->weapon_description = std::make_unique<MTextDropShadow>(
			name + "_wep_desc",
			this->weapon_description(wep_type::SPRAYER),
			WEAPON_DESCRIPTION_FONT,
			widgets->weapon_icon->get_rectangle().get_position() +
				Vector2F(WEAPON_DESC_X_OFFSET, 0.0f),
			this->get_render_resources(),
			WEAPON_DESCRIPTION_FONT_COLOUR,
			SHADOW_COLOUR,
			WEAPON_DESCRIPTION_SHADOW_OFFSET);

		this->texture_container_->add_child(widgets->weapon_icon.get());
		this->text_container_->add_child(widgets->weapon_name.get());
		this->text_container_->add_child(widgets->player.get());
		this->text_container_->add_child(widgets->weapon_description.get());

		this->player_widgets_.push_back(std::move(widgets));
	}
	this->texture_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->text_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);

	this->play_effect(this->music_, true, MUSIC_VOLUME);
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

MainMenuStageSelect::MainMenuStageSelect(MainMenuData* data) :
	MainMenuPage(data)
{

}

MainMenuStageSelect::MainMenuStageSelect(MainMenuData* data,
	MenuLevelSettings* settings) :
	MainMenuPage(data)
{
	this->get_main_menu_data()->set_level_settings(settings);
}
void MainMenuStageSelect::draw()
{
	std::vector<std::pair<MObject*, ID3D11SamplerState*>> mobjects;

	mobjects.push_back(std::make_pair(this->texture_container_.get(),
		this->get_point_clamp_sampler_state()));

	mobjects.push_back(std::make_pair(this->text_container_.get(), nullptr));

	this->draw_mobjects_in_viewports(&mobjects);
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
			if (this->select_state_.state == confirmation_state::UNCONFIRMED)
			{
				this->play_wave(this->cancel_sound_);
				this->get_context()->transition_to(
					std::make_unique<MainMenuWeaponSelect>(
						this->get_main_menu_data()));
				return;
			}
			if (this->select_state_.state == confirmation_state::CONFIRMED)
			{
				this->play_wave(this->cancel_sound_);
				this->select_state_.state =
					confirmation_state::UNCONFIRMED;
			}
			//no break here as I want simultaneous input from players to be possible
		}
		else if (inputs[i].action == menu_input_action::PROCEED)
		{
			if (this->select_state_.state == confirmation_state::CONFIRMED)
			{
				this->play_wave(this->confirm_sound_);
				this->stop_effect(this->music_, true);
				if (this->select_state_.stage == level_stage::RANDOM)
				{
					this->select_state_.stage = this->get_random_stage();
				}
				this->set_level_settings();
				*this->get_main_menu_data()->
					get_is_ready_to_load_level() = true;
				//no need to transition here as we should be going to load level
			}
			else
			{
				this->play_wave(this->ready_sound_);
				this->select_state_.state = confirmation_state::CONFIRMED;
			}
		}
		else if (this->select_state_.state == confirmation_state::UNCONFIRMED)
		{
			if (inputs[i].direction == menu_direction::LEFT)
			{
				this->play_wave(this->direction_sound_);
				this->cycle_stages(menu_direction::LEFT);
			}
			else if (inputs[i].direction == menu_direction::RIGHT)
			{
				this->play_wave(this->direction_sound_);
				this->cycle_stages(menu_direction::RIGHT);
			}
		}
	}
	this->update_stage_select_visuals();
}
void MainMenuStageSelect::set_level_settings() const
{
	this->get_main_menu_data()->get_level_settings()->set_stage(
		this->select_state_.stage);
}
void MainMenuStageSelect::update_stage_select_visuals()
{
	this->unconfirm_all_widgets();

	switch (this->select_state_.stage)
	{
	case level_stage::KING_OF_THE_HILL:
		this->stage_icon_->set_sprite_frame("stage_king_of_the_hill");
		this->stage_name_->set_text("King of the Hill");
		break;
	case level_stage::TURBULENCE:
		this->stage_icon_->set_sprite_frame("stage_turbulence");
		this->stage_name_->set_text("Turbulence");
		break;
	case level_stage::CLOSE_QUARTERS:
		this->stage_icon_->set_sprite_frame("stage_close_quarters");
		this->stage_name_->set_text("Close Quarters");
		break;
	case level_stage::RANDOM:
		this->stage_icon_->set_sprite_frame("random");
		this->stage_name_->set_text("Random");
		break;
	default:
		this->stage_icon_->set_sprite_frame("error");
		this->stage_name_->set_text("ERROR");
		break;
	}
	if (this->select_state_.state == confirmation_state::CONFIRMED)
	{
		this->ready_->set_hidden(false);
	}
}
void MainMenuStageSelect::unconfirm_all_widgets()
{
	this->stage_icon_->set_colour(
		main_menu_consts::STAGE_SELECT_UNSELECTED_COLOUR);
	this->stage_name_->set_colour(
		main_menu_consts::STAGE_SELECT_UNSELECTED_COLOUR);
	this->ready_->set_hidden(true);
}
level_stage MainMenuStageSelect::get_random_stage()
{
	int random = static_cast<int>(rand() %
		static_cast<int>(level_stage::MAX_STAGE));
	return static_cast<level_stage>(random);
}
void MainMenuStageSelect::init()
{
	this->select_state_.stage = this->get_main_menu_data()->
		get_level_settings()->get_stage();

	this->set_widget_spacing(STAGE_SELECT_WIDGET_SPACING);

	this->background_ = std::make_unique<MTexture>(
		"background",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, DEFAULT_RESOLUTION),
		this->get_render_resources(),
		PLAY_BACKGROUND_COLOUR);

	this->heading_ = std::make_unique<MTextDropShadow>(
		"heading",
		"Stage Select",
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->get_render_resources(),
		HEADING_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);


	this->stage_icon_ = std::make_unique<MTexture>(
		"stage_icon",
		"sprite_sheet_1",
		"stage_test_1",
		RectangleF(this->calculate_widget_position(0, 1), STAGE_SELECT_ICON_SIZE),
		this->get_render_resources(),
		PLAY_BACKGROUND_COLOUR);

	this->stage_name_ = std::make_unique<MTextDropShadow>(
		"stage_name",
		"Test 1",
		ITEM_FONT,
		this->calculate_widget_position(0, 5),
		this->get_render_resources(),
		STAGE_SELECT_UNSELECTED_COLOUR,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->ready_ = std::make_unique<MTextDropShadow>(
		"ready",
		"READY?",
		ANNOUNCEMENT_FONT,
		this->background_->get_rectangle().get_center() - Vector2F(400.0f, 100.0f),
		this->get_render_resources(),
		STAGE_SELECT_SELECTED_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET,
		true);

	const Vector2F resolution = this->get_float_resolution();

	this->texture_container_ = std::make_unique<MContainer>("texture_container");
	this->texture_container_->add_child(this->background_.get());
	this->texture_container_->add_child(this->stage_icon_.get());

	this->text_container_ = std::make_unique<MContainer>("text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->stage_name_.get());
	this->text_container_->add_child(this->ready_.get());

	this->texture_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->text_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);

	this->play_effect(this->music_, true, MUSIC_VOLUME);
}
void MainMenuStageSelect::cycle_stages(menu_direction direction)
{
	int enum_max = static_cast<int>(level_stage::MAX_STAGE);
	int enum_pos = static_cast<int>(this->select_state_.stage);
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
	this->select_state_.stage = static_cast<level_stage>(enum_pos);
}

#pragma endregion MainMenuStageSelect