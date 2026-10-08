#pragma once

// Debug-only internals of DebugDirector. Included by game/debug/*.cpp and by
// the acceptance case, always under COLOURWARS_DEV_TOOLS.

#include "game/debug/debug_clock.h"
#include "game/debug/debug_log.h"
#include "game/debug/debug_registry.h"
#include "game/debug/debug_targets.h"
#include "game/debug/debug_text_field.h"
#include "game/debug/match_plan.h"
#include <memory>
#include <optional>

class GameData;
class GameTuning;
class MenuPage;
class Presentation;

namespace labrador
{
	class Application;
}

class DebugDirectorImpl
{
public:
	DebugDirectorImpl(GameTuning* tuning, Presentation* presentation);
	~DebugDirectorImpl();

	DebugDirectorImpl(const DebugDirectorImpl&) = delete;
	DebugDirectorImpl& operator=(const DebugDirectorImpl&) = delete;

	void attach(labrador::Application* application, GameData* data);

	DebugLog& log() { return this->log_; }
	DebugRegistry& registry() { return this->registry_; }
	DebugClock& clock() { return this->clock_; }
	const DebugClock& clock() const { return this->clock_; }
	DebugTextField& field() { return this->field_; }
	GameData* data() const { return this->data_; }

	// Runs a console line and logs its result.
	DebugStatus execute(std::string_view line);

	// Hooks, as described on DebugDirector.
	void run_launch_statements(const std::vector<std::string>& statements);
	void attach_match(DebugMatchTarget& match);
	void detach_match(const DebugMatchTarget& match);
	void before_level_build();
	bool before_match_update(DebugMatchTarget& match);
	void update_match(DebugMatchTarget& match,
		const std::vector<PlayerInputData>& inputs, float dt);
	bool before_page_update(MenuPage& page);

	// The console reports its own lifetime so only one is ever open.
	void set_console_open(bool open);
	bool console_open() const;

	// Short "x0.25 PAUSED posed feedback-off" text, empty when all default.
	std::string status_strip() const;

	// What `players`, `stage`, `visual` and `reload` read and queue.
	const MatchRequest& request() const { return this->request_; }
	bool in_match() const { return this->match_ != nullptr; }

private:
	GameTuning* tuning_ = nullptr;
	Presentation* presentation_ = nullptr;
	labrador::Application* application_ = nullptr;
	GameData* data_ = nullptr;

	DebugLog log_;
	DebugRegistry registry_;
	DebugClock clock_;
	DebugTextField field_;

	DebugMatchTarget* match_ = nullptr;
	MatchRequest request_;
	bool leave_requested_ = false;
	bool console_open_ = false;
	bool waiting_notice_shown_ = false;

	// The visual a fresh match uses when the request names none: the last one
	// a debug plan applied.
	PlayerVisualMode remembered_visual_ = PlayerVisualMode::original;

	// Content validated by `reload`, assigned before the next level is built.
	std::unique_ptr<GameTuning> pending_tuning_;
	std::unique_ptr<Presentation> pending_presentation_;

	void register_commands();
	bool open_console_if_requested();
	void run_binds(DebugMatchTarget& match);
	MatchPlanner planner() const;
	MatchPlanner::ConnectedPads connected_pads() const;
	void apply_pending_content();
	bool has_pending_content() const;

	// Clears the queued request, logging what was dropped and why.
	void drop_request(const std::string& reason);

	// The visual the next plan would use: requested, current match, remembered.
	PlayerVisualMode planned_visual() const;
	void queue_visual(PlayerVisualMode mode);

	DebugStatus reload_content();
	std::vector<std::string> status_lines() const;
};
