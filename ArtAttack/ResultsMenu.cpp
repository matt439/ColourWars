#include "pch.h"
#include "ResultsMenu.h"

using namespace DirectX;
using namespace MattMath;
using namespace results_menu_consts;
using namespace colour_consts;

ResultsMenuData* ResultsMenuPage::get_results_menu_data() const
{
	return this->_data;
}

void ResultsMenuInitial::update()
{
	const float dt = *this->get_data()->get_dt();
	std::vector<ProcessedMenuInput> menu_inputs = this->get_menu_inputs();
	

	if (this->_delay_timer < RESULTS_MENU_TEAM_FILL_DELAY)
	{
		this->_delay_timer += dt;
	}
	else
	{		
		this->update_fill_box();
		if (this->_fill_timer >= RESULTS_MENU_TEAM_FILL_TIME)
		{
			// only play winner sound once
			if (!this->_winner_sound_played)
			{
				this->_winner_sound_played = true;
				this->stop_effect(FILL_SOUND, true);
				this->play_wave(WINNER_SOUND, WINNER_VOLUME);
			}
			this->_team_a_percentage->set_hidden(false);
			this->_team_b_percentage->set_hidden(false);
			this->_winner->set_hidden(false);
			
			if (this->_show_results_timer >= RESULTS_MENU_SHOW_RESULTS_DELAY)
			{
				this->_proceed->set_hidden(false);
				int continue_input = this->check_for_continue_input(menu_inputs);

				if (continue_input != -1)
				{
					this->play_wave(CONFIRM_SOUND);
					*this->get_results_menu_data()->get_action() =
						results_menu_action::CONTINUE_TO_END_MENU;
				}
			}
			this->_show_results_timer += dt;
		}
		this->_fill_timer += dt;
	}
}

void ResultsMenuInitial::init()
{
	const LevelEndInfo end_info = this->get_level_end_info();

	this->_box = std::make_unique<MTexture>(
		"box",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, RESULTS_MENU_BOX_SIZE),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		RESULTS_MENU_BOX_COLOUR);
	this->_box->set_position_at_center(DEFAULT_RESOLUTION / 2.0f);

	this->_heading = std::make_unique<MTextDropShadow>(
		"title",
		"Results",
		HEADING_FONT,
		HEADING_POSITION,
		this->get_sprite_batch(),
		this->get_resource_manager(),
		RESULTS_MENU_HEADING_TEXT_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->_fill_box = std::make_unique<MTexture>(
		"fill_box",
		"sprite_sheet_1",
		"results_menu_fill_box",
		RectangleF(Vector2F::ZERO, RESULTS_MENU_FILL_BOX_SIZE),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		RESULTS_MENU_BOX_COLOUR);
	auto fill_box_position = Vector2F(
		this->calculate_center_position(
		this->_fill_box->get_rectangle().get_width(), DEFAULT_RESOLUTION.x),
		this->_box->get_rectangle().get_position().y + RESULTS_MENU_FILL_BOX_Y_OFFSET);
	this->_fill_box->set_position(fill_box_position);

	this->_team_a_fill = std::make_unique<MTexture>(
		"team_a_fill",
		"sprite_sheet_1",
		"results_menu_team_a_fill",
		RectangleF(Vector2F::ZERO,
			Vector2F(0.0f, RESULTS_MENU_TEAM_FILL_SIZE.y)),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_results_menu_data()->get_level_end_info().team_colours.team_a);
	this->_team_a_fill->set_position(this->_fill_box->get_rectangle().get_position() +
		RESULTS_MENU_TEAM_FILL_BORDER);

	this->_team_b_fill = std::make_unique<MTexture>(
		"team_b_fill",
		"sprite_sheet_1",
		"results_menu_team_b_fill",
		RectangleF(Vector2F::ZERO,
			Vector2F(0.0f, RESULTS_MENU_TEAM_FILL_SIZE.y)),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		this->get_results_menu_data()->get_level_end_info().team_colours.team_b);

	this->_team_a_percentage = std::make_unique<MTextDropShadow>(
		"team_a_percentage",
		end_info.team_a_percentage_string(),
		DETAIL_FONT,
		this->_team_a_fill->get_rectangle().get_position(),
		this->get_sprite_batch(),
		this->get_resource_manager(),
		PERCENTAGE_TEXT_COLOUR,
		SHADOW_COLOUR,
		DETAIL_SHADOW_OFFSET,
		true);

	this->_team_b_percentage = std::make_unique<MTextDropShadow>(
		"team_b_percentage",
		end_info.team_b_percentage_string(),
		DETAIL_FONT,
		Vector2F::ZERO,
		this->get_sprite_batch(),
		this->get_resource_manager(),
		PERCENTAGE_TEXT_COLOUR,
		SHADOW_COLOUR,
		DETAIL_SHADOW_OFFSET,
		true);
	this->_team_b_percentage->set_position(Vector2F(
		this->calculate_center_position(this->get_widget_size().x, DEFAULT_RESOLUTION.x) +
		TEAM_B_PERCENTAGE_X_OFFSET,
		this->calculate_team_b_fill_top_right_position().y));

	this->_winner = std::make_unique<MTextDropShadow>(
		"winner",
		end_info.winning_team_string(),
		ITEM_FONT,
		WINNER_POSITION,
		this->get_sprite_batch(),
		this->get_resource_manager(),
		RESULTS_MENU_HEADING_TEXT_COLOUR,
		SHADOW_COLOUR,
		DETAIL_SHADOW_OFFSET,
		true);

	this->_proceed = std::make_unique<MTextDropShadow>(
		"proceed",
		"Press A to proceed",
		DETAIL_FONT,
		PROCEED_POSITION,
		this->get_sprite_batch(),
		this->get_resource_manager(),
		RESULTS_MENU_HEADING_TEXT_COLOUR,
		SHADOW_COLOUR,
		DETAIL_SHADOW_OFFSET,
		true);

	this->_texture_container = std::make_unique<MContainer>(
		"texture_container");
	this->_texture_container->add_child(this->_box.get());
	this->_texture_container->add_child(this->_fill_box.get());
	this->_texture_container->add_child(this->_team_a_fill.get());
	this->_texture_container->add_child(this->_team_b_fill.get());

	this->_text_container = std::make_unique<MContainer>(
		"text_container");
	this->_text_container->add_child(this->_heading.get());
	this->_text_container->add_child(this->_team_a_percentage.get());
	this->_text_container->add_child(this->_team_b_percentage.get());
	this->_text_container->add_child(this->_winner.get());
	this->_text_container->add_child(this->_proceed.get());

	const Vector2F resolution = this->get_float_resolution();

	this->_texture_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->_text_container->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);

	this->play_effect(FILL_SOUND, true, FILL_VOLUME);
}

