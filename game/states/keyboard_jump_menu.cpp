#include "game/pch.h"
#include "game/states/keyboard_jump_menu.h"
#include <algorithm>

using namespace labrador;
using namespace mattmath;

KeyboardJumpMenu::KeyboardJumpMenu(MenuContext* context, int player) :
	SettingsPanelPage(context, player),
	capture_input_(context->data()->gamepads(), context->data()->keyboard(), context->data()->tuning())
{
}

KeyboardJumpMenu::~KeyboardJumpMenu()
{
	if (this->capture_active_) { this->data()->debug().end_keyboard_capture(); }
}

void KeyboardJumpMenu::init()
{
	this->input()->reset_actions();
	this->selection_ = this->settings_session()->input_bindings();
	const ControlsSettingsPresentation& look = this->panel_look();
	const MenuStyle style = this->panel_style();
	this->focus_.set_style({ look.art.focus_ink, look.art.ink, style.disabled });
	this->status_ink_ = look.art.focus_ink;
	this->warning_ink_ = this->presentation().main_menu().options_warning_colour;
	this->set_prompts(MenuPrompt::select_back);
	this->build_panel(style);
	const auto& copy = this->text().keyboard_jump;
	this->add_text("heading", copy.heading, style.heading_font, look.heading_position);
	// One capture row per action, showing its pending control. Left/Right
	// change nothing; activating the row starts that action's gesture.
	const Vector2F value_limit(
		menu_geometry::settings_action_width(look, this->calculate_widget_position(1, 0).x),
		look.layout.spacing.y - look.art.focus_padding.y * 2.0f);
	const std::array<const char*, keyboard_action_count> names{ "move_left", "move_right", "jump",
		"drop", "shoot", "pause" };
	for (size_t index = 0; index < keyboard_action_count; ++index)
	{
		const int row = static_cast<int>(index);
		this->rows_[index] = std::make_unique<OptionRow>(names[index], copy.row(index),
			this->calculate_widget_position(0, row), this->calculate_widget_position(1, row), style,
			look.art.ink, this->render_resources(), [](Direction) {}, &look.art, value_limit);
		this->text_container_->add_child(this->rows_[index].get());
		const KeyboardAction action = static_cast<KeyboardAction>(index);
		this->focus_.add(this->rows_[index].get(), [this, action] { this->begin_capture(action); });
		this->actions_.push_back(this->rows_[index].get());
	}
	const auto action = [&](const char* name, const std::wstring& label, int row,
		Button::Action activate)
	{
		const Vector2F position = this->calculate_widget_position(0, row);
		UiText* widget = this->add_text(name, label, style.item_font, position);
		fit_text(*widget, RectangleF(position, Vector2F(
			menu_geometry::settings_action_width(look, position.x),
			look.layout.spacing.y - look.art.focus_padding.y * 2.0f)));
		this->focus_.add(widget, std::move(activate));
		this->actions_.push_back(widget);
	};
	const int last = static_cast<int>(keyboard_action_count);
	action("reset", copy.reset, last, [this] { this->reset_pending(); });
	action("apply", this->text().item(MenuItem::apply), last + 1, [this] { this->apply(); });
	action("back", this->text().item(MenuItem::back), last + 2, [this] { this->close(); });

	this->help_limit_ = RectangleF(look.controls_position, look.controls_size);
	this->status_limit_ = this->status_limit(style);
	this->help_ = this->add_text("help", L"", look.help_font, this->help_limit_.position());
	this->status_ = this->add_text("status", L"", look.help_font, this->status_limit_.position());
	this->refresh_values();
	this->set_status(copy.pending_status);
}

void KeyboardJumpMenu::set_status(const std::wstring& value, bool warning)
{
	this->status_->set_text(value);
	this->status_->set_colour(warning ? this->warning_ink_ : this->status_ink_);
	fit_text(*this->status_, this->status_limit_);
}

