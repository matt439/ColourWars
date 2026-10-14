#include "game/pch.h"
#include "game/states/menu_panel.h"
#include "game/states/controls_settings_menu.h"
#include "game/states/pause_menu.h"
#include "game/debug/debug_director.h"

using namespace DirectX;
using namespace mattmath;
using namespace menu_consts;
using namespace labrador;

bool match_owns_pad_slot(const MenuLevelSettings& settings, int slot)
{
	if (slot < 0)
	{
		return false;
	}
	return settings.player_index_for_slot(slot) != -1;
}

bool pad_seat_missing(int slot, const Gamepads& gamepads, const DebugDirector& debug)
{
	return input_slots::is_pad(slot) && !gamepads.connected(slot) && !debug.drives_slot(slot);
}

int disconnected_player_slot(const MenuLevelSettings& settings,
	const Gamepads& gamepads, const DebugDirector& debug)
{
	for (const MenuPlayerSettings& player : settings.player_settings())
	{
		if (pad_seat_missing(player.num, gamepads, debug))
		{
			return player.num;
		}
	}
	return -1;
}

void latch_intro_pause(bool intro_playing, std::vector<PlayerInputData>& player_inputs,
	std::array<bool, input_slots::count>& latched)
{
	const size_t slots = std::min(player_inputs.size(), latched.size());
	for (size_t i = 0; i < slots; i++)
	{
		PlayerInputData& input = player_inputs[i];
		if (intro_playing && (input.pause_held || input.toggle_pause_menu))
		{
			latched[i] = true;
		}
		if (latched[i] && input.toggle_pause_menu)
		{
			input.toggle_pause_menu = false;
			latched[i] = false;
		}
		else if (!input.pause_held)
		{
			// Released while nothing read it, under a menu or out of focus.
			latched[i] = false;
		}
	}
}

PauseMenuPage::PauseMenuPage(MenuContext* context, int player_num) :
	MenuPage(context),
	pausing_player_(player_num)
{
	this->window_open_sound_ =
		this->resolve_wave(this->look().window_open_sound);
}

const PauseMenuPresentation& PauseMenuPage::look() const
{
	return this->presentation().pause_menu();
}

std::wstring PauseMenuPage::player_number_text() const
{
	const int player = this->level_settings()->player_index_for_slot(
		this->pausing_player_);
	return player >= 0 ? L"Pause Menu P" + std::to_wstring(player + 1) :
		L"Pause Menu";
}

int PauseMenuPage::pausing_player() const
{
	return this->pausing_player_;
}

void PauseMenuPage::finish(PauseMenuAction action) const
{
	this->context()->pop(action);
}

bool PauseMenuInitial::debug_dismiss(bool leaving_match)
{
	this->finish(leaving_match ? PauseMenuAction::quit : PauseMenuAction::resume);
	return true;
}

bool PauseMenuConfirmation::debug_dismiss(bool leaving_match)
{
	this->finish(leaving_match ? PauseMenuAction::quit : PauseMenuAction::resume);
	return true;
}

ProcessedMenuInput PauseMenuPage::pausing_player_input(
	const std::vector<ProcessedMenuInput>& inputs) const
{
	if (this->pausing_player_ < 0 ||
		static_cast<size_t>(this->pausing_player_) >= inputs.size())
	{
		return ProcessedMenuInput();
	}

	const ProcessedMenuInput& pausing =
		inputs[static_cast<size_t>(this->pausing_player_)];
	if (pausing.connected)
	{
		return pausing;
	}

	// The opening device is disconnected. Other participants may navigate;
	// if none are connected, any device may reach Quit.
	const MenuLevelSettings& settings = *this->level_settings();
	bool participant_connected = false;
	for (size_t slot = 0; slot < inputs.size(); slot++)
	{
		const ProcessedMenuInput& input = inputs[slot];
		if (input.connected &&
			match_owns_pad_slot(settings, static_cast<int>(slot)))
		{
			participant_connected = true;
			if (input.action != MenuInputAction::none ||
				input.direction != labrador::Direction::none)
			{
				return input;
			}
		}
	}
	if (!participant_connected)
	{
		for (const ProcessedMenuInput& input : inputs)
		{
			if (input.connected && (input.action != MenuInputAction::none ||
				input.direction != labrador::Direction::none))
			{
				return input;
			}
		}
	}
	return ProcessedMenuInput();
}

PauseMenuInitial::PauseMenuInitial(MenuContext* context, int player_num, bool opening) :
	PauseMenuPage(context, player_num), opening_(opening)
{

}

