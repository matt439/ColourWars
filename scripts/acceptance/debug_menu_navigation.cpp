#include "game/pch.h"
#include "game/config/presentation/presentation.h"
#include "game/config/tuning.h"
#include "game/debug/debug_director_impl.h"
#include "game/debug/console/debug_menu_model.h"
#include "engine/core/file_path.h"
#include "scripts/acceptance/support/probe.h"
#include <filesystem>
#include <iostream>

// Describe keyboard navigation with the actual CPU menu model. No window,
// renderer, audio device, save or tune command is created/executed here.
int wmain(int argc, wchar_t** argv)
{
    return acceptance::run_probe("debug menu navigation", [&]
    {
        acceptance::require(argc == 3, "usage: debug_menu_navigation <source content> <staged content>");
        const std::filesystem::path source = argv[1];
        const std::filesystem::path content = argv[2];
        GameTuning tuning(labrador::path_to_utf8(content / "tuning.json").c_str());
        Presentation presentation(labrador::path_to_utf8(content / "presentation.json").c_str());
        DebugDirectorImpl director(&tuning, &presentation);
        director.set_content_directories(source, content);
        DebugMenuModel menu(director);
        const std::array<const char*, 4> labels{ "tuning", "tuning", "player", "x_acceleration" };
        bool first = true;
        const auto emit = [&first](std::size_t downs, bool enter)
        {
            if (!first) { std::cout << ','; }
            first = false;
            std::cout << "{\"downs\":" << downs << ",\"enter\":" << (enter ? "true" : "false") << '}';
        };
        std::cout << '[';
        for (std::size_t depth = 0; depth < labels.size(); ++depth)
        {
            bool found = false;
            for (int page = 0; page < 64 && !found; ++page)
            {
                const std::vector<DebugMenuRow> rows = menu.rows();
                for (std::size_t row = 0; row < rows.size(); ++row)
                {
                    if (rows[row].label != labels[depth]) { continue; }
                    const bool enter = depth + 1 < labels.size();
                    acceptance::require(rows[row].enabled &&
                        rows[row].kind == (enter ? DebugMenuRow::Kind::category : DebugMenuRow::Kind::variable),
                        "The driven menu path must select categories then a step-able variable");
                    emit(row, enter);
                    if (enter) { menu.confirm(row); }
                    found = true;
                    break;
                }
                if (!found)
                {
                    acceptance::require(!rows.empty() && rows.back().kind == DebugMenuRow::Kind::more,
                        std::string("Menu path has no child: ") + labels[depth]);
                    emit(rows.size() - 1, true);
                    menu.confirm(rows.size() - 1);
                }
            }
            acceptance::require(found, "Menu path exceeded the bounded page search");
        }
        std::cout << "]\n";
        return 0;
    });
}
