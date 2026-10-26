#pragma once

#include "scripts/acceptance/moving_structure/fixtures.h"
#include <filesystem>

namespace turbulence_slab_stage_acceptance
{
    namespace fs = std::filesystem;
    using moving_structure_acceptance::require;
    void qualify(const fs::path& content, const fs::path& repository, const fs::path& evidence);
}
