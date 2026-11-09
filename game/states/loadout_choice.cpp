#include "game/pch.h"
#include "game/states/loadout_choice.h"
#include "game/states/menu_cycle.h"

#include <stdexcept>

LoadoutChoice::LoadoutChoice(WeaponType tool, SubType sub) :
	tool_(tool), sub_(static_cast<std::size_t>(sub))
{
	if (static_cast<std::size_t>(tool) >= selectable_weapon_count || this->sub_ >= sub_count)
	{
		throw std::invalid_argument("LoadoutChoice: a tool or Random and a concrete sub");
	}
}

bool LoadoutChoice::cycle(labrador::Direction direction)
{
	if (this->step_ == Step::tool)
	{
		const WeaponType previous = this->tool_;
		this->tool_ = step_tool(previous, direction, selectable_weapon_count);
		return this->tool_ != previous;
	}
	if (this->step_ == Step::sub)
	{
		const std::size_t previous = this->sub_;
		this->sub_ = wrap_step(previous, sub_choice_count, direction);
		return this->sub_ != previous;
	}
	return false;
}

bool LoadoutChoice::proceed()
{
	if (this->step_ == Step::ready) { return false; }
	this->step_ = this->step_ == Step::tool ? Step::sub : Step::ready;
	return true;
}

bool LoadoutChoice::back()
{
	if (this->step_ == Step::tool) { return false; }
	this->step_ = this->step_ == Step::ready ? Step::sub : Step::tool;
	return true;
}

SubType LoadoutChoice::resolved_sub(unsigned int draw) const
{
	return this->sub_ == random_sub ? static_cast<SubType>(draw % sub_count) : static_cast<SubType>(this->sub_);
}
