#pragma once

#include "game/states/menu_context.h"
#include <functional>
#include <map>
#include <memory>

// The painter probe's stand-in for GameMenu or GameLevel as a MenuContext's
// MenuOwner (HYG#44). A check scripts the music clock and the stage folder of
// the context it drives; an owner with no music is silent, so a page exit
// leaves at once, as it does with no owner at all.
class ScriptedMenuOwner final : public MenuOwner
{
public:
	explicit ScriptedMenuOwner(MenuContext& context) : context_(&context) {}

	std::function<void(float)> music;
	std::function<void()> rescan;
	std::function<const UserStageScan*()> scan;

	void update_menu_music(float elapsed) override
	{
		if (this->music) { this->music(elapsed); return; }
		if (this->context_->music_exit_pending()) { this->context_->complete_music_exit(); }
	}
	void rescan_stages() override
	{
		if (this->rescan) { this->rescan(); }
	}
	const UserStageScan* stage_scan() const override
	{
		return this->scan ? this->scan() : nullptr;
	}
private:
	MenuContext* context_ = nullptr;
};

// The context's scripted owner, attached on first use. A context that does
// not wear its owner (a new context at a dead one's address) gets a fresh one.
inline ScriptedMenuOwner& scripted_menu_owner(MenuContext& context)
{
	static std::map<const MenuContext*, std::unique_ptr<ScriptedMenuOwner>> owners;
	std::unique_ptr<ScriptedMenuOwner>& owner = owners[&context];
	if (owner == nullptr || context.owner() != owner.get())
	{
		owner = std::make_unique<ScriptedMenuOwner>(context);
		context.set_owner(owner.get());
	}
	return *owner;
}

inline void script_menu_music(MenuContext& context, std::function<void(float)> music)
{
	scripted_menu_owner(context).music = std::move(music);
}

inline void script_stage_rescan(MenuContext& context, std::function<void()> rescan)
{
	scripted_menu_owner(context).rescan = std::move(rescan);
}

inline void script_stage_scan(MenuContext& context, std::function<const UserStageScan*()> scan)
{
	scripted_menu_owner(context).scan = std::move(scan);
}
