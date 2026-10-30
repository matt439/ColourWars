#pragma once

// Included once after EditorDpiProbe: this member definition uses its existing
// friendship to exercise production drawing without exposing editor test APIs.
#include "editor/paint_mechanism_guide.h"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <memory>
#include <optional>
#include <utility>

namespace colourwars_editor
{
    namespace
    {
        // ED#20: what every MechanismBitmap owned and whether each cleanup call
        // succeeded. Destructors cannot throw, so callers read this receipt instead
        // of a process-wide GDI count.
        struct MechanismBitmapLedger
        {
            std::size_t created = 0;
            std::size_t live = 0;
            std::size_t restore_failures = 0;
            std::size_t bitmap_delete_failures = 0;
            std::size_t dc_delete_failures = 0;
            std::size_t failures() const { return restore_failures + bitmap_delete_failures + dc_delete_failures; }
        };

        inline MechanismBitmapLedger& mechanism_bitmap_ledger()
        {
            static MechanismBitmapLedger ledger;
            return ledger;
        }

        class MechanismBitmap
        {
        public:
            MechanismBitmap(int width, int height) : width_(width), height_(height)
            {
                dc_ = CreateCompatibleDC(nullptr);
                require(dc_ != nullptr, "Could not create the native guide DC");
                MechanismBitmapLedger& ledger = mechanism_bitmap_ledger();
                BITMAPINFO information{};
                information.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                information.bmiHeader.biWidth = width;
                information.bmiHeader.biHeight = -height;
                information.bmiHeader.biPlanes = 1;
                information.bmiHeader.biBitCount = 32;
                information.bmiHeader.biCompression = BI_RGB;
                void* bits = nullptr;
                bitmap_ = CreateDIBSection(dc_, &information, DIB_RGB_COLORS, &bits, nullptr, 0);
                if (!bitmap_)
                {
                    if (!DeleteDC(dc_)) { ++ledger.dc_delete_failures; }
                    throw std::runtime_error("Could not create the native guide bitmap");
                }
                pixels_ = static_cast<std::uint32_t*>(bits);
                previous_ = SelectObject(dc_, bitmap_);
                if (previous_ == nullptr || previous_ == HGDI_ERROR)
                {
                    if (!DeleteObject(bitmap_)) { ++ledger.bitmap_delete_failures; }
                    if (!DeleteDC(dc_)) { ++ledger.dc_delete_failures; }
                    throw std::runtime_error("Could not select the native guide bitmap");
                }
                ++ledger.created; ++ledger.live;
                SetBkMode(dc_, TRANSPARENT);
                clear();
            }
            ~MechanismBitmap()
            {
                MechanismBitmapLedger& ledger = mechanism_bitmap_ledger();
                if (SelectObject(dc_, previous_) != bitmap_) { ++ledger.restore_failures; }
                if (!DeleteObject(bitmap_)) { ++ledger.bitmap_delete_failures; }
                if (!DeleteDC(dc_)) { ++ledger.dc_delete_failures; }
                --ledger.live;
            }
            MechanismBitmap(const MechanismBitmap&) = delete;
            MechanismBitmap& operator=(const MechanismBitmap&) = delete;
            HDC dc() const { return dc_; }
            RECT rectangle() const { return { 0, 0, width_, height_ }; }
            static std::uint32_t colour(COLORREF rgb)
            {
                return ((rgb & 0xffu) << 16) | (rgb & 0xff00u) | ((rgb >> 16) & 0xffu);
            }
            std::uint32_t pixel(int x, int y) const
            {
                require(x >= 0 && y >= 0 && x < width_ && y < height_, "Native guide sample escaped its bitmap");
                return pixels_[static_cast<std::size_t>(y) * width_ + x] & 0xffffffu;
            }
            void clear()
            {
                GdiFlush();
                std::fill(pixels_, pixels_ + static_cast<std::size_t>(width_) * height_, colour(RGB(28, 36, 46)));
            }
            std::size_t count(const RECT& bounds, COLORREF rgb) const
            {
                std::size_t found = 0;
                for (LONG y = bounds.top; y < bounds.bottom; ++y)
                    for (LONG x = bounds.left; x < bounds.right; ++x)
                        if (pixel(x, y) == colour(rgb)) { ++found; }
                return found;
            }
            std::vector<std::uint32_t> crop(const RECT& bounds) const
            {
                std::vector<std::uint32_t> result;
                for (LONG y = bounds.top; y < bounds.bottom; ++y)
                    for (LONG x = bounds.left; x < bounds.right; ++x) { result.push_back(pixel(x, y)); }
                return result;
            }
            void save(const std::filesystem::path& path) const
            {
                GdiFlush();
                const DWORD bytes = static_cast<DWORD>(static_cast<std::size_t>(width_) * height_ * sizeof(std::uint32_t));
                BITMAPFILEHEADER file{};
                file.bfType = 0x4d42;
                file.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
                file.bfSize = file.bfOffBits + bytes;
                BITMAPINFOHEADER information{};
                information.biSize = sizeof(information);
                information.biWidth = width_;
                information.biHeight = -height_;
                information.biPlanes = 1;
                information.biBitCount = 32;
                information.biCompression = BI_RGB;
                information.biSizeImage = bytes;
                std::ofstream output(path, std::ios::binary | std::ios::trunc);
                output.write(reinterpret_cast<const char*>(&file), sizeof(file));
                output.write(reinterpret_cast<const char*>(&information), sizeof(information));
                output.write(reinterpret_cast<const char*>(pixels_), bytes);
                require(output.good(), "Could not retain a native guide bitmap");
            }
        private:
            int width_, height_;
            HDC dc_ = nullptr;
            HBITMAP bitmap_ = nullptr;
            HGDIOBJ previous_ = nullptr;
            std::uint32_t* pixels_ = nullptr;
        };

