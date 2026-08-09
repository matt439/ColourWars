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

	// The tiles a projectile can possibly touch, found by division rather than
	// by asking every tile in the level whether it was hit.
	//
	// Each face was laid down as a run of equally sized tiles along one axis,
	// so the ones overlapping a box are a contiguous index range, and the
	// range is two divisions away. A projectile 20 units across on a wall of
	// 20-unit tiles reaches two or three of them; the loop this replaces
	// tested all fifteen hundred, every frame, for every projectile in the
	// match. Nothing about that scan was wrong - it was answering a question
	// arithmetic had already answered at load.
	for (const PaintStrip& strip : this->paint_strips_)
	{
		const float low = strip.along_x ? splash.left() : splash.top();
		const float high = strip.along_x ? splash.right() : splash.bottom();

		// Deliberately one wide at each end. The exact range is available -
		// floor for the first index, ceil minus one for the last - but it sits
		// on a boundary case at every tile seam, and the cost of being wrong
		// there is a projectile that visibly fails to paint. Two extra
		// candidates out of fifteen hundred is not a cost worth defending, and
		// the rectangle test below is still what decides. Clamped, so a
		// projectile hanging off the end of a wall paints the tiles it does
		// reach.
		int first = static_cast<int>(
			std::floor((low - strip.origin) / strip.pitch)) - 1;
		int last = static_cast<int>(
			std::ceil((high - strip.origin) / strip.pitch)) + 1;

		first = std::max(first, 0);
		last = std::min(last, strip.count - 1);

		for (int i = first; i <= last; i++)
		{
			PaintTile& paint_tile = this->paint_tiles_[
				static_cast<size_t>(strip.begin + i)];

			// Still asked, not assumed. The index range says which tiles are
			// near enough to be worth testing; this says whether the
			// projectile actually reaches. A tile and a projectile are both
			// axis-aligned rectangles, so it is the exact test.
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
}

std::vector<PaintTile> StructurePaintable::generate_paint_tiles()
{
	auto paint_tiles = std::vector<PaintTile>();
	this->paint_strips_.clear();

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
		this->paint_strips_.push_back(PaintStrip{
			static_cast<int>(paint_tiles.size()), num_paint_tiles_x,
			this->rectangle().left(), paint_tile_width, true });

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
		this->paint_strips_.push_back(PaintStrip{
			static_cast<int>(paint_tiles.size()), num_paint_tiles_x,
			this->rectangle().left(), paint_tile_width, true });

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
		this->paint_strips_.push_back(PaintStrip{
			static_cast<int>(paint_tiles.size()), num_paint_tiles_y,
			this->rectangle().top(), paint_tile_height, false });

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
		this->paint_strips_.push_back(PaintStrip{
			static_cast<int>(paint_tiles.size()), num_paint_tiles_y,
			this->rectangle().top(), paint_tile_height, false });

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