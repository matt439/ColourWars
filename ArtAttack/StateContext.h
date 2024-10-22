#ifndef STATECONTEXT_H
#define STATECONTEXT_H

#include "State.h"
#include <memory>

class StateContext
{
public:
	virtual ~StateContext() = default;
	StateContext() {}
	void update();
	void draw();
	void transition_to(std::unique_ptr<State> state);
private:
	std::unique_ptr<State> _state = nullptr;
};

#endif // !STATECONTEXTGENERIC_H