        struct NativeGuideStroke { double x1, y1, x2, y2; };
        struct NativeGuideLabel { double x, y; std::wstring words; };
        struct NativeGuideOracle
        {
            RECT panel;
            std::vector<NativeGuideStroke> strokes;
            std::vector<NativeGuideLabel> labels;
            LONG line_height = 0;
        };

        // Literal screen grammar is independent of the production drawing helper.
        NativeGuideOracle native_guide_oracle(const RECT& body, bool gate, int a_heading, UINT dpi, HDC dc)
        {
            const double scale = static_cast<double>(dpi) / 96.0;
            SIZE actual_line{};
            require(GetTextExtentPoint32W(dc, L"A", 1, &actual_line) != 0,
                "Could not measure the native oracle's actual font line");
            const double x = body.left + (body.right - body.left - 132.0 * scale) * 0.5;
            const double title_bottom = 4.0 * scale + actual_line.cy;
            const double first_row = std::max((gate ? 22.0 : 24.0) * scale, title_bottom + 4.0 * scale);
            const double second_row = gate ? first_row + 4.0 * scale :
                std::max(42.0 * scale, first_row + actual_line.cy + 4.0 * scale);
            const double feet = std::max(39.0 * scale, second_row + actual_line.cy + 4.0 * scale);
            const double height = gate ? feet + 4.0 * scale : second_row + actual_line.cy + 4.0 * scale;
            const double y = body.top - height - 6.0 * scale;
            NativeGuideOracle result;
            result.line_height = actual_line.cy;
            result.panel = { static_cast<LONG>(x), static_cast<LONG>(y),
                static_cast<LONG>(x + 132.0 * scale), static_cast<LONG>(y + height) };
            const auto stroke = [&](double x1, double y1, double x2, double y2)
            { result.strokes.push_back({ x + x1 * scale, y + y1, x + x2 * scale, y + y2 }); };
            const auto label = [&](double left, double top, const wchar_t* words)
            { result.labels.push_back({ x + left * scale, y + top, words }); };
            if (gate)
            {
                label(8, 4.0 * scale, L"PAINT GATE"); label(48, second_row, L"A / B");
                stroke(24, first_row, 108, first_row); stroke(24, first_row, 24, feet); stroke(108, first_row, 108, feet);
                stroke(24, feet, 39, feet); stroke(93, feet, 108, feet);
            }
            else
            {
                label(8, 4.0 * scale, L"PAINT FERRY"); label(10, first_row, L"A"); label(10, second_row, L"B");
                for (int row = 0; row < 2; ++row)
                {
                    const int heading = row == 0 ? a_heading : -a_heading;
                    const double tail = heading > 0 ? 34.0 : 118.0;
                    const double tip = heading > 0 ? 118.0 : 34.0;
                    const double centre = (row == 0 ? first_row : second_row) + actual_line.cy * 0.5;
                    stroke(tail, centre, tip, centre);
                    stroke(tip - heading * 7.0, centre - 5.0 * scale, tip, centre);
                    stroke(tip - heading * 7.0, centre + 5.0 * scale, tip, centre);
                }
            }
            return result;
        }

