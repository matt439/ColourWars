#pragma once

#include "editor/level_json.h"
#include "game/paint_polygon_geometry.h"

namespace colourwars_editor
{
    // Engine-free copy of the bounded fixed-slab construction. All arithmetic
    // defining its represented pose is float, as in the runtime OBB constructor.
    struct SlabGeometry
    {
        float center_x = 0, center_y = 0;
        float tangent_x = 0, tangent_y = 0, normal_x = 0, normal_y = 0;
        float length = 0;
        PaintPolygon corners{};
        // Authored joins for paint occlusion only. Guides/collision previews
        // continue to use the represented OBB corners above.
        PaintPolygon paint_corners{};
        PaintPoint to_world(float x, float y) const;
        PaintPoint to_local(const PaintPoint& point) const;
        Rect bounds() const;
    };

    SlabGeometry decode_slab(const level_json::Json& object);
    void validate_slab(const level_json::Json& object);
    void validate_slab_pairs(const level_json::Document& document);
    void move_slab(level_json::Json& object, const Rect& rectangle);
}
