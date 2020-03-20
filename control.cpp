#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include "control.h"
#include "button.h"
#include "util.h"

using namespace std;
namespace fs = std::filesystem;

int SDLCALL watch(void* udata, SDL_Event* evnt)
{
	static_cast<Control*>(udata)->handle(*evnt);
	return 0;
}

Control::Control()
	: _run(true)
	, _update(true)
	, _drag(false)
	, _zoom(1.f)
	, _minzoom(.01f)
	, _index(-1)
	, _win(RenderWindow::get_instance())
	, _cursor(nullptr, [](SDL_Cursor* p) { SDL_FreeCursor(p); })
{
	// Try and set texture filtering to linear
	if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
		cerr << "Warning: Linear texture filtering not enabled" << endl;
	}

	// Create cursor
	set_cursor(SDL_SYSTEM_CURSOR_ARROW);

	// Set event watcher
	SDL_AddEventWatch(watch, this);

	// Create UI components
	_win.get_size(&_winw, &_winh);

	_widgets[0] = make_unique<Button>("res/ui_minus.png");
	_widgets[0]->set_handler([&](Widget& w) {
		_focusx = _winw / 2;
		_focusy = _winh / 2;
		float i, f = modf(_zoom * 10.f, &i);
		zoom(f > 0.09 ? i / 10.f : _zoom - .1f);
	});

	_widgets[1] = make_unique<Text>("100%");
	_percent = (Text*)_widgets[1].get();
	_widgets[1]->set_handler([&](Widget& w) {
		_image->reset();
		fit();
	});

	_widgets[2] = make_unique<Button>("res/ui_plus.png");
	_widgets[2]->set_handler([&](Widget& w) {
		_focusx = _winw / 2;
		_focusy = _winh / 2;
		float i, f = modf(_zoom * 10.f, &i);
		zoom((f > 0.09 ? i / 10.f : _zoom) + .1f);
	});

	_widgets[3] = make_unique<Button>("res/ui_rotate_left.png");
	_widgets[3]->set_handler([&](Widget& w) {
		_image->rotate_ccw();
		fit();
	});

	_widgets[4] = make_unique<Button>("res/ui_rotate_right.png");
	_widgets[4]->set_handler([&](Widget& w) {
		_image->rotate_cw();
		fit();
	});

	_widgets[5] = make_unique<Button>("res/ui_flip_x.png");
	_widgets[5]->set_handler([&](Widget& w) {
		_image->flip_x();
		fit();
	});

	_widgets[6] = make_unique<Button>("res/ui_flip_y.png");
	_widgets[6]->set_handler([&](Widget& w) {
		_image->flip_y();
		fit();
	});

	_widgets[7] = make_unique<Button>("res/ui_first.png");
	_widgets[7]->set_handler([&](Widget& w) {
		load_index(0);
	});

	_widgets[8] = make_unique<Button>("res/ui_left.png");
	_widgets[8]->set_handler([&](Widget& w) {
		size_t i = _index - 1;
		if (i < 0)
			i = _paths.size() - 1;
		load_index(i);
	});

	_widgets[9] = make_unique<Text>("");
	_pagenum = (Text*)_widgets[9].get();
	_widgets[9]->set_handler([&](Widget& w) {
		load_index(0);
	});

	_widgets[10] = make_unique<Button>("res/ui_right.png");
	_widgets[10]->set_handler([&](Widget& w) {
		size_t i = _index + 1;
		if (i >= _paths.size())
			i = 0;
		load_index(i);
	});

	_widgets[11] = make_unique<Button>("res/ui_last.png");
	_widgets[11]->set_handler([&](Widget& w) {
		load_index(_paths.size() - 1);
	});
}

