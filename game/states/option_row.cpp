#include "game/pch.h"
#include "game/states/option_row.h"

using namespace labrador;
using namespace mattmath;

OptionRow::OptionRow(const std::string& name,
	const std::wstring& label,
	const Vector2F& label_position,
	const Vector2F& value_position,
	const MenuStyle& style,
	const Colour& value_colour,
	RenderResources* render_resources,
	Adjust on_adjust) :
	UiContainer(name),
	on_adjust_(std::move(on_adjust))
{
	this->label_ = std::make_unique<UiTextDropShadow>(
		name + "_label",
		label,
		style.item_font,
		label_position,
		render_resources,
		style.unhighlight,
		style.shadow_colour,
		style.item_shadow_offset);

	// The page must set the displayed value before the first draw.
	this->value_ = std::make_unique<UiTextDropShadow>(
		name + "_value",
		L"null",
		style.item_font,
		value_position,
		render_resources,
		value_colour,
		style.shadow_colour,
		style.item_shadow_offset);

	this->add_child(this->label_.get());
	this->add_child(this->value_.get());
}

void OptionRow::set_value(const std::wstring& value)
{
	this->value_->set_text(value);
}

void OptionRow::adjust(Direction direction) const
{
	if (this->on_adjust_)
	{
		this->on_adjust_(direction);
	}
}

void OptionRow::set_colour(const Colour& colour)
{
	this->label_->set_colour(colour);
}
