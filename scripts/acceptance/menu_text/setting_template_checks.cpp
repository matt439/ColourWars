#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"

// Match-duration and resolution templates.
namespace menu_text_acceptance
{
    void check_setting_duration_content(Variants& variants, World& world, const std::filesystem::path& content)
    {
        Registry<LevelLoadedInfo> levels("Duration acceptance level"); StageList stages;
        for (const char* name : { "king_of_the_hill", "turbulence", "close_quarters", "movement_course", "crossfeed" })
        {
            auto info = std::make_unique<LevelLoadedInfo>((content / "levels" / (std::string(name) + ".json")).string().c_str());
            info->validate(); stages.push_back({ name, info->display_name(), info->icon_frame(), info->include_in_random() });
            levels.add(name, std::move(info));
        }
        const GameTuning authored((content / "tuning.json").string().c_str());
        const rapidjson::Document tuning_source = read_document(content / "tuning.json");
        const auto tune = [&](float seconds, const std::vector<float>& choices)
        {
            rapidjson::Document d; d.CopyFrom(tuning_source, d.GetAllocator());
            d["match"]["duration"].SetFloat(seconds); auto& values = d["match"]["duration_choices"]; values.SetArray();
            for (const float choice : choices)
            {
                rapidjson::Value item(rapidjson::kObjectType);
                item.AddMember("seconds", choice, d.GetAllocator()); values.PushBack(item, d.GetAllocator());
            }
            rapidjson::StringBuffer buffer; rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            require(d.Accept(writer), "Cannot encode isolated duration tuning fixture");
            const auto path = variants.scratch / ("duration-tuning-" + std::to_string(variants.next++) + ".json");
            std::ofstream file(path, std::ios::binary); file.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
            file.close(); require(file.good(), "Cannot write isolated duration tuning fixture");
            return GameTuning(path.string().c_str());
        };
        const auto complete = [&](const GameTuning& balance, const Presentation& look)
        { validate_game_content(balance, look, &world.render, &world.audio, levels, stages, PlaySettings{}); };
        const auto rejected = [](const std::string& owner, auto action)
        {
            bool refused = false; std::string diagnostic = "accepted without exception";
            try { action(); }
            catch (const std::runtime_error& error)
            { diagnostic = error.what(); refused = diagnostic.find(owner) != std::string::npos; }
            require(refused, "Duration resource failure lost its actual owner '" + owner + "': " + diagnostic);
        };
        const auto add_font = [&](const char* name, int marker_width, int marker_height,
            float marker_x, float marker_y, char32_t missing = U'\0')
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                if (character == missing) { continue; }
                const bool marker = character == U'~';
                glyphs.push_back({ character, mattmath::RectangleI(0, 0, marker ? marker_width : 8,
                    marker ? marker_height : 16), marker ? marker_x : 0.0f, marker ? marker_y : 0.0f, 0.0f });
            }
            auto font = std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f);
            font->set_stand_in(U'?'); world.render.add_font(name, std::move(font));
        };
        add_font("menu_duration_right", 1136, 16, 0.5f, 0.0f);
        add_font("menu_duration_right_overflow", 1136, 16, 0.75f, 0.0f);
        add_font("menu_duration_height", 8, 82, 0.0f, 0.5f);
        add_font("menu_duration_height_overflow", 8, 82, 0.0f, 0.75f);
        add_font("menu_duration_missing_plus", 8, 16, 0.0f, 0.0f, U'+');
        const auto marker = [](rapidjson::Document& d, const char* font_name)
        {
            d["menu_style"]["fonts"]["item"].SetString(font_name, d.GetAllocator());
            d["menu_text"]["values"]["duration_minutes"].SetString("~{value}");
            d["menu_text"]["values"]["duration_default"].SetString("{value}");
        };
        const auto right = variants.load([&](rapidjson::Document& d) { marker(d, "menu_duration_right"); });
        const auto height = variants.load([&](rapidjson::Document& d) { marker(d, "menu_duration_height"); });
        const auto check_row = [&](const Presentation& look, bool right_edge, bool height_edge)
        {
            const MainMenuPresentation& main = look.main_menu(); MenuStyle style = look.menu();
            style.shadow_colour = Colour{};
            const mattmath::Vector2F label_position = style.layout.position + mattmath::Vector2F(0.0f, main.mode_select_spacing.y * 5.0f);
            const mattmath::Vector2F value_position = label_position + mattmath::Vector2F(main.mode_select_spacing.x * 2.0f, 0.0f);
            int adjustments = 0;
            OptionRow row("duration", L"Match Length", label_position, value_position, style, main.art.ink, &world.render,
                [&](Direction direction) { adjustments += direction == Direction::right ? 1 : -1; });
            UiTextDropShadow* value_widget = nullptr;
            for (const auto& child : row.children())
            { if (child.first == "duration_value") { value_widget = dynamic_cast<UiTextDropShadow*>(child.second); } }
            require(value_widget != nullptr, "Duration row no longer retains its real invisible text shadow");
            std::wstring words = L"~4"; row.set_value(words); words = L"Changed after set_value";
            const auto measured = world.render.measure_text(world.render.resolve_sprite_font(style.item_font), L"~4");
            const auto value_bounds = value_widget->bounds();
            require(value_widget->text() == L"~4" && value_bounds.position() == value_position &&
                value_bounds.size() == measured + mattmath::Vector2F(1.5f, 1.5f) &&
                label_position == mattmath::Vector2F(150.0f, 650.0f) && value_position == mattmath::Vector2F(650.0f, 650.0f),
                "Duration value changed copied text, unscaled font, inherited invisible shadow or authored columns");
            auto padded = row.bounds(); padded.inflate(main.setup_focus_padding);
            require(!right_edge || (measured == mattmath::Vector2F(1144.5f, 20.0f) &&
                padded.right() + main.setup_shadow_offset == 1826.0f),
                "Duration right-edge oracle no longer touches the actual setup interior at 1826");
            require(!height_edge || (measured == mattmath::Vector2F(16.0f, 82.5f) &&
                padded.height + main.setup_shadow_offset == 100.0f),
                "Duration height oracle no longer exactly touches its 100-unit complete-row limit");
            const mattmath::RectangleF panel((mattmath::Vector2F(1920.0f, style.footer_y) - main.setup_box_size) / 2.0f,
                main.setup_box_size);
            MenuPaperArtPresentation art = main.art; art.focus_padding = main.setup_focus_padding; art.shadow_offset = main.setup_shadow_offset;
            MainMenuModeSelect::check_duration_row(row, panel, art, main.mode_select_spacing.y);
            MainMenuModeSelect::validate_durations(&world.render, authored, look);
            auto produced = MainMenuModeSelect::make_duration_row(&world.render, look.menu(), main, look.menu_text());
            produced->set_value(L"~4");
            require(produced->bounds() == row.bounds(), "Actual Mode factory changed the independently constructed duration row");
            row.adjust(Direction::right); require(adjustments == 1, "Duration row lost its right adjustment callback");
            row.adjust(Direction::left);
            require(adjustments == 0 && value_widget->text() == L"~4", "Duration row lost its left adjustment callback or immutable displayed value");
            row.set_hidden(true); DrawList no_device; row.draw(no_device);
        };
        complete(authored, right); complete(authored, height);
        check_row(right, true, false); check_row(height, false, true);
        const auto right_overflow = variants.load([&](rapidjson::Document& d) { marker(d, "menu_duration_right_overflow"); });
        const auto height_overflow = variants.load([&](rapidjson::Document& d) { marker(d, "menu_duration_height_overflow"); });
        // All old presentation checks still accept these: only the actual
        // composed numeric value adds the final glyph/quarter unit.
        right_overflow.validate_resources(world.render, world.audio);
        height_overflow.validate_resources(world.render, world.audio);
        require(world.render.measure_text(world.render.resolve_sprite_font("menu_duration_right_overflow"), L"~4") ==
            mattmath::Vector2F(1144.75f, 20.0f) &&
            world.render.measure_text(world.render.resolve_sprite_font("menu_duration_height_overflow"), L"~4") ==
            mattmath::Vector2F(16.0f, 82.75f), "Duration overflow fixtures no longer add exactly one quarter unit");
        rejected("mode_select_spacing", [&] { complete(authored, right_overflow); });
        rejected("mode_select_spacing", [&] { complete(authored, height_overflow); });

        // An invisible negative item shadow still extends navigation geometry.
        // Use literal widgets to prove the detached arrow's left92 contact;
        // moving the entire row a quarter left must refuse that same geometry.
        const auto negative_shadow = [&](float label_x, float expected_arrow_left, bool fits)
        {
            MenuStyle style = right.menu(); style.item_shadow_offset = { -1.5f, -1.5f }; style.shadow_colour = Colour{};
            const MainMenuPresentation& main = right.main_menu();
            OptionRow row("negative_duration", L"Match Length", { label_x, 650.0f }, { label_x + 500.0f, 650.0f },
                style, main.art.ink, &world.render, {}); row.set_value(L"~4");
            MenuPaperArtPresentation art = main.art; art.focus_padding = main.setup_focus_padding; art.shadow_offset = main.setup_shadow_offset;
            auto padded = row.bounds(); padded.inflate(art.focus_padding);
            require(row.bounds().left() == label_x - 1.5f && row.bounds().top() == 648.5f &&
                padded.left() - art.keyline_width * 7.0f == expected_arrow_left,
                "Duration negative-shadow oracle lost its inherited extent or exact arrow edge");
            const mattmath::RectangleF panel(80.0f, 20.0f, 1760.0f, 920.0f);
            if (fits) { MainMenuModeSelect::check_duration_row(row, panel, art, 100.0f); }
            else { rejected("mode_select_spacing", [&] { MainMenuModeSelect::check_duration_row(row, panel, art, 100.0f); }); }
        };
        negative_shadow(149.5f, 92.0f, true); negative_shadow(149.25f, 91.75f, false);

        // The full startup/reload validator must inspect the live default AND
        // each authored choice. Neither is inferred from a fixed '240' stand-in.
        const auto narrow = variants.load([](rapidjson::Document& d)
        {
            d["main_menu"]["mode_select_spacing"]["x"].SetFloat(790.0f);
            d["menu_text"]["values"]["duration_minutes"].SetString("{value}");
            d["menu_text"]["values"]["duration_default"].SetString("{value}");
        });
        narrow.validate_resources(world.render, world.audio);
        complete(authored, narrow);
        const GameTuning long_choice = tune(240.0f, { 60.0f, 6000000000.0f });
        const GameTuning long_default = tune(6000000000.0f, { 60.0f, 120.0f });
        rejected("mode_select_spacing", [&] { complete(long_choice, narrow); });
        rejected("mode_select_spacing", [&] { complete(long_default, narrow); });
        const auto missing_plus = variants.load([](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_duration_missing_plus");
            d["menu_text"]["values"]["duration_minutes"].SetString("{value}");
            d["menu_text"]["values"]["duration_default"].SetString("{value}");
        });
        missing_plus.validate_resources(world.render, world.audio);
        require(world.render.font(world.render.resolve_sprite_font("menu_duration_missing_plus"))->contains(U'?') &&
            !world.render.font(world.render.resolve_sprite_font("menu_duration_missing_plus"))->contains(U'+'),
            "Duration missing-generated-glyph fixture lost its stand-in or missing plus");
        rejected("menu_text.values.duration_minutes", [&] { complete(long_choice, missing_plus); });
        rejected("menu_text.values.duration_default", [&] { complete(long_default, missing_plus); });
        for (const char* key : { "duration_minutes", "duration_default" })
        {
            variants.rejects(std::string("values.") + key, [&](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
                d["menu_text"]["values"][key].SetString("~{value}");
            }, &world);
        }
        const auto long_words = variants.load([](rapidjson::Document& d)
        {
            const std::string words = std::string(512, 'W') + "{value}";
            d["menu_text"]["values"]["duration_minutes"].SetString(words.c_str(), d.GetAllocator());
        });
        long_words.validate_resources(world.render, world.audio);
        rejected("mode_select_spacing", [&] { complete(authored, long_words); });
        // Preserve the established diagnostic owner before the appended paired
        // duration validation, even when both authored roles are invalid.
        const auto legacy_first = variants.load([](rapidjson::Document& d)
        {
            const std::string words(512, 'W');
            d["menu_text"]["home_subtitle"].SetString(words.c_str(), d.GetAllocator());
            d["menu_text"]["values"]["duration_minutes"].SetString((words + "{value}").c_str(), d.GetAllocator());
        });
        rejected("home_subtitle", [&] { complete(authored, legacy_first); });
    }

    void check_setting_duration_text(Variants& variants, const MenuText& defaults)
    {
        struct DurationExample { float seconds; const wchar_t* minutes; };
        const DurationExample examples[]{
            { 240.0f, L"4" }, { 90.0f, L"1.5" }, { 61.0f, L"1.01667" }, { 123.0f, L"2.05" },
            { 1.0f, L"0.0166667" }, { 6000000000.0f, L"1e+08" }, { 0.000006f, L"1e-07" },
            { std::numeric_limits<float>::min(), L"1.95916e-40" },
            { std::numeric_limits<float>::denorm_min(), L"0" },
            { std::numeric_limits<float>::max(), L"5.67137e+36" },
            // 599.9997f/60.0f rounds to 9.999995231628... before formatting.
            // Promoting the division to double instead prints 9.99999 at six digits.
            { 599.9997f, L"10" }
        };
        for (const auto& example : examples)
        {
            const std::wstring explicit_label = std::wstring(example.minutes) + L" min";
            require(defaults.setting_duration(example.seconds, false) == explicit_label &&
                defaults.setting_duration(example.seconds, true) == L"Default (" + explicit_label + L")",
                "Duration formatting changed FLOAT seconds/60.0f, six-significant defaultfloat copy or Default composition");
        }
        const auto rejects_format = [](auto format)
        {
            bool refused = false;
            try { format(); }
            catch (const std::invalid_argument&) { refused = true; }
            require(refused, "Invalid duration argument or directly mutated template formatted silently");
        };
        MenuText unused_wrapper = defaults; unused_wrapper.values.duration_default = L"{unknown}";
        require(unused_wrapper.setting_duration(240.0f, false) == L"4 min",
            "Explicit duration unexpectedly evaluated its unused Default wrapper");
        for (const float invalid : { 0.0f, -0.0f, -1.0f, -std::numeric_limits<float>::denorm_min(),
            std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::quiet_NaN() })
        {
            for (const bool use_default : { false, true })
            { rejects_format([&] { defaults.setting_duration(invalid, use_default); }); }
        }
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["values"]["duration_minutes"].SetString(
                "  {value}\xC2\xA0\xE2\x80\x9C" "min\xE2\x80\x9D\xE2\x80\x94\xE2\x80\xA6  ");
            d["menu_text"]["values"]["duration_default"].SetString("  <{value}>  ");
        });
        require(normalized.menu_text().values.duration_minutes == L"  {value} \"min\"-...  " &&
            normalized.menu_text().setting_duration(90.0f, false) == L"  1.5 \"min\"-...  " &&
            normalized.menu_text().setting_duration(90.0f, true) == L"  <  1.5 \"min\"-...  >  ",
            "Duration templates lost authored outer spaces, Unicode normalization or literal wrapper placement");
        const auto reversed = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["values"]["duration_minutes"].SetString("minutes:%{value}%%");
            d["menu_text"]["values"]["duration_default"].SetString("{value}:default");
        });
        require(reversed.menu_text().setting_duration(123.0f, true) == L"minutes:%2.05%%:default" &&
            reversed.menu_text().setting_duration(123.0f, false) == L"minutes:%2.05%%",
            "Duration templates treated literal percent signs as formatting syntax or fixed the token's position");
        struct DurationTemplate { const char* key; std::wstring MenuValueText::* field; bool wrapper; };
        const DurationTemplate templates[]{
            { "duration_minutes", &MenuValueText::duration_minutes, false },
            { "duration_default", &MenuValueText::duration_default, true }
        };
        for (const auto& entry : templates)
        {
            const std::string field = std::string("values.") + entry.key;
            for (const char* invalid : { "   ", " \xC2\xA0 ", "{value}\n", "{value}\r", "{value}\r\n",
                "{value}\t", "{value}\x01", "{value}\x7f", "{value}caf\xC3\xA9", "{value}\xF0\x9F\x8E\xA8", "{value}\xC3\x28",
                "No token", "{value}{value}", "{Value}", "{value }", "{{value}}", "{value", "value}",
                "{value}{unknown}", "{va{value}lue}", "{value}%{", "{value}}", "{}{value}" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                    { d["menu_text"]["values"][entry.key].SetString(invalid, d.GetAllocator()); });
            }
            for (int type = 0; type < 3; ++type)
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    auto& target = d["menu_text"]["values"][entry.key];
                    if (type == 0) { target.SetArray(); }
                    else if (type == 1) { target.SetObject(); }
                    else { target.SetInt(12); }
                });
            }
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                d["menu_text"]["values"][entry.key].SetString("{value}\0tail", 12, d.GetAllocator());
            });
            for (const wchar_t* invalid : { L"", L"   ", L"No token", L"{value}{value}", L"{{value}}",
                L"{va{value}lue}", L"{value}{unknown}", L"{value", L"value}", L"{value}}", L"{value}%{",
                L"{value}\n", L"{value}\r", L"{value}\t", L"{value}\x01", L"{value}\x7f",
                L"{value}\u00e9", L"{value}\u00a0", L"{value}\u2014" })
            {
                MenuText mutated = defaults; mutated.values.*entry.field = invalid;
                rejects_format([&] { mutated.setting_duration(240.0f, entry.wrapper); });
                if (!entry.wrapper) { rejects_format([&] { mutated.setting_duration(240.0f, true); }); }
            }
            MenuText nul = defaults; (nul.values.*entry.field).push_back(L'\0');
            rejects_format([&] { nul.setting_duration(240.0f, entry.wrapper); });
        }
        // Length is a resource/layout concern, not a hidden formatter cap.
        const auto long_supported = variants.load([](rapidjson::Document& d)
        {
            const std::string words = std::string(512, 'W') + "{value}";
            d["menu_text"]["values"]["duration_minutes"].SetString(words.c_str(), d.GetAllocator());
        });
        require(long_supported.menu_text().setting_duration(240.0f, false) == std::wstring(512, L'W') + L"4",
            "Duration formatter truncated supported authored text or imposed an unrelated length cap");
    }

    void check_setting_resolution_resources(Variants& variants, World& world)
    {
        const auto add_font = [&](const char* name, int eight_width, int eight_height,
            char32_t missing = U'\0', bool zero_resolution = false, float eight_offset = 0.0f)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                if (character == missing) { continue; }
                const bool eight = character == U'8';
                const bool zero = zero_resolution && ((character >= U'0' && character <= U'9') || character == U'x');
                glyphs.push_back({ character, mattmath::RectangleI(0, 0, zero ? 0 : eight ? eight_width : 8,
                    eight ? eight_height : 16), eight ? eight_offset : 0.0f, 0.0f, 0.0f });
            }
            auto font = std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f);
            font->set_stand_in(U'?'); world.render.add_font(name, std::move(font));
        };
        add_font("menu_resolution_width", 2000, 16);
        add_font("menu_resolution_height", 8, 200);
        add_font("menu_resolution_both", 2000, 2000);
        add_font("menu_resolution_missing9", 8, 16, U'9');
        add_font("menu_resolution_zero", 8, 16, U'\0', true);
        add_font("menu_resolution_nonfinite", 8, 16, U'\0', false, std::numeric_limits<float>::infinity());
        const auto check_row = [&](const Presentation& look, bool width_limited, bool height_limited)
        {
            const MainMenuPresentation& main = look.main_menu(); const MenuStyle& style = look.menu();
            const MenuPaperArtPresentation art = options_art(main);
            const mattmath::RectangleF paper((mattmath::Vector2F(1920.0f, style.footer_y) - main.options_box_size) / 2.0f,
                main.options_box_size);
            const mattmath::Vector2F value_position(main.options_value_x, main.options_position.y);
            const mattmath::Vector2F limit(paper.right() - art.shadow_offset - art.focus_padding.x - value_position.x,
                main.options_spacing.y - art.focus_padding.y * 2.0f);
            require(value_position == mattmath::Vector2F(820.0f, 220.0f) && limit == mattmath::Vector2F(930.0f, 86.0f),
                "Resolution fixture no longer exercises the actual Options value column and row");
            const FontHandle font_handle = world.render.resolve_sprite_font(style.item_font);
            const std::wstring expected = L"1280x720";
            const auto measured = world.render.measure_text(font_handle, expected);
            float expected_scale = 1.0f;
            if (measured.x > 930.0f) { expected_scale = 930.0f / measured.x; }
            require(width_limited == (expected_scale < 1.0f) &&
                height_limited == (measured.y * expected_scale > 86.0f),
                "Resolution font does not independently exercise its claimed width-then-height fitting path");
            if (measured.y * expected_scale > 86.0f) { expected_scale = 86.0f / measured.y; }
            int adjustments = 0;
            OptionRow row("resolution", L"Resolution", main.options_position, value_position, style, art.ink, &world.render,
                [&](Direction direction) { adjustments += direction == Direction::right ? 1 : -1; }, &art, limit);
            UiText* value = row_text(row, "resolution_value"); UiText* label = row_text(row, "resolution_label");
            require(value != nullptr && label != nullptr, "Resolution row lost its actual plain-ink OptionRow children");
            const auto retained_label = label->bounds();
            std::wstring formatted = look.menu_text().setting_resolution(1280, 720);
            require(formatted == expected, "Resolution fitting oracle lost its independent literal");
            row.set_value(formatted); formatted = L"Changed after set_value";
            const auto bounds = value->bounds();
            require(value->text() == expected && bounds.position() == value_position && label->bounds() == retained_label &&
                std::abs(bounds.width - measured.x * expected_scale) <= 0.01f &&
                std::abs(bounds.height - measured.y * expected_scale) <= 0.01f &&
                bounds.width > 0.0f && bounds.height > 0.0f && bounds.width <= 930.01f && bounds.height <= 86.01f,
                "Resolution OptionRow lost copied text, its fixed column/label or proportional fitting");
            auto padded = bounds; padded.inflate(art.focus_padding);
            require(paper.contains(padded) && bounds.height + art.focus_padding.y * 2.0f <= main.options_spacing.y + 0.01f,
                "Resolution value or focus padding escaped the actual Options paper/row");
            if (height_limited) { require(std::abs(bounds.height - 86.0f) <= 0.01f, "Resolution height fitting lost inclusive row contact"); }
            else if (width_limited) { require(std::abs(bounds.width - 930.0f) <= 0.01f, "Resolution width fitting lost inclusive column contact"); }
            else { require(bounds == mattmath::RectangleF(value_position, measured), "Short resolution value was enlarged"); }
            row.set_value(look.menu_text().setting_resolution(1, 1));
            require(value->text() == L"1x1" && value->bounds() == mattmath::RectangleF(value_position,
                world.render.measure_text(font_handle, L"1x1")) && label->bounds() == retained_label,
                "Short resolution retained the previous long/tall shrink scale or changed its label");
            row.adjust(Direction::right); require(adjustments == 1, "Resolution row lost its right adjustment callback");
            row.adjust(Direction::left); require(adjustments == 0, "Resolution row lost its left adjustment callback");
            row.set_hidden(true); DrawList no_device; row.draw(no_device);
        };
        const auto defaults = variants.load([](rapidjson::Document&) {});
        defaults.validate_resources(world.render, world.audio); check_row(defaults, false, false);
        for (int mode = 0; mode < 3; ++mode)
        {
            const char* names[]{ "menu_resolution_width", "menu_resolution_height", "menu_resolution_both" };
            const auto look = variants.load([&](rapidjson::Document& d)
                { d["menu_style"]["fonts"]["item"].SetString(names[mode], d.GetAllocator()); });
            // The wide digit intentionally cannot fit an unscaled Results
            // percentage. Check that refusal, then exercise Options fitting
            // directly with the same parsed font and independent row oracle.
            if (mode == 1) { look.validate_resources(world.render, world.audio); }
            else { variants.rejects("results.solo_coverage", [&](rapidjson::Document& d)
                { d["menu_style"]["fonts"]["item"].SetString(names[mode], d.GetAllocator()); }, &world); }
            const auto raw = world.render.measure_text(world.render.resolve_sprite_font(names[mode]), L"1280x720");
            require(raw == (mode == 0 ? mattmath::Vector2F(2056.0f, 20.0f) : mode == 1 ?
                mattmath::Vector2F(64.0f, 200.0f) : mattmath::Vector2F(2056.0f, 2000.0f)),
                "Adversarial resolution glyph fixture changed its independent literal dimensions");
            check_row(look, mode != 1, mode != 0);
        }
        for (const char* font_name : { "menu_resolution_missing9", "menu_resolution_zero", "menu_resolution_nonfinite" })
        {
            variants.rejects("values.resolution", [&](rapidjson::Document& d)
                { d["menu_style"]["fonts"]["item"].SetString(font_name, d.GetAllocator()); }, &world);
        }
        require(world.render.font(world.render.resolve_sprite_font("menu_resolution_missing9"))->contains(U'?') &&
            !world.render.font(world.render.resolve_sprite_font("menu_resolution_missing9"))->contains(U'9'),
            "Resolution decimal-glyph fixture lost its stand-in or missing digit");
        require(world.render.font(world.render.resolve_sprite_font("menu_text_limited"))->contains(U'?') &&
            !world.render.font(world.render.resolve_sprite_font("menu_text_limited"))->contains(U'~'),
            "Resolution literal-glyph fixture lost its stand-in or missing tilde");
        variants.rejects("values.resolution", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
            d["menu_text"]["values"]["resolution"].SetString("~{width}x{height}");
        }, &world);
        const auto separate_controls = variants.load([](rapidjson::Document& d)
        {
            d["controls_settings"]["item_font"].SetString("menu_text_limited");
            d["menu_text"]["values"]["resolution"].SetString("~{width}x{height}");
        });
        separate_controls.validate_resources(world.render, world.audio);
        const auto long_supported = variants.load([](rapidjson::Document& d)
        {
            const std::string words = std::string(512, 'W') + "{width}x{height}";
            d["menu_text"]["values"]["resolution"].SetString(words.c_str(), d.GetAllocator());
        });
        long_supported.validate_resources(world.render, world.audio);
        const MenuPaperArtPresentation art = options_art(long_supported.main_menu());
        OptionRow long_row("long_resolution", L"Resolution", { 280.0f, 220.0f }, { 820.0f, 220.0f },
            long_supported.menu(), art.ink, &world.render, {}, &art, { 930.0f, 86.0f });
        long_row.set_value(long_supported.menu_text().setting_resolution(1, 1));
        const UiText* long_value = row_text(long_row, "long_resolution_value");
        require(long_value != nullptr && long_value->text() == std::wstring(512, L'W') + L"1x1" &&
            std::abs(long_value->bounds().width - 930.0f) <= 0.01f && long_value->bounds().height < 86.0f,
            "Long supported resolution template was truncated or failed width-only fitting");
        // Preserve the existing value diagnostic before the appended resolution
        // guard when the shared item font cannot draw both authored literals.
        variants.rejects("values.on", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
            d["menu_text"]["values"]["on"].SetString("~");
            d["menu_text"]["values"]["resolution"].SetString("~{width}x{height}");
        }, &world);
        variants.rejects("values.duration_minutes", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
            d["menu_text"]["values"]["duration_minutes"].SetString("~{value}");
            d["menu_text"]["values"]["resolution"].SetString("~{width}x{height}");
        }, &world);
        Presentation mutated = defaults;
        const_cast<MenuText&>(mutated.menu_text()).values.resolution = L"{he{width}ight}";
        bool contextual = false;
        try { mutated.validate_resources(world.render, world.audio); }
        catch (const std::runtime_error& error)
        { contextual = std::string(error.what()).find("menu_text.values.resolution") != std::string::npos; }
        require(contextual, "Direct malformed resolution template escaped resource validation or lost its named role");
    }

    void check_setting_resolution_text(Variants& variants, const MenuText& defaults)
    {
        struct ResolutionExample { int width; int height; const wchar_t* label; };
        const ResolutionExample examples[]{
            { 1280, 720, L"1280x720" }, { 1920, 1080, L"1920x1080" },
            { 2560, 1440, L"2560x1440" }, { 3840, 2160, L"3840x2160" },
            { 1, 1, L"1x1" }, { 801, 601, L"801x601" }, { 12345, 67890, L"12345x67890" },
            { std::numeric_limits<int>::max(), 1, L"2147483647x1" },
            { 1, std::numeric_limits<int>::max(), L"1x2147483647" },
            { std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), L"2147483647x2147483647" }
        };
        for (const auto& example : examples)
        {
            require(defaults.setting_resolution(example.width, example.height) == example.label,
                "Resolution formatter changed decimal dimensions, separator/order or the full positive integer domain");
        }
        const auto rejects_format = [](auto action)
        {
            bool refused = false;
            try { action(); }
            catch (const std::invalid_argument&) { refused = true; }
            require(refused, "Invalid resolution dimensions or directly mutated template formatted silently");
        };
        for (const int invalid : { 0, -1, std::numeric_limits<int>::min() })
        {
            rejects_format([&] { defaults.setting_resolution(invalid, 720); });
            rejects_format([&] { defaults.setting_resolution(1280, invalid); });
            rejects_format([&] { defaults.setting_resolution(invalid, invalid); });
        }
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["values"]["resolution"].SetString(
                "  {height}\xC2\xA0\xE2\x80\x9C" "high\xE2\x80\x9D\xE2\x80\x94" "{width}\xE2\x80\xA6  ");
        });
        require(normalized.menu_text().values.resolution == L"  {height} \"high\"-{width}...  " &&
            normalized.menu_text().setting_resolution(801, 601) == L"  601 \"high\"-801...  ",
            "Resolution template lost normalized punctuation, authored outer spacing or reversed token order");
        const auto literals = variants.load([](rapidjson::Document& d)
            { d["menu_text"]["values"]["resolution"].SetString("H={height}% W={width}%%"); });
        require(literals.menu_text().setting_resolution(1, 23) == L"H=23% W=1%%",
            "Resolution template treated literal percent signs as another formatting language");
        for (const char* invalid : { "   ", " \xC2\xA0 ", "{width}x{height}\n", "{width}x{height}\r", "{width}x{height}\r\n",
            "{width}x{height}\t", "{width}x{height}\x01", "{width}x{height}\x7f", "{width}x{height}caf\xC3\xA9",
            "{width}x{height}\xF0\x9F\x8E\xA8", "{width}x{height}\xC3\x28", "No tokens", "{width}", "{height}",
            "{width}{width}{height}", "{width}{height}{height}", "{Width}{height}", "{width }{height}",
            "{{width}}{height}", "{width}{unknown}{height}", "{width}{height", "width}{height}",
            "{width}{height}}", "{width}{height}{", "{he{width}ight}", "{wi{width}dth}{height}", "{width}{he{height}ight}" })
        {
            variants.rejects("values.resolution", [&](rapidjson::Document& d)
                { d["menu_text"]["values"]["resolution"].SetString(invalid, d.GetAllocator()); });
        }
        for (int type = 0; type < 3; ++type)
        {
            variants.rejects("values.resolution", [&](rapidjson::Document& d)
            {
                auto& field = d["menu_text"]["values"]["resolution"];
                if (type == 0) { field.SetArray(); }
                else if (type == 1) { field.SetObject(); }
                else { field.SetInt(1280); }
            });
        }
        variants.rejects("values.resolution", [](rapidjson::Document& d)
        {
            const std::string nul("{width}x{height}\0tail", 21);
            d["menu_text"]["values"]["resolution"].SetString(nul.data(), static_cast<rapidjson::SizeType>(nul.size()), d.GetAllocator());
        });
        for (const wchar_t* invalid : { L"", L"   ", L"{width}", L"{height}", L"{width}{width}{height}",
            L"{width}{height}{height}", L"{Width}{height}", L"{{width}}{height}", L"{width}{height}}", L"{width}{height}{",
            L"{width}{height", L"{width}{unknown}{height}", L"{he{width}ight}", L"{wi{width}dth}{height}", L"{width}{he{height}ight}",
            L"{width}{height}\n", L"{width}{height}\r", L"{width}{height}\t", L"{width}{height}\x01", L"{width}{height}\x7f",
            L"{width}{height}\u00e9", L"{width}{height}\u00a0", L"{width}{height}\u2014" })
        {
            MenuText mutated = defaults; mutated.values.resolution = invalid;
            rejects_format([&] { mutated.setting_resolution(1280, 720); });
        }
        MenuText nul = defaults; nul.values.resolution.push_back(L'\0');
        rejects_format([&] { nul.setting_resolution(1280, 720); });
        const auto long_supported = variants.load([](rapidjson::Document& d)
        {
            const std::string words = std::string(512, 'W') + "{width}x{height}";
            d["menu_text"]["values"]["resolution"].SetString(words.c_str(), d.GetAllocator());
        });
        require(long_supported.menu_text().setting_resolution(1, 1) == std::wstring(512, L'W') + L"1x1",
            "Resolution formatting truncated supported text or introduced an unrelated length cap");
    }
}
