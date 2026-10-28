#include "game/pch.h"
#include "game/states/keyboard_jump_capture.h"

using namespace labrador;

namespace
{
	bool control_pressed(const DesktopControl& control, const KeyboardState& now, const KeyboardState& before,
		const MouseState& mouse_now, const MouseState& mouse_before)
	{
		return control.button != MouseButton::none ? pressed(mouse_now, mouse_before, control.button) :
			pressed(now, before, control.key);
	}

	bool control_released(const DesktopControl& control, const KeyboardState& now, const KeyboardState& before,
		const MouseState& mouse_now, const MouseState& mouse_before)
	{
		return control.button != MouseButton::none ? released(mouse_now, mouse_before, control.button) :
			released(now, before, control.key);
	}
}

void KeyboardJumpCapture::reset(const InputBindings& pending, KeyboardAction action)
{
	this->pending_ = pending;
	this->action_ = action;
	this->reset();
}

void KeyboardJumpCapture::reset()
{
	this->phase_ = Phase::waiting_neutral;
	this->rejection_ = Rejection::none;
	this->candidate_ = {};
	this->result_.reset();
	this->focus_cancelled_ = false;
}

std::optional<Key> KeyboardJumpCapture::result() const
{
	if (!this->result_ || this->result_->key == Key::none) { return std::nullopt; }
	return this->result_->key;
}

void KeyboardJumpCapture::focus_lost()
{
	this->reset();
	this->focus_cancelled_ = true;
}

void KeyboardJumpCapture::reject(Rejection reason)
{
	this->phase_ = Phase::waiting_neutral;
	this->rejection_ = reason;
	this->candidate_ = {};
}

void KeyboardJumpCapture::update(const KeyboardState& now,
	const KeyboardState& before, bool cancel_requested)
{
	// An unfocused mouse contributes no buttons.
	this->update(now, before, MouseState{}, MouseState{}, cancel_requested);
}

void KeyboardJumpCapture::update(const KeyboardState& now, const KeyboardState& before,
	const MouseState& mouse_now, const MouseState& mouse_before, bool cancel_requested)
{
	if (this->phase_ == Phase::complete || this->phase_ == Phase::cancelled) { return; }
	// Focus loss is not a release. The first focused frame cannot establish a
	// fresh gesture either; require a subsequent observed neutral frame.
	if (!now.focused)
	{
		this->focus_lost();
		return;
	}
	if (!before.focused)
	{
		this->phase_ = Phase::waiting_neutral;
		this->candidate_ = {};
		return;
	}

	int down_count = 0;
	DesktopControl sole;
	for (unsigned int value = 1; value < static_cast<unsigned int>(Key::count); ++value)
	{
		const Key key = static_cast<Key>(value);
		if (now.is_down(key)) { ++down_count; sole = { key, MouseButton::none }; }
	}
	if (this->accepts_mouse() && mouse_now.focused)
	{
		for (unsigned int value = 1; value < static_cast<unsigned int>(MouseButton::count); ++value)
		{
			const MouseButton button = static_cast<MouseButton>(value);
			if (mouse_now.is_down(button)) { ++down_count; sole = { Key::none, button }; }
		}
	}

	if (cancel_requested || pressed(now, before, Key::escape))
	{
		this->phase_ = Phase::cancelling;
		this->candidate_ = {};
	}
	if (this->phase_ == Phase::cancelling)
	{
		if (down_count == 0) { this->phase_ = Phase::cancelled; }
		return;
	}
	if (this->phase_ == Phase::waiting_neutral)
	{
		if (down_count == 0) { this->phase_ = Phase::ready; }
		return;
	}
	if (this->phase_ == Phase::holding)
	{
		if (down_count == 1 && sole == this->candidate_) { return; }
		if (down_count == 0 && control_released(this->candidate_, now, before, mouse_now, mouse_before))
		{
			this->result_ = this->candidate_;
			this->phase_ = Phase::complete;
			return;
		}
		this->reject(Rejection::ambiguous);
		return;
	}
	if (down_count == 0) { return; }
	if (down_count != 1)
	{
		this->reject(Rejection::ambiguous);
		return;
	}
	if (!control_pressed(sole, now, before, mouse_now, mouse_before))
	{
		// A held control whose press this capture did not observe is entry input.
		this->phase_ = Phase::waiting_neutral;
		return;
	}
	this->focus_cancelled_ = false;
	if (!keyboard_control_supported(this->action_, sole))
	{
		this->reject(Rejection::unsupported);
		return;
	}
	if (keyboard_control_conflict(this->pending_, this->action_, sole))
	{
		this->reject(Rejection::conflict);
		return;
	}
	this->candidate_ = sole;
	this->rejection_ = Rejection::none;
	this->phase_ = Phase::holding;
}
