#ifndef UPDATER_H
#define UPDATER_H

class Updater
{
private:
	const float* _dt = nullptr;
protected:
	const float get_dt() const { return *_dt; }
public:
	Updater(const float* dt) : _dt(dt) {}
};

#endif // UPDATER_H
