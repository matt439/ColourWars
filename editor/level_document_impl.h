#pragma once

// Private to the colourwars_level_document library; included only by the level_document TUs.

#include "editor/level_document.h"
#include "editor/level_json.h"

#include <cstddef>
#include <filesystem>
#include <string>
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

        template<typename Operation> void mutate(Operation operation)
        {
            Document candidate;
            candidate.CopyFrom(document, candidate.GetAllocator());
            operation(candidate);
            validate_document(candidate, motion_tuning);
            if (candidate == document) { return; }
            const std::string snapshot = json_text(candidate);
            if (snapshot == history[history_index]) { return; }
            // Allocate the new history before swapping the live document.
            std::vector<std::string> next(history.begin(),
                history.begin() + static_cast<std::ptrdiff_t>(history_index + 1));
            next.push_back(snapshot);
            history.swap(next);
            history_index = history.size() - 1;
            document.Swap(candidate);
        }
    };
}
