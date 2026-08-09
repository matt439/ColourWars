#include "game/pch.h"
#include "game/states/pause_menu.h"

using namespace DirectX;
using namespace mattmath;
using namespace menu_consts;
using namespace artattack;

PauseMenuPage::PauseMenuPage(MenuContext* context, int player_num) :
	MenuPage(context),
	SoundBankObject(this->menu_style().sound_bank,
		this->audio_resources()),
	pausing_player_(player_num)
{
	const MenuStyle& style = this->menu_style();
	this->direction_sound_ = this->resolve_wave(style.direction_sound);
	this->confirm_sound_ = this->resolve_wave(style.confirm_sound);
	this->cancel_sound_ = this->resolve_wave(style.cancel_sound);
	this->window_open_sound_ =
		this->resolve_wave(this->look().window_open_sound);
}

const PauseMenuPresentation& PauseMenuPage::look() const
{
	return this->presentation().pause_menu();
}

std::wstring PauseMenuPage::player_number_text(int player_num)
{
	switch (player_num)
	{
	case 0:
		return L"Pause Menu P1";
	case 1:
		return L"Pause Menu P2";
	case 2:
		return L"Pause Menu P3";
	case 3:
		return L"Pause Menu P4";
	default:
		return L"Pause Menu P error";
	};
}

int PauseMenuPage::pausing_player() const
{
	return this->pausing_player_;
}

void PauseMenuPage::finish(PauseMenuAction action) const
{
	this->context()->pop(action);
}

ProcessedMenuInput PauseMenuPage::pausing_player_input(
	const std::vector<ProcessedMenuInput>& inputs) const
{
	if (this->pausing_player_ < 0 ||
		static_cast<size_t>(this->pausing_player_) >= inputs.size())
	{
		return ProcessedMenuInput();
	}
	return inputs[static_cast<size_t>(this->pausing_player_)];
}

PauseMenuInitial::PauseMenuInitial(MenuContext* context, int player_num) :
	PauseMenuPage(context, player_num)
{

}

void PauseMenuInitial::update(float /*dt*/)
{
	const ProcessedMenuInput input =
		this->pausing_player_input(this->menu_inputs());

	if (input.action == MenuInputAction::back ||
		input.action == MenuInputAction::pause)
	{
		this->play_wave(this->cancel_sound_);
		this->finish(PauseMenuAction::resume);
	}
	else if (input.action == MenuInputAction::proceed)
	{
		this->focus_.activate(0);
	}
	else if (this->focus_.move(0, input.direction))
	{
		this->play_wave(this->direction_sound_);
	}
}
void PauseMenuInitial::init()
{
	const MenuStyle& style = this->menu_style();
	const PauseMenuPresentation& look = this->look();

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

	this->player_num_ = std::make_unique<UiTextDropShadow>(
		"player_num",
		this->player_number_text(this->pausing_player()),
		style.heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		style.heading_colour,
		style.shadow_colour,
		style.heading_shadow_offset);

	this->resume_ = std::make_unique<UiTextDropShadow>(
		"resume",
		L"Resume",
		style.item_font,
		this->calculate_widget_position(1, 2),
		this->render_resources(),
		style.highlight,
		style.shadow_colour,
		style.item_shadow_offset);

	this->restart_ = std::make_unique<UiTextDropShadow>(
		"restart",
		L"Restart",
		style.item_font,
		this->calculate_widget_position(1, 3),
		this->render_resources(),
		style.unhighlight,
		style.shadow_colour,
		style.item_shadow_offset);

	this->quit_ = std::make_unique<UiTextDropShadow>(
		"quit",
		L"Quit",
		style.item_font,
		this->calculate_widget_position(1, 4),
		this->render_resources(),
		style.unhighlight,
		style.shadow_colour,
		style.item_shadow_offset);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->box_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->player_num_.get());
	this->text_container_->add_child(this->resume_.get());
	this->text_container_->add_child(this->restart_.get());
	this->text_container_->add_child(this->quit_.get());

	this->focus_.add(this->resume_.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			this->finish(PauseMenuAction::resume);
		});
	this->focus_.add(this->restart_.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			this->context()->transition_to(std::make_unique<PauseMenuConfirmation>(
				this->menu_context(), this->pausing_player(),
				ConfirmationType::restart));
		});
	this->focus_.add(this->quit_.get(), [this]
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(std::make_unique<PauseMenuConfirmation>(
				this->menu_context(), this->pausing_player(),
				ConfirmationType::quit));
		});

	this->play_wave(this->window_open_sound_);
}
void PauseMenuInitial::draw(Renderer& renderer) const
{
	std::vector<UiLayer> layers;

	layers.push_back({ this->texture_container_.get(), TextureFilter::point });

	layers.push_back({ this->text_container_.get(), TextureFilter::linear });

	this->draw_ui_layers(renderer, layers);
}

