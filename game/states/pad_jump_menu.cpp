#include "game/pch.h"
#include "game/states/pad_jump_menu.h"
#include "game/objects/crude_drawing.h"
#include <algorithm>

using namespace labrador;
using namespace mattmath;

PadJumpMenu::PadJumpMenu(MenuContext* context, int player) : PauseMenuPage(context, player) {}

void PadJumpMenu::fit_text(UiText& widget, const RectangleF& available)
{
	widget.set_scale(1.0f);
	const RectangleF bounds = widget.bounds();
	if (bounds.width > 0.0f && bounds.height > 0.0f)
	{
		widget.set_scale(std::min({ 1.0f, available.width / bounds.width,
			available.height / bounds.height }));
	}
}

void PadJumpMenu::init()
{
	this->input()->reset_actions();
	this->selection_ = this->save()->input_bindings();
	if (input_slots::is_pad(this->pausing_player())) { this->slot_ = this->pausing_player(); }
	else
	{
		for (int slot = 0; slot < input_slots::pad_count; ++slot)
		{
			if (this->data()->gamepads()->connected(slot)) { this->slot_ = slot; break; }
		}
	}
	const ControlsSettingsPresentation& look = this->presentation().controls_settings();
	MenuStyle style = this->menu_style();
	style.item_font = look.item_font;
	this->set_widget_position(look.layout.position);
	this->set_widget_spacing(look.layout.spacing);
	this->focus_.set_style({ look.art.focus_ink, look.art.ink, style.disabled });
	this->set_prompts(MenuPrompt::sticks);
	this->background_ = std::make_unique<UiTexture>("background", style.sprite_sheet, style.fill_frame,
		RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), this->render_resources(),
		CrudeDrawing::premultiplied(this->presentation().main_menu().home_background_colour));
	this->paper_ = std::make_unique<MenuPaperArtwork>(RectangleF(
		(Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - look.box_size) / 2.0f,
		look.box_size), this->render_resources(), &style, &look.art, &this->focus_, nullptr,
		&this->focus_motion_, &this->save()->play_settings());
	this->texture_container_ = std::make_unique<UiContainer>("textures");
	this->texture_container_->add_child(this->background_.get());
	this->texture_container_->add_child(this->paper_.get());
	this->text_container_ = std::make_unique<UiContainer>("text");
	const auto add_text = [&](const char* name, const std::wstring& value,
		const std::string& font, const Vector2F& position)
	{
		this->text_.push_back(std::make_unique<UiText>(name, value, font, position,
			this->render_resources(), look.art.ink));
		UiText* widget = this->text_.back().get();
		this->text_container_->add_child(widget);
		return widget;
	};
	const auto& copy = this->text().pad_jump;
	add_text("heading", copy.heading, style.heading_font, look.heading_position);
	const Vector2F value_limit(look.controls_position.x - look.art.focus_padding.x * 2.0f -
		look.art.shadow_offset - this->calculate_widget_position(1, 0).x,
		look.layout.spacing.y - look.art.focus_padding.y * 2.0f);
	const auto row = [&](const char* name, const std::wstring& label, int index, OptionRow::Adjust adjust)
	{
		return std::make_unique<OptionRow>(name, label, this->calculate_widget_position(0, index),
			this->calculate_widget_position(1, index), style, look.art.ink, this->render_resources(),
			std::move(adjust), &look.art, value_limit);
	};
	this->pad_ = row("pad", this->text().item(MenuItem::pad), 0, [this](Direction direction)
	{
		this->slot_ = std::clamp(this->slot_ + (direction == Direction::right ? 1 : -1),
			0, input_slots::pad_count - 1);
		this->refresh_values();
	});
	this->jump_ = row("jump", copy.button, 1, [this](Direction direction)
	{
		constexpr std::array<GamepadButton, 4> choices{ GamepadButton::a, GamepadButton::b,
			GamepadButton::x, GamepadButton::y };
		const auto selected = std::find(choices.begin(), choices.end(), this->pending_button());
		const int index = static_cast<int>(std::distance(choices.begin(), selected));
		this->selection_.pad_jump[this->slot_] = choices[std::clamp(index +
			(direction == Direction::right ? 1 : -1), 0, static_cast<int>(choices.size()) - 1)];
		this->edited_[this->slot_] = true;
		this->refresh_values();
		this->set_status(this->text().pad_jump.pending_status);
	});
	for (OptionRow* option : { this->pad_.get(), this->jump_.get() })
	{
		this->text_container_->add_child(option);
		this->focus_.add(option);
	}
	const auto action = [&](const char* name, const std::wstring& label, int index, Button::Action activate)
	{
		const Vector2F position = this->calculate_widget_position(0, index);
		UiText* widget = add_text(name, label, style.item_font, position);
		fit_text(*widget, RectangleF(position, Vector2F(
			look.controls_position.x - look.art.focus_padding.x * 2.0f - look.art.shadow_offset - position.x,
			look.layout.spacing.y - look.art.focus_padding.y * 2.0f)));
		this->focus_.add(widget, std::move(activate));
	};
	action("reset", copy.reset, 2, [this] { this->reset_pending(); });
	action("apply", this->text().item(MenuItem::apply), 3, [this] { this->apply_settings(); });
	action("back", this->text().item(MenuItem::back), 4, [this] { this->close(); });
	this->help_limit_ = RectangleF(look.controls_position, look.controls_size);
	this->status_limit_ = RectangleF(look.status_position, Vector2F(
		this->paper_->rectangle().right() - look.art.shadow_offset - look.art.focus_padding.x - look.status_position.x,
		style.footer_y - look.status_position.y));
	this->help_ = add_text("help", L"", look.help_font, this->help_limit_.position());
	this->status_ = add_text("status", L"", look.help_font, this->status_limit_.position());
	this->refresh_values();
	this->set_status(copy.pending_status);
}

