#pragma once

// The developer-tools gate. Derived from the CRT's own _DEBUG, never from a
// project /D flag: the consumer acceptance probes (scripts/run-acceptance.ps1,
// scripts/check-movement-feedback.ps1, scripts/check-workshop-playable.ps1)
// compile game headers with only /MDd or /MD /DNDEBUG and link the game's own
// objects, so a project-only define could give the two sides different class
// layouts. No existing class has a member that depends on this switch.
#if defined(_DEBUG)
#define COLOURWARS_DEV_TOOLS 1
#else
#define COLOURWARS_DEV_TOOLS 0
#endif

namespace dev_tools
{
	inline constexpr bool enabled = COLOURWARS_DEV_TOOLS != 0;
}
