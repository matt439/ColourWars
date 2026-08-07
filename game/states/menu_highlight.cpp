#include "game/pch.h"
#include "game/states/menu_highlight.h"

using namespace MattMath;

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

MWidget* MenuHighlight::get_highlighted_widget() const
{
	return this->highlighted_widget_;
}

void MenuHighlight::set_highlighted_widget(MWidget* widget)
{
	this->highlighted_widget_ = widget;
}

void MenuHighlight::change_highlight(MWidget* next_highlight)
{
	this->unhighlight_widget(this->get_highlighted_widget());
	this->highlight_widget(next_highlight);
	this->set_highlighted_widget(next_highlight);
}

Colour MenuHighlight::get_highlight_colour() const
{
	return this->highlight_colour_;
}
void MenuHighlight::set_highlight_colour(Colour colour)
{
	this->highlight_colour_ = colour;
}

Colour MenuHighlight::get_unhighlight_colour() const
{
	return this->unhighlight_colour_;
}
void MenuHighlight::set_unhighlight_colour(Colour colour)
{
	this->unhighlight_colour_ = colour;
}
