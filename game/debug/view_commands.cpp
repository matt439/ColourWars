#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_director_impl.h"

// The overlay variables: stored on the director so every build re-applies
// them, and pushed into the running match immediately.
void DebugDirectorImpl::register_view_commands()
{
	DebugRegistry& registry = this->registry_;

	struct Overlay
	{
		const char* name;
		const char* help;
		bool DebugDirectorImpl::* flag;
	};
	static constexpr Overlay overlays[] = {
		{ "player", "every player's debug text, set on or off (F1 / pad Back still toggle "
			"one player, and this does not track them)",
			&DebugDirectorImpl::overlay_player_ },
		{ "collision", "collision bounds per pane: structures, paintables, jump-throughs, "
			"ramps, players, projectiles, and the out-of-bounds edge (F2)",
			&DebugDirectorImpl::overlay_collision_ },
		{ "camera", "per pane: the camera's scroll border, the camera bounds and the "
			"followed centre (F3)", &DebugDirectorImpl::overlay_camera_ },
		{ "paint", "paint coverage per team and per seat, bottom left (F4)",
			&DebugDirectorImpl::overlay_paint_ },
	};

	for (const Overlay& overlay : overlays)
	{
		const std::string name = std::string("overlay.") + overlay.name;
		bool DebugDirectorImpl::* flag = overlay.flag;
		auto variable = std::make_unique<BoolVariable>(name, overlay.help, false,
			[this, flag] { return this->*flag; },
			[this, flag, name](bool on)
			{
				this->*flag = on;
				this->apply_overlays(flag == &DebugDirectorImpl::overlay_player_);
				return DebugStatus::success(name + " = " + (on ? "on" : "off"));
			});
		DebugMenuTraits menu = variable->menu();
		menu.category = "view";
		variable->set_menu(std::move(menu));
		registry.add(std::move(variable));
	}

	auto command = std::make_unique<LambdaCommand>("overlay",
		"toggle one overlay, or set all on or off; no argument lists them",
		std::vector<DebugArgSpec>{ { "player|collision|camera|paint|all|none", true, []
			{
				return std::vector<std::string>{ "player", "collision", "camera", "paint",
					"all", "none" };
			} } },
		[this](const DebugArgs& args, DebugLog& log)
		{
			if (!args.has(0))
			{
				for (const Overlay& overlay : overlays)
				{
					log.info(std::string("  overlay.") + overlay.name + " = " +
						(this->*overlay.flag ? "on" : "off"));
				}
				return DebugStatus::success();
			}
			const std::string& word = args.text(0);
			if (word == "all" || word == "none")
			{
				for (const Overlay& overlay : overlays)
				{
					this->*overlay.flag = word == "all";
				}
				this->apply_overlays(true);
				return DebugStatus::success("overlays " + std::string(word == "all" ? "on" : "off"));
			}
			for (const Overlay& overlay : overlays)
			{
				if (word == overlay.name)
				{
					const bool on = !(this->*overlay.flag);
					this->*overlay.flag = on;
					this->apply_overlays(overlay.flag == &DebugDirectorImpl::overlay_player_);
					return DebugStatus::success("overlay." + word + " = " + (on ? "on" : "off"));
				}
			}
			return DebugStatus::failure("overlay: expected player, collision, camera, paint, "
				"all or none, got '" + word + "'");
		});
	DebugMenuTraits menu;
	menu.category = "view";
	menu.form = DebugMenuTraits::Form::pick;
	command->set_menu(std::move(menu));
	registry.add(std::move(command));
}

// Player text is pushed only when overlay.player itself was set, so the
// pane overlays leave each player's own F1 / pad Back toggle alone.
void DebugDirectorImpl::apply_overlays(bool player_text_changed)
{
	if (this->match_ != nullptr)
	{
		this->match_->debug_set_overlays({ this->overlay_collision_, this->overlay_camera_ });
		if (player_text_changed)
		{
			this->match_->debug_set_player_text(this->overlay_player_);
		}
	}
}
#endif
