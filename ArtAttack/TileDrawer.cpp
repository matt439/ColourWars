#include "pch.h"
#include "TileDrawer.h"

//using namespace DirectX;
//using namespace DirectX::SimpleMath;
//
//void TileDrawer::draw(const Tile* tile, const Vector3& camera)
//{
//    Rectangle draw_rectangle =
//        this->calculate_draw_rectangle(tile->get_sm_bounding_box(), camera);
//
//
//
//    if (tile->get_back_type() != tile_type::EMPTY)
//	{
//		//Color colour = tile->get_team_colour(tile->get_base_paint_team());
//
//		sprite_frame_reference texture =
//			this->get_tile_texture(tile->get_back_type());
//		SpriteSheet* sprite_sheet = this->get_resource_manager()->
//			get_sprite_sheet(texture.sprite_sheet_name);
//
//		sprite_sheet->draw(
//			this->get_sprite_batch(),
//			texture.sprite_frame_name,
//			draw_rectangle,
//			colour_consts::WHITE);
//	}
//    if (tile->get_front_type() != tile_type::EMPTY)
//    {
//        Color colour = tile->get_team_colour(tile->get_base_paint_team());
//
//        sprite_frame_reference texture =
//            this->get_tile_texture(tile->get_front_type());
//        SpriteSheet* sprite_sheet = this->get_resource_manager()->
//            get_sprite_sheet(texture.sprite_sheet_name);
//
//        sprite_sheet->draw(
//            this->get_sprite_batch(),
//            texture.sprite_frame_name,
//            draw_rectangle,
//            colour);
//
//        //this->get_sprite_batch()->Draw(
//        //    this->get_resource_manager()->
//        //    get_texture(this->get_tile_texture(tile->get_front_type())),
//        //    draw_rectangle,
//        //    colour);
//    }
//    if (abs(tile->get_colour_change()) >= 0.2f)
//    {
//        Color splatter_colour;
//        if (tile->get_base_paint_team() == player_team::A)
//        {
//            splatter_colour = tile->get_team_colour(player_team::B);
//        }
//        else if (tile->get_base_paint_team() == player_team::B)
//        {
//            splatter_colour = tile->get_team_colour(player_team::A);
//        }
//        else
//        {
//            if (tile->get_colour_change() < 0.0f)
//            {
//                splatter_colour = tile->get_team_colour(player_team::A);
//            }
//            else
//            {
//                splatter_colour = tile->get_team_colour(player_team::B);
//            }
//        }
//
//        sprite_frame_reference texture =
//			this->get_splatter_texture(tile->get_colour_change());
//        SpriteSheet* sprite_sheet = this->get_resource_manager()->
//			get_sprite_sheet(texture.sprite_sheet_name);
//
//        sprite_sheet->draw(this->get_sprite_batch(),
//            texture.sprite_frame_name,
//			draw_rectangle,
//			splatter_colour);
//
//   //     this->get_sprite_batch()->Draw(
//			//this->get_resource_manager()->
//			//    get_texture(this->get_splatter_texture(tile->get_colour_change())),
//			//draw_rectangle,
//   //         splatter_colour);
//    }
//}
//
//sprite_frame_reference TileDrawer::get_tile_texture(tile_type type)
//{
//    switch (type)
//    {
//    case tile_type::SKY:
//        return sprite_frame_reference("sprite_sheet_1", "sky");
//    case tile_type::CLOUD:
//        return sprite_frame_reference("sprite_sheet_1", "cloud");
//    case tile_type::GRASS:
//        return sprite_frame_reference("sprite_sheet_1", "grass");
//    case tile_type::WET_GRASS:
//        return sprite_frame_reference("sprite_sheet_1", "wet_grass");
//    case tile_type::DIRT:
//        return sprite_frame_reference("sprite_sheet_1", "dirt");
//    case tile_type::WET_DIRT:
//        return sprite_frame_reference("sprite_sheet_1", "wet_dirt");
//    case tile_type::FLAME:
//        return sprite_frame_reference("sprite_sheet_1", "flame");
//    case tile_type::SMOKE:
//        return sprite_frame_reference("sprite_sheet_1", "smoke");
//    case tile_type::PAINT:
//        return sprite_frame_reference("sprite_sheet_1", "paint");
//    default:
//        return sprite_frame_reference("sprite_sheet_1", "error");
//    }
//}
//
//sprite_frame_reference TileDrawer::get_splatter_texture(float colour_change)
//{
//    float abs_change = abs(colour_change);
//    if (abs_change >= 0.8f)
//    {
//        return sprite_frame_reference("sprite_sheet_1", "splatter_80");
//    }
//    else if (abs_change >= 0.6f)
//    {
//        return sprite_frame_reference("sprite_sheet_1", "splatter_60");
//    }
//    else if (abs_change >= 0.4f)
//    {
//        return sprite_frame_reference("sprite_sheet_1", "splatter_40");
//    }
//    else if (abs_change >= 0.2f)
//    {
//        return sprite_frame_reference("sprite_sheet_1", "splatter_20");
//    }
//    else
//    {
//        return sprite_frame_reference("sprite_sheet_1", "error");
//    }
//}