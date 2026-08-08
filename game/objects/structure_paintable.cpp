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
	const Colour& color,
	float rotation,
	const Vector2F& origin,
	SpriteFlip flip,
	float layer_depth) :
	Structure(sheet_name, frame_name, sprite_rectangle, collision_shape,
		render_resources, collision_type, color, rotation, origin,
		flip, layer_depth),
		team_colours_(team_colours),
		faces_(faces)
{
	this->paint_tiles_ = this->generate_paint_tiles();
	this->sound_bank_ = audio_resources->sound_bank(SOUND_BANK_NAME);
	this->paint_sound_ = this->sound_bank_->resolve_wave(SOUND_NAME);
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

void StructurePaintable::on_collision(const CollisionObject* other)
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
	const mattmath::RectangleF box = other->shape()->bounding_box();

	for (const PaintStrip& strip : this->paint_strips_)
	{
		const float low = strip.along_x ? box.left() : box.top();
		const float high = strip.along_x ? box.right() : box.bottom();

		// Deliberately one wide at each end. The exact range is available -
		// floor for the first index, ceil minus one for the last - but it sits
		// on a boundary case at every tile seam, and the cost of being wrong
		// there is a projectile that visibly fails to paint. Two extra
		// candidates out of fifteen hundred is not a cost worth defending, and
		// is_colliding is still what decides. Clamped, so a projectile hanging
		// off the end of a wall paints the tiles it does reach.
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
			// near enough to be worth testing, and only the shape test knows
			// whether the projectile reaches across the face's thickness.
			if (!paint_tile.is_colliding(other))
			{
				continue;
			}

			paint_tile.on_collision(other);

			if (!tile_painted)
			{
				this->sound_bank_->play_wave(this->paint_sound_, SOUND_VOLUME);
			}
			tile_painted = true;
		}
	}
}

std::vector<PaintTile> StructurePaintable::generate_paint_tiles()
{
	auto paint_tiles = std::vector<PaintTile>();
	this->paint_strips_.clear();

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
		this->paint_strips_.push_back(PaintStrip{
			static_cast<int>(paint_tiles.size()), num_paint_tiles_x,
			this->rectangle().left(), paint_tile_width, true });

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
				this->rectangle().bottom() - THICKNESS,
				paint_tile_width,
				THICKNESS);
			auto paint_tile = PaintTile(paint_tile_rectangle,
			                            SHEET_NAME, FRAME_NAME,
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
				THICKNESS,
				paint_tile_height);
			auto paint_tile = PaintTile(paint_tile_rectangle,
			                            SHEET_NAME, FRAME_NAME,
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
				this->rectangle().right() - THICKNESS,
				this->rectangle().top() + (paint_tile_height * i),
				THICKNESS,
				paint_tile_height);
			auto paint_tile = PaintTile(paint_tile_rectangle,
			                            SHEET_NAME, FRAME_NAME,
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