#ifndef MH_H
#define MH_H

#include "IGameObject.h"
#include "TextureObject.h"
#include "TextDropShadow.h"

class MObject : public IGameObject
{
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
private:
	std::string _name = "error_name";
	bool _hidden = false;
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
public:
	MWidget() = default;
	MWidget(const std::string& name, bool hidden = false);
	virtual ~MWidget() = default;

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

#endif // !MH_H