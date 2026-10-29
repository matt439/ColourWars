// Symmetry report (ED#25): the blockout summary's symmetry line pairs objects
// under the ED#24 mirror rule and splits scoring area at the out_of_bounds
// centre. Press Tower has no unmatched spawns or collision objects (its
// lone brush pot is art, listed apart) and equal halves; a
// deliberately unmatched copy is reported; the Ferry Trial's paint ferries
// are listed as authored. It reports only and decides no stage's fairness.
#include "scripts/acceptance/level_editor/fixtures.h"
#include "editor/blockout_aids.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace level_editor_acceptance
{
    namespace
    {
        bool close(double a, double b) { return std::abs(a - b) <= 1e-6 * std::max(1.0, std::abs(a) + std::abs(b)); }

        Entry named(const LevelDocument& document, EntryKind kind, const std::string& name)
        {
            for (const Entry& candidate : document.entries())
                if (candidate.kind == kind && candidate.label == name) return candidate;
            throw std::runtime_error("Expected object '" + name + "' is absent");
        }

        std::string labels(const std::vector<Entry>& entries)
        {
            std::string text;
            for (const Entry& entry : entries) text += (text.empty() ? "" : ", ") + entry.label;
            return text.empty() ? "none" : text;
        }

        struct Reading
        {
            LevelDocument::SymmetryReport report;
            SideScoring sides;
            double total = 0.0;
            std::string line;
        };

        Reading read(const LevelDocument& document, const BlockoutTuning& tuning)
        {
            const auto faces = scoring_faces(document, tuning);
            Reading reading{ document.symmetry(),
                side_scoring_areas(faces, document.rectangle(Entry{ EntryKind::bounds, 0, "" })),
                scoring_area(faces), symmetry_summary(document, faces) };
            require(close(reading.sides.left + reading.sides.right, reading.total),
                "Left and right scoring area do not add up to the total");
            return reading;
        }
    }

    void check_symmetry_report(const fs::path& repo, const fs::path& /*scratch*/)
    {
        const auto tuning_path = repo / "game/content/tuning.json";
        const auto levels = repo / "game/content/levels";
        const BlockoutTuning tuning = load_blockout_tuning(tuning_path, repo / "editor/movement-baseline.json");

        // Press Tower: every object and spawn has its partner and the halves match.
        {
            LevelDocument document(tuning_path);
            document.load(levels / "press_tower.json");
            const Reading reading = read(document, tuning);
            require(reading.report.unmatched.empty(),
                "Press Tower reports objects without a mirror partner: " + labels(reading.report.unmatched));
            require(reading.report.paint_ferries.empty(), "Press Tower reports paint ferries");
            // Its one lone decoration, the left brush pot, is art and listed apart.
            require(reading.report.unmatched_art.size() == 1 && reading.report.unmatched_art[0].label == "brush_pot",
                "Press Tower's unpartnered art is not exactly brush_pot: " + labels(reading.report.unmatched_art));
            require(reading.line.find("Art without partner: brush_pot") != std::string::npos,
                "Press Tower's symmetry line does not list brush_pot as art: " + reading.line);
            require(reading.total > 0.0 && close(reading.sides.left, reading.sides.right),
                "Press Tower's left and right scoring areas differ: " + std::to_string(reading.sides.left) +
                " / " + std::to_string(reading.sides.right));
            require(reading.line.find("every object has a mirror partner") != std::string::npos,
                "Press Tower's symmetry line does not say every object is partnered: " + reading.line);
            std::cout << "  press_tower: " << reading.line << '\n';

            // A deliberately unmatched copy: without stair_1_b, stair_1_a is
            // alone and the left half scores more. Mirror copy restores it.
            const std::string before = document.serialize();
            document.erase(named(document, EntryKind::collision, "stair_1_b"));
            const Reading lonely = read(document, tuning);
            require(lonely.report.unmatched.size() == 1 && lonely.report.unmatched[0].label == "stair_1_a",
                "Removing stair_1_b did not leave exactly stair_1_a unmatched: " + labels(lonely.report.unmatched));
            require(lonely.sides.left > lonely.sides.right, "Removing a right-hand stair did not unbalance the halves");
            require(lonely.line.find("1 without a mirror partner: stair_1_a") != std::string::npos,
                "The symmetry line does not name the unmatched stair: " + lonely.line);
            document.mirror_copy(named(document, EntryKind::collision, "stair_1_a"));
            const Reading restored = read(document, tuning);
            require(restored.report.unmatched.empty() && close(restored.sides.left, restored.sides.right),
                "A mirror copy of stair_1_a did not restore the symmetry");
            document.undo(); document.undo();
            require(document.serialize() == before, "Undo did not return Press Tower to its source");

            // A spawn moved 10 units no longer meets its other-team partner.
            const Entry spawn = entry(document, EntryKind::spawn_a, 1);
            Rect moved = document.rectangle(spawn); moved.x += 10.0;
            document.set_rectangle(spawn, moved);
            const Reading shifted = read(document, tuning);
            require(shifted.report.unmatched.size() == 2 && shifted.report.unmatched[0].kind == EntryKind::spawn_a &&
                shifted.report.unmatched[1].kind == EntryKind::spawn_b,
                "A moved spawn and its partner were not both reported: " + labels(shifted.report.unmatched));
            document.undo();

            // A centre-straddling object splits by region: one 300 wide across
            // the centre, 100 left of it, adds twice as much top area right.
            const Rect world = document.rectangle(Entry{ EntryKind::bounds, 0, "" });
            const double centre = world.x + world.width / 2.0;
            const Reading base = read(document, tuning);
            const Entry straddle = document.add_object("StructurePaintable", { centre - 100.0, 300.0, 300.0, 40.0 });
            const Reading split = read(document, tuning);
            require(split.report.unmatched.size() == 1 && split.report.unmatched[0].index == straddle.index &&
                split.report.unmatched[0].kind == straddle.kind,
                "An off-centre straddling object was not the one unmatched object: " + labels(split.report.unmatched));
            const double added_left = split.sides.left - base.sides.left;
            const double added_right = split.sides.right - base.sides.right;
            require(added_left > 0.0 && added_right > added_left,
                "A centre-straddling object did not split its area by region");
            document.undo();
            // Centred, it is its own reflection and is partnered.
            document.add_object("StructurePaintable", { centre - 150.0, 300.0, 300.0, 40.0 });
            const Reading centred = read(document, tuning);
            require(centred.report.unmatched.empty() && close(centred.sides.left, centred.sides.right),
                "A centred object was not its own mirror partner: " + labels(centred.report.unmatched));
        }

        // The Ferry Trial: both ferries head right for Team A, which has no
        // mirror rule; they pair by path and are listed as authored.
        {
            LevelDocument document(tuning_path);
            document.load(levels / "turbulence_ferry_trial.json");
            const Reading reading = read(document, tuning);
            require(reading.report.paint_ferries.size() == 2 &&
                reading.report.paint_ferries[0].label == "ferry_a" && reading.report.paint_ferries[1].label == "ferry_b",
                "The Ferry Trial's ferries were not listed as authored: " + labels(reading.report.paint_ferries));
            for (const Entry& entry : reading.report.unmatched)
                require(entry.label.find("ferry") == std::string::npos, "A paint ferry was reported unmatched");
            require(reading.line.find("Authored: paint-ferry heading (ferry_a, ferry_b)") != std::string::npos,
                "The Ferry Trial's line does not list the ferries as authored: " + reading.line);
            std::cout << "  turbulence_ferry_trial: " << reading.line << '\n';
        }

        // Every shipped stage reads; the lines are printed for the record and
        // judge nothing beyond the two cases above.
        for (const auto& file : fs::directory_iterator(levels))
        {
            if (file.path().extension() != ".json") continue;
            const std::string stage = file.path().stem().string();
            if (stage == "press_tower" || stage == "turbulence_ferry_trial") continue;
            LevelDocument document(tuning_path);
            document.load(file.path());
            std::cout << "  " << stage << ": " << read(document, tuning).line << '\n';
        }
        std::cout << "PASS symmetry report\n";
    }
}
