#include "game/pch.h"
#include "game/states/main_menu.h"
#include "game/states/match_start.h"
#include "game/states/end_menu.h"
#include "game/build_info.h"
#include "game/objects/crude_drawing.h"

using namespace labrador;

MainMenuPage::MainMenuPage(MenuContext* context) :
	MenuPage(context)
{
}

bool MainMenuPage::debug_start_match(const MatchSetup& plan)
{
	this->menu_context()->cancel_music_exit();
	*this->level_settings() = plan;
	this->context()->pop(MatchStart::developer);
	return true;
}

const MainMenuPresentation& MainMenuPage::look() const
{
	return this->presentation().main_menu();
}

RosterSetupPage::RosterSetupPage(MenuContext* context, std::optional<Session> between_rounds,
	bool replay) :
	MainMenuPage(context), between_rounds_(std::move(between_rounds)),
	replay_(between_rounds_.has_value() && replay)
{
	const MainMenuPresentation& look = this->look();

	this->error_sound_ = this->resolve_sound(look.error_sound);
	this->ready_sound_ = this->resolve_sound(look.ready_sound);
	this->unready_sound_ = this->resolve_wave(look.unready_sound.name);
}

bool RosterSetupPage::debug_start_match(const MatchSetup& plan)
{
	if (this->between_rounds_) { return false; }
	return MainMenuPage::debug_start_match(plan);
}

bool RosterSetupPage::debug_dismiss(bool leaving_match)
{
	if (!this->between_rounds_)
	{
		return MenuPage::debug_dismiss(leaving_match);
	}
	this->menu_context()->cancel_music_exit();
	this->context()->pop(leaving_match ? EndMenuAction::exit : EndMenuAction::restart);
	return true;
}

const std::optional<Session>& RosterSetupPage::between_rounds() const
{
	return this->between_rounds_;
}

bool RosterSetupPage::round_replay() const
{
	return this->replay_;
}

void RosterSetupPage::return_to_round_menu()
{
	this->context()->transition_to(std::make_unique<EndMenuInitial>(
		this->menu_context(), *this->between_rounds_, this->replay_));
}

int RosterSetupPage::player_count() const
{
	return this->level_settings()->
		player_count();
}

void RosterSetupPage::set_player_prompts(MenuPrompt prompts)
{
	this->player_prompts_ = prompts;
	this->set_prompts(prompts);
}

int RosterSetupPage::next_host_row(int row, Direction direction, bool joining) const
{
	if (direction != Direction::up && direction != Direction::down) { return row; }
	const int step = direction == Direction::up ? -1 : 1;
	const auto& players = this->level_settings()->player_settings();
	for (int candidate = row + step; candidate >= 0 && candidate < this->player_count(); candidate += step)
	{
		const auto& seat = players[static_cast<size_t>(candidate)];
		if (joining ? candidate > 0 && (seat.controller == PlayerController::bot || seat.num < 0) :
			candidate == 0 || seat.controller == PlayerController::bot)
		{
			return candidate;
		}
	}
	return row;
}

void RosterSetupPage::route_host_setup_input(std::vector<ProcessedMenuInput>& inputs, int& row)
{
	if (inputs.empty() || this->level_settings()->bot_count() == 0) { return; }
	const ProcessedMenuInput host = inputs[0];
	inputs[0] = { Direction::none, MenuInputAction::none, host.connected };
	if (host.action == MenuInputAction::none &&
		(host.direction == Direction::up || host.direction == Direction::down))
	{
		const int next = this->next_host_row(row, host.direction, false);
		if (next != row) { this->play_wave(this->direction_sound_); row = next; }
		return;
	}
	inputs[static_cast<size_t>(row)] = host;
}

std::unique_ptr<UiTexture> RosterSetupPage::make_host_cursor() const
{
	const MenuStyle& style = this->menu_style();
	return std::make_unique<UiTexture>("host_row", style.sprite_sheet, style.fill_frame,
		RectangleF(0.0f, 0.0f, style.rule_height, style.rule_height), this->render_resources(),
		CrudeDrawing::premultiplied(this->look().art.focus_ink));
}

void RosterSetupPage::place_host_cursor(UiTexture& cursor, const UiWidget& row) const
{
	const RectangleF bounds = row.bounds();
	const MenuStyle& style = this->menu_style();
	// An underline identifies the host's row independently of readiness tint.
	cursor.set_position(Vector2F(bounds.x, bounds.bottom() + style.rule_height));
	cursor.set_size(Vector2F(bounds.width, style.rule_height));
}

