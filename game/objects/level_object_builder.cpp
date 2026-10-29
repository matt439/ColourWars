#include "game/pch.h"
#include "game/objects/level_object_builder.h"
#include "engine/math/triangle.h"
#include "game/objects/airbrush_shadow.h"
#include "game/objects/drifting_visual.h"
#include "game/horizontal_motion_contract.h"
#include "game/objects/translating_structure_paintable.h"
#include "game/objects/team_gate_structure_paintable.h"
#include "game/team_gate_contract.h"
#include "game/objects/fixed_slab_paintable.h"
#include "game/objects/fixed_slab_shadow.h"
#include "game/objects/translated_airbrush_shadow.h"
#include "game/paint_face_geometry.h"
#include "game/fixed_slab_paint_geometry.h"
#include "engine/collision/narrow_phase.h"
#include <cmath>
#include <initializer_list>
#include <optional>

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
		if (name == "STRUCTURE_FIXED_SLAB") { return CollisionObjectType::structure_fixed_slab; }
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

	HorizontalMotionSpec decode_motion(const JsonValue& object, const RectangleF& anchor)
	{
		const JsonValue motion = object.object("motion");
		if (motion.string("type") != "horizontal")
		{
			throw std::runtime_error(motion.where() + ": only 'horizontal' motion is supported");
		}
		for (const char* key : { "axis", "travel_y", "rotation" })
		{
			if (motion.has(key))
			{
				throw std::runtime_error(motion.where() + ": unsupported motion field '" + key + "'");
			}
		}
		HorizontalMotionSpec result{ anchor, motion.number("travel_x"),
			motion.number("period"), motion.number("phase") };
		// The drive rule is shared with the level editor; only the JSON typing
		// and the message's location are the loader's.
		const auto text = [&motion](const char* key)
		{ return motion.has(key) ? std::optional<std::string>(motion.string(key)) : std::nullopt; };
		try
		{
			const auto drive = horizontal_motion_contract::decode_drive(text("drive"), text("team_a_heads"));
			result.drive = drive.drive;
			result.team_a_heading = drive.team_a_heading;
		}
		catch (const std::invalid_argument& error)
		{
			throw std::runtime_error(motion.where() + ": " + error.what());
		}
		try { HorizontalMotion::validate_spec(result); }
		catch (const std::invalid_argument& error)
		{
			throw std::runtime_error(motion.where() + ": " + error.what());
		}
		return result;
	}

	FixedSlabSpec decode_slab(const JsonValue& object)
	{
		for (const char* field : { "motion", "drift", "rotation", "rectangle", "rectangle_tlbr", "triangle" })
		{
			if (object.has(field))
			{ throw std::runtime_error(object.where() + ": fixed slabs do not support '" + field + "'"); }
		}
		if (object.string("material") != "eraser")
		{ throw std::runtime_error(object.where() + ": fixed slabs require the eraser material"); }
		const JsonValue slab = object.object("slab");
		for (const char* field : { "width", "height", "thickness", "angle", "rotation", "motion" })
		{
			if (slab.has(field))
			{ throw std::runtime_error(slab.where() + ": unsupported fixed slab field '" + field + "'"); }
		}
		const std::string rise = slab.string("rise");
		if (rise != "left" && rise != "right")
		{ throw std::runtime_error(slab.where() + ": rise must be 'left' or 'right'"); }
		const FixedSlabSpec spec{ { slab.number("x"), slab.number("y") },
			rise == "right" ? FixedSlabRise::right : FixedSlabRise::left };
		try { (void)FixedSlabGeometry(spec); }
		catch (const std::invalid_argument& error)
		{ throw std::runtime_error(slab.where() + ": " + error.what()); }
		return spec;
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
	TerrainFaces exposed_faces(const RectangleF& rectangle, std::span<const RectangleF> flat,
		std::span<const PaintPolygon> slabs = {})
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
		if (!slabs.empty())
		{
			faces.top = clip_paint_polygons(faces.top, { 0.0, rectangle.top() }, { 1.0, 0.0 }, slabs);
			faces.bottom = clip_paint_polygons(faces.bottom, { 0.0, rectangle.bottom() }, { 1.0, 0.0 }, slabs);
			faces.left = clip_paint_polygons(faces.left, { rectangle.left(), 0.0 }, { 0.0, 1.0 }, slabs);
			faces.right = clip_paint_polygons(faces.right, { rectangle.right(), 0.0 }, { 0.0, 1.0 }, slabs);
		}
		return faces;
	}

	// An optional Tool Props material; absent keeps the stretched frame.
	std::unique_ptr<const TerrainMaterial> material_of(const JsonValue& json,
		CollisionObjectType type, const Shape& shape, std::span<const RectangleF> flat,
		float mirror_x, RenderResources* render_resources, std::span<const PaintPolygon> slabs = {})
	{
		if (!json.has("material"))
		{
			return nullptr;
		}
		try
		{
			return TerrainMaterial::make(json.string("material"), type, shape,
				exposed_faces(shape.bounding_box(), flat, slabs), mirror_x, render_resources);
		}
		catch (const std::invalid_argument& error)
		{
			throw std::runtime_error(json.where() + ": " + error.what());
		}
	}

	// Platforms, not walls or the stage's foundations.
	constexpr float shadow_caster_limit = 200.0f;

	// The rules every collision record meets before its type is built, and
	// whether it is a team gate.
	bool checked_record_gate(const JsonValue& json)
	{
		if (json.has("drift"))
		{
			throw std::runtime_error(json.where() + ": drift belongs only on decorative Visual entries");
		}
		const std::string type = json.string("type");
		if (json.has("motion") && type != "StructurePaintable")
		{
			throw std::runtime_error(json.where() + ": motion belongs only on flat StructurePaintable entries");
		}
		// A team gate is read by the shared rule, so the editor refuses exactly
		// the gates refused here.
		bool gate = false;
		if (json.has("gate"))
		{
			team_gate_contract::Record record;
			record.gate = json.string("gate");
			record.structure_paintable = type == "StructurePaintable";
			record.moving = json.has("motion");
			if (record.structure_paintable)
			{
				record.solid = json.string("collision_type") == "STRUCTURE_PAINTABLE";
				const JsonValue face = json.object("paintable_faces");
				record.any_paintable_face = face.boolean("left") || face.boolean("top") ||
					face.boolean("right") || face.boolean("bottom");
			}
			try { gate = team_gate_contract::decode(record); }
			catch (const std::invalid_argument& error)
			{
				throw std::runtime_error(json.where() + ": gate: " + error.what());
			}
		}
		return gate;
	}
}

