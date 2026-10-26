#include "game/pch.h"
#include "game/objects/structure_paintable.h"
#include "game/objects/projectile.h"
#include "game/paint_face_geometry.h"

using namespace DirectX;
using namespace mattmath;
using namespace labrador;

namespace
{
	RectangleF clipped(const RectangleF& rectangle, const RectangleF& limit)
	{
		const float x = std::max(rectangle.left(), limit.left());
		const float y = std::max(rectangle.top(), limit.top());
		const float right = std::min(rectangle.right(), limit.right());
		const float bottom = std::min(rectangle.bottom(), limit.bottom());
		if (!(right > x && bottom > y)) { return RectangleF::ZERO; }
		return RectangleF(x, y, right - x, bottom - y);
	}
}

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
	std::unique_ptr<const TerrainMaterial> material,
	std::span<const PaintPolygon> paint_polygons) :
	Structure(sheet_name, frame_name, sprite_rectangle,
		std::move(collision_shape),
		render_resources, collision_type, color, rotation, origin,
		flip, layer_depth, std::move(material)),
		team_colours_(team_colours),
		faces_(faces),
		paint_(paint),
		preferences_(preferences),
		splash_clips_(PaintSplashClips::resolve(*paint, *render_resources)),
		runoff_sheet_(render_resources->resolve_sprite_sheet(paint->sheet)),
		runoff_frame_(render_resources->sprite_sheet(this->runoff_sheet_)->resolve_sprite_frame(paint->frame))
{
	this->paint_tiles_ = this->generate_paint_tiles(paint_obstacles, paint_polygons);
	// Preview construction shares this path. Cue names are validated at startup;
	// match playback is owned by Level's PaintAudio across every surface.
	(void)audio_resources;
}

void StructurePaintable::update(float dt)
{
	for (auto& paint_tile : this->paint_tiles_)
	{
		paint_tile.update(dt);
	}
	if (!(dt > 0.0f)) { return; }
	for (PaintStrip& strip : this->paint_strips_)
	{
		if (strip.runoff.team == PlayerTeam::none) { continue; }
		strip.runoff.age += dt;
		if (const auto visual = this->runoff_visual(strip))
		{
			strip.runoff.limit_y = visual->owned_band.bottom();
		}
		else { strip.runoff = {}; }
	}
}

void StructurePaintable::draw(DrawList& draw_list) const
{
	Structure::draw(draw_list);
	this->draw_paint(draw_list);
}

void StructurePaintable::draw_miniature(DrawList& draw_list) const
{
	Structure::draw_miniature(draw_list);
	this->draw_paint(draw_list);
}

void StructurePaintable::draw_paint(DrawList& draw_list) const
{
	// Use the actual exposed scoring strips: material and collision tags cannot
	// tell whether a one-way support or an individual face accepts paint.
	// Prime every strip first, so neutral sides never overwrite owned corners.
	for (const PaintStrip& strip : this->paint_strips_)
	{
		const RectangleF first = this->paint_tiles_[static_cast<size_t>(strip.begin)].bounds();
		const RectangleF last = this->paint_tiles_[static_cast<size_t>(strip.begin + strip.count - 1)].bounds();
		const RectangleF band = clipped(RectangleF::union_of(first, last), this->rectangle());
		const float depth = strip.along_x ? band.height : band.width;
		const float edge = std::min(this->paint_->edge_width, depth / 2.0f);
		this->render_resources()->sprite_sheet(this->runoff_sheet_)->draw(
			draw_list, this->runoff_frame_, band, Colour::white);
		const RectangleF inner = strip.along_x
			? RectangleF(band.x, strip.leading ? band.bottom() - edge : band.top(), band.width, edge)
			: RectangleF(strip.leading ? band.right() - edge : band.left(), band.y, edge, band.height);
		this->render_resources()->sprite_sheet(this->runoff_sheet_)->draw(
			draw_list, this->runoff_frame_, inner, Colour::black);
	}
	for (const auto& paint_tile : this->paint_tiles_)
	{
		paint_tile.draw_splash(draw_list);
	}
	for (const auto& paint_tile : this->paint_tiles_)
	{
		paint_tile.draw_surface(draw_list);
	}
	if (this->preferences_ == nullptr || !this->preferences_->reduce_effects)
	{
		for (const PaintStrip& strip : this->paint_strips_)
		{
			this->draw_runoff(draw_list, strip);
		}
	}
}

