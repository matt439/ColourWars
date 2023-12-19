#include "pch.h"
#include "Tile.h"

////using namespace DirectX::SimpleMath;
//using namespace MattMath;
//
//Colour Tile::get_team_colour(player_team team) const
//{
//	switch (team)
//	{
//	case player_team::A:
//		return this->_team_colours.team_a;
//	case player_team::B:
//		return this->_team_colours.team_b;
//	default:
//		return colour_consts::WHITE;
//	}
//}
//
////Rectangle Tile::get_rectangle() const
////{
////	DirectX::SimpleMath::Rectangle result(
////		static_cast<long>(this->_info.position.x),
////		static_cast<long>(this->_info.position.y),
////		static_cast<long>(this->_info.size.x),
////		static_cast<long>(this->_info.size.y));
////    return result;
////}
//
//tile_paint_info Tile::get_paint_info() const
//{
//	tile_paint_info result;
//	result.team = this->_base_paint_team;
//	result.strength = 1.0f - this->_colour_change;
//	return result;
//}