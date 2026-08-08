#pragma once

#include "game/objects/player_team.h"
#include "game/objects/projectile_consts.h"
#include "engine/render/animation_object.h"
#include "engine/core/moving_object.h"
#include "engine/collision/collision_object.h"
#include "game/objects/collision_object_type.h"

enum projectile_type
{
	SPRAY,
	JET,
	ROLLING,
	MIST,
	BALL
};

class Projectile : public artattack::MovingObject,
	public artattack::AnimationObject, public artattack::CollisionObject
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
		artattack::SpriteFlip flip = artattack::SpriteFlip::none,
		float layer_depth = 0.0f);

	void update(float dt) override = 0;
	void draw(artattack::DrawList& draw_list) const override = 0;
	mattmath::RectangleF bounds() const override = 0;

	const mattmath::Shape* shape() const override = 0;
	artattack::CollisionLayer layer() const override;
	artattack::CollisionMask mask() const override;
	artattack::CollisionTag tag() const override;
	void on_contact(const artattack::CollisionObject& other,
		const mattmath::Vector2F& normal, float penetration) override;
	bool for_deletion() const override;
	void set_for_deletion(bool for_deletion) override;

	float delete_timer() const;
	mattmath::Vector2F col_rect_size() const;
	float player_damage() const;
protected:
	const ProjectileDetails& details() const;

	PlayerTeam team() const;

	CollisionObjectType collision_type() const;

	int player_num() const;
	const mattmath::Colour& team_colour() const;
	projectile_type type() const;

	float timer() const;
	void alter_timer(float time);

	// Returns this step's displacement rather than leaving it on a member for
	// the caller to read back. Only the four subclasses immediately below the
	// call ever wanted it, and a member that is written and read inside one
	// call chain is a return value that has been talked out of being one.
	virtual mattmath::Vector2F update_movement(float gravity,
		float wind_resistance, float dt);
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
		artattack::SpriteFlip flip = artattack::SpriteFlip::none,
		float layer_depth = 0.0f);
	~DiffusingProjectile() override = default;


	void update(float dt) override = 0;
	void draw(artattack::DrawList& draw_list) const override = 0;
	mattmath::RectangleF bounds() const override = 0;

	const mattmath::Shape* shape() const override = 0;
protected:
	mattmath::Vector2F calculate_diffusion_size() const;
private:
	DiffusionDetails diffusion_details_ = DiffusionDetails();
	const mattmath::Vector2F& base_size() const;
};
