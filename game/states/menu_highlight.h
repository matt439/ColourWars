#pragma once

#include "engine/math/colour.h"
#include "engine/ui/widget.h"
#include "engine/math/matt_math.h"

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
	MWidget* highlighted_widget_ = nullptr;
	MattMath::Colour highlight_colour_ = colour_consts::WHITE;
	MattMath::Colour unhighlight_colour_ = colour_consts::GRAY;
};
