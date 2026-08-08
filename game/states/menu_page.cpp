#include "game/pch.h"
#include "game/states/menu_page.h"

using namespace DirectX;
using namespace mattmath;
using namespace artattack;

MenuPage::MenuPage(MenuContext* context) : context_(context)
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

MenuContext* MenuPage::menu_context() const
{
	return this->context_;
}
GameData* MenuPage::data() const
{
	return this->context_->data();
}
MenuLevelSettings* MenuPage::level_settings() const
{
	return this->context_->level_settings();
}
MenuInput* MenuPage::input() const
{
	return this->context_->input();
}
ResolutionManager* MenuPage::resolution_manager() const
{
	return this->data()->resolution_manager();
}
Save* MenuPage::save() const
{
	return this->data()->save();
}
RenderResources* MenuPage::render_resources() const
{
	return this->data()->render_resources();
}
const AudioResources* MenuPage::audio_resources() const
{
	return this->data()->audio_resources();
}
ViewportManager* MenuPage::viewport_manager() const
{
	return this->data()->viewport_manager();
}

void MenuPage::draw_ui_layers(Renderer& renderer,
	const std::vector<UiLayer>& layers) const
{
	const std::vector<Viewport> viewports =
		this->viewport_manager()->all_viewports();

	renderer.set_view_count(static_cast<int>(viewports.size()));

	for (size_t i = 0; i < viewports.size(); i++)
	{
		DrawList list = renderer.view(static_cast<int>(i));

		// Both, once, for the whole pane: the viewport clips it and the
		// camera carries design space onto it. Every widget below draws in
		// the units it was authored in and knows about neither.
		list.set_viewport(viewports[i]);
		list.set_camera(this->ui_camera(viewports[i]));

		for (const UiLayer& layer : layers)
		{
			list.set_filter(layer.second);
			layer.first->draw(list);
		}
	}
}

std::vector<ProcessedMenuInput> MenuPage::menu_inputs() const
{
	return this->input()->update_and_get_menu_inputs();
}

Vector2F MenuPage::float_resolution() const
{
	return this->resolution_manager()->resolution_vec();
}

// A Camera maps world to view as (world - translation) * scale. Here "world"
// is design space, and the view a menu draws into is the viewport's own
// space: DrawList::set_viewport has already offset and clipped rendering to
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
