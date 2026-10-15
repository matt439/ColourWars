#include "game/pch.h"
#include "game/states/end_menu.h"
#include "game/states/menu_panel.h"

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

EndMenuInitial::EndMenuInitial(MenuContext* context, const Session& session) :
	EndMenuPage(context), session_(session)
{

}

void EndMenuInitial::update_page(float /*dt*/)
{
	for (const ProcessedMenuInput& input : this->menu_inputs())
	{
		if (input.action == MenuInputAction::proceed)
		{
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
void EndMenuInitial::init()
{
	this->set_prompts(MenuPrompt::select);

	const MenuStyle& style = this->menu_style();
	const EndMenuPresentation& look = this->look();
	const bool ongoing = this->session_.is_series() && !this->session_.complete();

	this->focus_.set_style({ look.art.focus_ink, look.art.ink });

	this->set_widget_position(look.layout.position);
	this->set_widget_size(look.layout.size);
	this->set_widget_spacing(look.layout.spacing);

	this->heading_ = std::make_unique<UiText>(
		"heading",
		this->session_.is_series() ? (this->session_.complete() ?
			L"Series Complete" : L"Round Complete") : L"Match Complete",
		style.heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		look.art.ink);

	this->heading_->set_position(Vector2F(
		this->calculate_center_position(this->heading_->bounds().width, DESIGN_RESOLUTION.x),
		look.heading_y));

	this->change_teams_ = std::make_unique<UiText>(
		"change_teams",
		L"Change Teams",
		style.item_font,
		this->calculate_widget_position(0, 1),
		this->render_resources(),
		look.art.ink);

	this->change_weapons_ = std::make_unique<UiText>(
		"change_weapons",
		L"Change Weapons",
		style.item_font,
		this->calculate_widget_position(0, 2),
		this->render_resources(),
		look.art.ink);

	this->change_level_ = std::make_unique<UiText>(
		"change_level",
		L"Change Level",
		style.item_font,
		this->calculate_widget_position(0, 3),
		this->render_resources(),
		look.art.ink);

	this->restart_ = std::make_unique<UiText>(
		"restart",
		this->session_.continue_string(),
		style.item_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		look.art.focus_ink);

	this->exit_ = std::make_unique<UiText>(
		"exit",
		L"Exit to Main Menu",
		style.item_font,
		this->calculate_widget_position(0, ongoing ? 1 : 4),
		this->render_resources(),
		look.art.ink);

	this->box_ = std::make_unique<MenuPaperArtwork>(
		RectangleF(Vector2F::ZERO, look.box_size), this->render_resources(),
		&style, &look.art, &this->focus_, this->restart_.get());
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
	this->text_container_->add_child(this->change_teams_.get());
	this->text_container_->add_child(this->change_weapons_.get());
	this->text_container_->add_child(this->change_level_.get());
	this->text_container_->add_child(this->exit_.get());

	const auto choose = [this](EndMenuAction action, bool confirm)
		{
			this->play_wave(confirm ? this->confirm_sound_ : this->cancel_sound_);
			this->finish(action);
		};

	this->change_teams_->set_hidden(ongoing);
	this->change_weapons_->set_hidden(ongoing);
	this->change_level_->set_hidden(ongoing);
	this->focus_.add(this->restart_.get(),
		[choose] { choose(EndMenuAction::restart, true); });
	if (!ongoing)
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
