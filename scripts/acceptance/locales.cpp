#include "game/pch.h"
#include "game/config/locale.h"
#include "game/config/presentation/presentation.h"
#include "game/config/tuning.h"
#include "game/states/menu_content_validation.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/audio/audio_resources.h"
#include "engine/render/sprite_font_file.h"
#include "scripts/acceptance/support/probe.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

// The locale axis (GAP#L11, docs/localisation.md). Every shipped locale is
// composed over presentation.json and judged by the same checks as the base:
// each string's tokens, each character against the font that draws it, each
// composed line against its box. Then fixtures under the probe's scratch
// folder hold the composition rules: what a locale may change, its fallback
// chain, font substitution, and that a translation the fonts or boxes cannot
// carry is refused naming its locale.
using namespace labrador;
using acceptance::require;

namespace
{
    struct World
    {
        RenderResources render;
        AudioResources audio;

        explicit World(const std::filesystem::path& content)
        {
            for (const char* name : { "sprite_sheet_1", "painter", "menu_paint", "weapon_paint", "paint_fx", "terrain" })
            {
                render.add_sprite_sheet(name, read_sprite_sheet(
                    (content / "textures" / (std::string(name) + ".json")).string().c_str(), TextureHandle{}));
            }
            for (const char* name : { "bowlby_one_144", "lato_black_24",
                "lato_black_36", "lato_black_48", "lato_black_72",
                "lato_black_144", "courier_prime_bold_16" })
            {
                const SpriteFontFile file = read_sprite_font_file(
                    (content / "fonts" / (std::string(name) + ".spritefont")).string());
                auto font = std::make_unique<Font>(TextureHandle{}, file.glyphs, file.line_spacing);
                if (font->contains(U'?')) { font->set_stand_in(U'?'); }
                render.add_font(name, std::move(font));
            }
            audio.add_sound_bank("paint_exe", SoundBank::silent());
        }
    };

    std::string read_text(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        require(input.good(), "Cannot read " + path.string());
        return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
    }

