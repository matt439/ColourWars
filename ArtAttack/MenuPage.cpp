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
//SpriteBatch* MenuPage::get_sprite_batch() const
//{
//	return this->_data->get_sprite_batches()->at(0);
//}
ViewportManager* MenuPage::get_viewport_manager() const
{
	return this->_data->get_viewport_manager();
}

void MenuPage::draw_mobject_in_viewports(ID3D11DeviceContext* deferred_context,
	ID3D11CommandList*& command_list, SpriteBatch* sprite_batch,
	MObject* widget,
	ID3D11SamplerState* sampler_state)
{
	if (deferred_context->GetType() != D3D11_DEVICE_CONTEXT_DEFERRED)
	{
		throw std::exception("Deferred context not created");
	}

	std::vector<Viewport> viewports =
	 	this->get_viewport_manager()->get_all_viewports();

	size_t num_viewports = viewports.size();
	for (size_t i = 0; i < num_viewports; i++)
	{
		//this->get_viewport_manager()->apply_player_viewport(i);

		this->get_viewport_manager()->apply_player_viewport(
			i, deferred_context, sprite_batch);

		sprite_batch->Begin(SpriteSortMode_Deferred, nullptr, sampler_state);

		widget->draw(sprite_batch, viewports[i]);

		sprite_batch->End();
	}

	HRESULT hr = deferred_context->FinishCommandList(FALSE, &command_list);
	if (FAILED(hr))
	{
		throw std::exception("Failed to finish command list");
	}
}

void MenuPage::draw_mobjects_in_viewports(std::vector<std::pair<MObject*,
	ID3D11SamplerState*>>* mobjects)
{
	auto deferred_contexts = this->get_data()->get_device_resources()->get_deferred_contexts();
	//D3D11DeviceContext* deferred_context = deferred_contexts->at(0);

	std::vector<ID3D11CommandList*> command_lists(deferred_contexts->size(), nullptr);
	//ID3D11CommandList* command_list = command_lists[0];

	auto sprite_batches = this->get_data()->get_sprite_batches();
	//SpriteBatch* sprite_batch = sprite_batches->at(0);

	for (size_t i = 0; i < mobjects->size(); i++)
	{
		this->draw_mobject_in_viewports(deferred_contexts->at(i), command_lists[i],
			sprite_batches->at(i), mobjects->at(i).first, mobjects->at(i).second);
	}

	auto immediate_context = this->get_data()->get_device_resources()->GetD3DDeviceContext();

	for (size_t i = 0; i < command_lists.size(); i++)
	{
		if (command_lists[i] == nullptr)
		{
			continue;
		}

		immediate_context->ExecuteCommandList(command_lists[i], FALSE);
		command_lists[i]->Release();
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
