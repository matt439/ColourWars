#include "pch.h"
#include "TileBuilder.h"

////using namespace DirectX;
////using namespace DirectX::SimpleMath;
//using namespace tile_consts;
//using namespace MattMath;
//
//std::vector<std::unique_ptr<ICollidable>> TileBuilder::build_tiles(
//	const std::vector<std::vector<char>>& letters)
//{
//	std::vector<std::unique_ptr<ICollidable>> result;
//	for (size_t i = 0; i < letters.size(); i++)
//	{
//		for (size_t j = 0; j < letters[i].size(); j++)
//		{
//			RectangleF rec = { j * TILE_WIDTH, i * TILE_HEIGHT,
//				TILE_WIDTH, TILE_HEIGHT };
//
//			//result.push_back(this->convert_letter_to_tile(letters[i][j], rec));
//
//			switch (letters[i][j])
//			{
//			case 'G':
//				result.push_back(std::make_unique<PaintableFloorSquare>(rec));
//			case 'D':
//				result.push_back(std::make_unique<UnpaintableFloorSquare>(rec));
//			case 'W':
//				result.push_back(std::make_unique<WallSquare>(rec));
//			default:
//				break;
//			}
//		}
//	}
//	return result;
//}
//
////std::unique_ptr<ICollidable> TileBuilder::convert_letter_to_tile(char letter,
////	const RectangleF& rectangle)
////{
////	switch (letter)
////	{
////	case 'G':
////		return std::make_unique<PaintableFloorSquare>(rectangle);
////	case 'D':
////		return std::make_unique<UnpaintableFloorSquare>(rectangle);
////	case 'W':
////		return std::make_unique<WallSquare>(rectangle);
////	default:
////		std::exception("Invalid letter");
////	}
////}
//
////std::vector<std::string> TileBuilder::load_tile_data(level_stage stage)
////{
////    std::string temp, path;
////    switch (stage)
////    {
////    case level_stage::TEST_1:
////        path = "C:/Users/matsi/source/repos/ColourWars/levels/level_0.txt";
////        break;
////    case level_stage::TEST_2:
////        path = "C:/Users/matsi/source/repos/ColourWars/levels/level_0.txt";
////        break;
////    default:
////        path = "C:/Users/matsi/source/repos/ColourWars/levels/level_0.txt";
////        break;
////    };
////    std::ifstream infile(path);
////    std::vector<std::string> lines;
////    while (std::getline(infile, temp))
////    {
////        lines.push_back(temp);
////    }
////    return lines;
////}
//
////std::shared_ptr<std::vector<std::vector<Tile>>>
////    TileBuilder::build_tiles(
////        const std::vector<std::vector<tile_type>>& tiles,
////        const MenuLevelSettings& settings,
////        team_colour team_colours)
////{
////    std::shared_ptr<std::vector<std::vector<Tile>>> result;
////    result = std::make_shared<std::vector<std::vector<Tile>>>();
////
////    int tiles_size = static_cast<int>(tiles.size());
////    for (int i = 0; i < tiles_size; i++)
////    {
////		std::vector<Tile> line;
////        int line_size = static_cast<int>(tiles[i].size());
////        for (int j = 0; j < line_size; j++)
////        {
////			tile_info args;
////			args.size = { TILE_WIDTH, TILE_HEIGHT };
////			args.position = { j * TILE_WIDTH, i * TILE_HEIGHT };
////			args.coordinates = { j, i };
////			args.back_type = tiles[i][j];
////			args.team_colours = team_colours;
////
////			line.push_back(Tile(args));
////		}
////		result->push_back(line);
////	}
////    return result;
////}
//
////std::shared_ptr<std::vector<std::vector<Tile>>>
////TileBuilder::build_tiles(const MenuLevelSettings& settings,
////    team_colour team_colours)
////{
////    std::shared_ptr<std::vector<std::vector<Tile>>> tiles;
////    tiles = std::make_shared<std::vector<std::vector<Tile>>>();
////
////    std::vector<Tile> line;
////    char symbol;
////    tile_type type;
////    Vector2 tile_size = { tile_consts::TILE_WIDTH, tile_consts::TILE_HEIGHT };
////    Vector2 tile_position;
////    XMINT2 tile_coordinates;
////
////    std::vector<std::string> lines;
////    lines = this->load_tile_data(settings.get_stage());
////    for (int i = 0; i < lines.size(); i++)
////    {
////        for (int j = 0; j < lines[i].length(); j++)
////        {
////            symbol = lines[i].at(j);
////            type = this->convert_char_to_type(symbol);
////
////            tile_position = { j * tile_size.x, i * tile_size.y };
////            tile_coordinates = { j, i };
////
////            tile_info args;
////            args.size = tile_size;
////            args.position = tile_position;
////            args.coordinates = tile_coordinates;
////            args.back_type = type;
////            args.team_colours = team_colours;
////
////            line.push_back(Tile(args));
////        }
////        tiles.get()->push_back(line);
////        line.clear();
////    }
////    return tiles;
////}
//
