#include "game/pch.h"
#include "game/states/menu_panel.h"
#include "game/states/main_menu.h"
#include "game/states/pause_menu.h"
#include "game/debug/debug_director.h"
#include "game/states/player_label.h"

using namespace mattmath;
using namespace menu_consts;
using namespace labrador;

bool match_owns_pad_slot(const MatchSetup& settings, int slot)
{
	if (slot < 0)
	{
		return false;
	}
	return settings.player_index_for_slot(slot) != -1;
}

bool pad_seat_missing(const MatchSetup& settings, int slot,
	const Gamepads& gamepads, const DebugDirector& debug)
{
	return input_slots::is_pad(slot) && !gamepads.connected(settings.device_slot(slot)) &&
		!debug.drives_slot(slot);
}

int disconnected_player_slot(const MatchSetup& settings,
	const Gamepads& gamepads, const DebugDirector& debug)
{
	for (const SeatSetup& player : settings.player_settings())
	{
		if (pad_seat_missing(settings, player.num, gamepads, debug))
		{
			return player.num;
		}
	}
	return -1;
}

int reclaim_missing_seat(MatchSetup& settings,
	const std::vector<ProcessedMenuInput>& inputs, unsigned int missing_seats)
{
	const std::vector<SeatSetup>& players = settings.player_settings();
	int seat = -1;
	for (int player = 0; player < static_cast<int>(players.size()); ++player)
	{
		if ((missing_seats & (1u << player)) != 0)
		{
			seat = player;
			break;
		}
	}
	if (seat == -1)
	{
		return -1;
	}
	const int slot = players[static_cast<size_t>(seat)].num;
	for (int device = 0; device < input_slots::pad_count &&
		device < static_cast<int>(inputs.size()); ++device)
	{
		const ProcessedMenuInput& input = inputs[static_cast<size_t>(device)];
		if (input.connected && input.action == MenuInputAction::pause &&
			settings.player_index_for_device(device) == -1 &&
			settings.move_to_device(slot, device))
		{
			return slot;
		}
	}
	return -1;
}

