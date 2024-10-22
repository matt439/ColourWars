#include "pch.h"
#include "MenuPage.h"

using namespace DirectX;
using namespace MattMath;

MenuPage::MenuPage(MenuData* data) : _data(data)
{

}

Vector2F MenuPage::get_widget_position() const
{
	return this->_widget_position;
}
Vector2F MenuPage::get_widget_spacing() const
{
	return this->_widget_spacing;
}
Vector2F MenuPage::get_widget_size() const
{
	return this->_widget_size;
}
void MenuPage::set_widget_position(const Vector2F& widget_position)
{
	this->_widget_position = widget_position;
}
void MenuPage::set_widget_spacing(const Vector2F& widget_spacing)
{
	this->_widget_spacing = widget_spacing;
}
void MenuPage::set_widget_size(const Vector2F& widget_size)
{
	this->_widget_size = widget_size;
}
Vector2F MenuPage::calculate_widget_position(int h_index, int v_index) const
{
	Vector2F result = this->_widget_position;
	result.x += h_index * this->_widget_spacing.x;
	result.y += v_index * this->_widget_spacing.y;
	return result;
}

Vector2F MenuPage::calculate_center_position(
	const Vector2F& widget_size,
	const Vector2F& resolution)
{
	return {
		(resolution.x - widget_size.x) / 2.0f,
		(resolution.y - widget_size.y) / 2.0f
	};
}

float MenuPage::calculate_center_position(
	float widget_size, float resolution)
{
	return (resolution - widget_size) / 2.0f;
}

MenuData* MenuPage::get_data() const
{
	return this->_data;
}
MenuInput* MenuPage::get_input() const
{
	return this->_data->get_input();
}
ResolutionManager* MenuPage::get_resolution_manager() const
{
	return this->_data->get_resolution_manager();
}
Save* MenuPage::get_save() const
{
	return this->_data->get_save();
}
ResourceManager* MenuPage::get_resource_manager() const
{
	return this->_data->get_resource_manager();
}
SpriteBatch* MenuPage::get_sprite_batch() const
{
	return this->_data->get_sprite_batch();
}
ViewportManager* MenuPage::get_viewport_manager() const
{
	return this->_data->get_viewport_manager();
}

void MenuPage::draw_mobject_in_viewports(MObject* widget,
	ID3D11SamplerState* sampler_state)
{
	SpriteBatch* sprite_batch = this->get_sprite_batch();

	std::vector<Viewport> viewports =
	 	this->get_viewport_manager()->get_all_viewports();

	size_t num_viewports = viewports.size();
	for (size_t i = 0; i < num_viewports; i++)
	{
		this->get_viewport_manager()->apply_player_viewport(i);
		sprite_batch->Begin(SpriteSortMode_Deferred, nullptr, sampler_state);

		widget->draw(viewports[i]);

		sprite_batch->End();
	}
}
ID3D11SamplerState* MenuPage::get_point_clamp_sampler_state() const
{
	return this->get_data()->get_common_states()->PointClamp();
}

std::vector<ProcessedMenuInput> MenuPage::get_menu_inputs() const
{
	return this->get_data()->get_input()->update_and_get_menu_inputs();
}

Vector2F MenuPage::get_float_resolution() const
{
	return this->get_data()->get_resolution_manager()->get_resolution_vec();
}

Vector2I MenuPage::get_int_resolution() const
{
	return this->get_data()->get_resolution_manager()->get_resolution_ivec();
}
