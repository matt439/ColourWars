#ifndef LEVELDRAWER_H
#define LEVELDRAWER_H

//#include "Level.h"
//#include "IntRectangle.h"
//#include "tile_consts.h"
//#include <vector>
//#include <memory>
//#include "ResolutionManager.h"
//#include "tile_consts.h"
//#include "TileDrawer.h"
//#include "PlayerDrawer.h"
//#include "level_stage.h"
//#include "InterfaceGameplay.h"
//#include "Drawer.h"
////#include "SimpleMath.h"
////#include "DeviceResources.h"
//#include "ViewportManager.h"
//#include "AnimatedSprite.h"
//#include "DebugText.h"
//#include "MattMath.h"
//
//
//
//
//class LevelDrawer : public Drawer
//{
//private:
//	const ResolutionManager* _resolution_manager = nullptr;
//	//DX::DeviceResources* _device_resources = nullptr;
//	ViewportManager* _viewport_manager = nullptr;
//
//	std::unique_ptr<TileDrawer> _tile_drawer = nullptr;
//	std::unique_ptr<PlayerDrawer> _player_drawer = nullptr;
//	std::unique_ptr<InterfaceGameplay> _interface_gameplay = nullptr;
//	std::unique_ptr<DebugText> _debug_text = nullptr;
//	ID3D11SamplerState* _sampler_state = nullptr;
//
//	MattMath::RectangleI get_tiles_to_draw(const MattMath::Vector3F& camera,
//		const Level* level);
//	//void draw_background(const Level* level,
//	//	const DirectX::SimpleMath::Vector3& camera);
//	void draw_interface(const Level* level, int player_num);
//
//	std::string get_background_texture(level_stage stage);
//	
//
//	std::unique_ptr<AnimatedSprite> _test_animation = nullptr;
//	void draw_test_animation();
//
//public:
//	LevelDrawer(DirectX::SpriteBatch* sprite_batch,
//		ResourceManager* resource_manager,
//		const ResolutionManager* resolution_manager,
//		const float* dt,
//		ViewportManager* viewport_manager,
//		ID3D11SamplerState* sampler_state);
//
//	void draw(const Level* level);
//};

#endif // !LEVELDRAWER_H
