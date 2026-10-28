#pragma once

#include "editor/level_document.h"
#include "editor/blockout_aids.h"
#include "editor/content_catalog.h"

#include <windows.h>

#include <exception>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

// Editor is defined across editor_window.cpp (window shell) and editor_canvas.cpp (level canvas).
namespace colourwars_editor
{
    enum Command : int
    {
        file_new = 100, file_open, file_save, file_save_as, file_save_user_stage, file_exit,
        edit_undo, edit_redo, edit_duplicate, edit_delete,
        view_fit, view_selection, view_match, view_bounds, view_blockout, help_controls,
        add_object, apply_property, revert_property, objects_list,
        properties_list, property_value, palette, grid_size
    };

    inline bool same_entry(const Entry& a, const Entry& b)
    {
        return a.kind == b.kind && a.index == b.index;
    }

    struct DrawItem
    {
        Entry entry;
        Rect rectangle;
        std::map<std::string, std::string> values;
        std::optional<MotionGuide> motion;
        std::optional<PaintPolygon> slab;
    };

    class Editor
    {
    public:
        explicit Editor(HINSTANCE instance);
        ~Editor();
        int run(int show, const std::filesystem::path& initial_path);

    private:
        friend struct EditorDpiProbe;
        HINSTANCE instance_;
        UINT dpi_ = 96;
        HWND window_ = nullptr;
        HWND canvas_ = nullptr;
        HWND object_list_ = nullptr;
        HWND property_list_ = nullptr;
        HWND value_edit_ = nullptr;
        HWND property_label_ = nullptr;
        HWND status_ = nullptr;
        HWND palette_ = nullptr;
        HWND grid_ = nullptr;
        HWND object_heading_ = nullptr;
        HWND inspector_heading_ = nullptr;
        HWND hint_ = nullptr;
        HWND apply_ = nullptr;
        HWND revert_ = nullptr;
        HFONT font_ = nullptr;
        HMENU menu_ = nullptr;
        HACCEL accelerators_ = nullptr;
        std::vector<HWND> toolbar_;
        LevelDocument document_;
        std::vector<Entry> entries_;
        std::vector<DrawItem> draw_items_;
        std::vector<Property> properties_;
        std::optional<Entry> selection_;
        int property_index_ = -1;
        bool refreshing_ = false;
        bool show_bounds_ = false;
        bool show_blockout_ = true;
        std::optional<BlockoutTuning> blockout_tuning_;
        std::vector<ScoringFace> scoring_faces_;
        std::wstring blockout_error_;
        // Reread with tuning on every refresh; warnings only, never a blocked edit.
        ContentCatalog content_catalog_;
        std::vector<std::string> unresolved_;
        bool dragging_ = false;
        bool resizing_ = false;
        bool panning_ = false;
        POINT mouse_start_{};
        Rect drag_original_{};
        Rect drag_preview_{};
        double view_x_ = 0.0;
        double view_y_ = 0.0;
        double zoom_ = 0.2;
        int snap_ = 25;

        static LRESULT CALLBACK window_proc(HWND, UINT, WPARAM, LPARAM);
        static LRESULT CALLBACK canvas_proc(HWND, UINT, WPARAM, LPARAM);
        LRESULT message(UINT, WPARAM, LPARAM);
        LRESULT canvas_message(UINT, WPARAM, LPARAM);
        HWND control(const wchar_t*, const wchar_t*, DWORD, int);
        void create_controls();
        void create_menu();
        void create_window(int show, const std::filesystem::path& initial_path);
        void change_dpi(UINT dpi, const RECT& suggested);
        int pixels(int logical) const;
        void layout();
        void command(int);
        void refresh(bool keep_property = false);
        void refresh_properties(bool keep_property);
        void select_property(int);
        bool apply_pending();
        bool pending() const;
        bool confirm_discard();
        bool confirm_unresolved();
        bool save(bool save_as);
        std::optional<std::filesystem::path> choose_file(bool save_as);
        bool save_user_stage();
        std::optional<std::filesystem::path> choose_user_stage(const std::filesystem::path& folder);
        void select(const Entry&);
        void set_status(const std::wstring&);
        void error(const std::exception&);
        void paint(HDC, const RECT&);
        void line(HDC, int, int, int, int, COLORREF, int width = 1, int style = PS_SOLID) const;
        void outline(HDC, const RECT&, COLORREF, int width = 1, int style = PS_SOLID) const;
        void draw_item(HDC, const DrawItem&);
        void draw_blockout(HDC);
        void fit(const Rect&);
        RECT screen_rect(const Rect&) const;
        POINT screen_point(double, double) const;
        std::optional<Entry> hit(POINT) const;
        double snapped(double) const;
        void cancel_drag();
        void nudge(int, int);
    };
}
