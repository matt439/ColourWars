#ifndef TILE_H
#define TILE_H

//#include "tile_type.h"
//#include "spray_state.h"
//#include "player_team.h"
//#include "colour.h"
//#include "tile_paint_info.h"
//#include "TeamColour.h"
////#include "SimpleMath.h"
////#include "PhysicalObject.h"
////#include "CollidingObject.h"
//#include "RectangleColliding.h"
//
//#include "MattMath.h"
//
//struct tile_info
//{
//	//DirectX::SimpleMath::Vector2 size = DirectX::SimpleMath::Vector2(-1.0f, -1.0f);
//	//DirectX::SimpleMath::Vector2 position = DirectX::SimpleMath::Vector2(-1.0f, -1.0f);
//	//DirectX::XMINT2 coordinates = DirectX::XMINT2(-1, -1);
//
//	MattMath::RectangleF rectangle = MattMath::RectangleF::ZERO;
//	MattMath::RectangleF collision_rectangle = MattMath::RectangleF::ZERO;
//	std::vector<collidable_object_type> collidable_object_types;
//	collidable_object_shape shape = collidable_object_shape::NONE;
//	MattMath::Colour colour = colour_consts::WHITE;
//	tile_type back_type = tile_type::EMPTY;
//	team_colour team_colours = team_colour();
//};
//
//class Tile : public RectangleColliding
//{
//private:
//	//tile_info _info = tile_info();
//	tile_type _front_type = tile_type::EMPTY;
//	spray_state _sprayed = spray_state::NOT_SPRAYED;
//	player_team _sprayed_by_team = player_team::NONE;
//	float _colour_change = 0.0f;
//	float _timer = 0.0f;
//	float _colour_power = 0.0f;
//
//	bool _reverting = false;
//	bool _timer_on = false;
//	player_team _base_paint_team = player_team::NONE;
//
//	DirectX::XMINT2 _coordinates = DirectX::XMINT2(-1, -1);
//	tile_type _back_type = tile_type::EMPTY;
//	team_colour _team_colours = team_colour();
//	
//public:
//	Tile(const tile_info& info) :
//		PhysicalObject(info.position, info.size),
//		_coordinates(info.coordinates),
//		_back_type(info.back_type),
//		_team_colours(info.team_colours) {}
//
//	//DirectX::SimpleMath::Vector2 get_size() const { return this->_info.size; }
//	//void set_size(const DirectX::SimpleMath::Vector2& size) { this->_info.size = size; }
//	//DirectX::SimpleMath::Vector2 get_position() const { return this->_info.position; }
//	//void set_position(const DirectX::SimpleMath::Vector2& position) { this->_info.position = position; }
//	DirectX::XMINT2 get_coordinates() const { return this->_coordinates; }
//	void set_coordinates(const DirectX::XMINT2& coordinates) { this->_coordinates = coordinates; }
//	tile_type get_back_type() const { return this->_back_type; }
//	void set_back_type(tile_type type) { this->_back_type = type; }
//	tile_type get_front_type() const { return this->_front_type; }
//	void set_front_type(tile_type type) { this->_front_type = type; }
//	spray_state get_sprayed() const { return this->_sprayed; }
//	void set_sprayed(spray_state sprayed) { this->_sprayed = sprayed; }
//	player_team get_sprayed_by_team() const { return this->_sprayed_by_team; }
//	void set_sprayed_by_team(player_team team) { this->_sprayed_by_team = team; }
//	float get_colour_change() const { return this->_colour_change; }
//	void set_colour_change(float colour_change) { this->_colour_change = colour_change; }
//	float get_timer() const { return this->_timer; }
//	void set_timer(float timer) { this->_timer = timer; }
//	float get_colour_power() const { return this->_colour_power; }
//	void set_colour_power(float colour_power) { this->_colour_power = colour_power; }
//	team_colour get_team_colour() const { return this->_team_colours; }
//	void set_team_colour(team_colour team_colour) { this->_team_colours = team_colour; }
//	bool get_reverting() const { return this->_reverting; }
//	void set_reverting(bool reverting) { this->_reverting = reverting; }
//	bool get_timer_on() const { return this->_timer_on; }
//	void set_timer_on(bool timer_on) { this->_timer_on = timer_on; }
//	player_team get_base_paint_team() const { return this->_base_paint_team; }
//	void set_base_paint_team(player_team base_paint_team) { this->_base_paint_team = base_paint_team; }
//	
//	DirectX::SimpleMath::Color get_team_colour(player_team team) const;
//	//DirectX::SimpleMath::Rectangle get_rectangle() const;
//	tile_paint_info get_paint_info() const;
//};

#endif