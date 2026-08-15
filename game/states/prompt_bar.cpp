#include "game/pch.h"
#include "game/states/prompt_bar.h"

using namespace labrador;
using namespace mattmath;

namespace
{
	// Between the button and what it does, and between one prompt and the
	// next. Spaces rather than a measured layout: the bar is one text object,
	// so nothing can overlap anything and there is no arithmetic to get wrong.
	//
	// In code rather than in presentation.json on purpose. A gap written as a
	// JSON string is a run of characters nobody can see or count in a diff,
	// and this one is not a number an art pass would want to reach for - the
	// font, the colour and the position, which are, are all data.
	const std::wstring BUTTON_GAP = L" ";
	const std::wstring PROMPT_GAP = L"    ";

	std::wstring join(const std::vector<PromptBar::Prompt>& prompts)
	{
		std::wstring text;
		for (const PromptBar::Prompt& prompt : prompts)
		{
			if (!text.empty())
			{
				text += PROMPT_GAP;
			}
			text += prompt.button + BUTTON_GAP + prompt.action;
		}
		return text;
	}
}

PromptBar::PromptBar(const std::string& name,
	const std::vector<Prompt>& prompts,
	const PromptBarPresentation& look,
	RenderResources* render_resources) :
	UiWidget(name)
{
	this->text_ = std::make_unique<UiTextDropShadow>(
		name + "_text",
		join(prompts),
		look.font,
		look.position,
		render_resources,
		look.colour,
		look.shadow_colour,
		look.shadow_offset);
}

void PromptBar::update(float dt)
{
	this->text_->update(dt);
}

void PromptBar::draw(DrawList& draw_list) const
{
	// The hidden check is this object's own; the text it owns has its own and
	// they are not the same flag. A page hides the bar, never the string.
	if (this->hidden())
	{
		return;
	}

	this->text_->draw(draw_list);
}

RectangleF PromptBar::bounds() const
{
	return this->text_->bounds();
}

void PromptBar::set_colour(const Colour& colour)
{
	this->text_->set_colour(colour);
}
