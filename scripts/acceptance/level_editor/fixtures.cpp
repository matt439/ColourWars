// The shared helpers declared in fixtures.h.
#include "scripts/acceptance/level_editor/fixtures.h"
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <cmath>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace level_editor_acceptance
{
    void require(bool condition, const std::string& message)
    {
        if (!condition) throw std::runtime_error(message);
    }

    std::string contents(const fs::path& path)
    {
        std::ifstream stream(path, std::ios::binary);
        require(stream.good(), "Could not read acceptance input");
        return { std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
    }

    void write(const fs::path& path, const std::string& text)
    {
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        stream << text;
        stream.close();
        require(stream.good(), "Could not write acceptance fixture");
    }

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
}
