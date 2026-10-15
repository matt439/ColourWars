#include "game/pch.h"
#include "game/states/main_menu.h"

using namespace DirectX;
using namespace mattmath;
using namespace menu_consts;
using namespace labrador;

#pragma region MainMenuWeaponSelect

MainMenuWeaponSelect::MainMenuWeaponSelect(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuWeaponSelect::update_page(float /*dt*/)
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
				this->play_wave(this->unready_sound_, this->look().unready_sound.volume);
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
		const WeaponChoicePresentation& choice =
			this->weapon_select_art_.choice(this->select_states_[i].type);
		this->player_widgets_[i]->weapon_icon->set_sprite_frame(
			choice.frame, choice.paint_frame, choice.supply_strip);
		this->player_widgets_[i]->weapon_name->set_text(choice.name);

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
	this->weapon_select_art_ = this->look().weapon_select_art;
	this->weapon_descriptions_ = this->look().weapon_descriptions;

	this->set_player_prompts(MenuPrompt::weapon);

	this->set_widget_spacing(this->look().weapon_select.spacing);
	// A random palette is not chosen until the level is built. Keep that
	// preview neutral instead of consuming a different random draw here.
	const TeamColourPair palette = this->save()->team_palette();
	const TeamColour preview_colours = palette == TeamColourPair::random
		? TeamColour(Colour::white, Colour::white)
		: TeamColourTools::team_colours(palette, this->presentation().team_palette());

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
		this->text().heading(MenuHeading::weapons),
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
		const WeaponChoicePresentation& choice =
			this->weapon_select_art_.choice(state.type);
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
		widgets->badge = std::make_unique<UiPlayerBadge>(name + "_badge",
			UiPlayerBadge::before(widgets->player->bounds(), this->presentation().player_identity()),
			i, &this->presentation().player_identity(), this->render_resources());

		widgets->weapon_icon = std::make_unique<MenuIcon>(
			name + "_wep_icon",
			choice.frame,
			RectangleF(this->calculate_widget_position(2, i + 2), this->look().weapon_select.size),
			this->render_resources(),
			this->look().weapon_select.unselected, &this->menu_style(),
			this->weapon_select_art_.sheet, this->weapon_select_art_.fill_frame);
		widgets->weapon_icon->set_sprite_frame(
			choice.frame, choice.paint_frame, choice.supply_strip);
		widgets->weapon_icon->set_paint_colour(preview_colours.team_colour(
			this->level_settings()->player_team(i)));

		widgets->weapon_name = std::make_unique<UiTextDropShadow>(
			name + "_wep_name",
			choice.name,
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
			this->weapon_description(state.type),
			this->look().weapon_description_font,
			widgets->weapon_icon->rectangle().position() +
				Vector2F(this->look().weapon_description_x_offset, 0.0f),
			this->render_resources(),
			this->look().weapon_description_colour,
			this->menu_style().shadow_colour,
			this->look().weapon_description_shadow_offset);

		this->texture_container_->add_child(widgets->weapon_icon.get());
		this->texture_container_->add_child(widgets->badge.get());
		this->text_container_->add_child(widgets->weapon_name.get());
		this->text_container_->add_child(widgets->player.get());
		this->text_container_->add_child(widgets->weapon_description.get());

		this->player_widgets_.push_back(std::move(widgets));
	}
	this->update_weapon_select_visuals();
}
std::wstring MainMenuWeaponSelect::weapon_description(WeaponType type) const
{
	const size_t index = static_cast<size_t>(type);
	return this->weapon_descriptions_.at(index);
}

#pragma endregion MainMenuWeaponSelect
