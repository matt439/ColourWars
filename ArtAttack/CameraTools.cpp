#include "pch.h"
#include "CameraTools.h"

using namespace DirectX;
using namespace camera_consts;
using namespace MattMath;

Camera CameraTools::calculate_camera(
	const Vector2F& player_center,
	const Vector2F& viewport_size,
	const Camera& prev_camera,
	const RectangleF& camera_bounds) const
{
	border_thickness scroll_border =
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
		if (camera_rectangle.get_left() < camera_bounds.get_left())
		{
			new_cam.translation.x += camera_bounds.get_left() - camera_rectangle.get_left();
		}
		else if (camera_rectangle.get_right() > camera_bounds.get_right())
		{
			new_cam.translation.x += camera_bounds.get_right() - camera_rectangle.get_right();
		}
		if (camera_rectangle.get_top() < camera_bounds.get_top())
		{
			new_cam.translation.y += camera_bounds.get_top() - camera_rectangle.get_top();
		}
		else if (camera_rectangle.get_bottom() > camera_bounds.get_bottom())
		{
			new_cam.translation.y += camera_bounds.get_bottom() - camera_rectangle.get_bottom();
		}
		return new_cam;
	}
}

border_thickness CameraTools::calculate_camera_scroll_border(
	const Vector2F& viewport_size) const
{
	border_thickness result = {
		viewport_size.x * DEFAULT_BORDER_RATIO_LEFT,
		viewport_size.y * DEFAULT_BORDER_RATIO_TOP,
		viewport_size.x * DEFAULT_BORDER_RATIO_RIGHT,
		viewport_size.y * DEFAULT_BORDER_RATIO_BOTTOM };

	return result;

	//  return RectangleF(
		//viewport_size.x * DEFAULT_BORDER_RATIO_LEFT,
		//viewport_size.y * DEFAULT_BORDER_RATIO_TOP,
		//viewport_size.x * DEFAULT_BORDER_RATIO_RIGHT,
		//viewport_size.y * DEFAULT_BORDER_RATIO_BOTTOM);
}

//Vector3F CameraTools::calculate_camera(const calculate_cam& params) const
//{
//    RECT scroll_border =
//        this->calculate_camera_scroll_border(params.viewport_size);
//    
//    float left_edge = params.prev_camera.x + scroll_border.left;
//    float right_edge = params.prev_camera.x +
//        static_cast<float>(params.viewport_size.x) -
//        scroll_border.right;
//    float top_edge = params.prev_camera.y + scroll_border.top;
//    float bottom_edge = params.prev_camera.y +
//        static_cast<float>(params.viewport_size.y) -
//        scroll_border.bottom;
//
//    //Vector2 change = { 0.0f, 0.0f };
//    Vector3F new_cam = params.prev_camera;
//    // Test top/bottom of screen
//    if (params.player_center.y < top_edge)
//    {
//		new_cam.y += params.player_center.y - top_edge;
//	}
//    else if (params.player_center.y > bottom_edge)
//    {
//		new_cam.y += params.player_center.y - bottom_edge;
//	}
//    // Test left/right of screen
//    if (params.player_center.x < left_edge)
//    {
//        new_cam.x += params.player_center.x - left_edge;
//    }
//    else if (params.player_center.x > right_edge)
//    {
//		new_cam.x += params.player_center.x - right_edge;
//	}
//
//    // Test if camera is outside of camera bounds
//   RectangleI camera_rectangle = RectangleI(new_cam.x, new_cam.y,
//	   params.viewport_size.x, params.viewport_size.y);
//   RECT camera_rect = camera_rectangle.get_win_rect();
//
//   RectangleI camera_bounds = params.camera_bounds;
//   RECT camera_bounds_rect = camera_bounds.get_win_rect();
//
//   if (! params.camera_bounds.contains(camera_rect))
//   {
//       if (camera_rect.left < camera_bounds_rect.left)
//       {
//		   new_cam.x += static_cast<float>(camera_bounds_rect.left) -
//			   static_cast<float>(camera_rect.left);
//	   }
//       else if (camera_rect.right > camera_bounds_rect.right)
//       {
//		   new_cam.x += static_cast<float>(camera_bounds_rect.right) -
//			   static_cast<float>(camera_rect.right);
//	   }
//       if (camera_rect.top < camera_bounds_rect.top)
//       {
//		   new_cam.y += static_cast<float>(camera_bounds_rect.top) -
//			   static_cast<float>(camera_rect.top);
//	   }
//       else if (camera_rect.bottom > camera_bounds_rect.bottom)
//       {
//		   new_cam.y += static_cast<float>(camera_bounds_rect.bottom) -
//			   static_cast<float>(camera_rect.bottom);
//	   }
//   }
//    if (new_cam != params.prev_camera)
//    {
//        return new_cam;
//    }
//    else
//    {
//        return params.prev_camera;
//    }
//}

//RECT CameraTools::calculate_camera_scroll_border(
//    const Vector2F& viewport_size) const
//{
//    RECT result = {
//        static_cast<long>(viewport_size.x * DEFAULT_BORDER_RATIO_LEFT),
//        static_cast<long>(viewport_size.y * DEFAULT_BORDER_RATIO_TOP),
//        static_cast<long>(viewport_size.x * DEFAULT_BORDER_RATIO_RIGHT),
//        static_cast<long>(viewport_size.y * DEFAULT_BORDER_RATIO_BOTTOM) };
//    return result;
//}