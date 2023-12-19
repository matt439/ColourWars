#include "pch.h"
#include "LevelUpdater.h"

//using namespace DirectX;
////using namespace DirectX::SimpleMath;
//using namespace MattMath;
//
//LevelUpdater::LevelUpdater(const float* dt) : Updater(dt)
//{
//	this->_tile_updater = std::make_unique<TileUpdater>(dt);
//	this->_player_updater = std::make_unique<PlayerUpdater>(dt);
//	this->_player_collision = std::make_unique<PlayerCollision>();
//	this->_projectile_collision = std::make_unique<ProjectileCollision>();
//	this->_camera = std::make_unique<Camera>();
//}
//
//void LevelUpdater::update(Level* level, const std::vector<player_input>& inputs)
//{
//	if (level->get_state() == level_state::ACTIVE)
//	{
//		//update tiles
//		for (int i = 0; i < level->get_tiles_height(); i++) //y
//		{
//			for (int j = 0; j < level->get_tiles_width(); j++) //x
//			{
//				this->_tile_updater->update(level->get_tile(j, i));
//			}
//		}
//
//		// get player inputs
//		std::vector<player_input> player_inputs = inputs;
//		//std::vector<player_input> player_inputs =
//		//	this->_player_input->update_and_get_menu_inputs();
//
//		RectangleI camera_bounds = level->get_camera_bounding_rectangle();
//
//		//update players
//		for (int i = 0; i < level->get_players_size(); i++)
//		{
//			// set player input
//			level->get_player(i)->set_input(player_inputs[i]);
//			// update player
//			this->_player_updater->update(level->get_player(i));
//
//			this->update_camera(level->get_player(i), camera_bounds);
//
//			//update weapons
//			//update projectiles
//		}
//
//		//update all player collisions
//		this->_player_collision->update_all_player_collision(level);
//
//		//update all proj collisions
//		this->_projectile_collision->update_all_projectile_collision(level);
//
//		level->increment_timer(-this->get_dt());
//	}
//	if (level->get_timer() <= 0.0f)
//	{
//		level->set_state(level_state::TIMER_FINISHED);
//	}
//}
//
//void LevelUpdater::update_camera(Player* player,
//	const RectangleI& camera_bounds)
//{
//	calculate_cam args_cam;
//	args_cam.player_center = player->get_center();
//	args_cam.prev_camera = player->get_camera();
//
//	Viewport viewport = player->get_viewport();
//
//	//args_cam.screen_size = XMINT2(static_cast<int>(viewport.width),
//	//	static_cast<int>(viewport.height));
//	//args_cam.border = this->_camera->
//	//	calculate_camera_scroll_border(
//	//		Vector2(viewport.width, viewport.height));
//	args_cam.viewport_size = Vector2F(viewport.width, viewport.height);
//	args_cam.camera_bounds = camera_bounds;
//
//
//	Vector3F new_cam = this->_camera->calculate_camera(args_cam);
//	player->set_camera(new_cam);
//}
//
//level_end_info LevelUpdater::calculate_winner(const Level* level)
//{
//	level_end_info result;
//	float team_a = 0.0f;
//	float team_b = 0.0f;
//	for (int i = 0; i < level->get_tiles_height(); i++) //y
//	{
//		for (int j = 0; j < level->get_tiles_width(); j++) //x
//		{
//			const Tile* tile = level->get_tile_const(j, i);
//			tile_paint_info info = tile->get_paint_info();
//			if (info.team == player_team::A)
//			{
//				team_a += info.strength;
//			}
//			else if (info.team == player_team::B)
//			{
//				team_b += info.strength;
//			}
//		}
//	}
//	if (team_a > team_b)
//	{
//		result.winning_team = player_team::A;
//		result.winning_score = team_a;
//	}
//	else
//	{
//		result.winning_team = player_team::B;
//		result.winning_score = team_b;
//	}
//	result.team_colours = level->get_team_colour();
//	return result;
//}

//void LevelUpdater::reset(Level* level)
//{
//
//}