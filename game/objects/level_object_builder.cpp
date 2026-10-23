#include "game/pch.h"
#include "game/objects/level_object_builder.h"
#include "engine/math/triangle.h"
#include "game/objects/airbrush_shadow.h"
#include "game/objects/drifting_visual.h"
#include "game/paint_face_geometry.h"
#include <cmath>
#include <initializer_list>

using namespace mattmath;
using namespace labrador;

namespace
{
	// Game assets used to draw the viewport manager's divider rectangles.
	const std::string DIVIDER_SHEET_NAME = "sprite_sheet_1";
	const std::string DIVIDER_FRAME_NAME = "pixel";
	const Colour DIVIDER_COLOUR = Colour::black;

	class ViewportDivider final : public GameObject, public TextureObject
	{
	public:
		ViewportDivider(const ViewportManager* viewport_manager, size_t index,
			RenderResources* render_resources) :
			TextureObject(DIVIDER_SHEET_NAME, DIVIDER_FRAME_NAME,
				render_resources, DIVIDER_COLOUR),
			viewport_manager_(viewport_manager), index_(index)
		{
		}

		void update(float /*dt*/) override {}

		void draw(DrawList& draw_list) const override
		{
			this->TextureObject::draw(draw_list, this->bounds());
		}

		RectangleF bounds() const override
		{
			// The layout stays fixed for a match; its pixel dimensions do not.
			return this->viewport_manager_->viewport_dividers().at(this->index_);
		}

	private:
		const ViewportManager* viewport_manager_;
		size_t index_;
	};

	bool is_hex_rgb(const std::string& text)
	{
		return text.length() == 6 &&
			text.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos;
	}

	// A CSS name or RRGGBB, and grey: nothing in the picture has been coloured
	// yet, so team hues never appear in the scenery (docs/backstory.md).
	Colour colour_of(const JsonValue& json)
	{
		const std::string name = json.string("colour");
		const std::optional<Colour> named = colour_from_name(name);
		if (!named && !is_hex_rgb(name))
		{
			throw std::runtime_error(json.where() +
				": '" + name + "' is not a colour name or RRGGBB");
		}
		const Colour colour = named ? *named : Colour(name);
		if (colour.r != colour.g || colour.g != colour.b)
		{
			throw std::runtime_error(json.where() + ": '" + name +
				"' is not a grey; the picture is grey until a colour is saved");
		}
		return colour;
	}

	// Accept both rectangle encodings; report the object and expected keys
	// if neither is supplied.
	RectangleF checked_rectangle(const JsonValue& json,
		const RectangleF& rectangle)
	{
		if (!std::isfinite(rectangle.x) || !std::isfinite(rectangle.y) ||
			!std::isfinite(rectangle.width) || !std::isfinite(rectangle.height) ||
			rectangle.width <= 0.0f || rectangle.height <= 0.0f)
		{
			throw std::runtime_error(json.where() +
				" needs finite coordinates and positive width and height");
		}
		return rectangle;
	}

	RectangleF decode_rectangle(const JsonValue& json)
	{
		if (json.has("rectangle"))
		{
			const JsonValue rectangle = json.object("rectangle");
			return checked_rectangle(rectangle, RectangleF(
				rectangle.number("x"),
				rectangle.number("y"),
				rectangle.number("width"),
				rectangle.number("height")));
		}
		if (json.has("rectangle_tlbr"))
		{
			const JsonValue rectangle = json.object("rectangle_tlbr");
			return checked_rectangle(rectangle, RectangleF::from_top_left_bottom_right(
				Vector2F(rectangle.number("left"), rectangle.number("top")),
				Vector2F(rectangle.number("right"), rectangle.number("bottom"))));
		}
		throw std::runtime_error(json.where() +
			" has neither a 'rectangle' nor a 'rectangle_tlbr'");
	}

