#pragma once

#include "engine/render/renderer.h"
#include "game/config/presentation/presentation_match.h"
#include <algorithm>
#include <cstdlib>
#include <optional>
#include <string_view>

// Runtime is immutable for one launch: saved preferences take effect on restart.
// An explicit developer environment override never changes the saved preference.
namespace crt_trial
{
    inline std::optional<bool> environment_override()
    {
#ifdef _MSC_VER
        char buffer[2]{};
        size_t size = 0;
        if (getenv_s(&size, buffer, sizeof(buffer), "COLOURWARS_CRT_TRIAL") != 0) return std::nullopt;
        const char* value = buffer;
#else
        const char* value = std::getenv("COLOURWARS_CRT_TRIAL");
#endif
        if (value != nullptr && std::string_view(value) == "1") return true;
        if (value != nullptr && std::string_view(value) == "0") return false;
        return std::nullopt;
    }

    class Runtime
    {
    public:
        explicit Runtime(bool saved = false, std::optional<bool> override = environment_override()) :
            enabled_(override.value_or(saved)), overridden_(override.has_value()) {}
        bool enabled() const { return enabled_; }
        bool overridden() const { return overridden_; }
        int view_capacity() const { return enabled_ ? 5 : 4; }
        void initialize(labrador::Renderer& renderer) const
        {
            if (enabled_)
            {
                const std::array effects{ labrador::PostEffect::crt };
                renderer.set_post_effects(effects);
            }
        }
    private:
        const bool enabled_;
        const bool overridden_;
    };

    // Legacy isolated probes may omit the runtime; preserve their explicit flag.
    inline const Runtime& probe_runtime() { static const Runtime runtime; return runtime; }
    inline bool requested() { return probe_runtime().enabled(); }
    inline bool enabled(const Runtime* runtime) { return runtime ? runtime->enabled() : requested(); }
    inline int view_capacity() { return probe_runtime().view_capacity(); }
    inline void initialize(labrador::Renderer& renderer) { probe_runtime().initialize(renderer); }

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

    inline void clear(labrador::Renderer& renderer, const Runtime* runtime = nullptr)
    {
        if (enabled(runtime)) { renderer.set_post_values(labrador::PostEffect::crt, {}); }
    }
}
