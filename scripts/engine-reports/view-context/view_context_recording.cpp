// ENG#20 private preparation: existing public APIs, synthetic geometry only.
// API absence is established by source audit; this records a caller-owned
// context alternative, not a render bug or a replacement Scene implementation.
#include "engine/core/game_object.h"
#include "engine/math/rectanglef.h"
#include "engine/math/rectanglei.h"
#include "engine/math/vector2f.h"
#include "engine/render/camera.h"
#include "engine/render/colour.h"
#include "engine/render/null/recording.h"
#include "engine/render/render_resources.h"
#include "engine/render/renderer.h"
#include "engine/render/resource_factory.h"
#include "engine/render/texture_data.h"
#include "engine/render/viewport.h"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
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

    // Both this type and draw_with_context below belong to the synthetic
    // client. Neither is an existing or proposed-to-exist engine API call.
    struct ClientViewContext
    {
        Camera camera;
        Viewport viewport;
    };

    class ThinWorldObject final : public GameObject
    {
    public:
        explicit ThinWorldObject(TextureHandle texture) : texture_(texture) {}

        void update(float /*dt*/) override {}

        void draw(DrawList& list) const override
        {
            // Ordinary authored world geometry: the list maps it normally.
            list.draw_sprite(this->texture_, RectangleI(0, 0, 2, 2),
                this->authored_, Colour::white, 0.0f, Vector2F::ZERO,
                SpriteFlip::none, 0.0f);
        }

        RectangleF bounds() const override { return this->authored_; }

        RectangleF cull_bounds(float units_per_pixel) const override
        {
            // This scale-aware culling hook ALREADY exists. It is a pure
            // extent calculation, never a mutable handoff to later drawing.
            RectangleF result = this->authored_;
            result.height = std::max(result.height, units_per_pixel);
            return result;
        }

        void draw_with_context(DrawList& list, const ClientViewContext& context) const
        {
            RectangleF projected = context.camera.calculate_view_rectangle(this->authored_);
            // Explicitly client art policy, not an engine correctness rule:
            // retain at least one view pixel of thickness for this detail.
            projected.height = std::max(projected.height, 1.0f);
            list.set_viewport(context.viewport);
            list.set_camera(Camera::DEFAULT_CAMERA);
            list.draw_sprite(this->texture_, RectangleI(0, 0, 2, 2), projected,
                Colour::white, 0.0f, Vector2F::ZERO, SpriteFlip::none, 0.0f);
            // Restore values supplied/owned by this caller. No active-state
            // getter is assumed, and mismatched caller context is not fixed.
            list.set_viewport(context.viewport);
            list.set_camera(context.camera);
        }

    private:
        const TextureHandle texture_;
        const RectangleF authored_{ 16.0f, 16.0f, 16.0f, 1.0f };
    };

    class Harness
    {
    public:
        Harness()
        {
            this->renderer.create_device(nullptr, 128, 64, 2);
            require(this->renderer.device_info().backend == "null", "A null renderer is required.");
            this->renderer.set_resources(&this->resources);
            TextureData atlas;
            atlas.width = 2;
            atlas.height = 2;
            atlas.format = TextureFormat::r8g8b8a8_unorm;
            TextureLevel level;
            level.width = 2;
            level.height = 2;
            level.stride = 8;
            level.rows = 2;
            level.size = 16;
            atlas.levels.push_back(level);
            atlas.pixels.assign(16, 255);
            add_texture_asset(this->renderer, this->resources, "synthetic", atlas);
        }

        std::vector<RecordedSprite> record(const ThinWorldObject& object, bool explicit_context)
        {
            this->renderer.begin_frame();
            this->renderer.set_view_count(2);
            const ClientViewContext contexts[2] = {
                { Camera(0.0f, 0.0f, 1.0f), Viewport(0.0f, 0.0f, 64.0f, 64.0f) },
                { Camera(8.0f, 8.0f, 0.25f), Viewport(64.0f, 0.0f, 64.0f, 64.0f) }
            };
            for (int index = 0; index < 2; ++index)
            {
                DrawList list = this->renderer.view(index);
                list.set_viewport(contexts[index].viewport);
                list.set_camera(contexts[index].camera);
                if (explicit_context) { object.draw_with_context(list, contexts[index]); }
                // In the explicit frame this also verifies caller-state
                // restoration by recording the ordinary world draw after it.
                object.draw(list);
            }
            this->renderer.submit();
            std::vector<RecordedSprite> copy = recorded_sprites(this->renderer);
            this->renderer.end_frame();
            return copy;
        }

        // Public resource lifetime rule: resources outlive the renderer.
        RenderResources resources;
        Renderer renderer;
    };

    RectangleF recorded_box(const RecordedSprite& sprite)
    {
        float left = sprite.corners[0].position.x;
        float right = left;
        float top = sprite.corners[0].position.y;
        float bottom = top;
        for (const SpriteVertex& corner : sprite.corners)
        {
            left = std::min(left, corner.position.x);
            right = std::max(right, corner.position.x);
            top = std::min(top, corner.position.y);
            bottom = std::max(bottom, corner.position.y);
        }
        return RectangleF(left, top, right - left, bottom - top);
    }

    void expect_box(const char* name, const RecordedSprite& sprite,
        const RectangleF& expected)
    {
        require(recorded_box(sprite) == expected, name);
        std::cout << name << '=' << expected.width << 'x' << expected.height << '\n';
    }

    bool same_record(const RecordedSprite& a, const RecordedSprite& b)
    {
        if (a.view != b.view || a.texture.index() != b.texture.index() ||
            a.viewport != b.viewport || a.filter != b.filter) { return false; }
        for (std::size_t index = 0; index < 4; ++index)
        {
            if (a.corners[index].position != b.corners[index].position ||
                a.corners[index].texcoord != b.corners[index].texcoord ||
                a.corners[index].colour != b.corners[index].colour) { return false; }
        }
        return true;
    }
}