bool RosterSetupPage::wait_for_players(
	const std::vector<ProcessedMenuInput>& inputs)
{
	// Focus loss can temporarily hide every pad. It does not change the roster.
	if (!this->context()->active())
	{
		this->reset_menu_motion();
		return true;
	}
	const bool missing = std::any_of(inputs.begin(), inputs.end(),
		[](const ProcessedMenuInput& input) { return !input.connected; });
	if (missing != this->waiting_for_players_)
	{
		this->waiting_for_players_ = missing;
		if (missing)
		{
			this->set_prompts(MenuPrompt::missing_player);
		}
		else
		{
			this->set_prompts(this->player_prompts_);
		}
	}
	if (missing)
	{
		this->reset_menu_motion();
		// Any available device may recover a setup whose owner disconnected.
		for (const ProcessedMenuInput& input : this->menu_inputs())
		{
			if (input.action == MenuInputAction::back)
			{
				this->play_wave(this->cancel_sound_);
				// Between rounds the roster is the series': keep it.
				if (this->between_rounds_)
				{
					this->return_to_round_menu();
				}
				else
				{
					this->go_to(MainMenuScreen::player_count);
				}
				break;
			}
		}
	}
	return missing;
}

#pragma region MainMenuTitle

bool title_shows_no_controller(const std::vector<ProcessedMenuInput>& inputs)
{
	return std::none_of(inputs.begin(), inputs.end(),
		[](const ProcessedMenuInput& input) { return input.connected; });
}

