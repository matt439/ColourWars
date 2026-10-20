// ENG#12 private exhibit: current public Labrador JSON API, synthetic ASCII only.
// Compile this with engine/assets/json.cpp and engine/core/file_path.cpp.
// No game headers, assets, application, graphics, audio, or engine modifications.
#include "engine/assets/json.h"
#include "engine/core/file_path.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace
{
    void require(bool condition, const char* explanation)
    {
        if (!condition) { throw std::runtime_error(explanation); }
    }

    // This is client-owned plumbing, not an engine JSON entry point. The
    // candidate already exists in memory; the file exists only to reach the
    // same public JsonValue-based reader used for ordinary content.
    class CandidateFileBridge
    {
    public:
        explicit CandidateFileBridge(std::filesystem::path directory) :
            directory_(std::move(directory))
        {
            require(std::filesystem::create_directory(this->directory_),
                "Pass a new scratch directory whose parent already exists.");
        }

        labrador::JsonDocument parse(std::string_view candidate)
        {
            const std::filesystem::path path = this->directory_ /
                ("candidate-" + std::to_string(this->writes_) + ".json");
            require(!std::filesystem::exists(path), "Refusing to overwrite a scratch file.");
            struct RemoveOwnedFile
            {
                std::filesystem::path path;
                std::size_t& removed;
                ~RemoveOwnedFile()
                {
                    std::error_code error;
                    if (std::filesystem::remove(this->path, error)) { ++this->removed; }
                }
            };
            std::ofstream file(path, std::ios::binary);
            require(file.good(), "Cannot open the owned candidate file.");
            RemoveOwnedFile cleanup{ path, this->removals_ };
            file.write(candidate.data(), static_cast<std::streamsize>(candidate.size()));
            file.close();
            require(file.good(), "Cannot finish writing the owned candidate file.");
            ++this->writes_;
            this->last_source_ = labrador::path_to_utf8(path);
            return labrador::read_json_file(this->last_source_.c_str());
        }

        const std::string& last_source() const { return this->last_source_; }
        std::size_t writes() const { return this->writes_; }
        std::size_t removals() const { return this->removals_; }

    private:
        std::filesystem::path directory_;
        std::string last_source_;
        std::size_t writes_ = 0;
        std::size_t removals_ = 0;
    };

    struct Range { float low; float high; };

    // A tiny client-owned semantic reader. The engine owns parsing, checked
    // member/type access, and the context provided by where().
    Range read_range(const labrador::JsonDocument& document)
    {
        const labrador::JsonValue range = document.root().object("range");
        const Range result{ range.number("low"), range.number("high") };
        if (result.low > result.high)
        {
            throw std::runtime_error(range.where() + " low must not exceed high");
        }
        return result;
    }

    // The diagnostic rewrite is also client-owned. A named memory entry
    // point would let the logical label enter the engine at parse time.
    std::string with_logical_source(std::string message, std::string_view physical,
        std::string_view logical)
    {
        const std::size_t at = message.find(physical);
        require(at != std::string::npos, "Engine diagnostic omitted its physical source.");
        message.replace(at, physical.size(), logical);
        return message;
    }

    void observed(const char* name, bool value)
    {
        require(value, name);
        std::cout << name << "=true\n";
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: json_candidate_file_bridge <new-scratch-directory>");
        CandidateFileBridge bridge(labrador::path_from_utf8(argv[1]));
        const std::string logical = "synthetic-settings.json";

        {
            const labrador::JsonDocument document = bridge.parse(
                R"({"range":{"low":1,"high":4}})");
            const Range value = read_range(document);
            require(value.low == 1.0f && value.high == 4.0f, "Valid range changed.");
            std::cout << "valid.range=1..4\n";
            observed("valid.source_is_scratch", document.source_path() == bridge.last_source() &&
                document.source_path() != logical);
            observed("valid.file_removed", !std::filesystem::exists(
                labrador::path_from_utf8(bridge.last_source())));
        }

        std::size_t rewritten = 0;
        const auto rejected = [&](std::string_view candidate, const char* prefix,
            std::string_view context)
        {
            std::string message;
            try
            {
                const labrador::JsonDocument document = bridge.parse(candidate);
                static_cast<void>(read_range(document));
            }
            catch (const std::runtime_error& error) { message = error.what(); }
            require(!message.empty(), "Invalid candidate unexpectedly passed.");
            observed((std::string(prefix) + ".has_scratch_source").c_str(),
                message.find(bridge.last_source()) != std::string::npos);
            observed((std::string(prefix) + ".has_context").c_str(),
                message.find(context) != std::string::npos);
            const std::string shown = with_logical_source(message, bridge.last_source(), logical);
            require(shown.find(logical) != std::string::npos &&
                shown.find(bridge.last_source()) == std::string::npos,
                "Client diagnostic rewrite lost its logical source.");
            ++rewritten;
        };
        rejected(R"({"range":{"low":"bad","high":4}})", "typed_error", "range.low");
        rejected(R"({"range":{"low":9,"high":4}})", "semantic_error", "range low must not exceed high");

        require(bridge.writes() == 3 && bridge.removals() == 3 && rewritten == 2,
            "Unexpected bridge IO or diagnostic-rewrite count.");
        std::cout << "client.scratch_writes=3\nclient.scratch_removals=3\n"
            "client.logical_source_rewrites=2\nPASS json-candidate-file-bridge\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL json-candidate-file-bridge: " << error.what() << '\n';
        return 1;
    }
}
