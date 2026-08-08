#pragma once

#include "game/objects/player_team.h"
#include "engine/render/texture_object.h"
#include "game/objects/team_colour.h"
#include "game/objects/i_collision_game_object.h"
#include "engine/render/animation_object.h"

namespace paint_tile_consts
{
	constexpr float WIDTH = 4.0f;
	constexpr float HEIGHT = 4.0f;
	constexpr float THICKNESS = 12.0f;
	const static std::string SHEET_NAME = "sprite_sheet_1";
	const static std::string FRAME_NAME = "pixel";

	const static std::string SPLASH_SPRITE_SHEET_NAME = "sprite_sheet_1";
	const static std::string SPLASH_ANIMATION_STRIP_NAME = "splash_1";
	const static mattmath::RectangleF SPLASH_RECTANGLE = { 0.0f, 0.0f, THICKNESS * 2.0f, THICKNESS * 2.0f };
}

class PaintTileSplash final : public artattack::AnimationObject, public artattack::IGameObject
{
public:
	PaintTileSplash() = default;
	PaintTileSplash(
		const mattmath::RectangleF& rectangle,
		const std::string& sheet_name,
		const std::string& animation_strip_name,
		artattack::RenderResources* render_resources,
		const mattmath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	void set_rectangle_center(const mattmath::Vector2F& center);
	void reset_and_play();
	void set_colour(const mattmath::Colour& colour) override;

	void update(float dt) override;
	void draw(DirectX::SpriteBatch* sprite_batch,
		const mattmath::Camera& camera) const override;

	// Draws with the given tint without storing it. See TextureObject::draw_with.
	void draw_with_colour(DirectX::SpriteBatch* sprite_batch,
		const mattmath::Camera& camera, const mattmath::Colour& colour) const;

	mattmath::RectangleF bounds() const override;
private:
	mattmath::RectangleF rectangle_ = mattmath::RectangleF::ZERO;
};


class PaintTile final : public artattack::TextureObject, public ICollisionGameObject
{
public:
	PaintTile() = default;
	PaintTile(const mattmath::RectangleF& rectangle,
		const std::string& sheet_name,
		const std::string& frame_name,
		artattack::RenderResources* render_resources,
		const TeamColour& team_colours,
		const mattmath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);
	void draw(DirectX::SpriteBatch* sprite_batch,
		const mattmath::Camera& camera) const override;
	void update(float dt) override;
	float area() const;
	PlayerTeam team() const;
	bool is_colliding(const ICollisionGameObject* other) const override;
	const mattmath::Shape* shape() const override;

	void on_collision(const ICollisionGameObject* other) override;
	CollisionObjectType collision_object_type() const override;
	bool for_deletion() const override;
	mattmath::RectangleF bounds() const override;
private:
	mattmath::RectangleF rectangle_ = mattmath::RectangleF::ZERO;
	PlayerTeam team_ = PlayerTeam::none;
	TeamColour team_colours_ = TeamColour();
	PaintTileSplash splash_;
};
