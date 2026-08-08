#include "game/pch.h"
#include "game/states/end_menu.h"

using namespace mattmath;
using namespace colour_consts;
using namespace end_menu_consts;
using namespace artattack;

EndMenuPage::EndMenuPage(EndMenuData* data) :
	MenuPage(data),
	SoundBankObject(SOUND_BANK, this->audio_resources()),
	data_(data)
{
	this->direction_sound_ = this->resolve_wave(DIRECTION_SOUND);
	this->confirm_sound_ = this->resolve_wave(CONFIRM_SOUND);
	this->cancel_sound_ = this->resolve_wave(CANCEL_SOUND);
}

EndMenuData* EndMenuPage::end_menu_data() const
{
	return this->data_;
}

EndMenuInitial::EndMenuInitial(EndMenuData* data) : EndMenuPage(data)
{

}

void EndMenuInitial::update()
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
	this->focus_.set_style({ STANDARD_HIGHLIGHT, STANDARD_UNHIGHLIGHT });

	this->box_ = std::make_unique<UiTexture>(
		"box",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, END_MENU_BOX_SIZE),
		this->render_resources(),
		END_MENU_BOX_COLOUR);
	this->box_->set_position_at_center(DEFAULT_RESOLUTION / 2.0f);

	this->set_widget_position(END_MENU_INITIAL_WIDGET_POSITION);
	this->set_widget_size(END_MENU_INITIAL_WIDGET_SIZE);
	this->set_widget_spacing(END_MENU_INITIAL_WIDGET_SPACING);

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Level End",
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		END_MENU_HEADING_TEXT_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->change_teams_ = std::make_unique<UiTextDropShadow>(
		"change_teams",
		L"Change Teams",
		ITEM_FONT,
		this->calculate_widget_position(0, 2),
		this->render_resources(),
		STANDARD_HIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->change_weapons_ = std::make_unique<UiTextDropShadow>(
		"change_weapons",
		L"Change Weapons",
		ITEM_FONT,
		this->calculate_widget_position(0, 3),
		this->render_resources(),
		STANDARD_UNHIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->change_level_ = std::make_unique<UiTextDropShadow>(
		"change_level",
		L"Change Level",
		ITEM_FONT,
		this->calculate_widget_position(0, 4),
		this->render_resources(),
		STANDARD_UNHIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->restart_ = std::make_unique<UiTextDropShadow>(
		"restart",
		L"Restart",
		ITEM_FONT,
		this->calculate_widget_position(0, 5),
		this->render_resources(),
		STANDARD_UNHIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->exit_ = std::make_unique<UiTextDropShadow>(
		"exit",
		L"Exit to Main Menu",
		ITEM_FONT,
		this->calculate_widget_position(0, 6),
		this->render_resources(),
		STANDARD_UNHIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

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

	const Vector2F resolution = this->float_resolution();

	this->texture_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);
	this->text_container_->scale_objects_to_new_resolution(
		DEFAULT_RESOLUTION, resolution);

	const auto choose = [this](EndMenuAction action, bool confirm)
		{
			this->play_wave(confirm ? this->confirm_sound_ : this->cancel_sound_);
			*this->end_menu_data()->action() = action;
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
void EndMenuInitial::draw()
{
	std::vector<std::pair<UiObject*, ID3D11SamplerState*>> ui_objects;

	ui_objects.push_back(std::make_pair(this->texture_container_.get(),
		this->point_clamp_sampler_state()));

	ui_objects.push_back(std::make_pair(this->text_container_.get(), nullptr));

	this->draw_ui_objects_in_viewports(&ui_objects);
}