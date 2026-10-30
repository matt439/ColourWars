#pragma once

// Private to the colourwars_level_document library: the level-file JSON vocabulary
// and the rules for what a valid level is. Not included by main.cpp or the acceptance case.

#include "editor/level_document.h"
#include "game/level_format.h"

#include <rapidjson/document.h>

#include <array>
#include <string>
#include <string_view>

namespace colourwars_editor
{
    namespace level_json
    {
        using Json = rapidjson::Value;
        using Document = rapidjson::Document;
        using Allocator = Document::AllocatorType;

        // The format's words and tables are game/level_format.h's.
        namespace key = level_format::key;
        namespace object_type = level_format::object_type;
        inline constexpr const auto& bounds_names = level_format::bounds_keys;

        [[noreturn]] void fail(const std::string& text);
        const Json& member(const Json& object, const char* name);
        Json& member(Json& object, const char* name);
        double number(const Json& object, const char* name);
        std::string string(const Json& object, const char* name);
        const Json& array(const Json& object, const char* name);
        void check_rectangle(const Rect& rectangle);
        Rect plain_rectangle(const Json& value);
        Rect object_rectangle(const Json& value);
        void validate_document(const Document& document,
            const std::filesystem::path& motion_tuning = {});
        Document parse(std::string_view text);
        std::string json_text(const Json& value);
    }
}
