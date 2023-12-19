#ifndef LEVEL_H
#define LEVEL_H

#include "ICollisionGameObject.h"
#include "TeamColour.h"
#include "level_stage.h"
//#include "border_thickness.h"
#include "PlayerInput.h"
#include "Player.h"
#include "ResolutionManager.h"
#include "ViewportManager.h"
#include "DebugText.h"
#include "CameraTools.h"
#include "InterfaceGameplay.h"
#include "level_end_info.h"
#include "IPaintableGameObject.h"
#include "TextDropShadow.h"
#include "SoundBank.h"

enum class level_state
{
	//START_CONFIRMATION,
	START_COUNTDOWN,
	ACTIVE,
	//PAUSED,
	//RESTARTING,
	ZOOM_OUT,
	OVERVIEW,
	FINISHED,
	//END_SCREEN,
	//DELETABLE
};

namespace level_consts
{
	constexpr float TIMER = 60.0f;
	constexpr float START_TIMER = 3.0f;
	const static std::string COUNTDOWN_TEXT = "3";
	const static std::string COUNTDOWN_FONT_NAME = "gill_sans_mt_bold_144";
	const static MattMath::Colour COUNTDOWN_COLOUR = colour_consts::DARK_GRAY;
	const static MattMath::Colour COUNTDOWN_SHADOW_COLOUR = colour_consts::BLACK;
	constexpr float COUNTDOWN_SCALE = 2.0f;
	//constexpr float COUNTDOWN_SHADOW_SCALE = 2.0f;
	const static MattMath::Vector2F COUNTDOWN_SHADOW_OFFSET = { 5.0f, 5.0f };
	constexpr float COUNTDOWN_TEXT_WIDTH = 400.0f;
	constexpr float COUNTDOWN_TEXT_HEIGHT = 600.0f;

	constexpr float ZOOM_OUT_TIMER = 2.0f;
	constexpr float OVERVIEW_TIMER = 2.0f;

	//const std::string SOUND_BANK = "sound_bank_1";
	//const std::string MUSIC = "dragostea_din_tei"; //"level_music_1";
	//constexpr float MUSIC_VOLUME = 0.3f;
	const std::string COUNTDOWN_SOUND = "smash_countdown";
	constexpr float COUNTDOWN_SOUND_VOLUME = 1.0f;

	const std::string ZOOM_OUT_SOUND = "slide_whistle_up";
	constexpr float ZOOM_OUT_SOUND_VOLUME = 0.5f;
}

class Level
{
private:
	std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>>
		_non_collision_objects = nullptr;
	std::unique_ptr<std::vector<std::unique_ptr<ICollisionGameObject>>>
		_collision_objects = nullptr;
	std::unique_ptr<std::vector<std::unique_ptr<Player>>>
		_player_objects = nullptr;
	std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>>
		_viewport_dividers = nullptr;

	SoundBank* _sound_bank = nullptr;

	std::string _music_name = "";
	float _music_volume = 0.0f;

	std::unique_ptr<TextDropShadow> _countdown_text = nullptr;

	std::unique_ptr<CameraTools> _camera_tools = nullptr;
	std::unique_ptr<InterfaceGameplay> _interface_gameplay = nullptr;

	std::string _level_name = "";
	const ResolutionManager* _resolution_manager = nullptr;
	ViewportManager* _viewport_manager = nullptr;
	ResourceManager* _resource_manager = nullptr;

	float _timer = level_consts::TIMER;
	float _start_timer = level_consts::START_TIMER;
	float _zoom_out_timer = level_consts::ZOOM_OUT_TIMER;
	float _overview_timer = level_consts::OVERVIEW_TIMER;

	MattMath::Camera _zoom_out_camera = MattMath::Camera::DEFAULT_CAMERA;

	team_colour _team_colours = team_colour();
	level_stage _stage = level_stage::TEST_1;

