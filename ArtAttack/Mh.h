#ifndef MH_H
#define MH_H

#include "IGameObject.h"
#include "TextureObject.h"
#include "TextDropShadow.h"

class MObject : public IGameObject
{
private:
	std::string _name = "error_name";
	bool _hidden = false;
public:
	MObject() = default;
	MObject(const std::string& name, bool hidden = false);
	const std::string& get_name() const;

	void draw(const MattMath::Viewport& viewport);

	void set_hidden(bool hidden);
	bool get_hidden() const;

	virtual void scale_size_and_position(const MattMath::Vector2F& scale) = 0;

	virtual void update() = 0;
	virtual void draw(const MattMath::Camera& camera) = 0;
	virtual void draw() = 0;
	virtual bool is_visible_in_viewport(const MattMath::RectangleF& view) const = 0;
};

class MContainer : public MObject
{
public:
	MContainer() = default;
	MContainer(const std::string& name);
	void add_child(MObject* child);
	void remove_child(const std::string& name);
	void remove_child(MObject* child);
	void remove_all_children();
	size_t get_child_count() const;
	std::vector<std::pair<std::string, MObject*>> get_children();

	void scale_objects_to_new_resolution(
		const MattMath::Vector2F& prev_resolution,
		const MattMath::Vector2F& new_resolution);

	void scale_size_and_position(const MattMath::Vector2F& scale) override;

	void update() override;
	void draw(const MattMath::Camera& camera) override;
	void draw() override;
	bool is_visible_in_viewport(const MattMath::RectangleF& view) const override;
private:
	std::vector<std::pair<std::string, MObject*>> _children;
};

class MWidget : public MObject
{
private:
	//std::string _name = "error_name";
	//bool _hidden = false;
	/*std::vector<std::pair<std::string, MWidget*>> _children;*/
	//virtual void draw_self() = 0;
	//virtual void draw_self(const MattMath::Camera& camera) = 0;
	//virtual void scale_and_move_self(const MattMath::Vector2F& scale) = 0;
	//void scale_and_move_children(const MattMath::Vector2F& scale);
	//void draw_children();
	//void draw_children(const MattMath::Viewport& viewport);
	//void draw_children(const MattMath::Camera& camera);
protected:
	//std::vector<std::pair<std::string, MWidget*>> get_children();
	//size_t get_child_count() const;
public:
	MWidget() = default;
	MWidget(const std::string& name, bool hidden = false);
	virtual ~MWidget() = default;

	//void set_hidden(bool hidden);
	//bool get_hidden() const;

	//void add_child(MWidget* child);

	//const std::string& get_name() const;

	//void scale_and_move(const MattMath::Vector2F& scale);

	//void update_resolution(const MattMath::Vector2I& resolution);
	//void update_resolution(const MattMath::Vector2F& prev_resolution,
	//	const MattMath::Vector2F& new_resolution);

	//void draw(const MattMath::Camera& camera) override;
	//void draw() override;
	//void draw(const MattMath::Viewport& viewport);
	//virtual void draw() = 0;

	virtual void scale_size_and_position(const MattMath::Vector2F& scale) = 0;

	virtual void update() = 0;
	virtual void draw(const MattMath::Camera& camera) = 0;
	virtual void draw() = 0;
	virtual bool is_visible_in_viewport(const MattMath::RectangleF& view) const = 0;

	virtual void set_colour(const MattMath::Colour& colour) = 0;
};

