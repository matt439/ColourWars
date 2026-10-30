#include "game/pch.h"
#include "game/states/main_menu.h"
#include "game/objects/crude_drawing.h"
#include "game/carried/ui_text_drop_shadow.h"
#include "game/states/player_label.h"

using namespace mattmath;
using namespace menu_consts;
using namespace labrador;

#pragma region MainMenuTeamSelect

MainMenuTeamSelect::MainMenuTeamSelect(MenuContext* context) :
	RosterSetupPage(context), team_focus_(context->level_settings()->player_count())
{

}

void MainMenuTeamSelect::update_page(float /*dt*/)
{
	std::vector<ProcessedMenuInput> inputs =
		this->input()->player_inputs(*this->level_settings());
	if (this->wait_for_players(inputs))
	{
		return;
	}
	this->route_host_setup_input(inputs, this->host_row_);
	if (this->host_cursor_)
	{
		this->place_host_cursor(*this->host_cursor_, *this->player_widgets_[this->host_row_]->player);
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
		const PlayerTeam team = this->selected_team(i);
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
			if (this->confirmations_[i] == ConfirmationState::confirmed)
			{
				this->play_wave(this->unready_sound_, this->look().unready_sound.volume);
				this->confirmations_[i] =
					ConfirmationState::unconfirmed;
				this->player_widgets_[i]->motion.press();
			}
			// Continue so other players can act in the same frame.
		}
		else if (inputs[i].action == MenuInputAction::proceed &&
			team != PlayerTeam::none)
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
				if (this->confirmations_[i] != ConfirmationState::confirmed)
				{
					this->player_widgets_[i]->motion.press();
				}
				this->confirmations_[i] = ConfirmationState::confirmed;
			}
		}
		else if (this->confirmations_[i] ==
			ConfirmationState::unconfirmed)
		{
			if (inputs[i].direction == labrador::Direction::left)
			{
				this->play_wave(this->direction_sound_);
				if (team == PlayerTeam::none)
				{
					this->team_focus_.set_focused(i, this->team_widget(i, PlayerTeam::a));
				}
				else if (team == PlayerTeam::b)
				{
					this->team_focus_.set_focused(i, this->team_widget(i, PlayerTeam::none));
				}
			}
			else if (inputs[i].direction == labrador::Direction::right)
			{
				this->play_wave(this->direction_sound_);
				if (team == PlayerTeam::none)
				{
					this->team_focus_.set_focused(i, this->team_widget(i, PlayerTeam::b));
				}
				else if (team == PlayerTeam::a)
				{
					this->team_focus_.set_focused(i, this->team_widget(i, PlayerTeam::none));
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
	MatchSetup shuffled = *this->level_settings();
	if (!shuffled.shuffle_teams()) { return; }
	for (int i = 0; i < this->player_count(); ++i)
	{
		this->player_widgets_[i]->motion.reset();
		this->team_focus_.set_focused(i, this->team_widget(i, shuffled.player_team(i)));
		this->confirmations_[i] = ConfirmationState::unconfirmed;
	}
	this->both_teams_needed_->set_hidden(true);
	this->play_wave(this->direction_sound_);
	this->update_team_select_visuals();
	// A successful shuffle is visible even when a row keeps the same team.
	// Each row retains one bounded response rather than accumulating presses.
	for (const auto& widgets : this->player_widgets_) { widgets->motion.press(); }
}
bool MainMenuTeamSelect::confirm_completes_roster(int player) const
{
	for (int i = 0; i < this->player_count(); i++)
	{
		if (i != player &&
			this->confirmations_[i] != ConfirmationState::confirmed)
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
		switch (this->selected_team(i))
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
			set_player_team(i, this->selected_team(i));
	}
}
bool MainMenuTeamSelect::all_players_confirmed() const
{
	for (int i = 0; i < this->player_count(); i++)
	{
		if (this->confirmations_[i] != ConfirmationState::confirmed)
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
		if (this->confirmations_[i] == ConfirmationState::confirmed)
		{
			return false;
		}
	}
	return true;
}
void MainMenuTeamSelect::update_team_select_visuals()
{
	// The style follows the live presentation. The frames and enabled states
	// below follow only each row's team and readiness, so they are rewritten
	// when one of those changes; the empty record forces the first write.
	const SelectPresentation& look = this->look().team_select;
	this->team_focus_.set_style({ look.unselected, look.unselected, look.selected });
	const size_t player_count = static_cast<size_t>(this->player_count());
	bool changed = this->displayed_teams_.size() != player_count;
	for (size_t i = 0; !changed && i < player_count; i++)
	{
		changed = this->displayed_teams_[i] != this->selected_team(static_cast<int>(i)) ||
			this->displayed_confirmations_[i] != this->confirmations_[i];
	}
	if (!changed)
	{
		return;
	}
	this->displayed_teams_.resize(player_count);
	this->displayed_confirmations_.resize(player_count);

	this->deselect_and_unconfirm_all_widgets();
	const auto& frames = this->team_select_selected_frames_;

	for (int i = 0; i < this->player_count(); i++)
	{
		this->displayed_teams_[static_cast<size_t>(i)] = this->selected_team(i);
		this->displayed_confirmations_[static_cast<size_t>(i)] = this->confirmations_[i];
		MenuChoiceIcon* selected_widget = nullptr;
		switch (this->selected_team(i))
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
		this->player_widgets_[i]->motion.observe(selected_widget);
		selected_widget->set_motion_selected(true);
		if (this->confirmations_[i] ==
			ConfirmationState::confirmed)
		{
			// Readiness locks only this row's chosen icon; its cursor stays put.
			this->team_focus_.set_enabled(selected_widget, false);
		}
	}
}
void MainMenuTeamSelect::deselect_and_unconfirm_all_widgets()
{
	const auto& frames = this->team_select_frames_;
	for (int i = 0; i < this->player_count(); i++)
	{
		this->player_widgets_[i]->player_a->set_motion_selected(false);
		this->player_widgets_[i]->player_center->set_motion_selected(false);
		this->player_widgets_[i]->player_b->set_motion_selected(false);
		this->player_widgets_[i]->player_a->set_sprite_frame(
			frames[0]);
		this->player_widgets_[i]->player_center->set_sprite_frame(
			frames[1]);
		this->player_widgets_[i]->player_b->set_sprite_frame(
			frames[2]);

		this->team_focus_.set_enabled(this->player_widgets_[i]->player_a.get(), true);
		this->team_focus_.set_enabled(this->player_widgets_[i]->player_center.get(), true);
		this->team_focus_.set_enabled(this->player_widgets_[i]->player_b.get(), true);
	}
}

MenuChoiceIcon* MainMenuTeamSelect::team_widget(int player, PlayerTeam team) const
{
	const PlayerWidgets& widgets = *this->player_widgets_.at(static_cast<size_t>(player));
	switch (team)
	{
	case PlayerTeam::a: return widgets.player_a.get();
	case PlayerTeam::none: return widgets.player_center.get();
	case PlayerTeam::b: return widgets.player_b.get();
	}
	throw std::logic_error("Team Select has an invalid team choice.");
}

PlayerTeam MainMenuTeamSelect::selected_team(int player) const
{
	const UiWidget* focused = this->team_focus_.focused(player);
	const PlayerWidgets& widgets = *this->player_widgets_.at(static_cast<size_t>(player));
	if (focused == widgets.player_a.get()) { return PlayerTeam::a; }
	if (focused == widgets.player_center.get()) { return PlayerTeam::none; }
	if (focused == widgets.player_b.get()) { return PlayerTeam::b; }
	throw std::logic_error("Team Select cursor must remain in its player's row.");
}

void MainMenuTeamSelect::init()
{
	// A button held on the preceding page cannot shuffle or confirm this one.
	this->input()->reset_actions();
	const MainMenuPresentation& look = this->look();
	const MenuStyle& style = this->menu_style();
	this->setup_art_ = look.art;
	this->setup_art_.focus_padding = look.setup_focus_padding;
	this->setup_art_.shadow_offset = look.setup_shadow_offset;
	this->row_art_ = this->setup_art_;
	this->row_art_.paper = this->setup_art_.focus_fill;
	const Colour clear_shadow(0.0f, 0.0f, 0.0f, 0.0f);
	this->team_select_frames_ = look.team_select_frames;
	this->team_select_selected_frames_ = look.team_select_selected_frames;
	this->set_player_prompts(this->level_settings()->bot_count() > 0 ? MenuPrompt::bot_team :
		this->player_count() >= 2 ? MenuPrompt::team_shuffle : MenuPrompt::team);

	this->set_widget_spacing(this->look().team_select.spacing);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	
	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");

	this->background_ = std::make_unique<UiTexture>(
		"background",
		style.sprite_sheet,
		style.fill_frame,
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		CrudeDrawing::premultiplied(look.home_background_colour));
	this->paper_ = std::make_unique<MenuPaperArtwork>(
		RectangleF((Vector2F(DESIGN_RESOLUTION.x, style.footer_y) - look.team_box_size) / 2.0f,
			look.team_box_size),
		this->render_resources(), &style, &this->setup_art_, &this->focus_);

	this->texture_container_->add_child(this->background_.get());
	this->texture_container_->add_child(this->paper_.get());

	this->heading_ = std::make_unique<colourwars::UiTextDropShadow>(
		"heading",
		this->text().heading(MenuHeading::teams),
		this->menu_style().heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		this->setup_art_.ink,
		clear_shadow,
		this->menu_style().heading_shadow_offset);

	this->text_container_->add_child(this->heading_.get());

	// Keep the warning below the four player cards and inside the paper.
	this->both_teams_needed_ = std::make_unique<colourwars::UiTextDropShadow>(
		"both_teams_needed",
		this->text().connection.teams_required,
		this->menu_style().detail_font,
		look.team_warning_position,
		this->render_resources(),
		this->setup_art_.ink,
		clear_shadow,
		this->menu_style().detail_shadow_offset,
		true);

	this->text_container_->add_child(this->both_teams_needed_.get());


	for (int i = 0; i < this->player_count(); i++)
	{
		this->confirmations_.push_back(ConfirmationState::unconfirmed);

		auto widgets = std::make_unique<PlayerWidgets>(this->menu_style().motion);

		std::string name = "player_" + std::to_string(i + 1);
		std::wstring label_text = player_label(*this->level_settings(), i, this->text());

		widgets->player = std::make_unique<colourwars::UiTextDropShadow>(
			name,
			label_text,
			this->menu_style().detail_font,
			this->calculate_widget_position(0, i + 2),
			this->render_resources(),
			this->setup_art_.ink,
			clear_shadow,
			this->menu_style().item_shadow_offset);
		widgets->badge = std::make_unique<UiPlayerBadge>(name + "_badge",
			UiPlayerBadge::before(widgets->player->bounds(), this->presentation().player_identity()),
			i, &this->presentation().player_identity(), this->render_resources());

		widgets->player_a = std::make_unique<MenuChoiceIcon>(
			name + "_a",
			look.team_select_sheet,
			this->team_select_frames_[0],
			RectangleF(this->calculate_widget_position(2, i + 2), this->look().team_select.size),
			this->render_resources(), &widgets->motion, &this->save()->play_settings());

		auto move = Vector2F(this->look().team_select.size.x, 0.0f);

		widgets->player_center = std::make_unique<MenuChoiceIcon>(
			name + "_center",
			look.team_select_sheet,
			this->team_select_frames_[1],
			RectangleF(widgets->player_a->rectangle().position() + move,
				this->look().team_select.size),
			this->render_resources(), &widgets->motion, &this->save()->play_settings());

		widgets->player_b = std::make_unique<MenuChoiceIcon>(
			name + "_b",
			look.team_select_sheet,
			this->team_select_frames_[2],
			RectangleF(widgets->player_center->rectangle().position() + move,
				this->look().team_select.size),
			this->render_resources(), &widgets->motion, &this->save()->play_settings());
		RectangleF row = RectangleF::union_of(widgets->player->bounds(), widgets->badge->bounds());
		row = RectangleF::union_of(row, widgets->player_a->bounds());
		row = RectangleF::union_of(row, widgets->player_center->bounds());
		row = RectangleF::union_of(row, widgets->player_b->bounds());
		row.inflate(this->setup_art_.focus_padding);
		widgets->paper = std::make_unique<MenuPaperArtwork>(row,
			this->render_resources(), &style, &this->row_art_, &this->focus_);

		this->texture_container_->add_child(widgets->paper.get());
		this->texture_container_->add_child(widgets->player_a.get());
		this->texture_container_->add_child(widgets->player_center.get());
		this->texture_container_->add_child(widgets->player_b.get());
		this->texture_container_->add_child(widgets->badge.get());

		this->text_container_->add_child(widgets->player.get());

		this->register_selection_motion(&widgets->motion);
		this->player_widgets_.push_back(std::move(widgets));
		const PlayerWidgets& row_widgets = *this->player_widgets_.back();
		this->team_focus_.add(row_widgets.player_a.get());
		this->team_focus_.add(row_widgets.player_center.get());
		this->team_focus_.add(row_widgets.player_b.get());
		this->team_focus_.set_focused(i, this->team_widget(i, this->level_settings()->player_team(i)));
	}
	if (this->level_settings()->bot_count() > 0)
	{
		this->host_cursor_ = this->make_host_cursor();
		this->place_host_cursor(*this->host_cursor_, *this->player_widgets_[this->host_row_]->player);
		this->texture_container_->add_child(this->host_cursor_.get());
	}
	this->update_team_select_visuals();
	this->check_paper_geometry();
}

void MainMenuTeamSelect::check_paper_geometry() const
{
	const RectangleF panel = this->paper_->rectangle();
	const float edge = this->setup_art_.keyline_width * 3.0f;
	const RectangleF inside(panel.position() + Vector2F(edge),
		panel.size() - Vector2F(edge * 2.0f + this->setup_art_.shadow_offset));
	const auto check = [&](const RectangleF& bounds, const char* role)
	{
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
			!std::isfinite(bounds.width) || !std::isfinite(bounds.height) ||
			bounds.width <= 0.0f || bounds.height <= 0.0f || !inside.contains(bounds))
		{
			throw std::runtime_error(std::string("main_menu.team_box_size: Team Select ") + role +
				" must fit the paper including its retained text, badge and card bounds.");
		}
	};
	check(this->heading_->bounds(), "heading");
	float previous_bottom = this->heading_->bounds().bottom();
	for (const auto& widgets : this->player_widgets_)
	{
		const RectangleF card = widgets->paper->rectangle();
		check(card, "player row");
		if (card.y < previous_bottom)
		{
			throw std::runtime_error("main_menu.team_select.spacing: Team Select cards must follow the heading without overlap.");
		}
		if (widgets->player->bounds().right() > widgets->player_a->bounds().x)
		{
			throw std::runtime_error("main_menu.team_select.spacing: Team Select player labels must end before the choice icons.");
		}
		previous_bottom = card.bottom();
	}
	check(this->both_teams_needed_->bounds(), "warning");
	if (this->both_teams_needed_->bounds().y < previous_bottom)
	{
		throw std::runtime_error("main_menu.team_select.warning_position: Team Select warning must follow the complete player cards.");
	}
}

#pragma endregion MainMenuTeamSelect
