#pragma once

// The built-in solid texel that primitive drawing stretches over rectangles:
// the player badge, teammate marker, gate and ferry cues, the split-screen
// dividers and the developer overlays. The menus and the opening name their
// own sheet and fill frame in presentation.json instead. The presentation
// validator checks this frame resolves at startup.
namespace primitive_frame
{
    inline constexpr char sheet[] = "sprite_sheet_1";
    inline constexpr char pixel[] = "pixel";
}
