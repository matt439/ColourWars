#pragma once

#include "engine/math/colour.h"
#include "engine/ui/widget.h"
#include "engine/math/matt_math.h"

class MenuHighlight
{
public:
	MenuHighlight() = default;

protected:
	artattack::UiWidget* highlighted_widget() const;
	void set_highlighted_widget(artattack::UiWidget* widget);
	void highlight_widget(artattack::UiWidget* widget) const;
	void unhighlight_widget(artattack::UiWidget* widget) const;
	void change_highlight(artattack::UiWidget* next_highlight);

	mattmath::Colour highlight_colour() const;
	void set_highlight_colour(mattmath::Colour colour);

	mattmath::Colour unhighlight_colour() const;
	void set_unhighlight_colour(mattmath::Colour colour);

private:
	artattack::UiWidget* highlighted_widget_ = nullptr;
	mattmath::Colour highlight_colour_ = colour_consts::WHITE;
	mattmath::Colour unhighlight_colour_ = colour_consts::GRAY;
};
