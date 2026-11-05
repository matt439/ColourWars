#include "editor/level_document_impl.h"

#include "editor/horizontal_authoring.h"
#include "game/contracts/fixed_slab_paint_geometry.h"
#include "game/contracts/level_format.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace colourwars_editor
{
    namespace
    {
        void set_text(Json& object, const char* name, const std::string& text, Allocator& allocator)
        {
            member(object, name).SetString(text.data(), static_cast<rapidjson::SizeType>(text.size()), allocator);
        }

        // A cycle (mover or drift) half a period on is its own reflection.
        // Rounding to twelve places keeps an authored decimal such as 0.51
        // a short decimal (0.01), so the saved text reparses to the same value.
        void advance_half_cycle(Json& fields)
        {
            double phase = number(fields, "phase") + 0.5;
            if (phase >= 1.0) { phase -= 1.0; }
            phase = std::round(phase * 1e12) / 1e12;
            // A phase just under 0.5 can round up to a whole cycle, which is 0.
            if (phase >= 1.0) { phase -= 1.0; }
            set_number(fields, "phase", phase);
        }

        // Reflect one object record across x = sum / 2, where sum is the
        // out_of_bounds left + right. The game reads hand from the ramp's
        // collision type and the slab's rise, so those change with the shape.
        void mirror_object(Json& object, double sum, Allocator& allocator)
        {
            const std::string type = string(object, key::type);
            if (type == object_type::fixed_slab_paintable)
            {
                Json& slab = member(object, key::slab);
                const std::string rise = string(slab, "rise");
                if (rise != "left" && rise != "right") { fail("slab.rise must be left or right."); }
                // The stored endpoint is the left one; the far end becomes it.
                set_number(slab, "x", sum - number(slab, "x") - double{ slab_span });
                set_number(slab, "y", number(slab, "y") + (rise == "right" ? -double{ slab_rise } : double{ slab_rise }));
                set_text(slab, "rise", rise == "right" ? "left" : "right", allocator);
            }
            else if (type == object_type::structure_ramp)
            {
                Json& triangle = member(object, key::triangle);
                for (const char* x : { "x1", "x2", "x3" }) { set_number(triangle, x, sum - number(triangle, x)); }
                const std::string collision = string(object, key::collision_type);
                const std::optional<level_format::Collision> tag = level_format::collision_named(collision);
                if (tag == level_format::Collision::structure_ramp_right)
                {
                    set_text(object, key::collision_type,
                        level_format::collision_name(level_format::Collision::structure_ramp_left), allocator);
                }
                else if (tag == level_format::Collision::structure_ramp_left)
                {
                    set_text(object, key::collision_type,
                        level_format::collision_name(level_format::Collision::structure_ramp_right), allocator);
                }
            }
            else
            {
                Rect rectangle = object_rectangle(object);
                double reach = rectangle.width;
                if (object.HasMember(key::motion))
                {
                    Json& motion = member(object, key::motion);
                    if (motion.IsObject() && motion.HasMember("drive") && motion["drive"].IsString() &&
                        string(motion, "drive") == "paint")
                    {
                        fail("Mirror copy does not copy paint ferries or paint lifts yet: their heading and "
                            "paint-drive phase need their own rule. Duplicate it and set the copy by hand.");
                    }
                    // A horizontal rectangle is the leftmost anchor, so reflect
                    // the whole path and start it half a cycle on. A lift moves
                    // straight up and down, so only its anchor reflects.
                    if (!(motion.IsObject() && motion.HasMember("type") && motion["type"].IsString() &&
                        string(motion, "type") == "vertical"))
                    {
                        reach += number(motion, "travel_x");
                        advance_half_cycle(motion);
                    }
                }
                if (object.HasMember(key::drift) && object[key::drift].IsObject()) { advance_half_cycle(object[key::drift]); }
                rectangle.x = sum - rectangle.x - reach;
                move_shape(object, rectangle);
            }
            if (object.HasMember(key::paintable_faces) && object[key::paintable_faces].IsObject())
            {
                namespace side = level_format::face;
                Json& faces = object[key::paintable_faces];
                const bool left = faces.HasMember(side::left), right = faces.HasMember(side::right);
                if (left && right) { faces[side::left].Swap(faces[side::right]); }
                else if (left || right)
                {
                    Json value; value.CopyFrom(faces[left ? side::left : side::right], allocator);
                    faces.RemoveMember(left ? side::left : side::right);
                    faces.AddMember(Json(left ? side::right : side::left, allocator), value, allocator);
                }
            }
            if (object.HasMember(key::name) && object[key::name].IsString())
            {
                std::string name = string(object, key::name);
                if (name.ends_with("_a")) { name.back() = 'b'; }
                else if (name.ends_with("_b")) { name.back() = 'a'; }
                else { name += " mirror"; }
                set_text(object, key::name, name, allocator);
            }
        }
    }

    Entry LevelDocument::mirror_copy(const Entry& entry)
    {
        if (entry.kind == EntryKind::level || entry.kind == EntryKind::bounds)
        { fail("Select an object or spawn to mirror; level settings and bounds have no mirror copy."); }
        if (entry.kind == EntryKind::station)
        { fail("Stations have no mirror copy: each names one place. Duplicate it and move the copy."); }
        Entry added = entry;
        impl_->mutate([&](Document& document)
        {
            Allocator& allocator = document.GetAllocator();
            const Rect world = plain_rectangle(member(member(document, key::level_bounds), key::out_of_bounds));
            const double sum = world.x + (world.x + world.width);
            Json value;
            value.CopyFrom(entry_json(document, entry), allocator);
            if (entry.kind == EntryKind::spawn_a || entry.kind == EntryKind::spawn_b)
            {
                // A spawn point is the body's top-left, so reflect the whole
                // character from source tuning (the editor's 52-unit box without it).
                const double width = impl_->motion_tuning.empty() ? this->rectangle(entry).width :
                    static_cast<double>(load_motion_player_size(impl_->motion_tuning).width);
                set_number(value, "x", sum - number(value, "x") - width);
                added.kind = entry.kind == EntryKind::spawn_a ? EntryKind::spawn_b : EntryKind::spawn_a;
            }
            else { mirror_object(value, sum, allocator); }
            Json& objects = entry_array(document, added.kind);
            added.index = objects.Size();
            objects.PushBack(value, allocator);
        });
        for (const Entry& candidate : entries())
        {
            if (candidate.kind == added.kind && candidate.index == added.index) { return candidate; }
        }
        return added;
    }

    namespace
    {
        bool paint_ferry(const Json& object)
        {
            if (!object.IsObject() || !object.HasMember(key::motion) || !object[key::motion].IsObject()) { return false; }
            const Json& motion = object[key::motion];
            return motion.HasMember("drive") && motion["drive"].IsString() &&
                std::string_view(motion["drive"].GetString()) == "paint";
        }

        // Names are labels, not shape: a centred object reflects onto itself
        // under another name. A ferry's heading and phase have no mirror rule.
        void strip_unmirrored(Json& object, bool ferry)
        {
            if (object.IsObject()) { object.RemoveMember(key::name); }
            if (ferry)
            {
                Json& motion = object[key::motion];
                for (const char* field : { "drive", "team_a_heads", "phase" }) { motion.RemoveMember(field); }
            }
        }

        // Equal shapes up to the rounding the reflection itself introduces.
        bool same_shape(const Json& a, const Json& b)
        {
            if (a.IsNumber() && b.IsNumber())
            {
                const double x = a.GetDouble(), y = b.GetDouble();
                return std::abs(x - y) <= 1e-6 * std::max({ 1.0, std::abs(x), std::abs(y) });
            }
            if (a.IsObject() && b.IsObject())
            {
                if (a.MemberCount() != b.MemberCount()) { return false; }
                for (auto it = a.MemberBegin(); it != a.MemberEnd(); ++it)
                {
                    const auto other = b.FindMember(it->name);
                    if (other == b.MemberEnd() || !same_shape(it->value, other->value)) { return false; }
                }
                return true;
            }
            if (a.IsArray() && b.IsArray())
            {
                if (a.Size() != b.Size()) { return false; }
                for (rapidjson::SizeType i = 0; i < a.Size(); ++i)
                {
                    if (!same_shape(a[i], b[i])) { return false; }
                }
                return true;
            }
            return a == b;
        }
    }

    LevelDocument::SymmetryReport LevelDocument::symmetry() const
    {
        // Work on a copy, so the reflection helpers can edit freely.
        Document work;
        work.CopyFrom(impl_->document, work.GetAllocator());
        Allocator& allocator = work.GetAllocator();
        const Rect world = plain_rectangle(member(member(work, key::level_bounds), key::out_of_bounds));
        const double sum = world.x + (world.x + world.width);
        const double spawn_width = impl_->motion_tuning.empty() ? 52.0 :
            static_cast<double>(load_motion_player_size(impl_->motion_tuning).width);
        const std::vector<Entry> all = entries();
        const auto listed = [&](EntryKind kind)
        {
            std::vector<Entry> result;
            for (const Entry& entry : all) { if (entry.kind == kind) { result.push_back(entry); } }
            return result;
        };
        SymmetryReport report;

        // Spawns pair across teams, one to one, in document order.
        {
            const std::vector<Entry> team_a = listed(EntryKind::spawn_a), team_b = listed(EntryKind::spawn_b);
            std::vector<bool> used(team_b.size(), false);
            for (const Entry& spawn : team_a)
            {
                Json reflected;
                reflected.CopyFrom(entry_json(work, spawn), allocator);
                set_number(reflected, "x", sum - number(reflected, "x") - spawn_width);
                bool found = false;
                for (std::size_t j = 0; j < team_b.size() && !found; ++j)
                {
                    if (used[j] || !same_shape(reflected, entry_json(work, team_b[j]))) { continue; }
                    used[j] = found = true;
                }
                if (!found) { report.unmatched.push_back(spawn); }
            }
            for (std::size_t j = 0; j < team_b.size(); ++j)
            {
                if (!used[j]) { report.unmatched.push_back(team_b[j]); }
            }
        }

        // Objects pair within their own list; a self-reflection is its own partner.
        for (const EntryKind kind : { EntryKind::collision, EntryKind::decoration })
        {
            const std::vector<Entry> objects = listed(kind);
            std::vector<Json> shapes(objects.size()), reflections(objects.size());
            std::vector<bool> ferry(objects.size(), false), mirrored(objects.size(), false);
            for (std::size_t i = 0; i < objects.size(); ++i)
            {
                const Json& source = entry_json(work, objects[i]);
                ferry[i] = paint_ferry(source);
                shapes[i].CopyFrom(source, allocator);
                strip_unmirrored(shapes[i], ferry[i]);
                // Reflect the path alone: without its drive a ferry mirrors as
                // an ordinary mover, and its phase is then dropped like its heading.
                reflections[i].CopyFrom(source, allocator);
                if (ferry[i])
                {
                    for (const char* field : { "drive", "team_a_heads" }) { reflections[i][key::motion].RemoveMember(field); }
                }
                // A malformed record has no reflection; it simply stays unmatched.
                try { mirror_object(reflections[i], sum, allocator); mirrored[i] = true; }
                catch (const std::exception&) {}
                if (mirrored[i])
                {
                    if (ferry[i]) { reflections[i][key::motion].RemoveMember("phase"); }
                    reflections[i].RemoveMember(key::name);
                }
            }
            std::vector<bool> used(objects.size(), false);
            for (std::size_t i = 0; i < objects.size(); ++i)
            {
                if (used[i]) { continue; }
                if (mirrored[i])
                {
                    for (std::size_t j = i; j < objects.size(); ++j)
                    {
                        if (used[j] || ferry[j] != ferry[i] || !same_shape(reflections[i], shapes[j])) { continue; }
                        used[i] = used[j] = true;
                        if (ferry[i])
                        {
                            report.paint_ferries.push_back(objects[i]);
                            if (j != i) { report.paint_ferries.push_back(objects[j]); }
                        }
                        break;
                    }
                }
                if (!used[i]) { (kind == EntryKind::decoration ? report.unmatched_art : report.unmatched).push_back(objects[i]); }
            }
        }
        return report;
    }
}
