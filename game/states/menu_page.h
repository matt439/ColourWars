#pragma once

#include "engine/audio/sound_bank_object.h"
#include "engine/ui/focus.h"
#include "engine/ui/widget.h"
#include "game/states/menu_context.h"
#include "game/states/prompt_bar.h"
#include "engine/core/state.h"
#include "engine/math/shape.h"
#include "engine/math/vector2f.h"
#include "engine/render/camera.h"
#include "engine/render/viewport.h"
#include "engine/render/render_resources.h"
#include "engine/render/sprite_frame.h"
#include "game/debug/debug_targets.h"

namespace menu_consts
{
	// Menu geometry stays in these design units; ui_camera() maps it to the window.
	inline const mattmath::Vector2F DESIGN_RESOLUTION = { 1920.0f, 1080.0f };
}

// Shared menu layout, focus, prompts, and sound-bank access.
// DebugPageTarget is how the developer tools reach the page on top; its
// defaults say no, and the pages with a way out of a match say how.
class MenuPage : public labrador::State, public labrador::SoundBankObject,
	protected DebugPageTarget
{
public:
	explicit MenuPage(MenuContext* context);
	~MenuPage() override = default;

	// Gives the developer tools first look (the console key, a match queued by
	// the console or launch arguments), then runs the page's own update_page.
	void update(float dt) final;
	void init() override = 0;

	// Debug hooks (DebugPageTarget). Only main-menu pages, whose frame starts
	// a match on close, start one; only the pause, controls, results and end
	// pages dismiss themselves; every page forgets armed presses on request.
	bool debug_start_match(const MenuLevelSettings& plan) override;
	bool debug_dismiss(bool leaving_match) override;
	void debug_reset_input(int pad_slot) override;

	// Draw textures, text, and optional prompts once in fullscreen stack order.
	void draw(labrador::Renderer& renderer) const override;

	// Let the state stack draw the match and menus beneath this page.
	bool covers_screen() const override { return false; }
protected:
	virtual void update_page(float dt) = 0;

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
	MenuContext* menu_context() const;

	GameData* data() const;

	const Presentation& presentation() const;
	const MenuStyle& menu_style() const;

	MenuLevelSettings* level_settings() const;
	MenuInput* input() const;
	labrador::ResolutionManager* resolution_manager() const;
	Save* save() const;
	labrador::RenderResources* render_resources() const;
	const labrador::AudioResources* audio_resources() const;
	labrador::ViewportManager* viewport_manager() const;

	// Widget trees and texture filters, ordered back to front.
	using UiLayer = std::pair<labrador::UiObject*, labrador::TextureFilter>;

	// Records fullscreen layers in the final view, after lower states
	// and their world overlays.
	void draw_ui_layers(labrador::Renderer& renderer,
		const std::vector<UiLayer>& layers) const;

	std::vector<ProcessedMenuInput> menu_inputs() const;

	// Unused by pages with their own per-player selection state.
	labrador::FocusGroup focus_;

	// Pages own their widgets and attach them to these texture and text trees.
	std::unique_ptr<labrador::UiContainer> texture_container_ = nullptr;
	std::unique_ptr<labrador::UiContainer> text_container_ = nullptr;

	// Shared menu sounds resolved once per page.
	labrador::SoundBank::WaveHandle direction_sound_;
	labrador::SoundBank::WaveHandle confirm_sound_;
	labrador::SoundBank::WaveHandle cancel_sound_;

	// Menu and results sounds use the user's SFX gain. Music is separate so
	// it keeps its own gain without changing the authored relative volumes.
	void play_wave(labrador::SoundBank::WaveHandle wave, float volume = 1.0f,
		float pitch = 0.0f, float pan = 0.0f) const;
	void play_effect(labrador::SoundBank::EffectHandle effect, bool loop = false,
		float volume = 1.0f, float pitch = 0.0f, float pan = 0.0f) const;
	void play_music(labrador::SoundBank::EffectHandle effect, float volume) const;

	// Optional control legend, drawn after page widgets. Set only actions
	// this page accepts; it can be built before or after the widget trees.
	void set_prompts(const std::vector<PromptBar::Prompt>& prompts);

	mattmath::Vector2F float_resolution() const;

	// Maps unchanged design-space geometry into the current window.
	labrador::Camera ui_camera() const;
private:
	void draw_chrome(labrador::DrawList& list) const;
	labrador::RenderResources::SpriteSheetHandle chrome_sheet_;
	labrador::SpriteSheet::frame_handle chrome_pixel_;
	labrador::FontHandle brand_font_;
	MenuContext* context_ = nullptr;
	std::unique_ptr<PromptBar> prompt_bar_ = nullptr;
	mattmath::Vector2F widget_position_ = { 150.0f, 150.0f };
	mattmath::Vector2F widget_size_ = { 300.0f, 75.0f };
	mattmath::Vector2F widget_spacing_ = { 250.0f, 150.0f };
};
