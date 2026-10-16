#include "game/pch.h"
#include "game/presentation.h"
#include "game/states/menu_focus_motion.h"
#include "engine/ui/focus.h"
#include "engine/ui/widget.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <cstdlib>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace labrador;
using namespace mattmath;

namespace
{
    void require(bool condition, const std::string& message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    void require_near(float actual, float expected, const char* message)
    {
        require(std::isfinite(actual) && std::abs(actual - expected) <= 0.00001f, message);
    }

    class ProbeWidget final : public UiWidget
    {
    public:
        explicit ProbeWidget(float y) : UiWidget("focus-probe"), bounds_(40.0f, y, 100.0f, 40.0f) {}
        void update(float) override {}
        void draw(DrawList&) const override {}
        RectangleF bounds() const override { return this->bounds_; }
        void set_colour(const Colour&) override {}
    private:
        RectangleF bounds_;
    };

    void check_motion(const MenuFocusMotionPresentation& look)
    {
        ProbeWidget a(40.0f), b(140.0f), c(240.0f);
        const RectangleF original_a = a.bounds(), original_b = b.bounds(), original_c = c.bounds();
        MenuFocusMotion motion(look);
        const auto settled = [&]
        {
            require(motion.arrow_offset() == 0.0f && motion.press_fraction() == 0.0f,
                "Interrupted or completed menu motion retained an ornament offset");
        };
        motion.press(); settled();
        motion.observe(nullptr); motion.observe(&a); settled();
        motion.observe(&a); motion.update(10.0f, false); settled();
        motion.observe(&b);
        require_near(motion.arrow_offset(), look.arrow_travel, "New focus did not start its bounded inward settle");
        motion.press();
        require_near(motion.press_fraction(), look.press_fraction, "Accepted press did not start its bounded response");
        const float arrow = motion.arrow_offset(), pressed = motion.press_fraction();
        for (const float invalid : { 0.0f, -1.0f, std::numeric_limits<float>::infinity(),
            -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() })
        {
            motion.update(invalid, false);
            require(motion.arrow_offset() == arrow && motion.press_fraction() == pressed,
                "Nonpositive or nonfinite time advanced menu focus motion");
        }
        for (int redraw = 0; redraw < 100; ++redraw)
        {
            const MenuFocusMotion& sample = motion;
            require(sample.arrow_offset() == arrow && sample.press_fraction() == pressed,
                "Repeated const menu motion sampling advanced its clock");
        }
        motion.update(look.focus_duration * 0.5f, false);
        require_near(motion.arrow_offset(), look.arrow_travel * 0.25f,
            "Focus settle did not reach its halfway easing sample");
        motion.update(std::numeric_limits<float>::max(), false); settled();

        MenuFocusMotion one_step(look), split_steps(look);
        for (MenuFocusMotion* state : { &one_step, &split_steps })
        {
            state->observe(&a); state->observe(&b); state->press();
        }
        const float elapsed = std::min(look.focus_duration, look.press_duration) * 0.5f;
        one_step.update(elapsed, false);
        split_steps.update(elapsed * 0.25f, false); split_steps.update(elapsed * 0.75f, false);
        require_near(one_step.arrow_offset(), split_steps.arrow_offset(), "Focus motion changed with update partitioning");
        require_near(one_step.press_fraction(), split_steps.press_fraction(), "Press release changed with update partitioning");

        motion.press(); motion.update(look.press_duration * 0.5f, false);
        require_near(motion.press_fraction(), look.press_fraction * 0.5f, "Press release did not advance on active time");
        motion.press();
        require_near(motion.press_fraction(), look.press_fraction, "Repeated presses accumulated beyond the authored depression");
        motion.observe(&c);
        require_near(motion.arrow_offset(), look.arrow_travel, "Interrupted focus did not retarget immediately");
        require(motion.press_fraction() == 0.0f, "A previous row's press moved with the cursor");
        motion.observe(&b);
        require_near(motion.arrow_offset(), look.arrow_travel, "Rapid navigation stacked or lost its focus response");
        motion.reset(); settled();
        motion.observe(&b); settled();
        motion.observe(&c);
        require_near(motion.arrow_offset(), look.arrow_travel, "Reset forgot the retained cursor identity");
        motion.observe(nullptr); settled();
        motion.press(); settled();
        motion.observe(&a); settled();

        motion.observe(&b); motion.press();
        motion.update(0.0f, true); settled();
        motion.observe(&c); motion.press(); settled();
        motion.reset(); motion.observe(&a); motion.press(); settled();
        motion.update(std::numeric_limits<float>::quiet_NaN(), true); settled();
        motion.update(0.0f, false); settled();
        motion.observe(&a); settled();
        motion.observe(&b); motion.press();
        require(motion.arrow_offset() > 0.0f && motion.press_fraction() > 0.0f,
            "Restoring effects failed to allow fresh menu events");
        motion.update(0.5f, false); settled();

        // Drive the real focus group and actions. The ornament observes the
        // eligible result, without modifying navigation bounds or dispatch.
        FocusGroup focus;
        int activations = 0;
        focus.add(&a); focus.add(&b, [&] { ++activations; }); focus.add(&c, [&] { ++activations; });
        const auto observe = [&]
        {
            UiWidget* widget = focus.focused(0);
            motion.observe(widget != nullptr && !widget->hidden() && focus.enabled(widget) ? widget : nullptr);
        };
        motion.observe(nullptr); observe(); settled();
        require(focus.activate(0) == Activation::none && activations == 0,
            "Focus motion fixture changed an actionless setting row");
        require(focus.set_enabled(&b, false) && focus.move(0, Direction::down) && focus.focused(0) == &c,
            "Focus motion fixture failed to skip its disabled row");
        observe(); require(motion.arrow_offset() > 0.0f, "Real focus movement did not start the ornament");
        require(focus.activate(0) == Activation::ran && activations == 1, "Actual enabled activation was not dispatched once");
        motion.press(); require(motion.press_fraction() > 0.0f, "Dispatched activation did not permit press feedback");
        focus.set_enabled(&c, false); observe(); settled();
        require(focus.activate(0) == Activation::refused && activations == 1,
            "Disabled activation ran while testing menu feedback");
        focus.set_enabled(&c, true); observe(); settled();
        c.set_hidden(true); observe(); settled(); c.set_hidden(false); observe(); settled();
        require(a.bounds() == original_a && b.bounds() == original_b && c.bounds() == original_c,
            "Cosmetic menu motion changed actual widget navigation geometry");

        MenuFocusMotionPresentation changed = look;
        MenuFocusMotion captured(changed);
        changed.arrow_travel = 0.0f; changed.press_fraction = 0.0f;
        MenuFocusMotion reopened(changed);
        for (MenuFocusMotion* state : { &captured, &reopened }) { state->observe(&a); state->observe(&b); state->press(); }
        require_near(captured.arrow_offset(), look.arrow_travel, "Live presentation replacement changed a captured motion configuration");
        require_near(captured.press_fraction(), look.press_fraction, "Captured press configuration changed after replacement");
        require(reopened.arrow_offset() == 0.0f && reopened.press_fraction() == 0.0f,
            "A reopened motion model ignored its new zero-amplitude configuration");

        std::srand(415);
        const int expected_random = std::rand();
        std::srand(415);
        motion.observe(&a); motion.press(); motion.update(0.01f, false);
        static_cast<void>(motion.arrow_offset()); static_cast<void>(motion.press_fraction());
        require(std::rand() == expected_random, "Cosmetic menu motion consumed gameplay randomness");
    }

    void check_direct_configuration(const MenuFocusMotionPresentation& defaults)
    {
        const auto rejects = [&](const MenuFocusMotionPresentation& invalid)
        {
            bool rejected = false;
            try { const MenuFocusMotion motion(invalid); }
            catch (const std::invalid_argument&) { rejected = true; }
            require(rejected, "Direct menu motion construction accepted malformed limits");
        };
        for (const float value : { -1.0f, 0.0f, 0.5001f, std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::quiet_NaN() })
        {
            auto invalid = defaults; invalid.focus_duration = value; rejects(invalid);
            invalid = defaults; invalid.press_duration = value; rejects(invalid);
        }
        for (const float value : { -0.01f, 2.001f, std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::quiet_NaN() })
        {
            auto invalid = defaults; invalid.arrow_travel = value; rejects(invalid);
        }
        for (const float value : { -0.01f, 1.001f, std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::quiet_NaN() })
        {
            auto invalid = defaults; invalid.press_fraction = value; rejects(invalid);
        }
    }

    struct Variants
    {
        rapidjson::Document source;
        std::filesystem::path scratch;
        size_t next = 0;

        Variants(const std::filesystem::path& content, const std::filesystem::path& output) : scratch(output)
        {
            std::ifstream file(content / "presentation.json", std::ios::binary);
            require(file.good(), "Cannot read presentation for menu motion fixtures");
            const std::string text{ std::istreambuf_iterator<char>(file), {} };
            this->source.Parse(text.c_str());
            require(!this->source.HasParseError(), "Cannot parse menu motion fixture source");
            std::filesystem::create_directories(this->scratch);
        }

        template<class Edit>
        std::filesystem::path write(Edit edit)
        {
            rapidjson::Document document; document.CopyFrom(this->source, document.GetAllocator()); edit(document);
            rapidjson::StringBuffer buffer; rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            require(document.Accept(writer), "Cannot encode menu motion fixture");
            const auto path = this->scratch / ("motion-" + std::to_string(this->next++) + ".json");
            std::ofstream file(path, std::ios::binary | std::ios::trunc);
            file.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize())); file.close();
            require(file.good(), "Cannot write menu motion fixture");
            return path;
        }

