#include "game/pch.h"
#include "game/presentation.h"
#include "game/states/wrapped_text_drop_shadow.h"
#include "game/text_wrap.h"
#include "engine/render/font.h"
#include "engine/render/render_resources.h"
#include "engine/render/sprite_font_file.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace labrador;

// text_wrap and WrappedTextDropShadow against the engine's own measurement,
// with no device: a monospace stand-in font for exact greedy breaks, and the
// shipped atlases for the weapon descriptions the game wraps today.
namespace
{
    void require(bool condition, const std::string& message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    template<class Call>
    std::string thrown(Call call)
    {
        try { call(); }
        catch (const std::exception& error) { return error.what(); }
        return {};
    }

    // Every printable glyph exactly ten texels wide with no bearing or
    // adjustment, so a string of n characters measures 10n.
    void add_monospace(RenderResources& render)
    {
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            glyphs.push_back({ character, mattmath::RectangleI(0, 0, 10, 16), 0.0f, 0.0f, 0.0f });
        }
        render.add_font("wrap_mono", std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
    }

    void check_greedy_breaks(const RenderResources& render)
    {
        const FontHandle mono = render.resolve_sprite_font("wrap_mono");
        require(render.measure_text(mono, L"abcde").x == 50.0f, "The monospace fixture no longer measures 10 per character");

        // "aaa bbb ccc" at 70: "aaa bbb" is exactly 70 and fits; "ccc" does not join it.
        require(text_wrap::wrap(render, mono, L"aaa bbb ccc", 70.0f) == L"aaa bbb\nccc",
            "Greedy wrap did not fill a line to exactly its width or broke at the wrong space");
        require(text_wrap::wrap(render, mono, L"aaa bbb ccc", 69.0f) == L"aaa\nbbb\nccc",
            "Greedy wrap let a line exceed its width by one unit");
        require(text_wrap::wrap(render, mono, L"aaa bbb ccc", 110.0f) == L"aaa bbb ccc",
            "Text that fits was changed");

        // Authored breaks are kept and never joined, even with room to spare.
        require(text_wrap::wrap(render, mono, L"aa\nbb", 1000.0f) == L"aa\nbb",
            "An authored line break was removed");
        require(text_wrap::wrap(render, mono, L"aaa bbb\nccc ddd eee", 70.0f) == L"aaa bbb\nccc ddd\neee",
            "Wrapping after an authored break did not restart the line");
        require(text_wrap::wrap(render, mono, L"\n\naa\n", 30.0f) == L"\n\naa\n",
            "Empty authored lines were not kept");

        // Spaces: the one at a break is dropped, every other one is kept.
        require(text_wrap::wrap(render, mono, L"  aa  bb", 1000.0f) == L"  aa  bb",
            "Leading or repeated spaces in a fitting line changed");
        require(text_wrap::wrap(render, mono, L"aaa  bbb", 50.0f) == L"aaa\nbbb",
            "The spaces at a break were not dropped");
        require(text_wrap::wrap(render, mono, L"aa ", 1000.0f) == L"aa ",
            "Trailing spaces changed");
        require(text_wrap::wrap(render, mono, L"", 10.0f).empty(), "Empty text did not stay empty");

        // A word wider than the line stays whole on its own line.
        require(text_wrap::wrap(render, mono, L"a wwwwwwww b", 50.0f) == L"a\nwwwwwwww\nb",
            "An over-long word was split or joined to its neighbours");

        // Scale multiplies every measured width.
        require(text_wrap::wrap(render, mono, L"aaa bbb ccc", 140.0f, 2.0f) == L"aaa bbb\nccc",
            "The wrap width did not apply at the drawn scale");
        require(text_wrap::wrap(render, mono, L"aaa bbb ccc", 35.0f, 0.5f) == L"aaa bbb\nccc",
            "The wrap width did not apply at a reduced scale");

        for (const float bad : { 0.0f, -1.0f, std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::quiet_NaN() })
        {
            require(!thrown([&] { std::ignore = text_wrap::wrap(render, mono, L"a", bad); }).empty(),
                "wrap accepted a width that is not finite and positive");
        }
    }

    void check_fits(const RenderResources& render)
    {
        const FontHandle mono = render.resolve_sprite_font("wrap_mono");
        text_wrap::check_fits(render, mono, L"aaaaa bb\ncc", 50.0f, 1.0f, "fixture");
        const std::string wide = thrown([&]
            { text_wrap::check_fits(render, mono, L"ok\nfine wwwwww", 50.0f, 1.0f, "the fixture key"); });
        require(wide.find("the fixture key") != std::string::npos && wide.find("'wwwwww'") != std::string::npos &&
            wide.find("60") != std::string::npos && wide.find("50") != std::string::npos,
            "check_fits did not name the key, the word and both widths: " + wide);
        require(!thrown([&] { text_wrap::check_fits(render, mono, L"aaa", 50.0f, 2.0f, "scaled"); }).empty(),
            "check_fits ignored the drawn scale");
    }

    void check_widget(RenderResources& render)
    {
        WrappedTextDropShadow widget("wrapped", L"aaa bbb ccc", "wrap_mono", { 0.0f, 0.0f }, &render, 70.0f,
            Colour::white, Colour::black, { 1.0f, 1.0f });
        require(widget.text() == L"aaa bbb\nccc" && widget.authored_text() == L"aaa bbb ccc",
            "The widget did not wrap its first string");
        require(widget.bounds().width <= 70.0f + 1.0f, "The wrapped widget measures wider than its width plus shadow");
        widget.set_text(L"dd ee ff gg");
        require(widget.text() == L"dd ee\nff gg", "set_text did not wrap");
        widget.set_scale(2.0f);
        require(widget.text() == L"dd\nee\nff\ngg", "set_scale did not wrap the authored text again");
        widget.set_scale(1.0f);
        require(widget.text() == L"dd ee\nff gg", "Returning to scale 1 did not restore the original breaks");
        require(widget.max_width() == 70.0f, "The widget lost its width");
    }

    rapidjson::Document read_document(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        require(input.good(), "Cannot read " + path.string());
        const std::string text{ std::istreambuf_iterator<char>(input), {} };
        rapidjson::Document result;
        result.Parse(text.c_str());
        require(!result.HasParseError(), "Cannot parse " + path.string());
        return result;
    }

    std::string presentation_rejection(const std::filesystem::path& content,
        const std::filesystem::path& scratch, const char* name, void (*edit)(rapidjson::Document&))
    {
        rapidjson::Document document = read_document(content / "presentation.json");
        edit(document);
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        require(document.Accept(writer), "Cannot encode text wrap fixture");
        const auto path = scratch / (std::string("text-wrap-") + name + ".json");
        {
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            output.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
            require(output.good(), "Cannot write text wrap fixture");
        }
        return thrown([&] { const Presentation rejected(path.string().c_str()); std::ignore = rejected; });
    }

    // The shipped column: every description keeps exactly its authored breaks,
    // so turning wrapping on moved no pixel, and the reader refuses no width.
    void check_shipped(RenderResources& render, const std::filesystem::path& content,
        const std::filesystem::path& scratch)
    {
        const Presentation shipped((content / "presentation.json").string().c_str());
        const MainMenuPresentation& look = shipped.main_menu();
        require(look.weapon_description_max_width == 920.0f, "The shipped weapon description width changed");
        const FontHandle font = render.resolve_sprite_font(look.weapon_description_font);
        float widest = 0.0f;
        for (size_t i = 0; i < look.weapon_descriptions.size(); ++i)
        {
            const std::wstring& description = look.weapon_descriptions[i];
            require(text_wrap::wrap(render, font, description, look.weapon_description_max_width) == description,
                "Wrapping changed shipped weapon description " + std::to_string(i));
            text_wrap::check_fits(render, font, description, look.weapon_description_max_width, 1.0f,
                "shipped weapon description " + std::to_string(i));
            widest = std::max(widest, render.measure_text(font, description).x);
        }
        // The widest authored line stays within the column, with room left.
        require(widest > 0.0f && widest + std::abs(look.weapon_description_shadow_offset.x) <=
            look.weapon_description_max_width, "A shipped description line is wider than its wrap width");
        std::cout << "Widest shipped weapon description line: " << widest << " of " << look.weapon_description_max_width << '\n';

        const std::string missing = presentation_rejection(content, scratch, "missing", [](rapidjson::Document& d)
            { d["main_menu"]["weapon_description"].RemoveMember("max_width"); });
        require(missing.find("max_width") != std::string::npos, "A missing description width was accepted: " + missing);
        const std::string zero = presentation_rejection(content, scratch, "zero", [](rapidjson::Document& d)
            { d["main_menu"]["weapon_description"]["max_width"].SetDouble(0.0); });
        require(zero.find("max_width") != std::string::npos, "A zero description width was accepted: " + zero);
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: text_wrap.exe <staged content directory>");
        const std::filesystem::path content(argv[1]);
        const std::filesystem::path scratch = std::filesystem::absolute(argv[0]).parent_path() / "text-wrap-fixtures";
        std::filesystem::create_directories(scratch);

        RenderResources render;
        add_monospace(render);
        for (const char* name : { "gill_sans_mt_bold_24", "courier_new_bold_16" })
        {
            const SpriteFontFile file = read_sprite_font_file(
                (content / "fonts" / (std::string(name) + ".spritefont")).string());
            render.add_font(name, std::make_unique<Font>(TextureHandle{}, file.glyphs, file.line_spacing));
        }

        check_greedy_breaks(render);
        check_fits(render);
        check_widget(render);
        check_shipped(render, content, scratch);
        std::cout << "PASS text wrap: greedy breaks at the measured width and scale, authored breaks and spaces kept, "
            "over-long words whole and refused at load with their key, the wrapping widget rewraps on text and scale, "
            "and every shipped weapon description draws exactly as authored at its width.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL text wrap: " << error.what() << '\n';
        return 1;
    }
}
