#pragma once

#include "engine/collision/collision_object.h"
#include "engine/ui/widget.h"

#include <memory>
#include <string>
#include <vector>

class GameData;

// A static view of the real arena objects in menu design coordinates. The
// page supplies its backing card; the preview owns no match, players or clock.
class StagePreview final : public labrador::UiWidget
{
public:
	StagePreview(const std::string& name, const mattmath::RectangleF& rectangle,
		GameData* data);

	// An empty stage clears the view for the page's Random card. Repeating the
	// current stage does nothing, so focus updates do not reconstruct the arena.
	void set_stage(const std::string& stage);
	void update(float dt) override;
	void draw(labrador::DrawList& draw_list) const override;
	mattmath::RectangleF bounds() const override;
	void set_colour(const labrador::Colour& colour) override;

private:
	GameData* data_;
	mattmath::RectangleF rectangle_;
	mattmath::RectangleF world_bounds_;
	std::string stage_;
	std::vector<std::unique_ptr<labrador::GameObject>> objects_;
	std::vector<std::unique_ptr<labrador::CollisionObject>> collision_objects_;
};
