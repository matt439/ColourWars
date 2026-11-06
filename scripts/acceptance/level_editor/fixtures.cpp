// The shared helpers declared in fixtures.h.
#include "scripts/acceptance/level_editor/fixtures.h"
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <cmath>
#include <stdexcept>

namespace level_editor_acceptance
{
    rapidjson::Document parse(const std::string& text)
    {
        rapidjson::Document result;
        result.Parse(text.data(), text.size());
        require(!result.HasParseError() && result.IsObject(), "Output is not a JSON object");
        return result;
    }

    std::string encode(const rapidjson::Document& document)
    {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        require(document.Accept(writer), "Could not encode format-version fixture");
        return std::string(buffer.GetString(), buffer.GetSize());
    }

    bool equal(const Rect& actual, const Rect& expected)
    {
        constexpr double tolerance = 0.000001;
        return std::abs(actual.x - expected.x) < tolerance &&
            std::abs(actual.y - expected.y) < tolerance &&
            std::abs(actual.width - expected.width) < tolerance &&
            std::abs(actual.height - expected.height) < tolerance;
    }

    Entry entry(const LevelDocument& document, EntryKind kind, std::size_t index)
    {
        for (const Entry& candidate : document.entries())
            if (candidate.kind == kind && candidate.index == index) return candidate;
        throw std::runtime_error("Expected entry is absent");
    }

    Entry named(const LevelDocument& document, EntryKind kind, const std::string& name)
    {
        for (const Entry& candidate : document.entries())
            if (candidate.kind == kind && candidate.label == name) return candidate;
        throw std::runtime_error("Expected entry '" + name + "' is absent");
    }

    const Property* property(const std::vector<Property>& properties, const std::string& path)
    {
        for (const Property& candidate : properties)
            if (candidate.path == path) return &candidate;
        return nullptr;
    }

    void refuses(const std::function<void()>& action, const std::string& expected, const char* message)
    {
        bool rejected = false;
        try { action(); }
        catch (const std::exception& error)
        {
            rejected = true;
            require(std::string(error.what()).find(expected) != std::string::npos,
                std::string(message) + ": diagnostic was '" + error.what() + "'");
        }
        require(rejected, message);
    }

    void refuses_file(LevelDocument& document, const fs::path& path, const std::string& expected,
        const char* message)
    {
        const std::string before = document.serialize();
        refuses([&] { document.load(path); }, expected, message);
        require(document.serialize() == before, std::string(message) + ": the refused load changed the open document");
    }

    void erase_template_walls(LevelDocument& document)
    {
        const std::vector<Entry> all = document.entries();
        for (auto item = all.rbegin(); item != all.rend(); ++item)
            if (item->kind == EntryKind::collision &&
                (item->label == "left_wall" || item->label == "right_wall" || item->label == "ceiling"))
                document.erase(*item);
    }
}
