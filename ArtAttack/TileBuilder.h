#ifndef TILEBUILDER_H
#define TILEBUILDER_H

////#include "Tile.h"
//#include "tile_consts.h"
//#include "MenuLevelSettings.h"
//#include "TeamColour.h"
//#include "level_stage.h"
//#include <vector>
////#include <string>
////#include <memory>
////#include <iostream>
////#include <fstream>
////#include <sstream>
////#include "SimpleMath.h"
////#include "ICollidable.h"
//#include "IPaintable.h"
//#include "RectangleColliding.h"
//
//class TileBuilder
//{
//private:
//	//std::unique_ptr<ICollidable> build_paintable_floor();
//	//std::unique_ptr<ICollidable> build_unpaintable_floor();
//	//std::unique_ptr<ICollidable> build_wall();
//
//	//std::unique_ptr<ICollidable> convert_letter_to_tile(char letter,
//	//	const MattMath::RectangleF& rectangle);
//public:
//	TileBuilder() {}
//	//std::shared_ptr<std::vector<std::vector<Tile>>> build_tiles(
//	//	const MenuLevelSettings& settings, team_colour team_colours);
//
//	//std::vector<std::string> load_tile_data(level_stage stage);
//
//
//	//std::shared_ptr<std::vector<std::vector<Tile>>> build_tiles(
//	//	const std::vector<std::vector<tile_type>>& tiles,
//	//	const MenuLevelSettings& settings, team_colour team_colours);
//
//
//	std::vector<std::unique_ptr<ICollidable>> build_tiles(
//		const std::vector<std::vector<char>>& letters);
//};
//
//const std::vector<collidable_object_type> PAINTABLE_FLOOR_SQUARE_COLLIDABLE_OBJECT_TYPES =
//{
//	collidable_object_type::LEVEL_OBJECT_PAINTABLE,
//	collidable_object_type::LEVEL_OBJECT_FLOOR
//};
//
//const std::vector<collidable_object_type> UNPAINTABLE_FLOOR_SQUARE_COLLIDABLE_OBJECT_TYPES =
//{
//	collidable_object_type::LEVEL_OBJECT_UNPAINTABLE,
//	collidable_object_type::LEVEL_OBJECT_FLOOR
//};
//
//const std::vector<collidable_object_type> WALL_SQUARE_COLLIDABLE_OBJECT_TYPES =
//{
//	collidable_object_type::LEVEL_OBJECT_UNPAINTABLE,
//	collidable_object_type::LEVEL_OBJECT_WALL
//};
//
//class PaintableFloorSquare : public RectangleColliding, public IPaintable
//{
//
//public:
//	PaintableFloorSquare(const MattMath::RectangleF& bounding_box) :
//		RectangleColliding(bounding_box,
//			bounding_box,
//			PAINTABLE_FLOOR_SQUARE_COLLIDABLE_OBJECT_TYPES,
//			collidable_object_shape::RECTANGLE) {}
//};
//
//class UnpaintableFloorSquare : public RectangleColliding
//{
//public:
//	UnpaintableFloorSquare(const MattMath::RectangleF& bounding_box) :
//		RectangleColliding(bounding_box,
//			bounding_box,
//			UNPAINTABLE_FLOOR_SQUARE_COLLIDABLE_OBJECT_TYPES,
//			collidable_object_shape::RECTANGLE) {}
//};
//
//class WallSquare : public RectangleColliding
//{
//public:
//	WallSquare(const MattMath::RectangleF& bounding_box) :
//		RectangleColliding(bounding_box,
//			bounding_box,
//			WALL_SQUARE_COLLIDABLE_OBJECT_TYPES,
//			collidable_object_shape::RECTANGLE) {}
//};

#endif // !TILEBUILDER_H