    void write_text(const std::filesystem::path& path, const std::string& text)
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << text;
        output.close();
        require(output.good(), "Cannot write " + path.string());
    }

    // Startup's checks of one composed Presentation that need no stage list.
    void judge(const Presentation& presentation, World& world, const GameTuning& tuning)
    {
        presentation.validate_resources(world.render, world.audio);
        validate_mode_durations(&world.render, tuning, presentation);
        validate_mode_stage_rules(&world.render, presentation);
    }

    // A scratch locales folder: the index and each file, written fresh.
    struct Fixture
    {
        std::filesystem::path folder;

        Fixture(const std::filesystem::path& root, const std::string& name) : folder(root / name)
        {
            std::filesystem::remove_all(folder);
            std::filesystem::create_directories(folder);
        }

        Fixture& index(const std::string& locales)
        {
            write_text(folder / "locales.json",
                R"({ "base": { "tag": "en", "name": "English" }, "locales": [ )" + locales + " ] }");
            return *this;
        }

        Fixture& file(const std::string& tag, const std::string& text)
        {
            write_text(folder / (tag + ".json"), text);
            return *this;
        }

        Locales read() const { return Locales::read(folder.string()); }
    };

    // `action` must throw a runtime_error whose message has every fragment.
    template<class Action>
    void refuses(const char* what, std::initializer_list<const char*> fragments, Action action)
    {
        std::string message = "accepted";
        try
        {
            action();
        }
        catch (const std::exception& error)
        {
            message = error.what();
            bool all = true;
            for (const char* fragment : fragments) { all = all && message.find(fragment) != std::string::npos; }
            if (all) { return; }
        }
        throw std::runtime_error(std::string(what) + " was not refused as expected: " + message);
    }

    std::string one_locale(const std::string& tag, const std::string& extra = "")
    {
        return R"({ "tag": ")" + tag + R"(", "name": "Test")" + extra + " }";
    }

    std::string overlay(const std::string& presentation)
    {
        return R"({ "presentation": )" + presentation + " }";
    }

    void check_shipped(const std::filesystem::path& content, World& world, const GameTuning& tuning)
    {
        const std::string base_path = (content / "presentation.json").string();
        const std::string base_text = read_text(base_path);
        const Locales locales = Locales::read((content / "locales").string());
        require(locales.base().tag == "en" && locales.entries().size() >= 2,
            "The staged game must list the base and at least the test locale.");
        const LocaleEntry* test = locales.find("qps-ploc");
        require(test != nullptr && test->test && test->fallback == "en",
            "qps-ploc must be listed as a test locale falling back to en.");
        locales.check_structure(base_text, base_path);

        // The base is presentation.json byte for byte.
        require(locales.compose("en", base_text, base_path) == base_text,
            "Composing the base locale must return presentation.json unchanged.");

        const Presentation english(locales.presentation("en", base_path));
        for (const LocaleEntry& entry : locales.entries())
        {
            try
            {
                const Presentation localised(locales.presentation(entry.tag, base_path));
                judge(localised, world, tuning);
            }
            catch (const std::exception& error)
            {
                throw std::runtime_error("Locale '" + entry.tag + "' failed startup's checks: " + error.what());
            }
        }

        // The test locale changes copy in every role it may, and nothing else.
        const Presentation pseudo(locales.presentation("qps-ploc", base_path));
        require(pseudo.menu_text().title.start == L"[" + english.menu_text().title.start + L"]",
            "qps-ploc must bracket menu_text.title.start.");
        require(pseudo.hud().labels.health == L"[" + english.hud().labels.health + L"]" &&
            pseudo.hud().labels.team_a == L"[" + english.hud().labels.team_a + L"]",
            "qps-ploc must bracket the HUD's labels, and the HUD must draw them.");
        require(pseudo.menu_text().countdown.get_ready == L"[" + english.menu_text().countdown.get_ready + L"]",
            "qps-ploc must bracket menu_text.countdown.get_ready.");
        require(pseudo.main_menu().weapon_select_art.choice(WeaponType::sprayer).name ==
            L"[" + english.main_menu().weapon_select_art.choice(WeaponType::sprayer).name + L"]",
            "qps-ploc must bracket the Sprayer's name.");
        require(pseudo.team_palette().pairs.front().label != english.team_palette().pairs.front().label,
            "qps-ploc must change a team pair label.");
        require(pseudo.menu().item_font == english.menu().item_font &&
            pseudo.menu().heading_font == english.menu().heading_font,
            "qps-ploc must leave fonts as presentation.json has them.");

        // An unknown request runs the base, and says so on stderr.
        const ActiveLocale unknown(Locales::read((content / "locales").string()), "zz-ZZ");
        require(unknown.is_base() && unknown.tag() == "en", "An unlisted locale must fall back to the base.");
        const ActiveLocale chosen(Locales::read((content / "locales").string()), "qps-ploc");
        require(!chosen.is_base() && chosen.entry().test, "A listed test locale must be selectable.");
    }

    void check_rules(const std::filesystem::path& content, const std::filesystem::path& scratch,
        World& world, const GameTuning& tuning)
    {
        const std::string base_path = (content / "presentation.json").string();
        const std::string base_text = read_text(base_path);

        // No index: the base alone.
        {
            Fixture empty(scratch, "no-index");
            const Locales locales = empty.read();
            require(locales.entries().size() == 1 && locales.base().tag == "en",
                "A folder without locales.json must give only the base locale.");
        }

        // A three-step chain: the nearest locale wins each string, and what
        // neither names comes from presentation.json.
        {
            Fixture chain(scratch, "chain");
            chain.index(one_locale("fr") + ", " + one_locale("fr-CA", R"(, "fallback": "fr")"))
                .file("fr", overlay(R"({ "menu_text": { "title": { "start": "JOUONS", "subtitle": "SALIS TOUT" } } })"))
                .file("fr-CA", overlay(R"({ "menu_text": { "title": { "start": "ON JOUE" } } })"));
            const Locales locales = chain.read();
            const auto steps = locales.chain("fr-CA");
            require(steps.size() == 3 && steps[0]->tag == "fr-CA" && steps[1]->tag == "fr" && steps[2]->tag == "en",
                "fr-CA must fall back to fr, then the base.");
            const Presentation english(base_path.c_str());
            const Presentation canadian(locales.presentation("fr-CA", base_path));
            require(canadian.menu_text().title.start == L"ON JOUE", "The nearest locale must win a string.");
            require(canadian.menu_text().title.subtitle == L"SALIS TOUT", "A missing string must come from the fallback.");
            require(canadian.menu_text().title.no_controller == english.menu_text().title.no_controller,
                "A string no locale names must come from presentation.json.");
            judge(canadian, world, tuning);
        }

        // Fonts: a table substitutes every use of a font presentation.json names.
        {
            Fixture fonts(scratch, "fonts");
            fonts.index(one_locale("de"))
                .file("de", R"({ "fonts": { "lato_black_48": "lato_black_36" }, "presentation": {} })");
            const Presentation german(fonts.read().presentation("de", base_path));
            require(german.menu().item_font == "lato_black_36", "A font table must replace the font it names.");
            judge(german, world, tuning);

            Fixture unknown(scratch, "unknown-font");
            unknown.index(one_locale("de"))
                .file("de", R"({ "fonts": { "comic_sans_48": "lato_black_36" }, "presentation": {} })");
            refuses("A font presentation.json does not use", { "de.json", "comic_sans_48" },
                [&] { (void)unknown.read().compose("de", base_text, base_path); });
        }

        // A byte order mark from the translator's editor is not JSON's business.
        {
            Fixture bom(scratch, "bom");
            bom.index(one_locale("xx"))
                .file("xx", "\xEF\xBB\xBF" + overlay(R"({ "menu_text": { "title": { "start": "GO" } } })"));
            require(Presentation(bom.read().presentation("xx", base_path)).menu_text().title.start == L"GO",
                "A locale file with a byte order mark must still be read.");
        }

        // What a locale may not change.
        const auto composing = [&](const char* name, const std::string& presentation,
            std::initializer_list<const char*> fragments)
        {
            Fixture fixture(scratch, name);
            fixture.index(one_locale("xx")).file("xx", overlay(presentation));
            refuses(name, fragments, [&] { (void)fixture.read().compose("xx", base_text, base_path); });
        };
        composing("unknown-key", R"({ "menu_text": { "title": { "begin": "Go" } } })",
            { "xx.json", "menu_text.title", "begin" });
        composing("not-copy", R"({ "menu_style": { "highlight": "FF0000" } })",
            { "xx.json", "menu_style.highlight", "only text" });
        composing("number", R"({ "menu_style": { "layout": { "position": { "x": 1.0 } } } })",
            { "xx.json", "menu_style.layout.position.x", "a number" });
        composing("shape", R"({ "menu_text": { "title": "Go" } })",
            { "xx.json", "menu_text.title", "an object" });
        composing("array-length", R"({ "team_palette": { "pairs": [ { "label": "A / B" } ] } })",
            { "xx.json", "team_palette.pairs", "same length" });
        composing("pair-key", R"({ "team_palette": { "pairs": [ { "key": "x" }, null, null, null, null, null ] } })",
            { "xx.json", "team_palette.pairs[0].key", "only text" });

        // A null element keeps that element; an object element names its text.
        {
            Fixture pairs(scratch, "pairs");
            pairs.index(one_locale("xx"))
                .file("xx", overlay(R"({ "team_palette": { "pairs": [ null, { "label": "A: Rose / B: Vert" }, null, null, null, null ] } })"));
            const Presentation english(base_path.c_str());
            const Presentation localised(pairs.read().presentation("xx", base_path));
            require(localised.team_palette().pairs[0].label == english.team_palette().pairs[0].label &&
                localised.team_palette().pairs[1].label == L"A: Rose / B: Vert",
                "A pair overlay must change only the elements it names.");
        }

        // The index.
        const auto indexing = [&](const char* name, const std::string& locales,
            std::initializer_list<const char*> fragments)
        {
            Fixture fixture(scratch, name);
            fixture.index(locales).file("aa", overlay("{}")).file("bb", overlay("{}"));
            refuses(name, fragments, [&] { (void)fixture.read(); });
        };
        indexing("bad-tag", one_locale("french"), { "locales.json", "locales[0].tag", "french" });
        indexing("twice", one_locale("aa") + ", " + one_locale("aa"), { "locales.json", "locales[1].tag", "twice" });
        indexing("later-fallback", one_locale("aa", R"(, "fallback": "bb")") + ", " + one_locale("bb"),
            { "locales.json", "locales[0].fallback", "listed above" });
        indexing("missing-file", one_locale("cc"), { "cc.json", "cannot be opened" });
        indexing("undrawable-name", R"({ "tag": "aa", "name": "Fran\u00E7ais" })", { "locales.json", "locales[0].name" });
        {
            Fixture fixture(scratch, "root-key");
            fixture.index(one_locale("aa")).file("aa", R"({ "presentation": {}, "stages": {} })");
            refuses("root-key", { "aa.json", "stages" }, [&] { (void)fixture.read(); });
        }

        // Coverage and fit are Presentation's, and say which locale failed.
        {
            Fixture coverage(scratch, "coverage");
            coverage.index(one_locale("fr"))
                .file("fr", overlay(R"({ "menu_text": { "title": { "start": "C'EST PARTI \u00C0 FOND" } } })"));
            refuses("A character outside the fonts", { "(locale fr)", "menu_text.title", "start", "U+00C0" },
                [&] { static_cast<void>(Presentation(coverage.read().presentation("fr", base_path))); });

            Fixture fit(scratch, "fit");
            fit.index(one_locale("fr"))
                .file("fr", overlay(R"({ "menu_text": { "title": { "start": ")" + std::string(200, 'W') + R"(" } } })"));
            refuses("A string too long for its box", { "(locale fr)" }, [&]
                {
                    const Presentation refused(fit.read().presentation("fr", base_path));
                    judge(refused, world, tuning);
                });
        }
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("locales", [&]
    {
        require(argc == 2, "Usage: locales.exe <staged game directory>");
        const std::filesystem::path content(argv[1]);
        const std::filesystem::path scratch =
            std::filesystem::absolute(argv[0]).parent_path() / "locale-fixtures";
        World world(content);
        const GameTuning tuning((content / "tuning.json").string().c_str());
        check_shipped(content, world, tuning);
        check_rules(content, scratch, world, tuning);
        std::cout << "locales: every shipped locale passes startup's text checks; composition rules hold\n";
        return 0;
    });
}
