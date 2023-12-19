#include "pch.h"
#include "PlayerDrawer.h"

//using namespace DirectX;
////using namespace DirectX::SimpleMath;
//using namespace MattMath;
//
//PlayerDrawer::PlayerDrawer(DirectX::SpriteBatch* sprite_batch,
//    ResourceManager* resource_manager, const float* dt) :
//    Drawer(sprite_batch, resource_manager, dt)
//{
//    this->_weapon_drawer =
//        std::make_unique<WeaponDrawer>(sprite_batch, resource_manager, dt);
//    //this->_camera = std::make_unique<Camera>();
//}
//
//void PlayerDrawer::draw(const Player* player, const Vector3F& camera)
//{
//    if (player->get_state() == player_state::ALIVE)
//    {
//        //glm::vec3 camera = player->get_camera();
//        //Vector2 size = player->get_size();
//        //Vector2 draw_location = (player->get_position() - glm::vec2(camera.x, camera.y)) * camera.z;
//
//
//        RectangleI draw_rectangle = this->calculate_draw_rectangle(
//            player->get_bounding_box_i(), camera);
//        SpriteEffects sprite_effects = SpriteEffects_None;
//
//        if (!player->get_facing_right()) // facing left
//		{
//			sprite_effects = SpriteEffects_FlipHorizontally;
//		}
//
//        //if (player->get_facing_right())
//        //{
//        //    this->_sprite_renderer->DrawSprite(player->get_texture(), draw_location, size * camera.z, glm::vec3(1.0f));
//        //}
//        //else
//        //{
//        //    this->_sprite_renderer->DrawSprite(player->get_texture(), draw_location, size * camera.z, glm::vec3(1.0f), true, false);
//        //}
//
//        sprite_frame_reference texture = this->get_texture(player->get_player_num());
//        SpriteSheet* sprite_sheet = this->get_resource_manager()->
//            get_sprite_sheet(texture.sprite_sheet_name);
//
//        sprite_sheet->draw(
//            this->get_sprite_batch(),
//            texture.sprite_frame_name,
//            draw_rectangle,
//            colour_consts::WHITE,
//            0.0f,
//            Vector2F::ZERO,
//            sprite_effects,
//            0.0f);
//
//        //this->get_sprite_batch()->Draw(
//        //    this->get_resource_manager()->get_texture(player->get_texture()),
//        //    draw_rectangle,
//        //    nullptr,
//        //    Colors::White,
//        //    0.0f,
//        //    Vector2::Zero,
//        //    sprite_effects,
//        //    0.0f);
//
//        //this->draw_center(player, camera);
//
//        //this->_weapon_drawer->draw_weapon(player->get_primary_const(), player->get_camera());
//        this->_weapon_drawer->draw_weapon(player->get_primary_const(), camera);
//    }
//}
//
////void PlayerDrawer::draw_center(const Player* player, const Vector3& camera)
////{
////    Vector2 center = player->get_center() - Vector2(camera.x, camera.y);
////    Vector2 origin = Vector2(2.0f, 2.0f);
////    this->get_sprite_batch()->Draw(
////        this->get_resource_manager()->get_texture("pixel"),
////        center,
////        nullptr,
////        colour_consts::CYAN,
////        0.0f,
////        origin);
////}
//
//sprite_frame_reference PlayerDrawer::get_texture(int player_num) const
//{
//    switch (player_num)
//    {
//    case 0:
//        return sprite_frame_reference("sprite_sheet_1", "player_1_right");
//    case 1:
//        return sprite_frame_reference("sprite_sheet_1", "player_2_right");
//    default:
//        return sprite_frame_reference("sprite_sheet_1", "error");
//    }
//}
//
////Vector3 PlayerDrawer::calculate_camera(const Player* player)
////{
////    calculate_cam args_cam;
////    args_cam.player_center = player->get_center();
////    args_cam.prev_camera = player->get_camera();
////
////    Viewport viewport = player->get_viewport();
////    args_cam.screen_size = XMINT2(static_cast<int>(viewport.width),
////        static_cast<int>(viewport.height));
////    args_cam.border = this->_camera->
////        calculate_camera_scroll_border(
////            Vector2(viewport.width, viewport.height));
////    Vector3 new_cam = this->_camera->calculate_camera(args_cam);
////    //player->set_camera(new_cam);
////}