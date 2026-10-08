#pragma once

#include "engine/ui/navigation.h"
#include "engine/ui/widget.h"
#include "game/presentation.h"

#include <functional>
#include <memory>
#include <string>

// One focusable settings row, owning its label, value, and adjustment action.
class OptionRow final : public labrador::UiContainer
{
public:
	// The caller supplies only left or right.
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

	// Focus colours only the label; the value retains its own colour.
	void set_colour(const labrador::Colour& colour) override;

private:
	std::unique_ptr<labrador::UiTextDropShadow> label_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> value_ = nullptr;
	Adjust on_adjust_;
};