void PauseMenuInitial::update_page(float /*dt*/)
{
	if (!this->context()->active())
	{
		return;
	}
	this->refresh_connections();
	const ProcessedMenuInput input =
		this->pausing_player_input(this->menu_inputs());

	if (input.action == MenuInputAction::back ||
		input.action == MenuInputAction::pause)
	{
		if (!this->waiting_for_players_)
		{
			this->play_wave(this->cancel_sound_);
			this->finish(PauseMenuAction::resume);
		}
	}
	else if (input.action == MenuInputAction::proceed)
	{
		this->focus_.activate(0);
	}
	else if (this->focus_.move(0, input.direction))
	{
		this->play_wave(this->direction_sound_);
	}
}

void PauseMenuInitial::refresh_connections()
{
	std::wstring missing;
	int count = 0;
	const std::vector<MenuPlayerSettings> players = this->level_settings()->player_settings();
	for (int player = 0; player < static_cast<int>(players.size()); ++player)
	{
		const int slot = players[player].num;
		if (pad_seat_missing(slot, *this->data()->gamepads(), this->data()->debug()))
		{
			if (count > 0)
			{
				missing += count % 2 == 0 ? L"\n" : L"   ";
			}
			missing += this->level_settings()->player_label(player);
			++count;
		}
	}
	const bool was_waiting = this->waiting_for_players_;
	this->waiting_for_players_ = !missing.empty();
	this->connection_status_->set_text(this->waiting_for_players_ ? missing :
		L"All players connected.");
	this->player_num_->set_text(this->waiting_for_players_ ?
		L"Controller lost" : this->player_number_text());
	this->focus_.set_enabled(this->resume_.get(), !this->waiting_for_players_);
	this->focus_.set_enabled(this->restart_.get(), !this->waiting_for_players_);
	if (was_waiting != this->waiting_for_players_)
	{
		// Reconnection never consumes a held button or resumes automatically.
		this->input()->reset_actions();
		if (this->waiting_for_players_)
		{
			this->set_prompts({ { L"", L"Reconnect pads to resume" },
				{ L"A", L"Select" } });
		}
		else
		{
			this->set_prompts({ { L"A", L"Select" }, { L"B/Start", L"Resume" } });
		}
	}
}

void PauseMenuInitial::init()
{
	this->set_prompts({
		{ L"A", L"Select" },
		{ L"B/Start", L"Resume" } });

	const MenuStyle& style = this->menu_style();
	const PauseMenuPresentation& look = this->look();

	this->focus_.set_style({ style.highlight, style.unhighlight, style.disabled });

	this->box_ = std::make_unique<MenuPanel>(
		"box",
		RectangleF(Vector2F::ZERO, look.box_size),
		this->render_resources(),
		look.box_colour, &style);
	this->box_->set_position_at_center(DESIGN_RESOLUTION / 2.0f);

	this->set_widget_position(look.layout.position);
	this->set_widget_size(look.layout.size);
	this->set_widget_spacing(look.layout.spacing);

	this->player_num_ = std::make_unique<UiTextDropShadow>(
		"player_num",
		this->player_number_text(),
		style.heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		style.heading_colour,
		style.shadow_colour,
		style.heading_shadow_offset);

	this->resume_ = std::make_unique<UiTextDropShadow>(
		"resume",
		L"Resume",
		style.item_font,
		this->calculate_widget_position(1, 2),
		this->render_resources(),
		style.highlight,
		style.shadow_colour,
		style.item_shadow_offset);

	this->connection_status_ = std::make_unique<UiTextDropShadow>(
		"connection_status", L"", style.detail_font,
		this->calculate_widget_position(0, 1), this->render_resources(),
		style.warning, style.shadow_colour, style.detail_shadow_offset);

	this->restart_ = std::make_unique<UiTextDropShadow>(
		"restart",
		L"Restart",
		style.item_font,
		this->calculate_widget_position(1, 3),
		this->render_resources(),
		style.unhighlight,
		style.shadow_colour,
		style.item_shadow_offset);

	this->controls_ = std::make_unique<UiTextDropShadow>("controls", L"Controls / audio",
		style.item_font, this->calculate_widget_position(1, 4), this->render_resources(),
		style.unhighlight, style.shadow_colour, style.item_shadow_offset);

	this->quit_ = std::make_unique<UiTextDropShadow>(
		"quit",
		L"Quit",
		style.item_font,
		this->calculate_widget_position(1, 5),
		this->render_resources(),
		style.unhighlight,
		style.shadow_colour,
		style.item_shadow_offset);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->box_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->player_num_.get());
	this->text_container_->add_child(this->connection_status_.get());
	this->text_container_->add_child(this->resume_.get());
	this->text_container_->add_child(this->restart_.get());
	this->text_container_->add_child(this->controls_.get());
	this->text_container_->add_child(this->quit_.get());

	this->focus_.add(this->resume_.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			this->finish(PauseMenuAction::resume);
		});
	this->focus_.add(this->restart_.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			this->context()->transition_to(std::make_unique<PauseMenuConfirmation>(
				this->menu_context(), this->pausing_player(),
				ConfirmationType::restart));
		});
	this->focus_.add(this->controls_.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			this->context()->push(std::make_unique<ControlsSettingsMenu>(
				this->menu_context(), this->pausing_player()));
		});
	this->focus_.add(this->quit_.get(), [this]
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(std::make_unique<PauseMenuConfirmation>(
				this->menu_context(), this->pausing_player(),
				ConfirmationType::quit));
		});

	this->refresh_connections();
	if (this->opening_)
	{
		this->play_wave(this->window_open_sound_);
	}
}

