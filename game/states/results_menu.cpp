#include "game/pch.h"
#include "game/states/results_menu.h"

using namespace DirectX;
using namespace mattmath;
using namespace results_menu_consts;
using namespace colour_consts;
using namespace artattack;

ResultsMenuPage::ResultsMenuPage(ResultsMenuData* data) :
	MenuPage(data),
	SoundBankObject(results_menu_consts::SOUND_BANK,
		this->audio_resources()),
	data_(data)
{
	this->confirm_sound_ = this->resolve_wave(CONFIRM_SOUND);
	this->winner_sound_ = this->resolve_wave(WINNER_SOUND);
	this->fill_sound_ = this->resolve_effect(FILL_SOUND);
}

ResultsMenuData* ResultsMenuPage::results_menu_data() const
{
	return this->data_;
}

ResultsMenuInitial::ResultsMenuInitial(ResultsMenuData* data) :
	ResultsMenuPage(data)
{

}

void ResultsMenuInitial::update()
{
	const float dt = *this->data()->dt();
	std::vector<ProcessedMenuInput> menu_inputs = this->menu_inputs();
	

	if (this->delay_timer_ < RESULTS_MENU_TEAM_FILL_DELAY)
	{
		this->delay_timer_ += dt;
	}
	else
	{		
		this->update_fill_box();
		if (this->fill_timer_ >= RESULTS_MENU_TEAM_FILL_TIME)
		{
			// only play winner sound once
			if (!this->winner_sound_played_)
			{
				this->winner_sound_played_ = true;
				this->stop_effect(this->fill_sound_, true);
				this->play_wave(this->winner_sound_, WINNER_VOLUME);
			}
			this->team_a_percentage_->set_hidden(false);
			this->team_b_percentage_->set_hidden(false);
			this->winner_->set_hidden(false);
			
			if (this->show_results_timer_ >= RESULTS_MENU_SHOW_RESULTS_DELAY)
			{
				this->proceed_->set_hidden(false);
				int continue_input = this->check_for_continue_input(menu_inputs);

				if (continue_input != -1)
				{
					this->play_wave(this->confirm_sound_);
					*this->results_menu_data()->action() =
						ResultsMenuAction::continue_to_end_menu;
				}
			}
			this->show_results_timer_ += dt;
		}
		this->fill_timer_ += dt;
	}
}

void ResultsMenuInitial::init()
{
	const LevelEndInfo end_info = this->level_end_info();

	this->box_ = std::make_unique<UiTexture>(
		"box",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, RESULTS_MENU_BOX_SIZE),
		this->render_resources(),
		RESULTS_MENU_BOX_COLOUR);
	this->box_->set_position_at_center(DEFAULT_RESOLUTION / 2.0f);

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"title",
		L"Results",
		HEADING_FONT,
		HEADING_POSITION,
		this->render_resources(),
		RESULTS_MENU_HEADING_TEXT_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->fill_box_ = std::make_unique<UiTexture>(
		"fill_box",
		"sprite_sheet_1",
		"results_menu_fill_box",
		RectangleF(Vector2F::ZERO, RESULTS_MENU_FILL_BOX_SIZE),
		this->render_resources(),
		RESULTS_MENU_BOX_COLOUR);
	auto fill_box_position = Vector2F(
		this->calculate_center_position(
		this->fill_box_->rectangle().width, DEFAULT_RESOLUTION.x),
		this->box_->rectangle().position().y + RESULTS_MENU_FILL_BOX_Y_OFFSET);
	this->fill_box_->set_position(fill_box_position);

	this->team_a_fill_ = std::make_unique<UiTexture>(
		"team_a_fill",
		"sprite_sheet_1",
		"results_menu_team_a_fill",
		RectangleF(Vector2F::ZERO,
			Vector2F(0.0f, RESULTS_MENU_TEAM_FILL_SIZE.y)),
		this->render_resources(),
		this->results_menu_data()->level_end_info().team_colours.team_a);
	this->team_a_fill_->set_position(this->fill_box_->rectangle().position() +
		RESULTS_MENU_TEAM_FILL_BORDER);

	this->team_b_fill_ = std::make_unique<UiTexture>(
		"team_b_fill",
		"sprite_sheet_1",
		"results_menu_team_b_fill",
		RectangleF(Vector2F::ZERO,
			Vector2F(0.0f, RESULTS_MENU_TEAM_FILL_SIZE.y)),
		this->render_resources(),
		this->results_menu_data()->level_end_info().team_colours.team_b);

	this->team_a_percentage_ = std::make_unique<UiTextDropShadow>(
		"team_a_percentage",
		end_info.team_a_percentage_string(),
		DETAIL_FONT,
		this->team_a_fill_->rectangle().position(),
		this->render_resources(),
		PERCENTAGE_TEXT_COLOUR,
		SHADOW_COLOUR,
		DETAIL_SHADOW_OFFSET,
		true);

	this->team_b_percentage_ = std::make_unique<UiTextDropShadow>(
		"team_b_percentage",
		end_info.team_b_percentage_string(),
		DETAIL_FONT,
		Vector2F::ZERO,
		this->render_resources(),
		PERCENTAGE_TEXT_COLOUR,
		SHADOW_COLOUR,
		DETAIL_SHADOW_OFFSET,
		true);
	this->team_b_percentage_->set_position(Vector2F(
		this->calculate_center_position(this->widget_size().x, DEFAULT_RESOLUTION.x) +
		TEAM_B_PERCENTAGE_X_OFFSET,
		this->calculate_team_b_fill_top_right_position().y));

	this->winner_ = std::make_unique<UiTextDropShadow>(
		"winner",
		end_info.winning_team_string(),
		ITEM_FONT,
		WINNER_POSITION,
		this->render_resources(),
		RESULTS_MENU_HEADING_TEXT_COLOUR,
		SHADOW_COLOUR,
		DETAIL_SHADOW_OFFSET,
		true);

	this->proceed_ = std::make_unique<UiTextDropShadow>(
		"proceed",
		L"Press A to proceed",
		DETAIL_FONT,
		PROCEED_POSITION,
		this->render_resources(),
		RESULTS_MENU_HEADING_TEXT_COLOUR,
		SHADOW_COLOUR,
		DETAIL_SHADOW_OFFSET,
		true);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->box_.get());
	this->texture_container_->add_child(this->fill_box_.get());
	this->texture_container_->add_child(this->team_a_fill_.get());
	this->texture_container_->add_child(this->team_b_fill_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->team_a_percentage_.get());
	this->text_container_->add_child(this->team_b_percentage_.get());
	this->text_container_->add_child(this->winner_.get());
	this->text_container_->add_child(this->proceed_.get());

	const Vector2F resolution = this->float_resolution();

	this->texture_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->text_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);

	this->play_effect(this->fill_sound_, true, FILL_VOLUME);
}

