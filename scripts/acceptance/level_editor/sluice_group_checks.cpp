#include "scripts/acceptance/level_editor/fixtures.h"

#include <iostream>

namespace level_editor_acceptance
{
    void check_sluice_group_move(const fs::path& repo, const fs::path& scratch)
    {
        const fs::path tuning = repo / "game/content/tuning.json";
        const fs::path copy = scratch / "sluice-group-move.json";
        LevelDocument document(tuning);
        document.load(repo / "game/content/levels/sluice_works.json");
        document.save(copy);
        const std::string original = document.serialize();
        const std::vector<Entry> approach{
            named(document, EntryKind::collision, "gauge_step_a"),
            named(document, EntryKind::decoration, "gauge_trestle_a"),
            named(document, EntryKind::station, "Station gauge_step_a")
        };

        // Drag the left approach four 10-unit grid steps towards the gate.
        // Compare the whole level with the same three ordinary rectangle edits:
        // every other platform, marker, paint face and art setting must survive.
        constexpr double dx = 40.0;
        LevelDocument expected(tuning);
        expected.load(copy);
        for (const Entry& member : approach)
        {
            Rect position = document.rectangle(member);
            position.x += dx;
            expected.set_rectangle(member, position);
        }
        document.move_group(approach, dx, 0.0);
        const std::string moved = document.serialize();
        require(moved != original && parse(moved) == parse(expected.serialize()),
            "Moving the gauge step changed its spacing, its support art, its station or unrelated data");
        document.save(copy);
        LevelDocument reopened(tuning);
        reopened.load(copy);
        require(!reopened.dirty() && parse(reopened.serialize()) == parse(moved),
            "The relocated Sluice Works approach changed across save and reopen");
        require(document.undo() && !document.can_undo() && document.serialize() == original,
            "One Undo did not restore the entire original Sluice Works approach");
        require(document.redo() && document.serialize() == moved && !document.dirty(),
            "One Redo did not restore the saved Sluice Works approach");
        std::cout << "PASS Sluice Works platform, trestle and station group move, save, reopen and undo\n";
    }
}