	CollisionObjectType collision_type_named(const JsonValue& json,
		const std::string& name)
	{
		if (name == "STRUCTURE")
		{
			return CollisionObjectType::structure;
		}
		if (name == "STRUCTURE_JUMP_THROUGH")
		{
			return CollisionObjectType::structure_jump_through;
		}
		if (name == "STRUCTURE_PAINTABLE")
		{
			return CollisionObjectType::structure_paintable;
		}
		if (name == "STRUCTURE_RAMP_RIGHT")
		{
			return CollisionObjectType::structure_ramp_right;
		}
		if (name == "STRUCTURE_RAMP_LEFT")
		{
			return CollisionObjectType::structure_ramp_left;
		}
		throw std::runtime_error(json.where() +
			": '" + name + "' is not a collision type");
	}

	// Allowed collision tags for each object type.
	CollisionObjectType collision_type_of(const JsonValue& json,
		const std::string& object_type,
		std::initializer_list<CollisionObjectType> allowed)
	{
		const std::string name = json.string("collision_type");
		const CollisionObjectType type = collision_type_named(json, name);
		for (CollisionObjectType candidate : allowed)
		{
			if (candidate == type)
			{
				return type;
			}
		}
		throw std::runtime_error(json.where() + ": a " + object_type +
			" may not have collision type '" + name + "'");
	}
}

namespace
{
	// The faces of `rectangle` nothing in `flat` covers (paint_face_geometry.h).
	TerrainFaces exposed_faces(const RectangleF& rectangle, std::span<const RectangleF> flat)
	{
		std::vector<PaintFaceRectangle> obstacles;
		obstacles.reserve(flat.size());
		for (const RectangleF& other : flat)
		{
			obstacles.push_back({ other.left(), other.top(), other.right(), other.bottom() });
		}
		TerrainFaces faces;
		faces.top = exposed_paint_intervals(rectangle.left(), rectangle.right(),
			rectangle.top(), true, true, obstacles);
		faces.bottom = exposed_paint_intervals(rectangle.left(), rectangle.right(),
			rectangle.bottom(), true, false, obstacles);
		faces.left = exposed_paint_intervals(rectangle.top(), rectangle.bottom(),
			rectangle.left(), false, true, obstacles);
		faces.right = exposed_paint_intervals(rectangle.top(), rectangle.bottom(),
			rectangle.right(), false, false, obstacles);
		return faces;
	}

	// An optional Tool Props material; absent keeps the stretched frame.
	std::unique_ptr<const TerrainMaterial> material_of(const JsonValue& json,
		CollisionObjectType type, const Shape& shape, std::span<const RectangleF> flat,
		float mirror_x, RenderResources* render_resources)
	{
		if (!json.has("material"))
		{
			return nullptr;
		}
		try
		{
			return TerrainMaterial::make(json.string("material"), type, shape,
				exposed_faces(shape.bounding_box(), flat), mirror_x, render_resources);
		}
		catch (const std::invalid_argument& error)
		{
			throw std::runtime_error(json.where() + ": " + error.what());
		}
	}

	// Platforms, not walls or the stage's foundations.
	constexpr float shadow_caster_limit = 200.0f;
}

LevelObjectBuilder::LevelObjectBuilder(RenderResources* render_resources,
	const AudioResources* audio_resources,
	const PlaySettings* preferences,
	const PaintTuning* paint) :
	paint_(paint),
	render_resources_(render_resources),
	audio_resources_(audio_resources)
{
	this->preferences_ = preferences;

}

