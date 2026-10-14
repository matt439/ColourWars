#include "game/pch.h"
#include "game/presentation.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
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
    try
    {
        require(argc == 2, "Usage: team_palette.exe <game/content directory>");
        const std::filesystem::path source = std::filesystem::path(argv[1]) / "presentation.json";
        const Presentation shipped(source.string().c_str());
        const TeamPalettePresentation& palette = shipped.team_palette();
        require(palette.pairs.size() == 3, "Review the authored palette baseline if the set changes");
        const TeamColour orange = TeamColourTools::team_colours(TeamColourPair::blue_orange, palette);
        require(orange.team_a == labrador::Colour(6, 52, 204, 255) &&
            orange.team_b == labrador::Colour(255, 127, 0, 255), "Migration changed the authored blue/orange pair");
        const TeamColour pink = TeamColourTools::team_colours(TeamColourPair::pink_green, palette);
        const TeamColour yellow = TeamColourTools::team_colours(TeamColourPair::blue_yellow, palette);
        require(pink.team_a == labrador::Colour(255, 0, 127, 255) &&
            pink.team_b == labrador::Colour(0, 255, 0, 255) &&
            yellow.team_a == orange.team_a && yellow.team_b == labrador::Colour(255, 255, 0, 255),
            "Migration changed the other authored pairs");
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
            require(changed.random_pair(draw).key == changed.pairs[draw % 4].key,
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
        std::cout << "PASS team palette: authored values, reordered fixed preferences, data-only random expansion, startup rejection.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL team palette: " << error.what() << '\n';
        return 1;
    }
}
