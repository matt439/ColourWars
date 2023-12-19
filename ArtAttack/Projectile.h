#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "player_team.h"
#include "projectile_consts.h"
//#include "Colour.h"
#include "AnimationObject.h"
#include "MovingObject.h"
#include "ICollisionGameObject.h"
//#include <cmath>

enum projectile_type
{
	SPRAY,
	JET,
	ROLLING,
	MIST,
	BALL
};

class Projectile : public MovingObject, //public CollisionObject,
	public AnimationObject, public ICollisionGameObject
{
private:
	projectile_details _details = projectile_details();
	float _timer = 0.0f;
	int _player_num = -1;
	MattMath::Colour _team_colour = colour_consts::GRAY;
	projectile_type _type = projectile_type::SPRAY;

	player_team _team = player_team::NONE;
	bool _for_deletion = false;

	const float* _dt = nullptr;

protected:
	//void set_details(const projectile_details& details) { this->_details = details; }
	const projectile_details& get_details() const { return this->_details; }
	//float get_time() const { return this->_time; }
	//DirectX::SimpleMath::Vector2 get_base_size() const { return this->_details.size; }


	player_team get_team() const;
	void set_for_deletion(bool for_deletion);

	virtual bool is_matching_collision_object_type(
		const ICollisionGameObject* other) const;

	int get_player_num() const;
	const MattMath::Colour& get_team_colour() const;
	projectile_type get_type() const;

	float get_timer() const;
	void alter_timer(float time);

	virtual void update_movement(float gravity, float wind_resistance);
	float get_dt() const;
public:
	Projectile() = default;
	Projectile(const MattMath::Vector2F& velocity,
		player_team team,
		int player_num,
		const MattMath::Colour& team_colour,
		projectile_type type,
		const float* dt,
		DirectX::SpriteBatch* sprite_batch,
		ResourceManager* resource_manager,
		const projectile_details& details,
		const MattMath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	virtual void update() = 0;
	virtual void draw(const MattMath::Camera& camera) = 0;
	virtual bool is_visible_in_viewport(const MattMath::RectangleF& view) const = 0;

	virtual bool is_colliding(const ICollisionGameObject* other) const = 0;
	virtual void on_collision(const ICollisionGameObject* other) override;
	virtual collision_object_type get_collision_object_type() const override;
	virtual const MattMath::Shape* get_shape() const = 0;
	virtual bool get_for_deletion() const override;



	//rotation_origin get_rotation_origin() const { return this->_details.rotat_origin; }
	//float get_gravity() const { return this->_details.gravity; }
	//float get_wind_resistance() const { return this->_details.wind_resistance; }
	float get_delete_timer() const { return this->_details.delete_timer; }
	MattMath::Vector2F get_col_rect_size() const { return this->_details.col_rect_size; }
	float get_player_damage() const { return this->_details.player_damage; }
	//float get_colour_power() const { return this->_details.colour_power; }

	//float get_rotation() const { return this->_rotation; }
	//void set_rotation(float rotation) { this->_rotation = rotation; }

	//bool get_for_deletion() const { return this->_for_deletion; }
	//void set_for_deletion(bool for_deletion) { this->_for_deletion = for_deletion; }



	//bool get_collided() const { return this->_collided; }
	//void set_collided(bool collided) { this->_collided = collided; }

	//virtual MattMath::RectangleF get_collision_rectangle() const;

};

//class Jet : public Projectile
//{
//private:
//
//public:
//	Jet(const projectile_info& info);
//};
//
//class Rolling : public Projectile
//{
//private:
//
//public:
//	Rolling(const projectile_info& info);
//};
//
//class Ball : public Projectile
//{
//private:
//
//public:
//	Ball(const projectile_info& info);
//};

class DiffusingProjectile : public Projectile
{
private:
	diffusion_details _diffusion_details = diffusion_details();
	//diffusion_type get_diffusion_type() const { return this->_diffusion_details.type; }
	//MattMath::Vector2F calculate_diffusion_col_rect_size() const;
	const MattMath::Vector2F& get_base_size() const;
protected:
	//void set_diffusion_details(const diffusion_details& details) { this->_diffusion_details = details; }
	MattMath::Vector2F calculate_diffusion_size() const;
public:
	DiffusingProjectile(const MattMath::Vector2F& velocity,
		player_team team,
		int player_num,
		const MattMath::Colour& team_colour,
		projectile_type type,
		const float* dt,
		DirectX::SpriteBatch* sprite_batch,
		ResourceManager* resource_manager,
		const projectile_details& details,
		const diffusion_details& diffusion_details,
		const MattMath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);
	virtual ~DiffusingProjectile() = default;


	virtual void update() = 0;
	virtual void draw(const MattMath::Camera& camera) = 0;
	virtual bool is_visible_in_viewport(const MattMath::RectangleF& view) const = 0;

	virtual bool is_colliding(const ICollisionGameObject* other) const = 0;
	virtual const MattMath::Shape* get_shape() const = 0;

	//DirectX::SimpleMath::Vector2 get_size() const;
	//MattMath::RectangleF get_collision_rectangle() const;
};

//class Spray : public DiffusingProjectile
//{
//private:
//
//public:
//	Spray(const projectile_info& info);
//};

//class Mist : public DiffusingProjectile
//{
//private:
//
//public:
//	Mist(const projectile_info& info);
//};

#endif // !PROJECTILE_H
