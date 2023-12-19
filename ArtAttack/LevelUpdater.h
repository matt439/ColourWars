#ifndef LEVELUPDATER_H
#define LEVELUPDATER_H

//#include "Updater.h"
//#include "Level.h"
//#include "Player.h"
//#include "PlayerUpdater.h"
//#include "TileUpdater.h"
//#include <memory>
//#include "PlayerInput.h"
//#include "PlayerCollision.h"
//#include "ProjectileCollision.h"
//#include "level_end_info.h"
//#include "camera.h"
//#include "MattMath.h"
//
//class LevelUpdater : public Updater
//{
//private:
//	std::unique_ptr<TileUpdater> _tile_updater = nullptr;
//	std::unique_ptr<PlayerUpdater> _player_updater = nullptr;
//	std::unique_ptr<PlayerCollision> _player_collision = nullptr;
//	std::unique_ptr<ProjectileCollision> _projectile_collision = nullptr;
//	std::unique_ptr<Camera> _camera = nullptr;
//
//	void update_camera(Player* player,
//		const MattMath::RectangleI& level_bounds);
//	//const float* _dt = nullptr;
//public:
//	LevelUpdater(const float* dt);
//	void update(Level* level, const std::vector<player_input>& inputs);
//	//void reset(Level* level);
//	level_end_info calculate_winner(const Level* level);
//};

#endif // !LEVELUPDATER_H
