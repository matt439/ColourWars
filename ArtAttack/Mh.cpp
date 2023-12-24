#include "pch.h"
#include "Mh.h"

using namespace DirectX;
using namespace MattMath;

#pragma region MObject

MObject::MObject(const std::string& name, bool hidden) :
	_name(name), _hidden(hidden)
{

}
const std::string& MObject::get_name() const
{
	return this->_name;
}
void MObject::draw(const Viewport& viewport)
{
	this->draw(Camera(viewport));
}
void MObject::set_hidden(bool hidden)
{
	this->_hidden = hidden;
}
bool MObject::get_hidden() const
{
	return this->_hidden;
}

#pragma endregion MObject

#pragma region MContainer

MContainer::MContainer(const std::string& name) :
	MObject(name)
{

}
void MContainer::add_child(MObject* child)
{
	this->_children.push_back(std::make_pair(child->get_name(), child));
}
void MContainer::remove_child(const std::string& name)
{
	for (auto it = this->_children.begin(); it != this->_children.end(); ++it)
	{
		if (it->first == name)
		{
			this->_children.erase(it);
			return;
		}
	}
}
void MContainer::remove_child(MObject* child)
{
	for (auto it = this->_children.begin(); it != this->_children.end(); ++it)
	{
		if (it->second == child)
		{
			this->_children.erase(it);
			return;
		}
	}
}
void MContainer::remove_all_children()
{
	this->_children.clear();
}
size_t MContainer::get_child_count() const
{
	return this->_children.size();
}
std::vector<std::pair<std::string, MObject*>> MContainer::get_children()
{
	return this->_children;
}
void MContainer::scale_objects_to_new_resolution(
	const Vector2F& prev_resolution,
	const Vector2F& new_resolution)
{
	Vector2F scale_factor = new_resolution / prev_resolution;
	this->scale_size_and_position(scale_factor);
}
void MContainer::scale_size_and_position(const Vector2F& scale)
{
	for (auto const& child : this->_children)
	{
		child.second->scale_size_and_position(scale);
	}
}
void MContainer::update()
{
	for (auto const& child : this->_children)
	{
		child.second->update();
	}
}
void MContainer::draw(const MattMath::Camera& camera)
{
	for (auto const& child : this->_children)
	{
		child.second->draw(camera);
	}
}
void MContainer::draw()
{
	for (auto const& child : this->_children)
	{
		child.second->draw();
	}
}
bool MContainer::is_visible_in_viewport(const MattMath::RectangleF& view) const
{
	for (auto const& child : this->_children)
	{
		if (child.second->is_visible_in_viewport(view))
		{
			return true;
		}
	}
	return false;
}

#pragma endregion MContainer

#pragma region MWidget


MWidget::MWidget(const std::string& name, bool hidden) :
	MObject(name, hidden)
{

}
//const std::string& MWidget::get_name() const
//{
//	return this->_name;
//}
//void MWidget::set_hidden(bool hidden)
//{
//	this->_hidden = hidden;
//}
//bool MWidget::get_hidden() const
//{
//	return this->_hidden;
//}
//void MWidget::add_child(MWidget* child)
//{
//	this->_children.push_back(std::make_pair(child->get_name(), child));
//}
//std::vector<std::pair<std::string, MWidget*>> MWidget::get_children()
//{ 
//	return this->_children;
//}
//size_t MWidget::get_child_count() const
//{
//	return this->_children.size();
//}
//void MWidget::draw_children()
//{
//	for (auto const& child : this->_children)
//	{
//		child.second->draw();
//	}
//}
//void MWidget::draw_children(const Viewport& viewport)
//{
//	for (auto const& child : this->_children)
//	{
//		child.second->draw(viewport);
//	}
//}
//void MWidget::draw_children(const Camera& camera)
//{
//	for (auto const& child : this->_children)
//	{
//		child.second->draw(camera);
//	}
//}
//void MWidget::draw(const Viewport& viewport)
//{
//	this->draw(Camera(viewport));
//}
//void MWidget::draw(const Camera& camera)
//{
//	this->draw_self(camera);
//	this->draw_children(camera);
//}
//void MWidget::draw()
//{
//	this->draw_self();
//	this->draw_children();
//}

//void MWidget::update_resolution(const Vector2I& resolution)
//{
//	this->update_resolution(Vector2F(static_cast<float>(resolution.x),
//		static_cast<float>(resolution.y)));
//}

