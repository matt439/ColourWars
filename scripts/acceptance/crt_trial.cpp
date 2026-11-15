#include "game/pch.h"
#include "game/config/crt_trial.h"
#include "scripts/acceptance/support/probe.h"

int main()
{
    return acceptance::run_probe("CRT trial", []
    {
        using namespace labrador;
        using acceptance::require;
        require(crt_values_are_neutral({}), "Cleared CRT values must be identity");
        const CrtTrialPresentation look{90.0f, 0.12f, 0.08f, 0.06f, 1.04f};
        for (const float height : {16.0f, 240.0f, 360.0f, 720.0f, 1440.0f})
        {
            const RectangleF glass(10.0f, 20.0f, height * 1.5f, height);
            const auto settings = crt_trial::settings(look, glass);
            require(settings.scanline_count > 0 && height / settings.scanline_count >= min_pixels_per_scanline,
                "Scanline spacing must stay inside engine sampling bounds");
            require(settings.curvature == 0 && settings.aberration == 0,
                "Prototype must not distort pointer coordinates or split colour edges");
            require(!crt_values_are_neutral(crt_values(settings, Vector2F(3840, 2160))),
                "Enabled trial must produce an effect");
            require(crt_values_are_neutral(crt_values(crt_trial::settings({}, glass), Vector2F(3840, 2160))),
                "Omitted trial configuration must remain neutral");
        }
        return 0;
    });
}
