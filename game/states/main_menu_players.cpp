#include "game/pch.h"
#include "game/states/main_menu.h"
#include "game/objects/crude_drawing.h"
#include "game/carried/ui_text_drop_shadow.h"
#include "game/states/player_label.h"

using namespace mattmath;
using namespace labrador;

#pragma region MainMenuPlayerCount

MainMenuPlayerCount::MainMenuPlayerCount(MenuContext* context) :
	RosterSetupPage(context)
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
			this->go_to(MainMenuScreen::mode_select);
			return;
		}
		if (input.action == MenuInputAction::proceed)
		{
			this->activating_slot_ = slot;
			if (this->activate_focus() == Activation::ran)
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
	this->set_prompts(MenuPrompt::select_back);
	const MainMenuPresentation& look = this->look();
	const MenuStyle& style = this->menu_style();
	this->setup_art_ = menu_geometry::setup_art(look);
	// Keep the existing measured/navigation extents, including their offsets,
	// while lettering uses the paper pages' plain dark ink.
	const Colour clear_shadow(0.0f, 0.0f, 0.0f, 0.0f);

	// Devices can be connected and claimed on the next page.
	this->focus_.set_style({ look.art.focus_ink, look.art.ink, look.art.disabled_ink });

	this->set_widget_spacing(this->look().player_count_spacing);

	this->background_ = std::make_unique<UiTexture>(
		"background",
		style.sprite_sheet,
		style.fill_frame,
		menu_geometry::design_area(),
		this->render_resources(),
		CrudeDrawing::premultiplied(look.home_background_colour));

	this->heading_ = std::make_unique<colourwars::UiTextDropShadow>(
		"heading",
		this->text().heading(MenuHeading::players),
		this->menu_style().heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		look.art.ink,
		clear_shadow,
		this->menu_style().heading_shadow_offset);

	this->_1_player = std::make_unique<colourwars::UiTextDropShadow>(
		"1_player",
		this->text().item(MenuItem::one_player_practice),
		this->menu_style().item_font,
		this->calculate_widget_position(0, 2),
		this->render_resources(),
		look.art.focus_ink,
		clear_shadow,
		this->menu_style().item_shadow_offset);

	this->_2_players = std::make_unique<colourwars::UiTextDropShadow>(
		"2_players",
		this->text().item(MenuItem::two_players),
		this->menu_style().item_font,
		this->calculate_widget_position(0, 3),
		this->render_resources(),
		look.art.ink,
		clear_shadow,
		this->menu_style().item_shadow_offset);

	this->_3_players = std::make_unique<colourwars::UiTextDropShadow>(
		"3_players",
		this->text().item(MenuItem::three_players),
		this->menu_style().item_font,
		this->calculate_widget_position(0, 4),
		this->render_resources(),
		look.art.ink,
		clear_shadow,
		this->menu_style().item_shadow_offset);

	this->_4_players = std::make_unique<colourwars::UiTextDropShadow>(
		"4_players",
		this->text().item(MenuItem::four_players),
		this->menu_style().item_font,
		this->calculate_widget_position(0, 5),
		this->render_resources(),
		look.art.ink,
		clear_shadow,
		this->menu_style().item_shadow_offset);

	this->back_ = std::make_unique<colourwars::UiTextDropShadow>(
		"back",
		this->text().item(MenuItem::back),
		this->menu_style().item_font,
		this->calculate_widget_position(0, 6),
		this->render_resources(),
		look.art.ink,
		clear_shadow,
		this->menu_style().item_shadow_offset);
	this->paper_ = std::make_unique<MenuPaperArtwork>(
		menu_geometry::footer_paper(look.setup_box_size, style.footer_y),
		this->render_resources(), &style, &this->setup_art_, &this->focus_, nullptr,
		&this->focus_motion_, &this->settings_session()->play_settings());

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->background_.get());
	this->texture_container_->add_child(this->paper_.get());

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
	// Retain the previous count's focus, without retaining any device claims.
	const int count = std::max(this->level_settings()->player_count(),
		this->level_settings()->best_of() > 1 ? 2 : 1);
	switch (count)
	{
	case 4:
		this->focus_.set_focused(0, this->_4_players.get());
		break;
	case 3:
		this->focus_.set_focused(0, this->_3_players.get());
		break;
	case 2:
		this->focus_.set_focused(0, this->_2_players.get());
		break;
	default:
		this->focus_.set_focused(0, this->_1_player.get());
		break;
	}
	this->focus_.add(this->back_.get(), [this]
		{
			this->play_wave(this->cancel_sound_);
			this->go_to(MainMenuScreen::mode_select);
		});

}

