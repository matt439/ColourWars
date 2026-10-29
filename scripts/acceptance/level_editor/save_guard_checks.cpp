// Same-file saves must not replace another writer's bytes. Every file here is
// under the acceptance scratch directory; no player or authored stage is used.
#include "scripts/acceptance/level_editor/fixtures.h"
#include <Windows.h>

#include <iostream>
#include <utility>
#include <vector>

namespace level_editor_acceptance
{
    namespace
    {
        struct History
        {
            std::string current;
            fs::path path;
            bool dirty;
            std::vector<std::pair<std::string, bool>> undo;
            std::vector<std::pair<std::string, bool>> redo;

            bool operator==(const History&) const = default;
        };

        History history(LevelDocument& document)
        {
            History result{ document.serialize(), document.path(), document.dirty(), {}, {} };
            while (document.undo()) { result.undo.emplace_back(document.serialize(), document.dirty()); }
            for (std::size_t index = 0; index < result.undo.size(); ++index)
            { require(document.redo(), "Could not restore the original undo position"); }
            while (document.redo()) { result.redo.emplace_back(document.serialize(), document.dirty()); }
            for (std::size_t index = 0; index < result.redo.size(); ++index)
            { require(document.undo(), "Could not restore the original redo position"); }
            require(document.serialize() == result.current && document.path() == result.path &&
                document.dirty() == result.dirty, "History inspection changed the current document");
            return result;
        }

        void no_save_temporary(const fs::path& scratch)
        {
            for (const fs::directory_entry& item : fs::directory_iterator(scratch))
            {
                require(item.path().filename().wstring().find(L".editor-") == std::wstring::npos,
                    "A refused save retained its temporary file");
            }
        }

        void refuse_external_save(LevelDocument& document, const fs::path& path,
            const fs::path& scratch)
        {
            const History before = history(document);
            bool refused = false;
            try { document.save(path); }
            catch (const std::exception& error)
            {
                refused = true;
                require(std::string(error.what()).find("Save As") != std::string::npos,
                    "External-change refusal did not describe the recovery command");
            }
            require(refused, "Same-file save overwrote an external change or unreadable file");
            require(history(document) == before, "External-change refusal changed complete document history");
            no_save_temporary(scratch);
        }

        void edit_with_redo(LevelDocument& document)
        {
            const Entry level = entry(document, EntryKind::level);
            document.set_property(level, "/display_name", "Local first edit");
            document.set_property(level, "/display_name", "Local second edit");
            require(document.undo() && document.can_undo() && document.can_redo() && document.dirty(),
                "Conflict fixture needs live edits with both undo and redo history");
        }

        class ExclusiveReadLock
        {
        public:
            explicit ExclusiveReadLock(const fs::path& path) : file_(CreateFileW(path.c_str(),
                GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr))
            {
                require(file_ != INVALID_HANDLE_VALUE, "Could not make the saved file unreadable");
            }
            ~ExclusiveReadLock() { CloseHandle(file_); }
            ExclusiveReadLock(const ExclusiveReadLock&) = delete;
            ExclusiveReadLock& operator=(const ExclusiveReadLock&) = delete;
        private:
            HANDLE file_;
        };
    }

