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
	this->set_prompts(MenuPrompt::credits);
	this->focus_.set_style({ look.art.focus_ink, look.art.ink, look.art.disabled_ink });

	this->background_ = std::make_unique<UiTexture>("background",
		style.sprite_sheet, style.fill_frame,
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION), this->render_resources(),
		look.home_background_colour);
	this->heading_ = std::make_unique<UiText>("heading", this->text().heading(MenuHeading::credits),
		style.heading_font, look.credits_heading_position,
		this->render_resources(), look.art.ink);
	this->body_ = std::make_unique<UiText>("credits_body", look.credits_text,
		look.credits_font, look.credits_position, this->render_resources(),
		look.art.ink);
	this->back_ = std::make_unique<UiText>("back", this->text().item(MenuItem::back),
		style.item_font, look.credits_back_position, this->render_resources(),
		look.art.focus_ink);
	this->paper_ = std::make_unique<MenuPaperArtwork>(
		RectangleF((Vector2F(DESIGN_RESOLUTION.x, style.footer_y) - look.credits_box_size) / 2.0f,
			look.credits_box_size), this->render_resources(), &style, &look.art, &this->focus_);

	// Keep every authored credit visible above Back and inside the paper face.
	const RectangleF body = this->body_->bounds();
	const float available_width = this->paper_->rectangle().right() - look.art.shadow_offset -
		look.art.focus_padding.x - look.credits_position.x;
	const float available_height = look.credits_back_position.y - look.art.focus_padding.y -
		look.credits_position.y;
	float scale = 1.0f;
	if (available_width > 0.0f && body.width > available_width)
	{
		scale = available_width / body.width;
	}
	if (available_height > 0.0f && body.height * scale > available_height)
	{
		scale = available_height / body.height;
	}
	this->body_->set_scale(scale);

	this->texture_container_ = std::make_unique<UiContainer>("texture_container");
	this->texture_container_->add_child(this->background_.get());
	this->texture_container_->add_child(this->paper_.get());
	this->text_container_ = std::make_unique<UiContainer>("text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->body_.get());
	this->text_container_->add_child(this->back_.get());
	this->focus_.add(this->back_.get(), [this] { this->return_home(); });
}
