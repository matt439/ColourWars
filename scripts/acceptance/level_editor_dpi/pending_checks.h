#pragma once

#include "scripts/acceptance/level_editor_dpi/pending_modal_driver.h"
#include "editor/user_stage_save.h"
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <optional>

namespace colourwars_editor
{
    namespace
    {
        std::optional<std::wstring> pending_windows_profile()
        {
            SetLastError(ERROR_SUCCESS);
            const DWORD needed = GetEnvironmentVariableW(L"LOCALAPPDATA", nullptr, 0);
            if (needed == 0)
            {
                const DWORD error = GetLastError();
                require(error == ERROR_SUCCESS || error == ERROR_ENVVAR_NOT_FOUND, "Could not read the pending probe profile key");
                return error == ERROR_ENVVAR_NOT_FOUND ? std::nullopt : std::optional<std::wstring>{ L"" };
            }
            std::vector<wchar_t> value(needed);
            const DWORD copied = GetEnvironmentVariableW(L"LOCALAPPDATA", value.data(), needed);
            require(copied > 0 && copied < needed, "Pending probe profile changed while being read");
            return std::wstring(value.data(), copied);
        }
        std::optional<std::wstring> pending_crt_profile()
        {
            wchar_t* value = nullptr; std::size_t length = 0;
            require(_wdupenv_s(&value, &length, L"LOCALAPPDATA") == 0, "Could not read the pending probe CRT profile");
            const std::unique_ptr<wchar_t, decltype(&std::free)> owned(value, &std::free);
            return owned ? std::optional<std::wstring>{ owned.get() } : std::nullopt;
        }
        std::string pending_file_bytes(const std::filesystem::path& path)
        {
            std::ifstream input(path, std::ios::binary);
            require(input.good(), "Could not read the pending probe private stage");
            const std::string result{ std::istreambuf_iterator<char>(input), {} };
            require(!input.bad(), "Could not read the complete pending probe private stage");
            return result;
        }
    }