bool KeyboardJumpMenu::debug_collision() const
{
	for (size_t index = 0; index < keyboard_action_count; ++index)
	{
		const DesktopControl control = keyboard_control(this->selection_, static_cast<KeyboardAction>(index));
		if (control.key != Key::none && this->data()->debug().keyboard_jump_collision(control.key)) { return true; }
	}
	return false;
}

void KeyboardJumpMenu::refresh_values()
{
	for (size_t index = 0; index < keyboard_action_count; ++index)
	{
		this->rows_[index]->set_value(std::wstring(desktop_control_label(
			keyboard_control(this->selection_, static_cast<KeyboardAction>(index)))));
	}
	// The help text follows only the debug collision flag, so it is rebuilt
	// when that changes.
	const bool collision = this->debug_collision();
	if (this->help_shown_ && collision == this->shown_collision_) { return; }
	const auto& copy = this->text().keyboard_jump;
	std::wstring help = copy.help;
	if (collision) { help += L"\n" + copy.debug_collision; }
	this->help_->set_text(help);
	fit_text(*this->help_, this->help_limit_);
	this->help_shown_ = true;
	this->shown_collision_ = collision;
}

void KeyboardJumpMenu::set_actions_enabled(bool enabled)
{
	for (UiWidget* widget : this->actions_) { this->focus_.set_enabled(widget, enabled); }
}

void KeyboardJumpMenu::begin_capture(KeyboardAction action)
{
	this->input()->reset_actions();
	this->capture_input_.reset_actions();
	this->capture_.reset(this->selection_, action);
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
	if (this->opening_slot() >= 0)
	{
		return this->opening_slot_input(inputs).action == MenuInputAction::back;
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
		this->set_status(copy.release_key(std::wstring(desktop_control_label(this->capture_.candidate_control()))));
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
	else if (this->capture_.phase() == KeyboardJumpCapture::Phase::ready)
	{
		this->set_status(this->capture_.accepts_mouse() ? copy.capture_ready_mouse : copy.capture_ready);
	}
	else { this->set_status(copy.capture_wait_neutral); }
}

void KeyboardJumpMenu::finish_capture(bool accepted)
{
	if (accepted)
	{
		set_keyboard_control(this->selection_, this->capture_.action(), *this->capture_.control_result());
	}
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
	const Mouse& mouse = *this->data()->mouse();
	this->capture_.update(keyboard.state(), keyboard.previous_state(), mouse.state(), mouse.previous_state(),
		this->capture_pad_back());
	if (this->capture_.phase() == KeyboardJumpCapture::Phase::complete) { this->finish_capture(true); }
	else if (this->capture_.phase() == KeyboardJumpCapture::Phase::cancelled) { this->finish_capture(false); }
	else { this->refresh_capture_status(); }
}

void KeyboardJumpMenu::apply()
{
	// Merge only the keyboard choices, so newer pad profiles survive.
	InputBindings current = this->settings_session()->input_bindings();
	for (size_t index = 0; index < keyboard_action_count; ++index)
	{
		const KeyboardAction action = static_cast<KeyboardAction>(index);
		set_keyboard_control(current, action, keyboard_control(this->selection_, action));
	}
	const bool saved = this->settings_session()->set_input_bindings_and_save(current);
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

bool KeyboardJumpMenu::debug_dismiss(bool leaving_match)
{
	if (this->capture_active_) { return false; }
	return SettingsPanelPage::debug_dismiss(leaving_match);
}

void KeyboardJumpMenu::on_suspend()
{
	SettingsPanelPage::on_suspend();
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
	SettingsPanelPage::on_resume();
	this->input()->reset_actions();
	this->refresh_values();
}

void KeyboardJumpMenu::on_deactivated()
{
	SettingsPanelPage::on_deactivated();
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
	if (this->opening_slot() >= 0) { inputs = { this->opening_slot_input(inputs) }; }
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