	MattMath::RectangleF _out_of_bounds = MattMath::RectangleF::ZERO;
	MattMath::RectangleF _camera_bounds = MattMath::RectangleF::ZERO;
	MattMath::RectangleF _zoom_out_start_bounds = MattMath::RectangleF::ZERO;
	MattMath::RectangleF _zoom_out_finish_bounds = MattMath::RectangleF::ZERO;

	std::vector<MattMath::Vector2F> _team_a_spawns = std::vector<MattMath::Vector2F>();
	std::vector<MattMath::Vector2F> _team_b_spawns = std::vector<MattMath::Vector2F>();

	level_state _state = level_state::START_COUNTDOWN; 
	const float* _dt = nullptr;
	DirectX::SpriteBatch* _sprite_batch = nullptr;
	ID3D11SamplerState* _sampler_state = nullptr;

	std::vector<player_input> _player_inputs = std::vector<player_input>();
	std::unique_ptr<DebugText> _debug_text = nullptr;

	int count_projectiles() const;
	float get_dt() const;
	bool is_object_out_of_bounds(const ICollisionGameObject* object) const;
	void draw_end_screen() const;

	void update_level_logic(const std::vector<player_input>& player_inputs);

	void draw_active_level();
	void draw_zoom_out_level();

	float zoom_out_camera_ratio();
public:
	Level() = default;
	Level(std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>> non_collision_objects,
		std::unique_ptr<std::vector<std::unique_ptr<ICollisionGameObject>>> collision_objects,
		std::unique_ptr<std::vector<std::unique_ptr<Player>>> player_objects,
		std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>> viewport_dividers,
		level_stage stage,
		const team_colour& team_colours,
		const MattMath::RectangleF& out_of_bounds,
		const MattMath::RectangleF& camera_bounds,
		const MattMath::RectangleF& zoom_out_start_bounds,
		const MattMath::RectangleF& zoom_out_finish_bounds,
		const std::vector<MattMath::Vector2F>& team_a_spawns,
		const std::vector<MattMath::Vector2F>& team_b_spawns,
		const std::string& sound_bank_name,
		const std::string& music_name,
		float music_volume,
		const float* dt,
		DirectX::SpriteBatch* sprite_batch,
		ID3D11SamplerState* sampler_state,
		const std::string& level_name,
		const ResolutionManager* resolution_manager,
		ViewportManager* viewport_manager,
		ResourceManager* resource_manager);
	//~Level();

	void update(const std::vector<player_input>& player_inputs);
	void draw();

	level_state get_state() const;
	void set_state(level_state state);

	level_end_info get_level_end_info() const;

	void stop_music() const;

	//void set_player_inputs(const std::vector<player_input>& player_inputs);
};


