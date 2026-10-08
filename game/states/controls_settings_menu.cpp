#include "game/pch.h"
#include "game/states/controls_settings_menu.h"
#include <algorithm>

using namespace labrador;
using namespace mattmath;

ControlsSettingsMenu::ControlsSettingsMenu(MenuContext* context, int player) :
	PauseMenuPage(context, player)
{
}

void ControlsSettingsMenu::init()
{
	this->input()->reset_actions();
	this->selection_ = this->save()->play_settings();
	const MenuStyle& style = this->menu_style();
	const ControlsSettingsPresentation& look = this->presentation().controls_settings();
	this->set_widget_position(look.layout.position);
	this->set_widget_spacing(look.layout.spacing);
	this->focus_.set_style({ style.highlight, style.unhighlight, style.disabled });
	this->set_prompts({ { L"Left/Right", L"Change" }, { L"A", L"Select" },
		{ L"B", L"Back (discard unapplied changes)" } });
	this->background_ = std::make_unique<UiTexture>("background", this->presentation().main_menu().backdrop_sheet,
		this->presentation().main_menu().backdrop_frame, RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
		this->render_resources(), this->presentation().main_menu().options_background_colour);
	this->texture_container_ = std::make_unique<UiContainer>("textures");
	this->texture_container_->add_child(this->background_.get());
	this->text_container_ = std::make_unique<UiContainer>("text");
	const auto add_text = [this, &style](const char* name, const std::wstring& value,
		const std::string& font, const Vector2F& position, const Colour& colour)
	{
		this->text_.push_back(std::make_unique<UiTextDropShadow>(name, value, font,
			position, this->render_resources(), colour, style.shadow_colour,
			style.detail_shadow_offset));
		UiTextDropShadow* widget = this->text_.back().get();
		this->text_container_->add_child(widget);
		return widget;
	};
	add_text("heading", L"Controls / audio", style.heading_font,
		look.heading_position, style.heading_colour);
	const auto make_row = [this, &style](const char* name, const wchar_t* label,
		int row, int& value, int minimum, int maximum, int step)
	{
		return std::make_unique<OptionRow>(name, label,
			this->calculate_widget_position(0, row), this->calculate_widget_position(1, row),
			style, this->presentation().main_menu().options_value_colour, this->render_resources(),
			[this, &value, minimum, maximum, step](Direction direction)
			{
				value = std::clamp(value + (direction == Direction::right ? step : -step),
					minimum, maximum);
				this->refresh_values();
				this->status_->set_text(L"Apply to save changes for all players.");
				this->status_->set_colour(this->menu_style().highlight);
			});
	};
	this->aim_ = make_row("aim", L"Stick sensitivity", 0, this->selection_.aim_sensitivity, 1, 5, 1);
	this->music_ = make_row("music", L"Music volume", 1, this->selection_.music_volume, 0, 100, 10);
	this->sfx_ = make_row("sfx", L"SFX volume", 2, this->selection_.sfx_volume, 0, 100, 10);
	for (OptionRow* row : { this->aim_.get(), this->music_.get(), this->sfx_.get() })
	{
		this->text_container_->add_child(row);
		this->focus_.add(row);
	}
	this->refresh_values();
	add_text("aim_help", L"Higher sensitivity needs less stick travel.\nMouse aim follows the cursor; roller aim is fixed.",
		look.help_font, this->calculate_widget_position(0, 3), style.highlight);
	UiTextDropShadow* apply = add_text("apply", L"Apply", style.item_font,
		this->calculate_widget_position(0, 4), style.unhighlight);
	UiTextDropShadow* back = add_text("back", L"Back", style.item_font,
		this->calculate_widget_position(0, 5), style.unhighlight);
	this->status_ = add_text("status", L"Apply to save changes for all players.",
		look.help_font, this->calculate_widget_position(0, 6), style.highlight);
	this->focus_.add(apply, [this] { this->apply_settings(); });
	this->focus_.add(back, [this] { this->close(); });
	add_text("controls", L"CONTROLLER\nMove / drop: left stick\nAim: right stick    Shoot: RT\nJump: A / LT (hold for height)\nPause: Start    Debug: Back / View\nKEYBOARD + MOUSE\nMove: A / D    Drop: S / Down\nAim: mouse    Shoot: left click\nJump: Space (hold for height)\nPause: Esc    Debug: F1\nMENUS\nNavigate: stick / D-pad / arrows\nSelect: A / Enter    Back: B / Esc",
		style.detail_font, look.controls_position, style.highlight);
}

void ControlsSettingsMenu::refresh_values()
{
	const wchar_t* labels[] = { L"1 (low)", L"2", L"3 (normal)", L"4", L"5 (high)" };
	this->aim_->set_value(labels[this->selection_.aim_sensitivity - 1]);
	this->music_->set_value(std::to_wstring(this->selection_.music_volume) + L"%");
	this->sfx_->set_value(std::to_wstring(this->selection_.sfx_volume) + L"%");
}

void ControlsSettingsMenu::apply_settings()
{
	const bool saved = this->save()->set_play_settings_and_save(this->selection_);
	if (this->pausing_player() < 0)
	{
		const PresentationSound& music = this->presentation().main_menu().music;
		this->set_effect_volume(this->resolve_effect(music.name),
			music.volume * this->save()->play_settings().music_gain());
	}
	this->play_wave(this->confirm_sound_);
	this->status_->set_text(saved ? L"Applied and saved for all players." :
		L"Active for this run. Could not save.");
	this->status_->set_colour(saved ? this->menu_style().highlight :
		this->menu_style().warning);
}

void ControlsSettingsMenu::close()
{
	this->play_wave(this->cancel_sound_);
	this->input()->reset_actions();
	this->context()->pop();
}

void ControlsSettingsMenu::update_page(float /*dt*/)
{
	if (!this->context()->active())
	{
		return;
	}
	std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	if (this->pausing_player() >= 0)
	{
		inputs = { this->pausing_player_input(inputs) };
	}
	for (const ProcessedMenuInput& input : inputs)
	{
		if (input.action == MenuInputAction::back)
		{
			this->close();
			return;
		}
		if (input.action == MenuInputAction::proceed)
		{
			if (this->focus_.activate(0) == Activation::ran)
			{
				return;
			}
		}
		else if (input.direction == Direction::left || input.direction == Direction::right)
		{
			const OptionRow* row = dynamic_cast<const OptionRow*>(this->focus_.focused(0));
			if (row != nullptr)
			{
				row->adjust(input.direction);
				this->play_wave(this->direction_sound_);
			}
		}
		else if (this->focus_.move(0, input.direction))
		{
			this->play_wave(this->direction_sound_);
		}
	}
}
