#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace colourwars_editor
{
    enum class EntryKind { level, bounds, spawn_a, spawn_b, collision, decoration };

    struct Entry
    {
        EntryKind kind = EntryKind::level;
        std::size_t index = 0;
        std::string label;
    };

    struct Rect
    {
        double x = 0.0;
        double y = 0.0;
        double width = 0.0;
        double height = 0.0;
    };

    struct Property
    {
        std::string path;
        std::string label;
        std::string value;
        bool numeric = false;
        bool boolean = false;
    };

    // The document owns source JSON, including fields the editor does not understand.
    // Failed loads and edits leave its JSON, history, save path and dirty state intact.
    class LevelDocument
    {
    public:
        LevelDocument();
        ~LevelDocument();
        LevelDocument(const LevelDocument&) = delete;
        LevelDocument& operator=(const LevelDocument&) = delete;

        void load(const std::filesystem::path& path);
        void save(const std::filesystem::path& path);
        void create();

        std::vector<Entry> entries() const;
        Rect rectangle(const Entry& entry) const;
        void set_rectangle(const Entry& entry, const Rect& rectangle);
        std::vector<Property> properties(const Entry& entry) const;
        void set_property(const Entry& entry, const std::string& path,
            const std::string& value);

        // Palette: Structure, StructureJumpThrough, StructurePaintable,
        // StructureRampRight, StructureRampLeft, Visual, SpawnA, SpawnB.
        Entry add_object(const std::string& type, const Rect& rectangle);
        Entry duplicate(const Entry& entry);
        void erase(const Entry& entry);

        bool undo();
        bool redo();
        bool can_undo() const;
        bool can_redo() const;
        bool dirty() const;
        const std::filesystem::path& path() const;
        std::vector<std::string> validate() const;
        std::string serialize() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
