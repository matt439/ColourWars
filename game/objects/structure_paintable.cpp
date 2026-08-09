#include "game/pch.h"
#include "game/objects/structure_paintable.h"

using namespace DirectX;
using namespace mattmath;
using namespace artattack;

StructurePaintable::StructurePaintable(
	const std::string& sheet_name,
	const std::string& frame_name,
	const RectangleF& sprite_rectangle,
	std::unique_ptr<Shape> collision_shape,
	RenderResources* render_resources,
	const AudioResources* audio_resources,
	const PaintTuning* paint,
	CollisionObjectType collision_type,
	const TeamColour& team_colours,
	const PaintableFaces& faces,
	const Colour& color,
	float rotation,
	const Vector2F& origin,
	SpriteFlip flip,
	float layer_depth) :
	Structure(sheet_name, frame_name, sprite_rectangle,
		std::move(collision_shape),
		render_resources, collision_type, color, rotation, origin,
		flip, layer_depth),
		team_colours_(team_colours),
		faces_(faces),
		paint_(paint)
{
	this->paint_tiles_ = this->generate_paint_tiles();
	this->sound_bank_ = audio_resources->sound_bank(paint->sound_bank);
	this->paint_sound_ =
		this->sound_bank_->resolve_wave(paint->splash_sound.name);
}

void StructurePaintable::update(float dt)
{
	for (auto& paint_tile : this->paint_tiles_)
	{
		paint_tile.update(dt);
	}
}

void StructurePaintable::draw(DrawList& draw_list) const
{
	Structure::draw(draw_list);
	for (const auto& paint_tile : this->paint_tiles_)
	{
		paint_tile.draw(draw_list);
	}
}

void StructurePaintable::on_contact(const CollisionObject& other,
	const Vector2F& /*normal*/, float /*penetration*/)
{
	// Only paint paints. The mask lets players through as well, and a player
	// walking along a wall leaves nothing behind.
	const CollisionLayer other_layer = other.layer();
	if (other_layer != collision_layers::PROJECTILE_TEAM_A &&
		other_layer != collision_layers::PROJECTILE_TEAM_B)
	{
		return;
	}

	// Once, here, rather than a ten-way enum comparison inside the tile loop
	// recovering the same answer for every tile of the structure.
	const PlayerTeam team = other_layer == collision_layers::PROJECTILE_TEAM_A ?
		PlayerTeam::a : PlayerTeam::b;

	const RectangleF splash = other.shape()->bounding_box();

	bool tile_painted = false;
	for (PaintTile& paint_tile : this->paint_tiles_)
	{
		// A tile and a projectile are both axis-aligned rectangles, so this
		// is the exact test. The AABB-then-narrow block it replaces worked
		// that out for itself, per tile, after its own first branch had
		// already returned the answer.
		if (!paint_tile.bounds().intersects(splash))
		{
			continue;
		}

		paint_tile.paint(team);

		if (!tile_painted)
		{
			this->sound_bank_->play_wave(this->paint_sound_,
				this->paint_->splash_sound.volume);
		}
		tile_painted = true;
	}
}

std::vector<PaintTile> StructurePaintable::generate_paint_tiles() const
{
	auto paint_tiles = std::vector<PaintTile>();

	const PaintTuning& paint = *this->paint_;

	const int num_paint_tiles_x =
		static_cast<int>(std::fabs(this->rectangle().width / paint.size.x));
	const float paint_tile_width =
		this->rectangle().width / num_paint_tiles_x;

	const int num_paint_tiles_y =
		static_cast<int>(std::fabs(this->rectangle().height / paint.size.y));
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
				paint.thickness);
			auto paint_tile = PaintTile(paint_tile_rectangle,
			                            this->paint_,
			                            this->render_resources(),
			                            this->team_colours_);
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
				this->rectangle().bottom() - paint.thickness,
				paint_tile_width,
				paint.thickness);
			auto paint_tile = PaintTile(paint_tile_rectangle,
			                            this->paint_,
			                            this->render_resources(),
			                            this->team_colours_);
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
				paint.thickness,
				paint_tile_height);
			auto paint_tile = PaintTile(paint_tile_rectangle,
			                            this->paint_,
			                            this->render_resources(),
			                            this->team_colours_);
			paint_tiles.push_back(paint_tile);
		}
	}

	// right edge
	if (this->faces_.right)
	{
		for (int i = 0; i < num_paint_tiles_y; i++)
		{
			auto paint_tile_rectangle = RectangleF(
				this->rectangle().right() - paint.thickness,
				this->rectangle().top() + (paint_tile_height * i),
				paint.thickness,
				paint_tile_height);
			auto paint_tile = PaintTile(paint_tile_rectangle,
			                            this->paint_,
			                            this->render_resources(),
			                            this->team_colours_);
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