MainMenuTitle::MainMenuTitle(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuTitle::update_page(float /*dt*/)
{
	if (this->context()->active())
	{
		this->no_pad_->set_hidden(!title_shows_no_controller(this->menu_inputs()));
	}

	const std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	for (int i = 0; i < inputs.size(); i++)
	{
		if (inputs[i].action == MenuInputAction::proceed)
		{
			this->play_wave(this->confirm_sound_);
			this->go_to(MainMenuScreen::home);
			return;
		}
	}
}
void MainMenuTitle::init()
{
	this->set_prompts(MenuPrompt::title);
	const MainMenuPresentation& look = this->look();
	const MenuStyle& style = this->menu_style();
	this->focus_.set_style({ look.art.focus_ink, look.art.ink, look.art.disabled_ink });

	this->background_ = std::make_unique<UiTexture>("background",
		style.sprite_sheet, style.fill_frame, menu_geometry::design_area(),
		this->render_resources(), look.title_background_colour);
	this->title_ = std::make_unique<UiText>("title", this->text().title.wordmark,
		look.title_font, Vector2F(0.0f, look.title_y), this->render_resources(), look.art.ink);
	this->subtitle_ = std::make_unique<UiText>("subtitle", this->text().title.subtitle,
		style.detail_font, Vector2F(0.0f, look.title_subtitle_y), this->render_resources(), look.art.ink);
	this->start_ = std::make_unique<UiText>("start", this->text().title.start,
		style.item_font, Vector2F(0.0f, look.title_start_y), this->render_resources(), look.art.focus_ink);
	this->no_pad_ = std::make_unique<UiText>("no_pad", this->text().title.no_controller,
		style.item_font, Vector2F(0.0f, look.title_no_pad_y), this->render_resources(), look.art.ink, true);
	this->build_id_ = std::make_unique<UiText>("build_id", build_info::label,
		this->presentation().prompt_bar().font, Vector2F(0.0f, look.build_id_y),
		this->render_resources(), look.art.ink);

	// Fit the real font metrics, keeping the authored vertical rhythm.
	for (UiText* text : { this->title_.get(), this->subtitle_.get(), this->start_.get(),
		this->no_pad_.get(), this->build_id_.get() })
	{
		text->set_position(Vector2F(menu_geometry::centred_x(text->bounds().width), text->bounds().y));
	}
	this->focus_.add(this->start_.get());
	this->paper_ = std::make_unique<MenuPaperArtwork>(
		menu_geometry::footer_paper(look.title_box_size, style.footer_y),
		this->render_resources(), &style, &look.art, &this->focus_, this->start_.get(),
		&this->focus_motion_, &this->settings_session()->play_settings());

	this->texture_container_ = std::make_unique<UiContainer>("texture_container");
	this->texture_container_->add_child(this->background_.get());
	this->texture_container_->add_child(this->paper_.get());
	this->text_container_ = std::make_unique<UiContainer>("text_container");
	this->text_container_->add_child(this->title_.get());
	this->text_container_->add_child(this->subtitle_.get());
	this->text_container_->add_child(this->start_.get());
	this->text_container_->add_child(this->no_pad_.get());
	this->text_container_->add_child(this->build_id_.get());
}

#pragma endregion MainMenuTitle

#pragma region MainMenuHome

MainMenuHome::MainMenuHome(MenuContext* context, InitialFocus initial_focus) :
	MainMenuPage(context), initial_focus_(initial_focus)
{

}

void MainMenuHome::update_page(float /*dt*/)
{
	// Read focus for each device so later inputs see earlier navigation
	// from the same frame.
	for (const ProcessedMenuInput& input : this->menu_inputs())
	{
		if (input.action == MenuInputAction::back)
		{
			this->play_wave(this->cancel_sound_);
			this->go_to(MainMenuScreen::title);
			return;
		}
		if (input.action == MenuInputAction::proceed)
		{
			// Stop after one activation: every action here leaves the page.
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
void MainMenuHome::init()
{
	const MainMenuPresentation& look = this->look();
	const MenuStyle& style = this->menu_style();
	this->set_widget_position(look.home_position);
	this->set_widget_spacing(look.home_spacing);
	this->set_prompts(MenuPrompt::select_back);
	this->focus_.set_style({ look.art.focus_ink, look.art.ink, look.art.disabled_ink });

	this->background_ = std::make_unique<UiTexture>("background",
		style.sprite_sheet, style.fill_frame, menu_geometry::design_area(),
		this->render_resources(), CrudeDrawing::premultiplied(look.home_background_colour));
	this->heading_ = std::make_unique<UiText>("heading", this->text().heading(MenuHeading::home),
		style.heading_font, this->calculate_widget_position(0, 0), this->render_resources(), look.art.ink);
	this->subtitle_ = std::make_unique<UiText>("subtitle", this->text().home_subtitle,
		style.detail_font, look.home_subtitle_position, this->render_resources(), look.art.ink);
	this->play_ = std::make_unique<UiText>("play", this->text().item(MenuItem::play), style.item_font,
		this->calculate_widget_position(0, 2), this->render_resources(), look.art.focus_ink);
	this->options_ = std::make_unique<UiText>("options", this->text().item(MenuItem::options), style.item_font,
		this->calculate_widget_position(0, 3), this->render_resources(), look.art.ink);
	this->credits_ = std::make_unique<UiText>("credits", this->text().item(MenuItem::credits), style.item_font,
		this->calculate_widget_position(0, 4), this->render_resources(), look.art.ink);
	this->exit_ = std::make_unique<UiText>("exit", this->text().item(MenuItem::exit), style.item_font,
		this->calculate_widget_position(0, 5), this->render_resources(), look.art.ink);
	this->paper_ = std::make_unique<MenuPaperArtwork>(
		menu_geometry::footer_paper(look.home_box_size, style.footer_y),
		this->render_resources(), &style, &look.art, &this->focus_, nullptr,
		&this->focus_motion_, &this->settings_session()->play_settings());

	this->texture_container_ = std::make_unique<UiContainer>("texture_container");
	this->texture_container_->add_child(this->background_.get());
	this->texture_container_->add_child(this->paper_.get());
	this->text_container_ = std::make_unique<UiContainer>("text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->subtitle_.get());
	this->text_container_->add_child(this->play_.get());
	this->text_container_->add_child(this->options_.get());
	this->text_container_->add_child(this->credits_.get());
	this->text_container_->add_child(this->exit_.get());

	// Navigation uses widget bounds in design space, not declaration order.
	this->focus_.add(this->play_.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			this->go_to(MainMenuScreen::mode_select);
		});
	this->focus_.add(this->options_.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			this->go_to(MainMenuScreen::options);
		});
	this->focus_.add(this->credits_.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			this->go_to(MainMenuScreen::credits);
		});
	this->focus_.add(this->exit_.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			this->context()->push(std::make_unique<MainMenuExitConfirmation>(this->menu_context()));
		});

	// Child pages return to their entry row; fresh entry and Mode return use Play.
	switch (this->initial_focus_)
	{
	case InitialFocus::options:
		this->focus_.set_focused(0, this->options_.get());
		break;
	case InitialFocus::credits:
		this->focus_.set_focused(0, this->credits_.get());
		break;
	case InitialFocus::play:
		break;
	}
}

#pragma endregion MainMenuHome
