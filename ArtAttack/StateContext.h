#ifndef STATECONTEXT_H
#define STATECONTEXT_H

#include "State.h"
#include <memory>

class StateContext
{
private:
	std::unique_ptr<State> _state = nullptr;
public:
	StateContext() {}
	void update();
	void draw();
	void transition_to(std::unique_ptr<State> state);
};

#endif // !STATECONTEXTGENERIC_H
