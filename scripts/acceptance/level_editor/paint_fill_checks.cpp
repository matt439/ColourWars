#include "scripts/acceptance/level_editor/fixtures.h"
#include "scripts/acceptance/support/paint_fill_variants.h"

#include <iostream>

namespace level_editor_acceptance
{
    void check_paint_fill(const fs::path& repo, const fs::path& scratch)
    {
        const fs::path levels = repo / "game/content/levels";
        const fs::path source = levels / "turbulence_fill_trial.json";
        const fs::path copy = scratch / "authored-fill.json";
        const fs::path tuning = repo / "game/content/tuning.json";
        const std::string original = read_text(source);
        LevelDocument document(tuning);
        document.load(source);
        const Entry baffle = named(document, EntryKind::collision, "centre_baffle");
        reject_without_change(document, [&] { document.erase(baffle); }, "Erasing a referenced fill object succeeded");
        reject_without_change(document, [&] { document.set_property(baffle, "paintable_faces/left", "false"); },
            "Disabling one of the two authored trigger faces succeeded");
        document.save(copy);
        LevelDocument reopened(tuning);
        reopened.load(copy);
        require(parse(reopened.serialize()) == parse(original) && !reopened.dirty(),
            "The Fill Trial changed across save and reopen");
        acceptance::check_invalid_paint_fill(levels, scratch, [&](const fs::path& path, const auto&, bool)
        {
            refuses_file(reopened, path, "paint_fill", "The editor accepted an invalid paint fill");
            require(reopened.path() == copy && !reopened.dirty() && !reopened.can_undo(),
                "A refused fill file changed the open document or its history");
        });
        // A left face can receive paint from a left face on another object.
        auto split = parse(original);
        split["paint_fill"]["target"]["object"].SetString("crossover_a");
        split["paint_fill"]["target"]["face"].SetString("left");
        const fs::path split_path = scratch / "fill-two-objects.json";
        write_text(split_path, encode(split));
        reopened.load(split_path);
        require(reopened.validate().empty() && parse(reopened.serialize()) == split,
            "A distinct target object with a trigger's face name did not open");
        std::cout << "PASS Fill Trial round trip, referenced-object edits and malformed fill regions\n";
    }
}
