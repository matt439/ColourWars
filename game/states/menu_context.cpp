#include "game/pch.h"
#include "game/states/menu_context.h"

MenuContext::MenuContext(GameData* data, MenuInput* input,
	MenuLevelSettings* level_settings) :
	data_(data), input_(input), level_settings_(level_settings)
{

}

GameData* MenuContext::data() const
{
	return this->data_;
}

MenuInput* MenuContext::input() const
{
	return this->input_;
}

MenuLevelSettings* MenuContext::level_settings() const
{
	return this->level_settings_;
}