        template<class Edit>
        void rejects(const char* key, Edit edit)
        {
            const auto path = this->write(edit);
            bool contextual = false;
            try { const Presentation invalid(path.string().c_str()); }
            catch (const std::runtime_error& error)
            {
                const std::string message = error.what();
                contextual = message.find(path.filename().string()) != std::string::npos &&
                    message.find("menu_style") != std::string::npos && message.find("motion") != std::string::npos &&
                    message.find(key) != std::string::npos;
            }
            require(contextual, std::string("Invalid menu motion did not identify its source and field: ") + key);
        }
    };

    void check_parser(Variants& variants)
    {
        variants.rejects("motion", [](rapidjson::Document& d) { d["menu_style"].RemoveMember("motion"); });
        variants.rejects("motion", [](rapidjson::Document& d) { d["menu_style"]["motion"].SetArray(); });
        for (const char* key : { "focus_duration", "press_duration", "arrow_travel", "press_fraction" })
        {
            variants.rejects(key, [key](rapidjson::Document& d) { d["menu_style"]["motion"].RemoveMember(key); });
            variants.rejects(key, [key](rapidjson::Document& d) { d["menu_style"]["motion"][key].SetString("fast"); });
            variants.rejects(key, [key](rapidjson::Document& d) { d["menu_style"]["motion"][key].SetBool(false); });
            variants.rejects(key, [key](rapidjson::Document& d) { d["menu_style"]["motion"][key].SetDouble(-0.01); });
            variants.rejects(key, [key](rapidjson::Document& d) { d["menu_style"]["motion"][key].SetDouble(1e100); });
        }
        for (const char* key : { "focus_duration", "press_duration" })
        {
            variants.rejects(key, [key](rapidjson::Document& d) { d["menu_style"]["motion"][key].SetDouble(0.0); });
            variants.rejects(key, [key](rapidjson::Document& d) { d["menu_style"]["motion"][key].SetDouble(0.5001); });
        }
        variants.rejects("arrow_travel", [](rapidjson::Document& d) { d["menu_style"]["motion"]["arrow_travel"].SetDouble(2.001); });
        variants.rejects("press_fraction", [](rapidjson::Document& d) { d["menu_style"]["motion"]["press_fraction"].SetDouble(1.001); });
        const auto path = variants.write([](rapidjson::Document& d)
        {
            auto& motion = d["menu_style"]["motion"];
            motion["focus_duration"].SetDouble(0.5); motion["press_duration"].SetDouble(0.5);
            motion["arrow_travel"].SetInt(2); motion["press_fraction"].SetInt(1);
        });
        const Presentation boundary(path.string().c_str());
        const auto& motion = boundary.menu().motion;
        require(motion.focus_duration == 0.5f && motion.press_duration == 0.5f &&
            motion.arrow_travel == 2.0f && motion.press_fraction == 1.0f,
            "Inclusive menu motion parser bounds rejected or changed valid authored values");
        const auto quiet_path = variants.write([](rapidjson::Document& d)
        {
            d["menu_style"]["motion"]["arrow_travel"].SetInt(0);
            d["menu_style"]["motion"]["press_fraction"].SetInt(0);
        });
        const Presentation quiet(quiet_path.string().c_str());
        require(quiet.menu().motion.arrow_travel == 0.0f && quiet.menu().motion.press_fraction == 0.0f,
            "Zero-amplitude menu motion authoring must remain valid");
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: menu_focus_motion.exe <staged content directory>");
        const std::filesystem::path content(argv[1]);
        const Presentation presentation((content / "presentation.json").string().c_str());
        const auto& look = presentation.menu().motion;
        require(look.focus_duration == 0.14f && look.press_duration == 0.12f &&
            look.arrow_travel == 1.5f && look.press_fraction == 0.65f, "Authored menu motion defaults changed");
        check_motion(look); check_direct_configuration(look);
        Variants variants(content, std::filesystem::absolute(argv[0]).parent_path() / "menu-focus-motion-fixtures");
        check_parser(variants);
        std::cout << "PASS menu focus motion: real focus/activation composition, unchanged navigation bounds, "
            "bounded interrupted/repeated events, invalid/hitch time, const samples, reduced/reset/reopen, "
            "unchanged RNG and contextual presentation limits.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL menu focus motion: " << error.what() << '\n';
        return 1;
    }
}