void ResultsMenuInitial::draw()
{
	this->draw_mobject_in_viewports(this->_texture_container.get(),
		this->get_point_clamp_sampler_state());

	// draw text separately to use blend state
	this->draw_mobject_in_viewports(this->_text_container.get());
}

void ResultsMenuInitial::update_fill_box() const
{
	this->update_team_a_fill();
	this->update_team_b_fill();
}

float ResultsMenuInitial::fill_time_ratio() const
{
	return this->_fill_timer / RESULTS_MENU_TEAM_FILL_TIME;
}

void ResultsMenuInitial::update_team_a_fill() const
{
	//float score = this->get_team_percentage(player_team::A);
	float score = this->get_level_end_info().team_a_ratio();
	float max_width = RESULTS_MENU_TEAM_FILL_SIZE.x * score;
	float fill = this->fill_time_ratio();
	if (fill > 1.0f)
	{
		fill = 1.0f;
	}
	float width = fill * max_width;
	this->_team_a_fill->set_width(width);
}

void ResultsMenuInitial::update_team_b_fill() const
{
	float score = this->get_level_end_info().team_b_ratio();
	float max_width = RESULTS_MENU_TEAM_FILL_SIZE.x * score;
	float fill = this->fill_time_ratio();
	if (fill > 1.0f)
	{
		fill = 1.0f;
	}
	float width = fill * max_width;
	this->_team_b_fill->set_width(width);
	this->_team_b_fill->set_position_from_top_right_origin(
		this->calculate_team_b_fill_top_right_position());
}

Vector2F ResultsMenuInitial::calculate_team_b_fill_top_right_position() const
{
	Vector2F team_b_top_right_position = this->_fill_box->get_rectangle().get_position() +
		Vector2F(this->_fill_box->get_rectangle().get_width(), 0.0f) +
			Vector2F(-RESULTS_MENU_TEAM_FILL_BORDER.x,
				RESULTS_MENU_TEAM_FILL_BORDER.y);
	return team_b_top_right_position;
}

int ResultsMenuInitial::check_for_continue_input(
	const std::vector<ProcessedMenuInput>& menu_input)
{
	for (int i = 0; i < menu_input.size(); i++)
	{
		if (menu_input[i].action == menu_input_action::PROCEED)
		{
			return i;
		}
	}
	return -1;
}
LevelEndInfo ResultsMenuInitial::get_level_end_info() const
{
	return this->get_results_menu_data()->get_level_end_info();
}
