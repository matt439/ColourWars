#ifndef MENUPAGE_H
#define MENUPAGE_H

#include "Mh.h"
#include "MenuData.h"
#include "State.h"
#include "MattMath.h"

class MenuPage : public State
{
public:
	MenuPage(MenuData* data) : _data(data) {}
	virtual ~MenuPage() {}
	virtual void update() = 0;
	virtual void draw() = 0;
	virtual void init() = 0;
protected:
	MattMath::Vector2F get_widget_position() const;
	MattMath::Vector2F get_widget_size() const;
	MattMath::Vector2F get_widget_spacing() const;

	void set_widget_position(const MattMath::Vector2F& widget_position);
	void set_widget_size(const MattMath::Vector2F& widget_size);
	void set_widget_spacing(const MattMath::Vector2F& widget_spacing);

	MattMath::Vector2F calculate_widget_position(int h_index, int v_index);
	MattMath::Vector2F calculate_center_position(
		const MattMath::Vector2F& widget_size,
		const MattMath::Vector2F& resolution);
	float calculate_center_position(float widget_size, float resolution);
	MenuData* get_data();
	MenuInput* get_input();
	ResolutionManager* get_resolution_manager();
	Save* get_save();
	ResourceManager* get_resource_manager();
	DirectX::SpriteBatch* get_sprite_batch();
	ViewportManager* get_viewport_manager();
	void draw_mobject_in_viewports(MObject* widget,
		ID3D11SamplerState* sampler_state = nullptr);
	ID3D11SamplerState* get_point_clamp_sampler_state();
	std::vector<menu_input> get_menu_inputs();
	MattMath::Vector2F get_float_resolution();
	MattMath::Vector2I get_int_resolution();
private:
	MenuData* _data = nullptr;
	MattMath::Vector2F _widget_position = { 150.0f, 150.0f };
	MattMath::Vector2F _widget_size = { 300.0f, 75.0f };
	MattMath::Vector2F _widget_spacing = { 250.0f, 150.0f };
};
#endif // !MENUPAGE_H
