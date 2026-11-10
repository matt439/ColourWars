#pragma once

#include "engine/render/screen_resolution.h"

namespace labrador { class Application; }
class Save;

struct InitialDisplay
{
    labrador::ScreenResolution resolution = labrador::ScreenResolution::s_1280_720;
    bool fullscreen = false;
};

// The first-run preference only. Existing and imported saves keep their choice.
// Width and height are the work area in physical pixels; frame sizes include
// the title bar and borders at the primary display's DPI.
InitialDisplay choose_initial_display(int work_width, int work_height,
    int frame_width, int frame_height);
InitialDisplay initial_display_for_primary_monitor();
// Finish fitting to the monitor Windows actually selected, including its DPI
// and the default cascaded position. Called only for Save::first_run().
void fit_initial_window(labrador::Application& app, Save& save);
