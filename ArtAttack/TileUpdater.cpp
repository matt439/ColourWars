#include "pch.h"
#include "TileUpdater.h"


//void TileUpdater::update(Tile* tile)
//{
//    tile_paintable paintable = get_tile_paintable(tile->get_back_type());
//    //no need to update logic if back tile is not paintable
//    if (paintable == tile_paintable::PAINTABLE)
//    {
//        if (tile->get_timer_on())
//        {
//            tile->set_timer(tile->get_timer() + this->get_dt());
//        }
//        if (tile->get_sprayed() == spray_state::SPRAYED)
//        {
//            tile->set_timer_on(true);
//            tile->set_timer(0.0f);
//            tile->set_reverting(false);
//
//            if (tile->get_base_paint_team() == player_team::NONE)
//            {
//                this->update_unpainted_tile(tile);
//            }
//            //tile has already been fully painted
//            else
//            {
//                this->update_painted_tile(tile);
//            }
//        }
//        else if (tile->get_reverting())
//        {
//            if (tile->get_base_paint_team() == player_team::NONE)
//            {
//                revert_unpainted_tile(tile);
//            }
//            //tile has already been fully painted
//            else
//            {
//                revert_painted_tile(tile);
//            }
//        }
//
//        if (tile->get_timer() > tile_consts::PAINT_REVERT_DELAY)
//        {
//            tile->set_timer_on(false);
//            tile->set_reverting(true);
//            tile->set_timer(0.0f);
//        }
//    }
//    //reset all logic variables since they are not needed when not paintable
//    else
//    {
//        tile->set_timer(0.0f);
//        tile->set_timer_on(false);
//        tile->set_reverting(false);
//        tile->set_front_type(tile_type::EMPTY);
//        tile->set_sprayed_by_team(player_team::NONE);
//        tile->set_colour_change(0.0f);
//    }
//    tile->set_sprayed(spray_state::NOT_SPRAYED);
//    tile->set_colour_power(0.0f);
//}
//
////Tile TileUpdater::update_and_create_new(const Tile* tile)
////{
////	Tile result = tile;
////	TileUpdater::update(result);
////	return result;
////}
//
//void TileUpdater::update_unpainted_tile(Tile* tile)
//{
//    player_team team = tile->get_sprayed_by_team();
//    if (team == player_team::A)
//    {
//        tile->set_colour_change(tile->get_colour_change() -
//            tile->get_colour_power() * this->get_dt());
//    }
//    //player team B
//    else
//    {
//        tile->set_colour_change(tile->get_colour_change() +
//            tile->get_colour_power() * this->get_dt());
//    }
//
//    if (abs(tile->get_colour_change()) >= 1.0f)
//    {
//        tile->set_base_paint_team(team);
//        tile->set_colour_change(0.0f);
//        tile->set_front_type(tile_type::PAINT);
//        this->reset_timer(tile);
//    }
//}
//
//void TileUpdater::update_painted_tile(Tile* tile)
//{
//    player_team team = tile->get_sprayed_by_team();
//    //team is spraying its own painted tile
//    if (tile->get_base_paint_team() == team)
//    {
//        tile->set_colour_change(tile->get_colour_change() -
//            tile->get_colour_power() * this->get_dt());
//        if (tile->get_colour_change() < 0.0f)
//        {
//            tile->set_colour_change(0.0f);
//            this->reset_timer(tile);
//        }
//    }
//    //enemy team is spraying tile.
//    else
//    {
//        tile->set_colour_change(tile->get_colour_change() +
//            tile->get_colour_power() * this->get_dt());
//        if (tile->get_colour_change() >= 1.0f)
//        {
//            tile->set_base_paint_team(team);
//            tile->set_colour_change(0.0f);
//            this->reset_timer(tile);
//        }
//    }
//}
//
//void TileUpdater::revert_unpainted_tile(Tile* tile)
//{
//    if (tile->get_colour_change() < 0.0f)
//    {
//        tile->set_colour_change(tile->get_colour_change() +
//            tile_consts::PAINT_REVERT_RATE * this->get_dt());
//        if (tile->get_colour_change() > 0.0f)
//        {
//            tile->set_colour_change(0.0f);
//            tile->set_reverting(false);
//        }
//    }
//    else
//    {
//        tile->set_colour_change(tile->get_colour_change() -
//            tile_consts::PAINT_REVERT_RATE * this->get_dt());
//        if (tile->get_colour_change() < 0.0f)
//        {
//            tile->set_colour_change(0.0f);
//            tile->set_reverting(false);
//        }
//    }
//}
//void TileUpdater::revert_painted_tile(Tile* tile)
//{
//    tile->set_colour_change(tile->get_colour_change() -
//        tile_consts::PAINT_REVERT_RATE * this->get_dt());
//    if (tile->get_colour_change() < 0.0f)
//    {
//        tile->set_colour_change(0.0f);
//        tile->set_reverting(false);
//    }
//}
//
//
//void TileUpdater::reset_timer(Tile* tile)
//{
//    tile->set_timer(0.0f);
//    tile->set_timer_on(false);
//    tile->set_reverting(false);
//}
//
//void TileUpdater::reset(Tile* tile)
//{
//    this->reset_timer(tile);
//    tile->set_front_type(tile_type::EMPTY);
//    tile->set_sprayed(spray_state::NOT_SPRAYED);
//    tile->set_sprayed_by_team(player_team::NONE);
//    tile->set_base_paint_team(player_team::NONE);
//    tile->set_colour_change(0.0f);
//    tile->set_colour_power(0.0f);
//}

