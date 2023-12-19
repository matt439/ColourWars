#ifndef STATE_H
#define STATE_H

class StateContext;

class State
{
private:
	StateContext* _context = nullptr;
protected:
	StateContext* get_context();
public:
	State() {}
	virtual ~State() {}
	virtual void update() = 0;
	virtual void draw() = 0;
	virtual void init() = 0;
	void set_context(StateContext* context);
};
#endif // !STATE_H