void ResultsMenuInitial::draw()
{
	std::vector<std::pair<UiObject*, ID3D11SamplerState*>> ui_objects;

	ui_objects.push_back(std::make_pair(this->texture_container_.get(),
		this->point_clamp_sampler_state()));

	ui_objects.push_back(std::make_pair(this->text_container_.get(), nullptr));

	this->draw_ui_objects_in_viewports(&ui_objects);
}

void ResultsMenuInitial::update_fill_box() const
{
	this->update_team_a_fill();
	this->update_team_b_fill();
}

float ResultsMenuInitial::fill_time_ratio() const
{
	return this->fill_timer_ / RESULTS_MENU_TEAM_FILL_TIME;
}

void ResultsMenuInitial::update_team_a_fill() const
{
	//float score = this->team_percentage(PlayerTeam::a);
	float score = this->level_end_info().team_a_ratio();
	float max_width = RESULTS_MENU_TEAM_FILL_SIZE.x * score;
	float fill = this->fill_time_ratio();
	if (fill > 1.0f)
	{
		fill = 1.0f;
	}
	float width = fill * max_width;
	this->team_a_fill_->set_width(width);
}

void ResultsMenuInitial::update_team_b_fill() const
{
	float score = this->level_end_info().team_b_ratio();
	float max_width = RESULTS_MENU_TEAM_FILL_SIZE.x * score;
	float fill = this->fill_time_ratio();
	if (fill > 1.0f)
	{
		fill = 1.0f;
	}
	float width = fill * max_width;
	this->team_b_fill_->set_width(width);
	this->team_b_fill_->set_position_from_top_right_origin(
		this->calculate_team_b_fill_top_right_position());
}

Vector2F ResultsMenuInitial::calculate_team_b_fill_top_right_position() const
{
	Vector2F team_b_top_right_position = this->fill_box_->rectangle().position() +
		Vector2F(this->fill_box_->rectangle().width, 0.0f) +
			Vector2F(-RESULTS_MENU_TEAM_FILL_BORDER.x,
				RESULTS_MENU_TEAM_FILL_BORDER.y);
	return team_b_top_right_position;
}

int ResultsMenuInitial::check_for_continue_input(
	const std::vector<ProcessedMenuInput>& menu_input)
{
	for (int i = 0; i < menu_input.size(); i++)
	{
		if (menu_input[i].action == MenuInputAction::proceed)
		{
			return i;
		}
	}
	return -1;
}
LevelEndInfo ResultsMenuInitial::level_end_info() const
{
	return this->results_menu_data()->level_end_info();
}
