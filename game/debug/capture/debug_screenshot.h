#pragma once

#include "game/save/png_capture.h"
#include <string_view>

namespace labrador
{
	class Application;
}

// The developer `screenshot` command draws the whole stack into the capture.
ScreenCapture capture_stack(labrador::Application& application);

// `requested` kept to [A-Za-z0-9_-] (anything else becomes '_', a trailing
// ".png" is dropped), plus ".png"; empty requests the local-time default
// colourwars-YYYYMMDD-HHMMSS-mmm.png. Reserved Windows stems receive a leading
// underscore before the extension ("CON" becomes "_CON.png").
std::string screenshot_file_name(std::string_view requested,
	std::chrono::system_clock::time_point now);
