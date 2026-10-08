#include "game/pch.h"
#include "game/states/prompt_bar.h"

using namespace labrador;
using namespace mattmath;

namespace
{
	// Spacing within a prompt and between prompts in the single text object.
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