void MainMenuPlayerCount::start_join(int players, ScreenLayout layout)
{
	this->play_wave(this->confirm_sound_);
	MatchSetup* settings = this->level_settings();
	settings->set_player_count(players);
	settings->set_screen_layout(layout);
	settings->begin_input_claims(this->activating_slot_);
	this->take_profile_boost(this->activating_slot_);
	if (settings->bot_count() > 0)
	{
		constexpr std::array<ScreenLayout, 4> layouts{ ScreenLayout::one_player,
			ScreenLayout::two_player, ScreenLayout::three_player, ScreenLayout::four_player };
		settings->set_screen_layout(layouts[static_cast<size_t>(settings->human_count() - 1)]);
	}
	if (players == 1 && settings->player_team(0) != PlayerTeam::a &&
		settings->player_team(0) != PlayerTeam::b)
	{
		settings->set_player_team(0, PlayerTeam::a);
	}
	this->go_to(MainMenuScreen::join);
}

#pragma endregion MainMenuPlayerCount

#pragma region MainMenuJoin
MainMenuJoin::MainMenuJoin(MenuContext* context) : RosterSetupPage(context)
{
}

void MainMenuJoin::init()
{
	this->set_prompts(MenuPrompt::join);
	this->set_widget_spacing(this->look().player_count_spacing);
	const MainMenuPresentation& look = this->look();
	const MenuStyle& style = this->menu_style();
	this->setup_art_ = menu_geometry::setup_art(look);
	const Colour clear_shadow(0.0f, 0.0f, 0.0f, 0.0f);
	this->background_ = std::make_unique<UiTexture>("background",
		style.sprite_sheet, style.fill_frame, menu_geometry::design_area(),
		this->render_resources(), CrudeDrawing::premultiplied(look.home_background_colour));
	this->paper_ = std::make_unique<MenuPaperArtwork>(
		menu_geometry::footer_paper(look.setup_box_size, style.footer_y),
		this->render_resources(), &style, &this->setup_art_, &this->focus_, nullptr,
		&this->focus_motion_, &this->settings_session()->play_settings());
	this->texture_container_ = std::make_unique<UiContainer>("texture_container");
	this->texture_container_->add_child(this->background_.get());
	this->texture_container_->add_child(this->paper_.get());
	this->text_container_ = std::make_unique<UiContainer>("text_container");
	this->heading_ = std::make_unique<colourwars::UiTextDropShadow>("heading", this->text().heading(MenuHeading::join_players),
		style.heading_font, this->calculate_widget_position(0, 0),
		this->render_resources(), this->setup_art_.ink, clear_shadow,
		style.heading_shadow_offset);
	this->text_container_->add_child(this->heading_.get());
	std::ignore = this->check_paper_bounds(*this->heading_, "heading");
	for (int player = 0; player < this->player_count(); ++player)
	{
		this->players_.push_back(std::make_unique<colourwars::UiTextDropShadow>(
			"player_" + std::to_string(player), player_label(*this->level_settings(), player, this->text()), style.item_font,
			this->calculate_widget_position(0, player + 2), this->render_resources(),
			this->setup_art_.ink, clear_shadow, style.item_shadow_offset));
		this->text_container_->add_child(this->players_.back().get());
		auto motion = std::make_unique<JoinMotionRow>(style.motion);
		motion->motion.observe(this->players_.back().get());
		this->register_selection_motion(&motion->motion);
		const RectangleF row = this->players_.back()->bounds();
		motion->accent = std::make_unique<MenuJoinAccent>(
			"join_accent_" + std::to_string(player),
			RectangleF(row.x - style.focus_padding.x, row.y,
				std::max(0.0f, style.focus_padding.x), row.height),
			this->render_resources(), &style, &motion->motion, &this->settings_session()->play_settings());
		motion->accent->set_colour(this->setup_art_.focus_ink);
		this->texture_container_->add_child(motion->accent.get());
		this->join_motion_.push_back(std::move(motion));
	}
	this->status_ = std::make_unique<colourwars::UiTextDropShadow>("status", L"",
		style.detail_font, this->calculate_widget_position(0, 7),
		this->render_resources(), this->setup_art_.ink, clear_shadow,
		style.detail_shadow_offset);
	this->text_container_->add_child(this->status_.get());
	this->host_cursor_ = this->make_host_cursor();
	this->texture_container_->add_child(this->host_cursor_.get());
	this->refresh_players(this->input()->player_inputs(*this->level_settings()));
}

