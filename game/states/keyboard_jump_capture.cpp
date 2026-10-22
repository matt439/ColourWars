#include "game/pch.h"
#include "game/states/keyboard_jump_capture.h"
#include "game/input_bindings.h"

using namespace labrador;

void KeyboardJumpCapture::reset()
{
	this->phase_ = Phase::waiting_neutral;
	this->rejection_ = Rejection::none;
	this->candidate_ = Key::none;
	this->result_.reset();
	this->focus_cancelled_ = false;
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
	this->candidate_ = Key::none;
}

void KeyboardJumpCapture::update(const KeyboardState& now,
	const KeyboardState& before, bool cancel_requested)
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
		this->candidate_ = Key::none;
		return;
	}

	int down_count = 0;
	Key sole_key = Key::none;
	for (unsigned int value = 1; value < static_cast<unsigned int>(Key::count); ++value)
	{
		const Key key = static_cast<Key>(value);
		if (now.is_down(key)) { ++down_count; sole_key = key; }
	}

	if (cancel_requested || pressed(now, before, Key::escape))
	{
		this->phase_ = Phase::cancelling;
		this->candidate_ = Key::none;
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
		if (down_count == 1 && sole_key == this->candidate_) { return; }
		if (down_count == 0 && released(now, before, this->candidate_))
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
	if (!pressed(now, before, sole_key))
	{
		// A held key whose press this capture did not observe is entry input.
		this->phase_ = Phase::waiting_neutral;
		return;
	}
	this->focus_cancelled_ = false;
	if (!keyboard_jump_key_supported(sole_key))
	{
		this->reject(Rejection::unsupported);
		return;
	}
	if (keyboard_jump_conflict(sole_key))
	{
		this->reject(Rejection::conflict);
		return;
	}
	this->candidate_ = sole_key;
	this->rejection_ = Rejection::none;
	this->phase_ = Phase::holding;
}
