#ifndef CAMERATOOLS_H
#define CAMERATOOLS_H

#include "border_thickness.h"
#include "MattMath.h"

namespace camera_consts
{
    constexpr float DEFAULT_BORDER_RATIO_LEFT = 0.2f;
    constexpr float DEFAULT_BORDER_RATIO_TOP = 0.2f;
    constexpr float DEFAULT_BORDER_RATIO_RIGHT = 0.2f;
    constexpr float DEFAULT_BORDER_RATIO_BOTTOM = 0.4f;
}

//struct calculate_cam
//{
//    MattMath::Vector2F player_center = { 0.0f, 0.0f };
//    MattMath::Vector2F viewport_size = { 0.0f, 0.0f };
//    //MattMath::Vector2F border = { 0.0f, 0.0f };
//    MattMath::Vector3F prev_camera = { 0.0f, 0.0f, 0.0f };
//    //DirectX::SimpleMath::Rectangle level_bounds = { 0, 0, 0, 0 };
//    MattMath::RectangleI camera_bounds = { 0, 0, 0, 0 };
//    //MattMath::Vector2F tile_size = { 0.0f, 0.0f };
//};

class CameraTools
{
private:
    //RECT calculate_camera_scroll_border(
    //    const MattMath::Vector2F& viewport_size) const;

    border_thickness calculate_camera_scroll_border(
        const MattMath::Vector2F& viewport_size) const;
public:
    //MattMath::Vector3F calculate_camera(
    //    const calculate_cam& params) const;

    MattMath::Camera calculate_camera(
        const MattMath::Vector2F& player_center,
        const MattMath::Vector2F& viewport_size,
        const MattMath::Camera& prev_camera,
        const MattMath::RectangleF& camera_bounds) const;

};
#endif