void Control::handle(const SDL_Event& evnt) 
{
	// Do not process events after main loop has quit
	if (!_run)
		return;

	switch (evnt.type) {
		case SDL_QUIT:
			_run = false;
			break;
		case SDL_WINDOWEVENT:
		{
			SDL_WindowEvent we = evnt.window;
			switch (we.event) {
				case SDL_WINDOWEVENT_LEAVE:
				case SDL_WINDOWEVENT_FOCUS_LOST:
				case SDL_WINDOWEVENT_HIDDEN:
				case SDL_WINDOWEVENT_MINIMIZED:
					// Left window
					reset_widgets();
					break;
				case SDL_WINDOWEVENT_RESIZED:
				case SDL_WINDOWEVENT_SIZE_CHANGED:
				case SDL_WINDOWEVENT_MAXIMIZED:
				case SDL_WINDOWEVENT_RESTORED:
				{
					// Update window dimensions & related variables
					_win.get_size(&_winw, &_winh);

					_bar = { 0, _winh - 17, _winw, 17 };
					_winh -= _bar.h;

					int x = _winw / 2;
					int y = _bar.y;

					// Zoom controls
					_widgets[0]->set_position(0, y);
					set_percent();
					_widgets[2]->set_position(67, y);

					// Image controls
					_widgets[3]->set_position(x - 40, y);
					_widgets[4]->set_position(x - 20, y);
					_widgets[5]->set_position(x, y);
					_widgets[6]->set_position(x + 20, y);

					// Page controls
					_widgets[7]->set_position(_winw - 145, y);
					_widgets[8]->set_position(_winw - 125, y);
					set_pagenum();
					_widgets[10]->set_position(_winw - 40, y);
					_widgets[11]->set_position(_winw - 20, y);

					// Adjust image
					if (_zoom == _minzoom) {
						fit();
					} else {
						_focusx = _rect.x;
						_focusy = _rect.y;
						set_minzoom();
						zoom(_zoom);
					}
					break;
				}
			}
			break;
		}
		case SDL_KEYDOWN:
		{
			SDL_Keysym key = evnt.key.keysym;
			SDL_Keycode sym = key.sym;

			if (sym == SDLK_LEFT) {
				_widgets[8]->trigger();
			}
			if (sym == SDLK_RIGHT) {
				_widgets[10]->trigger();
			}
			if (_zoom > _minzoom) {
				if (sym == SDLK_UP
					|| sym == SDLK_PAGEUP)
					drag(0, (int)(_rect.h * .1f));
				if (sym == SDLK_DOWN
					|| sym == SDLK_PAGEDOWN)
					drag(0, (int)(_rect.h * -.1f));
				if (sym == SDLK_HOME)
					_rect.y = 0;
				if (sym == SDLK_END)
					_rect.y = _winh - _rect.h;
				_update = true;
			}
			if (key.mod & KMOD_CTRL) {
				switch (sym) {
					case SDLK_EQUALS:
						_widgets[2]->trigger();
						break;
					case SDLK_MINUS:
						_widgets[0]->trigger();
						break;
					case SDLK_0:
						zoom(1.f);
						break;
					case SDLK_1:
						fit();
						break;
					case SDLK_w:
						_run = false;
						break;
				}
			}

			break;
		}
		case SDL_MOUSEWHEEL:
		{
			SDL_MouseWheelEvent mwe = evnt.wheel;

			// If mouse is outside image, use center as focus
			SDL_GetMouseState(&_focusx, &_focusy);
			if (_focusx < _rect.x || _focusx > _rect.x + _rect.w ||
				_focusy < _rect.y || _focusy > _rect.y + _rect.h) {
				_focusx = _winw / 2;
				_focusy = _winh / 2;
			}

			float i, f = modf(_zoom * 10.f, &i);
			if (sign(mwe.y) > 0) {
				zoom((f > 0.09 ? i / 10.f : _zoom) + .1f);
			} else {
				zoom(f > 0.09 ? i / 10.f : _zoom - .1f);
			}
			break;
		}
		case SDL_MOUSEBUTTONDOWN:
		{
			SDL_MouseButtonEvent mbe = evnt.button;
			Widget* w = find_widget(mbe.x, mbe.y);

			if (w != nullptr) {
				// Activate widget
				w->set_state(Widget::ACTIVE);
				_update = true;
			} else if (mbe.y < _bar.y 
				&& (_rect.w > _winw || _rect.h > _winh)) {
				// Start dragging (clicked anywhere above bar)
				_drag = true;
			}
			break;
		}
		case SDL_MOUSEBUTTONUP:
		{
			SDL_MouseButtonEvent mbe = evnt.button;

			// Widget clicked
			Widget* w = find_widget(mbe.x, mbe.y);
			if (w != nullptr) {
				w->trigger();
				reset_widgets();
				w->set_state(Widget::FOCUSED);
			} else {
				reset_widgets();
			}

			// Stop dragging
			_drag = false;
			break;
		}
		case SDL_MOUSEMOTION:
		{
			SDL_MouseMotionEvent mme = evnt.motion;
			if (!(mme.state ^ SDL_BUTTON_LMASK) && _drag) {
				// Dragging
				drag(mme.xrel, mme.yrel);
			} else if (!mme.state) {
				// Moved on-top of widget
				reset_widgets();
				Widget* w = find_widget(mme.x, mme.y);
				if (w != nullptr) {
					w->set_state(Widget::FOCUSED);
				}

				// Update cursor
				set_cursor(mme.y < _bar.y
					&& (_rect.w > _winw || _rect.h > _winh)
					? SDL_SYSTEM_CURSOR_SIZEALL
					: SDL_SYSTEM_CURSOR_ARROW);
			}
			break;
		}
	}

	// Draw components
	if (_update) {
		_update = false;
		draw();
	}
}