//#include <vector>
//#include <memory>
//#include "Tile.h"
//#include "Player.h"
//#include "TeamColour.h"
//#include "level_stage.h"
//#include "tile_consts.h"
//#include "IPhysical.h"
////#include "ICollidable.h"
//#include "MattMath.h"
//#include "ICollidableWithOthers.h"
//
////#include "Projectile.h"
//
//struct level_info
//{
//	std::shared_ptr<std::vector<ICollidable>> collidable_objects = nullptr;
//	std::shared_ptr<std::vector<ICollidableWithOthers>> colliding_objects = nullptr;
//	std::shared_ptr<std::vector<IPhysical>>
//		non_colliding_objects = nullptr;
//	std::shared_ptr<std::vector<std::vector<Tile>>> tiles = nullptr;
//	std::shared_ptr<std::vector<Player>> players = nullptr;
//	float timer = 0.0f;
//	team_colour team_colours = team_colour();
//	level_stage stage = level_stage::TEST_1;
//	MattMath::RectangleI camera_boundary_border = { 0, 0, 0, 0 };
//};
//
//enum class level_state
//{
//	START_CONFIRMATION,
//	ACTIVE,
//	PAUSED,
//	RESTARTING,
//	TIMER_FINISHED,
//	END_SCREEN,
//	DELETABLE
//};
//
//namespace level_consts
//{
//	constexpr float TIMER = 60.0f;
//}
//
//class Level
//{
//private:
//	//level_info _info = level_info();
//	level_state _state = level_state::ACTIVE;
//	//float _timer = level_consts::TIMER;
//
//	std::shared_ptr<std::vector<ICollidable>>
//		_colliding_objects = nullptr;
//	std::shared_ptr<std::vector<IPhysical>>
//		_non_colliding_objects = nullptr;
//
//	std::shared_ptr<std::vector<std::vector<Tile>>> _tiles = nullptr;
//	std::shared_ptr<std::vector<Player>> _players = nullptr;
//	float _timer = 0.0f;
//	team_colour _team_colours = team_colour();
//	level_stage _stage = level_stage::TEST_1;
//	RECT _camera_boundary_border = { 0, 0, 0, 0 };
//public:
//	Level(const level_info& info) :
//		_colliding_objects(info.colliding_objects),
//		_non_colliding_objects(info.non_colliding_objects),
//		_tiles(info.tiles),
//		_players(info.players),
//		_timer(info.timer),
//		_team_colours(info.team_colours),
//		_stage(info.stage),
//		_camera_boundary_border(info.camera_boundary_border) {}
//	
//	std::vector<std::vector<Tile>>* get_tiles() { return this->_tiles.get(); }
//	const std::vector<std::vector<Tile>>* get_tiles_const() const { return this->_tiles.get(); }
//	Tile* get_tile(int x, int y) { return &(*this->_tiles)[y][x]; }
//	const Tile* get_tile_const(int x, int y) const { return &(*this->_tiles)[y][x]; }
//	int get_tiles_height() const { return static_cast<int>(this->_tiles->size()); }
//	int get_tiles_width() const { return static_cast<int>((*this->_tiles)[0].size()); }
//
//	MattMath::RectangleI get_level_bounding_rectangle() const;
//	MattMath::RectangleI get_camera_bounding_rectangle() const;
//
//	//std::vector<std::vector<std::shared_ptr<Tile>>> get_tiles() { return this->_info.tiles; }
//	//std::vector<std::vector<std::shared_ptr<Tile>>> get_tiles_const() const { return this->_info.tiles; }
//	//Tile* get_tile(int x, int y)& { return this->_info.tiles[y][x].get(); }
//	//const Tile* get_tile_const(int x, int y)& { return this->_info.tiles[y][x].get(); }
//	//int get_tiles_height() const { return static_cast<int>(this->_info.tiles.size()); }
//	//int get_tiles_width() const { return static_cast<int>(this->_info.tiles[0].size()); }
//
//	std::vector<Player>* get_players() { return this->_players.get(); }
//	const std::vector<Player>* get_players_const() const { return this->_players.get(); }
//	Player* get_player(int index) { return &(*this->_players)[index]; }
//	const Player* get_player_const(int index) const { return &(*this->_players)[index]; }
//	int get_players_size() const { return static_cast<int>(this->_players->size()); }
//
//	//std::vector<std::shared_ptr<Player>> get_players() { return this->_info.players; }
//	//std::vector<std::shared_ptr<Player>> get_players_const() const { return this->_info.players; }
//	//Player* get_player(int index) { return this->_info.players[index].get(); }
//	//const Player* get_player_const(int index) const { return this->_info.players[index].get(); }
//	//int get_players_size() const { return static_cast<int>(this->_info.players.size()); }
//
//	team_colour get_team_colour() const { return this->_team_colours; }
//	level_stage get_stage() const { return this->_stage; }
//
//	level_state get_state() const { return this->_state; }
//	void set_state(level_state state) { this->_state = state; }
//	void set_timer(float timer) { this->_timer = timer; }
//	float get_timer() const { return this->_timer; }
//	void increment_timer(float dt) { this->_timer += dt; }
//
//	//const RECT& get_camera_boundary_border() const { return this->_camera_boundary_border; }
//};

#endif
