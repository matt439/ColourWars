#include "game/pch.h"
#include "game/states/menu_page.h"

using namespace DirectX;
using namespace mattmath;
using namespace labrador;

// Read through the constructor argument: context_ is not initialized
// until after the SoundBankObject base.
MenuPage::MenuPage(MenuContext* context) :
	SoundBankObject(context->data()->presentation()->menu().sound_bank,
		context->data()->audio_resources()),
	context_(context)
{
	const MenuStyle& style = this->menu_style();
	this->direction_sound_ = this->resolve_wave(style.direction_sound);
	this->confirm_sound_ = this->resolve_wave(style.confirm_sound);
	this->cancel_sound_ = this->resolve_wave(style.cancel_sound);
	this->chrome_sheet_ = this->render_resources()->resolve_sprite_sheet("sprite_sheet_1");
	this->chrome_pixel_ = this->render_resources()->sprite_sheet(this->chrome_sheet_)->
		resolve_sprite_frame("pixel");
	this->brand_font_ = this->render_resources()->resolve_sprite_font(style.brand_font);
}

void MenuPage::update(float dt)
{
	if (this->data()->debug().before_page_update(*this, dt))
	{
		return;
	}
	this->update_page(dt);
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
		else if (button == L"B/Start")
		{
			button = L"B/Start/Esc";
		}

		with_keys.push_back({ button, prompt.action });
	}

	this->prompt_bar_ = std::make_unique<PromptBar>(
		"prompt_bar",
		with_keys,
		this->presentation().prompt_bar(),
		this->render_resources());
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
	list.draw_text(this->brand_font_, L"C O L O U R W A R S   /   LOCAL PAINT BATTLES",
		style.brand_position, style.unhighlight, 1.0f, 0.0f, Vector2F::ZERO, 0.0f);

	// Use the focus group's result, so the backing follows keyboard, pad,
	// disabled rows and compound option rows without duplicating navigation.
	const UiWidget* focused = this->focus_.focused(0);
	if (focused != nullptr && !focused->hidden())
	{
		list.set_filter(TextureFilter::point);
		RectangleF panel = focused->bounds();
		panel.inflate(style.focus_padding);
		sheet->draw(list, this->chrome_pixel_, panel, style.focus_panel_colour);
		sheet->draw(list, this->chrome_pixel_, RectangleF(panel.x, panel.y,
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

void MenuPage::play_wave(SoundBank::WaveHandle wave, float volume,
	float pitch, float pan) const
{
	SoundBankObject::play_wave(wave,
		volume * this->save()->play_settings().sfx_gain(), pitch, pan);
}

void MenuPage::play_effect(SoundBank::EffectHandle effect, bool loop,
	float volume, float pitch, float pan) const
{
	SoundBankObject::play_effect(effect, loop,
		volume * this->save()->play_settings().sfx_gain(), pitch, pan);
}

void MenuPage::play_music(SoundBank::EffectHandle effect, float volume) const
{
	SoundBankObject::play_effect(effect, true,
		volume * this->save()->play_settings().music_gain());
}
