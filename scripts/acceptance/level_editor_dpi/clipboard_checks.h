#pragma once

// ED#27: Edit > Copy and Paste through the real CF_UNICODETEXT clipboard in
// the hidden shell. Include after gesture_checks.h. The probe keeps whatever
// text the clipboard held before it and puts it back afterwards; a clipboard
// that held no text is left empty.
#include <cstring>
namespace colourwars_editor
{
    namespace
    {
        std::optional<std::wstring> clipboard_text()
        {
            for (int attempt = 0; attempt < 50; ++attempt)
            {
                if (OpenClipboard(nullptr))
                {
                    std::optional<std::wstring> text;
                    if (HANDLE data = GetClipboardData(CF_UNICODETEXT))
                    {
                        if (const auto* locked = static_cast<const wchar_t*>(GlobalLock(data)))
                        {
                            text = std::wstring(locked);
                            GlobalUnlock(data);
                        }
                    }
                    CloseClipboard();
                    return text;
                }
                Sleep(20);
            }
            throw std::runtime_error("Could not open the clipboard to read it");
        }

        void set_clipboard_text(const std::optional<std::wstring>& text)
        {
            for (int attempt = 0; attempt < 50; ++attempt)
            {
                if (OpenClipboard(nullptr))
                {
                    bool done = EmptyClipboard() != 0;
                    if (done && text)
                    {
                        const std::size_t bytes = (text->size() + 1) * sizeof(wchar_t);
                        HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
                        void* target = memory ? GlobalLock(memory) : nullptr;
                        if (target)
                        {
                            std::memcpy(target, text->c_str(), bytes);
                            GlobalUnlock(memory);
                            done = SetClipboardData(CF_UNICODETEXT, memory) != nullptr;
                            if (!done) { GlobalFree(memory); }
                        }
                        else { if (memory) { GlobalFree(memory); } done = false; }
                    }
                    CloseClipboard();
                    if (!done) { throw std::runtime_error("Could not write the clipboard"); }
                    return;
                }
                Sleep(20);
            }
            throw std::runtime_error("Could not open the clipboard to write it");
        }

        // Puts the caller's clipboard text and thread key state back on every exit.
        class ClipboardProbeState
        {
        public:
            ClipboardProbeState() : text_(clipboard_text())
            {
                require(GetKeyboardState(keys_.data()) != 0, "Could not retain the clipboard probe's key state");
            }
            ~ClipboardProbeState()
            {
                SetKeyboardState(keys_.data());
                try { set_clipboard_text(text_); }
                catch (const std::exception&) {}
            }
            ClipboardProbeState(const ClipboardProbeState&) = delete;
            ClipboardProbeState& operator=(const ClipboardProbeState&) = delete;
            void control(bool pressed)
            {
                auto state = keys_;
                state[VK_CONTROL] = state[VK_LCONTROL] = static_cast<BYTE>(pressed ? 0x80 : 0);
                state[VK_RCONTROL] = 0;
                state[VK_SHIFT] = state[VK_LSHIFT] = state[VK_RSHIFT] = 0;
                require(SetKeyboardState(state.data()) != 0 && ((GetKeyState(VK_CONTROL) & 0x8000) != 0) == pressed,
                    "Could not establish the clipboard probe's Ctrl state");
            }
        private:
            std::optional<std::wstring> text_;
            std::array<BYTE, 256> keys_{};
        };

        void pump_messages()
        {
            MSG pending{};
            while (PeekMessageW(&pending, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&pending);
                DispatchMessageW(&pending);
            }
        }
    }

