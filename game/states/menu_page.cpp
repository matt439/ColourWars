#include "game/pch.h"
#include "game/states/menu_page.h"

using namespace DirectX;
using namespace mattmath;

MenuPage::MenuPage(MenuData* data) : data_(data)
{

}

Vector2F MenuPage::get_widget_position() const
{
	return this->widget_position_;
}
Vector2F MenuPage::get_widget_spacing() const
{
	return this->widget_spacing_;
}
Vector2F MenuPage::get_widget_size() const
{
	return this->widget_size_;
}
void MenuPage::set_widget_position(const Vector2F& widget_position)
{
	this->widget_position_ = widget_position;
}
void MenuPage::set_widget_spacing(const Vector2F& widget_spacing)
{
	this->widget_spacing_ = widget_spacing;
}
void MenuPage::set_widget_size(const Vector2F& widget_size)
{
	this->widget_size_ = widget_size;
}
Vector2F MenuPage::calculate_widget_position(int h_index, int v_index) const
{
	Vector2F result = this->widget_position_;
	result.x += h_index * this->widget_spacing_.x;
	result.y += v_index * this->widget_spacing_.y;
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
	return this->data_;
}
MenuInput* MenuPage::get_input() const
{
	return this->data_->get_input();
}
ResolutionManager* MenuPage::get_resolution_manager() const
{
	return this->data_->get_resolution_manager();
}
Save* MenuPage::get_save() const
{
	return this->data_->get_save();
}
RenderResources* MenuPage::get_render_resources() const
{
	return this->data_->get_render_resources();
}
const AudioResources* MenuPage::get_audio_resources() const
{
	return this->data_->get_audio_resources();
}
//SpriteBatch* MenuPage::get_sprite_batch() const
//{
//	return this->data_->get_sprite_batches()->at(0);
//}
ViewportManager* MenuPage::get_viewport_manager() const
{
	return this->data_->get_viewport_manager();
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
		this->get_viewport_manager()->apply_player_viewport(
			static_cast<int>(i), deferred_context, sprite_batch);

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
	std::vector<ID3D11CommandList*> command_lists(deferred_contexts->size(), nullptr);
	auto sprite_batches = this->get_data()->get_sprite_batches();
	auto partitioner = this->get_data()->get_partitioner();
	int num_threads = this->get_data()->get_thread_pool()->get_max_num_threads();

	// partition MObjects objects
	auto partitioned_mobjects =
		partitioner->partition(mobjects->size(), num_threads);

	auto thread_pool = this->get_data()->get_thread_pool();

	// draw MObjects objects
	for (int i = 0; i < partitioned_mobjects.size(); i++)
	{
		thread_pool->add_task([this, i, &partitioned_mobjects, mobjects, deferred_contexts,
			&command_lists, sprite_batches]()
			{
				this->draw_range_of_mobjects_in_viewports(
					partitioned_mobjects[i].first, partitioned_mobjects[i].second,
					mobjects, deferred_contexts, &command_lists, sprite_batches);
			});
	}
	thread_pool->wait_for_tasks_to_complete();

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

void MenuPage::draw_range_of_mobjects_in_viewports(int start, int end,
	std::vector<std::pair<MObject*, ID3D11SamplerState*>>* mobjects,
	std::vector<ID3D11DeviceContext*>* deferred_contexts,
	std::vector<ID3D11CommandList*>* command_lists,
	std::vector<SpriteBatch*>* sprite_batches)
{
	for (int i = start; i < end; i++)
	{
		// Use the contexts passed in rather than re-fetching the same ones
		// through GameData from a worker thread.
		this->draw_mobject_in_viewports(
			deferred_contexts->at(i),
			command_lists->at(i), sprite_batches->at(i), mobjects->at(i).first, mobjects->at(i).second);
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
