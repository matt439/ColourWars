#include "game/pch.h"
#include "game/display_awake.h"

namespace
{
    bool request_display_idle(bool keep) noexcept
    {
        const EXECUTION_STATE flags = keep ?
            (ES_CONTINUOUS | ES_DISPLAY_REQUIRED) : ES_CONTINUOUS;
        if (::SetThreadExecutionState(flags) != 0) { return true; }
        // A failed request is nonfatal. The API does not promise a useful
        // GetLastError value; report only the failed operation, once.
        std::fputs(keep ? "display idle: could not acquire the match display request.\n" :
            "display idle: could not clear the match display request.\n", stderr);
        return false;
    }
}

DisplayAwake::DisplayAwake() noexcept : DisplayAwake(request_display_idle)
{
}