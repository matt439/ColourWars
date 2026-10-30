#include "game/pch.h"
#include "game/states/option_row.h"
#include "game/carried/ui_text_drop_shadow.h"

using namespace labrador;
using namespace mattmath;

OptionRow::OptionRow(const std::string& name,
	const std::wstring& label,
	const Vector2F& label_position,
	const Vector2F& value_position,
	const MenuStyle& style,
	const Colour& value_colour,
	RenderResources* render_resources,
	Adjust on_adjust,
	const MenuPaperArtPresentation* paper_art,
	const Vector2F& value_limit) :
	UiContainer(name),
	value_limit_(value_limit),
	on_adjust_(std::move(on_adjust))
{
	if (paper_art != nullptr)
	{
		auto label_text = std::make_unique<UiText>(name + "_label", label,
			style.item_font, label_position, render_resources, paper_art->ink);
		const RectangleF bounds = label_text->bounds();
		const float label_width = value_position.x - label_position.x - paper_art->focus_padding.x;
		float scale = 1.0f;
		if (label_width > 0.0f && bounds.width > label_width)
		{
			scale = label_width / bounds.width;
		}
		if (value_limit.y > 0.0f && bounds.height * scale > value_limit.y)
		{
			scale = value_limit.y / bounds.height;
		}
		label_text->set_scale(scale);
		this->label_ = std::move(label_text);

		auto value_text = std::make_unique<UiText>(name + "_value", L"null",
			style.item_font, value_position, render_resources, value_colour);
		this->value_text_ = value_text.get();
		this->value_ = std::move(value_text);
	}
	else
	{
		this->label_ = std::make_unique<colourwars::UiTextDropShadow>(
			name + "_label",
			label,
			style.item_font,
			label_position,
			render_resources,
			style.unhighlight,
			style.shadow_colour,
			style.item_shadow_offset);

		// The page must set the displayed value before the first draw.
		auto value_text = std::make_unique<colourwars::UiTextDropShadow>(
			name + "_value",
			L"null",
			style.item_font,
			value_position,
			render_resources,
			value_colour,
			style.shadow_colour,
			style.item_shadow_offset);
		this->value_text_ = value_text.get();
		this->value_ = std::move(value_text);
	}

	this->add_child(this->label_.get());
	this->add_child(this->value_.get());
}

void OptionRow::set_value(const std::wstring& value)
{
	this->value_text_->set_text(value);
	if (this->value_limit_ != Vector2F::ZERO)
	{
		// Refit from the authored font size when a longer or shorter choice appears.
		this->value_text_->set_scale(1.0f);
		const RectangleF bounds = this->value_->bounds();
		float scale = 1.0f;
		if (this->value_limit_.x > 0.0f && bounds.width > this->value_limit_.x)
		{
			scale = this->value_limit_.x / bounds.width;
		}
		if (this->value_limit_.y > 0.0f && bounds.height * scale > this->value_limit_.y)
		{
			scale = this->value_limit_.y / bounds.height;
		}
		this->value_text_->set_scale(scale);
	}
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
