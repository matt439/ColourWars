#ifndef MENUHIGHLIGHT_H
#define MENUHIGHLIGHT_H

#include "Colour.h"
#include "Mh.h"
#include "MattMath.h"

class MenuHighlight
{
public:
	MenuHighlight() = default;

protected:
	MWidget* get_highlighted_widget() const;
	void set_highlighted_widget(MWidget* widget);
	void highlight_widget(MWidget* widget) const;
	void unhighlight_widget(MWidget* widget) const;
	void change_highlight(MWidget* next_highlight);

	MattMath::Colour get_highlight_colour() const;
	void set_highlight_colour(MattMath::Colour colour);

	MattMath::Colour get_unhighlight_colour() const;
	void set_unhighlight_colour(MattMath::Colour colour);

private:
	MWidget* _highlighted_widget = nullptr;
	MattMath::Colour _highlight_colour = colour_consts::WHITE;
	MattMath::Colour _unhighlight_colour = colour_consts::GRAY;
};

#endif // !MENUPAGEHIGHLIGHT_H
