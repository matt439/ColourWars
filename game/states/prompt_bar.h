#pragma once

#include "engine/ui/widget.h"
#include "game/presentation.h"

#include <memory>
#include <string>
#include <vector>

// Text legend for menu controls. Keeps button labels and actions separate
// so pages do not depend on the legend's formatting.
class PromptBar final : public labrador::UiTextDropShadow
{
public:
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
