#pragma once

#include "scripts/acceptance/turbulence_movers/fixtures.h"
#include "editor/blockout_aids.h"

namespace turbulence_slopes_acceptance
{
    namespace fs = std::filesystem;
    using turbulence_movers_acceptance::Context;
    using turbulence_movers_acceptance::Trial;
    using turbulence_movers_acceptance::World;
    using turbulence_movers_acceptance::require;
    using turbulence_movers_acceptance::read_text;
    using turbulence_movers_acceptance::input;
    using turbulence_movers_acceptance::supported;
    using turbulence_movers_acceptance::mirrored;
    using turbulence_movers_acceptance::named_rectangle;
    using turbulence_movers_acceptance::settings;
    using turbulence_movers_acceptance::steering;

    struct SourceGuard
    {
        std::vector<std::pair<fs::path, std::string>> sources;
        SourceGuard(const fs::path& content, const fs::path& tuning, const fs::path& source);
        void check() const;
    };

    void author_trial(const fs::path& content, const fs::path& source, const fs::path& tuning, const fs::path& output);
    void geometry(Context& context);
    void routes(Context& context);
    void tool_checks(Context& context);
    void comparison_checks(Context& context, const fs::path& slab_stage = {});
}