RectangleF StructurePaintable::cull_bounds(float units_per_pixel) const
{
	RectangleF result = Structure::bounds();
	for (const PaintTile& tile : this->paint_tiles_)
	{
		result = RectangleF::union_of(result, tile.visual_bounds());
	}
	// Cardinally rotated impact edges can cover a final quantized view pixel.
	result.inflate(units_per_pixel);
	return result;
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
	this->paint_from_projectile(projectile, splash, team);
}

void StructurePaintable::paint_from_projectile(const Projectile& projectile,
	const RectangleF& splash)
{
	const CollisionLayer other_layer = projectile.layer();
	if (other_layer != collision_layers::PROJECTILE_TEAM_A &&
		other_layer != collision_layers::PROJECTILE_TEAM_B) { return; }
	const PlayerTeam team = other_layer == collision_layers::PROJECTILE_TEAM_A ?
		PlayerTeam::a : PlayerTeam::b;
	this->paint_from_projectile(projectile, splash, team);
}

void StructurePaintable::paint_from_projectile(const Projectile& projectile,
	const RectangleF& splash, PlayerTeam team)
{
	bool tile_painted = false;
	bool tile_captured = false;

	// Each face is a strip of equal-size tiles. Use its origin and pitch to
	// select nearby candidates instead of scanning the entire structure.
	for (PaintStrip& strip : this->paint_strips_)
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

		int first_painted = -1;
		int last_painted = -1;
		for (int i = first; i <= last; i++)
		{
			PaintTile& paint_tile = this->paint_tiles_[
				static_cast<size_t>(strip.begin + i)];

			// The strip range is conservative; require an actual rectangle overlap.
			if (!paint_tile.bounds().intersects(splash))
			{
				continue;
			}

			tile_captured |= paint_tile.team() != team;
			paint_tile.paint(team, projectile.player_num(), projectile.projectile_type());
			tile_painted = true;
			if (first_painted < 0) { first_painted = i; }
			last_painted = i;
		}
		if (!strip.along_x && first_painted >= 0)
		{
			// Repainting the source retires the old colour immediately. Repeated
			// friendly stream contacts leave a running phrase free to finish.
			if (strip.runoff.team != PlayerTeam::none)
			{
				if (const auto visual = this->runoff_visual(strip))
				{
					strip.runoff.limit_y = visual->owned_band.bottom();
				}
				else { strip.runoff = {}; }
			}
			if (strip.runoff.team == PlayerTeam::none)
			{
				const int source = std::clamp(static_cast<int>(std::floor(
					(splash.center().y - strip.origin) / strip.pitch)),
					first_painted, last_painted);
				const RectangleF source_bounds = this->paint_tiles_[
					static_cast<size_t>(strip.begin + source)].bounds();
				strip.runoff = Runoff{ team, source,
					std::clamp(splash.center().y, source_bounds.top(), source_bounds.bottom()),
					0.0f, strip.end };
				if (const auto visual = this->runoff_visual(strip))
				{
					strip.runoff.limit_y = visual->owned_band.bottom();
				}
				else { strip.runoff = {}; }
			}
		}
	}
	projectile.note_paint_contact(tile_painted, tile_captured);
}

