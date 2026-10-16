#pragma once

#include "game/presentation.h"

namespace labrador { class UiWidget; }

// Optional ornament motion follows the actual focus identity. It never changes
// a widget or its navigation bounds, and sampling does not advance its clock.
class MenuFocusMotion
{
public:
	explicit MenuFocusMotion(const MenuFocusMotionPresentation& look);

	void observe(const labrador::UiWidget* eligible_focus);
	// A different semantic choice can reuse the same eligible widget.
	void selection_changed();
	void update(float dt, bool reduce_effects);
	void press();
	// Forget interrupted effects, retaining the cursor's current identity.
	void reset();

	// Inward travel in keyline units; depression is a fraction of card shadow.
	float arrow_offset() const;
	float press_fraction() const;
	// Fraction of the selected icon's shorter side, always inside its canvas.
	float selection_inset() const;

private:
	MenuFocusMotionPresentation look_;
	const labrador::UiWidget* focus_ = nullptr;
	double focus_remaining_ = 0.0;
	double press_remaining_ = 0.0;
	bool reduced_ = false;
};
