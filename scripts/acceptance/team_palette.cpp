#include "game/pch.h"
#include "game/presentation.h"
#include "scripts/acceptance/support/probe.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    using acceptance::require;


    // Colour-vision check (ROAD#8). Machado, Oliveira and Fernandes (2009)
    // full-severity matrices in linear sRGB, then CIEDE2000 in CIELAB (D65).
    enum class Vision { normal, protan, deutan, tritan };
    constexpr Vision visions[] = { Vision::normal, Vision::protan, Vision::deutan, Vision::tritan };
    const char* vision_name(Vision vision)
    {
        switch (vision)
        {
        case Vision::protan: return "protan";
        case Vision::deutan: return "deutan";
        case Vision::tritan: return "tritan";
        default: return "normal";
        }
    }

    struct Lab { double l = 0.0, a = 0.0, b = 0.0; };

    double linear(float channel)
    {
        const double c = channel;
        return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
    }

    Lab perceived(const labrador::Colour& colour, Vision vision)
    {
        static constexpr double matrices[3][3][3] = {
            { { 0.152286, 1.052583, -0.204868 }, { 0.114503, 0.786281, 0.099216 }, { -0.003882, -0.048116, 1.051998 } },
            { { 0.367322, 0.860646, -0.227968 }, { 0.280085, 0.672501, 0.047413 }, { -0.011820, 0.042940, 0.968881 } },
            { { 1.255528, -0.076749, -0.178779 }, { -0.078411, 0.930809, 0.147602 }, { 0.004733, 0.691367, 0.303900 } } };
        double rgb[3] = { linear(colour.r), linear(colour.g), linear(colour.b) };
        if (vision != Vision::normal)
        {
            const auto& m = matrices[static_cast<int>(vision) - 1];
            double out[3] = {};
            for (int row = 0; row < 3; ++row)
            {
                out[row] = std::clamp(m[row][0] * rgb[0] + m[row][1] * rgb[1] + m[row][2] * rgb[2], 0.0, 1.0);
            }
            std::copy(std::begin(out), std::end(out), std::begin(rgb));
        }
        const double x = (0.4124564 * rgb[0] + 0.3575761 * rgb[1] + 0.1804375 * rgb[2]) / 0.95047;
        const double y = 0.2126729 * rgb[0] + 0.7151522 * rgb[1] + 0.0721750 * rgb[2];
        const double z = (0.0193339 * rgb[0] + 0.1191920 * rgb[1] + 0.9503041 * rgb[2]) / 1.08883;
        const auto f = [](double t) { return t > 216.0 / 24389.0 ? std::cbrt(t) : (24389.0 / 27.0 * t + 16.0) / 116.0; };
        return { 116.0 * f(y) - 16.0, 500.0 * (f(x) - f(y)), 200.0 * (f(y) - f(z)) };
    }

    double ciede2000(const Lab& first, const Lab& second)
    {
        constexpr double pi = 3.14159265358979323846;
        const auto radians = [&](double degrees) { return degrees * pi / 180.0; };
        const auto pow7 = [](double v) { return std::pow(v, 7.0); };
        const double mean_c = (std::hypot(first.a, first.b) + std::hypot(second.a, second.b)) / 2.0;
        const double g = 0.5 * (1.0 - std::sqrt(pow7(mean_c) / (pow7(mean_c) + pow7(25.0))));
        const double a1 = (1.0 + g) * first.a, a2 = (1.0 + g) * second.a;
        const double c1 = std::hypot(a1, first.b), c2 = std::hypot(a2, second.b);
        const auto hue = [&](double a, double b) { const double h = std::atan2(b, a) * 180.0 / pi; return h < 0.0 ? h + 360.0 : h; };
        const double h1 = hue(a1, first.b), h2 = hue(a2, second.b);
        double dh = 0.0;
        if (c1 * c2 != 0.0)
        {
            dh = h2 - h1;
            if (dh > 180.0) { dh -= 360.0; } else if (dh < -180.0) { dh += 360.0; }
        }
        const double d_l = second.l - first.l, d_c = c2 - c1;
        const double d_h = 2.0 * std::sqrt(c1 * c2) * std::sin(radians(dh / 2.0));
        const double mean_l = (first.l + second.l) / 2.0, mean_cp = (c1 + c2) / 2.0;
        double mean_h = h1 + h2;
        if (c1 * c2 != 0.0)
        {
            mean_h = std::fabs(h1 - h2) <= 180.0 ? (h1 + h2) / 2.0 :
                (h1 + h2 < 360.0 ? (h1 + h2 + 360.0) / 2.0 : (h1 + h2 - 360.0) / 2.0);
        }
        const double t = 1.0 - 0.17 * std::cos(radians(mean_h - 30.0)) + 0.24 * std::cos(radians(2.0 * mean_h)) +
            0.32 * std::cos(radians(3.0 * mean_h + 6.0)) - 0.20 * std::cos(radians(4.0 * mean_h - 63.0));
        const double d_theta = 30.0 * std::exp(-std::pow((mean_h - 275.0) / 25.0, 2.0));
        const double r_c = 2.0 * std::sqrt(pow7(mean_cp) / (pow7(mean_cp) + pow7(25.0)));
        const double s_l = 1.0 + 0.015 * std::pow(mean_l - 50.0, 2.0) / std::sqrt(20.0 + std::pow(mean_l - 50.0, 2.0));
        const double s_c = 1.0 + 0.045 * mean_cp, s_h = 1.0 + 0.015 * mean_cp * t;
        const double r_t = -std::sin(radians(2.0 * d_theta)) * r_c;
        return std::sqrt(std::pow(d_l / s_l, 2.0) + std::pow(d_c / s_c, 2.0) + std::pow(d_h / s_h, 2.0) +
            r_t * (d_c / s_c) * (d_h / s_h));
    }

    double difference(const labrador::Colour& first, const labrador::Colour& second, Vision vision)
    {
        return ciede2000(perceived(first, vision), perceived(second, vision));
    }

    // Every structure colour the levels author is a surface paint lands on.
    void collect_level_colours(const rapidjson::Value& value, std::vector<labrador::Colour>& out)
    {
        if (value.IsArray())
        {
            for (const auto& item : value.GetArray()) { collect_level_colours(item, out); }
            return;
        }
        if (!value.IsObject()) { return; }
        for (const auto& member : value.GetObject())
        {
            if (std::string(member.name.GetString()) == "colour" && member.value.IsString())
            {
                const std::string text = member.value.GetString();
                const std::optional<labrador::Colour> named = labrador::colour_from_name(text);
                const labrador::Colour colour = named.has_value() ? *named : labrador::Colour(text);
                if (std::find(out.begin(), out.end(), colour) == out.end()) { out.push_back(colour); }
            }
            else { collect_level_colours(member.value, out); }
        }
    }

    // A pair passes when the two teams stay at least 40 CIEDE2000 apart under
    // normal, protan, deutan and tritan vision, and each team colour stays at
    // least 15 from every arena surface colour and from the HUD health bar.
    struct VisionReport { double pair = 1e9, surroundings = 1e9; std::string worst; };
    constexpr double minimum_pair_difference = 40.0;
    constexpr double minimum_surround_difference = 15.0;

    VisionReport check_vision(const TeamPaletteEntry& entry, const std::vector<labrador::Colour>& surroundings)
    {
        VisionReport report;
        for (const Vision vision : visions)
        {
            const double pair = difference(entry.colours.team_a, entry.colours.team_b, vision);
            if (pair < report.pair) { report.pair = pair; report.worst = vision_name(vision); }
            for (const labrador::Colour& team : { entry.colours.team_a, entry.colours.team_b })
            {
                for (const labrador::Colour& surface : surroundings)
                {
                    report.surroundings = std::min(report.surroundings, difference(team, surface, vision));
                }
            }
        }
        return report;
    }

    class Fixture
    {
    public:
        explicit Fixture(const std::filesystem::path& path) : path_(path) {}
        ~Fixture() { std::error_code ignored; std::filesystem::remove(path_, ignored); }
        Presentation load(const rapidjson::Document& document) const
        {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            document.Accept(writer);
            { std::ofstream output(path_); output << buffer.GetString(); require(output.good(), "Could not write palette fixture"); }
            return Presentation(path_.string().c_str());
        }
        void reject(const rapidjson::Document& document, const char* expected) const
        {
            bool rejected = false;
            try { this->load(document); }
            catch (const std::exception& error)
            {
                const std::string message = error.what();
                rejected = message.find(expected) != std::string::npos &&
                    message.find("team_palette") != std::string::npos;
            }
            require(rejected, "Invalid palette must fail at startup with palette field context");
        }
    private:
        std::filesystem::path path_;
    };
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("team palette", [&]
    {
        require(argc == 2, "Usage: team_palette.exe <game/content directory>");
        const std::filesystem::path source = std::filesystem::path(argv[1]) / "presentation.json";
        const Presentation shipped(source.string().c_str());
        const TeamPalettePresentation& palette = shipped.team_palette();
        require(palette.pairs.size() == 6, "Review the authored palette baseline if the set changes");
        const TeamColour orange = TeamColourTools::team_colours(TeamColourPair::blue_orange, palette);
        require(orange.team_a == labrador::Colour(6, 52, 204, 255) &&
            orange.team_b == labrador::Colour(255, 127, 0, 255), "Migration changed the authored blue/orange pair");
        const TeamColour pink = TeamColourTools::team_colours(TeamColourPair::pink_green, palette);
        const TeamColour yellow = TeamColourTools::team_colours(TeamColourPair::blue_yellow, palette);
        require(pink.team_a == labrador::Colour(255, 0, 127, 255) &&
            pink.team_b == labrador::Colour(0, 255, 0, 255) &&
            yellow.team_a == orange.team_a && yellow.team_b == labrador::Colour(255, 255, 0, 255),
            "Migration changed the other authored pairs");
        const TeamColour purple = TeamColourTools::team_colours(TeamColourPair::purple_gold, palette);
        const TeamColour red = TeamColourTools::team_colours(TeamColourPair::red_azure, palette);
        const TeamColour magenta = TeamColourTools::team_colours(TeamColourPair::magenta_forest, palette);
        require(purple.team_a == labrador::Colour(123, 31, 162, 255) && purple.team_b == labrador::Colour(255, 194, 10, 255) &&
            red.team_a == labrador::Colour(224, 32, 27, 255) && red.team_b == labrador::Colour(30, 144, 255, 255) &&
            magenta.team_a == labrador::Colour(255, 0, 255, 255) && magenta.team_b == labrador::Colour(38, 153, 0, 255),
            "The ROAD#8 pairs changed; rerun the colour-vision review before accepting new values");

        // Colour vision. The two legacy pairs predate the check and are kept as
        // they shipped (pink/green falls to 27 under deutan; yellow sits 10 from
        // white). They stay listed only while they still fail, so a recolour that
        // fixes one has to take it off this list.
        std::vector<labrador::Colour> surroundings;
        for (const auto& level : std::filesystem::directory_iterator(std::filesystem::path(argv[1]) / "levels"))
        {
            if (level.path().extension() != ".json") { continue; }
            std::ifstream stream(level.path());
            const std::string json((std::istreambuf_iterator<char>(stream)), {});
            rapidjson::Document parsed;
            parsed.Parse(json.c_str());
            require(!parsed.HasParseError(), "Could not parse a level for its surface colours");
            collect_level_colours(parsed, surroundings);
        }
        require(surroundings.size() >= 2, "Found no authored level surface colours to check against");
        surroundings.push_back(shipped.hud().health_colour);
        const std::vector<std::string> legacy = { "pink_green", "blue_yellow" };
        for (const TeamPaletteEntry& entry : palette.pairs)
        {
            const VisionReport report = check_vision(entry, surroundings);
            const bool passes = report.pair >= minimum_pair_difference &&
                report.surroundings >= minimum_surround_difference;
            std::cout << "  " << entry.key << ": pair " << static_cast<int>(report.pair) << " (worst " << report.worst <<
                "), surroundings " << static_cast<int>(report.surroundings) << (passes ? "" : " - below the bar") << '\n';
            const bool exempt = std::find(legacy.begin(), legacy.end(), entry.key) != legacy.end();
            if (exempt)
            {
                require(!passes, "A legacy pair now passes the colour-vision check; remove it from the exemption list");
            }
            else
            {
                require(passes, "A team pair fails the colour-vision check (see the line above)");
            }
        }
        std::ifstream input(source);
        const std::string text((std::istreambuf_iterator<char>(input)), {});
        rapidjson::Document document;
        document.Parse(text.c_str());
        require(!document.HasParseError(), "Could not parse shipped presentation");
        rapidjson::Document baseline;
        baseline.CopyFrom(document, baseline.GetAllocator());
        const Fixture fixture(std::filesystem::absolute(argv[0]).parent_path() / "team-palette-fixture.json");
        rapidjson::Value& pairs = document["team_palette"]["pairs"];
        pairs[0].Swap(pairs[2]);
        rapidjson::Value extra(rapidjson::kObjectType);
        extra.AddMember("key", "orange_green", document.GetAllocator());
        extra.AddMember("label", "A: Orange / B: Green", document.GetAllocator());
        extra.AddMember("team_a", "orange", document.GetAllocator());
        extra.AddMember("team_b", "green", document.GetAllocator());
        pairs.PushBack(extra, document.GetAllocator());
        document["team_palette"]["colours"][0]["colour"].SetString("102030");
        const Presentation edited = fixture.load(document);
        const TeamPalettePresentation& changed = edited.team_palette();
        require(changed.fixed_pair(TeamColourPair::blue_orange).colours.team_a == labrador::Colour(16, 32, 48, 255),
            "A data edit or reordering failed to reach the fixed saved preference");
        for (size_t draw = 0; draw < 40; ++draw)
        {
            require(changed.random_pair(draw).key == changed.pairs[draw % changed.pairs.size()].key,
                "Random draw must include added data pairs and wrap by the list size");
        }
        std::srand(0);
        bool saw_extra = false;
        for (int draw = 0; draw < 100; ++draw)
        {
            const TeamColour colours = TeamColourTools::team_colours(TeamColourPair::random, changed);
            saw_extra |= colours.team_a == orange.team_b && colours.team_b == pink.team_b;
        }
        require(saw_extra, "Runtime random selection never reached the added pair");
        const auto reset = [&] { document.CopyFrom(baseline, document.GetAllocator()); };
        reset(); document["team_palette"]["pairs"].Clear(); fixture.reject(document, "missing fixed team pair");
        reset(); document["team_palette"]["pairs"][0]["team_a"].SetString("missing"); fixture.reject(document, "unknown team colour");
        reset(); document["team_palette"]["pairs"][1]["key"].SetString("blue_orange"); fixture.reject(document, "duplicate team pair");
        reset(); document["team_palette"]["colours"][1]["name"].SetString("blue"); fixture.reject(document, "duplicate team colour");
        reset(); document["team_palette"]["colours"][0]["colour"].SetString("ZZZZZZ"); fixture.reject(document, "colour");
        reset(); document["team_palette"]["pairs"][0]["key"].SetString("random"); fixture.reject(document, "preference");
        reset(); document["team_palette"]["pairs"][0]["label"].SetString(""); fixture.reject(document, "label");
        reset(); document["team_palette"]["colours"][0]["name"].SetString(""); fixture.reject(document, "name");
        reset(); document["team_palette"]["pairs"][0]["key"].SetString(""); fixture.reject(document, "key");
        reset(); document.RemoveMember("team_palette"); fixture.reject(document, "team_palette");
        std::cout << "PASS team palette: authored values, colour-vision bar, reordered fixed preferences, data-only random expansion, startup rejection.\n";
        return 0;
    });
}