std::optional<PaintRunoffVisual> StructurePaintable::runoff_visual(
	const PaintStrip& strip) const
{
	const Runoff& state = strip.runoff;
	const PaintRunoffTuning& look = this->paint_->runoff;
	if (strip.along_x || state.team == PlayerTeam::none ||
		!(look.duration > 0.0f) || state.age >= look.delay + look.duration)
	{
		return std::nullopt;
	}
	const PaintTile& source = this->paint_tiles_[
		static_cast<size_t>(strip.begin + state.source_tile)];
	if (source.team() != state.team) { return std::nullopt; }

	const RectangleF source_bounds = source.bounds();
	// Only inspect tiles the bounded phrase can actually reach. Exposed strips
	// already end at obstacles; current ownership may end the corridor sooner.
	const float travel_end = std::min(state.limit_y, state.source_y + look.distance);
	float bottom = std::min(source_bounds.bottom(), travel_end);
	for (int index = state.source_tile + 1; index < strip.count && bottom < travel_end; ++index)
	{
		const PaintTile& tile = this->paint_tiles_[static_cast<size_t>(strip.begin + index)];
		if (tile.team() != state.team) { break; }
		bottom = std::min(tile.bounds().bottom(), travel_end);
	}
	const float colour_depth = std::max(0.0f, this->paint_->thickness - this->paint_->edge_width);
	const RectangleF band(strip.leading ? source_bounds.left() :
		source_bounds.right() - colour_depth, source_bounds.top(),
		colour_depth, bottom - source_bounds.top());
	const float progress = std::clamp((state.age - look.delay) / look.duration, 0.0f, 1.0f);
	const float head = state.source_y + look.distance * progress * progress;
	if (!(head < bottom && band.width > 0.0f && band.height > 0.0f))
	{
		return std::nullopt;
	}
	PaintRunoffVisual visual{ state.team, state.age, state.source_y, head, band };
	if (state.age < look.delay) { return visual; }

	const float x = strip.leading ? band.left() + look.face_inset :
		band.right() - look.face_inset - look.width;
	visual.bead = clipped(RectangleF(x, head - look.bead_height,
		look.width, look.bead_height), band);
	const float trail_top = std::max(state.source_y,
		head - look.bead_height - look.trail_length);
	visual.trail = clipped(RectangleF(x + (look.width - look.trail_width) / 2.0f,
		trail_top, look.trail_width, std::max(0.0f, head - look.bead_height / 2.0f - trail_top)), band);
	return visual;
}

std::vector<PaintRunoffVisual> StructurePaintable::runoff_visuals() const
{
	std::vector<PaintRunoffVisual> visuals;
	for (const PaintStrip& strip : this->paint_strips_)
	{
		if (const auto visual = this->runoff_visual(strip)) { visuals.push_back(*visual); }
	}
	return visuals;
}

void StructurePaintable::draw_runoff(DrawList& draw_list, const PaintStrip& strip) const
{
	const auto visual = this->runoff_visual(strip);
	if (!visual || !(visual->bead.width > 0.0f)) { return; }
	const PaintRunoffTuning& look = this->paint_->runoff;
	const Colour tint = this->team_colours_.team_colour(visual->team);
	const Colour shade(tint.r * (1.0f - look.shade), tint.g * (1.0f - look.shade),
		tint.b * (1.0f - look.shade), 1.0f);
	const Colour highlight(tint.r + (1.0f - tint.r) * look.highlight,
		tint.g + (1.0f - tint.g) * look.highlight,
		tint.b + (1.0f - tint.b) * look.highlight, 1.0f);
	const auto fill = [&](const RectangleF& rectangle, const Colour& colour)
	{
		const RectangleF box = clipped(rectangle, visual->owned_band);
		if (!(box.width > 0.0f && box.height > 0.0f)) { return; }
		this->render_resources()->sprite_sheet(this->runoff_sheet_)->draw(
			draw_list, this->runoff_frame_, box, colour);
	};
	fill(visual->trail, shade);
	const float x = visual->bead.x;
	const float y = visual->head_y - look.bead_height;
	const float pen = look.keyline_width;
	// Two crossing rectangles leave the coarse corners open. Every piece is
	// clipped before submission; neither this bead nor its trail places paint.
	fill(RectangleF(x + pen, y, look.width - pen * 2.0f, look.bead_height), Colour::black);
	fill(RectangleF(x, y + pen, look.width, look.bead_height - pen * 2.0f), Colour::black);
	fill(RectangleF(x + pen, y + pen, look.width - pen * 2.0f,
		look.bead_height - pen * 2.0f), shade);
	fill(RectangleF(x + pen, y + pen, std::min(pen, look.width - pen * 2.0f), std::min(pen * 2.0f,
		look.bead_height - pen * 2.0f)), highlight);
}

