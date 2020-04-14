#include <functional>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <SDL.h>
#include "bsemaphore.h"
#include "control.h"
#include "render.h"
#include "button.h"
#include "widget.h"
#include "image.h"
#include "text.h"
#include "util.h"

using namespace std;
namespace fs = std::filesystem;

/*  Widgets & friends:
 *  0: zoom out
 *  1: zoom percentage
 *  2: zoom in
 *  3: rotate ccw
 *  4: rotate cw
 *  5: flip x
 *  6: flip y
 *  7: first page
 *  8: prev page
 *  9: page number
 * 10: next page
 * 11: last page
 */
unique_ptr<Widget> _widgets[12];
Text* _percent;
Text* _pagenum;
unique_ptr<Text> _status;

/* Program status & user event(s) */
atomic_bool _run(true);
atomic_bool _update(true);
Uint32 _uevnt;

/* Paths & index */
vector<fs::path> _paths;
atomic_size_t _index(-1);

/* Image & friends */
unique_ptr<Image> _image;
SDL_Rect _rect;
bool _drag(false);
float _zoom(1.f);
float _minzoom(.01f);
int _focusx;
int _focusy;

/* Window, cursor & friends */
RenderWindow* _win;
int _winh;
int _winw;
SDL_Rect _bar;
unique_ptr<SDL_Cursor, function<void(SDL_Cursor*)>> _cursor(nullptr, [](SDL_Cursor* p) { SDL_FreeCursor(p); });

/* Worker threads & sync */
SDL_Thread* _worker;
BSemaphore _sem;
recursive_mutex _mut;

void draw()
{
	// Update textures
	for_each(begin(_widgets), end(_widgets), [](auto& w) {
		static_cast<Drawable*>(w.get())->update();
	});

	SDL_Renderer* r = _win->get_renderer();
	_win->clear(35, 35, 35);

	{
		// Draw image if loaded, status otherwise
		unique_lock<recursive_mutex> lk(_mut, try_to_lock);
		if (lk) {
			static_cast<Drawable*>(_image.get())->update();
			_image->draw(_rect);
		} else {
			static_cast<Drawable*>(_status.get())->update();
			_status->draw();
		}
	}

	// Draw UI bar
	SDL_SetRenderDrawColor(r, 73, 73, 73, 0xFF);
	SDL_RenderFillRect(r, &_bar);

	// Draw widgets
	for (auto& w : _widgets)
		w->draw();

	_win->display();
}

void drag(int dx, int dy)
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

void center()
{
	_rect.y = (_winh - _rect.h) / 2;
	_rect.x = (_winw - _rect.w) / 2;
	_update = true;
}

void set_minzoom()
{
	int w, h;
	_image->get_size(&w, &h);

	float sw = (float)_winw / (float)w;
	float sh = (float)_winh / (float)h;
	_minzoom = std::min({sw, sh, 1.f});
}

void set_percent()
{
	// If image isn't ready use placeholder
	unique_lock<recursive_mutex> lk(_mut, try_to_lock);
	if (lk) {
		char buff[5];
		sprintf_s(buff, 5, "%.0f%%", _zoom * 100.f);
		_percent->set_string(buff);
	} else {
		_percent->set_string("---");
	}

	int w;
	_percent->get_size(&w, nullptr);
	_percent->set_position(26 + (35 - w) / 2, _bar.y);

	_update = true;
}

void set_cursor(SDL_SystemCursor c)
{
	_cursor.reset(SDL_CreateSystemCursor(c));
	SDL_SetCursor(_cursor.get());
	_update = true;
}

void zoom(float f)
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

void fit()
{
	set_minzoom();
	zoom(_minzoom);
}

void push_event()
{
	SDL_Event evnt;
	evnt.type = _uevnt;
	SDL_PushEvent(&evnt);
}

Widget* find_widget(int x, int y)
{
	auto it = find_if(begin(_widgets), end(_widgets), [x, y](auto& w) {
		return w->contains(x, y) && w->get_state() != Widget::DISABLED;
	});

	return (it == end(_widgets) ? nullptr : it->get());
}

void reset_widgets()
{
	for_each(begin(_widgets), end(_widgets), [](auto& w) {
		if (w->get_state() != Widget::DISABLED)
			w->set_state(Widget::IDLE);
	});
	_update = true;
}

void set_pagenum()
{
	char buff[6];
	sprintf_s(buff, 6, "%zu", _index + 1);
	_pagenum->set_string(buff);

	int w;
	_pagenum->get_size(&w, nullptr);
	_pagenum->set_position(_winw - 99 + (53 - w) / 2, _bar.y);

	_update = true;
}

