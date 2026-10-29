#include "game/pch.h"
#include "game/states/menu_page.h"

using namespace DirectX;
using namespace mattmath;
using namespace labrador;

// Read through the constructor argument: context_ is not initialized
// until after the SoundBankObject base.
MenuPage::MenuPage(MenuContext* context) :
	colourwars::SoundBankObject(context->data()->presentation()->menu().sound_bank,
		context->data()->audio_resources()),
	focus_motion_(context->data()->presentation()->menu().motion),
	context_(context)
{
	this->input()->reset_directions();
	const MenuStyle& style = this->menu_style();
	this->text_ = this->presentation().menu_text();
	this->prompt_look_ = this->presentation().prompt_bar();
	this->widget_position_ = style.layout.position;
	this->widget_size_ = style.layout.size;
	this->widget_spacing_ = style.layout.spacing;
	this->direction_sound_ = this->resolve_sound(style.direction_sound);
	this->confirm_sound_ = this->resolve_sound(style.confirm_sound);
	this->cancel_sound_ = this->resolve_sound(style.cancel_sound);
	this->chrome_sheet_ = this->render_resources()->resolve_sprite_sheet(style.sprite_sheet);
	this->chrome_pixel_ = this->render_resources()->sprite_sheet(this->chrome_sheet_)->
		resolve_sprite_frame(style.fill_frame);
	this->brand_font_ = this->render_resources()->resolve_sprite_font(style.brand_font);
}

void MenuPage::update(float dt)
{
	this->data()->debug().sync_background_audio(this->context()->active());
	const bool raw_keyboard_owned = this->owns_raw_keyboard_input();
	if (raw_keyboard_owned)
	{
		this->data()->debug().capture_input_frame(*this);
	}
	else if (this->data()->debug().before_page_update(*this, dt, this->context()->active()))
	{
		this->menu_context()->suspend_music();
		this->reset_menu_motion();
		return;
	}
	this->data()->debug().sync_background_audio(this->context()->active());
	if (!this->context()->active() && this->data()->debug().background_scripts())
	{
		this->menu_context()->suspend_music();
		this->reset_menu_motion();
		return;
	}
	this->focus_motion_.observe(this->motion_focus());
	this->focus_motion_.update(this->context()->active() ? dt : 0.0f,
		this->save()->play_settings().reduce_effects);
	for (MenuFocusMotion* motion : this->selection_motion_)
	{
		motion->update(this->context()->active() ? dt : 0.0f,
			this->save()->play_settings().reduce_effects);
	}
	if (!this->context()->active()) { this->reset_menu_motion(); }
	const bool leaving = this->menu_context()->music_exit_pending();
	const MenuStyle& style = this->menu_style();
	if (raw_keyboard_owned) { this->input()->reset_directions(); }
	else
	{
		this->input()->update_directions(dt, style.repeat_delay, style.repeat_interval,
			this->accepts_stick_navigation());
	}
	if (dt > 0.0f && this->context()->active())
	{
		this->menu_context()->update_connection_audio();
		this->menu_context()->update_music(dt);
	}
	if (leaving || this->menu_context()->music_exit_pending()) { return; }
	this->update_page(dt);
	// StateContext defers page destruction until update returns. Observe the
	// final cursor after every input, including programmatic focus changes.
	this->focus_motion_.observe(this->motion_focus());
	if (!this->context()->active()) { this->reset_menu_motion(); }
}

void MenuPage::on_suspend()
{
	this->reset_menu_motion();
}

void MenuPage::on_resume()
{
	this->input()->reset_directions();
	this->reset_menu_motion();
}

void MenuPage::on_deactivated()
{
	this->data()->debug().sync_background_audio(false);
	this->reset_menu_motion();
}

void MenuPage::on_activated()
{
	this->data()->debug().sync_background_audio(true);
}

void MenuPage::register_selection_motion(MenuFocusMotion* motion)
{
	this->selection_motion_.push_back(motion);
}