void PadJumpMenu::set_status(const std::wstring& value, bool warning)
{
	this->status_->set_text(value);
	this->status_->set_colour(warning ? this->presentation().main_menu().options_warning_colour :
		this->presentation().controls_settings().art.focus_ink);
	fit_text(*this->status_, this->status_limit_);
}

void PadJumpMenu::refresh_values()
{
	for (int slot = 0; slot < input_slots::pad_count; ++slot)
	{
		if (!this->edited_[slot]) { this->selection_.pad_jump[slot] = this->save()->input_bindings().pad_jump[slot]; }
	}
	this->pad_->set_value(this->text().device_label(this->slot_));
	this->jump_->set_value(std::wstring(input_button_label(this->pending_button())));
	const GamepadButton saved = this->save()->input_bindings().pad_jump[this->slot_];
	const std::array<int, 3> shown{ this->slot_, static_cast<int>(saved), static_cast<int>(this->pending_button()) };
	if (this->shown_help_ != shown)
	{
		this->help_->set_text(this->text().pad_jump.help_block(input_button_label(saved),
			input_button_label(this->pending_button())));
		fit_text(*this->help_, this->help_limit_);
		this->shown_help_ = shown;
	}
}

void PadJumpMenu::reset_pending()
{
	this->selection_.pad_jump[this->slot_] = GamepadButton::a;
	this->edited_[this->slot_] = true;
	this->refresh_values();
	this->set_status(this->text().pad_jump.pending_status);
	this->play_wave(this->confirm_sound_);
}

void PadJumpMenu::apply_settings()
{
	InputBindings applied = this->save()->input_bindings();
	for (int slot = 0; slot < input_slots::pad_count; ++slot)
	{
		if (this->edited_[slot]) { applied.pad_jump[slot] = this->selection_.pad_jump[slot]; }
	}
	const bool saved = this->save()->set_input_bindings_and_save(applied);
	if (saved) { this->edited_.fill(false); }
	this->refresh_values();
	this->play_wave(this->confirm_sound_);
	this->set_status(saved ? this->text().pad_jump.saved_status : this->text().status.active_save_failed, !saved);
}

void PadJumpMenu::close()
{
	this->play_wave(this->cancel_sound_);
	this->input()->reset_actions();
	this->context()->pop();
}

bool PadJumpMenu::debug_dismiss(bool /*leaving_match*/) { this->close(); return true; }

void PadJumpMenu::update_page(float /*dt*/)
{
	if (!this->context()->active()) { return; }
	this->refresh_values();
	std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	if (this->pausing_player() >= 0) { inputs = { this->pausing_player_input(inputs) }; }
	for (const ProcessedMenuInput& input : inputs)
	{
		if (input.action == MenuInputAction::back) { this->close(); return; }
		if (input.action == MenuInputAction::proceed)
		{
			if (this->activate_focus() == Activation::ran) { return; }
		}
		else if (input.direction == Direction::left || input.direction == Direction::right)
		{
			const OptionRow* option = dynamic_cast<const OptionRow*>(this->focus_.focused(0));
			if (option != nullptr && this->focus_.enabled(option))
			{
				option->adjust(input.direction);
				this->press_focus();
				this->play_wave(this->direction_sound_);
			}
		}
		else if (this->focus_.move(0, input.direction)) { this->play_wave(this->direction_sound_); }
	}
}