void Control::draw() const
{
	SDL_Renderer* r = _win.get_renderer();
	_win.clear(35, 35, 35);

	_image->draw(_rect);

	SDL_SetRenderDrawColor(r, 73, 73, 73, 0xFF);
	SDL_RenderFillRect(r, &_bar);

	for (auto& w : _widgets)
		w->draw();

	_win.display();
}

void Control::load_index(size_t i)
{
	size_t prev = _index;
	_index = clamp(i, (size_t)0, _paths.size() - 1);

	fs::path p = _paths[_index];
	_image.reset(new Image(p));
	fit();

	_win.set_title(p.filename().string() + " - Comix");
	set_pagenum();
}

void Control::drag(int dx, int dy)
{
	// Vertical movement
	if (_winh < _rect.h) {
		if (_rect.y + dy <= 0 && _rect.y + _rect.h + dy >= _winh) {
			_rect.y += dy;
		} else {
			_rect.y = (dy > 0 ? 0 : _winh - _rect.h);
		}
	} else {
		_rect.y = (_winh - _rect.h) / 2;
	}

	// Horizontal movement
	if (_winw < _rect.w) {
		if (_rect.x + dx <= 0 && _rect.x + _rect.w + dx >= _winw) {
			_rect.x += dx;
		} else {
			_rect.x = (dx > 0 ? 0 : _winw - _rect.w);
		}
	} else {
		_rect.x = (_winw - _rect.w) / 2;
	}

	_update = true;
}

void Control::zoom(float f) 
{
	_zoom = clamp(f, _minzoom, 2.f);

	// Get pre-zoom values
	float x = (float)(_focusx - _rect.x);
	float y = (float)(_focusy - _rect.y);
	float w = (float)_rect.w;
	float h = (float)_rect.h;

	// Resize _rect
	_image->get_size(&_rect.w, &_rect.h);
	_rect.w = (int)((float)_rect.w * _zoom);
	_rect.h = (int)((float)_rect.h * _zoom);
	
	// Move image
	if (_zoom > _minzoom) {
		int dx = (int)(x - (x * (float)_rect.w / w));
		int dy = (int)(y - (y * (float)_rect.h / h));

		if (_rect.x + _rect.w < _winw)
			dx *= -1;
		if (_rect.y + _rect.h < _winh)
			dy *= -1;

		drag(dx, dy);
	} else {
		center();
	}

	// Update text
	set_percent();

	// Update cursor
	int cy;
	SDL_GetMouseState(nullptr, &cy);
	set_cursor(cy < _bar.y
		&& (_rect.w > _winw || _rect.h > _winh)
		? SDL_SYSTEM_CURSOR_SIZEALL
		: SDL_SYSTEM_CURSOR_ARROW);

	_update = true;
}

