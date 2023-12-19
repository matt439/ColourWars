#include "pch.h"
#include "WeaponDrawer.h"

//using namespace DirectX;
//using namespace DirectX::SimpleMath;
//
//WeaponDrawer::WeaponDrawer(DirectX::SpriteBatch* sprite_batch,
//    ResourceManager* resource_manager, const float* dt) :
//    Drawer(sprite_batch, resource_manager, dt)
//{
//    this->_projectile_drawer =
//        std::make_unique<ProjectileDrawer>(
//            sprite_batch, resource_manager, dt);
//}
//
//void WeaponDrawer::draw_weapon(const Weapon* weapon, const Vector3& camera)
//{
//    //draw weapon
//    Rectangle draw_rectangle =
//        this->calculate_draw_rectangle(weapon->get_sm_bounding_box(), camera);
//
//    //Vector2 draw_location = (weapon->get_draw_pos() - Vector2(camera.x, camera.y)) * camera.z;
//
//    Vector2 origin = this->calculate_sprite_origin(
//        weapon->get_size(), rotation_origin::LEFT_CENTER);
//
//        //Draw(this->get_texture(weapon->get_type()),
//        //draw_location, weapon->get_size() * camera.z, weapon->get_rotation(), rotation_origin::LEFT_CENTER,
//        //glm::vec3(1.0f, 1.0f, 1.0f), weapon->get_invert_x(), weapon->get_invert_y());
//
//    SpriteEffects effects = SpriteEffects::SpriteEffects_None;
//    bool invert_y = weapon->get_invert_y();
//    bool invert_x = weapon->get_invert_x();
//    if (invert_y && invert_x)
//    {
//		effects = SpriteEffects::SpriteEffects_FlipBoth;
//	}
//	else if (invert_y)
//	{
//		effects = SpriteEffects::SpriteEffects_FlipVertically;
//	}
//    else if (invert_x)
//    {
//        effects = SpriteEffects::SpriteEffects_FlipHorizontally;
//    }
//
//    sprite_frame_reference texture = this->get_texture(weapon->get_type());
//    SpriteSheet* sheet = this->get_resource_manager()->
//        get_sprite_sheet(texture.sprite_sheet_name);
//
//    sheet->draw(
//        this->get_sprite_batch(),
//        texture.sprite_frame_name,
//        draw_rectangle,
//        colour_consts::WHITE,
//        weapon->get_rotation(),
//        origin,
//        effects,
//        0.0f);
//
//    //this->get_sprite_batch()->Draw(
//    //    this->get_resource_manager()->get_texture(
//    //        this->get_texture(weapon->get_type())),
//    //    draw_rectangle,
//    //    nullptr,
//    //    Colors::White,
//    //    weapon->get_rotation(),
//    //    origin,
//    //    effects,
//    //    0.0f);
//
//    //draw nozzle
//    
//    //Vector2 noz_draw_location = (weapon->get_nozzle_pos() - Vector2(2.5f, 2.5f) - Vector2(camera.x, camera.y)) * camera.z;
//    //this->get_sprite_renderer()->DrawSprite(ResourceManager::get_texture("nozzle"),
//    //    noz_draw_location, Vector2(5.0f, 5.0f) * camera.z,
//    //    glm::vec3(1.0f, 1.0f, 1.0f));
//
//    Rectangle draw_rectangle_noz =
//        this->calculate_draw_rectangle(weapon->get_nozzle_rectangle(), camera);
//    Vector2 origin_noz = this->calculate_sprite_origin(
//		weapon->get_nozzle_size(), rotation_origin::CENTER);
//    //Vector2 origin_noz = weapon->get_nozzle_pos();
//
//    texture = sprite_frame_reference("sprite_sheet_1", "nozzle");
//    sheet = this->get_resource_manager()->
//		get_sprite_sheet(texture.sprite_sheet_name);
//
//    sheet->draw(
//		this->get_sprite_batch(),
//		texture.sprite_frame_name,
//		draw_rectangle_noz,
//		colour_consts::WHITE,
//		0.0f,
//		origin_noz,
//		SpriteEffects::SpriteEffects_None,
//		0.0f);
//
//    //this->get_sprite_batch()->Draw(
//    //    this->get_resource_manager()->get_texture("nozzle"),
//    //    draw_rectangle_noz,
//    //    nullptr,
//    //    Colors::White,
//    //    0.0f,
//    //    origin_noz);
//
//
//    //draw projectiles
//    for (int i = 0; i < weapon->get_num_projectiles(); i++)
//    {
//        this->_projectile_drawer->draw(weapon->get_projectile_const(i), camera);
//    }
//}
//
//
//sprite_frame_reference WeaponDrawer::get_texture(wep_type type)
//{
//    switch (type)
//    {
//    case wep_type::SPRAYER:
//        return sprite_frame_reference("sprite_sheet_1", "sprayer");
//    case wep_type::SNIPER:
//        return sprite_frame_reference("sprite_sheet_1", "sniper");
//    case wep_type::ROLLER:
//        return sprite_frame_reference("sprite_sheet_1", "roller");
//    case wep_type::MISTER:
//        return sprite_frame_reference("sprite_sheet_1", "mister");
//    case wep_type::BUCKET:
//        return sprite_frame_reference("sprite_sheet_1", "bucket");
//    default:
//        return sprite_frame_reference("sprite_sheet_1", "error");
//    }
//}