std::vector<PaintTile> StructurePaintable::generate_paint_tiles(
    std::span<const RectangleF> paint_obstacles,
    std::span<const PaintPolygon> paint_polygons)
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
                static_cast<int>(paint_tiles.size()), count, begin, pitch, end, along_x,
                along_x && leading, leading, {} });
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
                paint_tiles.emplace_back(tile, this->paint_, &this->splash_clips_,
                    this->render_resources(), this->team_colours_, Colour::white,
                    0.0f, Vector2F::ZERO, SpriteFlip::none, 0.0f, along_x, leading);
            }
        };

        const auto exposed = exposed_paint_intervals(low, high, edge, along_x, leading, obstacles);
        const auto intervals = paint_polygons.empty() ? exposed : clip_paint_polygons(exposed,
            along_x ? PaintPoint{ 0.0, edge } : PaintPoint{ edge, 0.0 },
            along_x ? PaintPoint{ 1.0, 0.0 } : PaintPoint{ 0.0, 1.0 }, paint_polygons);
        for (const std::pair<float, float>& interval : intervals)
            append_strip(interval.first, interval.second);
    };

    append_face(this->faces_.top, true, true);
    append_face(this->faces_.bottom, true, false);
    append_face(this->faces_.left, false, true);
    append_face(this->faces_.right, false, false);
    return paint_tiles;
}
PlayerTeam StructurePaintable::top_paint_team(float x) const
{
	for (const PaintStrip& strip : this->paint_strips_)
	{
		if (!strip.top || !(x >= strip.origin && x < strip.end))
		{
			continue;
		}
		const int index = std::min(static_cast<int>((x - strip.origin) / strip.pitch),
			strip.count - 1);
		return this->paint_tiles_[static_cast<size_t>(strip.begin + index)].team();
	}
	return PlayerTeam::none;
}

PlayerTeam StructurePaintable::top_paint_team_translated(float world_x, float offset_x) const
{
	for (const PaintStrip& strip : this->paint_strips_)
	{
		if (!strip.top) { continue; }
		for (int index = 0; index < strip.count; ++index)
		{
			const PaintTile& tile = this->paint_tiles_[static_cast<size_t>(strip.begin + index)];
			const RectangleF rectangle = tile.bounds();
			const float left = offset_x == 0.0f ? rectangle.left() : rectangle.left() + offset_x;
			const float right = offset_x == 0.0f ? rectangle.right() : rectangle.right() + offset_x;
			if (world_x >= left && world_x < right) { return tile.team(); }
		}
	}
	return PlayerTeam::none;
}

std::optional<Colour> StructurePaintable::top_paint_colour(float x) const
{
	const PlayerTeam team = this->top_paint_team(x);
	if (team == PlayerTeam::none) { return std::nullopt; }
	return this->team_colours_.team_colour(team);
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

void StructurePaintable::append_paint_surfaces(std::vector<PaintSurfaceSample>& samples,
    std::size_t source_index) const
{
    samples.reserve(samples.size() + this->paint_tiles_.size());
    for (const PaintStrip& strip : this->paint_strips_)
    {
        const PaintSurfaceFace face = strip.along_x
            ? (strip.leading ? PaintSurfaceFace::top : PaintSurfaceFace::bottom)
            : (strip.leading ? PaintSurfaceFace::left : PaintSurfaceFace::right);
        for (int index = 0; index < strip.count; ++index)
        {
            const PaintTile& tile = this->paint_tiles_[static_cast<std::size_t>(strip.begin + index)];
            const RectangleF rectangle = tile.bounds();
            PaintSurfaceSample sample;
            sample.corners = { Vector2F(rectangle.left(), rectangle.top()),
                Vector2F(rectangle.right(), rectangle.top()),
                Vector2F(rectangle.right(), rectangle.bottom()),
                Vector2F(rectangle.left(), rectangle.bottom()) };
            sample.face = face;
            sample.team = tile.team();
            sample.paint_owner = tile.player_num();
            sample.scored_area = tile.area();
            sample.source_index = source_index;
            samples.push_back(sample);
        }
    }
}
