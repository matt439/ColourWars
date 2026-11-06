#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"

#include <fstream>
#include <iterator>
#include <stdexcept>

// The fixture helpers more than one group of checks uses.
namespace menu_text_acceptance
{
    UiText* row_text(OptionRow& row, const std::string& name)
    {
        for (const auto& child : row.children())
        {
            if (child.first == name) { return dynamic_cast<UiText*>(child.second); }
        }
        return nullptr;
    }

    MenuPaperArtPresentation options_art(const MainMenuPresentation& main)
    {
        MenuPaperArtPresentation art = main.art;
        art.focus_padding = main.setup_focus_padding;
        art.shadow_offset = main.setup_shadow_offset;
        return art;
    }
}
