#include "game/pch.h"
#include "game/states/keyboard_jump_menu.h"
#include "game/objects/crude_drawing.h"
#include <algorithm>

using namespace labrador;
using namespace mattmath;

KeyboardJumpMenu::KeyboardJumpMenu(MenuContext* context, int player) :
	PauseMenuPage(context, player),
	capture_input_(context->data()->gamepads(), context->data()->keyboard(), context->data()->tuning())
{
}

KeyboardJumpMenu::~KeyboardJumpMenu()
{
	if (this->capture_active_) { this->data()->debug().end_keyboard_capture(); }
}

void KeyboardJumpMenu::fit_text(UiText& widget, const RectangleF& available)
{
	widget.set_scale(1.0f);
	const RectangleF bounds = widget.bounds();
	if (bounds.width > 0.0f && bounds.height > 0.0f)
	{
		widget.set_scale(std::min({ 1.0f, available.width / bounds.width,
			available.height / bounds.height }));
	}
}

void KeyboardJumpMenu::init()
{
	this->input()->reset_actions();
	this->selection_ = this->save()->input_bindings();
	const ControlsSettingsPresentation& look = this->presentation().controls_settings();
	MenuStyle style = this->menu_style();
	style.item_font = look.item_font;
	this->set_widget_position(look.layout.position);
	this->set_widget_spacing(look.layout.spacing);
	this->focus_.set_style({ look.art.focus_ink, look.art.ink, style.disabled });
	this->status_ink_ = look.art.focus_ink;
	this->warning_ink_ = this->presentation().main_menu().options_warning_colour;
	this->set_prompts(MenuPrompt::select_back);
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
	const auto& copy = this->text().keyboard_jump;
	add_text("heading", copy.heading, style.heading_font, look.heading_position);
	const auto action = [&](const char* name, const std::wstring& label, int row,
		Button::Action activate)
	{
		const Vector2F position = this->calculate_widget_position(0, row);
		UiText* widget = add_text(name, label, style.item_font, position);
		fit_text(*widget, RectangleF(position, Vector2F(
			look.controls_position.x - look.art.focus_padding.x * 2.0f - look.art.shadow_offset - position.x,
			look.layout.spacing.y - look.art.focus_padding.y * 2.0f)));
		this->focus_.add(widget, std::move(activate));
		this->actions_.push_back(widget);
	};
	action("capture", copy.capture, 0, [this] { this->begin_capture(); });
	action("reset", copy.reset, 1, [this] { this->reset_pending(); });
	action("apply", this->text().item(MenuItem::apply), 2, [this] { this->apply(); });
	action("back", this->text().item(MenuItem::back), 3, [this] { this->close(); });

	this->help_limit_ = RectangleF(look.controls_position, look.controls_size);
	this->status_limit_ = RectangleF(look.status_position, Vector2F(
		this->paper_->rectangle().right() - look.art.shadow_offset - look.art.focus_padding.x - look.status_position.x,
		style.footer_y - look.status_position.y));
	this->help_ = add_text("help", L"", look.help_font, this->help_limit_.position());
	this->status_ = add_text("status", L"", look.help_font, this->status_limit_.position());
	this->refresh_values();
	this->set_status(copy.pending_status);
}

void KeyboardJumpMenu::set_status(const std::wstring& value, bool warning)
{
	this->status_->set_text(value);
	this->status_->set_colour(warning ? this->warning_ink_ : this->status_ink_);
	fit_text(*this->status_, this->status_limit_);
}

void KeyboardJumpMenu::refresh_values()
{
	const auto& copy = this->text().keyboard_jump;
	std::wstring help = copy.help_block(std::wstring(input_key_label(this->save()->input_bindings().keyboard_jump)),
		std::wstring(input_key_label(this->selection_.keyboard_jump)));
	if (this->data()->debug().keyboard_jump_collision(this->selection_.keyboard_jump))
	{
		help += L"\n" + copy.debug_collision;
	}
	this->help_->set_text(help);
	fit_text(*this->help_, this->help_limit_);
}

void KeyboardJumpMenu::set_actions_enabled(bool enabled)
{
	for (UiWidget* widget : this->actions_) { this->focus_.set_enabled(widget, enabled); }
}

void KeyboardJumpMenu::begin_capture()
{
	this->input()->reset_actions();
	this->capture_input_.reset_actions();
	this->capture_.reset();
	this->data()->debug().begin_keyboard_capture();
	this->capture_active_ = true;
	this->set_actions_enabled(false);
	this->set_prompts(std::vector<PromptBar::Prompt>{});
	this->reset_menu_motion();
	this->play_wave(this->confirm_sound_);
	this->refresh_capture_status();
}

