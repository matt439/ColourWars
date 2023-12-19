#ifndef VIEWPORTMANAGER_H
#define VIEWPORTMANAGER_H

#include "screen_layout.h"
#include "ResolutionManager.h"
#include "DeviceResources.h"
#include "MattMath.h"
#include "Colour.h"

namespace viewport_consts
{
	constexpr float DIVIDER_THICKNESS = 2.0f;
	const std::string DIVIDER_SHEET_NAME = "sprite_sheet_1";
	const std::string DIVIDER_FRAME_NAME = "pixel";
	const MattMath::Colour DIVIDER_COLOUR = colour_consts::BLACK;
}

class ViewportManager
{
private:
	ResolutionManager* _resolution_manager = nullptr;
	DirectX::SpriteBatch* _sprite_batch = nullptr;
	DX::DeviceResources* _device_resources = nullptr;

	screen_layout _layout = screen_layout::ONE_PLAYER;

	//D3D11_VIEWPORT* _test_vps = new D3D11_VIEWPORT[2];
	//int _player_num = 0;

	int get_player_count_from_layout(screen_layout layout) const;

	D3D11_VIEWPORT calculate_d3d11_viewport(screen_layout layout,
		int player_num, const MattMath::Vector2F& screen_size) const;
	MattMath::Viewport calculate_viewport(screen_layout layout,
		int player_num, const MattMath::Vector2F& screen_size) const;

	MattMath::Viewport get_fullscreen_viewport() const;
	D3D11_VIEWPORT get_fullscreen_d3d11_viewport() const;
public:
	ViewportManager(ResolutionManager* resolution_manager,
		DirectX::SpriteBatch* sprite_batch,
		DX::DeviceResources* device_resources) :
		_resolution_manager(resolution_manager),
		_sprite_batch(sprite_batch),
		_device_resources(device_resources) {}

	void set_layout(screen_layout layout);
	screen_layout get_layout() const { return _layout; }
	//void apply_fullscreen_viewport();
	void apply_player_viewport(int player_num);
	MattMath::Viewport get_player_viewport(int player_num) const;
	std::vector<MattMath::Viewport> get_all_viewports() const;

	MattMath::RectangleF get_camera_adjusted_player_viewport_rect(
		int player_num, const MattMath::Camera& camera) const;

	//std::vector<MattMath::RectangleF> get_viewport_dividers(int player_num) const;
	std::vector<MattMath::RectangleF> get_viewport_dividers() const;

};

#endif // !VIEWPORT_H
