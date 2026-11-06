#include "game/pch.h"
// Carried from Labrador v0.2.0; v0.3.0 removes this type (730dd99,
// "A game that used one should copy the class ... into its own source").
// Shape unchanged; it lives in colourwars:: so it can never be ambiguous with
// an engine type. See docs/review/engine-reports.md, HYG#25.
#include "game/carried/camera_tools.h"

#include <algorithm>

using namespace labrador;


namespace colourwars
{
	namespace
	{
		// The scroll border, as a fraction of the viewport and with a floor in
		// pixels. Private to the one function that reads them: these tune how the
		// camera chases a player and configure nothing else (CONVENTIONS,
		// Constants and enumerators).
		constexpr float default_border_ratio_left = 0.4f;
		constexpr float default_border_ratio_top = 0.2f;
		constexpr float default_border_ratio_right = 0.4f;
		constexpr float default_border_ratio_bottom = 0.4f;

		constexpr float min_border_left = 400.0f;
		constexpr float min_border_top = 250.0f;
		constexpr float min_border_right = 400.0f;
		constexpr float min_border_bottom = 250.0f;
	}

	Camera CameraTools::calculate_camera(
		const Vector2F& player_center,
		const Vector2F& viewport_size,
		const Camera& prev_camera,
		const RectangleF& camera_bounds) const
	{
		BorderThickness scroll_border =
			this->calculate_camera_scroll_border(viewport_size);

		float left_edge = prev_camera.translation.x + scroll_border.left;
		float right_edge = prev_camera.translation.x + viewport_size.x - scroll_border.right;
		float top_edge = prev_camera.translation.y + scroll_border.top;
		float bottom_edge = prev_camera.translation.y + viewport_size.y - scroll_border.bottom;


		Camera new_cam = prev_camera;
		// Test top/bottom of screen
		if (player_center.y < top_edge)
		{
			new_cam.translation.y += player_center.y - top_edge;
		}
		else if (player_center.y > bottom_edge)
		{
			new_cam.translation.y += player_center.y - bottom_edge;
		}
		// Test left/right of screen
		if (player_center.x < left_edge)
		{
			new_cam.translation.x += player_center.x - left_edge;
		}
		else if (player_center.x > right_edge)
		{
			new_cam.translation.x += player_center.x - right_edge;
		}

		// Test if camera is outside of camera bounds
		RectangleF camera_rectangle = RectangleF(new_cam.translation.x,
			new_cam.translation.y,
			viewport_size.x, viewport_size.y);

		if (camera_bounds.contains(camera_rectangle))
		{
			return new_cam;
		}
		else
		{
			if (camera_rectangle.left() < camera_bounds.left())
			{
				new_cam.translation.x += camera_bounds.left() - camera_rectangle.left();
			}
			else if (camera_rectangle.right() > camera_bounds.right())
			{
				new_cam.translation.x += camera_bounds.right() - camera_rectangle.right();
			}
			if (camera_rectangle.top() < camera_bounds.top())
			{
				new_cam.translation.y += camera_bounds.top() - camera_rectangle.top();
			}
			else if (camera_rectangle.bottom() > camera_bounds.bottom())
			{
				new_cam.translation.y += camera_bounds.bottom() - camera_rectangle.bottom();
			}
			return new_cam;
		}
	}

	BorderThickness CameraTools::calculate_camera_scroll_border(
		const Vector2F& viewport_size)
	{
		BorderThickness result =
		{
			viewport_size.x * default_border_ratio_left,
			viewport_size.y * default_border_ratio_top,
			viewport_size.x * default_border_ratio_right,
			viewport_size.y * default_border_ratio_bottom
		};

		// Half the viewport is a hard geometric limit and the pixel floor is a
		// preference, so the limit is applied last and wins. It has to: with
		// opposing borders each capped at half, left + right can never exceed
		// the width and top + bottom can never exceed the height, which is the
		// invariant calculate_camera() reads them under - it derives
		// left_edge/right_edge and top_edge/bottom_edge from them and assumes
		// the first of each pair is the smaller.
		//
		// labrador::clamp cannot express this. With a floor above the ceiling
		// it returns the floor for a value below the floor and the ceiling for
		// anything else, so which bound wins depends on the value. At 1280x720
		// two-player - the default resolution, and what an unparseable save
		// file coerces to - each pane is 1280x360 and both vertical floors are
		// 250 against a ceiling of 180, so a floor that won would put top_edge
		// 140 px *below* bottom_edge, and a motionless player would satisfy
		// both branches in turn, every frame.
		result.left = std::min(std::max(result.left, min_border_left),
			viewport_size.x / 2.0f);
		result.right = std::min(std::max(result.right, min_border_right),
			viewport_size.x / 2.0f);
		result.top = std::min(std::max(result.top, min_border_top),
			viewport_size.y / 2.0f);
		result.bottom = std::min(std::max(result.bottom, min_border_bottom),
			viewport_size.y / 2.0f);

		return result;
	}
}
