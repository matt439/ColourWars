#include "game/pch.h"
#include "game/states/pause_menu.h"

using namespace DirectX;
using namespace mattmath;
using namespace pause_menu_consts;
using namespace menu_consts;
using namespace colour_consts;
using namespace artattack;

PauseMenuPage::PauseMenuPage(MenuContext* context, int player_num) :
	MenuPage(context),
	SoundBankObject(pause_menu_consts::SOUND_BANK,
		this->audio_resources()),
	pausing_player_(player_num)
{
	this->direction_sound_ = this->resolve_wave(DIRECTION_SOUND);
	this->confirm_sound_ = this->resolve_wave(CONFIRM_SOUND);
	this->cancel_sound_ = this->resolve_wave(CANCEL_SOUND);
	this->window_open_sound_ = this->resolve_wave(WINDOW_OPEN_SOUND);
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
	this->focus_.set_style({ STANDARD_HIGHLIGHT, STANDARD_UNHIGHLIGHT });

	this->box_ = std::make_unique<UiTexture>(
		"box",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, PAUSE_MENU_BOX_SIZE),
		this->render_resources(),
		PAUSE_MENU_BOX_COLOUR);
	this->box_->set_position_at_center(DESIGN_RESOLUTION / 2.0f);

	this->set_widget_position(PAUSE_MENU_INITIAL_WIDGET_POSITION);
	this->set_widget_size(PAUSE_MENU_INITIAL_WIDGET_SIZE);
	this->set_widget_spacing(PAUSE_MENU_INITIAL_WIDGET_SPACING);

	this->player_num_ = std::make_unique<UiTextDropShadow>(
		"player_num",
		this->player_number_text(this->pausing_player()),
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		PAUSE_MENU_HEADING_TEXT_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->resume_ = std::make_unique<UiTextDropShadow>(
		"resume",
		L"Resume",
		ITEM_FONT,
		this->calculate_widget_position(1, 2),
		this->render_resources(),
		STANDARD_HIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->restart_ = std::make_unique<UiTextDropShadow>(
		"restart",
		L"Restart",
		ITEM_FONT,
		this->calculate_widget_position(1, 3),
		this->render_resources(),
		STANDARD_UNHIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->quit_ = std::make_unique<UiTextDropShadow>(
		"quit",
		L"Quit",
		ITEM_FONT,
		this->calculate_widget_position(1, 4),
		this->render_resources(),
		STANDARD_UNHIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

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
	this->focus_.set_style({ STANDARD_HIGHLIGHT, STANDARD_UNHIGHLIGHT });

	this->box_ = std::make_unique<UiTexture>(
		"box",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, PAUSE_MENU_BOX_SIZE),
		this->render_resources(),
		PAUSE_MENU_BOX_COLOUR);
	this->box_->set_position_at_center(DESIGN_RESOLUTION / 2.0f);

	this->set_widget_position(PAUSE_MENU_INITIAL_WIDGET_POSITION);
	this->set_widget_size(PAUSE_MENU_INITIAL_WIDGET_SIZE);
	this->set_widget_spacing(PAUSE_MENU_INITIAL_WIDGET_SPACING);

	this->player_num_ = std::make_unique<UiTextDropShadow>(
		"player_num",
		this->player_number_text(this->pausing_player()),
		HEADING_FONT,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		PAUSE_MENU_HEADING_TEXT_COLOUR,
		SHADOW_COLOUR,
		HEADING_SHADOW_OFFSET);

	this->question_ = std::make_unique<UiTextDropShadow>(
		"question",
		question_text(this->type_),
		DETAIL_FONT,
		this->calculate_widget_position(0, 2),
		this->render_resources(),
		PAUSE_MENU_HEADING_TEXT_COLOUR,
		SHADOW_COLOUR,
		DETAIL_SHADOW_OFFSET);

	this->yes_ = std::make_unique<UiTextDropShadow>(
		"yes",
		L"Yes",
		ITEM_FONT,
		this->calculate_widget_position(1, 3),
		this->render_resources(),
		STANDARD_UNHIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);

	this->no_ = std::make_unique<UiTextDropShadow>(
		"no",
		L"No",
		ITEM_FONT,
		this->calculate_widget_position(1, 4),
		this->render_resources(),
		STANDARD_HIGHLIGHT,
		SHADOW_COLOUR,
		ITEM_SHADOW_OFFSET);


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