PauseMenuConfirmation::PauseMenuConfirmation(MenuContext* context,
	int player_num, ConfirmationType type) :
	PauseMenuPage(context, player_num), type_(type)
{

}

void PauseMenuConfirmation::update_page(float /*dt*/)
{
	if (!this->context()->active())
	{
		return;
	}
	if (this->type_ == ConfirmationType::restart &&
		disconnected_player_slot(*this->level_settings(), *this->data()->gamepads(),
			this->data()->debug()) != -1)
	{
		this->context()->transition_to(std::make_unique<PauseMenuInitial>(
			this->menu_context(), this->pausing_player(), false));
		return;
	}
	const ProcessedMenuInput input =
		this->pausing_player_input(this->menu_inputs());

	if (input.action == MenuInputAction::back)
	{
		this->play_wave(this->cancel_sound_);
		this->context()->transition_to(std::make_unique<PauseMenuInitial>(
			this->menu_context(), this->pausing_player(), false));
		return;
	}
	if (input.action == MenuInputAction::proceed)
	{
		this->focus_.activate(0);
	}
	else if (this->focus_.move(0, input.direction))
	{
		this->play_wave(this->direction_sound_);
	}
}

void PauseMenuConfirmation::init()
{
	this->set_prompts({
		{ L"A", L"Select" },
		{ L"B", L"Back" } });

	const MenuStyle& style = this->menu_style();
	const PauseMenuPresentation& look = this->look();

	this->focus_.set_style({ style.highlight, style.unhighlight });

	this->box_ = std::make_unique<MenuPanel>(
		"box",
		RectangleF(Vector2F::ZERO, look.box_size),
		this->render_resources(),
		look.box_colour, &style);
	this->box_->set_position_at_center(DESIGN_RESOLUTION / 2.0f);

	this->set_widget_position(look.layout.position);
	this->set_widget_size(look.layout.size);
	this->set_widget_spacing(look.layout.spacing);

	this->player_num_ = std::make_unique<UiTextDropShadow>(
		"player_num",
		this->player_number_text(),
		style.heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		style.heading_colour,
		style.shadow_colour,
		style.heading_shadow_offset);

	this->question_ = std::make_unique<UiTextDropShadow>(
		"question",
		question_text(this->type_),
		style.detail_font,
		this->calculate_widget_position(0, 2),
		this->render_resources(),
		style.heading_colour,
		style.shadow_colour,
		style.detail_shadow_offset);

	this->yes_ = std::make_unique<UiTextDropShadow>(
		"yes",
		L"Yes",
		style.item_font,
		this->calculate_widget_position(1, 3),
		this->render_resources(),
		style.unhighlight,
		style.shadow_colour,
		style.item_shadow_offset);

	this->no_ = std::make_unique<UiTextDropShadow>(
		"no",
		L"No",
		style.item_font,
		this->calculate_widget_position(1, 4),
		this->render_resources(),
		style.highlight,
		style.shadow_colour,
		style.item_shadow_offset);


	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->box_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->player_num_.get());
	this->text_container_->add_child(this->question_.get());
	this->text_container_->add_child(this->yes_.get());
	this->text_container_->add_child(this->no_.get());

	// Register No first so confirmation defaults to the safe answer.
	this->focus_.add(this->no_.get(), [this]
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(std::make_unique<PauseMenuInitial>(
				this->menu_context(), this->pausing_player(), false));
		});
	this->focus_.add(this->yes_.get(), [this]
		{
			switch (this->type_)
			{
			case ConfirmationType::restart:
				this->play_wave(this->confirm_sound_);
				this->finish(PauseMenuAction::restart);
				break;
			case ConfirmationType::quit:
				this->play_wave(this->cancel_sound_);
				this->finish(PauseMenuAction::quit);
				break;
			}
		});
}


std::wstring PauseMenuConfirmation::question_text(
	ConfirmationType type)
{
	switch (type)
	{
	case ConfirmationType::restart:
		return L"Are you sure you want to restart?";
	case ConfirmationType::quit:
		return L"Are you sure you want to quit?";
	default:
		return L"Error";
	}
}
