#ifndef MENUHIGHLIGHT_H
#define MENUHIGHLIGHT_H

//#include "MenuPage.h"
#include "Colour.h"
#include "Mh.h"
#include "MattMath.h"

class MenuHighlight
{
private:
	MWidget* _highlighted_widget = nullptr;
	MattMath::Colour _highlight_colour = colour_consts::WHITE;
	MattMath::Colour _unhighlight_colour = colour_consts::GRAY;
protected:
	MWidget* get_highlighted_widget();
	void set_highlighted_widget(MWidget* widget);
	void highlight_widget(MWidget* widget);
	void unhighlight_widget(MWidget* object);
	void change_highlight(MWidget* next_highlight);

	MattMath::Colour get_highlight_colour();
	void set_highlight_colour(MattMath::Colour colour);

	MattMath::Colour get_unhighlight_colour();
	void set_unhighlight_colour(MattMath::Colour colour);

public:
	//MenuPageHighlight(MenuData* data) : MenuPage(data) {}
	MenuHighlight() {}
	//virtual ~MenuHighlight() {}
	//virtual void update() = 0;
	//virtual void draw() = 0;
	//virtual void init() = 0;
};

#endif // !MENUPAGEHIGHLIGHT_H