bool KeyboardJumpMenu::capture_pad_back()
{
	std::vector<ProcessedMenuInput> inputs = this->capture_input_.menu_inputs();
	for (size_t slot = 0; slot < inputs.size(); ++slot)
	{
		ProcessedMenuInput& input = inputs[slot];
		input.direction = Direction::none;
		if (!input_slots::is_pad(static_cast<int>(slot)) || input.action != MenuInputAction::back)
		{
			input.action = MenuInputAction::none;
		}
	}
	if (this->pausing_player() >= 0)
	{
		return this->pausing_player_input(inputs).action == MenuInputAction::back;
	}
	return std::any_of(inputs.begin(), inputs.end(), [](const ProcessedMenuInput& input)
	{
		return input.action == MenuInputAction::back;
	});
}

void KeyboardJumpMenu::refresh_capture_status()
{
	const auto& copy = this->text().keyboard_jump;
	if (this->capture_.phase() == KeyboardJumpCapture::Phase::holding)
	{
		this->set_status(copy.release_key(std::wstring(input_key_label(this->capture_.candidate()))));
	}
	else if (this->capture_.phase() == KeyboardJumpCapture::Phase::cancelling)
	{
		this->set_status(copy.capture_wait_neutral);
	}
	else if (this->capture_.rejection() != KeyboardJumpCapture::Rejection::none)
	{
		this->set_status(this->capture_.rejection() == KeyboardJumpCapture::Rejection::conflict ?
			copy.capture_conflict : copy.capture_rejected, true);
	}
	else if (this->capture_.focus_cancelled()) { this->set_status(copy.capture_cancelled, true); }
	else
	{
		this->set_status(this->capture_.phase() == KeyboardJumpCapture::Phase::ready ?
			copy.capture_ready : copy.capture_wait_neutral);
	}
}

void KeyboardJumpMenu::finish_capture(bool accepted)
{
	if (accepted) { this->selection_.keyboard_jump = *this->capture_.result(); }
	this->data()->debug().end_keyboard_capture();
	this->capture_active_ = false;
	this->input()->reset_actions();
	this->capture_input_.reset_actions();
	this->set_actions_enabled(true);
	this->set_prompts(MenuPrompt::select_back);
	this->reset_menu_motion();
	this->refresh_values();
	this->play_wave(accepted ? this->confirm_sound_ : this->cancel_sound_);
	this->set_status(accepted ? this->text().keyboard_jump.pending_status :
		this->text().keyboard_jump.capture_cancelled);
}

void KeyboardJumpMenu::update_capture()
{
	const Keyboard& keyboard = *this->data()->keyboard();
	this->capture_.update(keyboard.state(), keyboard.previous_state(),
		this->data()->debug().capture_blocked_key_down(), this->capture_pad_back());
	if (this->capture_.phase() == KeyboardJumpCapture::Phase::complete) { this->finish_capture(true); }
	else if (this->capture_.phase() == KeyboardJumpCapture::Phase::cancelled) { this->finish_capture(false); }
	else { this->refresh_capture_status(); }
}

void KeyboardJumpMenu::apply()
{
	const bool saved = this->save()->set_input_bindings_and_save(this->selection_);
	this->refresh_values();
	this->play_wave(this->confirm_sound_);
	this->set_status(saved ? this->text().keyboard_jump.saved_status :
		this->text().status.active_save_failed, !saved);
}

void KeyboardJumpMenu::reset_pending()
{
	this->selection_ = InputBindings{};
	this->refresh_values();
	this->play_wave(this->confirm_sound_);
	this->set_status(this->text().keyboard_jump.pending_status);
}

void KeyboardJumpMenu::close()
{
	this->play_wave(this->cancel_sound_);
	this->input()->reset_actions();
	this->context()->pop();
}

bool KeyboardJumpMenu::debug_dismiss(bool /*leaving_match*/)
{
	if (this->capture_active_) { return false; }
	this->close();
	return true;
}

void KeyboardJumpMenu::on_suspend()
{
	PauseMenuPage::on_suspend();
	this->input()->reset_actions();
	this->capture_input_.reset_actions();
	if (this->capture_active_)
	{
		this->capture_.focus_lost();
		this->refresh_capture_status();
	}
}

void KeyboardJumpMenu::on_resume()
{
	PauseMenuPage::on_resume();
	this->input()->reset_actions();
	this->refresh_values();
}

void KeyboardJumpMenu::on_deactivated()
{
	PauseMenuPage::on_deactivated();
	this->input()->reset_actions();
	this->capture_input_.reset_actions();
	if (this->capture_active_)
	{
		this->capture_.focus_lost();
		this->refresh_capture_status();
	}
}

void KeyboardJumpMenu::update_page(float /*dt*/)
{
	if (!this->context()->active()) { return; }
	if (this->capture_active_) { this->update_capture(); return; }
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
		else if (this->focus_.move(0, input.direction)) { this->play_wave(this->direction_sound_); }
	}
}
