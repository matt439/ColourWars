#include "pch.h"
#include "ProjectileDrawer.h"
//
//using namespace DirectX;
//using namespace DirectX::SimpleMath;
//
//void ProjectileDrawer::draw(const Projectile& proj, Vector3 camera)
//{
//    Rectangle draw_rectangle =
//        this->calculate_draw_rectangle(proj.get_sm_bounding_box(), camera);
//    Color colour = proj.get_team_colour();
//    float rotation = proj.get_rotation();
//    Vector2 origin = this->calculate_sprite_origin(
//        proj.get_size(), proj.get_rotation_origin());
//
//    sprite_frame_reference texture = this->get_texture(proj.get_type());
//    SpriteSheet* sprite_sheet = this->get_resource_manager()->get_sprite_sheet(
//		texture.sprite_sheet_name);
//
//    sprite_sheet->draw(
//        this->get_sprite_batch(),
//        texture.sprite_frame_name,
//		draw_rectangle,
//		colour,
//		rotation,
//		origin,
//		SpriteEffects_None,
//		0.0f);
//}
//
//sprite_frame_reference ProjectileDrawer::get_texture(projectile_type type)
//{
//    switch (type)
//    {
//    case projectile_type::SPRAY:
//        return sprite_frame_reference("sprite_sheet_1", "paint");
//    case projectile_type::JET:
//        return sprite_frame_reference("sprite_sheet_1", "paint");
//    case projectile_type::ROLLING:
//        return sprite_frame_reference("sprite_sheet_1", "paint");
//    case projectile_type::MIST:
//        return sprite_frame_reference("sprite_sheet_1", "paint");
//    case projectile_type::BALL:
//        return sprite_frame_reference("sprite_sheet_1", "paint");
//    default:
//        return sprite_frame_reference("sprite_sheet_1", "paint");
//    }
//}