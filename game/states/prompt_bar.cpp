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

// The whole class. There used to be two hidden flags here - the bar's and the
// text object it owned - and a comment explaining that a page hides the bar
// and never the string. One object, one flag, nothing to explain.
PromptBar::PromptBar(const std::string& name,
	const std::vector<Prompt>& prompts,
	const PromptBarPresentation& look,
	RenderResources* render_resources) :
	UiTextDropShadow(name,
		join(prompts),
		look.font,
		look.position,
		render_resources,
		look.colour,
		look.shadow_colour,
		look.shadow_offset)
{

}
