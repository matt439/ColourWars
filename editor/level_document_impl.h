#pragma once

// Private to the colourwars_level_document library; included only by the level_document TUs.

#include "editor/motion_authoring.h"
#include "editor/level_document.h"
#include "editor/level_json.h"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace colourwars_editor
{
    using namespace level_json;

    namespace level_document_detail
    {
        Json& entry_json(Document& document, const Entry& entry);
        Json& entry_array(Document& document, EntryKind kind);
        void set_number(Json& object, const char* name, double value);
        void move_shape(Json& object, const Rect& rectangle);
        // The added entry as entries() lists it, label included.
        Entry labelled(const std::vector<Entry>& entries, const Entry& added);
    }

    using namespace level_document_detail;

    struct LevelDocument::Impl
    {
        Document document;
        std::filesystem::path path;
        std::filesystem::path motion_tuning;
        std::vector<std::string> history;
        std::size_t history_index = 0;
        std::string saved;
        std::filesystem::path source_path;
        std::string source_bytes;

        static constexpr MotionPlayerSize shipped_player_size{ 52.0f, 120.0f };
        mutable std::optional<std::filesystem::file_time_type> marker_stamp;
        mutable MotionPlayerSize marker_cache = shipped_player_size;

        // A spawn or station is the character's top-left: source tuning's
        // player size, or the shipped 52 by 120 for a document without tuning.
        // An edit that needs it (mirroring a spawn) throws on unreadable tuning.
        MotionPlayerSize player_size() const
        {
            return motion_tuning.empty() ? shipped_player_size : load_motion_player_size(motion_tuning);
        }

        // The box rectangle() reports for a spawn or station. It is a display
        // read, so it never throws: unreadable or malformed tuning gives the
        // shipped size, and tuning is parsed again only when the file changes.
        MotionPlayerSize marker_size() const
        {
            if (motion_tuning.empty()) { return shipped_player_size; }
            std::error_code error;
            const std::filesystem::file_time_type stamp = std::filesystem::last_write_time(motion_tuning, error);
            if (error) { return shipped_player_size; }
            if (!marker_stamp || *marker_stamp != stamp)
            {
                try { marker_cache = load_motion_player_size(motion_tuning); }
                catch (const std::exception&) { marker_cache = shipped_player_size; }
                marker_stamp = stamp;
            }
            return marker_cache;
        }

        template<typename Operation> void mutate(Operation operation)
        {
            Document candidate;
            candidate.CopyFrom(document, candidate.GetAllocator());
            operation(candidate);
            validate_document(candidate, motion_tuning);
            if (candidate == document) { return; }
            std::string snapshot = json_text(candidate);
            if (snapshot == history[history_index]) { return; }
            // Allocate before changing anything (HYG#97): once the capacity is
            // reserved, erasing the redo tail, moving the snapshot in and
            // swapping the live document cannot throw, so a failed edit still
            // leaves history and document as they were. Growth stays
            // geometric, and no retained snapshot is copied.
            const std::size_t needed = history_index + 2;
            if (history.capacity() < needed)
            {
                history.reserve((std::max)(needed, history.capacity() * 2));
            }
            history.erase(history.begin() + static_cast<std::ptrdiff_t>(history_index + 1),
                history.end());
            history.push_back(std::move(snapshot));
            history_index = history.size() - 1;
            document.Swap(candidate);
        }
    };
}
