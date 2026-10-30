// Explicit authoring/registration tool. Never linked into a consumer case.
#include "editor/level_document.h"
#include "editor/stage_registration.h"
#include "scripts/acceptance/support/probe.h"
#include <rapidjson/document.h>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    namespace fs = std::filesystem;
    using namespace colourwars_editor;
    constexpr const char* stage_name = "turbulence_slab_trial";

    using acceptance::require;

    std::string read(const fs::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        require(file.good(), "Cannot read registration input");
        std::string bytes{ std::istreambuf_iterator<char>(file), {} };
        require(!file.bad(), "Cannot finish registration input");
        return bytes;
    }

    void write_new(const fs::path& path, const std::string& bytes)
    {
        require(!fs::exists(path), "Refusing an earlier registration receipt; inspect the previous outcome first");
        fs::create_directories(path.parent_path());
        std::ofstream file(path, std::ios::binary); file << bytes; file.close();
        require(file.good(), "Cannot finish registration receipt");
    }

    rapidjson::Document parse(const std::string& bytes)
    {
        rapidjson::Document value; value.Parse(bytes.c_str());
        require(!value.HasParseError() && value.IsObject(), "Invalid registration fixture JSON");
        return value;
    }

    std::vector<std::string> old_names(const std::string& manifest)
    {
        const auto value = parse(manifest);
        std::vector<std::string> names;
        for (const auto& group : value["assets"].GetArray())
        {
            if (std::string(group["kind"].GetString()) != "level") { continue; }
            for (const auto& name : group["names"].GetArray()) { names.emplace_back(name.GetString()); }
        }
        require(names.size() == 8, "Registration authoring requires the current eight-stage source baseline");
        return names;
    }

    void check_originals(const fs::path& repo, const fs::path& receipt)
    {
        const auto content = repo / "game/content";
        for (const auto& name : old_names(read(receipt / "manifest-before.json")))
        {
            require(read(content / "levels" / (name + ".json")) == read(receipt / "before/levels" / (name + ".json")),
                "An existing source arena changed during slab authoring/registration");
        }
        for (const char* name : { "tuning.json", "presentation.json" })
        { require(read(content / name) == read(receipt / "before" / name), "Source tuning/presentation changed during registration"); }
    }

    void check_geometry(const fs::path& repo, const std::string& private_bytes, const std::string& authored_bytes)
    {
        const auto source = parse(private_bytes), authored = parse(authored_bytes);
        const auto baseline = parse(read(repo / "game/content/levels/turbulence_mover_trial.json"));
        require(source["collision_objects"].Size() == 24 && authored["collision_objects"].Size() == 24 &&
            baseline["collision_objects"].Size() == 22, "Slab registration requires qualified24 objects above unchanged22");
        require(source["collision_objects"] == authored["collision_objects"], "Actual editor authoring changed qualified slab geometry");
        for (rapidjson::SizeType index = 0; index < 22; ++index)
        { require(authored["collision_objects"][index] == baseline["collision_objects"][index], "Slab registration changed an original object"); }
        for (auto field = source.MemberBegin(); field != source.MemberEnd(); ++field)
        {
            const std::string name = field->name.GetString();
            if (name == "display_name" || name == "include_in_random") { continue; }
            require(authored.HasMember(field->name) && authored[field->name] == field->value,
                "Slab registration changed non-display source metadata or spawns");
        }
        require(std::string(authored["display_name"].GetString()) == "Turbulence Yard - Slab Trial" &&
            authored.HasMember("include_in_random") && authored["include_in_random"].IsBool() &&
            !authored["include_in_random"].GetBool(), "Slab trial must have its reviewed explicit-only display identity");
    }

    void author(const fs::path& repo, const fs::path& source, const fs::path& receipt)
    {
        const auto content = repo / "game/content";
        const auto target = content / "levels" / (std::string(stage_name) + ".json");
        require(!fs::exists(receipt) && !fs::exists(target), "Authoring outcome already exists; inspect receipts and use explicit apply/reconciliation");
        const auto manifest = read(content / "manifest.json");
        const auto runtime = read(repo / "cmake/runtime_assets.cmake");
        const auto names = old_names(manifest);
        const auto private_bytes = read(source);
        fs::create_directories(receipt);
        write_new(receipt / "manifest-before.json", manifest);
        write_new(receipt / "runtime-before.cmake", runtime);
        write_new(receipt / "qualified-private-original.json", private_bytes);
        for (const auto& name : names)
        { write_new(receipt / "before/levels" / (name + ".json"), read(content / "levels" / (name + ".json"))); }
        for (const char* name : { "tuning.json", "presentation.json" })
        { write_new(receipt / "before" / name, read(content / name)); }
        LevelDocument document(content / "tuning.json"); document.load(source);
        document.set_property({ EntryKind::level, 0, "" }, "display_name", "Turbulence Yard - Slab Trial");
        document.set_property({ EntryKind::level, 0, "" }, "include_in_random", "false");
        require(document.validate().empty(), "Actual editor rejected the qualified slab trial");
        document.save(target);
        LevelDocument reopened(content / "tuning.json"); reopened.load(target);
        require(!document.dirty() && reopened.serialize() == document.serialize(), "Actual source slab save/reopen changed the document");
        const auto created = read(target);
        check_geometry(repo, private_bytes, created);
        check_originals(repo, receipt);
        require(read(source) == private_bytes && read(content / "manifest.json") == manifest &&
            read(repo / "cmake/runtime_assets.cmake") == runtime, "Editor authoring changed source input or registration lists");
        write_new(receipt / "stage-created.json", created);
        const auto preview = preview_stage_registration(repo, target);
        require(!preview.already_registered && preview.name == stage_name, "Slab stage preview was not a fresh explicit registration");
        write_new(receipt / "manifest-proposed.json", preview.manifest.proposed);
        write_new(receipt / "runtime-proposed.cmake", preview.runtime_assets.proposed);
        write_new(receipt / "author-result.json", "{\"outcome\":\"authored-and-previewed\",\"registered\":false,\"actual_editor_save_reopen\":true,\"preserved_source_arenas\":8,\"objects\":24,\"original_objects\":22}\n");
        std::cout << "PASS actual editor slab author/save/reopen and registration preview; no list mutation. Review receipt manifest/runtime proposals before --apply.\n";
    }

    void apply(const fs::path& repo, const fs::path& source, const fs::path& receipt)
    {
        const auto target = repo / "game/content/levels" / (std::string(stage_name) + ".json");
        const auto private_bytes = read(receipt / "qualified-private-original.json");
        require(read(source) == private_bytes && read(target) == read(receipt / "stage-created.json"),
            "Slab source/candidate changed after authoring; do not apply stale proposals");
        check_geometry(repo, private_bytes, read(target)); check_originals(repo, receipt);
        const auto preview = preview_stage_registration(repo, target);
        const auto proposed_manifest = read(receipt / "manifest-proposed.json");
        const auto proposed_runtime = read(receipt / "runtime-proposed.cmake");
        if (preview.already_registered)
        {
            require(preview.manifest.original == proposed_manifest && preview.runtime_assets.original == proposed_runtime,
                "An existing registration differs from the exact task-owned proposal");
            std::cout << "PASS reconciled registration already matches both exact proposals; no mutation or repeated apply.\n";
            return;
        }
        require(!fs::exists(receipt / "apply-started.json"), "Uncertain earlier apply requires receipt/outcome inspection before retry");
        require(preview.manifest.original == read(receipt / "manifest-before.json") &&
            preview.runtime_assets.original == read(receipt / "runtime-before.cmake") &&
            preview.manifest.proposed == proposed_manifest && preview.runtime_assets.proposed == proposed_runtime,
            "Registration lists/proposals changed after review; refusing stale two-file mutation");
        write_new(receipt / "apply-started.json", "{\"operation\":\"apply_stage_registration\",\"state\":\"started\"}\n");
        require(apply_stage_registration(preview), "Fresh slab registration unexpectedly did not apply");
        const auto after = preview_stage_registration(repo, target);
        require(after.already_registered && after.manifest.original == proposed_manifest &&
            after.runtime_assets.original == proposed_runtime && read(target) == preview.stage_original,
            "Slab registration did not produce both exact reviewed lists");
        check_originals(repo, receipt);
        write_new(receipt / "apply-result.json", "{\"outcome\":\"registered\",\"stage\":\"turbulence_slab_trial\",\"random\":false,\"actual_registration_api\":true,\"preserved_source_arenas\":8,\"objects\":24,\"original_objects\":22}\n");
        std::cout << "PASS actual two-file slab registration API; stage explicit-only and source8/tuning preserved.\n";
    }
}

int wmain(int argc, wchar_t** argv)
{
    return acceptance::run_probe("explicit slab registration", [&]
    {
        require(argc == 5, "Usage: slab-register (--author-preview|--apply) <absolute-repo> <absolute-qualified-private24> <absolute-new-receipt>");
        const std::wstring_view mode(argv[1]);
        require(mode == L"--author-preview" || mode == L"--apply", "Explicit author-preview or --apply mode is required");
        const fs::path repo(argv[2]), source(argv[3]), receipt(argv[4]);
        require(repo.is_absolute() && source.is_absolute() && receipt.is_absolute(), "Registration helper accepts absolute paths only");
        if (mode == L"--author-preview") { author(repo, source, receipt); }
        else { apply(repo, source, receipt); }
        return 0;
    });
}