//void MWidget::update_resolution(const Vector2F& prev_resolution,
//	const Vector2F& new_resolution)
//{
//	//Vector2F size = this->get_size();
//	//Vector2F position = this->get_position();
//	//float scale_factor_x = resolution.x / size.x;
//	//float scale_factor_y = resolution.y / size.y;
//	//Vector2F scale_factor = Vector2F(scale_factor_x, scale_factor_y);
//
//	////scale window's children
//	//for (auto const& child : this->get_children())
//	//{
//	//	Vector2F child_position = child.second->get_position();
//	//	if (child_position != Vector2F(0.0f))
//	//	{
//	//		Vector2F child_relative_position = child_position / size;
//	//		child.second->
//	//			set_position(child_relative_position * resolution);
//	//	}
//	//	child.second->scale_size(scale_factor);
//	//}
//	////scale window
//	//this->scale_size(scale_factor);
//	//if (position != Vector2F(0.0f))
//	//{
//	//	Vector2F relative_position = position / size;
//	//	this->set_position(relative_position * resolution);
//	//}
//
//	Vector2F scale_factor = new_resolution / prev_resolution;
//	this->scale_and_move(scale_factor);
//}
//void MWidget::scale_and_move(const Vector2F& scale)
//{
//	this->scale_and_move_self(scale);
//	this->scale_and_move_children(scale);
//}
//void MWidget::scale_and_move_children(const Vector2F& scale)
//{
//	for (auto const& child : this->_children)
//	{
//		child.second->scale_and_move(scale);
//	}
//}

#pragma endregion MWidget

#pragma region MTexture

MTexture::MTexture(const std::string& name,
	const std::string& sheet_name,
	const std::string& frame_name,
	const RectangleF& rectangle,
	SpriteBatch* sprite_batch,
	ResourceManager* resource_manager,
	const Colour& color,
	bool hidden,
	float rotation,
	const Vector2F& origin,
	SpriteEffects effects,
	float layer_depth) :
	MWidget(name, hidden),
	TextureObject(sheet_name, frame_name, sprite_batch,
		resource_manager, color, rotation, origin, effects, layer_depth),
	_rectangle(rectangle)
{

}
void MTexture::update()
{
	return;
}
bool MTexture::is_visible_in_viewport(const RectangleF& view) const
{
	return this->is_visible_in_viewport(view);
}
void MTexture::set_texture(const std::string& sheet_name,
	const std::string& frame_name)
{
	this->set_sprite_sheet_name(sheet_name);
	this->set_element_name(frame_name);
}
void MTexture::set_sprite_frame(const std::string& frame_name)
{
	this->set_element_name(frame_name);
}
void MTexture::set_colour(const Colour& colour)
{
	this->SpriteSheetObject::set_colour(colour);
}
void MTexture::scale_size_and_position(const Vector2F& scale)
{
	this->_rectangle.scale_size_and_position(scale);
}
void MTexture::draw(const Camera& camera)
{
	if (this->MWidget::get_hidden())
	{
		return;
	}
	this->TextureObject::draw(this->_rectangle, camera);
}
void MTexture::draw()
{
	if (this->MWidget::get_hidden())
	{
		return;
	}
	this->TextureObject::draw(this->_rectangle);
}
const RectangleF& MTexture::get_rectangle() const
{
	return this->_rectangle;
}
void MTexture::set_position(const Vector2F& position)
{
	this->_rectangle.set_position(position);
}
void MTexture::set_position_at_center(const Vector2F& position)
{
	this->_rectangle.set_position_at_center(position);
}
void MTexture::set_width(float width)
{
	this->_rectangle.set_width(width);
}
void MTexture::set_height(float height)
{
	this->_rectangle.set_height(height);
}
void MTexture::set_size(const Vector2F& size)
{
	this->_rectangle.set_size(size);
}
void MTexture::set_position_from_top_right_origin(const Vector2F& position)
{
	this->_rectangle.set_position_from_top_right(position);
}

#pragma endregion MTexture

#pragma region MText

MText::MText(const std::string& name,
	const std::string& text,
	const std::string& font_name,
	const Vector2F& position,
	SpriteBatch* sprite_batch,
	ResourceManager* resource_manager,
	const Colour& color,
	bool hidden,
	float scale,
	float rotation,
	const Vector2F& origin,
	SpriteEffects effects,
	float layer_depth) :
	MWidget(name, hidden),
	Text(text, font_name, position, sprite_batch,
		resource_manager, color,
		scale, rotation, origin, effects, layer_depth)
{

}
void MText::scale_size_and_position(const Vector2F& scale)
{
	this->set_position(this->get_position() * scale);
	this->set_scale(this->get_scale() * scale.x);
}
void MText::draw(const Camera& camera)
{
	if (this->MWidget::get_hidden())
	{
		return;
	}
	this->Text::draw(camera);
}
void MText::draw()
{
	if (this->MWidget::get_hidden())
	{
		return;
	}
	this->Text::draw();

}
void MText::update()
{
	return;
}
bool MText::is_visible_in_viewport(const RectangleF& view) const
{
	// TODO: implement text bounding box
	return true;
}
void MText::set_colour(const Colour& colour)
{
	this->Text::set_colour(colour);
}

#pragma endregion MText

#pragma region MTextDropShadow