PauseMenuConfirmation::PauseMenuConfirmation(MenuContext* context,
	int player_num, ConfirmationType type) :
	PauseMenuPage(context, player_num), type_(type)
{

}

void PauseMenuConfirmation::update(float /*dt*/)
{
	const ProcessedMenuInput input =
		this->pausing_player_input(this->menu_inputs());

	if (input.action == MenuInputAction::back)
	{
		this->play_wave(this->cancel_sound_);
		this->context()->transition_to(std::make_unique<PauseMenuInitial>(
			this->menu_context(), this->pausing_player()));
		return;
	}
	if (input.action == MenuInputAction::proceed)
	{
		this->focus_.activate(0);
	}
	else if (this->focus_.move(0, input.direction))
	{
		this->play_wave(this->direction_sound_);
	}
}

void PauseMenuConfirmation::init()
{
	const MenuStyle& style = this->menu_style();
	const PauseMenuPresentation& look = this->look();

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

	this->player_num_ = std::make_unique<UiTextDropShadow>(
		"player_num",
		this->player_number_text(this->pausing_player()),
		style.heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		style.heading_colour,
		style.shadow_colour,
		style.heading_shadow_offset);

	this->question_ = std::make_unique<UiTextDropShadow>(
		"question",
		question_text(this->type_),
		style.detail_font,
		this->calculate_widget_position(0, 2),
		this->render_resources(),
		style.heading_colour,
		style.shadow_colour,
		style.detail_shadow_offset);

	this->yes_ = std::make_unique<UiTextDropShadow>(
		"yes",
		L"Yes",
		style.item_font,
		this->calculate_widget_position(1, 3),
		this->render_resources(),
		style.unhighlight,
		style.shadow_colour,
		style.item_shadow_offset);

	this->no_ = std::make_unique<UiTextDropShadow>(
		"no",
		L"No",
		style.item_font,
		this->calculate_widget_position(1, 4),
		this->render_resources(),
		style.highlight,
		style.shadow_colour,
		style.item_shadow_offset);


	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->box_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->player_num_.get());
	this->text_container_->add_child(this->question_.get());
	this->text_container_->add_child(this->yes_.get());
	this->text_container_->add_child(this->no_.get());

	// "No" first, because the first widget added takes the focus and a
	// confirmation dialogue defaults to the safe answer. Adding it first also
	// says so, where set_highlighted_widget(no_) at the bottom of a
	// hundred-line init did not.
	this->focus_.add(this->no_.get(), [this]
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(std::make_unique<PauseMenuInitial>(
				this->menu_context(), this->pausing_player()));
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

void PauseMenuConfirmation::draw(Renderer& renderer) const
{
	std::vector<UiLayer> layers;

	layers.push_back({ this->texture_container_.get(), TextureFilter::point });

	layers.push_back({ this->text_container_.get(), TextureFilter::linear });

	this->draw_ui_layers(renderer, layers);
}

std::wstring PauseMenuConfirmation::question_text(
	ConfirmationType type)
{
	switch (type)
	{
	case ConfirmationType::restart:
		return L"Are you sure you want to restart?";
	case ConfirmationType::quit:
		return L"Are you sure you want to quit?";
	default:
		return L"Error";
	}
}