#pragma once
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/file_path.h"
#include "engine/render/dds_file.h"
#include "engine/render/png_file.h"
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

namespace acceptance
{
    // CPU Worlds have no uploaded texture to query. Decode the same single
    // DDS/PNG the manifest loader would use, then give the sheet loader its
    // actual dimensions so its authored-frame bounds checks still run.
    // A modified JSON fixture can name its original atlas through atlas_base.
    inline std::unique_ptr<labrador::SpriteSheet> read_cpu_sprite_sheet(
        const char* json_path, labrador::TextureHandle texture,
        const std::filesystem::path& atlas_base = {})
    {
        const std::filesystem::path base = atlas_base.empty() ?
            labrador::path_from_utf8(json_path) : atlas_base;
        std::filesystem::path dds = base;
        std::filesystem::path png = base;
        dds.replace_extension(".dds");
        png.replace_extension(".png");
        const bool has_dds = std::filesystem::exists(dds);
        const bool has_png = std::filesystem::exists(png);
        const std::string dds_path = labrador::path_to_utf8(dds);
        const std::string png_path = labrador::path_to_utf8(png);
        if (has_dds && has_png)
        {
            throw std::runtime_error("Texture '" + labrador::path_to_utf8(base.stem()) +
                "' has two files, " + dds_path + " and " + png_path + ": delete one.");
        }
        if (!has_dds && !has_png)
        {
            throw std::out_of_range("No file at " + dds_path + " or " + png_path + ".");
        }
        const labrador::TextureData atlas = has_png ?
            labrador::read_png_file(png_path) : labrador::read_dds_file(dds_path);
        return labrador::read_sprite_sheet(json_path, texture, { atlas.width, atlas.height });
    }
}
