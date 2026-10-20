// Synthetic engine-only illustration of the current out-of-band redraw.
// No game code/assets, loop, visible window, input request or present call.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include "engine/app/application.h"
#include "engine/core/state.h"
#include "engine/core/state_context.h"
#include "engine/render/render_resources.h"
#include "engine/render/resource_factory.h"
#include "engine/render/texture_data.h"

#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    struct Counts
    {
        unsigned initializes = 0;
        unsigned updates = 0;
        unsigned draws = 0;
        unsigned readbacks = 0;
        unsigned suspends = 0;
        unsigned resumes = 0;
        bool blue = false;
    };

    struct FinalCaptureResult
    {
        std::vector<unsigned char> pixels;
        unsigned captures = 0;
        unsigned updates = 0;
        unsigned destroyed = 0;
        unsigned world_draws_at_capture = 0;
        int depth_while_pop_pending = 0;
        bool captured = false;
    };

    class SyntheticState final : public labrador::State
    {
    public:
        SyntheticState(labrador::TextureHandle texture, Counts& counts)
            : texture_(texture), counts_(counts) {}

        void init() override { ++counts_.initializes; }
        void update(float) override { ++counts_.updates; }
        void on_suspend() override { ++counts_.suspends; }
        void on_resume() override { ++counts_.resumes; }
        void draw(labrador::Renderer& renderer) const override
        {
            ++counts_.draws;
            const auto size = renderer.back_buffer_size();
            renderer.set_view_count(1);
            auto view = renderer.view(0);
            view.set_viewport(labrador::Viewport(0.0f, 0.0f, size.x, size.y));
            const labrador::Colour tint = counts_.blue
                ? labrador::Colour(0, 0, 255) : labrador::Colour(255, 0, 0);
            view.draw_sprite(texture_, mattmath::RectangleI(0, 0, 1, 1),
                mattmath::RectangleF(0.0f, 0.0f, size.x, size.y), tint,
                0.0f, mattmath::Vector2F(0.0f, 0.0f),
                labrador::SpriteFlip::none, 0.0f);
        }

    private:
        labrador::TextureHandle texture_;
        Counts& counts_;
    };

    // A non-covering top State runs after the world's draw. It submits and
    // reads once; the outer render's subsequent submit is idempotent.
    // It changes stack lifecycle and consumes its next update, which is a
    // client policy cost, not an invisible host post-submit callback.
    class FinalCaptureState final : public labrador::State
    {
    public:
        FinalCaptureState(FinalCaptureResult& result, Counts& counts)
            : result_(result), counts_(counts) {}
        ~FinalCaptureState() override { ++result_.destroyed; }
        void init() override {}
        bool covers_screen() const override { return false; }
        void draw(labrador::Renderer& renderer) const override
        {
            if (result_.captured) { return; }
            renderer.submit();
            renderer.read_back_buffer(result_.pixels);
            ++result_.captures;
            ++counts_.readbacks;
            result_.world_draws_at_capture = counts_.draws;
            result_.captured = true;
        }
        void update(float) override
        {
            ++result_.updates;
            auto* pending_context = context();
            auto* pending_result = &result_;
            // StateContext defers this pop until update() returns. Never pop
            // from draw(): its stack walk does not provide that deferral.
            pending_context->pop();
            pending_result->depth_while_pop_pending = pending_context->depth();
        }

    private:
        FinalCaptureResult& result_;
        Counts& counts_;
    };

    // The client workaround: draw the complete stack again, submit it and
    // synchronously copy its current pixels, without calling end_frame.
    // This is scoped to the existing D3D11 build, not portable abandonment proof.
    std::vector<unsigned char> redraw_and_read(labrador::Application& app, Counts& counts)
    {
        auto& renderer = *app.renderer();
        renderer.begin_frame();
        app.draw(renderer);
        renderer.submit();
        std::vector<unsigned char> pixels;
        renderer.read_back_buffer(pixels);
        ++counts.readbacks;
        return pixels;
    }

    void require_colour(const std::vector<unsigned char>& pixels,
        unsigned char red, unsigned char green, unsigned char blue)
    {
        constexpr std::size_t expected_bytes = 1280u * 720u * 4u;
        require(pixels.size() == expected_bytes, "RGBA byte count differs from the configured buffer");
        for (std::size_t offset = 0; offset < pixels.size(); offset += 4u)
        {
            require(pixels[offset] == red && pixels[offset + 1u] == green &&
                pixels[offset + 2u] == blue && pixels[offset + 3u] == 255u,
                "full-buffer synthetic colour oracle failed");
        }
    }
}