int SDLCALL load(void* udata)
{
	while (_run) {
		_sem.down();

		if (!_run)
			return 0;

		{
			aquire(_mut);
			_image.reset(new Image(_paths[_index]));
		}

		push_event();
	}

	return 0;
}

void load_index(size_t i)
{
	_index = clamp(i, (size_t)0, _paths.size() - 1);

	// Update window title & pagenum
	fs::path p = _paths[_index];
	_win->set_title(p.filename().string() + " - Comix");
	set_pagenum();

	// Disable image operation widgets
	for (int i = 0; i < 7; ++i)
		_widgets[i]->set_state(Widget::DISABLED);

	// Set placeholder for percentage
	_percent->set_string("---");

	int w;
	_percent->get_size(&w, nullptr);
	_percent->set_position(26 + (35 - w) / 2, _bar.y);

	// Let worker thread start
	_sem.up();
	_update = true;
}

int SDLCALL handle(void* udata, SDL_Event* evnt)
{
    // Stop processing after main loop exit
	if (!_run)
		return 0;

	switch (evnt->type) {
		case SDL_QUIT:
			_run = false;
			break;
		case SDL_WINDOWEVENT:
		{
			SDL_WindowEvent we = evnt->window;
			switch (we.event) {
				case SDL_WINDOWEVENT_LEAVE:
				case SDL_WINDOWEVENT_FOCUS_LOST:
				case SDL_WINDOWEVENT_HIDDEN:
				case SDL_WINDOWEVENT_MINIMIZED:
					// Left window
					reset_widgets();
					break;
				default:
				{
					// Update window dimensions & related variables
					_win->get_size(&_winw, &_winh);

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

					// Status text
					int w, h;
					_status->get_size(&w, &h);
					_status->set_position((_winw - w) / 2, (_winh - h) / 2);

					// Adjust image (if loaded)
                    try_aquire(_mut) 
                    {
                        if (_zoom == _minzoom) {
                            fit();
                        } else {
                            _focusx = _rect.x;
                            _focusy = _rect.y;
                            set_minzoom();
                            zoom(_zoom);
                        }
                    }
					break;
				}
			}
			break;
		}
		case SDL_KEYDOWN:
		{
			SDL_Keysym key = evnt->key.keysym;
			SDL_Keycode sym = key.sym;

            // Directory navigation
			if (sym == SDLK_LEFT) {
				_widgets[8]->trigger();
			}
			if (sym == SDLK_RIGHT) {
				_widgets[10]->trigger();
			}

            // Image operations (if loaded)
            try_aquire(_mut)
            {
                if (_zoom > _minzoom) {
                    if (sym == SDLK_UP
                        || sym == SDLK_PAGEUP)
                        drag(0, (int)(_rect.h * .02f));
                    if (sym == SDLK_DOWN
                        || sym == SDLK_PAGEDOWN)
                        drag(0, (int)(_rect.h * -.02f));
                    if (sym == SDLK_HOME)
                        _rect.y = 0;
                    if (sym == SDLK_END)
                        _rect.y = _winh - _rect.h;
                    _update = true;
                }

                // Zoom, fit & fill
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
                    }
                }
            }

            // Quit program
			_run = !((key.mod & KMOD_CTRL) && sym == SDLK_w);
			break;
		}
		case SDL_MOUSEWHEEL:
		{
            try_aquire(_mut)
            {
                SDL_MouseWheelEvent mwe = evnt->wheel;

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
            }
			break;
		}
		case SDL_MOUSEBUTTONDOWN:
		{
			SDL_MouseButtonEvent mbe = evnt->button;
			Widget* w = find_widget(mbe.x, mbe.y);

			if (w != nullptr) { 
                // Activate widget
				w->set_state(Widget::ACTIVE);
                _update = true;
			} else { 
                // Start dragging (clicked anywhere above bar & image loaded)
                try_aquire(_mut)
                {
				    _drag = mbe.y < _bar.y && (_rect.w > _winw || _rect.h > _winh);
                }
			}
			break;
		}
		case SDL_MOUSEBUTTONUP:
		{
			SDL_MouseButtonEvent mbe = evnt->button;
			Widget* w = find_widget(mbe.x, mbe.y);

			if (w != nullptr) {
			    // Widget clicked
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
			SDL_MouseMotionEvent mme = evnt->motion;
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
        default:
            // Process user event (i.e. image load complete)
            if (evnt->type == _uevnt) {

                // Re-enable widgets
                for (int i = 0; i < 7; ++i)
                    _widgets[i]->set_state(Widget::IDLE);
                
                fit();
            }
	}

	// Draw components
	if (_update) {
		_update = false;
		draw();
	}

	return 0;
}

