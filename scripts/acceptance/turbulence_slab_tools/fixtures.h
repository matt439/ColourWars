#pragma once

#include "scripts/acceptance/moving_structure/fixtures.h"
#include <filesystem>

namespace turbulence_slab_tools_acceptance
{
    namespace fs = std::filesystem;
    using moving_structure_acceptance::require;
    void run(const fs::path& content, const fs::path& slab, const fs::path& evidence);
}
