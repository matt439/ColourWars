#include "game/pch.h"
#include "game/states/end_menu.h"

using namespace mattmath;
using namespace menu_consts;
using namespace artattack;

// The end menu makes no sound of its own: the three MenuPage resolves are the
// three it plays, which is why this is empty rather than gone.
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
	this->context()->pop(action);
}

EndMenuInitial::EndMenuInitial(MenuContext* context) : EndMenuPage(context)
{

}

void EndMenuInitial::update(float /*dt*/)
{
	for (const ProcessedMenuInput& input : this->menu_inputs())
	{
		if (input.action == MenuInputAction::proceed)
		{
			if (this->focus_.activate(0))
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
	this->set_prompts({
		{ L"A", L"Select" } });

	const MenuStyle& style = this->menu_style();
	const EndMenuPresentation& look = this->look();

	this->focus_.set_style({ style.highlight, style.unhighlight });

	this->box_ = std::make_unique<UiTexture>(
		"box",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, style.box_size),
		this->render_resources(),
		look.box_colour);
	this->box_->set_position_at_center(DESIGN_RESOLUTION / 2.0f);

	this->set_widget_position(look.layout.position);
	this->set_widget_size(look.layout.size);
	this->set_widget_spacing(look.layout.spacing);

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Level End",
		style.heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		style.heading_colour,
		style.shadow_colour,
		style.heading_shadow_offset);

	this->change_teams_ = std::make_unique<UiTextDropShadow>(
		"change_teams",
		L"Change Teams",
		style.item_font,
		this->calculate_widget_position(0, 2),
		this->render_resources(),
		style.highlight,
		style.shadow_colour,
		style.item_shadow_offset);

	this->change_weapons_ = std::make_unique<UiTextDropShadow>(
		"change_weapons",
		L"Change Weapons",
		style.item_font,
		this->calculate_widget_position(0, 3),
		this->render_resources(),
		style.unhighlight,
		style.shadow_colour,
		style.item_shadow_offset);

	this->change_level_ = std::make_unique<UiTextDropShadow>(
		"change_level",
		L"Change Level",
		style.item_font,
		this->calculate_widget_position(0, 4),
		this->render_resources(),
		style.unhighlight,
		style.shadow_colour,
		style.item_shadow_offset);

	this->restart_ = std::make_unique<UiTextDropShadow>(
		"restart",
		L"Restart",
		style.item_font,
		this->calculate_widget_position(0, 5),
		this->render_resources(),
		style.unhighlight,
		style.shadow_colour,
		style.item_shadow_offset);

	this->exit_ = std::make_unique<UiTextDropShadow>(
		"exit",
		L"Exit to Main Menu",
		style.item_font,
		this->calculate_widget_position(0, 6),
		this->render_resources(),
		style.unhighlight,
		style.shadow_colour,
		style.item_shadow_offset);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->box_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->change_teams_.get());
	this->text_container_->add_child(this->change_weapons_.get());
	this->text_container_->add_child(this->change_level_.get());
	this->text_container_->add_child(this->restart_.get());
	this->text_container_->add_child(this->exit_.get());

	const auto choose = [this](EndMenuAction action, bool confirm)
		{
			this->play_wave(confirm ? this->confirm_sound_ : this->cancel_sound_);
			this->finish(action);
		};

	this->focus_.add(this->change_teams_.get(),
		[choose] { choose(EndMenuAction::change_teams, true); });
	this->focus_.add(this->change_weapons_.get(),
		[choose] { choose(EndMenuAction::change_weapons, true); });
	this->focus_.add(this->change_level_.get(),
		[choose] { choose(EndMenuAction::change_level, true); });
	this->focus_.add(this->restart_.get(),
		[choose] { choose(EndMenuAction::restart, true); });
	this->focus_.add(this->exit_.get(),
		[choose] { choose(EndMenuAction::exit, false); });
}
