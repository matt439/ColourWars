#include "game/pch.h"
#include "game/config/crt_trial.h"
#include "scripts/acceptance/support/probe.h"
#include "scripts/acceptance/support/isolated_save.h"
#include "game/config/settings_session.h"
#include "game/save/save.h"

int main()
{
    return acceptance::run_probe("CRT trial", []
    {
        using namespace labrador;
        using acceptance::require;
        require(crt_values_are_neutral({}), "Cleared CRT values must be identity");
        const acceptance::IsolatedSave isolated(L"crt-settings");
        Save save;
        save.load_save_file();
        SettingsSession session(save);
        require(!session.play_settings().crt_monitor, "Fresh CRT preference was not off");
        const crt_trial::Runtime launched(false, std::nullopt);
        require(!launched.enabled() && launched.view_capacity() == 4, "Off launch changed view capacity");
        require(session.set_crt_monitor_and_save(true), "Could not save CRT preference");
        require(!launched.enabled(), "Saved choice changed running graphics state");
        Save restored; restored.load_save_file();
        const crt_trial::Runtime restarted(restored.saved_play_settings().crt_monitor, std::nullopt);
        require(restarted.enabled() && restarted.view_capacity() == 5, "Restart did not pick up CRT choice");
        require(crt_trial::Runtime(false, true).enabled() && crt_trial::Runtime(true, false).overridden() &&
            !crt_trial::Runtime(true, false).enabled(), "Developer override precedence failed");
        const std::string before = isolated.save_bytes();
        {
            acceptance::ReplacementLock lock(isolated.save_path());
            require(lock.hold(), "Could not lock CRT save fixture");
            require(!session.set_crt_monitor_and_save(false), "Blocked CRT save reported success");
            require(session.play_settings().crt_monitor && isolated.save_bytes() == before,
                "Failed CRT save changed effective preference or prior file");
        }
        require(session.set_crt_monitor_and_save(false), "Could not turn CRT off for next launch");
        require(restarted.enabled(), "Saved off choice changed current launch");
        Save off; off.load_save_file();
        require(!off.saved_play_settings().crt_monitor, "Off choice failed roundtrip");
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