    inline void EditorDpiProbe::run_pending(const std::filesystem::path& scratch)
    {
        require(scratch.is_absolute() && std::filesystem::is_directory(scratch) && std::filesystem::is_empty(scratch),
            "Pending property qualification needs an existing empty absolute scratch directory");
        const auto old_windows = pending_windows_profile(), old_crt = pending_crt_profile();
        const auto profile = scratch / "profile";
        require(!std::filesystem::exists(profile) && std::filesystem::create_directory(profile),
            "Pending property qualification needs a fresh private profile");
        bool environment_restored = false;
        std::size_t cases = 0;
        {
            struct RestoreProfile
            {
                const std::optional<std::wstring>& windows;
                const std::optional<std::wstring>& crt;
                bool& restored;
                ~RestoreProfile()
                {
                    const errno_t crt_result = _wputenv_s(L"LOCALAPPDATA", crt ? crt->c_str() : L"");
                    const BOOL windows_result = SetEnvironmentVariableW(L"LOCALAPPDATA", windows ? windows->c_str() : nullptr);
                    restored = crt_result == 0 && windows_result != FALSE;
                }
            } restore{ old_windows, old_crt, environment_restored };
            require(_wputenv_s(L"LOCALAPPDATA", profile.c_str()) == 0 &&
                SetEnvironmentVariableW(L"LOCALAPPDATA", profile.c_str()) != FALSE,
                "Could not select the pending probe private profile in both environment views");
            require(pending_windows_profile() == profile.wstring() && pending_crt_profile() == profile.wstring() &&
                user_stage_folder() == profile / "ColourWars" / "stages",
                "Pending probe stage destinations escaped its private profile");
            const auto stage = scratch / "pending-stage.json";
            LevelDocument seed; seed.save(stage);
            const std::string source = pending_file_bytes(stage);
            Editor editor(GetModuleHandleW(nullptr));
            editor.create_window(SW_HIDE, stage);
            const Entry chosen{ EntryKind::spawn_a, 0, "Team A spawn 1" };
            const Entry other{ EntryKind::spawn_b, 0, "Team B spawn 1" };
            std::ofstream evidence(scratch / "pending-properties.csv", std::ios::trunc);
            std::ofstream dialogs(scratch / "pending-dialogs.csv", std::ios::trunc);
            require(evidence.good() && dialogs.good(), "Could not create pending property receipts");
            evidence << "case,driver,expired,timer_owned,source,document,history,selection,property,pending,view,keys,focus_capture,shell,profile,no_save,focus_before,focus_after,edit_window,owner_window,capture_before,capture_after,active_before,active_after,focus_field,capture_none,gestures_idle,focus_owned,focus_expected\n";
            dialogs << "case,step,dialog,answer,owned,title,body,button,hidden,attempted,posted,expired,gone,acknowledgement\n";
            PendingDialogs driver(editor.window_);
            enum class Action { apply, save, save_as, my_stages, select, navigate, close_cancel, close_yes, expiry };
            const char* names[] = { "apply", "save", "save-as", "my-stages", "selection", "navigation", "close-cancel", "close-yes-error", "expiry-cancel" };
            for (Action action : { Action::apply, Action::save, Action::save_as, Action::my_stages, Action::select,
                Action::navigate, Action::close_cancel, Action::close_yes, Action::expiry })
            {
                editor.document_.load(stage); editor.selection_ = chosen;
                editor.document_.set_property(chosen, "x", "125");
                editor.document_.set_property(chosen, "x", "150");
                require(editor.document_.undo(), "Could not prepare pending property redo history");
                editor.refresh();
                int row = -1, other_row = -1;
                for (std::size_t index = 0; index < editor.properties_.size(); ++index)
                    if (editor.properties_[index].path == "x") row = static_cast<int>(index);
                for (std::size_t index = 0; index < editor.entries_.size(); ++index)
                    if (same_entry(editor.entries_[index], other)) other_row = static_cast<int>(index);
                require(row >= 0 && other_row >= 0, "Pending property fixture lost its real numeric row or destination object");
                ListView_SetItemState(editor.property_list_, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
                ListView_SetItemState(editor.property_list_, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                editor.select_property(row);
                editor.zoom_ = 0.65; editor.view_x_ = 10.25; editor.view_y_ = 100.5;
                SetWindowTextW(editor.value_edit_, L"true");
                SendMessageW(editor.value_edit_, EM_SETSEL, 1, 3);
                SetFocus(editor.value_edit_);
                require(editor.pending() && editor.document_.dirty() && editor.document_.can_undo() && editor.document_.can_redo(),
                    "Pending property fixture lacks invalid numeric text and complete history");
                const std::string document = editor.document_.serialize();
                const auto history = native_history(editor.document_);
                const auto selection = editor.selection_;
                const LRESULT object_row = SendMessageW(editor.object_list_, LB_GETCURSEL, 0, 0);
                const std::wstring status = window_text(editor.status_), title = window_text(editor.window_);
                const UINT dpi = editor.dpi_; const HFONT font = editor.font_;
                RECT rectangle{}; require(GetWindowRect(editor.window_, &rectangle), "Could not retain the pending caller rectangle");
                std::array<BYTE, 256> keys{};
                require(GetKeyboardState(keys.data()) != FALSE, "Could not retain pending caller thread keys");
                const char* name = names[static_cast<std::size_t>(action)];
                const HWND focus_before = GetFocus(), capture_before = GetCapture(), active_before = GetActiveWindow();
                std::cout << "BEGIN pending property " << name << std::endl;
                const PendingDialogStep error{ L"ColourWars Level Editor", L"This property needs a finite number.", IDOK, true };
                const bool close = action == Action::close_cancel || action == Action::close_yes || action == Action::expiry;
                const PendingDialogStep prompt{ L"Unsaved level", L"Save your changes before continuing?",
                    action == Action::close_cancel ? IDCANCEL : IDYES };
                bool driver_ok = false, expired = false, timer_owned = false;
                const auto drive = [&]
                {
                    switch (action)
                    {
                    case Action::apply:
                        SendMessageW(editor.window_, WM_COMMAND, MAKEWPARAM(apply_property, BN_CLICKED), reinterpret_cast<LPARAM>(editor.apply_)); break;
                    case Action::save: SendMessageW(editor.window_, WM_COMMAND, file_save, 0); break;
                    case Action::save_as: SendMessageW(editor.window_, WM_COMMAND, file_save_as, 0); break;
                    case Action::my_stages: SendMessageW(editor.window_, WM_COMMAND, file_save_user_stage, 0); break;
                    case Action::select:
                        SendMessageW(editor.object_list_, LB_SETCURSEL, static_cast<WPARAM>(other_row), 0);
                        SendMessageW(editor.window_, WM_COMMAND, MAKEWPARAM(objects_list, LBN_SELCHANGE), reinterpret_cast<LPARAM>(editor.object_list_)); break;
                    case Action::navigate: SendMessageW(editor.canvas_, WM_KEYDOWN, VK_HOME, 0); break;
                    default: SendMessageW(editor.window_, WM_CLOSE, 0, 0); break;
                    }
                    driver_ok = driver.finish(); expired = driver.expired(); timer_owned = driver.running();
                    for (std::size_t index = 0; index < driver.count(); ++index)
                    {
                        const auto& receipt = driver.receipts()[index];
                        dialogs << name << ',' << index << ',' << reinterpret_cast<UINT_PTR>(receipt.dialog) << ',' << receipt.answer
                            << ',' << receipt.owned << ',' << receipt.title << ',' << receipt.body << ',' << receipt.button << ','
                            << receipt.hidden << ',' << receipt.attempted << ',' << receipt.posted << ',' << receipt.expired << ',' << receipt.gone << ','
                            << receipt.acknowledgement << '\n';
                    }
                    dialogs.flush();
                };
                if (action == Action::close_yes) driver.begin({ prompt, error });
                else if (close) driver.begin({ prompt }, action == Action::expiry);
                else driver.begin({ error });
                drive();
                const HWND focus_after = GetFocus(), capture_after = GetCapture(), active_after = GetActiveWindow();
                DWORD start = 0, end = 0;
                SendMessageW(editor.value_edit_, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
                std::array<BYTE, 256> after_keys{}; const BOOL have_keys = GetKeyboardState(after_keys.data());
                RECT after_rectangle{}; const BOOL have_rectangle = GetWindowRect(editor.window_, &after_rectangle);
                const bool unchanged_source = pending_file_bytes(stage) == source;
                const bool unchanged_document = editor.document_.serialize() == document && editor.document_.path() == stage && editor.document_.dirty();
                const bool unchanged_history = editor.document_.can_undo() && editor.document_.can_redo() && native_history(editor.document_) == history;
                const bool unchanged_selection = editor.selection_ && selection && same_entry(*editor.selection_, *selection) &&
                    SendMessageW(editor.object_list_, LB_GETCURSEL, 0, 0) == object_row;
                const bool unchanged_property = editor.property_index_ == row && ListView_GetNextItem(editor.property_list_, -1, LVNI_SELECTED) == row &&
                    window_text(editor.status_) == status && window_text(editor.window_) == title;
                const bool unchanged_pending = editor.pending() && window_text(editor.value_edit_) == L"true" && start == 1 && end == 3;
                const bool unchanged_view = editor.zoom_ == 0.65 && editor.view_x_ == 10.25 && editor.view_y_ == 100.5;
                const bool unchanged_keys = have_keys != FALSE && after_keys == keys;
                const bool focus_field = focus_after == editor.value_edit_;
                const bool focus_owned = focus_after == editor.window_ ||
                    (focus_after && IsChild(editor.window_, focus_after));
                // Validation errors explicitly refocus the value field. Cancel
                // returns to the hidden editor's window or one of its controls.
                const bool focus_expected = action == Action::close_cancel || action == Action::expiry ? focus_owned : focus_field;
                const bool focus_capture = focus_expected && capture_after == nullptr &&
                    !editor.dragging_ && !editor.resizing_ && !editor.panning_;
                const bool shell = IsWindow(editor.window_) && IsWindowEnabled(editor.window_) && !IsWindowVisible(editor.window_) &&
                    have_rectangle && same_rectangle(rectangle, after_rectangle) && editor.dpi_ == dpi && editor.font_ == font;
                const bool same_profile = pending_windows_profile() == profile.wstring() && pending_crt_profile() == profile.wstring();
                const bool no_save = std::filesystem::is_empty(profile);
                evidence << name << ',' << driver_ok << ',' << expired << ',' << timer_owned << ',' << unchanged_source << ','
                    << unchanged_document << ',' << unchanged_history << ',' << unchanged_selection << ',' << unchanged_property << ','
                    << unchanged_pending << ',' << unchanged_view << ',' << unchanged_keys << ',' << focus_capture << ',' << shell << ','
                    << same_profile << ',' << no_save << ',' << reinterpret_cast<UINT_PTR>(focus_before) << ',' << reinterpret_cast<UINT_PTR>(focus_after)
                    << ',' << reinterpret_cast<UINT_PTR>(editor.value_edit_) << ',' << reinterpret_cast<UINT_PTR>(editor.window_)
                    << ',' << reinterpret_cast<UINT_PTR>(capture_before) << ',' << reinterpret_cast<UINT_PTR>(capture_after)
                    << ',' << reinterpret_cast<UINT_PTR>(active_before) << ',' << reinterpret_cast<UINT_PTR>(active_after)
                    << ',' << (focus_after == editor.value_edit_) << ',' << (capture_after == nullptr)
                    << ',' << (!editor.dragging_ && !editor.resizing_ && !editor.panning_) << ',' << focus_owned << ',' << focus_expected << '\n';
                evidence.flush();
                require(evidence.good() && dialogs.good(), "Could not retain pending property action receipts");
                require(driver_ok && timer_owned && expired == (action == Action::expiry), "Pending property modal driver failed its exact bounded action contract");
                require(unchanged_source && unchanged_document && unchanged_history && unchanged_selection && unchanged_property &&
                    unchanged_pending && unchanged_view && unchanged_keys && focus_capture && shell && same_profile && no_save,
                    "Rejected pending property action changed its complete private caller state");
                ++cases;
            }
            const bool driver_cleanup = driver.finish_fixture();
            std::ofstream cleanup(scratch / "pending-driver-cleanup.csv", std::ios::trunc);
            cleanup << "cases,timer_killed,no_owned_dialogs\n" << cases << ',' << driver.killed() << ',' << driver_cleanup << '\n';
            cleanup.flush();
            require(cleanup.good() && driver_cleanup, "Pending property driver did not release its timer and owned dialogs");
        }
        const bool windows_restored = pending_windows_profile() == old_windows;
        const bool crt_restored = pending_crt_profile() == old_crt;
        std::ofstream environment(scratch / "pending-environment.csv", std::ios::trunc);
        environment << "restore_operations_ok,windows_restored,crt_restored\n" << environment_restored << ','
            << windows_restored << ',' << crt_restored << '\n';
        environment.flush();
        require(environment.good() && environment_restored && windows_restored && crt_restored,
            "Pending property probe did not restore both caller profile environment views");
        std::cout << "PASS native pending properties: " << cases << " owned Apply/Save/SaveAs/MyStages/selection/navigation/"
            "Close cancellation+Yes refusal/controlled expiry contexts; exact private state and caller environments, no save or destination creation\n";
    }
}
