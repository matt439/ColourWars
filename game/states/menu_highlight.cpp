#include "game/pch.h"
#include "game/states/menu_highlight.h"

using namespace mattmath;

void MenuHighlight::highlight_widget(MWidget* widget) const
{
	if (widget == nullptr)
	{
		return;
	}
	widget->set_colour(this->highlight_colour_);
}

void MenuHighlight::unhighlight_widget(MWidget* widget) const
{
	if (widget == nullptr)
	{
		return;
	}
	widget->set_colour(this->unhighlight_colour_);
}

MWidget* MenuHighlight::highlighted_widget() const
{
	return this->highlighted_widget_;
}

void MenuHighlight::set_highlighted_widget(MWidget* widget)
{
	this->highlighted_widget_ = widget;
}

void MenuHighlight::change_highlight(MWidget* next_highlight)
{
	this->unhighlight_widget(this->highlighted_widget());
	this->highlight_widget(next_highlight);
	this->set_highlighted_widget(next_highlight);
}

Colour MenuHighlight::highlight_colour() const
{
	return this->highlight_colour_;
}
void MenuHighlight::set_highlight_colour(Colour colour)
{
	this->highlight_colour_ = colour;
}

Colour MenuHighlight::unhighlight_colour() const
{
	return this->unhighlight_colour_;
}
void MenuHighlight::set_unhighlight_colour(Colour colour)
{
	this->unhighlight_colour_ = colour;
}
