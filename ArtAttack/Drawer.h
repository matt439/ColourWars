#ifndef DRAWER_H
#define DRAWER_H

#include "SpriteBatch.h"
#include "ResourceManager.h"
#include "rotation_origin.h"
#include "MattMath.h"

class Drawer
{
public:
	Drawer(DirectX::SpriteBatch* sprite_batch,
		ResourceManager* resource_manager,
		const float* dt);

protected:
	DirectX::SpriteBatch* get_sprite_batch() const;
	ResourceManager* get_resource_manager() const;
	float get_dt() const;
	static MattMath::RectangleI calculate_draw_rectangle(
		const MattMath::RectangleI& rec,
		const MattMath::Vector3F& camera);
	static MattMath::RectangleI calculate_draw_rectangle(
		const MattMath::Vector2F& position,
		const MattMath::Vector2F& size,
		const MattMath::Vector3F& camera);
	static MattMath::Vector2F calculate_sprite_origin(
		const MattMath::Vector2F& size,
		rotation_origin origin);

private:
	DirectX::SpriteBatch* _sprite_batch = nullptr;
	ResourceManager* _resource_manager = nullptr;
	const float* _dt = nullptr;
};

#endif // !DRAWER_H
