#include <iostream>
#include <utility>
#include <cstdlib>
#include <SDL_image.h>
#include "render.h"
#include "button.h"
#include "util.h"

using namespace std;
namespace fs = std::filesystem;

Button::Button(const fs::path& p)
	: Widget()
{
	if (!Util::is_image(p)) {
		cerr << "Internal error: " << p << " is not an image" << endl;
		exit(1);
	}

	// Load image into surface
	_surface = IMG_Load(p.string().c_str());
	if (!_surface) {
		cerr << "Failed to load surface: " << p << endl;
		exit(1);
	}

	// Initialize self as widget
	_w = _surface->w;
	_h = _surface->h / 4;
	_rect.w = _w;
	_rect.h = _h;

	set_state(IDLE);
	_uflag = true;
}

Button::Button(Button&& other)
	: Widget(forward<Widget>(other))
	, _rect(exchange(other._rect, {0}))
{}

void Button::set_state(const State s) 
{
	aquire(_mut);
	Widget::set_state(s);

	/* Change texture rectangle position according
	 * to pre-determined UI sprite sheet format:
	 *
	 *  0 | ACTIVE
	 *  h | FOCUSED
	 * 2h | IDLE
	 * 3h | DISABLED
	 */
	switch (_state) {
		case ACTIVE:
			_rect.y = 0;
			break;
		case FOCUSED:
			_rect.y = _rect.h;
			break;
		case IDLE:
			_rect.y = _rect.h * 2;
			break;
		case DISABLED:
			_rect.y = _rect.h * 3;
			break;
	}
}

void Button::draw() const 
{
	aquire(_mut);
	SDL_Rect dst = { _x, _y, _w, _h };
	RenderWindow::get_instance().render(_texture, &_rect, &dst);
}