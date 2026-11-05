#include "game/pch.h"
#include "game/states/menu_focus_motion.h"
#include "game/config/content_value.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

MenuFocusMotion::MenuFocusMotion(const MenuFocusMotionPresentation& look) : look_(look)
{
	using Bounds = MenuFocusMotionPresentation;
	if (!std::isfinite(look.focus_duration) || look.focus_duration <= 0.0f || look.focus_duration > Bounds::max_duration ||
		!std::isfinite(look.press_duration) || look.press_duration <= 0.0f || look.press_duration > Bounds::max_duration ||
		!std::isfinite(look.arrow_travel) || look.arrow_travel < 0.0f || look.arrow_travel > Bounds::max_arrow_travel ||
		!std::isfinite(look.press_fraction) || look.press_fraction < 0.0f || look.press_fraction > 1.0f ||
		!std::isfinite(look.selection_inset_fraction) || look.selection_inset_fraction < 0.0f ||
		look.selection_inset_fraction > Bounds::max_selection_inset_fraction)
	{
		throw std::invalid_argument("Menu focus motion requires durations in (0," +
			content_value::number_text(Bounds::max_duration) + "], arrow travel in [0," +
			content_value::number_text(Bounds::max_arrow_travel) +
			"], press fraction in [0,1] and selection inset fraction in [0," +
			content_value::number_text(Bounds::max_selection_inset_fraction) + "].");
	}
}

void MenuFocusMotion::observe(const labrador::UiWidget* eligible_focus)
{
	if (eligible_focus == this->focus_) { return; }
	const bool moved = this->focus_ != nullptr && eligible_focus != nullptr;
	this->focus_ = eligible_focus;
	this->reset();
	if (moved && !this->reduced_) { this->focus_remaining_ = this->look_.focus_duration; }
}

void MenuFocusMotion::selection_changed()
{
	this->reset();
	if (this->focus_ != nullptr && !this->reduced_)
	{
		this->focus_remaining_ = this->look_.focus_duration;
	}
}

void MenuFocusMotion::update(float dt, bool reduce_effects)
{
	this->reduced_ = reduce_effects;
	if (reduce_effects)
	{
		this->reset();
		return;
	}
	if (!std::isfinite(dt) || dt <= 0.0f) { return; }
	this->focus_remaining_ = std::max(0.0, this->focus_remaining_ - static_cast<double>(dt));
	this->press_remaining_ = std::max(0.0, this->press_remaining_ - static_cast<double>(dt));
}

void MenuFocusMotion::press()
{
	if (this->focus_ != nullptr && !this->reduced_)
	{
		// Repeated accepted presses restart one response; they never stack.
		this->press_remaining_ = this->look_.press_duration;
	}
}

void MenuFocusMotion::reset()
{
	this->focus_remaining_ = 0.0;
	this->press_remaining_ = 0.0;
}

float MenuFocusMotion::arrow_offset() const
{
	const float remaining = static_cast<float>(this->focus_remaining_ / this->look_.focus_duration);
	return this->look_.arrow_travel * remaining * remaining;
}

float MenuFocusMotion::press_fraction() const
{
	return this->look_.press_fraction * static_cast<float>(this->press_remaining_ / this->look_.press_duration);
}

float MenuFocusMotion::selection_inset() const
{
	const float focus = static_cast<float>(this->focus_remaining_ / this->look_.focus_duration);
	return this->look_.selection_inset_fraction *
		std::clamp(std::max(focus * focus, this->press_fraction()), 0.0f, 1.0f);
}