void MenuPage::reset_menu_motion()
{
	this->focus_motion_.reset();
	for (MenuFocusMotion* motion : this->selection_motion_) { motion->reset(); }
}

const UiWidget* MenuPage::motion_focus() const
{
	const UiWidget* focused = this->focus_.focused(0);
	return focused != nullptr && !focused->hidden() && this->focus_.enabled(focused) ?
		focused : nullptr;
}

Activation MenuPage::activate_focus()
{
	const Activation result = this->focus_.activate(0);
	if (result == Activation::ran) { this->press_focus(); }
	return result;
}

void MenuPage::press_focus()
{
	if (!this->context()->active() || this->save()->play_settings().reduce_effects)
	{
		this->focus_motion_.reset();
		return;
	}
	const UiWidget* focused = this->motion_focus();
	this->focus_motion_.observe(focused);
	if (focused != nullptr) { this->focus_motion_.press(); }
}

bool MenuPage::debug_start_match(const MenuLevelSettings& /*plan*/)
{
	return false;
}

bool MenuPage::debug_dismiss(bool /*leaving_match*/)
{
	return false;
}

void MenuPage::debug_reset_input(int pad_slot)
{
	this->input()->reset_pad_actions(pad_slot);
	this->reset_menu_motion();
}

// Add keyboard equivalents to the shared pad labels. Keep these mappings
// in sync with MenuInput; Left/Right already describes keyboard navigation.
void MenuPage::set_prompts(const std::vector<PromptBar::Prompt>& prompts)
{
	std::vector<PromptBar::Prompt> with_keys;
	with_keys.reserve(prompts.size());

	for (const PromptBar::Prompt& prompt : prompts)
	{
		std::wstring button = prompt.button;
		if (button == L"A")
		{
			button = L"A/Enter";
		}
		else if (button == L"B")
		{
			button = L"B/Esc";
		}
		else if (button == L"X")
		{
			button = L"X/R";
		}
		else if (button == L"B/Start")
		{
			button = L"B/Start/Esc";
		}

		with_keys.push_back({ button, prompt.action });
	}

	this->prompt_bar_ = std::make_unique<PromptBar>(
		"prompt_bar",
		with_keys,
		this->prompt_look_,
		this->render_resources());
}

void MenuPage::set_prompts(MenuPrompt prompt)
{
	this->prompt_bar_ = std::make_unique<PromptBar>("prompt_bar",
		this->text_.prompt(prompt), this->prompt_look_, this->render_resources());
}

void MenuPage::set_prompts(const std::wstring& legend, const PromptBarPresentation& look, float scale)
{
	PromptBarPresentation scaled = look;
	scaled.shadow_offset *= scale;
	this->prompt_bar_ = std::make_unique<PromptBar>("prompt_bar", legend, scaled, this->render_resources());
	this->prompt_bar_->set_scale(scale);
	this->prompt_bar_->set_shadow_scale(scale);
}

