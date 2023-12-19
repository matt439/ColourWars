#include "pch.h"
#include "MenuHighlight.h"

using namespace MattMath;

//template <class T>
void MenuHighlight::highlight_widget(MWidget* widget)
{
	if (widget == nullptr)
	{
		return;
	}
	widget->set_colour(this->_highlight_colour);
}

//template <class T>
void MenuHighlight::unhighlight_widget(MWidget* widget)
{
	if (widget == nullptr)
	{
		return;
	}
	widget->set_colour(this->_unhighlight_colour);
}

MWidget* MenuHighlight::get_highlighted_widget()
{
	return this->_highlighted_widget;
}

void MenuHighlight::set_highlighted_widget(MWidget* widget)
{
	this->_highlighted_widget = widget;
}

void MenuHighlight::change_highlight(MWidget* next_highlight)
{
	this->unhighlight_widget(this->get_highlighted_widget());
	this->highlight_widget(next_highlight);
	this->set_highlighted_widget(next_highlight);
}

Colour MenuHighlight::get_highlight_colour()
{
	return this->_highlight_colour;
}
void MenuHighlight::set_highlight_colour(Colour colour)
{
	this->_highlight_colour = colour;
}

Colour MenuHighlight::get_unhighlight_colour()
{
	return this->_unhighlight_colour;
}
void MenuHighlight::set_unhighlight_colour(Colour colour)
{
	this->_unhighlight_colour = colour;
}

//template <class T>
//void MenuPageHighlight::change_highlight(menu_element next_highlight)
//{
//	this->unhighlight_widget(this->get_element_widget(
//		this->highlighted_element));
//	this->highlight_widget(this->get_element_widget(next_highlight));
//	this->highlighted_element = next_highlight;
//}