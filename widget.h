#pragma once
#include <functional>

class Widget {
public:
	enum State { DISABLED, IDLE, FOCUSED, ACTIVE };

	Widget();
	Widget(Widget&&);

	void set_position(int, int);
	void get_position(int*, int*) const;

	void get_size(int*, int*) const;
	bool contains(int, int) const;

	void set_handler(std::function<void(Widget&)>&&);
	void trigger();

	virtual void set_state(const State);
	State get_state() const;

	virtual void draw() const {};
protected:
	int _x;
	int _y;
	int _w;
	int _h;

	State _state;
	std::function<void(Widget&)> _handler;
};