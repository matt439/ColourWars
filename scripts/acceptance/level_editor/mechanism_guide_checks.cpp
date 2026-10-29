// Authored paint-mechanism marks: these describe the saved rules, not a live
// ownership, gate-permission or motion simulation.
#include "scripts/acceptance/level_editor/fixtures.h"
#include "editor/paint_mechanism_guide.h"

#include <cmath>
#include <iostream>
#include <map>

namespace level_editor_acceptance
{
    namespace
    {
        std::map<std::string, std::string> guide_properties(const LevelDocument& document, const Entry& item)
        {
            std::map<std::string, std::string> values;
            for (const Property& property : document.properties(item))
                values[property.path.starts_with('/') ? property.path.substr(1) : property.path] = property.value;
            return values;
        }

        std::optional<PaintMechanismGuide> guide(const LevelDocument& document, const Entry& item)
        {
            return paint_mechanism_guide(guide_properties(document, item));
        }

        bool close(double first, double second) { return std::abs(first - second) < 0.000001; }

        void translated_drawing(const PaintMechanismDrawing& before, const PaintMechanismDrawing& after,
            double dx, double dy)
        {
            require(close(after.panel.x, before.panel.x + dx) && close(after.panel.y, before.panel.y + dy) &&
                close(after.panel.width, before.panel.width) && close(after.panel.height, before.panel.height),
                "Provisional body movement did not carry its fixed-pixel schematic panel");
            require(before.strokes.size() == after.strokes.size() && before.labels.size() == after.labels.size(),
                "Changing selection or a provisional position changed authored guide content");
            for (std::size_t index = 0; index < before.strokes.size(); ++index)
            {
                const auto& a = before.strokes[index];
                const auto& b = after.strokes[index];
                require(close(b.x1, a.x1 + dx) && close(b.x2, a.x2 + dx) &&
                    close(b.y1, a.y1 + dy) && close(b.y2, a.y2 + dy),
                    "A guide stroke did not follow the drawn body's position");
            }
            for (std::size_t index = 0; index < before.labels.size(); ++index)
            {
                const auto& a = before.labels[index];
                const auto& b = after.labels[index];
                require(b.text == a.text && close(b.x, a.x + dx) && close(b.y, a.y + dy),
                    "A team letter did not follow the provisional guide position");
            }
        }

        void ferry_arrows(const PaintMechanismDrawing& drawing, bool a_right)
        {
            require(drawing.labels.size() == 3 && drawing.labels[0].text == L"PAINT FERRY" &&
                drawing.labels[1].text == L"A" && drawing.labels[2].text == L"B" && drawing.strokes.size() == 6,
                "A ferry guide needs its explicit rule name, A/B letters and two arrow shapes");
            for (std::size_t team = 0; team < 2; ++team)
            {
                const auto& shaft = drawing.strokes[team * 3];
                const auto& upper = drawing.strokes[team * 3 + 1];
                const auto& lower = drawing.strokes[team * 3 + 2];
                const bool right = team == 0 ? a_right : !a_right;
                require((right ? shaft.x2 > shaft.x1 : shaft.x2 < shaft.x1) && close(shaft.y1, shaft.y2) &&
                    close(upper.x2, shaft.x2) && close(lower.x2, shaft.x2) &&
                    close(upper.y2, shaft.y2) && close(lower.y2, shaft.y2) &&
                    upper.y1 < shaft.y1 && lower.y1 > shaft.y1 &&
                    (right ? upper.x1 < shaft.x2 && lower.x1 < shaft.x2 :
                        upper.x1 > shaft.x2 && lower.x1 > shaft.x2),
                    "The monochrome arrowheads do not point in their team's authored direction");
            }
        }
    }

