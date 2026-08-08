#pragma once

#include "engine/ui/focus.h"
#include "engine/ui/widget.h"
#include "game/states/menu_data.h"
#include "engine/core/state.h"
#include "engine/math/matt_math.h"

class MenuPage : public artattack::State
{
public:
	explicit MenuPage(MenuData* data);
	~MenuPage() override = default;
	void update() override = 0;
	void draw() override = 0;
	void init() override = 0;
protected:
	mattmath::Vector2F widget_position() const;
	mattmath::Vector2F widget_size() const;
	mattmath::Vector2F widget_spacing() const;

	void set_widget_position(const mattmath::Vector2F& widget_position);
	void set_widget_size(const mattmath::Vector2F& widget_size);
	void set_widget_spacing(const mattmath::Vector2F& widget_spacing);

	mattmath::Vector2F calculate_widget_position(int h_index, int v_index) const;
	static mattmath::Vector2F calculate_center_position(
		const mattmath::Vector2F& widget_size,
		const mattmath::Vector2F& resolution);
	static float calculate_center_position(float widget_size, float resolution);
	MenuData* data() const;
	MenuInput* input() const;
	artattack::ResolutionManager* resolution_manager() const;
	Save* save() const;
	artattack::RenderResources* render_resources() const;
	const artattack::AudioResources* audio_resources() const;
	artattack::ViewportManager* viewport_manager() const;

	void draw_ui_object_in_viewports(ID3D11DeviceContext* deferred_context,
		ID3D11CommandList*& command_list,
		DirectX::SpriteBatch* sprite_batch, artattack::UiObject* ui_object,
		ID3D11SamplerState* sampler_state = nullptr);

	void draw_ui_objects_in_viewports(std::vector<std::pair<artattack::UiObject*,
		ID3D11SamplerState*>>* ui_objects);

	void draw_range_of_ui_objects_in_viewports(int start, int end,
		std::vector<std::pair<artattack::UiObject*, ID3D11SamplerState*>>*ui_objects,
		std::vector<ID3D11DeviceContext*>* deferred_contexts,
		std::vector<ID3D11CommandList*>* command_lists,
		std::vector<DirectX::SpriteBatch*>* sprite_batches);

	ID3D11SamplerState* point_clamp_sampler_state() const;
	std::vector<ProcessedMenuInput> menu_inputs() const;

	// The page's focusable widgets, what activating each one means, and which
	// one the cursor is on. Pages that are not lists - the title card, the
	// per-player select screens with their own confirmation state machines -
	// leave it empty and it costs two empty vectors.
	//
	// It is a member and not a base class. MenuHighlight, which this replaces,
	// was inherited by six pages purely to reach a widget pointer and two
	// colours, with its entire API protected so inheritance was the only way
	// to use it at all. T11 names inheritance a tool of last resort.
	artattack::FocusGroup focus_;
	mattmath::Vector2F float_resolution() const;
	mattmath::Vector2I int_resolution() const;
private:
	MenuData* data_ = nullptr;
	mattmath::Vector2F widget_position_ = { 150.0f, 150.0f };
	mattmath::Vector2F widget_size_ = { 300.0f, 75.0f };
	mattmath::Vector2F widget_spacing_ = { 250.0f, 150.0f };
};
