#include "pch.h"
#include "StructurePaintable.h"

using namespace DirectX;
using namespace MattMath;
using namespace paint_tile_consts;
using namespace structure_paintable_consts;

StructurePaintable::StructurePaintable(
	const std::string& sheet_name,
	const std::string& frame_name,
	const RectangleF& rectangle,
	SpriteBatch* sprite_batch,
	ResourceManager* resource_manager,
	collision_object_type collision_type,
	const team_colour& team_colours,
	const paintable_faces& faces,
	const float* dt,
	const Colour& color,
	float rotation,
	const Vector2F& origin,
	SpriteEffects effects,
	float layer_depth) :
	Structure(sheet_name, frame_name, rectangle,
		sprite_batch, resource_manager,
		collision_type, color, rotation, origin,
		effects, layer_depth),
	_team_colours(team_colours),
	_faces(faces),
	_dt(dt)
{
	this->_paint_tiles = this->generate_paint_tiles();
	this->_sound_bank = resource_manager->get_sound_bank(SOUND_BANK_NAME);
}
void StructurePaintable::update()
{
	for (auto& paint_tile : this->_paint_tiles)
	{
		paint_tile.update();
	}
}
void StructurePaintable::draw(const Camera& camera)
{
	Structure::draw(camera);
	for (auto& paint_tile : this->_paint_tiles)
	{
		paint_tile.draw(camera);
	}
}
void StructurePaintable::draw()
{
	Structure::draw();
	for (auto& paint_tile : this->_paint_tiles)
	{
		paint_tile.draw();
	}

}
void StructurePaintable::on_collision(const ICollisionGameObject* other)
{
	// check if offensive projectile
	collision_object_type other_type = other->get_collision_object_type();
	bool is_offensive_projectile =
		other_type == collision_object_type::PROJECTILE_SPRAY_TEAM_A ||
		other_type == collision_object_type::PROJECTILE_SPRAY_TEAM_B ||
		other_type == collision_object_type::PROJECTILE_JET_TEAM_A ||
		other_type == collision_object_type::PROJECTILE_JET_TEAM_B ||
		other_type == collision_object_type::PROJECTILE_ROLLING_TEAM_A ||
		other_type == collision_object_type::PROJECTILE_ROLLING_TEAM_B ||
		other_type == collision_object_type::PROJECTILE_BALL_TEAM_A ||
		other_type == collision_object_type::PROJECTILE_BALL_TEAM_B ||
		other_type == collision_object_type::PROJECTILE_MIST_TEAM_A ||
		other_type == collision_object_type::PROJECTILE_MIST_TEAM_B;

	// if not offensive projectile, return
	if (!is_offensive_projectile)
	{
		return;
	}

	bool tile_painted = false;

	// check projectile against paint tiles
	for (auto& paint_tile : this->_paint_tiles)
	{
		if (paint_tile.is_colliding(other))
		{
			paint_tile.on_collision(other);
			
			if (!tile_painted)
			{
				this->_sound_bank->play_wave(SOUND_NAME, SOUND_VOLUME);
			}
			tile_painted = true;
		}
	}
}
std::vector<PaintTile> StructurePaintable::generate_paint_tiles() const
{
	std::vector<PaintTile> paint_tiles = std::vector<PaintTile>();
	
	const int num_paint_tiles_x =
		static_cast<int>(std::fabs(this->get_rectangle().get_width() / WIDTH));
	const float paint_tile_width =
		this->get_rectangle().get_width() / num_paint_tiles_x;

	const int num_paint_tiles_y =
		static_cast<int>(std::fabs(this->get_rectangle().get_height() / HEIGHT));
	const float paint_tile_height =
		this->get_rectangle().get_height() / num_paint_tiles_y;

	// top edge
	if (this->_faces.top)
	{
		for (int i = 0; i < num_paint_tiles_x; i++)
		{
			RectangleF paint_tile_rectangle = RectangleF(
				this->get_rectangle().get_left() + (paint_tile_width * i),
				this->get_rectangle().get_top(),
				paint_tile_width,
				THICKNESS);
			PaintTile paint_tile = PaintTile(paint_tile_rectangle,
				SHEET_NAME, FRAME_NAME,
				this->get_sprite_batch(),
				this->get_resource_manager(),
				this->_team_colours,
				this->_dt);
			paint_tiles.push_back(paint_tile);

		}
	}

	// bottom edge
	if (this->_faces.bottom)
	{
		for (int i = 0; i < num_paint_tiles_x; i++)
		{
			RectangleF paint_tile_rectangle = RectangleF(
				this->get_rectangle().get_left() + (paint_tile_width * i),
				this->get_rectangle().get_bottom() - THICKNESS,
				paint_tile_width,
				THICKNESS);
			PaintTile paint_tile = PaintTile(paint_tile_rectangle,
				SHEET_NAME, FRAME_NAME,
				this->get_sprite_batch(),
				this->get_resource_manager(),
				this->_team_colours,
				this->_dt);
			paint_tiles.push_back(paint_tile);
		}
	}

	// left edge
	if (this->_faces.left)
	{
		for (int i = 0; i < num_paint_tiles_y; i++)
		{
			RectangleF paint_tile_rectangle = RectangleF(
				this->get_rectangle().get_left(),
				this->get_rectangle().get_top() + (paint_tile_height * i),
				THICKNESS,
				paint_tile_height);
			PaintTile paint_tile = PaintTile(paint_tile_rectangle,
				SHEET_NAME, FRAME_NAME,
				this->get_sprite_batch(),
				this->get_resource_manager(),
				this->_team_colours,
				this->_dt);
			paint_tiles.push_back(paint_tile);
		}
	}

	// right edge
	if (this->_faces.right)
	{
		for (int i = 0; i < num_paint_tiles_y; i++)
		{
			RectangleF paint_tile_rectangle = RectangleF(
				this->get_rectangle().get_right() - THICKNESS,
				this->get_rectangle().get_top() + (paint_tile_height * i),
				THICKNESS,
				paint_tile_height);
			PaintTile paint_tile = PaintTile(paint_tile_rectangle,
				SHEET_NAME, FRAME_NAME,
				this->get_sprite_batch(),
				this->get_resource_manager(),
				this->_team_colours,
				this->_dt);
			paint_tiles.push_back(paint_tile);
		}
	}

	return paint_tiles;
}
paint_total StructurePaintable::get_paint_total() const
{
	paint_total total = paint_total();
	for (auto& paint_tile : this->_paint_tiles)
	{
		player_team team = paint_tile.get_team();
		switch (team)
		{
		case player_team::A:
			total.team_a += paint_tile.get_area();
			break;
		case player_team::B:
			total.team_b += paint_tile.get_area();
			break;
		default:
			break;
		};
	}
	return total;
}