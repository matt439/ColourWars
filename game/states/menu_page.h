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

// A page of a menu, and the four things every one of the twelve in this game
// turned out to be doing identically.
//
// THE SOUND BANK IS A BASE HERE. All four menu families declared
// `: public MenuPage, public labrador::SoundBankObject` and this header named
// it nowhere, so four constructors resolved the same bank from the same
// menu_style().sound_bank and three of them resolved the same three waves out
// of it. A menu page makes sounds; that is not a per-family fact.
class MenuPage : public labrador::State, public labrador::SoundBankObject
{
public:
	explicit MenuPage(MenuContext* context);
	~MenuPage() override = default;
	void update(float dt) override = 0;
	void init() override = 0;

	// Textures behind text, point behind linear, into every viewport.
	//
	// Concrete, and it was twelve byte-identical overrides. Each built the
	// same two-element vector from the same two members and passed it to
	// draw_ui_layers, which is why this is the shape it is rather than a
	// virtual a page fills in: what varies between pages is what goes in the
	// containers, and that is decided in init(). A page that one day needs a
	// third layer overrides this and calls draw_ui_layers itself.
	void draw(labrador::Renderer& renderer) const override;

	// A menu is a box over whatever is behind it, and behind it is either
	// nothing or a match that has to keep drawing (state.h). The main menu's
	// own pages sit at the bottom of the stack, where it makes no difference.
	bool covers_screen() const override { return false; }
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
	// What every page of every menu shares, and what a page hands to the next
	// page it builds. One accessor, where there used to be one per menu family
	// returning one subclass each.
	MenuContext* menu_context() const;

	GameData* data() const;

	// What the shell looks like, and the style every menu family shares.
	//
	// Two accessors rather than one because menu() is what almost every call
	// site wants: the fonts, the shadows and the highlight pair used to be a
	// copied block in each of the four menu headers, and this is the one they
	// collapsed into.
	const Presentation& presentation() const;
	const MenuStyle& menu_style() const;

	MenuLevelSettings* level_settings() const;
	MenuInput* input() const;
	labrador::ResolutionManager* resolution_manager() const;
	Save* save() const;
	labrador::RenderResources* render_resources() const;
	const labrador::AudioResources* audio_resources() const;
	labrador::ViewportManager* viewport_manager() const;

	// What a page draws: its widget trees, each with the filtering it wants,
	// in back-to-front order. Every menu in the game is a call to this and
	// nothing else.
	using UiLayer = std::pair<labrador::UiObject*, labrador::TextureFilter>;

	// Draws each layer into every viewport the layout covers.
	//
	// NO FAN-OUT HERE, and that is the change. This used to partition the
	// *widget list* across the thread pool and index a deferred context and a
	// sprite batch by widget ordinal - so a menu with more widgets than the
	// shell happened to create contexts drew the extras into nothing, and a
	// menu drawn from sixteen threads ran sixteen non-const helpers on the same
	// page. The unit of work is a view (renderer.h), there are at most four of
	// them, and a menu is a handful of quads: the loop below is the whole of it.
	void draw_ui_layers(labrador::Renderer& renderer,
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
	labrador::FocusGroup focus_;

	// What a page draws, in the two layers draw() puts them in. Both were
	// declared on all twelve pages and neither was ever anything else: the
	// textures go in one, the text in the other, and init() fills them.
	//
	// Owned here rather than by the page so that draw() can be concrete. A
	// page still builds its own widgets and still owns those; these are only
	// the two trees they hang from.
	std::unique_ptr<labrador::UiContainer> texture_container_ = nullptr;
	std::unique_ptr<labrador::UiContainer> text_container_ = nullptr;

	// The three the shared MenuStyle names, resolved once here. Every page
	// family resolved these out of the same style block in its own
	// constructor; the sounds that are actually a family's own - an error
	// beep, a window-open chime, the drum roll - stay with that family.
	labrador::SoundBank::WaveHandle direction_sound_;
	labrador::SoundBank::WaveHandle confirm_sound_;
	labrador::SoundBank::WaveHandle cancel_sound_;

	// Says what this page's buttons do. Call it from init() with whatever this
	// page actually reads - and only with what it reads, because a legend that
	// is wrong is worse than no legend at all.
	//
	// It is a separate layer in draw() rather than a child of text_container_,
	// so it does not care whether it is called before or after the page builds
	// its widgets. A page that says nothing simply never calls it.
	void set_prompts(const std::vector<PromptBar::Prompt>& prompts);

	mattmath::Vector2F float_resolution() const;

	// Design space to this viewport's local space, in one place.
	//
	// Widgets hold the geometry they were authored with and never have it
	// rewritten; this is the whole of what makes 1920x1080 layout show up
	// correctly on a 1280x720 screen. Its predecessor was a destructive walk
	// over the widget tree in engine/ui, called at 26 sites, which left no
	// authoritative geometry behind for anything that wrote a position later.
	labrador::Camera ui_camera(const labrador::Viewport& viewport) const;
private:
	MenuContext* context_ = nullptr;
	std::unique_ptr<PromptBar> prompt_bar_ = nullptr;
	mattmath::Vector2F widget_position_ = { 150.0f, 150.0f };
	mattmath::Vector2F widget_size_ = { 300.0f, 75.0f };
	mattmath::Vector2F widget_spacing_ = { 250.0f, 150.0f };
};