void Control::init(fs::path path)
{
    // Validate path
	if (!fs::exists(path)) {
		cerr << path << " does not exist" << endl;
		exit(1);
	}

	// Get parent directory
	fs::path first;
	if (!fs::is_directory(path)) {
		if (!Util::is_image(path)) {
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
		if (Util::is_image(p))
			_paths.push_back(p);
	}

    // Check images found in path
    if (_paths.empty()) {
		cerr << path << " does not contain any images" << endl;
		exit(1);
	}

	// Create window & renderer
	_win = &RenderWindow::get_instance();

    // Register user event(s)
    if ((_uevnt = SDL_RegisterEvents(1)) == ((Uint32)-1)) {
        cerr << "Failed to register user event(s)" << endl;
        exit(1);
    }

    // Try and set texture filtering to linear
	if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
		cerr << "Warning: Linear texture filtering not enabled" << endl;
	}

	// Create cursor
	set_cursor(SDL_SYSTEM_CURSOR_ARROW);

	// Create widgets
	_widgets[0] = make_unique<Button>(Util::get_respath("ui_minus.png"));
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

	_widgets[2] = make_unique<Button>(Util::get_respath("ui_plus.png"));
	_widgets[2]->set_handler([&](Widget& w) {
		_focusx = _winw / 2;
		_focusy = _winh / 2;
		float i, f = modf(_zoom * 10.f, &i);
		zoom((f > 0.09 ? i / 10.f : _zoom) + .1f);
	});

	_widgets[3] = make_unique<Button>(Util::get_respath("ui_rotate_left.png"));
	_widgets[3]->set_handler([&](Widget& w) {
		_image->rotate_ccw();
		fit();
	});

	_widgets[4] = make_unique<Button>(Util::get_respath("ui_rotate_right.png"));
	_widgets[4]->set_handler([&](Widget& w) {
		_image->rotate_cw();
		fit();
	});

	_widgets[5] = make_unique<Button>(Util::get_respath("ui_flip_x.png"));
	_widgets[5]->set_handler([&](Widget& w) {
		_image->flip_x();
		fit();
	});

	_widgets[6] = make_unique<Button>(Util::get_respath("ui_flip_y.png"));
	_widgets[6]->set_handler([&](Widget& w) {
		_image->flip_y();
		fit();
	});

	_widgets[7] = make_unique<Button>(Util::get_respath("ui_first.png"));
	_widgets[7]->set_handler([&](Widget& w) {
		load_index(0);
	});

	_widgets[8] = make_unique<Button>(Util::get_respath("ui_left.png"));
	_widgets[8]->set_handler([&](Widget& w) {
		size_t i = _index - 1;
		if (i < 0)
			i = _paths.size() - 1;
		load_index(i);
	});

	_widgets[9] = make_unique<Text>(" ");
	_pagenum = (Text*)_widgets[9].get();
	_widgets[9]->set_handler([&](Widget& w) {
		load_index(0);
	});

	_widgets[10] = make_unique<Button>(Util::get_respath("ui_right.png"));
	_widgets[10]->set_handler([&](Widget& w) {
		size_t i = _index + 1;
		if (i >= _paths.size())
			i = 0;
		load_index(i);
	});

	_widgets[11] = make_unique<Button>(Util::get_respath("ui_last.png"));
	_widgets[11]->set_handler([&](Widget& w) {
		load_index(_paths.size() - 1);
	});

	_status = make_unique<Text>("Loading...");

    // Disable navigation when viewing a single image
    if (_paths.size() == 1) {
		_widgets[7]->set_state(Widget::DISABLED);
		_widgets[8]->set_state(Widget::DISABLED);
		_widgets[9]->set_state(Widget::DISABLED);
		_widgets[10]->set_state(Widget::DISABLED);
		_widgets[11]->set_state(Widget::DISABLED);
	}

	// Create image loading worker thread
	_worker = SDL_CreateThread(load, "th-image", nullptr);

	// Add event watcher
	SDL_AddEventWatch(handle, nullptr);

    // Get initial index & load first image
	auto found = find(_paths.begin(), _paths.end(), first);
	load_index((found == _paths.end() ? 0 : distance(_paths.begin(), found)));
}

void Control::loop()
{
    // Main loop
    while (_run) {
        SDL_PumpEvents();
    }

    // Join worker thread
    _sem.up();
    SDL_WaitThread(_worker, NULL);
}