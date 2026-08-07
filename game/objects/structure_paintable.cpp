#include "game/pch.h"
#include "game/objects/structure_paintable.h"

using namespace DirectX;
using namespace mattmath;
using namespace paint_tile_consts;
using namespace structure_paintable_consts;
using namespace artattack;

StructurePaintable::StructurePaintable(
	const std::string& sheet_name,
	const std::string& frame_name,
	const RectangleF& sprite_rectangle,
	const Shape* collision_shape,
	RenderResources* render_resources,
	const AudioResources* audio_resources,
	CollisionObjectType collision_type,
	const TeamColour& team_colours,
	const PaintableFaces& faces,
	const float* dt,
	const Colour& color,
	float rotation,
	const Vector2F& origin,
	SpriteEffects effects,
	float layer_depth) :
	Structure(sheet_name, frame_name, sprite_rectangle, collision_shape,
		render_resources, collision_type, color, rotation, origin,
		effects, layer_depth),
		team_colours_(team_colours),
		faces_(faces),
		dt_(dt)
{
	this->paint_tiles_ = this->generate_paint_tiles();
	this->sound_bank_ = audio_resources->sound_bank(SOUND_BANK_NAME);
	this->paint_sound_ = this->sound_bank_->resolve_wave(SOUND_NAME);
}

void StructurePaintable::update()
{
	for (auto& paint_tile : this->paint_tiles_)
	{
		paint_tile.update();
	}
}

void StructurePaintable::draw(SpriteBatch* sprite_batch, const Camera& camera) const
{
	Structure::draw(sprite_batch, camera);
	for (const auto& paint_tile : this->paint_tiles_)
	{
		paint_tile.draw(sprite_batch, camera);
	}
}

void StructurePaintable::draw(SpriteBatch* sprite_batch) const
{
	Structure::draw(sprite_batch);
	for (const auto& paint_tile : this->paint_tiles_)
	{
		paint_tile.draw(sprite_batch);
	}

}

void StructurePaintable::on_collision(const ICollisionGameObject* other)
{
	// check if offensive projectile
	CollisionObjectType other_type = other->collision_object_type();
	bool is_offensive_projectile =
		other_type == CollisionObjectType::projectile_spray_team_a ||
		other_type == CollisionObjectType::projectile_spray_team_b ||
		other_type == CollisionObjectType::projectile_jet_team_a ||
		other_type == CollisionObjectType::projectile_jet_team_b ||
		other_type == CollisionObjectType::projectile_rolling_team_a ||
		other_type == CollisionObjectType::projectile_rolling_team_b ||
		other_type == CollisionObjectType::projectile_ball_team_a ||
		other_type == CollisionObjectType::projectile_ball_team_b ||
		other_type == CollisionObjectType::projectile_mist_team_a ||
		other_type == CollisionObjectType::projectile_mist_team_b;

	// if not offensive projectile, return
	if (!is_offensive_projectile)
	{
		return;
	}

	bool tile_painted = false;

	// check projectile against paint tiles
	for (auto& paint_tile : this->paint_tiles_)
	{
		if (paint_tile.is_colliding(other))
		{
			paint_tile.on_collision(other);

			if (!tile_painted)
			{
				this->sound_bank_->play_wave(this->paint_sound_, SOUND_VOLUME);
			}
			tile_painted = true;
		}
	}
}

std::vector<PaintTile> StructurePaintable::generate_paint_tiles() const
{
	auto paint_tiles = std::vector<PaintTile>();

	const int num_paint_tiles_x =
		static_cast<int>(std::fabs(this->rectangle().width / WIDTH));
	const float paint_tile_width =
		this->rectangle().width / num_paint_tiles_x;

	const int num_paint_tiles_y =
		static_cast<int>(std::fabs(this->rectangle().height / HEIGHT));
	const float paint_tile_height =
		this->rectangle().height / num_paint_tiles_y;

	// top edge
	if (this->faces_.top)
	{
		for (int i = 0; i < num_paint_tiles_x; i++)
		{
			auto paint_tile_rectangle = RectangleF(
				this->rectangle().left() + (paint_tile_width * i),
				this->rectangle().top(),
				paint_tile_width,
				THICKNESS);
			auto paint_tile = PaintTile(paint_tile_rectangle,
			                            SHEET_NAME, FRAME_NAME,
			                            this->render_resources(),
			                            this->team_colours_,
			                            this->dt_);
			paint_tiles.push_back(paint_tile);

		}
	}

	// bottom edge
	if (this->faces_.bottom)
	{
		for (int i = 0; i < num_paint_tiles_x; i++)
		{
			auto paint_tile_rectangle = RectangleF(
				this->rectangle().left() + (paint_tile_width * i),
				this->rectangle().bottom() - THICKNESS,
				paint_tile_width,
				THICKNESS);
			auto paint_tile = PaintTile(paint_tile_rectangle,
			                            SHEET_NAME, FRAME_NAME,
			                            this->render_resources(),
			                            this->team_colours_,
			                            this->dt_);
			paint_tiles.push_back(paint_tile);
		}
	}

	// left edge
	if (this->faces_.left)
	{
		for (int i = 0; i < num_paint_tiles_y; i++)
		{
			auto paint_tile_rectangle = RectangleF(
				this->rectangle().left(),
				this->rectangle().top() + (paint_tile_height * i),
				THICKNESS,
				paint_tile_height);
			auto paint_tile = PaintTile(paint_tile_rectangle,
			                            SHEET_NAME, FRAME_NAME,
			                            this->render_resources(),
			                            this->team_colours_,
			                            this->dt_);
			paint_tiles.push_back(paint_tile);
		}
	}

	// right edge
	if (this->faces_.right)
	{
		for (int i = 0; i < num_paint_tiles_y; i++)
		{
			auto paint_tile_rectangle = RectangleF(
				this->rectangle().right() - THICKNESS,
				this->rectangle().top() + (paint_tile_height * i),
				THICKNESS,
				paint_tile_height);
			auto paint_tile = PaintTile(paint_tile_rectangle,
			                            SHEET_NAME, FRAME_NAME,
			                            this->render_resources(),
			                            this->team_colours_,
			                            this->dt_);
			paint_tiles.push_back(paint_tile);
		}
	}

	return paint_tiles;
}
PaintTotal StructurePaintable::paint_total() const
{
	auto total = PaintTotal();
	for (auto& paint_tile : this->paint_tiles_)
	{
		PlayerTeam team = paint_tile.team();
		switch (team)
		{
		case PlayerTeam::a:
			total.team_a += paint_tile.area();
			break;
		case PlayerTeam::b:
			total.team_b += paint_tile.area();
			break;
		default:
			break;
		};
	}
	return total;
}