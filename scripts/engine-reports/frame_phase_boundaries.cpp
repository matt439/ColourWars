// Optional ENG#14 illustration of the existing public CPU call boundaries.
// This owns a MANUAL loop; it does not observe Application's private frame loop.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include "engine/app/application.h"

#include <array>
#include <chrono>
#include <cmath>
#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace
{
    using Clock = std::chrono::steady_clock;
    struct Counts { int init = 0; int update = 0; int draw = 0; };
    class EmptyState final : public labrador::State
    {
    public:
        explicit EmptyState(Counts& counts) : counts_(counts) {}
        void init() override { ++this->counts_.init; }
        void update(float) override { ++this->counts_.update; }
        void draw(labrador::Renderer& renderer) const override
        {
            ++this->counts_.draw;
            renderer.set_view_count(1);
        }
    private:
        Counts& counts_;
    };
    double wall_ms(Clock::time_point begin, Clock::time_point end)
    {
        const double value = std::chrono::duration<double, std::milli>(end - begin).count();
        if (!std::isfinite(value) || value < 0.0)
        {
            throw std::runtime_error("nonfinite or negative observed wall duration");
        }
        return value;
    }
    struct Sample
    {
        double state_update = 0.0;
        double prepare_record = 0.0;
        double submit = 0.0;
        double end_frame = 0.0;
    };
}

int main()
{
    try
    {
        labrador::ApplicationOptions options;
        options.window_class_name = L"LabradorManualFramePhaseExhibit";
        options.window_title = L"Hidden manual CPU call-boundary illustration";
        options.visible = false;
        options.fullscreen = false;
        options.min_threads = 1;
        options.max_threads = 1;
        options.view_capacity = 1;
        labrador::Application app(options);
        app.initialize();
        const HWND handle = static_cast<HWND>(app.window());
        DWORD owner = 0;
        if (handle == nullptr || IsWindow(handle) == FALSE ||
            GetWindowThreadProcessId(handle, &owner) == 0 || owner != GetCurrentProcessId() ||
            IsWindowVisible(handle) != FALSE)
        {
            throw std::runtime_error("expected an owned, live hidden Application window");
        }
        Counts counts;
        labrador::StateContext manual_context;
        manual_context.transition_to(std::make_unique<EmptyState>(counts));
        auto& renderer = *app.renderer();
        std::array<Sample, 3> samples{};
        for (Sample& sample : samples)
        {
            const auto begin = Clock::now();
            manual_context.update(1.0f / 60.0f);
            const auto after_update = Clock::now();
            renderer.begin_frame();
            manual_context.draw(renderer);
            const auto after_record = Clock::now();
            renderer.submit();
            const auto after_submit = Clock::now();
            renderer.end_frame();
            const auto after_end = Clock::now();
            sample = Sample{wall_ms(begin, after_update), wall_ms(after_update, after_record),
                wall_ms(after_record, after_submit), wall_ms(after_submit, after_end)};
        }
        if (counts.init != 1 || counts.update != 3 || counts.draw != 3 ||
            IsWindowVisible(handle) != FALSE)
        {
            throw std::runtime_error("manual call counts or hidden window changed");
        }
        std::cout << "MANUAL_PUBLIC_SEAM backend=" << renderer.device_info().backend
            << " hidden=1 assets=0 app_run_calls=0 app_phase_stats_observed=0 gpu_timed=0\n"
            << "manual_iteration,state_update_cpu_wall_ms,prepare_record_cpu_wall_ms,"
            << "submit_call_cpu_wall_ms,end_frame_call_cpu_wall_ms\n" << std::setprecision(17);
        int iteration = 0;
        for (const Sample& sample : samples)
        {
            std::cout << ++iteration << ',' << sample.state_update << ',' << sample.prepare_record
                << ',' << sample.submit << ',' << sample.end_frame << '\n';
        }
        std::cout << "PASS manual_context_updates=3 manual_draws=3"
            << " application_frame_interval_measured=0 timing_thresholds=0\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "manual phase illustration failed: " << error.what() << '\n';
        return 1;
    }
}
