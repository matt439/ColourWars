#pragma once

#include "engine/render/renderer.h"
#include "game/config/presentation/presentation_match.h"
#include <algorithm>
#include <cstdlib>
#include <string_view>

// A local opt-in experiment, not a saved player preference. The default chain
// stays empty. Only exact "1" enables it, fixed for the process lifetime.
namespace crt_trial
{
    inline bool requested()
    {
        static const bool enabled = [] {
            #ifdef _MSC_VER
            char buffer[2]{};
            size_t size = 0;
            if (getenv_s(&size, buffer, sizeof(buffer), "COLOURWARS_CRT_TRIAL") != 0) return false;
            const char* value = buffer;
#else
            const char* value = std::getenv("COLOURWARS_CRT_TRIAL");
#endif
            return value != nullptr && std::string_view(value) == "1";
        }();
        return enabled;
    }

    inline labrador::CrtSettings settings(const CrtTrialPresentation& look,
        const labrador::RectangleF& glass)
    {
        labrador::CrtSettings result;
        result.screen = glass;
        // No curvature or chromatic split: authored cursor positions and
        // the region boundary stay exact, including the glass transition.
        result.scanline_count = std::min(look.scanline_count,
            glass.height / labrador::min_pixels_per_scanline);
        result.scanline_strength = look.scanline_strength;
        result.mask_strength = look.mask_strength;
        result.vignette = look.vignette;
        result.brightness = look.brightness;
        return result;
    }

    inline void clear(labrador::Renderer& renderer)
    {
        if (requested()) { renderer.set_post_values(labrador::PostEffect::crt, {}); }
    }
}
