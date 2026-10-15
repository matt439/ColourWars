#include "game/pch.h"
#include "game/objects/structure_paintable.h"
#include "game/objects/projectile.h"
#include "game/paint_face_geometry.h"

using namespace DirectX;
using namespace mattmath;
using namespace labrador;

StructurePaintable::StructurePaintable(
	const std::string& sheet_name,
	const std::string& frame_name,
	const RectangleF& sprite_rectangle,
	std::unique_ptr<Shape> collision_shape,
	RenderResources* render_resources,
	const AudioResources* audio_resources,
	const PlaySettings* preferences,
	const PaintTuning* paint,
	CollisionObjectType collision_type,
	const TeamColour& team_colours,
	const PaintableFaces& faces,
	std::span<const RectangleF> paint_obstacles,
	const Colour& color,
	float rotation,
	const Vector2F& origin,
	SpriteFlip flip,
	float layer_depth,
	float tile_world_units_per_texel) :
	Structure(sheet_name, frame_name, sprite_rectangle,
		std::move(collision_shape),
		render_resources, collision_type, color, rotation, origin,
		flip, layer_depth, tile_world_units_per_texel),
		team_colours_(team_colours),
		faces_(faces),
		paint_(paint)
{
	this->preferences_ = preferences;
	this->paint_tiles_ = this->generate_paint_tiles(paint_obstacles);
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
	// The mask also accepts players, but only projectiles paint the surface.
	const CollisionLayer other_layer = other.layer();
	if (other_layer != collision_layers::PROJECTILE_TEAM_A &&
		other_layer != collision_layers::PROJECTILE_TEAM_B)
	{
		return;
	}

	const PlayerTeam team = other_layer == collision_layers::PROJECTILE_TEAM_A ?
		PlayerTeam::a : PlayerTeam::b;

	const Projectile& projectile = dynamic_cast<const Projectile&>(other);
	const RectangleF splash = other.shape()->bounding_box();

	bool tile_painted = false;

	// Each face is a strip of equal-size tiles. Use its origin and pitch to
	// select nearby candidates instead of scanning the entire structure.
	for (const PaintStrip& strip : this->paint_strips_)
	{
		const float low = strip.along_x ? splash.left() : splash.top();
		const float high = strip.along_x ? splash.right() : splash.bottom();

		// Pad the candidate range at tile seams, then clamp it to the face.
		// The rectangle test below decides which candidates are actually touched.
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

			// The strip range is conservative; require an actual rectangle overlap.
			if (!paint_tile.bounds().intersects(splash))
			{
				continue;
			}

			paint_tile.paint(team, projectile.player_num());

			if (!tile_painted)
			{
				this->sound_bank_->play_wave(this->paint_sound_,
					this->paint_->splash_sound.volume * this->preferences_->sfx_gain());
			}
			tile_painted = true;
		}
	}
}

std::vector<PaintTile> StructurePaintable::generate_paint_tiles(
    std::span<const RectangleF> paint_obstacles)
{
    std::vector<PaintTile> paint_tiles;
    this->paint_strips_.clear();
    const RectangleF rectangle = this->rectangle();
    const PaintTuning& paint = *this->paint_;
    std::vector<PaintFaceRectangle> obstacles;
    for (const RectangleF& obstacle : paint_obstacles)
        obstacles.push_back({obstacle.left(), obstacle.top(), obstacle.right(), obstacle.bottom()});

    const auto append_face = [&](bool enabled, bool along_x, bool leading)
    {
        if (!enabled)
        {
            return;
        }
        const float low = along_x ? rectangle.left() : rectangle.top();
        const float high = along_x ? rectangle.right() : rectangle.bottom();
        const float edge = along_x
            ? (leading ? rectangle.top() : rectangle.bottom())
            : (leading ? rectangle.left() : rectangle.right());
        const auto append_strip = [&](float begin, float end)
        {
            if (begin >= end)
            {
                return;
            }
            const float length = end - begin;
            const float authored_pitch = along_x ? paint.size.x : paint.size.y;
            const int count = std::max(1, static_cast<int>(length / authored_pitch));
            const float pitch = length / count;
            this->paint_strips_.push_back(PaintStrip{
                static_cast<int>(paint_tiles.size()), count, begin, pitch, along_x });
            for (int i = 0; i < count; ++i)
            {
                // Fit even partial edge tiles to the exposed segment, so
                // rendering, contact lookup and scored area agree exactly.
                const float position = begin + pitch * i;
                const RectangleF tile = along_x
                    ? RectangleF(position, leading ? edge : edge - paint.thickness,
                        pitch, paint.thickness)
                    : RectangleF(leading ? edge : edge - paint.thickness, position,
                        paint.thickness, pitch);
                paint_tiles.emplace_back(tile, this->paint_,
                    this->render_resources(), this->team_colours_);
            }
        };

        for (const std::pair<float, float>& interval :
            exposed_paint_intervals(low, high, edge, along_x, leading, obstacles))
            append_strip(interval.first, interval.second);
    };

    append_face(this->faces_.top, true, true);
    append_face(this->faces_.bottom, true, false);
    append_face(this->faces_.left, false, true);
    append_face(this->faces_.right, false, false);
    return paint_tiles;
}
PaintTotal StructurePaintable::paint_total() const
{
	auto total = PaintTotal();
	for (auto& paint_tile : this->paint_tiles_)
	{
		total.total += paint_tile.area();
		const int owner = paint_tile.player_num();
		if (owner >= 0 && static_cast<size_t>(owner) < total.player_areas.size())
		{
			total.player_areas[owner] += paint_tile.area();
		}

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