int main()
{
    try
    {
        Harness harness;
        const ThinWorldObject object(harness.resources.resolve_texture("synthetic"));
        const RectangleF original_bounds = object.bounds();
        const std::vector<RecordedSprite> ordinary = harness.record(object, false);
        const std::vector<RecordedSprite> explicit_context = harness.record(object, true);
        require(ordinary.size() == 2 && explicit_context.size() == 4,
            "Expected two ordinary records and four explicit/restoration records.");
        require(ordinary[0].view == 0 && ordinary[1].view == 1 &&
            explicit_context[0].view == 0 && explicit_context[2].view == 1,
            "Recordings must remain in view order.");

        expect_box("ordinary.view0.pixels", ordinary[0], RectangleF(16.0f, 16.0f, 16.0f, 1.0f));
        expect_box("ordinary.view1.pixels", ordinary[1], RectangleF(2.0f, 2.0f, 4.0f, 0.0f));
        expect_box("explicit.view0.pixels", explicit_context[0], RectangleF(16.0f, 16.0f, 16.0f, 1.0f));
        expect_box("explicit.view1.pixels", explicit_context[2], RectangleF(2.0f, 2.0f, 4.0f, 1.0f));
        require(recorded_box(explicit_context[2]).position() == Vector2F(2.0f, 2.0f),
            "Camera translation must be applied in viewport-local pixels.");
        std::cout << "explicit.view1.local_origin=2,2\n";
        require(explicit_context[2].viewport.position() == Vector2F(64.0f, 0.0f),
            "Pane origin belongs to the viewport, not local geometry.");
        std::cout << "explicit.view1.viewport_origin=64,0\n";
        observed("explicit.restores_caller_camera_and_viewport",
            same_record(ordinary[0], explicit_context[1]) &&
            same_record(ordinary[1], explicit_context[3]));
        observed("object.bounds_unchanged", object.bounds() == original_bounds);
        observed("cull_bounds.scale_allowance_supported",
            object.cull_bounds(1.0f).height == 1.0f &&
            object.cull_bounds(4.0f).height == 4.0f);
        std::cout << "PASS view-context-recording\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL view-context-recording: " << error.what() << '\n';
        return 1;
    }
}