void latch_intro_pause(bool intro_playing, std::vector<SlotInput>& player_inputs,
	std::array<bool, input_slots::count>& latched)
{
	const size_t slots = std::min(player_inputs.size(), latched.size());
	for (size_t i = 0; i < slots; i++)
	{
		SlotInput& input = player_inputs[i];
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

PauseMenuPage::PauseMenuPage(MenuContext* context, int player_num, bool practice) :
	MenuPage(context),
	pausing_player_(player_num), practice_(practice)
{
	this->window_open_sound_ = this->resolve_sound(this->look().window_open_sound);
}

const PauseMenuPresentation& PauseMenuPage::look() const
{
	return this->presentation().pause_menu();
}

std::wstring PauseMenuPage::player_number_text() const
{
	const int player = this->level_settings()->player_index_for_slot(
		this->pausing_player_);
	return player >= 0 ? this->text().pause_title_for_player(player + 1) :
		this->text().connection.pause_title;
}

int PauseMenuPage::pausing_player() const
{
	return this->pausing_player_;
}

bool PauseMenuPage::practice() const
{
	return this->practice_;
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
	return menu_input_for_owner(inputs, this->pausing_player_, *this->level_settings());
}

PauseMenuInitial::PauseMenuInitial(MenuContext* context, int player_num, bool opening,
	bool practice) :
	PauseMenuPage(context, player_num, practice), opening_(opening)
{

}

void PauseMenuInitial::cycle_tool(Direction direction)
{
	// The five real tools; Random is a setup choice, not something to hold.
	const int count = static_cast<int>(WeaponType::max_prim_wep);
	const int step = direction == Direction::left ? count - 1 : 1;
	this->tool_choice_ = static_cast<WeaponType>(
		(static_cast<int>(this->tool_choice_) + step) % count);
	this->tool_->set_text(this->text().practice_tool_label(
		this->presentation().main_menu().weapon_select_art.choice(this->tool_choice_).name));
}

void PauseMenuInitial::update_page(float /*dt*/)
{
	if (!this->context()->active())
	{
		return;
	}
	this->refresh_connections();
	const std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	if (this->waiting_for_players_ && reclaim_missing_seat(*this->level_settings(),
		inputs, this->shown_missing_seats_.value_or(0)) != -1)
	{
		// The seat's "back" cue plays from its new pad; the list shrinks now.
		this->refresh_connections();
		return;
	}
	const ProcessedMenuInput input = this->pausing_player_input(inputs);

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
		this->activate_focus();
	}
	else if (this->focus_.move(0, input.direction))
	{
		this->play_wave(this->direction_sound_);
	}
	else if (this->tool_ && this->focus_.focused(0) == this->tool_.get() &&
		this->focus_.enabled(this->tool_.get()) &&
		(input.direction == Direction::left || input.direction == Direction::right))
	{
		this->play_wave(this->direction_sound_);
		this->cycle_tool(input.direction);
	}
}

void PauseMenuInitial::refresh_connections()
{
	const std::vector<SeatSetup>& players = this->level_settings()->player_settings();
	// Both texts follow only which seats are missing, so they are rebuilt when
	// that set changes; the empty record forces the first build.
	unsigned int missing_seats = 0;
	for (int player = 0; player < static_cast<int>(players.size()); ++player)
	{
		if (pad_seat_missing(*this->level_settings(), players[player].num,
			*this->data()->gamepads(), this->data()->debug()))
		{
			missing_seats |= 1u << player;
		}
	}
	const bool was_waiting = this->waiting_for_players_;
	if (this->shown_missing_seats_ != missing_seats)
	{
		std::wstring missing;
		int count = 0;
		for (int player = 0; player < static_cast<int>(players.size()); ++player)
		{
			if ((missing_seats & (1u << player)) != 0)
			{
				if (count > 0)
				{
					missing += count % 2 == 0 ? L"\n" : L"   ";
				}
				missing += player_label(*this->level_settings(), player, this->text());
				++count;
			}
		}
		this->waiting_for_players_ = !missing.empty();
		this->connection_status_->set_text(this->waiting_for_players_ ? missing :
			this->text().connection.all_connected);
		this->player_num_->set_text(this->waiting_for_players_ ?
			this->text().connection.controller_lost : this->player_number_text());
		this->shown_missing_seats_ = missing_seats;
	}
	this->connection_status_->set_position(Vector2F(
		this->calculate_center_position(this->connection_status_->bounds().width,
			DESIGN_RESOLUTION.x), this->look().status_y));
	this->player_num_->set_position(Vector2F(
		this->calculate_center_position(this->player_num_->bounds().width,
			DESIGN_RESOLUTION.x), this->look().heading_y));
	this->focus_.set_enabled(this->resume_.get(), !this->waiting_for_players_);
	this->focus_.set_enabled(this->restart_.get(), !this->waiting_for_players_);
	if (this->tool_) { this->focus_.set_enabled(this->tool_.get(), !this->waiting_for_players_); }
	if (was_waiting != this->waiting_for_players_)
	{
		// Reconnection never consumes a held button or resumes automatically.
		this->input()->reset_actions();
		if (this->waiting_for_players_)
		{
			this->set_prompts(MenuPrompt::pause_missing);
		}
		else
		{
			this->set_prompts(MenuPrompt::pause);
		}
	}
}

void PauseMenuInitial::init()
{
	this->set_prompts(MenuPrompt::pause);

	const MenuStyle& style = this->menu_style();
	const PauseMenuPresentation& look = this->look();

	this->focus_.set_style({ look.art.focus_ink, look.art.ink, look.art.disabled_ink });

	this->box_ = std::make_unique<MenuPaperArtwork>(
		RectangleF(Vector2F::ZERO, look.box_size),
		this->render_resources(),
		&style, &look.art, &this->focus_, nullptr,
		&this->focus_motion_, &this->save()->play_settings());
	this->box_->set_position_at_center(DESIGN_RESOLUTION / 2.0f);

	this->set_widget_position(look.layout.position);
	this->set_widget_size(look.layout.size);
	// Practice fits a fifth row, Tool, into the same paper.
	const bool practice = this->practice();
	this->set_widget_spacing(practice ?
		Vector2F(look.layout.spacing.x, look.layout.spacing.y * 0.8f) : look.layout.spacing);
	const int first_row = practice ? 1 : 0;

	this->player_num_ = std::make_unique<UiText>(
		"player_num",
		this->player_number_text(),
		style.heading_font,
		Vector2F(0.0f, look.heading_y),
		this->render_resources(),
		look.art.ink);

	this->resume_ = std::make_unique<UiText>(
		"resume",
		this->text().item(MenuItem::resume),
		style.item_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		look.art.focus_ink);

	this->connection_status_ = std::make_unique<UiText>(
		"connection_status", L"", style.detail_font,
		Vector2F(0.0f, look.status_y), this->render_resources(), look.art.ink);

	if (practice)
	{
		// Start from the tool in hand, so Select on an untouched row is a
		// clean course with the same tool, where the player stands.
		const std::vector<SeatSetup>& players = this->level_settings()->player_settings();
		if (!players.empty() && players.front().weapon < WeaponType::max_prim_wep)
		{
			this->tool_choice_ = players.front().weapon;
		}
		this->tool_ = std::make_unique<UiText>("tool",
			this->text().practice_tool_label(
				this->presentation().main_menu().weapon_select_art.choice(this->tool_choice_).name),
			style.item_font, this->calculate_widget_position(0, 1), this->render_resources(),
			look.art.ink);
	}

	this->restart_ = std::make_unique<UiText>(
		"restart",
		this->text().item(MenuItem::restart),
		style.item_font,
		this->calculate_widget_position(0, first_row + 1),
		this->render_resources(),
		look.art.ink);

	this->options_ = std::make_unique<UiText>("options", this->text().item(MenuItem::options),
		style.item_font, this->calculate_widget_position(0, first_row + 2), this->render_resources(),
		look.art.ink);

	this->quit_ = std::make_unique<UiText>(
		"quit",
		this->text().item(MenuItem::quit),
		style.item_font,
		this->calculate_widget_position(0, first_row + 3),
		this->render_resources(),
		look.art.ink);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	// The opaque paper covers the shared chrome's focus backing before
	// drawing its own selection card and the labels above it.
	this->text_container_->add_child(this->box_.get());
	this->text_container_->add_child(this->player_num_.get());
	this->text_container_->add_child(this->connection_status_.get());
	this->text_container_->add_child(this->resume_.get());
	if (this->tool_) { this->text_container_->add_child(this->tool_.get()); }
	this->text_container_->add_child(this->restart_.get());
	this->text_container_->add_child(this->options_.get());
	this->text_container_->add_child(this->quit_.get());

	this->focus_.add(this->resume_.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			this->finish(PauseMenuAction::resume);
		});
	if (this->tool_)
	{
		this->focus_.add(this->tool_.get(), [this]
			{
				this->play_wave(this->confirm_sound_);
				this->level_settings()->set_player_weapon(0, this->tool_choice_);
				this->finish(PauseMenuAction::change_tool);
			});
	}
	this->focus_.add(this->restart_.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			this->context()->transition_to(std::make_unique<PauseMenuConfirmation>(
				this->menu_context(), this->pausing_player(),
				ConfirmationType::restart, this->practice()));
		});
	this->focus_.add(this->options_.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			this->context()->push(std::make_unique<MainMenuOptions>(
				this->menu_context(), this->pausing_player()));
		});
	this->focus_.add(this->quit_.get(), [this]
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(std::make_unique<PauseMenuConfirmation>(
				this->menu_context(), this->pausing_player(),
				ConfirmationType::quit, this->practice()));
		});

	this->refresh_connections();
	if (this->opening_)
	{
		this->play_wave(this->window_open_sound_);
	}
}

