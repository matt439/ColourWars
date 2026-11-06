#include "game/pch.h"
#include "game/states/prompt_bar.h"
#include "game/carried/ui_text_drop_shadow.h"

using namespace labrador;

namespace
{
	// Spacing within a prompt and between prompts in the single text object.
	const std::wstring button_gap = L" ";
	const std::wstring prompt_gap = L"    ";

	std::wstring join(const std::vector<PromptBar::Prompt>& prompts)
	{
		std::wstring text;
		for (const PromptBar::Prompt& prompt : prompts)
		{
			if (!text.empty())
			{
				text += prompt_gap;
			}
			text += prompt.button + button_gap + prompt.action;
		}
		return text;
	}
}

PromptBar::PromptBar(const std::string& name,
	const std::vector<Prompt>& prompts,
	const PromptBarPresentation& look,
	RenderResources* render_resources) :
	PromptBar(name, join(prompts), look, render_resources)
{
}

PromptBar::PromptBar(const std::string& name, const std::wstring& legend,
	const PromptBarPresentation& look, RenderResources* render_resources) :
	colourwars::UiTextDropShadow(name,
		legend,
		look.font,
		look.position,
		render_resources,
		look.colour,
		look.shadow_colour,
		look.shadow_offset)
{

}
