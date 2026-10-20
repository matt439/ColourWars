#include "game/pch.h"
#include "game/states/keyboard_jump_capture.h"
#include "game/states/menu_input.h"
#include "game/input_bindings.h"
#include "game/input_slots.h"
#include "engine/input/gamepad_reader.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>

// The concrete reader seam used by input_bindings acceptance. Every pad is
// absent; these checks need no XInput device, window, renderer or player's save.
namespace labrador
{
    struct GamepadReader::Impl {};
    GamepadReader::GamepadReader() : impl_(std::make_unique<Impl>()) {}
    GamepadReader::~GamepadReader() = default;
    GamepadReader::GamepadReader(GamepadReader&&) noexcept = default;
    GamepadReader& GamepadReader::operator=(GamepadReader&&) noexcept = default;
    void GamepadReader::sample() {}
    GamepadState GamepadReader::read(int) const { return {}; }
    void GamepadReader::suspend() {}
    void GamepadReader::resume() {}
}

using namespace labrador;

namespace
{
    using Phase = KeyboardJumpCapture::Phase;
    using Rejection = KeyboardJumpCapture::Rejection;

    void require(bool condition, const std::string& message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    constexpr std::array<Key, 27> binding_keys{{
        Key::a, Key::b, Key::c, Key::d, Key::e, Key::f, Key::g, Key::h, Key::i,
        Key::j, Key::k, Key::l, Key::m, Key::n, Key::o, Key::p, Key::q, Key::r,
        Key::s, Key::t, Key::u, Key::v, Key::w, Key::x, Key::y, Key::z, Key::space
    }};

    bool in_binding_domain(Key key)
    {
        return std::find(binding_keys.begin(), binding_keys.end(), key) != binding_keys.end();
    }

    struct CaptureHarness
    {
        Keyboard keyboard;
        KeyboardJumpCapture capture;

        CaptureHarness()
        {
            this->keyboard.set_focused(true);
            this->keyboard.poll();
            this->keyboard.poll();
            require(this->capture.phase() == Phase::waiting_neutral,
                "New capture skipped its neutral entry gate");
        }

        void frame(bool blocked = false, bool cancel = false)
        {
            this->keyboard.poll();
            this->capture.update(this->keyboard.state(), this->keyboard.previous_state(), blocked, cancel);
        }

        void ready()
        {
            this->frame();
            require(this->capture.phase() == Phase::ready && !this->capture.result(),
                "Observed neutral keyboard did not arm capture");
        }

        void down(Key key) { this->keyboard.on_key_down(key); this->frame(); }
        void up(Key key) { this->keyboard.on_key_up(key); this->frame(); }

        void no_choice(const char* message) const
        {
            require(!this->capture.result() && this->capture.candidate() == Key::none, message);
        }

        void complete(Key key)
        {
            require(this->capture.phase() == Phase::complete && this->capture.result() == key &&
                this->capture.candidate() == key && this->capture.rejection() == Rejection::none,
                "Sole supported key press/release did not complete with that physical key");
            // Terminal snapshots are repeatable even if another consumer asks
            // again on the same frame, or unrelated input arrives later.
            for (int repeat = 0; repeat < 4; ++repeat)
            {
                const auto result = this->capture.result();
                this->capture.update(this->keyboard.state(), this->keyboard.previous_state());
                require(this->capture.phase() == Phase::complete && this->capture.result() == result,
                    "Repeated terminal-frame update changed an accepted result");
            }
            this->keyboard.on_key_down(Key::escape);
            this->frame(true, true);
            this->keyboard.set_focused(false);
            this->frame();
            require(this->capture.phase() == Phase::complete && this->capture.result() == key,
                "Later cancellation or focus loss rewrote a completed result");
        }
    };

    void domain_and_rejection()
    {
        const auto domain = keyboard_jump_keys();
        require(domain == binding_keys, "Public capture domain no longer names exactly A-Z and Space");
        size_t accepted = 0;
        for (const Key key : domain)
        {
            CaptureHarness h;
            h.ready();
            h.down(key);
            if (key == Key::a || key == Key::d || key == Key::s)
            {
                require(h.capture.phase() == Phase::waiting_neutral &&
                    h.capture.rejection() == Rejection::conflict,
                    "A/D/S did not report their existing gameplay conflict");
                h.no_choice("A conflicting gameplay key became a capture candidate");
                h.frame();
                require(h.capture.phase() == Phase::waiting_neutral,
                    "Held conflicting key bypassed neutral drain");
                h.up(key);
                require(h.capture.phase() == Phase::ready && h.capture.rejection() == Rejection::conflict,
                    "Conflict release did not rearm capture while retaining visible rejection feedback");
                h.down(Key::j);
                h.up(Key::j);
                h.complete(Key::j);
                continue;
            }
            require(h.capture.phase() == Phase::holding && h.capture.candidate() == key && !h.capture.result(),
                "A supported unconflicted press did not begin one pending gesture");
            h.keyboard.on_key_down(key); // OS key repeat is another down, not a second gesture.
            h.frame();
            h.frame();
            require(h.capture.phase() == Phase::holding && h.capture.candidate() == key && !h.capture.result(),
                "Key repeat accepted or discarded a still-held candidate");
            h.up(key);
            h.complete(key);
            ++accepted;
        }
        require(accepted == 24, "The explicit A-Z/Space domain lost an unconflicted key");

        size_t unsupported = 0;
        for (unsigned int value = 1; value < static_cast<unsigned int>(Key::count); ++value)
        {
            const Key key = static_cast<Key>(value);
            if (in_binding_domain(key) || key == Key::escape) { continue; }
            CaptureHarness h;
            h.ready();
            h.down(key);
            require(h.capture.phase() == Phase::waiting_neutral &&
                h.capture.rejection() == Rejection::unsupported,
                "An unsupported named physical key was accepted or misclassified");
            h.no_choice("Unsupported key became a capture candidate");
            h.down(Key::j);
            h.up(key);
            require(h.capture.phase() == Phase::waiting_neutral,
                "Releasing unsupported key captured a second key held during drain");
            h.up(Key::j);
            require(h.capture.phase() == Phase::ready, "All-up did not rearm an unsupported-key rejection");
            h.down(Key::k);
            h.up(Key::k);
            h.complete(Key::k);
            ++unsupported;
        }
        std::cout << "PASS keyboard capture domain: 24 choices, A/D/S conflicts, " << unsupported <<
            " unsupported named keys including modifiers; repeat and rejection drain\n";
    }

    void entry_and_chords()
    {
        for (const Key held : { Key::space, Key::r, Key::w, Key::shift })
        {
            CaptureHarness h;
            h.down(held);
            require(h.capture.phase() == Phase::waiting_neutral,
                "A key held when capture opened bypassed its entry gate");
            h.frame();
            h.up(held);
            require(h.capture.phase() == Phase::ready, "Entry release did not establish neutral");
            h.no_choice("Releasing entry input selected a binding");
            h.down(Key::j);
            h.up(Key::j);
            h.complete(Key::j);
        }
        for (const Key second : { Key::k, Key::shift, Key::control, Key::alt, Key::digit_1 })
        {
            for (const bool simultaneous : { false, true })
            {
                for (const bool candidate_up_first : { false, true })
                {
                    CaptureHarness h;
                    h.ready();
                    h.keyboard.on_key_down(Key::j);
                    if (!simultaneous) { h.frame(); }
                    h.keyboard.on_key_down(second);
                    h.frame();
                    require(h.capture.phase() == Phase::waiting_neutral &&
                        h.capture.rejection() == Rejection::ambiguous,
                        "A simultaneous or staggered chord was not rejected");
                    h.up(candidate_up_first ? Key::j : second);
                    h.no_choice("Partial chord release selected a binding");
                    require(h.capture.phase() == Phase::waiting_neutral,
                        "A remaining chord key bypassed all-up drain");
                    h.up(candidate_up_first ? second : Key::j);
                    require(h.capture.phase() == Phase::ready, "Full chord release did not rearm capture");
                    h.down(Key::k);
                    h.up(Key::k);
                    h.complete(Key::k);
                }
            }
        }
        CaptureHarness rollover;
        rollover.ready();
        rollover.down(Key::j);
        rollover.keyboard.on_key_up(Key::j);
        rollover.keyboard.on_key_down(Key::k);
        rollover.frame();
        require(rollover.capture.phase() == Phase::waiting_neutral &&
            rollover.capture.rejection() == Rejection::ambiguous,
            "Same-frame rollover replaced a pending candidate");
        rollover.up(Key::k);
        rollover.no_choice("Rollover release selected a binding");
        rollover.down(Key::j);
        rollover.up(Key::j);
        rollover.complete(Key::j);

        CaptureHarness missed_press;
        missed_press.ready();
        missed_press.keyboard.on_key_down(Key::j);
        missed_press.keyboard.poll(); // Another context consumed the press frame.
        missed_press.frame();
        require(missed_press.capture.phase() == Phase::waiting_neutral,
            "A held key whose press was not observed became a fresh gesture");
        missed_press.up(Key::j);
        missed_press.no_choice("Release of an unobserved press selected a binding");
        std::cout << "PASS keyboard capture entry/chords: held opening keys, both chord timings/release orders, "
            "modifier chords and same-frame rollover require a fresh neutral gesture\n";
    }

    void blocked_and_focus()
    {
        CaptureHarness blocked;
        blocked.frame(true);
        blocked.frame(true);
        require(blocked.capture.phase() == Phase::waiting_neutral,
            "Intercepted held F10 did not block neutral entry");
        blocked.frame();
        require(blocked.capture.phase() == Phase::ready, "Releasing intercepted F10 did not establish neutral");
        blocked.keyboard.on_key_down(Key::j);
        blocked.frame(true);
        require(blocked.capture.phase() == Phase::waiting_neutral &&
            blocked.capture.rejection() == Rejection::ambiguous,
            "A supported key plus intercepted F10 was not a chord");
        blocked.keyboard.on_key_up(Key::j);
        blocked.frame(true);
        blocked.no_choice("Intercepted F10 drain selected a binding");
        blocked.frame();
        blocked.keyboard.on_key_down(Key::f10);
        blocked.frame(true);
        require(blocked.capture.rejection() == Rejection::unsupported,
            "F10 reported through both channels was counted twice as a chord");
        blocked.up(Key::f10);
        blocked.down(Key::j);
        blocked.up(Key::j);
        blocked.complete(Key::j);

        CaptureHarness focus;
        focus.ready();
        focus.down(Key::j);
        focus.keyboard.set_focused(false);
        focus.frame();
        require(focus.capture.phase() == Phase::waiting_neutral && focus.capture.focus_cancelled(),
            "Focus loss did not abandon the pending gesture");
        focus.no_choice("Focus loss was interpreted as a candidate release");
        focus.keyboard.set_focused(true);
        focus.keyboard.on_key_down(Key::j);
        focus.frame();
        focus.frame();
        require(focus.capture.phase() == Phase::waiting_neutral,
            "A key held across focus return became a fresh candidate");
        focus.up(Key::j);
        require(focus.capture.phase() == Phase::ready && focus.capture.focus_cancelled(),
            "Focus return did not wait for observed neutral");
        focus.down(Key::k);
        require(!focus.capture.focus_cancelled(), "A fresh valid gesture retained the old focus-cancel notice");
        focus.up(Key::k);
        focus.complete(Key::k);

        CaptureHarness neutral_focus;
        neutral_focus.ready();
        neutral_focus.capture.focus_lost();
        require(neutral_focus.capture.phase() == Phase::waiting_neutral &&
            neutral_focus.capture.focus_cancelled(), "Explicit focus notification did not reset capture");
        neutral_focus.keyboard.set_focused(false);
        neutral_focus.frame();
        neutral_focus.keyboard.set_focused(true);
        neutral_focus.frame();
        require(neutral_focus.capture.phase() == Phase::waiting_neutral,
            "First focused snapshot established neutral before a fresh focused frame");
        neutral_focus.ready();
        std::cout << "PASS keyboard capture blocking/focus: intercepted F10, dual-channel F10, "
            "focus cancellation, held return and fresh neutral rearming\n";
    }

    void cancellations()
    {
        CaptureHarness escape;
        escape.ready();
        escape.down(Key::j);
        escape.down(Key::escape);
        require(escape.capture.phase() == Phase::cancelling, "Fresh Escape did not cancel a pending choice");
        escape.up(Key::escape);
        require(escape.capture.phase() == Phase::cancelling, "Escape cancellation ignored another held key");
        escape.up(Key::j);
        require(escape.capture.phase() == Phase::cancelled, "Escape cancellation did not finish after all-up");
        escape.no_choice("Escape cancellation returned a binding");
        escape.down(Key::k);
        escape.up(Key::k);
        require(escape.capture.phase() == Phase::cancelled && !escape.capture.result(),
            "Repeated input reopened a terminal cancellation");
        escape.capture.reset();
        require(escape.capture.phase() == Phase::waiting_neutral &&
            escape.capture.rejection() == Rejection::none && !escape.capture.focus_cancelled(),
            "Reset retained a terminal result or previous cancellation state");
        escape.ready();
        escape.down(Key::j);
        escape.up(Key::j);
        escape.complete(Key::j);

        CaptureHarness entry_escape;
        entry_escape.keyboard.on_key_down(Key::escape);
        entry_escape.keyboard.poll(); // It was already held before capture began sampling.
        entry_escape.frame();
        require(entry_escape.capture.phase() == Phase::waiting_neutral,
            "An entry-held Escape cancelled without a fresh press");
        entry_escape.up(Key::escape);
        require(entry_escape.capture.phase() == Phase::ready && !entry_escape.capture.result(),
            "Entry-held Escape release did not rearm without a choice");

        CaptureHarness waiting_escape;
        waiting_escape.down(Key::space); // Opening menu key still held at entry.
        waiting_escape.down(Key::escape);
        require(waiting_escape.capture.phase() == Phase::cancelling,
            "Fresh Escape could not cancel while waiting for entry input to become neutral");
        waiting_escape.up(Key::escape);
        waiting_escape.up(Key::space);
        require(waiting_escape.capture.phase() == Phase::cancelled && !waiting_escape.capture.result(),
            "Entry cancellation did not drain every held key before finishing");

        CaptureHarness owner_back;
        owner_back.ready();
        owner_back.down(Key::j);
        owner_back.frame(true, true); // Owning pad's Back release, while F10 is still held.
        require(owner_back.capture.phase() == Phase::cancelling, "Owner Back did not request cancellation");
        owner_back.keyboard.on_key_up(Key::j);
        owner_back.frame(true);
        require(owner_back.capture.phase() == Phase::cancelling,
            "Owner cancellation completed while an intercepted key remained held");
        owner_back.frame();
        require(owner_back.capture.phase() == Phase::cancelled && !owner_back.capture.result(),
            "Owner cancellation did not complete after every input was neutral");
        CaptureHarness neutral_back;
        neutral_back.ready();
        neutral_back.frame(false, true);
        require(neutral_back.capture.phase() == Phase::cancelled && !neutral_back.capture.result(),
            "Owner Back at neutral did not complete cancellation");
        std::cout << "PASS keyboard capture cancellation: fresh/entry-held Escape, owner Back, "
            "all-up drain, terminal stability and reset\n";
    }

    void menu_ownership(const char* content)
    {
        const GameTuning tuning((std::filesystem::path(content) / "tuning.json").string().c_str());
        for (const Key key : { Key::space, Key::r, Key::w })
        {
            GamepadReader reader;
            Gamepads pads(&reader);
            CaptureHarness h;
            MenuInput menu(&pads, &h.keyboard, &tuning);
            const auto read_menu = [&]
            {
                menu.update_directions(1.0f / 60.0f, 0.4f, 0.1f);
                const auto first = menu.menu_inputs()[input_slots::keyboard_and_mouse];
                const auto again = menu.menu_inputs()[input_slots::keyboard_and_mouse];
                require(first.action == again.action && first.direction == again.direction,
                    "Repeated menu reads changed a captured-key exit frame");
                return first;
            };
            std::ignore = read_menu();
            h.keyboard.on_key_down(key);
            h.keyboard.poll();
            std::ignore = read_menu(); // The old menu owned the opening key press.
            menu.reset_actions();
            h.capture.reset();
            h.frame();
            require(h.capture.phase() == Phase::waiting_neutral,
                "Opening menu key became a capture candidate");
            h.up(key);
            const auto entry_release = read_menu();
            require(entry_release.action == MenuInputAction::none && entry_release.direction == Direction::none,
                "Opening key release escaped into menu actions or navigation");

            h.down(key);
            h.up(key);
            require(h.capture.phase() == Phase::complete && h.capture.result() == key,
                "Menu-alias key was unavailable as a fresh capture choice");
            menu.reset_actions(); // The page does this before returning ownership.
            const auto exit_release = read_menu();
            require(exit_release.action == MenuInputAction::none && exit_release.direction == Direction::none,
                "Captured Space/R/W release activated the underlying menu");
            h.keyboard.on_key_down(key);
            h.keyboard.poll();
            const auto fresh_press = read_menu();
            require(fresh_press.action == MenuInputAction::none &&
                fresh_press.direction == (key == Key::w ? Direction::up : Direction::none),
                "Capture exit prevented a fresh menu press or fired a release action early");
            h.keyboard.on_key_up(key);
            h.keyboard.poll();
            const auto fresh_release = read_menu();
            const MenuInputAction expected = key == Key::space ? MenuInputAction::proceed :
                key == Key::r ? MenuInputAction::shuffle : MenuInputAction::none;
            require(fresh_release.action == expected && fresh_release.direction == Direction::none,
                "Capture changed fixed Space/R/W menu controls after a fresh gesture");
        }
        std::cout << "PASS keyboard capture menu ownership: entry/exit reset suppresses Space/R/W releases; "
            "fresh fixed menu actions and navigation rearm\n";
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: keyboard_jump_capture.exe <staged content directory>");
        domain_and_rejection();
        entry_and_chords();
        blocked_and_focus();
        cancellations();
        menu_ownership(argv[1]);
        std::cout.flush();
        std::fputs("PASS keyboard Jump capture: all five check groups completed\n", stdout);
        require(std::fflush(stdout) == 0, "Could not flush the completed capture acceptance receipt");
        return EXIT_SUCCESS;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL keyboard Jump capture: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
