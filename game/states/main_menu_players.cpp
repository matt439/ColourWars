#include "game/pch.h"
#include "game/states/main_menu.h"

using namespace DirectX;
using namespace mattmath;
using namespace menu_consts;
using namespace labrador;

#pragma region MainMenuPlayerCount

MainMenuPlayerCount::MainMenuPlayerCount(MenuContext* context) :
	MainMenuPage(context)
{

}
void MainMenuPlayerCount::update_page(float /*dt*/)
{
	if (!this->context()->active())
	{
		return;
	}
	const std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	for (int slot = 0; slot < static_cast<int>(inputs.size()); ++slot)
	{
		const ProcessedMenuInput& input = inputs[slot];
		if (input.action == MenuInputAction::back)
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuModeSelect>(this->menu_context()));
			return;
		}
		if (input.action == MenuInputAction::proceed)
		{
			this->activating_slot_ = slot;
			if (this->focus_.activate(0) == Activation::ran)
			{
				return;
			}
		}
		else if (this->focus_.move(0, input.direction))
		{
			this->play_wave(this->direction_sound_);
		}
	}
}
void MainMenuPlayerCount::init()
{
	this->set_prompts({
		{ L"A", L"Select" },
		{ L"B", L"Back" } });

	// Devices can be connected and claimed on the next page.
	this->focus_.set_style({ this->menu_style().highlight,
		this->menu_style().unhighlight,
		this->menu_style().disabled });

	this->set_widget_spacing(this->look().player_count_spacing);

	this->background_ = std::make_unique<UiTexture>(
		"background",
		this->look().backdrop_sheet,
		this->look().backdrop_frame,
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		this->look().play_background_colour);

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Number of Players",
		this->menu_style().heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		this->menu_style().heading_colour,
		this->menu_style().shadow_colour,
		this->menu_style().heading_shadow_offset);

	this->_1_player = std::make_unique<UiTextDropShadow>(
		"1_player",
		L"1 Player (practice)",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 2),
		this->render_resources(),
		this->menu_style().highlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->_2_players = std::make_unique<UiTextDropShadow>(
		"2_players",
		L"2 Players",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 3),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->_3_players = std::make_unique<UiTextDropShadow>(
		"3_players",
		L"3 Players",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 4),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->_4_players = std::make_unique<UiTextDropShadow>(
		"4_players",
		L"4 Players",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 5),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->back_ = std::make_unique<UiTextDropShadow>(
		"back",
		L"Back",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 6),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->background_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->_1_player.get());
	this->text_container_->add_child(this->_2_players.get());
	this->text_container_->add_child(this->_3_players.get());
	this->text_container_->add_child(this->_4_players.get());
	this->text_container_->add_child(this->back_.get());

	this->focus_.add(this->_1_player.get(),
		[this] { this->start_join(1, ScreenLayout::one_player); });
	this->focus_.add(this->_2_players.get(),
		[this] { this->start_join(2, ScreenLayout::two_player); });
	this->focus_.add(this->_3_players.get(),
		[this] { this->start_join(3, ScreenLayout::three_player); });
	this->focus_.add(this->_4_players.get(),
		[this] { this->start_join(4, ScreenLayout::four_player); });
	// A series needs both teams; solo remains available through Single Match.
	this->focus_.set_enabled(this->_1_player.get(),
		this->level_settings()->best_of() == 1);
	if (this->level_settings()->best_of() > 1)
	{
		this->focus_.set_focused(0, this->_2_players.get());
	}
	this->focus_.add(this->back_.get(), [this]
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuModeSelect>(this->menu_context()));
		});

	this->play_music(this->music_, this->look().music.volume);
}

void MainMenuPlayerCount::start_join(int players, ScreenLayout layout)
{
	this->play_wave(this->confirm_sound_);
	MenuLevelSettings* settings = this->level_settings();
	settings->set_player_count(players);
	settings->set_screen_layout(layout);
	settings->begin_input_claims(this->activating_slot_);
	if (players == 1)
	{
		settings->set_player_team(0, PlayerTeam::a);
	}
	this->context()->transition_to(
		std::make_unique<MainMenuJoin>(this->menu_context()));
}

#pragma endregion MainMenuPlayerCount

#pragma region MainMenuJoin
MainMenuJoin::MainMenuJoin(MenuContext* context) : MainMenuPage(context)
{
}