RectangleF MainMenuJoin::check_paper_bounds(const UiWidget& widget, const char* role,
	const UiWidget* accent) const
{
	const RectangleF panel = this->paper_->rectangle();
	const RectangleF inside = menu_geometry::paper_interior(panel, this->setup_art_);
	const RectangleF bounds = accent == nullptr ? widget.bounds() :
		RectangleF::union_of(widget.bounds(), accent->bounds());
	if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
		!std::isfinite(bounds.width) || !std::isfinite(bounds.height) ||
		bounds.width <= 0.0f || bounds.height <= 0.0f || !inside.contains(bounds))
	{
		throw std::runtime_error(std::string("main_menu.setup_box_size: Join ") + role +
			" must fit the paper including its retained text and acknowledgement bounds.");
	}
	return bounds;
}

void MainMenuJoin::refresh_players(const std::vector<ProcessedMenuInput>& inputs)
{
	bool ready = this->level_settings()->input_roster_complete();
	const std::vector<SeatSetup>& players = this->level_settings()->player_settings();
	float previous_bottom = this->heading_->bounds().bottom();
	for (int player = 0; player < this->player_count(); ++player)
	{
		const bool connected = inputs[player].connected;
		JoinMotionRow& motion = *this->join_motion_[player];
		// A reserved pad's reconnect note takes the place of its profile's name.
		const int worn = players[player].controller == PlayerController::human && connected ?
			this->settings_session()->worn_profile(players[player].num) : -1;
		const std::wstring label = this->text().worn_profile_label(
			player_label(*this->level_settings(), player, this->text()), worn < 0 ? std::string_view() :
			std::string_view(this->save()->profiles().list[static_cast<size_t>(worn)].name)) +
			(!connected && players[player].num >= 0 ? this->text().connection.join_reconnect_suffix : L"");
		const bool row_changed = motion.slot != players[player].num || motion.connected != connected ||
			this->players_[player]->text() != label;
		if (row_changed)
		{
			// Device changes settle quietly; only accepted join/leave actions
			// below may respond, after the final roster has been refreshed.
			motion.motion.reset();
		}
		motion.slot = players[player].num;
		motion.connected = connected;
		ready = ready && connected;
		// The label follows only the row's seat and connection. Init labels each
		// row for its starting record (slot -1, disconnected), which carries no
		// reconnect suffix, so an unchanged row already shows the right text.
		if (row_changed)
		{
			this->players_[player]->set_text(label);
		}
		this->players_[player]->set_colour(connected ?
			this->setup_art_.focus_ink : this->setup_art_.disabled_ink);
		const RectangleF row = this->check_paper_bounds(*this->players_[player], "player row", motion.accent.get());
		if (row.y < previous_bottom)
		{
			throw std::runtime_error("main_menu.player_count_spacing: Join rows must follow the heading without overlap.");
		}
		previous_bottom = row.bottom();
	}
	const auto editable = [&](int row)
	{
		return row > 0 && row < this->player_count() &&
			(players[static_cast<size_t>(row)].controller == PlayerController::bot ||
				players[static_cast<size_t>(row)].num < 0);
	};
	if (!editable(this->host_row_))
	{
		this->host_row_ = this->next_host_row(0, Direction::down, true);
	}
	this->host_cursor_->set_hidden(!editable(this->host_row_));
	if (editable(this->host_row_))
	{
		this->place_host_cursor(*this->host_cursor_, *this->players_[static_cast<size_t>(this->host_row_)]);
		std::ignore = this->check_paper_bounds(*this->host_cursor_, "host row cursor");
	}
	const std::wstring& status = ready ? this->text().connection.join_ready :
		this->text().connection.join_waiting;
	if (this->status_->text() != status)
	{
		this->status_->set_text(status);
	}
	if (this->check_paper_bounds(*this->status_, "status").y < previous_bottom)
	{
		throw std::runtime_error("main_menu.player_count_spacing: Join status must follow the player rows without overlap.");
	}
}

