#ifndef MENUPAGE_H
#define MENUPAGE_H

#include "Mh.h"
#include "MenuData.h"
#include "State.h"
#include "MattMath.h"

class MenuPage : public State
{
public:
	explicit MenuPage(MenuData* data);
	~MenuPage() override = default;
	void update() override = 0;
	void draw() override = 0;
	void init() override = 0;
protected:
	MattMath::Vector2F get_widget_position() const;
	MattMath::Vector2F get_widget_size() const;
	MattMath::Vector2F get_widget_spacing() const;

	void set_widget_position(const MattMath::Vector2F& widget_position);
	void set_widget_size(const MattMath::Vector2F& widget_size);
	void set_widget_spacing(const MattMath::Vector2F& widget_spacing);

	MattMath::Vector2F calculate_widget_position(int h_index, int v_index) const;
	static MattMath::Vector2F calculate_center_position(
		const MattMath::Vector2F& widget_size,
		const MattMath::Vector2F& resolution);
	static float calculate_center_position(float widget_size, float resolution);
	MenuData* get_data() const;
	MenuInput* get_input() const;
	ResolutionManager* get_resolution_manager() const;
	Save* get_save() const;
	ResourceManager* get_resource_manager() const;
	DirectX::SpriteBatch* get_sprite_batch() const;
	ViewportManager* get_viewport_manager() const;
	void draw_mobject_in_viewports(MObject* widget,
		ID3D11SamplerState* sampler_state = nullptr);
	ID3D11SamplerState* get_point_clamp_sampler_state() const;
	std::vector<ProcessedMenuInput> get_menu_inputs() const;
	MattMath::Vector2F get_float_resolution() const;
	MattMath::Vector2I get_int_resolution() const;
private:
	MenuData* _data = nullptr;
	MattMath::Vector2F _widget_position = { 150.0f, 150.0f };
	MattMath::Vector2F _widget_size = { 300.0f, 75.0f };
	MattMath::Vector2F _widget_spacing = { 250.0f, 150.0f };
};
#endif // !MENUPAGE_H
