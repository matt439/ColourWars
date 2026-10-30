#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"
#include "game/carried/ui_text_drop_shadow.h"

// Connection copy, device and roster labels and the Join/Pause/Team bounds.
namespace menu_text_acceptance
{
    namespace
    {
    void select_connection_font(rapidjson::Document& d, const char* key, const char* font)
    {
        const std::string role(key);
        const char* field = role == "join_reconnect_suffix" ? "item" :
            role == "pause_title" || role == "pause_player_title" || role == "controller_lost" ? "heading" : "detail";
        d["menu_style"]["fonts"][field].SetString(font, d.GetAllocator());
    }

    void compact_connection_copy(rapidjson::Document& d)
    {
        for (const auto& entry : connection_text)
        {
            d["menu_text"]["connection"][entry.key].SetString("Fit");
        }
        d["menu_text"]["connection"]["pause_player_title"].SetString("P{player}");
        for (const char* role : {"heading", "item", "detail"})
        {
            d["menu_style"]["fonts"][role].SetString("menu_text_exact");
        }
    }
    }

    void check_connection_text(Variants& variants, World& world)
    {
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            for (const auto& entry : connection_text)
            {
                d["menu_text"]["connection"][entry.key].SetString(
                    "  A\xC2\xA0\xE2\x80\x9C" "B" "\xE2\x80\x9D\xE2\x80\x94" "C\xE2\x80\xA6  ");
            }
            d["menu_text"]["connection"]["pause_player_title"].SetString(
                "  A\xC2\xA0\xE2\x80\x9C" "B" "\xE2\x80\x9D\xE2\x80\x94{player}\xE2\x80\xA6  ");
        });
        normalized.validate_resources(world.render, world.audio);
        for (const auto& entry : connection_text)
        {
            require(normalized.menu_text().connection.*entry.field ==
                (std::string(entry.key) == "pause_player_title" ? L"  A \"B\"-{player}...  " : L"  A \"B\"-C...  "),
                std::string("Connection copy lost normalization or authored outer spaces: ") + entry.key);
            const std::string field = std::string("connection.") + entry.key;
            for (const char* invalid : {"   ", " \xC2\xA0 ", "first\nsecond", "first\rsecond", "tab\ttext",
                "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3\x28"})
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                    { d["menu_text"]["connection"][entry.key].SetString(invalid, d.GetAllocator()); });
            }
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                select_connection_font(d, entry.key, "menu_text_limited");
                d["menu_text"]["connection"][entry.key].SetString(
                    std::string(entry.key) == "pause_player_title" ? "Glyph~{player}" : "Glyph~", d.GetAllocator());
            }, &world);
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                const std::string long_copy = std::string(2000, 'W') +
                    (std::string(entry.key) == "pause_player_title" ? "{player}" : "");
                d["menu_text"]["connection"][entry.key].SetString(long_copy.c_str(), d.GetAllocator());
            }, &world);
        }
        for (int number = 1; number <= 4; ++number)
        {
            require(normalized.menu_text().pause_title_for_player(number) ==
                L"  A \"B\"-" + std::to_wstring(number) + L"...  ",
                "Pause player formatter changed normalized literals or did not replace its token");
        }
        for (const char* malformed : {"Pause P1", "{Player}", "{player }", "{player}{player}",
            "{player} {seconds}", "{{player}}", "{player} }", "{ {player}", "{player", "player}"})
        {
            variants.rejects("connection.pause_player_title", [&](rapidjson::Document& d)
                { d["menu_text"]["connection"]["pause_player_title"].SetString(malformed, d.GetAllocator()); });
        }
        for (const wchar_t* malformed : {L"Pause P1", L"{player}{player}", L"{player} {other}", L"{{player}}"})
        {
            MenuText mutated = normalized.menu_text();
            mutated.connection.pause_player_title = malformed;
            bool rejected = false;
            try { mutated.pause_title_for_player(1); }
            catch (const std::invalid_argument&) { rejected = true; }
            require(rejected, "Directly mutated Pause formatter accepted an invalid template");
        }
        for (const char* format : {"{player}", "{player} AFTER", "BEFORE {player}", "BEFORE {player} AFTER"})
        {
            const auto copy = variants.load([&](rapidjson::Document& d)
                { d["menu_text"]["connection"]["pause_player_title"].SetString(format, d.GetAllocator()); });
            copy.validate_resources(world.render, world.audio);
            for (int number = 1; number <= 4; ++number)
            {
                const std::wstring expected = std::string(format) == "{player}" ? std::to_wstring(number) :
                    std::string(format) == "{player} AFTER" ? std::to_wstring(number) + L" AFTER" :
                    std::string(format) == "BEFORE {player}" ? L"BEFORE " + std::to_wstring(number) :
                    L"BEFORE " + std::to_wstring(number) + L" AFTER";
                require(copy.menu_text().pause_title_for_player(number) == expected,
                    "Pause token at a supported position changed its literal prefix or suffix");
            }
        }
        for (int number : {(std::numeric_limits<int>::min)(), -1, 0, 5, 99, (std::numeric_limits<int>::max)()})
        {
            bool rejected = false;
            try { normalized.menu_text().pause_title_for_player(number); }
            catch (const std::invalid_argument&) { rejected = true; }
            require(rejected, "Pause title formatter accepted a number outside roster players 1-4");
        }
        // P4 must be validated after substitution, even if the original
        // template's letters/braces and the first three digits are drawable.
        std::vector<Glyph> missing_four;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            if (character != U'4')
            {
                missing_four.push_back({character, mattmath::RectangleI(0, 0, 8, 16), 0.0f, 0.0f, 0.0f});
            }
        }
        world.render.add_font("menu_connection_no_four",
            std::make_unique<Font>(TextureHandle{}, std::move(missing_four), 20.0f));
        variants.rejects("connection.pause_player_title", [](rapidjson::Document& d)
        {
            compact_connection_copy(d);
            select_connection_font(d, "pause_player_title", "menu_connection_no_four");
        }, &world);
    }

    namespace
    {
    template<class Action> void rejects_label_format(Action action)
    {
        bool rejected = false;
        try { action(); }
        catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "Invalid device/roster template or argument was formatted silently");
    }
    }

    void check_label_text(Variants& variants, World& world, const MenuText& defaults)
    {
        // Literal expectations retain the original slot identities and roster
        // numbers independently of either production label formatter.
        constexpr const wchar_t* devices[]{L"Waiting to join", L"Pad 1", L"Pad 2", L"Pad 3", L"Pad 4", L"Keyboard/mouse"};
        for (int slot = -1; slot <= 4; ++slot)
        {
            const std::wstring device = devices[slot + 1];
            require(defaults.device_label(slot) == device, "Default device wording or pad slot numbering changed");
            for (int player = 0; player < 4; ++player)
            {
                require(defaults.roster_label(player, slot) == L"P" + std::to_wstring(player + 1) + L" - " + device,
                    "Default roster label used pad numbering in place of the roster player");
            }
        }
        for (int invalid : {-2, 5, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()})
        {
            rejects_label_format([&] { defaults.device_label(invalid); });
            rejects_label_format([&] { defaults.roster_label(0, invalid); });
        }
        for (int invalid : {-1, 4, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()})
        {
            rejects_label_format([&] { defaults.roster_label(invalid, 0); });
        }
        const auto authored = variants.load([](rapidjson::Document& d)
        {
            auto& labels = d["menu_text"]["labels"];
            labels["pad"].SetString("  Seat\xC2\xA0{pad}  ");
            labels["keyboard_mouse"].SetString("Keys\xE2\x80\x94mouse");
            labels["waiting"].SetString("Open\xE2\x80\xA6");
            labels["roster_row"].SetString("{device} / {player}");
        });
        authored.validate_resources(world.render, world.audio);
        require(authored.menu_text().labels.pad == L"  Seat {pad}  " &&
            authored.menu_text().labels.keyboard_mouse == L"Keys-mouse" &&
            authored.menu_text().labels.waiting == L"Open...",
            "Device labels lost normalized punctuation/NBSP or authored outer spaces");
        for (int slot = -1; slot <= 4; ++slot)
        {
            const std::wstring device = slot == -1 ? L"Open..." : slot == 4 ? L"Keys-mouse" :
                L"  Seat " + std::to_wstring(slot + 1) + L"  ";
            require(authored.menu_text().device_label(slot) == device, "Authored device label was replaced by fixed wording");
            for (int player = 0; player < 4; ++player)
            {
                require(authored.menu_text().roster_label(player, slot) == device + L" / P" + std::to_wstring(player + 1),
                    "Authored roster template lost its original token order or device copy");
            }
            if (slot >= 0 && slot < 4)
            {
                require(authored.menu_text().stick_connection(slot, true) == device + L" - connected" &&
                    authored.menu_text().stick_connection(slot, false) == device + L" - disconnected",
                    "Pad Sticks connection formatter ignored the captured authored device label");
            }
        }
        for (const auto& entry : label_text)
        {
            const std::string field = std::string("labels.") + entry.key;
            for (const char* invalid : {"   ", " \xC2\xA0 ", "first\nsecond", "first\rsecond", "tab\ttext",
                "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3\x28"})
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                    { d["menu_text"]["labels"][entry.key].SetString(invalid, d.GetAllocator()); });
            }
            const bool pad = entry.field == &MenuLabelText::pad;
            const bool row = entry.field == &MenuLabelText::roster_row;
            const std::vector<const char*> malformed = pad ? std::vector<const char*>{
                "Pad", "{Pad}", "{pad }", "{pad}{pad}", "{pad}{device}", "{{pad}}", "{pad", "pad}", "{pa{pad}d}"} :
                row ? std::vector<const char*>{"{player}", "{device}", "{Player}{device}", "{player }{device}",
                    "{player}{player}{device}", "{player}{device}{device}", "{player}{device}{pad}",
                    "{{player}}{device}", "{player}{device", "{de{player}vice}", "{pla{device}yer}"} :
                std::vector<const char*>{"{pad}", "{player}", "{device}", "Plain {", "Plain }", "{}"};
            for (const char* invalid : malformed)
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                    { d["menu_text"]["labels"][entry.key].SetString(invalid, d.GetAllocator()); });
                MenuText mutated = defaults;
                mutated.labels.*entry.field = std::wstring(invalid, invalid + std::strlen(invalid));
                rejects_label_format([&] { mutated.roster_label(3, pad ? 2 : row ? 0 :
                    entry.field == &MenuLabelText::waiting ? -1 : 4); });
                if (!row)
                {
                    rejects_label_format([&] { mutated.device_label(pad ? 2 :
                        entry.field == &MenuLabelText::waiting ? -1 : 4); });
                }
            }
            for (const wchar_t* invalid : {L"", L"   ", L"first\nsecond", L"first\rsecond", L"tab\ttext", L"bad\x7f"})
            {
                MenuText mutated = defaults;
                mutated.labels.*entry.field = invalid;
                rejects_label_format([&] { mutated.roster_label(0, pad ? 0 : row ? 0 :
                    entry.field == &MenuLabelText::waiting ? -1 : 4); });
            }
        }
    }

    void check_label_resources(Variants& variants, World& world)
    {
        for (const auto& entry : label_text)
        {
            const std::string field = std::string("labels.") + entry.key;
            const bool pad = entry.field == &MenuLabelText::pad;
            const bool row = entry.field == &MenuLabelText::roster_row;
            const std::string token = pad ? "{pad}" : row ? "{player}{device}" : "";
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                const std::string text = std::string(500, 'W') + token;
                d["menu_text"]["labels"][entry.key].SetString(text.c_str(), d.GetAllocator());
            }, &world);
            for (const char* role : {"item", "detail", "results"})
            {
                if (entry.field == &MenuLabelText::waiting && std::string(role) != "item") { continue; }
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    if (std::string(role) == "results")
                    {
                        d["results_menu"]["contribution_font"].SetString("menu_text_limited");
                    }
                    else { d["menu_style"]["fonts"][role].SetString("menu_text_limited"); }
                    d["menu_text"]["labels"][entry.key].SetString((token + '~').c_str(), d.GetAllocator());
                }, &world);
            }
        }
        for (const char* role : {"item_font", "help_font"})
        {
            variants.rejects("labels.pad", [&](rapidjson::Document& d)
            {
                d["controls_settings"][role].SetString("menu_text_limited");
                d["menu_text"]["labels"]["pad"].SetString("{pad}~");
            }, &world);
        }
        const auto add_font = [&](const char* name, int tilde_width)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                glyphs.push_back({character, mattmath::RectangleI(0, 0, character == U'~' ? tilde_width : 8, 16),
                    0.0f, 0.0f, 0.0f});
            }
            world.render.add_font(name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        };
        add_font("menu_labels_pause_edge", 489);
        add_font("menu_labels_pause_over", 490);
        const auto pause = [](rapidjson::Document& d, const char* font)
        {
            d["menu_style"]["fonts"]["detail"].SetString(font, d.GetAllocator());
            d["menu_text"]["labels"]["pad"].SetString("{pad}");
            d["menu_text"]["labels"]["keyboard_mouse"].SetString("K");
            d["menu_text"]["labels"]["waiting"].SetString("W");
            d["menu_text"]["labels"]["bot"].SetString("{difficulty}");
            for (const char* role : { "bot_easy", "bot_normal", "bot_hard" })
                { d["menu_text"]["labels"][role].SetString("B"); }
            d["menu_text"]["labels"]["roster_row"].SetString("{player}{device}~");
            d["main_menu"]["team_select"]["spacing"]["x"].SetFloat(273.0f);
            d["main_menu"]["weapon_select"]["spacing"]["x"].SetFloat(273.0f);
        };
        const auto edge = variants.load([&](rapidjson::Document& d) { pause(d, "menu_labels_pause_edge"); });
        edge.validate_resources(world.render, world.audio);
        // Pause is plain UiText. Its three-space separator and LF are part of
        // the complete measured list; a one-row-only bound misses this edge.
        UiText missing("missing", L"P11~   P22~\nP33~   P44~", edge.menu().detail_font,
            mattmath::Vector2F(435.0f, 315.0f), &world.render, Colour::black);
        require(missing.bounds() == mattmath::RectangleF(435.0f, 315.0f, 1050.0f, 40.0f),
            "Complete Pause missing-pad list lost its exact full-paper width, separator or second line");
        variants.rejects("labels.roster_row", [&](rapidjson::Document& d)
            { pause(d, "menu_labels_pause_over"); }, &world);

        add_font("menu_labels_weapon_edge", 466);
        const auto weapon = [](rapidjson::Document& d, bool outside)
        {
            d["menu_style"]["fonts"]["detail"].SetString("menu_labels_weapon_edge");
            d["menu_text"]["labels"]["pad"].SetString("{pad}");
            d["menu_text"]["labels"]["keyboard_mouse"].SetString("K");
            d["menu_text"]["labels"]["waiting"].SetString("W");
            d["menu_text"]["labels"]["bot"].SetString("{difficulty}");
            for (const char* role : { "bot_easy", "bot_normal", "bot_hard" })
                { d["menu_text"]["labels"][role].SetString("B"); }
            d["menu_text"]["labels"]["roster_row"].SetString("{player}{device}~");
            d["main_menu"]["team_select"]["spacing"]["x"].SetFloat(273.0f);
            d["main_menu"]["weapon_select"]["spacing"]["x"].SetFloat(outside ? 249.875f : 250.0f);
        };
        const auto weapon_edge = variants.load([&](rapidjson::Document& d) { weapon(d, false); });
        weapon_edge.validate_resources(world.render, world.audio);
        colourwars::UiTextDropShadow weapon_label("weapon-label", L"P11~", weapon_edge.menu().detail_font,
            mattmath::Vector2F(150.0f, 400.0f), &world.render, Colour::black, Colour());
        colourwars::UiTextDropShadow authored_shadow("team-label", L"P11~", weapon_edge.menu().detail_font,
            mattmath::Vector2F(150.0f, 400.0f), &world.render, Colour::black, Colour(), weapon_edge.menu().item_shadow_offset);
        // The weapon tile starts at 650 minus six units of icon padding and
        // the two-unit rule. Its implicit shadow differs from Team's 1.5.
        require(weapon_label.bounds() == mattmath::RectangleF(150.0f, 400.0f, 492.0f, 22.0f) &&
            authored_shadow.bounds().right() == 641.5f && weapon_label.bounds().right() == 642.0f,
            "Weapon label lost its retained default two-unit shadow at the exact padded tile boundary");
        variants.rejects("labels.roster_row", [&](rapidjson::Document& d) { weapon(d, true); }, &world);
    }

    void check_label_results_extrema(Variants& variants, World& world)
    {
        // Nonuniform ink, advances, bearings and heights exercise complete
        // composed bounds. Tiny bearings also retain sequential float rounding.
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            const bool digit = character >= U'0' && character <= U'9';
            const int width = character == U'8' ? 80 : digit ? 1 : 16;
            glyphs.push_back({character, mattmath::RectangleI(0, 0, width, character == U'8' ? 24 : 16),
                character == U'8' ? 0.00003f : character == U'6' ? -5.0f : 0.0f, 0.0f,
                character == U'8' ? -80.0f : character == U'1' || character == U'7' ? 19.0f : 0.0f});
        }
        world.render.add_font("menu_labels_results_extrema", std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        const Font& font = *world.render.font(world.render.resolve_sprite_font("menu_labels_results_extrema"));
        constexpr const wchar_t* devices[]{L"Pad 1", L"Pad 2", L"Pad 3", L"Pad 4", L"Keyboard/mouse"};
        float maximum = 0.0f, maximum_at_100 = 0.0f;
        std::wstring attained;
        // One font and literal complete strings form an independent oracle for
        // every legal displayed suffix, rather than copying the glyph walker.
        for (int player = 1; player <= 4; ++player)
        {
            for (const wchar_t* device : devices)
            {
                for (const wchar_t* team : {L" | Team A | ", L" | Team B | "})
                {
                    const std::wstring prefix = L"P" + std::to_wstring(player) + L" - " + device + team;
                    maximum_at_100 = std::max(maximum_at_100, font.measure(prefix + L"100.0%").x);
                    for (int tenths = 0; tenths <= 1000; ++tenths)
                    {
                        const std::wstring text = prefix + std::to_wstring(tenths / 10) + L'.' +
                            std::to_wstring(tenths % 10) + L'%';
                        const float width = font.measure(text).x;
                        if (width > maximum) { maximum = width; attained = text; }
                    }
                }
            }
        }
        require(maximum > maximum_at_100 && font.measure(attained).y == 24.0f,
            "Results fixture failed to distinguish 100.0% from the widest legal percentage or variable glyph height");
        require(percentage_text(68.499f) == L"68.4%" && percentage_text(99.999f) == L"99.9%" &&
            percentage_text(100.0f) == L"100.0%", "Results numeric formatter changed truncation to rounding");
        const float x = 1610.0f - maximum;
        const auto edit = [&](rapidjson::Document& d, bool outside)
        {
            d["results_menu"]["contribution_font"].SetString("menu_labels_results_extrema");
            d["results_menu"]["contribution_position"]["x"].SetFloat(x + (outside ? 0.25f : 0.0f));
            d["results_menu"]["fill"]["box_size"]["x"].SetFloat(1264.0f);
            d["menu_text"]["records"]["new_best"].SetString("R");
            d["menu_text"]["records"]["stage_best"].SetString("R");
        };
        const auto fits = variants.load([&](rapidjson::Document& d) { edit(d, false); });
        fits.validate_resources(world.render, world.audio);
        colourwars::UiTextDropShadow row("results-row", attained, fits.results_menu().contribution_font,
            mattmath::Vector2F(x, 815.0f), &world.render, Colour::black, Colour(), mattmath::Vector2F::ZERO);
        require(row.bounds().right() == 1610.0f && row.bounds().height == 24.0f,
            "Complete Results row lost its unscaled zero-shadow bounds or inclusive full suffix width");
        variants.rejects("labels.roster_row", [&](rapidjson::Document& d) { edit(d, true); }, &world);
    }

    void check_connection_bounds(Variants& variants, World& world)
    {
        const auto add_font = [&](const std::string& name, int width, int height)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                glyphs.push_back({character, mattmath::RectangleI(0, 0,
                    character == U'~' ? width : 8, character == U'~' ? height : 16), 0.0f, 0.0f, 0.0f});
            }
            world.render.add_font(name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        };
        const auto target = [](rapidjson::Document& d, const char* key, const std::string& font)
        {
            compact_connection_copy(d);
            select_connection_font(d, key, font.c_str());
            d["menu_text"]["connection"][key].SetString(
                std::string(key) == "pause_player_title" ? "~{player}" : "~", d.GetAllocator());
        };
        const auto original = variants.load([](rapidjson::Document&) {});
        require(original.main_menu().setup_box_size == mattmath::Vector2F(1760.0f, 920.0f) &&
            original.main_menu().team_box_size == mattmath::Vector2F(1840.0f, 920.0f) &&
            original.menu().layout.position == mattmath::Vector2F(150.0f, 150.0f) &&
            original.main_menu().player_count_spacing.y == 100.0f &&
            original.pause_menu().box_size == mattmath::Vector2F(1050.0f, 900.0f),
            "Connection boundary fixtures no longer match the shipped setup/team/Pause papers");

        // Known eight-unit ordinary glyphs isolate one long supported field.
        // Pause text is plain and centered on the full 1920x1080 paper;
        // Join/Team retain positive shadow extents inside their inset paper.
        struct Boundary { const char* key; int width; int height; };
        constexpr Boundary boundaries[]{
            {"join_ready", 1675, 75}, {"join_waiting", 1675, 75},
            {"pause_title", 1050, 135}, {"pause_player_title", 1042, 135},
            {"controller_lost", 1050, 135}, {"all_connected", 1050, 122},
            {"teams_required", 1715, 65}
        };
        for (const auto& entry : boundaries)
        {
            const std::string name = std::string("menu_connection_") + entry.key;
            add_font(name + "_width_edge", entry.width, 20);
            add_font(name + "_width_over", entry.width + 1, 20);
            add_font(name + "_height_edge", 8, entry.height);
            add_font(name + "_height_over", 8, entry.height + 1);
            const std::string field = std::string("connection.") + entry.key;
            const auto width_edge = variants.load([&](rapidjson::Document& d)
                { target(d, entry.key, name + "_width_edge"); });
            width_edge.validate_resources(world.render, world.audio);
            const auto height_edge = variants.load([&](rapidjson::Document& d)
                { target(d, entry.key, name + "_height_edge"); });
            height_edge.validate_resources(world.render, world.audio);
            variants.rejects(field, [&](rapidjson::Document& d)
                { target(d, entry.key, name + "_width_over"); }, &world);
            variants.rejects(field, [&](rapidjson::Document& d)
                { target(d, entry.key, name + "_height_over"); }, &world);

            const bool pause_heading = std::string(entry.key) == "pause_title" ||
                std::string(entry.key) == "pause_player_title" || std::string(entry.key) == "controller_lost";
            if (pause_heading || std::string(entry.key) == "all_connected")
            {
                const auto role = std::find_if(std::begin(connection_text), std::end(connection_text),
                    [&](const ConnectionText& candidate) { return std::string(candidate.key) == entry.key; });
                require(role != std::end(connection_text), "Connection boundary role has no independent default");
                const std::wstring words = std::string(entry.key) == "pause_player_title" ?
                    width_edge.menu_text().pause_title_for_player(4) : width_edge.menu_text().connection.*role->field;
                const auto& pause = width_edge.pause_menu();
                const float y = pause_heading ? pause.heading_y : pause.status_y;
                UiText text("connection", words, pause_heading ? width_edge.menu().heading_font : width_edge.menu().detail_font,
                    mattmath::Vector2F(0.0f, y), &world.render, pause.art.ink);
                text.set_position(mattmath::Vector2F((1920.0f - text.bounds().width) / 2.0f, y));
                require(text.bounds().x == 435.0f && text.bounds().right() == 1485.0f &&
                    text.bounds().width == 1050.0f && text.bounds().height == 20.0f,
                    "Plain centered connection text changed its unscaled inclusive full-paper width");
            }
            else
            {
                const bool team = std::string(entry.key) == "teams_required";
                const auto& style = width_edge.menu();
                const mattmath::Vector2F position(150.0f, team ? 860.0f : 850.0f);
                colourwars::UiTextDropShadow text("connection", L"~", style.detail_font, position, &world.render,
                    Colour::black, Colour(0.0f, 0.0f, 0.0f, 0.0f), style.detail_shadow_offset);
                require(text.bounds().x == 150.0f && text.bounds().right() == (team ? 1866.0f : 1826.0f),
                    "Join/Team connection text lost its retained transparent-shadow width at the inclusive inset edge");
                colourwars::UiTextDropShadow tall("connection-tall", L"~", height_edge.menu().detail_font, position, &world.render,
                    Colour::black, Colour(0.0f, 0.0f, 0.0f, 0.0f), height_edge.menu().detail_shadow_offset);
                require(tall.bounds().bottom() == 926.0f,
                    "Join/Team actual glyph height and transparent shadow missed the inset paper bottom");
            }
        }

        // Reconnect is appended only to reserved pads. The keyboard/mouse
        // label is checked separately by the validator and never disconnected.
        // "P4 - Pad 4" is ten ordinary eight-unit glyphs. The 1594-unit suffix
        // plus a two-unit retained shadow exactly fills x=150 through x=1826.
        add_font("menu_connection_suffix_edge", 1594, 98);
        add_font("menu_connection_suffix_width_over", 1595, 98);
        add_font("menu_connection_suffix_height_over", 1594, 99);
        const auto suffix = [&](rapidjson::Document& d, const char* font)
        {
            target(d, "join_reconnect_suffix", font);
            d["menu_style"]["shadow"]["item_offset"]["x"].SetFloat(2.0f);
            d["menu_style"]["shadow"]["item_offset"]["y"].SetFloat(2.0f);
        };
        const auto suffix_edge = variants.load([&](rapidjson::Document& d)
            { suffix(d, "menu_connection_suffix_edge"); });
        suffix_edge.validate_resources(world.render, world.audio);
        colourwars::UiTextDropShadow reserved("reserved-pad", L"P4 - Pad 4~", suffix_edge.menu().item_font,
            mattmath::Vector2F(150.0f, 650.0f), &world.render, Colour::black,
            Colour(0.0f, 0.0f, 0.0f, 0.0f), suffix_edge.menu().item_shadow_offset);
        require(reserved.bounds() == mattmath::RectangleF(150.0f, 650.0f, 1676.0f, 100.0f),
            "Reserved-pad suffix no longer touches the exact inclusive paper edge and next row");
        for (const char* overflow : {"menu_connection_suffix_width_over", "menu_connection_suffix_height_over"})
        {
            variants.rejects("connection.join_reconnect_suffix", [&](rapidjson::Document& d)
                { suffix(d, overflow); }, &world);
        }
        const auto team_order = [](rapidjson::Document& d)
        {
            compact_connection_copy(d);
            d["main_menu"]["team_select"]["warning_position"]["y"].SetFloat(172.5f);
        };
        const auto touches_heading = variants.load(team_order);
        colourwars::UiTextDropShadow team_heading("team-heading", L"Fit", touches_heading.menu().heading_font,
            mattmath::Vector2F(150.0f, 150.0f), &world.render, Colour::black, Colour(), touches_heading.menu().heading_shadow_offset);
        colourwars::UiTextDropShadow heading_warning("heading-warning", L"Fit", touches_heading.menu().detail_font,
            mattmath::Vector2F(150.0f, 172.5f), &world.render, Colour::black, Colour(), touches_heading.menu().detail_shadow_offset);
        require(team_heading.bounds().bottom() == 172.5f && heading_warning.bounds().y == team_heading.bounds().bottom(),
            "Team warning lost its exact heading-contact geometry");
        // Heading contact leaves no space for the four complete player cards.
        variants.rejects("labels.roster_row", team_order, &world);
        variants.rejects("connection.teams_required", [&](rapidjson::Document& d)
        {
            team_order(d);
            d["main_menu"]["team_select"]["warning_position"]["y"].SetFloat(172.25f);
        }, &world);
        const auto full_row_edge = [&](rapidjson::Document& d)
        {
            compact_connection_copy(d);
            d["main_menu"]["team_select"]["warning_position"]["y"].SetFloat(832.0f);
        };
        const auto touches_final_card = variants.load(full_row_edge);
        touches_final_card.validate_resources(world.render, world.audio);
        require(touches_final_card.main_menu().team_select.spacing == mattmath::Vector2F(250.0f, 125.0f) &&
            touches_final_card.main_menu().team_select.size == mattmath::Vector2F(50.0f, 50.0f) &&
            touches_final_card.main_menu().setup_focus_padding == mattmath::Vector2F(28.0f, 7.0f) &&
            touches_final_card.player_identity().ui_size == mattmath::Vector2F(54.0f, 42.0f) &&
            touches_final_card.player_identity().ui_gap == 12.0f,
            "Team complete-row boundary fixture no longer matches the authored icons, spacing, focus or badge");
        colourwars::UiTextDropShadow final_label("final-label", L"P4 - Pad 4", touches_final_card.menu().detail_font,
            mattmath::Vector2F(150.0f, 775.0f), &world.render, Colour::black, Colour(), touches_final_card.menu().item_shadow_offset);
        mattmath::RectangleF final_card = mattmath::RectangleF::union_of(final_label.bounds(),
            mattmath::RectangleF(84.0f, 764.75f, 54.0f, 42.0f));
        final_card = mattmath::RectangleF::union_of(final_card, mattmath::RectangleF(650.0f, 775.0f, 150.0f, 50.0f));
        final_card.inflate(mattmath::Vector2F(28.0f, 7.0f));
        colourwars::UiTextDropShadow final_warning("final-warning", L"Fit", touches_final_card.menu().detail_font,
            mattmath::Vector2F(150.0f, 832.0f), &world.render, Colour::black, Colour(), touches_final_card.menu().detail_shadow_offset);
        require(final_card.bottom() == 832.0f && final_warning.bounds().y == final_card.bottom() &&
            final_warning.bounds().bottom() == 853.0f,
            "Team warning lost exact complete-card contact including the badge, icons and focus padding");
        variants.rejects("labels.roster_row", [&](rapidjson::Document& d)
        {
            full_row_edge(d);
            d["main_menu"]["team_select"]["warning_position"]["y"].SetFloat(831.75f);
        }, &world);
    }
}
