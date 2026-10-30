#include "game/pch.h"
#include "game/states/stage_preview.h"

#include "game/game_data.h"
#include "game/objects/level_object_builder.h"
#include "game/objects/structure.h"
#include "game/states/menu_page.h"

using namespace labrador;
using namespace mattmath;

StagePreview::StagePreview(const std::string& name, const RectangleF& rectangle,
	GameData* data) :
	UiWidget(name), data_(data), rectangle_(rectangle)
{
}

void StagePreview::set_stage(const std::string& stage)
{
	if (stage == this->stage_)
	{
		return;
	}
	if (stage.empty())
	{
		this->objects_.clear();
		this->collision_objects_.clear();
		this->stage_.clear();
		return;
	}

	const LevelLoadedInfo* level = this->data_->level_infos()->get(stage);
	const LevelObjectBuilder builder(this->data_->render_resources(),
		this->data_->audio_resources(), &this->data_->settings_session()->play_settings(),
		&this->data_->tuning()->paint(), &this->data_->presentation()->gate_permission(),
		&this->data_->presentation()->ferry_direction());

	// Build exactly the objects a fresh match uses, including mirrored ramps
	// and unpainted surfaces. Nothing updates or receives collision contacts.
	// Finish construction before replacing the previous selection.
	auto objects = builder.build_non_collision_objects(
		level->non_collision_objects_json());
	for (auto& shadow : builder.build_shadows(level->collision_objects_json()))
	{
		objects.push_back(std::move(shadow));
	}
	auto collision_objects = builder.build_collision_objects(
		level->collision_objects_json(), TeamColour{});
	for (auto& shadow : builder.build_moving_shadows(level->collision_objects_json(), collision_objects))
	{
		objects.push_back(std::move(shadow));
	}
	// This is a miniature by path, even when its scale reaches ordinary size.
	// Configure only the new objects before replacing the previous selection.
	for (const auto& object : collision_objects)
	{
		if (Structure* structure = Collider::of(*object).structure())
		{
			structure->set_miniature(true);
		}
	}
	const RectangleF world_bounds = level->zoom_out_finish_bounds_rectangle();

	this->objects_ = std::move(objects);
	this->collision_objects_ = std::move(collision_objects);
	this->world_bounds_ = world_bounds;
	this->stage_ = stage;
}

void StagePreview::update(float /*dt*/)
{
}

void StagePreview::draw(DrawList& draw_list) const
{
	if (this->hidden() || this->stage_.empty())
	{
		return;
	}

	// MenuPage uses this same fit for every UI layer. Derive the physical
	// preview pane from it, preserving letterboxing and arbitrary window sizes.
	const Viewport fullscreen = this->data_->viewport_manager()->fullscreen_viewport();
	const Camera menu_camera = Camera::frame(
		RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), fullscreen);
	RectangleF pane = menu_camera.calculate_view_rectangle(this->rectangle_);
	pane.offset(fullscreen.position());
	const Viewport viewport(pane);
	const Camera camera = Camera::frame(this->world_bounds_, viewport);
	draw_list.set_viewport(viewport);
	draw_list.set_camera(camera);

	// The viewport clips decorative art outside the authored overview. Keep
	// the same painter order as Scene: scenery first, then solid structures.
	// The enclosing texture layer's filtering is left unchanged.
	for (const auto& object : this->objects_)
	{
		object->draw(draw_list);
	}
	for (const auto& object : this->collision_objects_)
	{
		object->draw(draw_list);
	}

	draw_list.set_viewport(fullscreen);
	draw_list.set_camera(menu_camera);
}

RectangleF StagePreview::bounds() const
{
	return this->rectangle_;
}

void StagePreview::set_colour(const Colour& /*colour*/)
{
	// Focus tint belongs to the page's card; retain the actual stage colours.
}
