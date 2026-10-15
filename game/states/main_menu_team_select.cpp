#include "game/pch.h"
#include "game/states/main_menu.h"

using namespace DirectX;
using namespace mattmath;
using namespace menu_consts;
using namespace labrador;

#pragma region MainMenuTeamSelect

MainMenuTeamSelect::MainMenuTeamSelect(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuTeamSelect::update_page(float /*dt*/)
{
	const std::vector<ProcessedMenuInput> inputs =
		this->input()->player_inputs(*this->level_settings());
	if (this->wait_for_players(inputs))
	{
		return;
	}
	if (this->player_count() >= 2 && std::any_of(inputs.begin(), inputs.end(),
		[](const ProcessedMenuInput& input) { return input.action == MenuInputAction::shuffle; }))
	{
		this->shuffle_teams();
		// The resulting teams must be seen before anyone confirms them. Discard
		// other actions in this frame, and presses held across this shuffle.
		this->input()->reset_actions();
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
				this->play_wave(this->unready_sound_, this->look().unready_sound.volume);
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
void MainMenuTeamSelect::shuffle_teams()
{
	// Like manual choices, a shuffle is provisional until everyone confirms.
	// Only the final team commit writes the shared match settings.
	MenuLevelSettings shuffled = *this->level_settings();
	if (!shuffled.shuffle_teams()) { return; }
	for (int i = 0; i < this->player_count(); ++i)
	{
		this->select_states_[i].team = shuffled.player_team(i);
		this->select_states_[i].state = ConfirmationState::unconfirmed;
	}
	this->both_teams_needed_->set_hidden(true);
	this->play_wave(this->direction_sound_);
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
	const auto& frames = this->team_select_selected_frames_;

	for (int i = 0; i < this->player_count(); i++)
	{
		UiWidget* selected_widget = nullptr;
		switch (this->select_states_[i].team)
		{
		case PlayerTeam::a:
			this->player_widgets_[i]->player_a->set_sprite_frame(
				frames[0]);
			selected_widget = this->player_widgets_[i]->player_a.get();
			break;
		case PlayerTeam::none:
			this->player_widgets_[i]->player_center->set_sprite_frame(
				frames[1]);
			selected_widget = this->player_widgets_[i]->player_center.get();
			break;
		case PlayerTeam::b:
			this->player_widgets_[i]->player_b->set_sprite_frame(
				frames[2]);
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
	const auto& frames = this->team_select_frames_;
	for (int i = 0; i < this->player_count(); i++)
	{
		this->player_widgets_[i]->player_a->set_sprite_frame(
			frames[0]);
		this->player_widgets_[i]->player_center->set_sprite_frame(
			frames[1]);
		this->player_widgets_[i]->player_b->set_sprite_frame(
			frames[2]);

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
	// A button held on the preceding page cannot shuffle or confirm this one.
	this->input()->reset_actions();
	const MainMenuPresentation& look = this->look();
	this->team_select_frames_ = look.team_select_frames;
	this->team_select_selected_frames_ = look.team_select_selected_frames;
	std::vector<PromptBar::Prompt> prompts{
		{ L"Left/Right", L"Pick a team" },
		{ L"A", L"Confirm" },
		{ L"B", L"Cancel" } };
	if (this->player_count() >= 2) { prompts.push_back({ L"X", L"Random teams" }); }
	this->set_player_prompts(prompts);

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
		widgets->badge = std::make_unique<UiPlayerBadge>(name + "_badge",
			UiPlayerBadge::before(widgets->player->bounds(), this->presentation().player_identity()),
			i, &this->presentation().player_identity(), this->render_resources());

		widgets->player_a = std::make_unique<UiTexture>(
			name + "_a",
			look.team_select_sheet,
			this->team_select_frames_[0],
			RectangleF(this->calculate_widget_position(2, i + 2), this->look().team_select.size),
			this->render_resources());

		auto move = Vector2F(this->look().team_select.size.x, 0.0f);

		widgets->player_center = std::make_unique<UiTexture>(
			name + "_center",
			look.team_select_sheet,
			this->team_select_frames_[1],
			RectangleF(widgets->player_a->rectangle().position() + move,
				this->look().team_select.size),
			this->render_resources());

		widgets->player_b = std::make_unique<UiTexture>(
			name + "_b",
			look.team_select_sheet,
			this->team_select_frames_[2],
			RectangleF(widgets->player_center->rectangle().position() + move,
				this->look().team_select.size),
			this->render_resources());

		this->texture_container_->add_child(widgets->player_a.get());
		this->texture_container_->add_child(widgets->player_center.get());
		this->texture_container_->add_child(widgets->player_b.get());
		this->texture_container_->add_child(widgets->badge.get());

		this->text_container_->add_child(widgets->player.get());

		this->player_widgets_.push_back(std::move(widgets));
	}

}

#pragma endregion MainMenuTeamSelect
