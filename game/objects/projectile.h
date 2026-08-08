#pragma once

#include "game/objects/player_team.h"
#include "game/objects/projectile_consts.h"
#include "engine/render/animation_object.h"
#include "engine/core/moving_object.h"
#include "game/objects/i_collision_game_object.h"

enum projectile_type
{
	SPRAY,
	JET,
	ROLLING,
	MIST,
	BALL
};

class Projectile : public artattack::MovingObject,
	public artattack::AnimationObject, public ICollisionGameObject
{
public:
	Projectile() = default;
	Projectile(const mattmath::Vector2F& velocity,
		PlayerTeam team,
		int player_num,
		const mattmath::Colour& team_colour,
		projectile_type type,
		artattack::RenderResources* render_resources,
		const ProjectileDetails& details,
		const mattmath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	void update(float dt) override = 0;
	void draw(DirectX::SpriteBatch* sprite_batch,
		const mattmath::Camera& camera) const override = 0;
	mattmath::RectangleF bounds() const override = 0;

	bool is_colliding(const ICollisionGameObject* other) const override = 0;
	void on_collision(const ICollisionGameObject* other) override;
	CollisionObjectType collision_object_type() const override;
	const mattmath::Shape* shape() const override = 0;
	bool for_deletion() const override;
	void set_for_deletion(bool for_deletion) override;

	float delete_timer() const;
	mattmath::Vector2F col_rect_size() const;
	float player_damage() const;
protected:
	const ProjectileDetails& details() const;

	PlayerTeam team() const;

	virtual bool is_matching_collision_object_type(const ICollisionGameObject* other) const;

	int player_num() const;
	const mattmath::Colour& team_colour() const;
	projectile_type type() const;

	float timer() const;
	void alter_timer(float time);

	virtual void update_movement(float gravity, float wind_resistance, float dt);
private:
	ProjectileDetails details_ = ProjectileDetails();
	float timer_ = 0.0f;
	int player_num_ = -1;
	mattmath::Colour team_colour_ = colour_consts::GRAY;
	projectile_type type_ = SPRAY;

	PlayerTeam team_ = PlayerTeam::none;
	bool for_deletion_ = false;

};

class DiffusingProjectile : public Projectile
{
public:
	DiffusingProjectile(const mattmath::Vector2F& velocity,
		PlayerTeam team,
		int player_num,
		const mattmath::Colour& team_colour,
		projectile_type type,
		artattack::RenderResources* render_resources,
		const ProjectileDetails& details,
		const DiffusionDetails& diffusion_details,
		const mattmath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);
	~DiffusingProjectile() override = default;


	void update(float dt) override = 0;
	void draw(DirectX::SpriteBatch* sprite_batch,
		const mattmath::Camera& camera) const override = 0;
	mattmath::RectangleF bounds() const override = 0;

	bool is_colliding(const ICollisionGameObject* other) const override = 0;
	const mattmath::Shape* shape() const override = 0;
protected:
	mattmath::Vector2F calculate_diffusion_size() const;
private:
	DiffusionDetails diffusion_details_ = DiffusionDetails();
	const mattmath::Vector2F& base_size() const;
};