std::unique_ptr<CollisionObject>
	LevelObjectBuilder::build_collision_object(const JsonValue& json,
		const TeamColour& team_colours,
		const StageContext& context) const
{
	if (json.has("drift"))
	{
		throw std::runtime_error(json.where() + ": drift belongs only on decorative Visual entries");
	}
	const std::string type = json.string("type");
	if (type == "Structure")
	{
		const CollisionObjectType collision_type = collision_type_of(json, type,
			{ CollisionObjectType::structure,
			  CollisionObjectType::structure_jump_through });

		const RectangleF rectangle = decode_rectangle(json);

		return std::make_unique<Structure>(
			json.string("sheet_name"),
			json.string("frame_name"),
			rectangle,
			std::make_unique<RectangleF>(rectangle),
			this->render_resources_,
			collision_type,
			colour_of(json),
			0.0f,
			Vector2F::ZERO,
			SpriteFlip::none,
			0.0f,
			material_of(json, collision_type, rectangle, context.solid, context.mirror_x,
				this->render_resources_));
	}
	if (type == "StructurePaintable")
	{
		const CollisionObjectType collision_type = collision_type_of(json, type,
			{ CollisionObjectType::structure_paintable,
			  CollisionObjectType::structure_jump_through });

		const RectangleF rectangle = decode_rectangle(json);

		const JsonValue faces_json = json.object("paintable_faces");
		PaintableFaces faces;
		faces.left = faces_json.boolean("left");
		faces.top = faces_json.boolean("top");
		faces.right = faces_json.boolean("right");
		faces.bottom = faces_json.boolean("bottom");

		return std::make_unique<StructurePaintable>(
			json.string("sheet_name"),
			json.string("frame_name"),
			rectangle,
			std::make_unique<RectangleF>(rectangle),
			this->render_resources_,
			this->audio_resources_,
			this->preferences_,
			this->paint_,
			collision_type,
			team_colours,
			faces,
			context.flat,
			colour_of(json),
			0.0f,
			Vector2F::ZERO,
			SpriteFlip::none,
			0.0f,
			material_of(json, collision_type, rectangle, context.solid, context.mirror_x,
				this->render_resources_));
	}
	if (type == "StructureRamp")
	{
		const CollisionObjectType collision_type = collision_type_of(json, type,
			{ CollisionObjectType::structure_ramp_right,
			  CollisionObjectType::structure_ramp_left });

		const JsonValue triangle_json = json.object("triangle");
		const TriangleRightAxisAligned triangle = TriangleRightAxisAligned(
			triangle_json.number("x1"),
			triangle_json.number("y1"),
			triangle_json.number("x2"),
			triangle_json.number("y2"),
			triangle_json.number("x3"),
			triangle_json.number("y3"));

		// The sheet's ramp frame is right-handed; mirror it for left-handed collision geometry.
		const SpriteFlip flip =
			collision_type == CollisionObjectType::structure_ramp_left
			? SpriteFlip::horizontal
			: SpriteFlip::none;

		return std::make_unique<Structure>(
			json.string("sheet_name"),
			json.string("frame_name"),
			triangle.bounding_box(),
			std::make_unique<TriangleRightAxisAligned>(triangle),
			this->render_resources_,
			collision_type,
			colour_of(json),
			0.0f,
			Vector2F::ZERO,
			flip,
			0.0f,
			material_of(json, collision_type, triangle, context.solid, context.mirror_x,
				this->render_resources_));
	}

	throw std::runtime_error(json.where() +
		": '" + type + "' is not a collision object type");
}

std::unique_ptr<GameObject>
	LevelObjectBuilder::build_non_collision_object(const JsonValue& json) const
{
	const std::string type = json.string("type");
	if (type == "Visual")
	{
		if (json.has("drift"))
		{
			const JsonValue drift = json.object("drift");
			try
			{
				return std::make_unique<DriftingVisual>(json.string("sheet_name"),
					json.string("frame_name"), decode_rectangle(json), this->render_resources_,
					this->preferences_, colour_of(json), drift.number("distance"),
					drift.number("period"), drift.number("phase"));
			}
			catch (const std::invalid_argument& error)
			{
				throw std::runtime_error(drift.where() + ": " + error.what());
			}
		}
		return std::make_unique<Visual>(
			json.string("sheet_name"),
			json.string("frame_name"),
			decode_rectangle(json),
			this->render_resources_,
			colour_of(json));
	}

	throw std::runtime_error(json.where() +
		": '" + type + "' is not a non-collision object type");
}

LevelObjectBuilder::StageContext
	LevelObjectBuilder::stage_context(const JsonValue& json)
{
	// Only flat structures block paint. Ramps deliberately let projectiles pass
	// through, and decorative art must not remove reachable scoring surface.
	// The same rectangles decide which edges of the terrain are outlined.
	StageContext context;
	float left = 0.0f;
	float right = 0.0f;
	for (size_t index = 0; index < json.size(); ++index)
	{
		const JsonValue object = json.at(index);
		const std::string type = object.string("type");
		if (type == "Structure" || type == "StructurePaintable")
		{
			const RectangleF rectangle = decode_rectangle(object);
			left = context.flat.empty() ? rectangle.left() : std::min(left, rectangle.left());
			right = context.flat.empty() ? rectangle.right() : std::max(right, rectangle.right());
			context.flat.push_back(rectangle);
			context.solid.push_back(rectangle);
		}
		else if (type == "StructureRamp")
		{
			const JsonValue t = object.object("triangle");
			context.solid.push_back(TriangleRightAxisAligned(t.number("x1"), t.number("y1"),
				t.number("x2"), t.number("y2"), t.number("x3"), t.number("y3")).bounding_box());
		}
	}
	context.mirror_x = (left + right) / 2.0f;
	return context;
}

