#include "game/pch.h"
#include "game/states/menu_page.h"

using namespace DirectX;
using namespace mattmath;
using namespace artattack;

MenuPage::MenuPage(MenuData* data) : data_(data)
{

}

Vector2F MenuPage::widget_position() const
{
	return this->widget_position_;
}
Vector2F MenuPage::widget_spacing() const
{
	return this->widget_spacing_;
}
Vector2F MenuPage::widget_size() const
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

MenuData* MenuPage::data() const
{
	return this->data_;
}
MenuInput* MenuPage::input() const
{
	return this->data_->input();
}
ResolutionManager* MenuPage::resolution_manager() const
{
	return this->data_->resolution_manager();
}
Save* MenuPage::save() const
{
	return this->data_->save();
}
RenderResources* MenuPage::render_resources() const
{
	return this->data_->render_resources();
}
const AudioResources* MenuPage::audio_resources() const
{
	return this->data_->audio_resources();
}
//SpriteBatch* MenuPage::sprite_batch() const
//{
//	return this->data_->sprite_batches()->at(0);
//}
ViewportManager* MenuPage::viewport_manager() const
{
	return this->data_->viewport_manager();
}

void MenuPage::draw_ui_object_in_viewports(ID3D11DeviceContext* deferred_context,
	ID3D11CommandList*& command_list, SpriteBatch* sprite_batch,
	UiObject* ui_object,
	ID3D11SamplerState* sampler_state)
{
	if (deferred_context->GetType() != D3D11_DEVICE_CONTEXT_DEFERRED)
	{
		throw std::exception("Deferred context not created");
	}

	std::vector<Viewport> viewports =
	 	this->viewport_manager()->all_viewports();

	size_t num_viewports = viewports.size();
	for (size_t i = 0; i < num_viewports; i++)
	{
		this->viewport_manager()->apply_player_viewport(
			static_cast<int>(i), deferred_context, sprite_batch);

		sprite_batch->Begin(SpriteSortMode_Deferred, nullptr, sampler_state);

		ui_object->draw(sprite_batch, this->ui_camera(viewports[i]));

		sprite_batch->End();
	}

	HRESULT hr = deferred_context->FinishCommandList(FALSE, &command_list);
	if (FAILED(hr))
	{
		throw std::exception("Failed to finish command list");
	}
}

void MenuPage::draw_ui_objects_in_viewports(std::vector<std::pair<UiObject*,
	ID3D11SamplerState*>>* ui_objects)
{
	auto deferred_contexts = this->data()->device_resources()->deferred_contexts();
	std::vector<ID3D11CommandList*> command_lists(deferred_contexts->size(), nullptr);
	auto sprite_batches = this->data()->sprite_batches();
	auto partitioner = this->data()->partitioner();
	int num_threads = this->data()->thread_pool()->max_num_threads();

	// partition the objects
	auto partitioned_ui_objects =
		partitioner->partition(ui_objects->size(), num_threads);

	auto thread_pool = this->data()->thread_pool();

	// draw the objects
	for (int i = 0; i < partitioned_ui_objects.size(); i++)
	{
		thread_pool->add_task([this, i, &partitioned_ui_objects, ui_objects, deferred_contexts,
			&command_lists, sprite_batches]()
			{
				this->draw_range_of_ui_objects_in_viewports(
					partitioned_ui_objects[i].first, partitioned_ui_objects[i].second,
					ui_objects, deferred_contexts, &command_lists, sprite_batches);
			});
	}
	thread_pool->wait_for_tasks_to_complete();

	auto immediate_context = this->data()->device_resources()->GetD3DDeviceContext();

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

void MenuPage::draw_range_of_ui_objects_in_viewports(int start, int end,
	std::vector<std::pair<UiObject*, ID3D11SamplerState*>>* ui_objects,
	std::vector<ID3D11DeviceContext*>* deferred_contexts,
	std::vector<ID3D11CommandList*>* command_lists,
	std::vector<SpriteBatch*>* sprite_batches)
{
	for (int i = start; i < end; i++)
	{
		// Use the contexts passed in rather than re-fetching the same ones
		// through GameData from a worker thread.
		this->draw_ui_object_in_viewports(
			deferred_contexts->at(i),
			command_lists->at(i), sprite_batches->at(i), ui_objects->at(i).first, ui_objects->at(i).second);
	}
}

ID3D11SamplerState* MenuPage::point_clamp_sampler_state() const
{
	return this->data()->common_states()->PointClamp();
}

std::vector<ProcessedMenuInput> MenuPage::menu_inputs() const
{
	return this->data()->input()->update_and_get_menu_inputs();
}

Vector2F MenuPage::float_resolution() const
{
	return this->data()->resolution_manager()->resolution_vec();
}

// A Camera maps world to view as (world - translation) * scale. Here "world"
// is design space, and the view a menu draws into is the viewport's own
// space: apply_player_viewport() has already offset and clipped rendering to
// the viewport's screen rectangle, so a widget at design position p has to
// land at p * s - viewport_origin, where s carries 1920 wide onto the screen.
// Solving for the camera's translation gives viewport_origin / s.
//
// One scale rather than two: every supported resolution is 16:9, so the
// horizontal and vertical ratios against a 16:9 design space are the same
// number, and a Camera carries one. A non-16:9 resolution would need a
// letterbox rule, which is a decision nobody has had to make yet - and
// ResolutionManager snaps anything it does not recognise to 720p, so nobody
// can reach it by accident.
Camera MenuPage::ui_camera(const Viewport& viewport) const
{
	const float scale = this->float_resolution().x / menu_consts::DESIGN_RESOLUTION.x;
	return Camera(viewport.position() / scale, scale);
}