LevelObjectBuilder::LevelObjectBuilder(RenderResources* render_resources,
	const AudioResources* audio_resources,
	const PlaySettings* preferences,
	const PaintTuning* paint, const GatePermissionPresentation* gate_cue,
	const FerryDirectionPresentation* ferry_cue) :
	paint_(paint), gate_cue_(gate_cue), ferry_cue_(ferry_cue),
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
	const bool gate = checked_record_gate(json);
	const std::string type = json.string("type");
	if (type == "FixedSlabPaintable")
	{
		return this->build_fixed_slab(json, team_colours, context);
	}
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
				this->render_resources_, context.slabs));
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

		if (json.has("motion"))
		{
			const HorizontalMotionSpec motion = decode_motion(json, rectangle);
			return std::make_unique<TranslatingStructurePaintable>(
				json.string("sheet_name"), json.string("frame_name"), motion,
				this->render_resources_, this->audio_resources_, this->preferences_, this->paint_,
				collision_type, team_colours, faces, colour_of(json), Vector2F::ZERO,
				SpriteFlip::none, 0.0f,
				material_of(json, collision_type, rectangle, {}, context.mirror_x,
					this->render_resources_), this->ferry_cue_);
		}

		if (gate)
		{
			try
			{
				return std::make_unique<TeamGateStructurePaintable>(
					json.string("sheet_name"), json.string("frame_name"), rectangle,
					this->render_resources_, this->audio_resources_, this->preferences_, this->paint_,
					team_colours, faces, context.flat, colour_of(json),
					material_of(json, collision_type, rectangle, context.solid, context.mirror_x,
						this->render_resources_, context.slabs), context.slabs, this->gate_cue_);
			}
			catch (const std::invalid_argument& error)
			{
				throw std::runtime_error(json.where() + ": gate: " + error.what());
			}
		}

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
				this->render_resources_, context.slabs), context.slabs);
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
	if (json.has("motion"))
	{
		throw std::runtime_error(json.where() + ": motion belongs only on flat StructurePaintable entries");
	}
	if (json.has("gate"))
	{
		throw std::runtime_error(json.where() + ": gate: gate belongs only on a StructurePaintable");
	}
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
		// A moving anchor must not permanently bury a static scoring face or outline.
		// Its complete travel/standing envelope is checked by Level admission.
		if (object.has("motion")) { continue; }
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
		else if (type == "FixedSlabPaintable")
		{
			const FixedSlabGeometry geometry(decode_slab(object));
			PaintPolygon polygon;
			for (std::size_t corner = 0; corner < polygon.size(); ++corner)
			{
				const Vector2F point = geometry.shape().points()[corner];
				polygon[corner] = { point.x, point.y };
			}
			const FixedSlabSpec& spec = geometry.spec();
			const float dy = spec.rise == FixedSlabRise::right ? -80.0f : 80.0f;
			const Vector2F inward = geometry.inward_normal();
			context.slabs.push_back(canonical_slab_paint_polygon(polygon,
				{ spec.top_left.x, spec.top_left.y }, { spec.top_left.x + 160.0f, spec.top_left.y + dy },
				{ inward.x, inward.y }));
		}
	}
	context.mirror_x = (left + right) / 2.0f;
	return context;
}

