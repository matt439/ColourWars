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

    // A station is one of the level's named "stations" (goto targets): its
    // name, x and y are edited here; its lesson block is preserved as written.
    enum class EntryKind { level, bounds, spawn_a, spawn_b, station, collision, decoration };

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
        // Align the shape origin (the stored endpoint for fixed slabs) in one
        // validated edit. An already aligned origin preserves undo/redo state.
        void align_origin(const Entry& entry, int grid);
        std::vector<Property> properties(const Entry& entry) const;
        void set_property(const Entry& entry, const std::string& path,
            const std::string& value);

        // Palette: Structure, StructureJumpThrough, StructurePaintable,
        // MovingPaintable, MovingJumpThrough, StructureRampRight,
        // StructureRampLeft, FixedSlabRight, FixedSlabLeft, TeamGate, Visual, SpawnA,
        // SpawnB, Station. A TeamGate is a solid eraser StructurePaintable with
        // "gate": "paint" and its bottom face unpainted, as it stands on the
        // floor. A Station is {name, x, y} at the rectangle's top-left, named
        // station_<n> with the first unused n; it never gains a lesson.
        Entry add_object(const std::string& type, const Rect& rectangle);
        // A station copy is renamed <name>_copy (then _copy_2, ...) and drops
        // its lesson, which belongs to the original's words and area.
        Entry duplicate(const Entry& entry);
        // One validated copy reflected across the out_of_bounds centre: x
        // becomes left + right - x - width, ramps and fixed slabs change hand,
        // left/right paint faces swap, cycle movers reflect their whole path
        // with phase + 0.5 (drifting visuals likewise), a team spawn becomes
        // the other team's, and a trailing _a/_b name swaps. Paint ferries,
        // stations, level settings and bounds are refused.
        Entry mirror_copy(const Entry& entry);
        // Mirror partners under the mirror_copy rule, ignoring names: an
        // object partners one other object in its list (or itself, when it is
        // its own reflection) and a spawn one of the other team's. Paint
        // ferries have no mirror rule for heading or phase, so they are paired
        // by path alone and listed apart as authored, not as unmatched.
        // Unpartnered decorations (art, no collision or paint) are listed
        // apart from unmatched spawns and collision objects.
        struct SymmetryReport
        {
            std::vector<Entry> unmatched;
            std::vector<Entry> unmatched_art;
            std::vector<Entry> paint_ferries;
        };
        SymmetryReport symmetry() const;
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
