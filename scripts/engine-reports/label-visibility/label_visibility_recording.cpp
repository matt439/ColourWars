// ENG#18 private preparation: actual Label + null renderer, synthetic data only.
// A recording is not a rasterizer, blend result, or performance measurement.
#include "engine/render/colour.h"
#include "engine/render/font.h"
#include "engine/render/label.h"
#include "engine/render/null/recording.h"
#include "engine/render/render_resources.h"
#include "engine/render/renderer.h"
#include "engine/render/resource_factory.h"
#include "engine/render/texture_data.h"
#include "engine/math/rectanglef.h"
#include "engine/math/rectanglei.h"
#include "engine/math/vector2f.h"

#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
    using namespace labrador;
    using namespace mattmath;

    void require(bool condition, const char* explanation)
    {
        if (!condition) { throw std::runtime_error(explanation); }
    }

    void observed(const char* name, bool condition)
    {
        require(condition, name);
        std::cout << name << "=true\n";
    }

    class Harness
    {
    public:
        Harness()
        {
            this->renderer.create_device(nullptr, 64, 64, 1);
            require(this->renderer.device_info().backend == "null",
                "This exhibit must use the null renderer.");
            this->renderer.set_resources(&this->resources);

            // A valid single-level RGBA atlas in memory. No font/texture file,
            // loader shortcut, private backend header, or application is needed.
            TextureData atlas;
            atlas.width = 8;
            atlas.height = 8;
            atlas.format = TextureFormat::r8g8b8a8_unorm;
            TextureLevel level;
            level.width = 8;
            level.height = 8;
            level.stride = 32;
            level.rows = 8;
            level.offset = 0;
            level.size = 256;
            atlas.levels.push_back(level);
            atlas.pixels.assign(256, 255);
            add_texture_asset(this->renderer, this->resources, "synthetic-atlas", atlas);
            const TextureHandle texture = this->resources.resolve_texture("synthetic-atlas");

            Glyph a;
            a.character = U'A';
            a.subrect = RectangleI(0, 0, 4, 8);
            Glyph b;
            b.character = U'B';
            b.subrect = RectangleI(4, 0, 4, 8);
            std::vector<Glyph> glyphs{ a, b };
            this->resources.add_font("synthetic-font",
                std::make_unique<Font>(texture, std::move(glyphs), 8.0f));
        }

        std::vector<RecordedSprite> record(const Label& label)
        {
            this->renderer.begin_frame();
            this->renderer.set_view_count(1);
            DrawList list = this->renderer.view(0);
            label.draw(list);
            this->renderer.submit();
            // Copy before the next begin_frame invalidates the recording.
            std::vector<RecordedSprite> copy = recorded_sprites(this->renderer);
            this->renderer.end_frame();
            return copy;
        }

        // The resources outlive the renderer, as required by their public seam.
        RenderResources resources;
        Renderer renderer;
    };

    bool all_colour(const std::vector<RecordedSprite>& sprites, const Colour& colour)
    {
        if (sprites.empty()) { return false; }
        for (const RecordedSprite& sprite : sprites)
        {
            for (const SpriteVertex& corner : sprite.corners)
            {
                if (corner.colour != colour) { return false; }
            }
        }
        return true;
    }

    bool same_geometry(const std::vector<RecordedSprite>& first,
        const std::vector<RecordedSprite>& second)
    {
        if (first.size() != second.size()) { return false; }
        for (std::size_t index = 0; index < first.size(); ++index)
        {
            const RecordedSprite& a = first[index];
            const RecordedSprite& b = second[index];
            if (a.view != b.view || a.texture.index() != b.texture.index() ||
                a.viewport != b.viewport || a.filter != b.filter) { return false; }
            for (std::size_t corner = 0; corner < 4; ++corner)
            {
                if (a.corners[corner].position != b.corners[corner].position ||
                    a.corners[corner].texcoord != b.corners[corner].texcoord)
                {
                    return false;
                }
            }
        }
        return true;
    }

    void count(const char* name, const std::vector<RecordedSprite>& sprites,
        std::size_t expected)
    {
        require(sprites.size() == expected, name);
        std::cout << name << '=' << expected << '\n';
    }
}

int main()
{
    try
    {
        Harness harness;
        const Colour shown(0.5f, 0.25f, 0.0f, 1.0f);
        const Colour zero(0.0f, 0.0f, 0.0f, 0.0f);
        Label label(L"AB", "synthetic-font", Vector2F(8.0f, 16.0f),
            &harness.resources, shown);
        const RectangleF bounds = label.bounds();
        require(bounds == RectangleF(8.0f, 16.0f, 8.0f, 8.0f),
            "The synthetic label must have the expected nonempty extent.");

        const std::vector<RecordedSprite> baseline = harness.record(label);
        count("shown.sprites", baseline, 2);
        require(all_colour(baseline, shown), "Shown colour must reach every corner.");

        // The actual station-label client workaround: visibility is encoded
        // into colour. This does not ask the renderer to omit the draw.
        label.set_colour(zero);
        const std::vector<RecordedSprite> hidden = harness.record(label);
        count("zero_tint.sprites", hidden, 2);
        observed("zero_tint.rgba_zero", all_colour(hidden, zero));
        observed("zero_tint.text_and_bounds_preserved",
            label.text() == L"AB" && label.bounds() == bounds);
        observed("zero_tint.geometry_matches_shown", same_geometry(baseline, hidden));

        // A state-level negative control, not a pixel/blending assertion.
        Colour alpha_only = shown;
        alpha_only.make_transparent();
        label.set_colour(alpha_only);
        const std::vector<RecordedSprite> alpha = harness.record(label);
        count("alpha_only.sprites", alpha, 2);
        observed("alpha_only.retains_rgb", alpha_only.a == 0.0f &&
            alpha_only.r == shown.r && alpha_only.g == shown.g &&
            alpha_only.b == shown.b && all_colour(alpha, alpha_only));

        label.set_colour(shown);
        const std::vector<RecordedSprite> restored = harness.record(label);
        count("restored_tint.sprites", restored, 2);
        observed("restored_tint.matches_shown", same_geometry(baseline, restored) &&
            all_colour(restored, shown));

        // Existing alternative: empty text emits no glyphs, but changes both
        // text state and measured extent. The client would retain the words.
        label.set_text(L"");
        const std::vector<RecordedSprite> empty = harness.record(label);
        count("empty_text.sprites", empty, 0);
        observed("empty_text.bounds_empty", label.bounds().width == 0.0f &&
            label.bounds().height == 0.0f);
        label.set_text(L"AB");
        const std::vector<RecordedSprite> restored_text = harness.record(label);
        observed("restored_text.matches_shown", label.bounds() == bounds &&
            same_geometry(baseline, restored_text) && all_colour(restored_text, shown));

        std::cout << "PASS label-visibility-recording\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL label-visibility-recording: " << error.what() << '\n';
        return 1;
    }
}
