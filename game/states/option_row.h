#pragma once

#include "engine/ui/navigation.h"
#include "engine/ui/widget.h"
#include "game/presentation.h"

#include <functional>
#include <memory>
#include <string>

// A settings row: a label on the left, the value it currently holds on the
// right, and what left and right do to it.
//
// ONE FOCUSABLE THING RATHER THAN TWO WIDGETS AND A LOOKUP. The options page
// used to register the label alone and keep the value beside it as a separate
// widget nothing pointed at, so changing a value meant recovering which value
// belonged to the focused label - a chain of pointer comparisons, one arm per
// row, that every new setting would have had to lengthen. The row owns its own
// value and its own meaning now, and the page asks the focused widget whether
// it is one of these at all.
//
// It could not exist until 93821c7: UiContainer was final and was not a
// UiWidget, so it could not be derived from and could not be handed to
// FocusGroup::add. That was report 4's larger half, and this is what it was
// filed for.
class OptionRow final : public labrador::UiContainer
{
public:
	// What left or right on this row does. Called with the direction, which is
	// only ever left or right - the page filters, because a row that has to
	// know which directions are its own is a row that can disagree with the
	// page about it.
	using Adjust = std::function<void(labrador::Direction)>;

	OptionRow(const std::string& name,
		const std::wstring& label,
		const mattmath::Vector2F& label_position,
		const mattmath::Vector2F& value_position,
		const MenuStyle& style,
		const labrador::Colour& value_colour,
		labrador::RenderResources* render_resources,
		Adjust on_adjust);

	void set_value(const std::wstring& value);
	void adjust(labrador::Direction direction) const;

	// THE LABEL ONLY, which is the one thing this overrides its base to do.
	//
	// UiContainer::set_colour reaches every child, and FocusGroup calls it on
	// every repaint - so inheriting it would paint the value in the focus
	// colour and lose the colour that tells a value from a label. The value
	// keeps its own; the label carries the highlight, exactly as when these
	// were two loose widgets.
	void set_colour(const labrador::Colour& colour) override;

private:
	std::unique_ptr<labrador::UiTextDropShadow> label_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> value_ = nullptr;
	Adjust on_adjust_;
};