void MainMenuJoin::init()
{
	this->set_prompts({ { L"A", L"Join / P1 continue" },
		{ L"B", L"Leave / P1 back" } });
	this->set_widget_spacing(this->look().player_count_spacing);
	const MenuStyle& style = this->menu_style();
	this->background_ = std::make_unique<UiTexture>("background",
		this->look().backdrop_sheet,
		this->look().backdrop_frame, RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(), this->look().play_background_colour);
	this->texture_container_ = std::make_unique<UiContainer>("texture_container");
	this->texture_container_->add_child(this->background_.get());
	this->text_container_ = std::make_unique<UiContainer>("text_container");
	this->heading_ = std::make_unique<UiTextDropShadow>("heading", L"Join Players",
		style.heading_font, this->calculate_widget_position(0, 0),
		this->render_resources(), style.heading_colour, style.shadow_colour,
		style.heading_shadow_offset);
	this->text_container_->add_child(this->heading_.get());
	for (int player = 0; player < this->player_count(); ++player)
	{
		this->players_.push_back(std::make_unique<UiTextDropShadow>(
			"player_" + std::to_string(player), L"", style.item_font,
			this->calculate_widget_position(0, player + 2), this->render_resources(),
			style.unhighlight, style.shadow_colour, style.item_shadow_offset));
		this->text_container_->add_child(this->players_.back().get());
	}
	this->status_ = std::make_unique<UiTextDropShadow>("status", L"",
		style.detail_font, this->calculate_widget_position(0, 7),
		this->render_resources(), style.warning, style.shadow_colour,
		style.detail_shadow_offset);
	this->text_container_->add_child(this->status_.get());
	this->refresh_players(this->input()->player_inputs(*this->level_settings()));
	this->play_music(this->music_, this->look().music.volume);
}

void MainMenuJoin::refresh_players(const std::vector<ProcessedMenuInput>& inputs)
{
	bool ready = this->level_settings()->input_roster_complete();
	const std::vector<MenuPlayerSettings> players = this->level_settings()->player_settings();
	for (int player = 0; player < this->player_count(); ++player)
	{
		const bool connected = inputs[player].connected;
		ready = ready && connected;
		std::wstring label = this->level_settings()->player_label(player);
		if (!connected && players[player].num >= 0)
		{
			label += L" - Reconnect";
		}
		this->players_[player]->set_text(label);
		this->players_[player]->set_colour(connected ?
			this->menu_style().highlight : this->menu_style().disabled);
	}
	this->status_->set_text(ready ? L"Everyone joined. Player 1 can continue." :
		L"Press A / Enter on each device to join. Reconnect reserved pads.");
}

void MainMenuJoin::update_page(float /*dt*/)
{
	if (!this->context()->active())
	{
		return;
	}
	MenuLevelSettings* settings = this->level_settings();
	const std::vector<ProcessedMenuInput> players = this->input()->player_inputs(*settings);
	const std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	// Evaluate readiness before accepting joins: the last join cannot also
	// dismiss the page in the same frame as Player 1's confirmation.
	const bool ready = settings->input_roster_complete() &&
		std::all_of(players.begin(), players.end(),
			[](const ProcessedMenuInput& input) { return input.connected; });
	bool continue_requested = false;
	bool roster_changed = false;
	for (int slot = 0; slot < static_cast<int>(inputs.size()); ++slot)
	{
		const int player = settings->player_index_for_slot(slot);
		if (inputs[slot].action == MenuInputAction::back)
		{
			if (player == 0 || !players[0].connected)
			{
				this->play_wave(this->cancel_sound_);
				this->context()->transition_to(
					std::make_unique<MainMenuPlayerCount>(this->menu_context()));
				return;
			}
			if (settings->release_input_slot(slot))
			{
				roster_changed = true;
				this->play_wave(this->unready_sound_, this->look().unready_sound.volume);
			}
		}
		else if (inputs[slot].action == MenuInputAction::proceed)
		{
			if (player == 0 && ready)
			{
				continue_requested = true;
			}
			if (settings->claim_input_slot(slot))
			{
				roster_changed = true;
				this->play_wave(this->ready_sound_);
			}
		}
	}
	if (continue_requested && !roster_changed)
	{
		this->play_wave(this->confirm_sound_);
		if (this->player_count() == 1)
		{
			this->context()->transition_to(
				std::make_unique<MainMenuWeaponSelect>(this->menu_context()));
		}
		else
		{
			this->context()->transition_to(
				std::make_unique<MainMenuTeamSelect>(this->menu_context()));
		}
		return;
	}
	this->refresh_players(this->input()->player_inputs(*settings));
}
#pragma endregion MainMenuJoin
