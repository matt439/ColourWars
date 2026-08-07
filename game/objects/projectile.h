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
		RenderResources* render_resources,
		const ProjectileDetails& details,
		const MattMath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	void update() override = 0;
	void draw(DirectX::SpriteBatch* sprite_batch,
		const MattMath::Camera& camera) override = 0;
	bool is_visible_in_viewport(const MattMath::RectangleF& view) const override = 0;

	bool is_colliding(const ICollisionGameObject* other) const override = 0;
	void on_collision(const ICollisionGameObject* other) override;
	collision_object_type get_collision_object_type() const override;
	const MattMath::Shape* get_shape() const override = 0;
	bool get_for_deletion() const override;
	void set_for_deletion(bool for_deletion) override;

	float get_delete_timer() const;
	MattMath::Vector2F get_col_rect_size() const;
	float get_player_damage() const;
protected:
	const ProjectileDetails& get_details() const;

	player_team get_team() const;

	virtual bool is_matching_collision_object_type(const ICollisionGameObject* other) const;

	int get_player_num() const;
	const MattMath::Colour& get_team_colour() const;
	projectile_type get_type() const;

	float get_timer() const;
	void alter_timer(float time);

	virtual void update_movement(float gravity, float wind_resistance);
	float get_dt() const;
private:
	ProjectileDetails details_ = ProjectileDetails();
	float timer_ = 0.0f;
	int player_num_ = -1;
	MattMath::Colour team_colour_ = colour_consts::GRAY;
	projectile_type type_ = SPRAY;

	player_team team_ = player_team::NONE;
	bool for_deletion_ = false;

	const float* dt_ = nullptr;
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
		RenderResources* render_resources,
		const ProjectileDetails& details,
		const DiffusionDetails& diffusion_details,
		const MattMath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);
	~DiffusingProjectile() override = default;


	void update() override = 0;
	void draw(DirectX::SpriteBatch* sprite_batch,
		const MattMath::Camera& camera) override = 0;
	bool is_visible_in_viewport(const MattMath::RectangleF& view) const override = 0;

	bool is_colliding(const ICollisionGameObject* other) const override = 0;
	const MattMath::Shape* get_shape() const override = 0;
protected:
	MattMath::Vector2F calculate_diffusion_size() const;
private:
	DiffusionDetails diffusion_details_ = DiffusionDetails();
	const MattMath::Vector2F& get_base_size() const;
};