    void check_paint_mechanism_guides(const fs::path& repo, const fs::path& scratch)
    {
        LevelDocument document(repo / "game/content/tuning.json");
        const Entry mover = document.add_object("MovingPaintable", { 600.0, 600.0, 320.0, 20.0 });
        require(!guide(document, mover), "An automatic cycle gained a paint-ownership guide");
        document.set_property(mover, "motion/drive", "paint");
        const PaintMechanismGuide right{ PaintMechanismKind::ferry, 1 };
        const PaintMechanismGuide left{ PaintMechanismKind::ferry, -1 };
        require(guide(document, mover) == right, "The inspector's new paint drive did not show A right/B left");
        const std::string before_guide = document.serialize();
        const auto motion = document.motion_guide(mover);
        require(motion.has_value(), "The paint ferry has no initial motion envelope");
        const PaintMechanismDrawing initial = paint_mechanism_drawing(right, motion->initial_pose);
        ferry_arrows(initial, true);
        // Selection changes only the canvas outline. The same model is drawn
        // for selected and unselected bodies, without reading any selection state.
        translated_drawing(initial, paint_mechanism_drawing(*guide(document, mover), motion->initial_pose), 0.0, 0.0);
        const Rect anchor = document.rectangle(mover);
        const Rect preview{ anchor.x + 40.0, anchor.y - 75.0, anchor.width + 50.0, anchor.height };
        const auto preview_motion = document.motion_guide(mover, preview);
        require(preview_motion.has_value(), "Provisional ferry pose was lost");
        const PaintMechanismDrawing provisional = paint_mechanism_drawing(right, preview_motion->initial_pose);
        translated_drawing(initial, provisional, 65.0, -75.0);
        require(document.serialize() == before_guide &&
            equal(document.motion_guide(mover)->travel_envelope, motion->travel_envelope) &&
            equal(document.motion_guide(mover)->standing_envelope, motion->standing_envelope),
            "Drawing a provisional guide changed serialized geometry or admitted motion envelopes");

        document.set_property(mover, "motion/team_a_heads", "left");
        require(guide(document, mover) == left, "A heading inspector edit did not refresh both team arrows");
        ferry_arrows(paint_mechanism_drawing(*guide(document, mover), motion->initial_pose), false);
        require(document.undo() && guide(document, mover) == right &&
            document.redo() && guide(document, mover) == left, "Undo/redo retained a stale ferry guide");
        document.set_property(mover, "motion/phase", "0.5");
        require(guide(document, mover) == left, "Initial end-stop position changed the authored direction guide");
        const Entry copy = document.duplicate(mover);
        require(guide(document, copy) == left, "Duplicating a ferry lost its authored team headings");
        const fs::path ferry_file = scratch / "mechanism-ferry-guide.json";
        const std::string ferry_json = document.serialize();
        document.save(ferry_file);
        LevelDocument reopened(repo / "game/content/tuning.json");
        reopened.load(ferry_file);
        require(reopened.serialize() == ferry_json && guide(reopened, mover) == left && guide(reopened, copy) == left,
            "Save/reopen changed ferry bytes or retained a stale mechanism guide");
        document.set_property(mover, "motion/drive", "cycle");
        require(!guide(document, mover), "A cycle retained the former ferry's team arrows");
        require(document.undo() && guide(document, mover) == left, "Undoing a cycle switch did not restore the ferry guide");
        document.set_property(mover, "motion/drive", "");
        require(!guide(document, mover), "An omitted default cycle drive gained a paint ferry mark");

        LevelDocument gates(repo / "game/content/tuning.json");
        const Entry gate = gates.add_object("TeamGate", { 1000.0, 600.0, 80.0, 240.0 });
        const PaintMechanismGuide gate_rule{ PaintMechanismKind::gate };
        require(guide(gates, gate) == gate_rule, "The TeamGate palette object has no distinct schematic");
        const std::string gate_json = gates.serialize();
        const PaintMechanismDrawing gate_drawing = paint_mechanism_drawing(gate_rule, gates.rectangle(gate));
        require(gate_drawing.labels.size() == 2 && gate_drawing.labels[0].text == L"PAINT GATE" &&
            gate_drawing.labels[1].text == L"A / B" && gate_drawing.strokes.size() == 5,
            "The gate's monochrome lintel/posts and explicit team letters are missing");
        translated_drawing(gate_drawing, paint_mechanism_drawing(gate_rule, { 1040.0, 550.0, 100.0, 260.0 }), 50.0, -50.0);
        require(gates.serialize() == gate_json, "Gate guide generation changed authored gate JSON");
        const Entry gate_copy = gates.duplicate(gate);
        require(guide(gates, gate_copy) == gate_rule, "A duplicated gate lost its mechanism mark");
        gates.set_property(gate, "gate", "");
        require(!guide(gates, gate) && guide_properties(gates, gate).at("material") == "eraser",
            "An ordinary eraser retained a removed gate's schematic");
        require(gates.undo() && guide(gates, gate) == gate_rule && gates.redo() && !guide(gates, gate),
            "Gate removal undo/redo did not refresh the mechanism guide");
        const fs::path gate_file = scratch / "mechanism-gate-guide.json";
        const std::string saved_gate_json = gates.serialize();
        gates.save(gate_file);
        LevelDocument gate_reopened(repo / "game/content/tuning.json");
        gate_reopened.load(gate_file);
        require(gate_reopened.serialize() == saved_gate_json && !guide(gate_reopened, gate) &&
            guide(gate_reopened, gate_copy) == gate_rule,
            "Save/reopen did not distinguish an ordinary eraser from the retained gate");
        std::cout << "PASS authored paint gate/ferry guides, both team headings, monochrome shapes, "
            "provisional poses, inspector/history/duplication and save/reopen\n";
    }
}
