#ifndef DRAWER_H
#define DRAWER_H

#include "SpriteBatch.h"
#include "ResourceManager.h"
#include "rotation_origin.h"
#include "MattMath.h"

class Drawer
{
private:
	DirectX::SpriteBatch* _sprite_batch = nullptr;
	ResourceManager* _resource_manager = nullptr;
	const float* _dt = nullptr;
protected:
	DirectX::SpriteBatch* get_sprite_batch() { return this->_sprite_batch; }
	ResourceManager* get_resource_manager() { return this->_resource_manager; }
	float get_dt() { return *this->_dt; }
	MattMath::RectangleI calculate_draw_rectangle(
		const MattMath::RectangleI& rec,
		const MattMath::Vector3F& camera);
	MattMath::RectangleI calculate_draw_rectangle(
		const MattMath::Vector2F& position,
		const MattMath::Vector2F& size,
		const MattMath::Vector3F& camera);
	MattMath::Vector2F calculate_sprite_origin(
		const MattMath::Vector2F& size,
		rotation_origin origin);
public:
	Drawer(DirectX::SpriteBatch* sprite_batch,
		ResourceManager* resource_manager,
		const float* dt) :
		_sprite_batch(sprite_batch),
		_resource_manager(resource_manager),
		_dt(dt) {}
};

#endif // !DRAWER_H