        std::size_t verify_native_guide(const MechanismBitmap& bitmap, const NativeGuideOracle& oracle, HFONT font, UINT dpi)
        {
            require(contained(bitmap.rectangle(), oracle.panel), "Native guide panel escaped its canvas fixture");
            const std::size_t panel = bitmap.count(oracle.panel, RGB(36, 45, 57));
            require(panel > static_cast<std::size_t>((oracle.panel.right - oracle.panel.left) *
                (oracle.panel.bottom - oracle.panel.top) / 2), "Actual editor drawing omitted or displaced its mechanism panel");
            RECT guide_region = bitmap.rectangle();
            // The unrelated canvas legend has antialiased text at the top. Its
            // blended edge pixels are not evidence about a mechanism plaque.
            guide_region.top = MulDiv(40, static_cast<int>(dpi), 96);
            require(bitmap.count(guide_region, RGB(36, 45, 57)) == panel,
                "Actual editor drawing extended its mechanism panel beyond the independent bounds");
            const double half_pen = static_cast<double>(MulDiv(2, static_cast<int>(dpi), 96)) * 0.5;
            for (const NativeGuideStroke& line : oracle.strokes)
            {
                require(std::min(line.x1, line.x2) - half_pen >= oracle.panel.left &&
                    std::max(line.x1, line.x2) + half_pen <= oracle.panel.right &&
                    std::min(line.y1, line.y2) - half_pen >= oracle.panel.top &&
                    std::max(line.y1, line.y2) + half_pen <= oracle.panel.bottom,
                    "Actual-width mechanism stroke cannot fit its panel");
                const int middle_x = static_cast<int>((line.x1 + line.x2) * 0.5);
                const int middle_y = static_cast<int>((line.y1 + line.y2) * 0.5);
                bool ink = false;
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx)
                        ink = ink || bitmap.pixel(middle_x + dx, middle_y + dy) == MechanismBitmap::colour(RGB(240, 225, 190));
                require(ink, "Actual GDI guide omitted an independent lintel/post/arrow stroke");
            }
            HGDIOBJ previous = SelectObject(bitmap.dc(), font);
            std::size_t glyph_pixels = 0;
            std::vector<RECT> label_bounds;
            for (const NativeGuideLabel& label : oracle.labels)
            {
                SIZE extent{};
                require(GetTextExtentPoint32W(bitmap.dc(), label.words.c_str(), static_cast<int>(label.words.size()), &extent) != 0,
                    "Could not measure actual mechanism-label GDI extents");
                const RECT bounds{ static_cast<LONG>(label.x), static_cast<LONG>(label.y),
                    static_cast<LONG>(label.x) + extent.cx, static_cast<LONG>(label.y) + extent.cy };
                if (!contained(oracle.panel, bounds))
                {
                    throw std::runtime_error("Actual GDI mechanism label '" + narrow(label.words) + "' at DPI " +
                        std::to_string(dpi) + " has bounds [" + std::to_string(bounds.left) + "," +
                        std::to_string(bounds.top) + "," + std::to_string(bounds.right) + "," +
                        std::to_string(bounds.bottom) + "] outside panel [" + std::to_string(oracle.panel.left) + "," +
                        std::to_string(oracle.panel.top) + "," + std::to_string(oracle.panel.right) + "," +
                        std::to_string(oracle.panel.bottom) + "].");
                }
                for (const RECT& other : label_bounds)
                { require(!overlaps(bounds, other), "Actual GDI mechanism label cells overlap"); }
                for (const NativeGuideStroke& stroke : oracle.strokes)
                {
                    const RECT stroke_bounds{
                        static_cast<LONG>(std::floor(std::min(stroke.x1, stroke.x2) - half_pen)),
                        static_cast<LONG>(std::floor(std::min(stroke.y1, stroke.y2) - half_pen)),
                        static_cast<LONG>(std::ceil(std::max(stroke.x1, stroke.x2) + half_pen)),
                        static_cast<LONG>(std::ceil(std::max(stroke.y1, stroke.y2) + half_pen)) };
                    require(!overlaps(bounds, stroke_bounds), "Actual GDI label cell intersects the gate/arrow schematic");
                    if (label_bounds.empty())
                    { require(bounds.bottom <= stroke_bounds.top, "Mechanism title does not precede its schematic"); }
                }
                label_bounds.push_back(bounds);
                std::size_t ink = 0;
                for (LONG y = bounds.top; y < bounds.bottom; ++y)
                    for (LONG x = bounds.left; x < bounds.right; ++x)
                    {
                        const bool in_stroke = std::any_of(oracle.strokes.begin(), oracle.strokes.end(), [&](const NativeGuideStroke& line)
                        {
                            return x >= std::min(line.x1, line.x2) - half_pen - 1.0 &&
                                x <= std::max(line.x1, line.x2) + half_pen + 1.0 &&
                                y >= std::min(line.y1, line.y2) - half_pen - 1.0 &&
                                y <= std::max(line.y1, line.y2) + half_pen + 1.0;
                        });
                        if (!in_stroke && bitmap.pixel(x, y) != MechanismBitmap::colour(RGB(36, 45, 57))) { ++ink; }
                    }
                require(ink > 4, "Actual GDI drawing omitted a rule label or team letter");
                glyph_pixels += ink;
            }
            SelectObject(bitmap.dc(), previous);
            return glyph_pixels;
        }