void Control::center() 
{
	_rect.y = (_winh - _rect.h) / 2;
	_rect.x = (_winw - _rect.w) / 2;
	_update = true;
}

void Control::fit() 
{
	set_minzoom();
	zoom(_minzoom);
}

void Control::set_cursor(SDL_SystemCursor c) 
{
	_cursor.reset(SDL_CreateSystemCursor(c));
	SDL_SetCursor(_cursor.get());
}

Widget* Control::find_widget(int x, int y)
{
	auto it = find_if(begin(_widgets), end(_widgets), [x, y](auto& w) {
		return w->contains(x, y) && w->get_state() != Widget::DISABLED;
	});

	return (it == end(_widgets) ? nullptr : it->get());
}

void Control::reset_widgets() 
{
	for_each(begin(_widgets), end(_widgets), [](auto& w) {
		if (w->get_state() != Widget::DISABLED)
			w->set_state(Widget::IDLE);
	});
	_update = true;
}

void Control::set_minzoom()
{
	int w, h;
	_image->get_size(&w, &h);
	
	float sw = (float)_winw / (float)w;
	float sh = (float)_winh / (float)h;
	_minzoom = std::min({ sw, sh, 1.f });
}

void Control::set_pagenum() 
{
	char buff[6];
	sprintf_s(buff, 6, "%zu", _index + 1);
	_pagenum->set_string(buff);

	int w;
	_pagenum->get_size(&w, nullptr);
	_pagenum->set_position(_winw - 99 + (53 - w) / 2, _bar.y);
}

void Control::set_percent()
{
	char buff[5];
	sprintf_s(buff, 5, "%.0f%%", _zoom * 100.f);
	_percent->set_string(buff);

	int w;
	_percent->get_size(&w, nullptr);
	_percent->set_position(26 + (35 - w) / 2, _bar.y);
}

Control::~Control() 
{
	IMG_Quit();
}

Control& Control::get_instance()
{
	static Control instance;
	return instance;
}

void Control::loop(fs::path path) 
{
	// Reset controller state
	reset_widgets();
	_paths.clear();

	// Validate path
	if (!fs::exists(path)) {
		cerr << path << " does not exist" << endl;
		exit(1);
	}

	// Get parent directory
	fs::path first;
	if (!fs::is_directory(path)) {
		if (!is_image(path)) {
			cerr << path << " is not an image" << endl;
			exit(1);
		}
		first = path;
		path = path.parent_path();
	}

	// Discover image files
	fs::directory_iterator directory(path);
	for (auto &it : directory) {
		fs::path p = it.path();
		if (is_image(p))
			_paths.push_back(p);
	}

	if (_paths.empty()) {
		cerr << path << " does not contain any images" << endl;
		exit(1);
	} else if (_paths.size() == 1) {
		// Disable navigation for a single image
		_widgets[7]->set_state(Widget::DISABLED);
		_widgets[8]->set_state(Widget::DISABLED);
		_widgets[9]->set_state(Widget::DISABLED);
		_widgets[10]->set_state(Widget::DISABLED);
		_widgets[11]->set_state(Widget::DISABLED);
	}

	// Get initial index & load first image
	auto found = find(_paths.begin(), _paths.end(), first);
	load_index((found == _paths.end() ? 0 : distance(_paths.begin(), found)));

	// Push initial event (to kickstart UI)
	SDL_Event evnt;
	evnt.type = SDL_WINDOWEVENT;
	evnt.window.event = SDL_WINDOWEVENT_RESTORED;
	SDL_PushEvent(&evnt);

	/* Enter main loop:
	 * For an explanation on why SDL_AddEventWatch() is used
	 * instead of SDL_WaitEvent() or any other single-threaded
	 * event handling see: https://stackoverflow.com/q/46869687
	 *
	 * Even though the event watch may run on a separate thread
	 * because the main loop below does not modify any data 
	 * members the code is thread-safe.
	 */
	while (_run) {
		SDL_PumpEvents();
	}
}