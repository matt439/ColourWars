#pragma once

// Private to the colourwars_level_document library: the level-file JSON vocabulary
// and the rules for what a valid level is. Not included by main.cpp or the acceptance case.

#include "editor/level_document.h"

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

        inline constexpr std::array<const char*, 4> bounds_names = {
            "out_of_bounds", "camera_bounds", "zoom_out_start_bounds", "zoom_out_finish_bounds" };

        [[noreturn]] void fail(const std::string& text);
        const Json& member(const Json& object, const char* name);
        Json& member(Json& object, const char* name);
        double number(const Json& object, const char* name);
        std::string string(const Json& object, const char* name);
        const Json& array(const Json& object, const char* name);
        void check_rectangle(const Rect& rectangle);
        Rect plain_rectangle(const Json& value);
        Rect object_rectangle(const Json& value);
        // The materials a collision tag may carry, comma-separated, empty for none.
        std::string allowed_materials(std::string_view collision_type);
        void validate_document(const Document& document);
        Document parse(std::string_view text);
        std::string json_text(const Json& value);
    }
}
