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
        const DPI_AWARENESS_CONTEXT before = GetThreadDpiAwarenessContext();
        const InitialDisplay actual = initial_display_for_primary_monitor();
        acceptance::require(AreDpiAwarenessContextsEqual(before, GetThreadDpiAwarenessContext()) != FALSE,
            "The monitor query changed the caller's DPI context");
        acceptance::require(actual.resolution == ScreenResolution::s_1280_720 ||
            actual.resolution == ScreenResolution::s_1920_1080,
            "The monitor query returned an unsupported first-run resolution");
        std::cout << "PASS initial display: work area, DPI frame, small-screen fullscreen, "
            "fresh-save defaults, existing preferences and restored DPI context\n";
        return 0;
    });
}
