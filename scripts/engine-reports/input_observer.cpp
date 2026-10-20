#include "engine/core/state.h"
#include "engine/core/state_context.h"
#include "engine/input/keyboard.h"

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>
#include <stdexcept>

using namespace labrador;

namespace
{
    constexpr float elapsed = 1.0f / 60.0f;

    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    void prime(Keyboard& keyboard)
    {
        keyboard.set_focused(true);
        keyboard.poll();
        keyboard.poll();
    }

    struct Counts
    {
        int updates = 0;
        int presses = 0;
        int releases = 0;
    };

    // Client policy, deliberately not an imagined engine capture API.
    struct Gate
    {
        bool active = false;
        bool closing_frame = false;
        bool blocks_reader() const { return active || closing_frame; }
    };

    class ReaderState final : public State
    {
    public:
        ReaderState(const Keyboard& keyboard, Counts& counts, const Gate* gate = nullptr) :
            keyboard_(keyboard), counts_(counts), gate_(gate) {}

        void update(float) override
        {
            ++this->counts_.updates;
            if (this->gate_ != nullptr && this->gate_->blocks_reader()) { return; }
            if (this->keyboard_.pressed(Key::space)) { ++this->counts_.presses; }
            if (this->keyboard_.released(Key::space)) { ++this->counts_.releases; }
        }
        void init() override {}
        void draw(Renderer&) const override {}
        bool covers_screen() const override { return false; }

    private:
        const Keyboard& keyboard_;
        Counts& counts_;
        const Gate* gate_;
    };

    // A feasible alternative using existing APIs: the application sees one
    // permanent state, and this client host schedules an overlay before its
    // own nested stack. A real host must also forward its chosen lifecycle,
    // draw and result behavior. This example does not create/render a window.
    class LiveHost final : public State
    {
    public:
        LiveHost(const Keyboard& keyboard, Gate& gate, Counts& world, Counts& overlay) :
            keyboard_(keyboard), gate_(gate), overlay_(overlay)
        {
            this->world_.transition_to(std::make_unique<ReaderState>(keyboard, world, &gate));
        }

        void update(float frame_elapsed) override
        {
            // This is explicit client scheduling, not an Application hook.
            if (this->gate_.active)
            {
                ++this->overlay_.updates;
                if (this->keyboard_.pressed(Key::space)) { ++this->overlay_.presses; }
            }
            this->world_.update(frame_elapsed);
            this->gate_.closing_frame = false;
        }
        void init() override {}
        void draw(Renderer& renderer) const override { this->world_.draw(renderer); }

    private:
        const Keyboard& keyboard_;
        Gate& gate_;
        Counts& overlay_;
        StateContext world_;
    };

    void pushed_overlay()
    {
        Keyboard keyboard;
        prime(keyboard);
        Counts world;
        Counts overlay;
        StateContext stack;
        stack.transition_to(std::make_unique<ReaderState>(keyboard, world));
        stack.update(elapsed);
        stack.push(std::make_unique<ReaderState>(keyboard, overlay));
        keyboard.on_key_down(Key::space);
        keyboard.poll();
        stack.update(elapsed);
        require(world.updates == 1 && overlay.updates == 1 && overlay.presses == 1,
            "A pushed non-covering state must suspend the lower update.");
        stack.pop();
        keyboard.on_key_up(Key::space);
        keyboard.poll();
        stack.update(elapsed);
        require(world.updates == 2 && world.releases == 1,
            "Popping must resume the lower state, which reads the raw release.");
        std::puts("PASS pushed non-covering state suspends the lower update");
    }

    void repeated_raw_reads()
    {
        Keyboard keyboard;
        prime(keyboard);
        keyboard.on_key_down(Key::space);
        keyboard.on_text(U' ');
        keyboard.poll();
        const bool overlay_press = keyboard.pressed(Key::space);
        const bool world_press = keyboard.pressed(Key::space);
        require(overlay_press && world_press && keyboard.held(Key::space),
            "Raw snapshots must remain readable by independent consumers.");
        require(keyboard.typed() == " " && keyboard.typed() == " ",
            "Reading typed text must not consume its snapshot.");
        std::puts("PASS independent raw readers observe the same edge and text");
    }

    void client_host_and_gate()
    {
        Keyboard keyboard;
        prime(keyboard);
        Gate gate;
        Counts world;
        Counts overlay;
        StateContext stack;
        stack.transition_to(std::make_unique<LiveHost>(keyboard, gate, world, overlay));
        stack.update(elapsed);

        gate.active = true;
        keyboard.on_key_down(Key::space);
        keyboard.on_text(U' ');
        keyboard.poll();
        stack.update(elapsed);
        require(world.updates == 2 && world.presses == 0 && overlay.presses == 1,
            "A client host can keep the world ticking while its gate blocks an action.");
        require(keyboard.pressed(Key::space),
            "A direct raw reader bypasses the cooperative gate by design.");

        // The client chooses to close on release and suppress that entire
        // closing frame. Other neutral/release policies are intentionally absent.
        gate.active = false;
        gate.closing_frame = true;
        keyboard.on_key_up(Key::space);
        keyboard.poll();
        stack.update(elapsed);
        require(world.updates == 3 && world.releases == 0 && keyboard.released(Key::space),
            "Closing-release suppression belongs to the cooperative client gate.");

        keyboard.on_key_down(Key::space);
        keyboard.poll();
        stack.update(elapsed);
        require(world.updates == 4 && world.presses == 1 && world.releases == 0,
            "A fresh press after capture must reach the resumed reader.");
        std::puts("PASS composed root host and client gate preserve live updates");
    }
}

int main()
{
    try
    {
        pushed_overlay();
        repeated_raw_reads();
        client_host_and_gate();
        std::puts("PASS synthetic overlay scheduling and cooperative input routing");
        std::fflush(stdout);
        return EXIT_SUCCESS;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "FAIL %s\n", error.what());
        return EXIT_FAILURE;
    }
}
