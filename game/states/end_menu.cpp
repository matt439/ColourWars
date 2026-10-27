#include "game/pch.h"
#include "game/states/end_menu.h"
#include "game/states/menu_panel.h"
#include "game/states/main_menu.h"
#include "game/series_stage_rule.h"

using namespace mattmath;
using namespace menu_consts;
using namespace labrador;

EndMenuPage::EndMenuPage(MenuContext* context) :
	MenuPage(context)
{

}

const EndMenuPresentation& EndMenuPage::look() const
{
	return this->presentation().end_menu();
}

void EndMenuPage::finish(EndMenuAction action) const
{
	this->menu_context()->finish_with_music([context = this->context(), action]
	{
		context->pop(action);
	});
}

bool EndMenuPage::debug_dismiss(bool leaving_match)
{
	this->menu_context()->cancel_music_exit();
	this->context()->pop(leaving_match ? EndMenuAction::exit : EndMenuAction::restart);
	return true;
}

EndMenuInitial::EndMenuInitial(MenuContext* context, const Session& session, bool replay) :
	EndMenuPage(context), session_(session), replay_(replay)
{

}

void EndMenuInitial::update_page(float /*dt*/)
{
	for (const ProcessedMenuInput& input : this->menu_inputs())
	{
		if (input.action == MenuInputAction::proceed)
		{
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
void EndMenuInitial::init()
{
	this->set_prompts(MenuPrompt::select);

	const MenuStyle& style = this->menu_style();
	const EndMenuPresentation& look = this->look();
	const bool ongoing = this->session_.is_series() && !this->session_.complete();
	// The continuation is row 0. Watch Replay follows it when there is one;
	// between rounds Change Teams is hidden and the rest move up a row.
	const int replay_rows = this->replay_ ? 1 : 0;
	const int teams_row = 1 + replay_rows;
	const int setup_row = ongoing ? teams_row : teams_row + 1;

	this->focus_.set_style({ look.art.focus_ink, look.art.ink });

	this->set_widget_position(look.layout.position);
	this->set_widget_size(look.layout.size);
	this->set_widget_spacing(look.layout.spacing);

	this->heading_ = std::make_unique<UiText>(
		"heading",
		this->session_.is_series() ? (this->session_.complete() ?
			this->text().end.series_complete : this->text().end.round_complete) :
			this->text().end.match_complete,
		style.heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		look.art.ink);

	this->heading_->set_position(Vector2F(
		this->calculate_center_position(this->heading_->bounds().width, DESIGN_RESOLUTION.x),
		look.heading_y));

	this->change_teams_ = std::make_unique<UiText>(
		"change_teams",
		this->text().item(MenuItem::change_teams),
		style.item_font,
		this->calculate_widget_position(0, teams_row),
		this->render_resources(),
		look.art.ink);

	this->watch_replay_ = std::make_unique<UiText>(
		"watch_replay",
		this->text().item(MenuItem::watch_replay),
		style.item_font,
		this->calculate_widget_position(0, 1),
		this->render_resources(),
		look.art.ink);

	this->change_weapons_ = std::make_unique<UiText>(
		"change_weapons",
		this->text().item(MenuItem::change_weapons),
		style.item_font,
		this->calculate_widget_position(0, setup_row),
		this->render_resources(),
		look.art.ink);

	this->change_level_ = std::make_unique<UiText>(
		"change_level",
		this->text().item(MenuItem::change_level),
		style.item_font,
		this->calculate_widget_position(0, setup_row + 1),
		this->render_resources(),
		look.art.ink);

	this->restart_ = std::make_unique<UiText>(
		"restart",
		this->session_.continue_string(this->text().end),
		style.item_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		look.art.focus_ink);

	this->exit_ = std::make_unique<UiText>(
		"exit",
		this->text().item(MenuItem::exit_to_main_menu),
		style.item_font,
		this->calculate_widget_position(0, setup_row + 2),
		this->render_resources(),
		look.art.ink);

	this->box_ = std::make_unique<MenuPaperArtwork>(
		RectangleF(Vector2F::ZERO, look.box_size), this->render_resources(),
		&style, &look.art, &this->focus_, this->restart_.get(),
		&this->focus_motion_, &this->save()->play_settings());
	this->box_->set_position_at_center(DESIGN_RESOLUTION / 2.0f);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	// Paint the opaque page after the shared chrome's focus backing, then
	// its labels. This leaves the shared footer and control legend untouched.
	this->text_container_->add_child(this->box_.get());
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->restart_.get());
	this->text_container_->add_child(this->watch_replay_.get());
	this->text_container_->add_child(this->change_teams_.get());
	this->text_container_->add_child(this->change_weapons_.get());
	this->text_container_->add_child(this->change_level_.get());
	this->text_container_->add_child(this->exit_.get());

	const auto choose = [this](EndMenuAction action, bool confirm)
		{
			this->play_wave(confirm ? this->confirm_sound_ : this->cancel_sound_);
			this->finish(action);
		};

	// Between rounds the teams hold, since the ledger counts their wins, but
	// tools, boosts and the stage may change. Those pages replace this one in
	// its frame: Back returns here and their last confirmation plays on.
	const auto repick = [this](bool tools)
		{
			this->play_wave(this->confirm_sound_);
			if (tools)
			{
				this->context()->transition_to(std::make_unique<MainMenuWeaponSelect>(
					this->menu_context(), this->session_));
			}
			else
			{
				this->context()->transition_to(std::make_unique<MainMenuStageSelect>(
					this->menu_context(), this->session_));
			}
		};

	this->change_teams_->set_hidden(ongoing);
	this->watch_replay_->set_hidden(!this->replay_);
	// Under Loser's pick a decisive round's losers choose the next stage
	// before it plays; a draw or a bot-only loser plays on straight away.
	const PlayerTeam picker = series_stage_picker(*this->level_settings(), this->session_);
	if (picker == PlayerTeam::none)
	{
		this->focus_.add(this->restart_.get(),
			[choose] { choose(EndMenuAction::restart, true); });
	}
	else
	{
		this->focus_.add(this->restart_.get(), [this, picker]
			{
				this->play_wave(this->confirm_sound_);
				this->context()->transition_to(std::make_unique<MainMenuStageSelect>(
					this->menu_context(), this->session_, picker));
			});
	}
	if (this->replay_)
	{
		this->focus_.add(this->watch_replay_.get(),
			[choose] { choose(EndMenuAction::watch_replay, true); });
	}
	if (ongoing)
	{
		this->focus_.add(this->change_weapons_.get(), [repick] { repick(true); });
		this->focus_.add(this->change_level_.get(), [repick] { repick(false); });
	}
	else
	{
		this->focus_.add(this->change_teams_.get(),
			[choose] { choose(EndMenuAction::change_teams, true); });
		this->focus_.add(this->change_weapons_.get(),
			[choose] { choose(EndMenuAction::change_weapons, true); });
		this->focus_.add(this->change_level_.get(),
			[choose] { choose(EndMenuAction::change_level, true); });
	}
	this->focus_.add(this->exit_.get(),
		[choose] { choose(EndMenuAction::exit, false); });
	if (this->session_.is_series() && this->session_.complete())
	{
		this->play_wave(this->resolve_wave(look.series_sound.name), look.series_sound.volume);
	}
}
