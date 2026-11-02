#pragma once

#include "engine/render/viewport_manager.h"

// Restores the screen layout that was active when it was made. GameLevel::init
// makes one before it sets a match layout, so only a screen that changed the
// layout puts it back.
class ScreenLayoutRestore
{
public:
	explicit ScreenLayoutRestore(labrador::ViewportManager& viewports) :
		viewports_(viewports), previous_(viewports.layout())
	{
	}
	~ScreenLayoutRestore() { this->viewports_.set_layout(this->previous_); }
	ScreenLayoutRestore(const ScreenLayoutRestore&) = delete;
	ScreenLayoutRestore& operator=(const ScreenLayoutRestore&) = delete;

private:
	labrador::ViewportManager& viewports_;
	labrador::ScreenLayout previous_;
};