PauseMenuConfirmation::PauseMenuConfirmation(MenuContext* context,
	int player_num, ConfirmationType type, bool practice) :
	PauseMenuPage(context, player_num, practice), type_(type)
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
			this->menu_context(), this->pausing_player(), false, this->practice()));
		return;
	}
	const ProcessedMenuInput input =
		this->pausing_player_input(this->menu_inputs());

	if (input.action == MenuInputAction::back)
	{
		this->play_wave(this->cancel_sound_);
		this->context()->transition_to(std::make_unique<PauseMenuInitial>(
			this->menu_context(), this->pausing_player(), false, this->practice()));
		return;
	}
	if (input.action == MenuInputAction::proceed)
	{
		this->activate_focus();
	}
	else if (this->focus_.move(0, input.direction))
	{
		this->play_wave(this->direction_sound_);
	}
}

void PauseMenuConfirmation::init()
{
	this->set_prompts(MenuPrompt::select_back);

	const MenuStyle& style = this->menu_style();
	const PauseMenuPresentation& look = this->look();

	this->focus_.set_style({ look.art.focus_ink, look.art.ink, look.art.disabled_ink });

	this->box_ = std::make_unique<MenuPaperArtwork>(
		RectangleF(Vector2F::ZERO, look.box_size),
		this->render_resources(),
		&style, &look.art, &this->focus_, nullptr,
		&this->focus_motion_, &this->save()->play_settings());
	this->box_->set_position_at_center(DESIGN_RESOLUTION / 2.0f);

	this->set_widget_position(look.layout.position);
	this->set_widget_size(look.layout.size);
	this->set_widget_spacing(look.layout.spacing);

	this->player_num_ = std::make_unique<UiText>(
		"player_num",
		this->player_number_text(),
		style.heading_font,
		Vector2F(0.0f, look.heading_y),
		this->render_resources(),
		look.art.ink);
	this->player_num_->set_position(Vector2F(
		this->calculate_center_position(this->player_num_->bounds().width,
			DESIGN_RESOLUTION.x), look.heading_y));

	this->question_ = std::make_unique<UiText>(
		"question",
		this->question_text(this->type_),
		style.detail_font,
		Vector2F(0.0f, look.status_y),
		this->render_resources(),
		look.art.ink);
	this->question_->set_position(Vector2F(
		this->calculate_center_position(this->question_->bounds().width,
			DESIGN_RESOLUTION.x), look.status_y));

	this->yes_ = std::make_unique<UiText>(
		"yes",
		this->text().item(MenuItem::yes),
		style.item_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		look.art.ink);

	this->no_ = std::make_unique<UiText>(
		"no",
		this->text().item(MenuItem::no),
		style.item_font,
		this->calculate_widget_position(0, 1),
		this->render_resources(),
		look.art.focus_ink);


	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->box_.get());
	this->text_container_->add_child(this->player_num_.get());
	this->text_container_->add_child(this->question_.get());
	this->text_container_->add_child(this->yes_.get());
	this->text_container_->add_child(this->no_.get());

	// Register No first so confirmation defaults to the safe answer.
	this->focus_.add(this->no_.get(), [this]
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(std::make_unique<PauseMenuInitial>(
				this->menu_context(), this->pausing_player(), false, this->practice()));
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
	ConfirmationType type) const
{
	switch (type)
	{
	case ConfirmationType::restart:
		return this->text().confirmations.restart;
	case ConfirmationType::quit:
		return this->text().confirmations.quit;
	default:
		return L"Error";
	}
}
