#pragma once

#include "engine/ui/focus.h"
#include "engine/ui/widget.h"
#include "game/states/menu_data.h"
#include "engine/core/state.h"
#include "engine/math/matt_math.h"

namespace menu_consts
{
	// The resolution every menu is authored against. Widget positions, box
	// sizes and font sizes are all in these units and stay in them; the
	// mapping onto whatever the screen actually is happens once, at draw
	// time, in MenuPage::ui_camera().
	//
	// It is not the *default* resolution, which is 1280x720 - and the four
	// per-menu constants that used to carry that name, one per header, all
	// held 1920x1080 and could have disagreed at any time.
	inline const mattmath::Vector2F DESIGN_RESOLUTION = { 1920.0f, 1080.0f };
}

class MenuPage : public artattack::State
{
public:
	explicit MenuPage(MenuData* data);
	~MenuPage() override = default;
	void update(float dt) override = 0;
	void draw(artattack::Renderer& renderer) const override = 0;
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

	// What a page draws: its widget trees, each with the filtering it wants,
	// in back-to-front order. Every menu in the game is a call to this and
	// nothing else.
	using UiLayer = std::pair<artattack::UiObject*, artattack::TextureFilter>;

	// Draws each layer into every viewport the layout covers.
	//
	// NO FAN-OUT HERE, and that is the change. This used to partition the
	// *widget list* across the thread pool and index a deferred context and a
	// sprite batch by widget ordinal - so a menu with more widgets than the
	// shell happened to create contexts drew the extras into nothing, and a
	// menu drawn from sixteen threads ran sixteen non-const helpers on the same
	// page. The unit of work is a view (renderer.h), there are at most four of
	// them, and a menu is a handful of quads: the loop below is the whole of it.
	void draw_ui_layers(artattack::Renderer& renderer,
		const std::vector<UiLayer>& layers) const;

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

	// Design space to this viewport's local space, in one place.
	//
	// Widgets hold the geometry they were authored with and never have it
	// rewritten; this is the whole of what makes 1920x1080 layout show up
	// correctly on a 1280x720 screen. Its predecessor was a destructive walk
	// over the widget tree in engine/ui, called at 26 sites, which left no
	// authoritative geometry behind for anything that wrote a position later.
	mattmath::Camera ui_camera(const mattmath::Viewport& viewport) const;
private:
	MenuData* data_ = nullptr;
	mattmath::Vector2F widget_position_ = { 150.0f, 150.0f };
	mattmath::Vector2F widget_size_ = { 300.0f, 75.0f };
	mattmath::Vector2F widget_spacing_ = { 250.0f, 150.0f };
};