void MenuPage::draw(Renderer& renderer) const
{
	std::vector<UiLayer> layers;

	layers.push_back({ this->texture_container_.get(), TextureFilter::point });

	layers.push_back({ this->text_container_.get(), TextureFilter::linear });

	if (this->prompt_bar_ != nullptr)
	{
		layers.push_back({ this->prompt_bar_.get(), TextureFilter::linear });
	}

	this->draw_ui_layers(renderer, layers);
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
const Presentation& MenuPage::presentation() const
{
	return *this->context_->data()->presentation();
}
const MenuStyle& MenuPage::menu_style() const
{
	return this->context_->data()->presentation()->menu();
}
const MenuText& MenuPage::text() const
{
	return this->text_;
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
	// Append to the final view with a fullscreen viewport so menus draw after
	// world overlays and preserve stack order, including nested menus.
	if (renderer.view_count() == 0)
	{
		renderer.set_view_count(1);
	}
	DrawList list = renderer.view(renderer.view_count() - 1);
	const Viewport viewport = this->viewport_manager()->fullscreen_viewport();
	const Camera camera = this->ui_camera();
	list.set_viewport(viewport);
	list.set_camera(camera);

	// Dim the whole window, including the extra area exposed when the menu's
	// design aspect differs from the viewport. Keep the artwork and widgets
	// in their authored geometry so they remain centered without stretching.
	list.set_filter(TextureFilter::point);
	this->render_resources()->sprite_sheet(this->chrome_sheet_)->draw(list,
		this->chrome_pixel_, camera.visible_rectangle(viewport),
		this->menu_style().scrim_colour);

	for (const UiLayer& layer : layers)
	{
		list.set_filter(layer.second);
		if (layer.first == this->text_container_.get())
		{
			this->draw_chrome(list);
		}
		layer.first->draw(list);
	}
}

void MenuPage::draw_chrome(DrawList& list) const
{
	const MenuStyle& style = this->menu_style();
	const SpriteSheet* sheet = this->render_resources()->sprite_sheet(this->chrome_sheet_);
	list.set_filter(TextureFilter::point);

	// Frame the controls consistently, including menus layered over a match.
	sheet->draw(list, this->chrome_pixel_, RectangleF(0.0f, style.footer_y,
		menu_consts::DESIGN_RESOLUTION.x, menu_consts::DESIGN_RESOLUTION.y - style.footer_y),
		style.scrim_colour);
	sheet->draw(list, this->chrome_pixel_, RectangleF(style.brand_position.x,
		style.footer_y, menu_consts::DESIGN_RESOLUTION.x - style.brand_position.x * 2.0f,
		style.rule_height), style.panel_edge_colour);
	list.set_filter(TextureFilter::linear);
	if (this->shows_brand())
	{
		list.draw_text(this->brand_font_, this->text_.brand,
			style.brand_position, style.unhighlight, 1.0f, 0.0f, Vector2F::ZERO, 0.0f);
	}

	// Use the focus group's result, so the backing follows keyboard, pad,
	// disabled rows and compound option rows without duplicating navigation.
	const UiWidget* focused = this->focus_.focused(0);
	if (this->shows_focus_backing() && focused != nullptr && !focused->hidden())
	{
		list.set_filter(TextureFilter::point);
		RectangleF panel = focused->bounds();
		panel.inflate(style.focus_padding);
		sheet->draw(list, this->chrome_pixel_, panel, style.focus_panel_colour);
		// Move only the accent within its existing padding. Text, panel bounds
		// and the focus group's navigation geometry keep their resting layout.
		const bool animate = this->animates_focus_backing() &&
			this->focus_.enabled(focused) && !this->save()->play_settings().reduce_effects;
		const float inward = animate ? std::min(
			style.accent_width * (this->focus_motion_.arrow_offset() + this->focus_motion_.press_fraction()),
			std::max(0.0f, style.focus_padding.x - style.accent_width)) : 0.0f;
		sheet->draw(list, this->chrome_pixel_, RectangleF(panel.x + inward, panel.y,
			style.accent_width, panel.height),
			this->focus_.enabled(focused) ? style.highlight : style.disabled);
	}
	list.set_filter(TextureFilter::linear);
}

std::vector<ProcessedMenuInput> MenuPage::menu_inputs() const
{
	return this->input()->menu_inputs();
}

Vector2F MenuPage::float_resolution() const
{
	return this->resolution_manager()->resolution_vec();
}

// Fit the whole design into the actual window, including arbitrary drag sizes
// and monitors with a different aspect ratio.
Camera MenuPage::ui_camera() const
{
	return Camera::frame(
		RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
		this->viewport_manager()->fullscreen_viewport());
}

MenuSound MenuPage::resolve_sound(const PresentationSound& sound) const
{
	return { this->resolve_wave(sound.name), sound.volume };
}

void MenuPage::play_wave(const MenuSound& sound) const
{
	this->play_wave(sound.wave, sound.volume);
}
