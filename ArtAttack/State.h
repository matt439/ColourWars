#ifndef STATE_H
#define STATE_H

class StateContext;

class State
{
public:
	State() {}
	virtual ~State() {}
	virtual void update() = 0;
	virtual void draw() = 0;
	virtual void init() = 0;
	void set_context(StateContext* context);
protected:
	StateContext* get_context();
private:
	StateContext* _context = nullptr;
};
#endif // !STATE_H
