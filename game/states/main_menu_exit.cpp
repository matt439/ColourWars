#include "game/pch.h"
#include "game/states/main_menu.h"
#include "game/objects/crude_drawing.h"

using namespace labrador;
using namespace mattmath;
using namespace menu_consts;

MainMenuExitConfirmation::MainMenuExitConfirmation(MenuContext* context) :
	MenuPage(context), look_(context->data()->presentation()->pause_menu())
{
}

void MainMenuExitConfirmation::init()
{
	this->input()->reset_actions();
	const MenuStyle& style = this->menu_style();
	const PauseMenuPresentation& look = this->look_;
	this->set_prompts(MenuPrompt::select_back);
	this->set_widget_position(look.layout.position);
	this->set_widget_spacing(look.layout.spacing);
	this->focus_.set_style({ look.art.focus_ink, look.art.ink, look.art.disabled_ink });

	this->background_ = std::make_unique<UiTexture>("background", style.sprite_sheet,
		style.fill_frame, RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION), this->render_resources(),
		CrudeDrawing::premultiplied(this->presentation().main_menu().home_background_colour));
	this->paper_ = std::make_unique<MenuPaperArtwork>(
		RectangleF((DESIGN_RESOLUTION - look.box_size) / 2.0f, look.box_size),
		this->render_resources(), &style, &look.art, &this->focus_, nullptr,
		&this->focus_motion_, &this->save()->play_settings());
	this->heading_ = std::make_unique<UiText>("heading", this->text().item(MenuItem::exit),
		style.heading_font, Vector2F(0.0f, look.heading_y), this->render_resources(), look.art.ink);
	this->heading_->set_position(Vector2F(
		this->calculate_center_position(this->heading_->bounds().width, DESIGN_RESOLUTION.x), look.heading_y));
	this->question_ = std::make_unique<UiText>("question", this->text().confirmations.exit,
		style.detail_font, Vector2F(0.0f, look.status_y), this->render_resources(), look.art.ink);
	this->question_->set_position(Vector2F(
		this->calculate_center_position(this->question_->bounds().width, DESIGN_RESOLUTION.x), look.status_y));
	this->yes_ = std::make_unique<UiText>("yes", this->text().item(MenuItem::yes), style.item_font,
		this->calculate_widget_position(0, 0), this->render_resources(), look.art.ink);
	this->no_ = std::make_unique<UiText>("no", this->text().item(MenuItem::no), style.item_font,
		this->calculate_widget_position(0, 1), this->render_resources(), look.art.focus_ink);

	this->texture_container_ = std::make_unique<UiContainer>("texture_container");
	this->texture_container_->add_child(this->background_.get());
	this->texture_container_->add_child(this->paper_.get());
	this->text_container_ = std::make_unique<UiContainer>("text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->question_.get());
	this->text_container_->add_child(this->yes_.get());
	this->text_container_->add_child(this->no_.get());
	this->focus_.add(this->no_.get(), [this] { this->cancel(); });
	this->focus_.add(this->yes_.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			this->menu_context()->finish_with_music([application = this->data()->application()]
				{
					application->quit();
				});
		});
}

void MainMenuExitConfirmation::cancel()
{
	this->play_wave(this->cancel_sound_);
	this->input()->reset_actions();
	this->context()->pop();
}

bool MainMenuExitConfirmation::debug_dismiss(bool /*leaving_match*/)
{
	this->menu_context()->cancel_music_exit();
	this->cancel();
	return true;
}

void MainMenuExitConfirmation::update_page(float /*dt*/)
{
	if (!this->context()->active()) { return; }
	const std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	// Any device can cancel, even when another confirms in the same frame.
	for (const ProcessedMenuInput& input : inputs)
	{
		if (input.action == MenuInputAction::back)
		{
			this->cancel();
			return;
		}
	}
	for (const ProcessedMenuInput& input : inputs)
	{
		if (input.action == MenuInputAction::proceed && this->activate_focus() == Activation::ran)
		{
			// activate_focus applies its response after the callback; a confirmed
			// exit holds the selected answer still throughout the music fade.
			if (this->menu_context()->music_exit_pending()) { this->reset_menu_motion(); }
			return;
		}
		if (this->focus_.move(0, input.direction))
		{
			this->play_wave(this->direction_sound_);
			// Show the new answer before another device can confirm it.
			return;
		}
	}
}
