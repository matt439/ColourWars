#pragma once

#include "engine/core/state.h"
#include "game/debug/debug_console_view.h"

class DebugDirectorImpl;
class GameData;

// The frozen developer console: a non-covering state pushed above whatever is
// on top (a menu page or the match). Only the top state updates, so while it
// is open the match is frozen and no game reader sees the keyboard. It draws
// last, over every pane, in window pixels. The editing and drawing are
// DebugConsoleView's, shared with the live console (`console.live 1`).
class DebugConsoleState final : public labrador::State
{
public:
	DebugConsoleState(GameData* data, DebugDirectorImpl* director);
	~DebugConsoleState() override;

	void init() override;
	void update(float dt) override;
	void draw(labrador::Renderer& renderer) const override;
	bool covers_screen() const override { return false; }

private:
	GameData* data_ = nullptr;
	DebugDirectorImpl* director_ = nullptr;
	DebugConsoleView view_{ false };
	bool closing_ = false;

	void close();
};
