#pragma once

// Carried from Labrador v0.2.0; v0.3.0 removes this type (730dd99,
// "A game that used one should copy the class ... into its own source").
// Shape unchanged; it lives in colourwars:: so it can never be ambiguous with
// an engine type. See docs/review/engine-reports.md, HYG#25.

namespace colourwars
{
    enum class RotationOrigin
    {
        center,
        top_left,
        left_center,
        no_rotation
    };
}
