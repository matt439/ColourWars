#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/script/debug_file_name.h"
#include "game/debug/capture/debug_screenshot.h"
#include "engine/app/application.h"
#include "engine/render/renderer.h"
#include <cctype>

ScreenCapture capture_stack(labrador::Application& application)
{
	return capture_frame(*application.renderer(),
		[&application](labrador::Renderer& renderer) { application.draw(renderer); });
}

std::string screenshot_file_name(std::string_view requested,
	std::chrono::system_clock::time_point now)
{
	std::string name(requested);
	if (name.size() > 4)
	{
		std::string tail = name.substr(name.size() - 4);
		for (char& c : tail)
		{
			c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		}
		if (tail == ".png")
		{
			name.resize(name.size() - 4);
		}
	}
	for (char& c : name)
	{
		c = debug_file_name::stem_character(c) ? c : '_';
	}
	if (name.empty())
	{
		name = "colourwars-" + capture_file_timestamp(now);
	}
	if (debug_file_name::reserved_stem(name))
	{
		name.insert(name.begin(), '_');
	}
	return name + ".png";
}
#endif