MTextDropShadow::MTextDropShadow(const std::string& name,
	const std::string& text,
	const std::string& font_name,
	const Vector2F& position,
	SpriteBatch* sprite_batch,
	ResourceManager* resource_manager,
	const Colour& color,
	const Colour& shadow_color,
	const Vector2F& shadow_offset,
	bool hidden,
	float scale,
	float shadow_scale,
	float rotation,
	const Vector2F& origin,
	SpriteEffects effects,
	float layer_depth) :
	MWidget(name, hidden),
	TextDropShadow(text, font_name, position, sprite_batch,
		resource_manager, color, shadow_color, shadow_offset,
		scale, shadow_scale, rotation, origin, effects, layer_depth)
{

}

void MTextDropShadow::scale_size_and_position(const Vector2F& scale)
{
	this->set_position(this->get_position() * scale);
	this->set_scale(this->get_scale() * scale.x);
	this->set_shadow_offset(this->get_shadow_offset() * scale);
	this->set_shadow_scale(this->get_shadow_scale() * scale.x);
}
void MTextDropShadow::draw(const Camera& camera)
{
	if (this->MWidget::get_hidden())
	{
		return;
	}
	this->TextDropShadow::draw(camera);
}
void MTextDropShadow::draw()
{
	if (this->MWidget::get_hidden())
	{
		return;
	}
	this->TextDropShadow::draw();

}
void MTextDropShadow::update()
{
	return;
}
bool MTextDropShadow::is_visible_in_viewport(const RectangleF& view) const
{
	// TODO: implement text bounding box
	return true;
}
void MTextDropShadow::set_colour(const Colour& colour)
{
	this->TextDropShadow::set_colour(colour);
}

#pragma endregion MTextDropShadow

//void MLabel::draw_self()
//{
//	if (this->get_hidden())
//		return;
//
//	this->get_sprite_font()->DrawString(
//		this->get_sprite_batch(),
//		this->get_text().c_str(),
//		this->get_position().get_xm_vector(),
//		this->get_colour().get_xm_vector(),
//		0.0f,
//		Vector2F::ZERO.get_xm_vector(),
//		this->get_scale(),
//		SpriteEffects_None, 0.0f);
//}
//
//void MLabel::draw_self(const Viewport& viewport)
//{
//	if (this->get_hidden())
//		return;
//
//	Vector2F position = this->get_position();
//	position.x -= viewport.x;
//	position.y -= viewport.y;
//
//	this->get_sprite_font()->DrawString(
//		this->get_sprite_batch(),
//		this->get_text().c_str(),
//		position.get_xm_vector(),
//		this->get_colour().get_xm_vector(),
//		0.0f,
//		Vector2F::ZERO.get_xm_vector(),
//		this->get_scale(),
//		SpriteEffects_None, 0.0f);
//}
//
//void MLabel::draw()
//{
//	this->draw_self();
//	this->draw_children();
//}
//
//void MLabel::draw(const Viewport& viewport)
//{
//	this->draw_self(viewport);
//	this->draw_children(viewport);
//}
//
//void MImage::draw_self()
//{
//	if (this->get_hidden())
//		return;
//
//	//sprite_frame_reference frame_ref = this->get_sprite_frame_reference();
//	if (this->get_sprite_sheet_name() != "")
//	{
//		this->get_sprite_sheet()->draw(
//			this->get_sprite_batch(),
//			this->get_sprite_frame_name(),
//			this->get_bounding_box_i(),
//			this->get_colour());
//	}
//	else
//	{
//		this->get_sprite_batch()->Draw(
//			this->get_texture(),
//			this->get_bounding_box_i().get_sm_rectangle(),
//			this->get_colour().get_xm_vector());
//	}
//}
//
//void MImage::draw_self(const Viewport& viewport)
//{
//	if (this->get_hidden())
//		return;
//
//	RectangleI bounds = this->get_bounding_box_i();
//	bounds.x -= static_cast<long>(viewport.x);
//	bounds.y -= static_cast<long>(viewport.y);
//	//bounds.x -= 1;
//	//bounds.y -= 1;
//	//bounds.width += 2;
//	//bounds.height += 2;
//
//	//sprite_frame_reference frame_ref = this->get_sprite_frame_reference();
//
//	if (this->get_sprite_sheet_name() != "")
//	{
//		this->get_sprite_sheet()->draw(
//			this->get_sprite_batch(),
//			this->get_sprite_frame_name(),
//			bounds,
//			this->get_colour());
//	}
//	else
//	{
//		this->get_sprite_batch()->Draw(
//			this->get_texture(),
//			bounds.get_win_rect(),
//			this->get_colour().get_xm_vector());
//	}
//}
//
//void MImage::draw()
//{
//	this->draw_self();
//	this->draw_children();
//}
//
//void MImage::draw(const Viewport& viewport)
//{
//	this->draw_self(viewport);
//	this->draw_children(viewport);
//}