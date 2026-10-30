#pragma once

#include "engine/ui/widget.h"
#include "game/states/menu_page.h"
#include "game/states/menu_panel.h"
#include "engine/render/colour.h"
#include "engine/math/shape.h"
#include "engine/math/vector2f.h"

#include <memory>
#include <string>
#include <vector>

// A settings panel: Controls / audio and the four pages it opens (Pad sticks,
// Keyboard keys, Pad buttons, Profiles). Each is pushed above Options or Pause
// with no result, so its only way out is close(), which pops it like Back;
// leaving one never resumes a paused match. A non-negative opening slot is the
// pause owner's input slot; -1 allows the main menu's any-device navigation.
// All five draw on the same paper card from controls_settings.
class SettingsPanelPage : public MenuPage
{
public:
	SettingsPanelPage(MenuContext* context, int opening_slot);
	~SettingsPanelPage() override = default;
	void init() override = 0;
	bool covers_screen() const override { return true; }
	// Closes like Back.
	bool debug_dismiss(bool leaving_match) override;
protected:
	void update_page(float dt) override = 0;
	// The paper draws its own focus card.
	bool shows_focus_backing() const override { return false; }

	const ControlsSettingsPresentation& panel_look() const;

	// The input slot that opened the page, or -1 for any device.
	int opening_slot() const;
	// The opening slot's input as menu_input_for_owner reads it.
	ProcessedMenuInput opening_slot_input(const std::vector<ProcessedMenuInput>& inputs) const;

	// The menu style with the panel's item font.
	MenuStyle panel_style() const;
	// Lays widgets out on the panel's grid and builds the full-screen
	// background, the centred paper card and the two widget trees holding them.
	// `style` is panel_style(), with whatever the page changed.
	void build_panel(const MenuStyle& style);
	// Adds a text widget owned by this page to the text tree, in the panel's
	// ink unless a colour is given.
	labrador::UiText* add_text(const char* name, const std::wstring& value,
		const std::string& font, const mattmath::Vector2F& position);
	labrador::UiText* add_text(const char* name, const std::wstring& value,
		const std::string& font, const mattmath::Vector2F& position, const labrador::Colour& colour);
	// Scales `widget` down, never up, to fit `available`.
	static void fit_text(labrador::UiText& widget, const mattmath::RectangleF& available);
	// The status line's box: from controls_settings.status_position to the
	// paper's padded right edge and down to the footer.
	mattmath::RectangleF status_limit(const MenuStyle& style) const;

	// Plays the cancel sound, forgets armed presses and pops this page.
	void close();

	std::unique_ptr<labrador::UiTexture> background_;
	std::unique_ptr<MenuPaperArtwork> paper_;
	std::vector<std::unique_ptr<labrador::UiText>> text_;
	labrador::UiText* status_ = nullptr;
private:
	int opening_slot_ = -1;
};