std::vector<std::unique_ptr<CollisionObject>>
	LevelObjectBuilder::build_collision_objects(const JsonValue& json,
		const TeamColour& team_colours) const
{
	const StageContext context = stage_context(json);
	std::vector<std::unique_ptr<CollisionObject>> collision_objects;
	collision_objects.reserve(json.size());
	for (size_t index = 0; index < json.size(); ++index)
	{
		collision_objects.push_back(
			build_collision_object(json.at(index), team_colours, context));
	}
	return collision_objects;
}

std::vector<std::unique_ptr<GameObject>>
	LevelObjectBuilder::build_shadows(const JsonValue& json) const
{
	const StageContext context = stage_context(json);
	std::vector<std::unique_ptr<GameObject>> shadows;
	for (size_t index = 0; index < json.size(); ++index)
	{
		const JsonValue object = json.at(index);
		const std::string type = object.string("type");
		if (!object.has("material") || (type != "Structure" && type != "StructurePaintable"))
		{
			continue;
		}
		const RectangleF rectangle = decode_rectangle(object);
		if (!(rectangle.width > rectangle.height && rectangle.height < shadow_caster_limit))
		{
			continue;
		}
		std::vector<PaintFaceRectangle> obstacles;
		for (const RectangleF& other : context.solid)
		{
			obstacles.push_back({ other.left(), other.top(), other.right(), other.bottom() });
		}
		for (const auto& [low, high] : exposed_paint_intervals(rectangle.left(),
			rectangle.right(), rectangle.bottom(), true, false, obstacles))
		{
			shadows.push_back(std::make_unique<AirbrushShadow>(
				low, high, rectangle.bottom(), this->render_resources_));
		}
	}
	return shadows;
}

std::vector<std::unique_ptr<GameObject>>
	LevelObjectBuilder::build_non_collision_objects(const JsonValue& json) const
{
	std::vector<std::unique_ptr<GameObject>> non_collision_objects;
	non_collision_objects.reserve(json.size());
	for (size_t index = 0; index < json.size(); ++index)
	{
		non_collision_objects.push_back(
			build_non_collision_object(json.at(index)));
	}
	return non_collision_objects;
}

void LevelObjectBuilder::validate_objects(const JsonValue& collision_objects,
	const JsonValue& non_collision_objects) const
{
	const TeamColour colours;
	const StageContext context = stage_context(collision_objects);
	for (size_t index = 0; index < collision_objects.size(); ++index)
	{
		const JsonValue object = collision_objects.at(index);
		try
		{
			build_collision_object(object, colours, context);
		}
		catch (const std::exception& error)
		{
			throw std::runtime_error(object.where() + ": " + error.what());
		}
	}
	for (size_t index = 0; index < non_collision_objects.size(); ++index)
	{
		const JsonValue object = non_collision_objects.at(index);
		try
		{
			build_non_collision_object(object);
		}
		catch (const std::exception& error)
		{
			throw std::runtime_error(object.where() + ": " + error.what());
		}
	}
	build_shadows(collision_objects);
}

std::vector<std::unique_ptr<GameObject>>
	LevelObjectBuilder::build_viewport_dividers(
		const ViewportManager* viewport_manager) const
{
	std::vector<std::unique_ptr<GameObject>> viewport_dividers;

	const std::vector<RectangleF> viewport_rectangles =
		viewport_manager->viewport_dividers();
	for (size_t index = 0; index < viewport_rectangles.size(); ++index)
	{
		viewport_dividers.push_back(std::make_unique<ViewportDivider>(
			viewport_manager, index, this->render_resources_));
	}

	return viewport_dividers;
}