    void check_external_save_changes(const fs::path& scratch)
    {
        const fs::path source = scratch / "external-save-source.json";
        LevelDocument seed;
        seed.save(source);
        const std::string normalized = contents(source);
        const std::string original = " \n\t" + normalized + "\n ";
        write(source, original);

        LevelDocument local;
        local.load(source);
        require(local.serialize() != original && !local.dirty(),
            "Load fixture did not retain a distinct normalized document");
        local.save(source);
        require(contents(source) == normalized && !local.dirty(),
            "Unchanged raw source was refused or failed to establish normalized save bytes");
        local.save(source);

        LevelDocument other;
        other.load(source);
        edit_with_redo(local);
        other.set_property(entry(other, EntryKind::level), "/display_name", "Other writer's arena");
        other.save(source);
        const std::string other_bytes = contents(source);
        refuse_external_save(local, source.parent_path() / "." / source.filename(), scratch);
        refuse_external_save(local, scratch / "EXTERNAL-SAVE-SOURCE.JSON", scratch);
        const fs::path alias = scratch / "external-save-alias.json";
        fs::create_hard_link(source, alias);
        refuse_external_save(local, alias, scratch);
        require(contents(alias) == other_bytes, "Refused equivalent-file save changed the external alias");
        require(contents(source) == other_bytes, "Refused stale editor save changed the other editor's file");

        const fs::path rescue = scratch / "external-save-rescue.json";
        const History conflicted = history(local);
        local.save(rescue);
        require(local.path() == rescue && !local.dirty() && contents(source) == other_bytes &&
            local.serialize() == conflicted.current, "Save As failed to preserve conflicted open work elsewhere");
        require(local.can_undo() && local.can_redo(), "Save As discarded the open undo/redo history");
        local.set_property(entry(local, EntryKind::level), "/display_name", "Rescued later edit");
        local.save(rescue);
        require(!local.dirty(), "Save As did not establish the new file's byte baseline");

        const fs::path same_length = scratch / "external-save-same-length.json";
        write(same_length, normalized);
        LevelDocument measured;
        measured.load(same_length);
        edit_with_redo(measured);
        const fs::file_time_type timestamp = fs::last_write_time(same_length);
        std::string changed = normalized;
        const std::size_t name = changed.find("New Stage");
        require(name != std::string::npos, "Same-length fixture could not find the source name");
        changed.replace(name, 9, "Off Stage");
        write(same_length, changed);
        fs::last_write_time(same_length, timestamp);
        require(changed.size() == normalized.size() && fs::last_write_time(same_length) == timestamp,
            "External edit did not retain both byte length and timestamp");
        refuse_external_save(measured, same_length, scratch);
        require(contents(same_length) == changed, "Refusal replaced a same-length external edit");
        // A failed attempt must not adopt the rejected writer's bytes as its baseline.
        write(same_length, normalized);
        measured.save(same_length);
        const std::string last_saved = contents(same_length);
        measured.set_property(entry(measured, EntryKind::level), "/display_name", "A newer local edit");
        write(same_length, normalized);
        refuse_external_save(measured, same_length, scratch);
        require(contents(same_length) == normalized, "A later conflict changed external bytes");
        write(same_length, last_saved);
        measured.save(same_length);
        require(!measured.dirty(), "Retry did not use the most recent successful save baseline");

        const fs::path deleted = scratch / "external-save-deleted.json";
        LevelDocument missing;
        missing.save(deleted);
        const std::string missing_baseline = contents(deleted);
        edit_with_redo(missing);
        require(fs::remove(deleted), "Could not remove the isolated saved stage");
        refuse_external_save(missing, deleted, scratch);
        refuse_external_save(missing, scratch / "EXTERNAL-SAVE-DELETED.JSON", scratch);
        require(!fs::exists(deleted), "Refused save silently recreated a removed external file");
        write(deleted, missing_baseline);
        missing.save(deleted);
        require(!missing.dirty(), "Restoring the original bytes did not permit a retry");

        const fs::path unreadable = scratch / "external-save-unreadable.json";
        LevelDocument locked;
        locked.save(unreadable);
        const std::string readable_bytes = contents(unreadable);
        edit_with_redo(locked);
        {
            const ExclusiveReadLock lock(unreadable);
            refuse_external_save(locked, unreadable, scratch);
        }
        require(contents(unreadable) == readable_bytes, "Unreadable-file refusal changed saved bytes");
        locked.save(unreadable);
        require(!locked.dirty(), "Read failure damaged the baseline used after unlocking");

        const fs::path unavailable = scratch / "external-save-no-parent" / "unwritten.json";
        locked.set_property(entry(locked, EntryKind::level), "/display_name", "After failed Save As");
        reject_without_change(locked, [&] { locked.save(unavailable); },
            "Save As to a missing parent unexpectedly succeeded");
        locked.save(unreadable);
        require(!locked.dirty() && !fs::exists(unavailable),
            "Failed Save As replaced the previous successful baseline");
        no_save_temporary(scratch);
        std::cout << "PASS external-save byte baselines, competing editors, metadata-preserving edit, "
            "deletion/read refusal, complete history, Save As and latest-success retry\n";
    }
}