int wmain()
{
    try
    {
        Counts counts;
        FinalCaptureResult final_capture;
        HWND handle = nullptr;
        {
            labrador::ApplicationOptions options;
            options.window_class_name = L"LabradorFrameReadbackExhibit";
            options.window_title = L"Hidden out-of-band readback illustration";
            options.visible = false;
            options.fullscreen = false;
            options.resolution = labrador::ScreenResolution::s_1280_720;
            options.min_threads = 1;
            options.max_threads = 1;
            options.view_capacity = 1;
            labrador::Application app(options);
            app.initialize();
            handle = static_cast<HWND>(app.window());
            DWORD owner = 0;
            require(handle != nullptr && IsWindow(handle) != FALSE &&
                GetWindowThreadProcessId(handle, &owner) != 0 && owner == GetCurrentProcessId(),
                "hidden application window is not live and owned");
            require(IsWindowVisible(handle) == FALSE, "application window became visible");
            auto& renderer = *app.renderer();
            require(renderer.device_info().backend == "d3d11", "fixture requires the checked D3D11 build");
            const auto size = renderer.back_buffer_size();
            require(size.x == 1280.0f && size.y == 720.0f, "unexpected hidden buffer size");

            labrador::TextureData white;
            white.width = 1;
            white.height = 1;
            white.pixels = { 255u, 255u, 255u, 255u };
            white.levels.push_back(labrador::texture_level(white.format, 1, 1, 0));
            labrador::add_texture_asset(renderer, *app.render_resources(), "synthetic:white", white);
            const auto texture = app.render_resources()->resolve_texture("synthetic:white");
            app.push(std::make_unique<SyntheticState>(texture, counts));
            require(app.depth() == 1 && counts.initializes == 1, "synthetic stack was not installed");

            const auto red = redraw_and_read(app, counts);
            require_colour(red, 255u, 0u, 0u);
            counts.blue = true;
            const auto blue = redraw_and_read(app, counts);
            require_colour(blue, 0u, 0u, 255u);
            require_colour(red, 255u, 0u, 0u);
            require(red != blue, "two deliberate draw states returned the same buffer");
            require(counts.draws == 2 && counts.readbacks == 2 && counts.updates == 0,
                "capture did not perform exactly two stack draws without updates");

            // Model the host's draw/submit/end-marker sequence without present.
            // The top State captures the third world draw without redrawing it.
            counts.blue = false;
            app.push(std::make_unique<FinalCaptureState>(final_capture, counts));
            require(app.depth() == 2 && counts.suspends == 1, "capture overlay did not suspend the world");
            renderer.begin_frame();
            renderer.begin_marker(L"FinalStateCapture");
            app.draw(renderer);
            require(final_capture.captured && final_capture.captures == 1 &&
                final_capture.world_draws_at_capture == 3 && counts.draws == 3,
                "final-State capture did not follow exactly one world draw");
            renderer.submit();
            renderer.end_marker();
            std::vector<unsigned char> after_outer_submit;
            renderer.read_back_buffer(after_outer_submit);
            ++counts.readbacks;
            require_colour(final_capture.pixels, 255u, 0u, 0u);
            require(after_outer_submit == final_capture.pixels && counts.draws == 3,
                "outer idempotent submit/end-marker changed the captured composition");

            static_cast<labrador::StateContext&>(app).update(0.0f);
            require(final_capture.updates == 1 && final_capture.depth_while_pop_pending == 2 &&
                final_capture.destroyed == 1 && app.depth() == 1 && counts.resumes == 1,
                "capture overlay pop was not deferred through update return and world resume");
            require(counts.updates == 0 && counts.draws == 3 && counts.readbacks == 4,
                "capture overlay update unexpectedly advanced the underlying world");
            require(IsWindowVisible(handle) == FALSE, "capture showed its application window");
            std::cout << "FRAME_READBACK buffer=1280x720 first=red second=blue draws=" << counts.draws
                << " updates=" << counts.updates << " readbacks=" << counts.readbacks
                << " end_frame_calls=0 app_run_calls=0 hidden=1 backend="
                << renderer.device_info().backend << '\n';
            std::cout << "FINAL_STATE captures=1 additional_world_draws=1 outer_submit_unchanged=1"
                << " deferred_pop_depth=2 after_pop_depth=1 world_suspends=1 world_resumes=1"
                << " consumed_updates=1 world_updates=0\n";
            app.clear();
        }
        require(IsWindow(handle) == FALSE, "application teardown left its window alive");
        std::cout << "PASS out_of_band_stack_redraw=1 final_state_capture_without_redraw=1"
            << " rgba_oracles=3 saved_first_copy_unchanged=1"
            << " presented_or_displayed_tested=0\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "frame readback illustration failed: " << error.what() << '\n';
        return 1;
    }
}
