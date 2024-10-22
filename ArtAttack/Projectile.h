#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "player_team.h"
#include "projectile_consts.h"
#include "AnimationObject.h"
#include "MovingObject.h"
#include "ICollisionGameObject.h"

enum projectile_type
{
	SPRAY,
	JET,
	ROLLING,
	MIST,
	BALL
};

class Projectile : public MovingObject,
	public AnimationObject, public ICollisionGameObject
{
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
		const ProjectileDetails& details,
		const MattMath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	void update() override = 0;
	void draw(const MattMath::Camera& camera) override = 0;
	bool is_visible_in_viewport(const MattMath::RectangleF& view) const override = 0;

	bool is_colliding(const ICollisionGameObject* other) const override = 0;
	void on_collision(const ICollisionGameObject* other) override;
	collision_object_type get_collision_object_type() const override;
	const MattMath::Shape* get_shape() const override = 0;
	bool get_for_deletion() const override;

	float get_delete_timer() const { return this->_details.delete_timer; }
	MattMath::Vector2F get_col_rect_size() const { return this->_details.col_rect_size; }
	float get_player_damage() const { return this->_details.player_damage; }
protected:
	const ProjectileDetails& get_details() const { return this->_details; }

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
private:
	ProjectileDetails _details = ProjectileDetails();
	float _timer = 0.0f;
	int _player_num = -1;
	MattMath::Colour _team_colour = colour_consts::GRAY;
	projectile_type _type = SPRAY;

	player_team _team = player_team::NONE;
	bool _for_deletion = false;

	const float* _dt = nullptr;
};

class DiffusingProjectile : public Projectile
{
public:
	DiffusingProjectile(const MattMath::Vector2F& velocity,
		player_team team,
		int player_num,
		const MattMath::Colour& team_colour,
		projectile_type type,
		const float* dt,
		DirectX::SpriteBatch* sprite_batch,
		ResourceManager* resource_manager,
		const ProjectileDetails& details,
		const DiffusionDetails& diffusion_details,
		const MattMath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);
	~DiffusingProjectile() override = default;


	void update() override = 0;
	void draw(const MattMath::Camera& camera) override = 0;
	bool is_visible_in_viewport(const MattMath::RectangleF& view) const override = 0;

	bool is_colliding(const ICollisionGameObject* other) const override = 0;
	const MattMath::Shape* get_shape() const override = 0;
protected:
	MattMath::Vector2F calculate_diffusion_size() const;
private:
	DiffusionDetails _diffusion_details = DiffusionDetails();
	const MattMath::Vector2F& get_base_size() const;
};

#endif // !PROJECTILE_H