std::unique_ptr<FixedSlabPaintable>
	LevelObjectBuilder::build_fixed_slab(const JsonValue& json,
		const TeamColour& team_colours, const StageContext& context) const
{
	const std::string type = json.string("type");
	(void)collision_type_of(json, type, { CollisionObjectType::structure_fixed_slab });
	const FixedSlabSpec spec = decode_slab(json);
	const JsonValue face = json.object("paintable_faces");
	const PaintableFaces faces{ face.boolean("left"), face.boolean("top"),
		face.boolean("right"), face.boolean("bottom") };
	return std::make_unique<FixedSlabPaintable>(spec, this->render_resources_, this->audio_resources_,
		this->preferences_, this->paint_, team_colours, faces, context.flat);
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
	// This bounded shape supports clipping against existing flat terrain.
	// Intersecting slabs need a separate shared-boundary/credit contract.
	const auto is_slab = [](const CollisionObject& object)
	{
		const Structure* structure = Collider::of(object).structure();
		return structure != nullptr && structure->slab_geometry() != nullptr;
	};
	for (size_t index = 0; index < collision_objects.size(); ++index)
	{
		const CollisionObject& slab = *collision_objects[index];
		if (!is_slab(slab)) { continue; }
		for (size_t previous = 0; previous < index; ++previous)
		{
			const CollisionObject& other = *collision_objects[previous];
			if (is_slab(other) && narrow_phase(*slab.shape(), *other.shape()))
			{ throw std::runtime_error(json.at(index).where() + ": intersecting fixed slabs are unsupported"); }
		}
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
		if (object.has("motion")) { continue; }
		if (type == "FixedSlabPaintable")
		{
			(void)checked_record_gate(object);
			const auto slab = this->build_fixed_slab(object, TeamColour{}, context);
			shadows.push_back(std::make_unique<FixedSlabShadow>(slab->geometry(), slab->exposed_bottom(), this->render_resources_));
			continue;
		}
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
		const auto exposed = exposed_paint_intervals(rectangle.left(),
			rectangle.right(), rectangle.bottom(), true, false, obstacles);
		const auto intervals = context.slabs.empty() ? exposed : clip_paint_polygons(exposed,
			{ 0.0, rectangle.bottom() }, { 1.0, 0.0 }, context.slabs);
		for (const auto& [low, high] : intervals)
		{
			shadows.push_back(std::make_unique<AirbrushShadow>(
				low, high, rectangle.bottom(), this->render_resources_));
		}
	}
	return shadows;
}

std::vector<std::unique_ptr<GameObject>>
	LevelObjectBuilder::build_moving_shadows(const JsonValue& json,
		const std::vector<std::unique_ptr<CollisionObject>>& objects) const
{
	if (json.size() != objects.size())
	{
		throw std::invalid_argument("Moving shadows require the same constructed collision objects.");
	}
	std::vector<std::unique_ptr<GameObject>> shadows;
	for (size_t index = 0; index < objects.size(); ++index)
	{
		const Structure* structure = Collider::of(*objects[index]).structure();
		const TranslatingStructurePaintable* mover = structure != nullptr ? structure->mover() : nullptr;
		if (mover == nullptr || !json.at(index).has("material")) { continue; }
		const RectangleF anchor = mover->motion_state().spec.anchor;
		if (anchor.width > anchor.height && anchor.height < shadow_caster_limit)
		{
			try { shadows.push_back(std::make_unique<TranslatedAirbrushShadow>(*mover, this->render_resources_)); }
			catch (const std::exception& error)
			{
				throw std::runtime_error(json.at(index).where() + ": motion shadow: " + error.what());
			}
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
	// Only the new paths retain temporary owners for composed admission.
	// Legacy static validation keeps its construction/destruction order.
	bool moving = false;
	for (size_t index = 0; index < collision_objects.size(); ++index)
	{
		moving |= collision_objects.at(index).has("motion") ||
			collision_objects.at(index).string("type") == "FixedSlabPaintable";
	}
	if (moving)
	{
		const auto objects = build_collision_objects(collision_objects, colours);
		build_moving_shadows(collision_objects, objects);
	}
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
