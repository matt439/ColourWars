#include "game/pch.h"
#include "game/config/initial_display.h"
#include "game/save/save.h"
#include "scripts/acceptance/support/probe.h"
#include "scripts/acceptance/support/isolated_save.h"

#include <iostream>

namespace
{
    void check_size(int width, int height, int frame_width, int frame_height,
        labrador::ScreenResolution resolution, bool fullscreen)
    {
        const InitialDisplay chosen = choose_initial_display(width, height, frame_width, frame_height);
        acceptance::require(chosen.resolution == resolution && chosen.fullscreen == fullscreen,
            "First-run display did not fit its work area and window frame");
    }
}

int main()
{
    return acceptance::run_probe("initial display", []
    {
        using labrador::ScreenResolution;
        check_size(1920, 1040, 16, 39, ScreenResolution::s_1280_720, false);
        check_size(1920, 1020, 24, 58, ScreenResolution::s_1280_720, false);
        check_size(2560, 1400, 16, 39, ScreenResolution::s_1920_1080, false);
        check_size(1296, 759, 16, 39, ScreenResolution::s_1280_720, false);
        check_size(1295, 759, 16, 39, ScreenResolution::s_1280_720, true);
        check_size(1366, 728, 16, 39, ScreenResolution::s_1280_720, true);
        check_size(0, 0, 0, 0, ScreenResolution::s_1280_720, true);

        const acceptance::IsolatedSave isolated(L"initial-display");
        Save fresh(ScreenResolution::s_1280_720, true);
        fresh.load_save_file();
        acceptance::require(fresh.first_run() && fresh.resolution() == ScreenResolution::s_1280_720 && fresh.fullscreen(),
            "A fresh save did not keep the monitor-fitted display defaults");
        Save existing(ScreenResolution::s_1920_1080, false);
        existing.load_save_file();
        acceptance::require(!existing.first_run() && existing.resolution() == ScreenResolution::s_1280_720 && existing.fullscreen(),
            "First-run display defaults overwrote an existing save");
        isolated.write_save("{\"resolution\":{\"x\":2560,\"y\":1440},\"fullscreen\":false}");
        Save preferred(ScreenResolution::s_1280_720, true);
        preferred.load_save_file();
        acceptance::require(preferred.resolution() == ScreenResolution::s_2560_1440 && !preferred.fullscreen(),
            "The player's saved display preference was fitted again");

        // POL#49: Windows' Animation effects chooses a fresh save's shared
        // Reduce effects, and nothing else. The probe only reads the setting;
        // an SPI_SET call would change the player's whole desktop.
        acceptance::require(choose_initial_reduce_effects(false) && !choose_initial_reduce_effects(true),
            "Reduce effects did not start On exactly when Windows' animations are off");
        {
            const acceptance::IsolatedSave calm(L"initial-reduce-on");
            Save fresh_on(ScreenResolution::s_1280_720, false, true);
            fresh_on.load_save_file();
            acceptance::require(fresh_on.first_run() && fresh_on.saved_play_settings().reduce_effects,
                "A fresh save did not start with Reduce effects On when Windows' animations are off");
            acceptance::require(fresh_on.saved_play_settings().reduce_effects_for(0) &&
                fresh_on.saved_play_settings().reduce_effects_for(-1),
                "The seats did not follow the fresh save's shared Reduce effects");
            Save reread(ScreenResolution::s_1280_720, false, false);
            reread.load_save_file();
            acceptance::require(!reread.first_run() && reread.saved_play_settings().reduce_effects,
                "The fresh save's Reduce effects was not written to the file");

            calm.write_save("{\"reduce_effects\":false}");
            Save kept(ScreenResolution::s_1280_720, false, true);
            kept.load_save_file();
            acceptance::require(!kept.first_run() && !kept.saved_play_settings().reduce_effects,
                "Windows' animation setting overwrote an existing save's Reduce effects");

            calm.write_save("{ not json");
            Save corrupt(ScreenResolution::s_1280_720, false, true);
            corrupt.load_save_file();
            acceptance::require(!corrupt.first_run() && !corrupt.saved_play_settings().reduce_effects,
                "An unreadable save took the first-run Reduce effects instead of the defaults");
        }
        {
            const acceptance::IsolatedSave animated(L"initial-reduce-off");
            Save fresh_off(ScreenResolution::s_1280_720, false, false);
            fresh_off.load_save_file();
            acceptance::require(fresh_off.first_run() && !fresh_off.saved_play_settings().reduce_effects &&
                !fresh_off.saved_play_settings().reduce_effects_for(0),
                "A fresh save did not keep Reduce effects Off when Windows' animations are on");
        }
        BOOL animation = TRUE;
        const bool queried = SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &animation, 0) != FALSE;
        acceptance::require(initial_reduce_effects_from_system() == (queried && animation == FALSE),
            "The live Reduce effects default did not follow Windows' Animation effects");

        const DPI_AWARENESS_CONTEXT before = GetThreadDpiAwarenessContext();
        const InitialDisplay actual = initial_display_for_primary_monitor();
        acceptance::require(AreDpiAwarenessContextsEqual(before, GetThreadDpiAwarenessContext()) != FALSE,
            "The monitor query changed the caller's DPI context");
        acceptance::require(actual.resolution == ScreenResolution::s_1280_720 ||
            actual.resolution == ScreenResolution::s_1920_1080,
            "The monitor query returned an unsupported first-run resolution");
        std::cout << "PASS initial display: work area, DPI frame, small-screen fullscreen, "
            "fresh-save defaults, existing preferences, Reduce effects from Windows' Animation "
            "effects on fresh saves only and restored DPI context\n";
        return 0;
    });
}