void MainMenuJoin::change_host_row(Direction direction)
{
	MatchSetup& settings = *this->level_settings();
	if (this->host_row_ <= 0 || this->host_row_ >= this->player_count()) { return; }
	const auto& seat = settings.player_settings()[static_cast<size_t>(this->host_row_)];
	if (seat.controller == PlayerController::human)
	{
		if (seat.num >= 0) { return; }
		settings.set_player_bot(this->host_row_, BotDifficulty::normal);
	}
	else if (direction == Direction::left && seat.difficulty == BotDifficulty::hard)
	{
		settings.set_player_bot(this->host_row_, BotDifficulty::normal);
	}
	else if (direction == Direction::right && seat.difficulty == BotDifficulty::easy)
	{
		settings.set_player_bot(this->host_row_, BotDifficulty::normal);
	}
	else if (direction == Direction::left && seat.difficulty == BotDifficulty::normal)
	{
		settings.set_player_bot(this->host_row_, BotDifficulty::easy);
	}
	else if (direction == Direction::right && seat.difficulty == BotDifficulty::normal)
	{
		settings.set_player_bot(this->host_row_, BotDifficulty::hard);
	}
	else { settings.set_player_human(this->host_row_); }
	constexpr std::array<ScreenLayout, 4> layouts{ ScreenLayout::one_player,
		ScreenLayout::two_player, ScreenLayout::three_player, ScreenLayout::four_player };
	settings.set_screen_layout(layouts[static_cast<size_t>(settings.human_count() - 1)]);
	this->play_wave(this->direction_sound_);
}

void RosterSetupPage::take_profile_boost(int slot)
{
	// A seat starts from the Boost its pad's profile carries; Weapon Select
	// can still change it, and the match carries the choice back.
	const int worn = this->settings_session()->worn_profile(slot);
	const int player = this->level_settings()->player_index_for_slot(slot);
	if (worn >= 0 && player >= 0)
	{
		this->level_settings()->set_player_handicap(player,
			this->save()->profiles().list[static_cast<size_t>(worn)].boost);
	}
}

void MainMenuJoin::update_page(float /*dt*/)
{
	if (!this->context()->active())
	{
		return;
	}
	MatchSetup* settings = this->level_settings();
	const std::vector<ProcessedMenuInput> players = this->input()->player_inputs(*settings);
	const std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	// Evaluate readiness before accepting joins: the last join cannot also
	// dismiss the page in the same frame as Player 1's confirmation.
	const bool ready = settings->input_roster_complete() &&
		std::all_of(players.begin(), players.end(),
			[](const ProcessedMenuInput& input) { return input.connected; });
	bool continue_requested = false;
	bool roster_changed = false;
	std::vector<bool> changed_rows(static_cast<size_t>(this->player_count()), false);
	// Convert before accepting other devices' claims, so a simultaneous join
	// cannot turn an already connected human into a bot.
	const ProcessedMenuInput& host = players[0];
	if (host.connected && host.action == MenuInputAction::none)
	{
		if (host.direction == Direction::up || host.direction == Direction::down)
		{
			const int next = this->next_host_row(this->host_row_, host.direction, true);
			if (next != this->host_row_) { this->host_row_ = next; this->play_wave(this->direction_sound_); }
		}
		else if (host.direction == Direction::left || host.direction == Direction::right)
		{
			this->change_host_row(host.direction);
			roster_changed = true;
		}
	}
	for (int slot = 0; slot < static_cast<int>(inputs.size()); ++slot)
	{
		const int player = settings->player_index_for_slot(slot);
		if (inputs[slot].action == MenuInputAction::back)
		{
			if (player == 0 || !players[0].connected)
			{
				this->play_wave(this->cancel_sound_);
				this->go_to(MainMenuScreen::player_count);
				return;
			}
			if (settings->release_input_slot(slot))
			{
				roster_changed = true;
				changed_rows[static_cast<size_t>(player)] = true;
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
				changed_rows[static_cast<size_t>(settings->player_index_for_slot(slot))] = true;
				this->take_profile_boost(slot);
				this->play_wave(this->ready_sound_);
			}
		}
		else if (inputs[slot].action == MenuInputAction::shuffle && player >= 0 && input_slots::is_pad(slot))
		{
			// A joined pad steps through the profiles it may wear, then none.
			const int next = this->settings_session()->next_wearable_profile(slot, 1);
			if (next != this->settings_session()->worn_profile(slot) && this->settings_session()->wear_profile(slot, next))
			{
				this->take_profile_boost(slot);
				roster_changed = true;
				changed_rows[static_cast<size_t>(player)] = true;
				this->play_wave(this->direction_sound_);
			}
		}
	}
	if (continue_requested && !roster_changed)
	{
		this->play_wave(this->confirm_sound_);
		if (this->player_count() == 1)
		{
			this->go_to(MainMenuScreen::weapon_select);
		}
		else
		{
			this->go_to(MainMenuScreen::team_select);
		}
		return;
	}
	this->refresh_players(this->input()->player_inputs(*settings));
	for (size_t player = 0; player < changed_rows.size(); ++player)
	{
		if (changed_rows[player])
		{
			// A release and replacement in the same row produce one response.
			this->join_motion_[player]->motion.reset();
			this->join_motion_[player]->motion.press();
		}
	}
}
#pragma endregion MainMenuJoin
