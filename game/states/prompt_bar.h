#pragma once

#include "engine/ui/widget.h"
#include "game/presentation.h"

#include <memory>
#include <string>
#include <vector>

// The line along the bottom of a menu page saying what the buttons do.
//
// Nothing in this game has ever said which button does anything. Of the wide
// literals in the whole shell exactly one named a control - "Press A to
// proceed", on the results screen - and the scheme is not guessable from
// looking: team select, weapon select and stage select all respond to left and
// right and say so nowhere.
//
// A UiTextDropShadow, which is what it always was. It used to compose one and
// forward update, draw, bounds and set_colour to it, because every leaf in
// engine/ui was final; that was the exhibit report 4 was filed with, and the
// engine dropped final in 93821c7. So the four forwarders are gone and this is
// a constructor.
//
// It takes the prompts apart rather than as one finished string, even though it
// joins them immediately. There is no button-glyph art in the sheet and every
// font in the tree is 95 ASCII characters, so today a prompt can only be the
// button's name in text; the day there is art, this is the one place that
// changes and the twelve pages that fill it do not.
class PromptBar final : public labrador::UiTextDropShadow
{
public:
	// One entry: a button, and what it does on this page.
	struct Prompt
	{
		std::wstring button;
		std::wstring action;
	};

	PromptBar(const std::string& name,
		const std::vector<Prompt>& prompts,
		const PromptBarPresentation& look,
		labrador::RenderResources* render_resources);
};
