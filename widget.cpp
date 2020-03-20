#include <utility>
#include "widget.h"

using namespace std;

Widget::Widget()
	: _x(0)
	, _y(0)
	, _w(0)
	, _h(0)
	, _state(IDLE)
{}

Widget::Widget(Widget&& other) 
	: _x(other._x)
	, _y(other._y)
	, _w(other._w)
	, _h(other._h)
	, _state(exchange(other._state, DISABLED))
	, _handler(exchange(other._handler, function<void(Widget&)>()))
{}

void Widget::set_position(int x, int y)
{
	_x = x;
	_y = y;
}

void Widget::get_position(int *x, int *y) const
{
	if (x) *x = _x;
	if (y) *y = _y;
}

void Widget::get_size(int *w, int *h) const
{
	if (w) *w = _w;
	if (h) *h = _h;
}

bool Widget::contains(int x, int y) const
{
	return (_x <= x && x <= _x + _w)
		&& (_y <= y && y <= _y + _h);
}

void Widget::set_handler(function<void(Widget&)>&& f)
{
	swap(_handler, f);
}

void Widget::trigger()
{
	if (_handler && _state != DISABLED)
		_handler(*this);
}

void Widget::set_state(const State s)
{
	_state = s;
}

Widget::State Widget::get_state() const
{
	return _state;
}