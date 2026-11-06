#pragma once

// For probes that check a content reader refuses bad JSON: read the shipped
// file once, write an edited copy per case to a private scratch folder (the
// one policy in scratch.h), and require that loading the copy throws with the
// expected words in its message.

#include "scripts/acceptance/support/probe.h"
#include "scripts/acceptance/support/scratch.h"

#include <cstddef>
#include <exception>
#include <filesystem>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace acceptance
{
    // The parsed file. Throws when it cannot be read or parsed.
    inline rapidjson::Document read_document(const std::filesystem::path& path)
    {
        const std::string text = read_text(path);
        rapidjson::Document result;
        result.Parse(text.c_str());
        require(!result.HasParseError(), "Cannot parse " + utf8_path(path));
        return result;
    }

    // Replaces the file with the document, compact. Throws when it cannot.
    inline void write_document(const rapidjson::Document& document, const std::filesystem::path& path)
    {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        require(document.Accept(writer), "Cannot encode " + utf8_path(path));
        write_text(path, std::string_view(buffer.GetString(), buffer.GetSize()));
    }

    // Edited copies of one JSON file, each written to its own numbered file,
    // <stem>-<n>.json, in a folder the caller names or, by default, a fresh
    // private one that goes when the variants do.
    class JsonVariants
    {
    public:
        JsonVariants(const std::filesystem::path& source, const wchar_t* stem) :
            source_(read_document(source)), stem_(stem), scratch_(std::in_place, stem)
        {
            this->folder_ = this->scratch_->root();
        }

        JsonVariants(const std::filesystem::path& source, const wchar_t* stem, const std::filesystem::path& folder) :
            source_(read_document(source)), stem_(stem), folder_(folder)
        {
            std::filesystem::create_directories(this->folder_);
        }

        const rapidjson::Document& source() const { return this->source_; }
        const std::filesystem::path& folder() const { return this->folder_; }

        // A fresh copy of the source, edited by `edit(document)`, on disk.
        // Const: the source never changes, only the count of files written.
        template<class Edit>
        std::filesystem::path write(Edit edit) const
        {
            rapidjson::Document document;
            document.CopyFrom(this->source_, document.GetAllocator());
            edit(document);
            const std::filesystem::path path = this->folder_ /
                (this->stem_ + L"-" + std::to_wstring(this->next_++) + L".json");
            write_document(document, path);
            return path;
        }

    private:
        rapidjson::Document source_;
        std::wstring stem_;
        std::optional<ScratchDirectory> scratch_;
        std::filesystem::path folder_;
        mutable std::size_t next_ = 0;
    };

    // Writes the edited copy and requires that `load(path)` throws an
    // `Error` whose message holds every fragment. `message` names the case
    // when it does not; the loader's own message, or "accepted", follows it.
    template<class Error = std::exception, class Edit, class Load>
    void rejects_variant(const JsonVariants& variants, Edit edit, Load load,
        std::initializer_list<std::string_view> fragments, const std::string& message)
    {
        const std::filesystem::path path = variants.write(edit);
        std::string diagnostic = "accepted";
        try
        {
            load(path);
        }
        catch (const Error& error)
        {
            diagnostic = error.what();
            bool all = true;
            for (const std::string_view fragment : fragments)
            {
                all = all && diagnostic.find(fragment) != std::string::npos;
            }
            if (all) { return; }
        }
        require(false, message + ": " + diagnostic);
    }
}
