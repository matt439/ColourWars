#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "game/paint_polygon_geometry.h"

namespace colourwars_editor
{
    struct ContentCatalog;

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

    struct MotionGuide
    {
        Rect anchor;
        Rect initial_pose;
        Rect travel_envelope;
        Rect standing_envelope;
    };

    // The document owns source JSON, including fields the editor does not understand.
    // Failed loads and edits leave its JSON, history, save path and dirty state intact.
    class LevelDocument
    {
    public:
        LevelDocument();
        // Movers require source player.size for their complete standing corridor.
        // Static documents do not read this optional path.
        explicit LevelDocument(const std::filesystem::path& motion_tuning);
        ~LevelDocument();
        LevelDocument(const LevelDocument&) = delete;
        LevelDocument& operator=(const LevelDocument&) = delete;

        void load(const std::filesystem::path& path);
        // Saving the loaded/saved file refuses external byte changes, deletion
        // or unreadability, checked again before replacement. Save As to another
        // file preserves the open edits and establishes that file's baseline.
        void save(const std::filesystem::path& path);
        void create();

        std::vector<Entry> entries() const;
        Rect rectangle(const Entry& entry) const;
        std::optional<MotionGuide> motion_guide(const Entry& entry,
            const std::optional<Rect>& preview = std::nullopt) const;
        std::optional<PaintPolygon> slab_guide(const Entry& entry,
            const std::optional<Rect>& preview = std::nullopt) const;
        void set_rectangle(const Entry& entry, const Rect& rectangle);
        std::vector<Property> properties(const Entry& entry) const;
        void set_property(const Entry& entry, const std::string& path,
            const std::string& value);

        // Palette: Structure, StructureJumpThrough, StructurePaintable,
        // MovingPaintable, MovingJumpThrough, StructureRampRight,
        // StructureRampLeft, FixedSlabRight, FixedSlabLeft, TeamGate, Visual, SpawnA,
        // SpawnB. A TeamGate is a solid eraser StructurePaintable with "gate": "paint"
        // and its bottom face unpainted, as it stands on the floor.
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
        // Resource names the game would refuse at startup: one line each, empty when
        // every name resolves in the catalog. Separate from validate(), which is
        // structure only and blocks edits; these only warn.
        std::vector<std::string> unresolved_resources(const ContentCatalog& catalog) const;
        std::string serialize() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