class MTexture : public MWidget, public TextureObject
{
public:
	MTexture() = default;
	MTexture(const std::string& name,
		const std::string& sheet_name,
		const std::string& frame_name,
		const MattMath::RectangleF& rectangle,
		DirectX::SpriteBatch* sprite_batch,
		ResourceManager* resource_manager,
		const MattMath::Colour& color = colour_consts::WHITE,
		bool hidden = false,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	void scale_size_and_position(const MattMath::Vector2F& scale) override;

	void update() override;
	void draw(const MattMath::Camera& camera) override;
	void draw() override;
	//void draw(const MattMath::Viewport& viewport) override;
	bool is_visible_in_viewport(const MattMath::RectangleF& view) const override;

	void set_texture(const std::string& sheet_name, const std::string& frame_name);
	void set_sprite_frame(const std::string& frame_name);
	void set_colour(const MattMath::Colour& colour) override;
	void set_position(const MattMath::Vector2F& position);
	void set_position_at_center(const MattMath::Vector2F& position);
	void set_width(float width);
	void set_height(float height);
	void set_size(const MattMath::Vector2F& size);
	void set_position_from_top_right_origin(const MattMath::Vector2F& position);

	const MattMath::RectangleF& get_rectangle() const;
private:
	MattMath::RectangleF _rectangle = MattMath::RectangleF::ZERO;
};

class MText : public MWidget, public Text
{
public:
	MText() = default;
	MText(const std::string& name,
		const std::string& text,
		const std::string& font_name,
		const MattMath::Vector2F& position,
		DirectX::SpriteBatch* sprite_batch,
		ResourceManager* resource_manager,
		const MattMath::Colour& color = colour_consts::WHITE,
		bool hidden = false,
		float scale = 1.0f,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	void scale_size_and_position(const MattMath::Vector2F& scale) override;

	void update() override;
	void draw(const MattMath::Camera& camera) override;
	void draw() override;
	bool is_visible_in_viewport(const MattMath::RectangleF& view) const override;
	void set_colour(const MattMath::Colour& colour) override;
private:

};

class MTextDropShadow : public MWidget, public TextDropShadow
{
public:
	MTextDropShadow() = default;
	MTextDropShadow(const std::string& name,
		const std::string& text,
		const std::string& font_name,
		const MattMath::Vector2F& position,
		DirectX::SpriteBatch* sprite_batch,
		ResourceManager* resource_manager,
		const MattMath::Colour& color = colour_consts::WHITE,
		const MattMath::Colour& shadow_color = colour_consts::BLACK,
		const MattMath::Vector2F& shadow_offset = { 2.0f, 2.0f },
		bool hidden = false,
		float scale = 1.0f,
		float shadow_scale = 1.0f,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	void scale_size_and_position(const MattMath::Vector2F& scale) override;

	void update() override;
	void draw(const MattMath::Camera& camera) override;
	void draw() override;
	bool is_visible_in_viewport(const MattMath::RectangleF& view) const override;
	void set_colour(const MattMath::Colour& colour) override;
};

//class MSprite
//{
//private:
//	std::string _texture_name = "";
//	sprite_frame_reference _sprite_frame_reference = sprite_frame_reference();
//protected:
//	const std::string& get_texture_name() const { return this->_texture_name; }
//	//const std::string& get_sprite_sheet_name() const { return this->_sprite_sheet_name; }
//	//const std::string& get_sprite_frame_name() const { return this->_sprite_frame_name; }
//	const sprite_frame_reference& get_sprite_frame_reference() const { return this->_sprite_frame_reference; }
//	const std::string& get_sprite_sheet_name() const { return this->_sprite_frame_reference.sprite_sheet_name; }
//	const std::string& get_sprite_frame_name() const { return this->_sprite_frame_reference.sprite_frame_name; }
//public:
//	MSprite(const std::string& texture_name) : _texture_name(texture_name) {}
//	MSprite(const sprite_frame_reference& frame_ref) : _sprite_frame_reference(frame_ref) {}
//	void set_texture(const std::string& texture_name) { this->_texture_name = texture_name; }
//	//void set_sprite_sheet(const std::string& sprite_sheet_name) { this->_sprite_sheet_name = sprite_sheet_name; }
//	//void set_sprite_frame(const std::string& sprite_frame_name) { this->_sprite_frame_name = sprite_frame_name; }
//	void set_sprite_frame_reference(const sprite_frame_reference& frame_ref) { this->_sprite_frame_reference = frame_ref; }
//	void set_sprite_sheet_name(const std::string& sprite_sheet_name) { this->_sprite_frame_reference.sprite_sheet_name = sprite_sheet_name; }
//	void set_sprite_frame_name(const std::string& sprite_frame_name) { this->_sprite_frame_reference.sprite_frame_name = sprite_frame_name; }
//};
//class MText
//{
//private:
//	std::wstring _text = std::wstring(L"");
//	std::string _sprite_font_name = "";
//	float _scale = 1.0f;
//protected:
//	const std::string& get_sprite_font_name() const { return this->_sprite_font_name; }
//	float get_scale() { return this->_scale; }
//public:
//	MText(const std::string& sprite_font_name) :
//		_sprite_font_name(sprite_font_name) {}
//	void set_text(const std::wstring& text) { this->_text = text; }
//	void set_text(const std::string& text) { this->_text = std::wstring(text.begin(), text.end()); }
//	std::wstring get_text() { return this->_text; }
//	void set_scale(float scale) { this->_scale = scale; }
//};
//class MLabel : public MWidget, public MText
//{
//private:
//	void draw_self();
//	void draw_self(const MattMath::Viewport& viewport);
//	DirectX::SpriteFont* get_sprite_font() { return this->get_resource_manager()->get_sprite_font(this->get_sprite_font_name()); }
//public:
//	MLabel(const std::string& name,
//		ResourceManager* resource_manager,
//		DirectX::SpriteBatch* sprite_batch,
//		const std::string& sprite_font_name) :
//		MWidget(name, resource_manager, sprite_batch),
//		MText(sprite_font_name) {}
//	void draw() override;
//	void draw(const MattMath::Viewport& viewport) override;
//};
//class MImage : public MWidget, public MSprite
//{
//private:
//	void draw_self();
//	void draw_self(const MattMath::Viewport& viewport);
//	ID3D11ShaderResourceView* get_texture() { return this->get_resource_manager()->get_texture(this->get_texture_name()); }
//	SpriteSheet* get_sprite_sheet() { return this->get_resource_manager()->get_sprite_sheet(this->get_sprite_frame_reference().sprite_sheet_name); }
//public:
//	MImage(const std::string& name,
//		ResourceManager* resource_manager,
//		DirectX::SpriteBatch* sprite_batch,
//		const std::string& texture) :
//		MWidget(name, resource_manager, sprite_batch),
//		MSprite(texture) {}
//	MImage(const std::string& name,
//		ResourceManager* resource_manager,
//		DirectX::SpriteBatch* sprite_batch,
//		const sprite_frame_reference& frame_ref) :
//		MWidget(name, resource_manager, sprite_batch),
//		MSprite(frame_ref) {}
//	void draw() override;
//	void draw(const MattMath::Viewport& viewport) override;
//};

#endif // !MH_H