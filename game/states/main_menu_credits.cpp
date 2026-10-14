#include "game/pch.h"
#include "game/states/main_menu.h"

using namespace labrador;
using namespace mattmath;
using namespace menu_consts;

MainMenuCredits::MainMenuCredits(MenuContext* context) : MainMenuPage(context)
{
}

void MainMenuCredits::return_home()
{
	this->play_wave(this->cancel_sound_);
	this->context()->transition_to(std::make_unique<MainMenuHome>(this->menu_context()));
}

void MainMenuCredits::update_page(float /*dt*/)
{
	for (const ProcessedMenuInput& input : this->menu_inputs())
	{
		if (input.action == MenuInputAction::back)
		{
			this->return_home();
			return;
		}
		if (input.action == MenuInputAction::proceed &&
			this->focus_.activate(0) == Activation::ran)
		{
			return;
		}
	}
}

void MainMenuCredits::init()
{
	const MenuStyle& style = this->menu_style();
	const MainMenuPresentation& look = this->look();
	this->set_prompts({ { L"A", L"Back" }, { L"B", L"Back" } });
	this->focus_.set_style({ style.highlight, style.unhighlight });

	this->background_ = std::make_unique<UiTexture>("background",
		look.backdrop_sheet, look.backdrop_frame,
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION), this->render_resources(),
		look.options_background_colour);
	this->heading_ = std::make_unique<UiTextDropShadow>("heading", L"Credits",
		style.heading_font, this->calculate_widget_position(0, 0),
		this->render_resources(), style.heading_colour, style.shadow_colour,
		style.heading_shadow_offset);
	this->body_ = std::make_unique<UiTextDropShadow>("credits_body", look.credits_text,
		look.credits_font, look.credits_position, this->render_resources(),
		style.unhighlight, style.shadow_colour, style.detail_shadow_offset);
	this->back_ = std::make_unique<UiTextDropShadow>("back", L"Back",
		style.item_font, look.credits_back_position, this->render_resources(),
		style.highlight, style.shadow_colour, style.item_shadow_offset);

	this->texture_container_ = std::make_unique<UiContainer>("texture_container");
	this->texture_container_->add_child(this->background_.get());
	this->text_container_ = std::make_unique<UiContainer>("text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->body_.get());
	this->text_container_->add_child(this->back_.get());
	this->focus_.add(this->back_.get(), [this] { this->return_home(); });
	this->play_music(this->music_, look.music.volume);
}
