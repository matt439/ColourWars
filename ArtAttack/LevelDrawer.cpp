#include "pch.h"
#include "LevelDrawer.h"
//
//using namespace DirectX;
////using namespace DirectX::SimpleMath;
//using namespace MattMath;
//
//LevelDrawer::LevelDrawer(DirectX::SpriteBatch* sprite_batch,
//    ResourceManager* resource_manager,
//    const ResolutionManager* resolution_manager,
//    const float* dt, ViewportManager* viewport_manager,
//    ID3D11SamplerState* sampler_state) :
//    Drawer(sprite_batch, resource_manager, dt),
//    _resolution_manager(resolution_manager),
//    _viewport_manager(viewport_manager),
//    _sampler_state(sampler_state)
//{
//    this->_tile_drawer = std::make_unique<TileDrawer>(
//        sprite_batch, resource_manager, dt);
//    this->_player_drawer = std::make_unique<PlayerDrawer>(
//        sprite_batch, resource_manager, dt);
//    this->_interface_gameplay = std::make_unique<InterfaceGameplay>(
//        sprite_batch, resource_manager, dt);
//    this->_debug_text = std::make_unique<DebugText>(sprite_batch,
//        resource_manager, dt, resolution_manager);
//
//    this->_test_animation = std::make_unique<AnimatedSprite>(
//		this->get_resource_manager()->get_sprite_sheet("sprite_sheet_1"),
//		"test",
//		dt);
//}
//
//void LevelDrawer::draw(const Level* level)
//{
//    int player_count = level->get_players_size();
//    for (int i = 0; i < player_count; i++)
//    {
//        //viewport current_viewport = level->get_player_const(i)->get_viewport();
//        //glViewport(current_viewport.x, current_viewport.y,
//        //    current_viewport.width, current_viewport.height);
//
//        //glm::mat4 projection = level->get_player_const(i)->get_projection();
//        //ResourceManager::get_shader("sprite").SetMatrix4("projection", projection);
//        //ResourceManager::get_shader("text").SetMatrix4("projection", projection);
//
//        //this->get_sprite_batch()->SetViewport(
//        //    level->get_player_const(i)->get_viewport());
//
//        //D3D11_VIEWPORT viewport = static_cast<D3D11_VIEWPORT>
//        //    (Viewport(0.0f, 0.0f, 1920.0f, 540.0f));
//
//        //D3D11_VIEWPORT viewport = level->get_player_const(i)->get_viewport();
//        //this->get_sprite_batch()->SetViewport(viewport);
//        //auto context = this->_device_resources->GetD3DDeviceContext();
//        //context->RSSetViewports(1, &viewport);
//
//        this->_viewport_manager->apply_player_viewport(
//            level->get_player_const(i)->get_player_num());
//
//        this->get_sprite_batch()->Begin(SpriteSortMode_Deferred,
//            nullptr, this->_sampler_state);
//        
//        Vector3 camera = level->get_player_const(i)->get_camera();
//
//        //this->draw_background(level, camera);
//
//        IntRectangle tiles_to_draw = get_tiles_to_draw(
//            camera, level);
//
//        for (int j = tiles_to_draw.top_left_x;
//            j <= tiles_to_draw.bottom_right_x; j++)
//        {
//            for (int k = tiles_to_draw.top_left_y;
//                k <= tiles_to_draw.bottom_right_y; k++)
//            {
//                this->_tile_drawer->draw(level->get_tile_const(j, k), camera);
//            }
//        }
//        //draw other players
//        for (int j = 0; j < player_count; j++)
//        {
//            if (j != i)
//            {
//                this->_player_drawer->draw(level->get_player_const(j), camera);
//            }
//        }
//        //draw player (which includes weapon & projectiles)
//        this->_player_drawer->draw(level->get_player_const(i), camera);
//
//        // draw interface
//        this->draw_interface(level, i);
//
//        if (level->get_player_const(i)->get_showing_debug())
//        {
//            //this->draw_debug_info(level, i);
//            this->_debug_text->draw_debug_info(level, i);
//        }
//
//        this->draw_test_animation();
//
//        this->get_sprite_batch()->End();
//    }
//
//    // test split rendering
//    // 
//    //this->_viewport_manager->apply_player_viewport(0);
//    //this->get_sprite_batch()->Begin();
//    //this->get_sprite_batch()->Draw(
//    //    this->get_resource_manager()->get_texture("split_render_test"),
//    //    Vector2(710.0f, -250.0f));
//    //this->get_sprite_batch()->End();
//    //
//    //this->_viewport_manager->apply_player_viewport(1);
//    //this->get_sprite_batch()->Begin();
//    //this->get_sprite_batch()->Draw(
//    //    this->get_resource_manager()->get_texture("split_render_test"),
//    //    Vector2(710.0f, 290.0f));
//    //this->get_sprite_batch()->End();
//}
//
//void LevelDrawer::draw_test_animation()
//{
//    this->_test_animation->update();
//    this->_test_animation->draw(this->get_sprite_batch(), Vector2(1000.0f, 300.0f));
//}
//
//void LevelDrawer::draw_interface(const Level* level, int player_num)
//{
//    interface_draw args;
//    args.resolution = this->_resolution_manager->get_resolution_ivec();
//    args.timer = level->get_timer();
//
//    const Player* player = level->get_player_const(player_num);
//    args.ammo = player->get_primary_const()->get_ammo();
//    args.health = player->get_health();
//    args.team = player->get_team();
//    args.team_colour = player->get_team_colour();
//    
//    this->_interface_gameplay->draw_gameplay_interface(args);
//}
//
//
//
//std::string LevelDrawer::get_background_texture(level_stage stage)
//{
//    switch (stage)
//    {
//    case level_stage::TEST_1:
//		return "level_1";
//    default:
//		return "level_1";
//    }
//}
//
//RectangleI LevelDrawer::get_tiles_to_draw(
//    const DirectX::SimpleMath::Vector3& camera,
//    const Level* level)
//{
//    float scale = camera.z;
//    XMINT2 resolution = this->_resolution_manager->get_resolution_ivec();
//
//    Rectangle rect = Rectangle(
//        static_cast<long>(camera.x),
//        static_cast<long>(camera.y),
//        static_cast<long>(resolution.x / scale),
//        static_cast<long>(resolution.y / scale));
//    float tile_width = tile_consts::TILE_WIDTH;
//    float tile_height = tile_consts::TILE_HEIGHT;
//
//    int tile_width_num = level->get_tiles_width();
//    int tile_height_num = level->get_tiles_height();
//
//    IntRectangle result;
//    result.top_left_x = static_cast<int>(floor(
//        (rect.x - tile_width) / tile_width));
//    if (result.top_left_x < 0)
//    {
//        result.top_left_x = 0;
//    }
//    result.top_left_y = static_cast<int>(floor(
//        (rect.y - tile_height) / tile_height));
//    if (result.top_left_y < 0)
//    {
//        result.top_left_y = 0;
//    }
//    result.bottom_right_x = static_cast<int>(floor(
//        (rect.x + rect.width + tile_width) / tile_width));
//    if (result.bottom_right_x > static_cast<int>(tile_width_num) - 1)
//    {
//        result.bottom_right_x = static_cast<int>(tile_width_num) - 1;
//    }
//    result.bottom_right_y = static_cast<int>(floor(
//        (rect.y + rect.height + tile_height) / tile_height));
//    if (result.bottom_right_y > static_cast<int>(tile_height_num) - 1)
//    {
//        result.bottom_right_y = static_cast<int>(tile_height_num) - 1;
//    }
//    return result;
//}
//
////void LevelDrawer::draw_background(const Level* level,
////    const DirectX::SimpleMath::Vector3& camera)
////{
////    Vector2 draw_location = (Vector2(0.0f, 0.0f) -
////        Vector2(camera.x, camera.y)) * camera.z;
////
////    int horiz_tile_num = level->get_tiles_width();
////    int vert_tile_num = level->get_tiles_height();
////    
////    Vector2 level_size = Vector2(horiz_tile_num * tile_consts::TILE_WIDTH,
////        vert_tile_num * tile_consts::TILE_HEIGHT);
////
////    Rectangle draw_rect = Rectangle(
////        static_cast<long>(draw_location.x),
////        static_cast<long>(draw_location.y),
////        static_cast<long>(level_size.x * camera.z),
////        static_cast<long>(level_size.y * camera.z));
////
////    //this->_sprite_renderer->DrawSprite(this->get_background_texture(level->get_stage()),
////    //    draw_location,
////    //    level_size * camera.z, glm::vec3(1.0f, 1.0f, 1.0f));
////
////    this->get_sprite_batch()->Draw(
////        this->get_resource_manager()->get_texture(
////            this->get_background_texture(level->get_stage())),
////        draw_rect);
////}