    inline void EditorDpiProbe::check_clipboard(Editor& editor, const std::filesystem::path& scratch)
    {
        const auto original_path = editor.document_.path();
        const std::string original_document = editor.document_.serialize();
        const auto original_selection = editor.selection_;
        const HWND original_focus = GetFocus();
        require(!editor.pending() && !editor.document_.dirty() && !IsWindowVisible(editor.window_),
            "Clipboard qualification needs a clean private hidden shell");
        ClipboardProbeState state;
        state.control(false);

        LevelDocument source_seed;
        const Entry object = source_seed.add_object("StructurePaintable", { 600, 500, 160, 80 });
        const auto source_stage = scratch / "clipboard-native-source.json";
        source_seed.save(source_stage);
        const auto target_stage = scratch / "clipboard-native-target.json";
        LevelDocument target_seed;
        target_seed.save(target_stage);
        const auto menu_enabled = [&](int id, bool expected)
        {
            const UINT menu_state = GetMenuState(GetSubMenu(editor.menu_, 1), static_cast<UINT>(id), MF_BYCOMMAND);
            require(menu_state != 0xffffffffu && ((menu_state & (MF_DISABLED | MF_GRAYED)) == 0) == expected,
                "Edit > Copy/Paste availability does not follow the selection");
        };
        const auto key = [&](HWND target, WPARAM virtual_key)
        {
            state.control(true);
            MSG message{ target, WM_KEYDOWN, virtual_key, 1, static_cast<DWORD>(GetMessageTime()), {} };
            editor.dispatch(message);
            MSG release{ target, WM_KEYUP, virtual_key, static_cast<LPARAM>(0xC0000001u), static_cast<DWORD>(GetMessageTime()), {} };
            editor.dispatch(release);
            state.control(false);
            pump_messages();
        };

        // Copy from the first stage: Edit > Copy and the Ctrl+C accelerator.
        editor.cancel_drag(); editor.document_.load(source_stage); editor.selection_ = object; editor.refresh();
        menu_enabled(edit_copy, true); menu_enabled(edit_paste, true);
        const std::wstring expected = wide(editor.document_.copy_text(object));
        set_clipboard_text(L"sentinel");
        SendMessageW(editor.window_, WM_COMMAND, edit_copy, 0);
        require(clipboard_text() == expected, "Edit > Copy did not put the object's JSON on the clipboard as CF_UNICODETEXT");
        require(!editor.document_.dirty() && !editor.document_.can_undo(), "Copy changed the source document");
        set_clipboard_text(L"sentinel");
        SetFocus(editor.canvas_);
        key(editor.canvas_, 'C');
        require(clipboard_text() == expected, "Ctrl+C on the canvas did not copy the selected object");

        // Paste into a second stage, as one undo step that selects the copy.
        editor.document_.load(target_stage); editor.selection_.reset(); editor.refresh();
        const std::string target_baseline = editor.document_.serialize();
        const std::size_t before = editor.document_.entries().size();
        SendMessageW(editor.window_, WM_COMMAND, edit_paste, 0);
        const std::string pasted = editor.document_.serialize();
        require(editor.document_.entries().size() == before + 1 && editor.selection_ &&
            editor.selection_->kind == EntryKind::collision &&
            wide(editor.document_.copy_text(*editor.selection_)) == expected,
            "Edit > Paste did not append and select the copied object");
        require(editor.document_.dirty() && editor.document_.can_undo() && !editor.document_.can_redo() &&
            editor.document_.undo() && editor.document_.serialize() == target_baseline && !editor.document_.can_undo() &&
            editor.document_.redo() && editor.document_.serialize() == pasted,
            "Edit > Paste was not exactly one undo step");
        editor.document_.undo(); editor.refresh();
        SetFocus(editor.canvas_);
        key(editor.canvas_, 'V');
        require(editor.document_.serialize() == pasted, "Ctrl+V on the canvas did not paste the copied object");

        // Refusals leave the document and its history as they were.
        const auto refused = [&](const std::optional<std::wstring>& text, const char* diagnostic)
        {
            set_clipboard_text(text);
            const std::string ready = editor.document_.serialize();
            bool rejected = false;
            try { editor.command(edit_paste); }
            catch (const std::exception& error)
            { rejected = std::string(error.what()).find(diagnostic) != std::string::npos; }
            require(rejected && editor.document_.serialize() == ready && editor.document_.can_undo() &&
                !editor.document_.can_redo(), "A refused native paste changed the document or its history");
        };
        refused(L"not json", "does not hold a copied level object");
        refused(std::nullopt, "holds no text to paste");
        refused(L"{\"colourwars_level_object\": 1, \"list\": \"team_a\", \"object\": {\"x\": 2800, \"y\": 1000}}",
            "Team A spawns on the left half");

        // Level settings are not copyable: the menu is greyed and copying refuses.
        editor.selection_ = Entry{ EntryKind::level, 0, "Level settings" }; editor.refresh();
        menu_enabled(edit_copy, false);

        // The property field keeps its own Ctrl+C and Ctrl+V.
        editor.selection_ = *std::find_if(editor.entries_.begin(), editor.entries_.end(),
            [](const Entry& entry) { return entry.kind == EntryKind::collision; });
        editor.refresh();
        int x_row = -1;
        for (std::size_t index = 0; index < editor.properties_.size(); ++index)
            if (editor.properties_[index].path == "rectangle/x") x_row = static_cast<int>(index);
        require(x_row >= 0, "Clipboard fixture lacks its X inspector row");
        editor.select_property(x_row);
        SetFocus(editor.value_edit_);
        require(GetFocus() == editor.value_edit_, "Could not focus the hidden property field");
        const std::string field_ready = editor.document_.serialize();
        set_clipboard_text(L"sentinel");
        SendMessageW(editor.value_edit_, EM_SETSEL, 0, -1);
        key(editor.value_edit_, 'C');
        require(clipboard_text() == window_text(editor.value_edit_) && editor.document_.serialize() == field_ready,
            "Ctrl+C in the property field copied an object instead of its text");
        set_clipboard_text(L"4321");
        SendMessageW(editor.value_edit_, EM_SETSEL, 0, -1);
        key(editor.value_edit_, 'V');
        require(window_text(editor.value_edit_) == L"4321" && editor.document_.serialize() == field_ready,
            "Ctrl+V in the property field pasted an object instead of text");
        editor.select_property(x_row);

        editor.cancel_drag();
        editor.document_.load(original_path); editor.selection_ = original_selection; editor.refresh();
        if (!original_selection) editor.selection_.reset();
        SetFocus(original_focus);
        require(editor.document_.serialize() == original_document && editor.document_.path() == original_path &&
            !editor.document_.dirty() && !editor.pending() && !IsWindowVisible(editor.window_),
            "Clipboard qualification did not restore the private caller shell");
        std::cout << "PASS native clipboard: Edit > Copy and Ctrl+C put CF_UNICODETEXT JSON on the clipboard, Edit > Paste "
            "and Ctrl+V append it to a second stage as one undo step, malformed/empty/invalid clipboard refused without "
            "change, level settings not copyable, the property field keeps its own Ctrl+C/V, caller clipboard restored\n";
    }
}
