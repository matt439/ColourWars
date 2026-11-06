#pragma once

// The CPU half of what wWinMain's manifest load gives a probe: every sprite
// sheet and font `content`/manifest.json lists, with no texture behind them,
// and a silent stand-in for every sound bank. A probe World that builds real
// game objects calls this instead of copying the manifest's lists, so a sheet
// or font added to the game reaches every probe the day it lands.

#include "scripts/acceptance/support/stage_catalog.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/audio/audio_resources.h"
#include "engine/audio/sound_bank.h"
#include "engine/core/file_path.h"
#include "engine/render/font.h"
#include "engine/render/render_resources.h"
#include "engine/render/sprite_font_file.h"
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace acceptance
{
    // Refuses a manifest group of a kind this World does not load, so a new
    // kind of asset cannot be skipped by every probe without notice.
    inline void require_known_manifest_kinds(const std::filesystem::path& content)
    {
        const labrador::JsonDocument manifest =
            labrador::read_json_file(labrador::path_to_utf8(content / "manifest.json").c_str());
        const labrador::JsonValue assets = manifest.root().array("assets");
        for (size_t group = 0; group < assets.size(); ++group)
        {
            const std::string kind = assets.at(group).string("kind");
            if (kind != "font" && kind != "sprite_sheet" && kind != "sound_bank" && kind != "level")
            {
                throw std::runtime_error("manifest.json lists a kind no probe World loads: " + kind);
            }
        }
    }

    // Every manifest sprite sheet but those named in `without_sheets`, and
    // every manifest font, each with the engine loader's '?' stand-in. Sound
    // banks are left to the caller: for a probe that opens a recording bank of
    // its own.
    inline void load_cpu_render_assets(const std::filesystem::path& content,
        labrador::RenderResources& render, std::initializer_list<std::string_view> without_sheets = {})
    {
        require_known_manifest_kinds(content);
        for (const ManifestEntry& sheet : manifest_entries(content, "sprite_sheet", ".json"))
        {
            bool skipped = false;
            for (const std::string_view name : without_sheets) { skipped = skipped || sheet.name == name; }
            if (skipped) { continue; }
            render.add_sprite_sheet(sheet.name, labrador::read_sprite_sheet(
                labrador::path_to_utf8(sheet.path).c_str(), labrador::TextureHandle{}));
        }
        for (const ManifestEntry& font : manifest_entries(content, "font", ".spritefont"))
        {
            const labrador::SpriteFontFile file = labrador::read_sprite_font_file(labrador::path_to_utf8(font.path));
            auto loaded = std::make_unique<labrador::Font>(labrador::TextureHandle{}, file.glyphs, file.line_spacing);
            if (loaded->contains(U'?')) { loaded->set_stand_in(U'?'); }
            render.add_font(font.name, std::move(loaded));
        }
    }

    // load_cpu_render_assets, and a silent bank under every sound bank name
    // the manifest lists, as the loader substitutes for a missing optional one.
    inline void load_cpu_assets(const std::filesystem::path& content,
        labrador::RenderResources& render, labrador::AudioResources& audio,
        std::initializer_list<std::string_view> without_sheets = {})
    {
        load_cpu_render_assets(content, render, without_sheets);
        for (const ManifestEntry& bank : manifest_entries(content, "sound_bank", ".xwb"))
        {
            audio.add_sound_bank(bank.name, labrador::SoundBank::silent());
        }
    }

    // The resources alone, loaded as above: the World of a probe that checks
    // content against them and builds no Level.
    struct ContentWorld
    {
        labrador::RenderResources render;
        labrador::AudioResources audio;

        explicit ContentWorld(const std::filesystem::path& content)
        {
            load_cpu_assets(content, render, audio);
        }
    };
}