        std::vector<std::pair<std::string, bool>> native_history(LevelDocument& document)
        {
            const std::string current = document.serialize();
            std::vector<std::pair<std::string, bool>> history;
            std::size_t original = 0;
            while (document.undo()) { ++original; }
            history.emplace_back(document.serialize(), document.dirty());
            std::size_t revisions = 0;
            while (document.redo()) { history.emplace_back(document.serialize(), document.dirty()); ++revisions; }
            for (std::size_t index = original; index < revisions; ++index)
            { require(document.undo(), "Could not restore native guide history fixture"); }
            require(document.serialize() == current, "Native guide history inspection changed the current revision");
            return history;
        }

        std::vector<double> native_geometry(const LevelDocument& document, const Entry& entry)
        {
            std::vector<double> result;
            const auto append = [&](const Rect& rectangle)
            { for (double value : { rectangle.x, rectangle.y, rectangle.width, rectangle.height }) { result.push_back(value); } };
            append(document.rectangle(entry));
            if (const auto motion = document.motion_guide(entry))
            { append(motion->anchor); append(motion->initial_pose); append(motion->travel_envelope); append(motion->standing_envelope); }
            return result;
        }
    }

    inline void EditorDpiProbe::check_mechanisms(Editor& editor, const std::filesystem::path& scratch)
    {
        const std::array<UINT, 3> dpis{ 96u, 144u, 192u };
        std::array<std::unique_ptr<MechanismBitmap>, 3> sheets;
        for (std::size_t index = 0; index < dpis.size(); ++index)
            sheets[index] = std::make_unique<MechanismBitmap>(MulDiv(1500, static_cast<int>(dpis[index]), 96),
                MulDiv(900, static_cast<int>(dpis[index]), 96));
        const std::array<const char*, 5> names{ "eraser", "gate", "cycle", "ferry-a-right", "ferry-a-left" };
        const std::array<const wchar_t*, 3> states{ L"unselected", L"selected", L"drag preview" };
        std::ofstream evidence(scratch / "mechanism-canvas.csv", std::ios::trunc);
        evidence << "case,dpi,state,panel_x,panel_y,panel_width,panel_height,font_line_height,glyph_pixels\n";
        const RECT work = editor_monitor_work_area(MonitorFromWindow(editor.window_, MONITOR_DEFAULTTONEAREST));
        std::size_t contexts = 0;
        for (std::size_t kind = 0; kind < names.size(); ++kind)
        {
            editor.dragging_ = false; editor.resizing_ = false; editor.panning_ = false;
            editor.selection_.reset(); editor.document_.create();
            const bool moving = kind >= 2;
            const Entry object = editor.document_.add_object(moving ? "MovingPaintable" : "TeamGate",
                moving ? Rect{ 600, 600, 320, 20 } : Rect{ 600, 600, 80, 240 });
            if (kind == 0) { editor.document_.set_property(object, "gate", ""); }
            if (moving) { editor.document_.set_property(object, "motion/phase", "0.25"); }
            if (kind >= 3)
            {
                editor.document_.set_property(object, "motion/drive", "paint");
                editor.document_.set_property(object, "motion/team_a_heads", kind == 3 ? "right" : "left");
            }
            const Entry level{ EntryKind::level, 0, "Level settings" };
            editor.document_.set_property(level, "/display_name", "Native guide first");
            editor.document_.set_property(level, "/display_name", "Native guide second");
            require(editor.document_.undo(), "Native guide fixture needs a redo branch");
            editor.refresh();
            DrawItem item;
            bool found = false;
            for (const DrawItem& candidate : editor.draw_items_)
                if (same_entry(candidate.entry, object)) { item = candidate; found = true; break; }
            require(found, "Actual editor refresh did not construct its mechanism fixture");
            editor.draw_items_ = { item }; editor.show_blockout_ = false;
            const std::string json = editor.document_.serialize();
            const std::filesystem::path path = editor.document_.path();
            const bool dirty = editor.document_.dirty();
            const auto history = native_history(editor.document_);
            const auto geometry = native_geometry(editor.document_, object);
            const bool has_guide = kind == 1 || kind >= 3;
            for (std::size_t dpi_index = 0; dpi_index < dpis.size(); ++dpi_index)
            {
                const UINT dpi = dpis[dpi_index];
                const double scale = static_cast<double>(dpi) / 96.0;
                const RECT suggested = initial_editor_rectangle(dpi, work);
                SendMessageW(editor.window_, WM_DPICHANGED, MAKEWPARAM(dpi, dpi), reinterpret_cast<LPARAM>(&suggested));
                const Rect pose = item.motion ? item.motion->initial_pose : item.rectangle;
                editor.zoom_ = 0.5 * scale;
                editor.view_x_ = pose.x + pose.width * 0.5 - 150.0 * scale / editor.zoom_;
                editor.view_y_ = pose.y - 160.0 * scale / editor.zoom_;
                const POINT hit_point = editor.screen_point(pose.x + pose.width * 0.5, pose.y + pose.height * 0.5);
                const auto before_hit = editor.hit(hit_point);
                require(before_hit && same_entry(*before_hit, object), "Native mechanism fixture is not hit-testable");
                std::vector<std::uint32_t> unselected_guide;
                for (int state = 0; state < 3; ++state)
                {
                    editor.selection_ = state == 0 ? std::optional<Entry>{} : object;
                    editor.dragging_ = state == 2; editor.resizing_ = state == 2;
                    editor.drag_preview_ = { item.rectangle.x - 40, item.rectangle.y - 40,
                        item.rectangle.width + 40, item.rectangle.height };
                    Rect drawn = state == 2 ? editor.drag_preview_ : item.rectangle;
                    if (item.motion)
                    {
                        const auto motion = state == 2 ? editor.document_.motion_guide(object, editor.drag_preview_) : item.motion;
                        require(motion.has_value(), "Valid native mover preview lost its initial motion pose");
                        drawn = motion->initial_pose;
                    }
                    const RECT body = editor.screen_rect(drawn);
                    const int width = MulDiv(300, static_cast<int>(dpi), 96), height = MulDiv(300, static_cast<int>(dpi), 96);
                    MechanismBitmap direct(width, height), composite(width, height);
                    SelectObject(direct.dc(), editor.font_);
                    const NativeGuideOracle oracle = native_guide_oracle(body, kind == 1, kind == 4 ? -1 : 1, dpi, direct.dc());
                    editor.draw_item(direct.dc(), item);
                    const RECT client = composite.rectangle();
                    editor.paint(composite.dc(), client);
                    GdiFlush();
                    std::size_t glyph_pixels = 0;
                    if (has_guide)
                    {
                        glyph_pixels = verify_native_guide(direct, oracle, editor.font_, dpi);
                        verify_native_guide(composite, oracle, editor.font_, dpi);
                        const auto crop = direct.crop(oracle.panel);
                        if (state == 0) { unselected_guide = crop; }
                        else require(crop == unselected_guide, "Selection/valid drag preview changed actual guide content instead of translating it");
                        require(!editor.hit(POINT{ (oracle.panel.left + oracle.panel.right) / 2,
                            (oracle.panel.top + oracle.panel.bottom) / 2 }), "A drawn schematic became hit-test geometry");
                    }
                    else
                    {
                        RECT guide_region = direct.rectangle();
                        guide_region.top = MulDiv(40, static_cast<int>(dpi), 96);
                        require(direct.count(guide_region, RGB(36, 45, 57)) == 0 &&
                            composite.count(guide_region, RGB(36, 45, 57)) == 0,
                            "Actual ordinary eraser/cycle drawing gained a paint-mechanism plaque");
                    }
                    const auto after_hit = editor.hit(hit_point);
                    require(after_hit && same_entry(*after_hit, object) && editor.document_.serialize() == json &&
                        editor.document_.path() == path && editor.document_.dirty() == dirty &&
                        editor.document_.can_undo() && editor.document_.can_redo() &&
                        native_geometry(editor.document_, object) == geometry,
                        "Actual native drawing changed document/history, hit tests or motion admission envelopes");
                    HDC sheet = sheets[dpi_index]->dc();
                    const int column = static_cast<int>(kind) * width, row = state * height;
                    BitBlt(sheet, column, row, width, height, direct.dc(), 0, 0, SRCCOPY);
                    HGDIOBJ previous = SelectObject(sheet, editor.font_);
                    SetTextColor(sheet, RGB(220, 230, 240));
                    const std::wstring caption = std::wstring(names[kind], names[kind] + std::char_traits<char>::length(names[kind])) + L" / " + states[state];
                    TextOutW(sheet, column + MulDiv(8, static_cast<int>(dpi), 96), row + MulDiv(8, static_cast<int>(dpi), 96),
                        caption.c_str(), static_cast<int>(caption.size()));
                    SelectObject(sheet, previous);
                    evidence << names[kind] << ',' << dpi << ',' << state << ',' << oracle.panel.left << ',' << oracle.panel.top << ','
                        << oracle.panel.right - oracle.panel.left << ',' << oracle.panel.bottom - oracle.panel.top << ','
                        << oracle.line_height << ',' << glyph_pixels << '\n';
                    ++contexts;
                }
            }
            editor.dragging_ = false; editor.resizing_ = false;
            require(native_history(editor.document_) == history && native_geometry(editor.document_, object) == geometry,
                "Native guide matrix changed the complete document history or admitted geometry");
        }
        require(evidence.good(), "Could not retain native guide state evidence");
        for (std::size_t index = 0; index < dpis.size(); ++index)
            sheets[index]->save(scratch / ("mechanism-canvas-" + std::to_string(dpis[index]) + ".bmp"));
        std::cout << "PASS native editor mechanisms: " << contexts << " eraser/gate/cycle/both-heading contexts, actual GDI "
            "labels/strokes at96/144/192, selection/drag translation, unchanged history/hit/envelopes; 3 BMPs + CSV\n";
    }
}
