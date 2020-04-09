#pragma once
#include <filesystem>
#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <SDL.h>
#include "widget.h"
#include "image.h"
#include "text.h"
#include "render.h"

class Control {
	/**
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
	std::unique_ptr<Widget> _widgets[12];
	std::vector<std::filesystem::path> _paths;
	std::unique_ptr<Image> _image;
	std::unique_ptr<SDL_Cursor, std::function<void(SDL_Cursor*)>> _cursor;

	bool _run;
	bool _update;
	bool _drag;
	bool _rdy;
	float _zoom;
	float _minzoom;
	int _winh;
	int _winw;
	int _focusx;
	int _focusy;
	Text* _percent;
	Text* _pagenum;
	Text _status;
	size_t _index;
	RenderWindow& _win;
	SDL_Rect _rect;
	SDL_Rect _bar;

	Control();
	static int SDLCALL watch(void*, SDL_Event*);
	void handle(const SDL_Event&);
	void draw() const;
	void load_index(size_t);
	void drag(int, int);
	void zoom(float);
	void center();
	void fit();
	void set_cursor(SDL_SystemCursor);
	Widget* find_widget(int, int);
	void reset_widgets();
	void set_minzoom();
	void set_pagenum();
	void set_percent();
public:
	Control(const Control&) = delete;
	Control& operator=(const Control&) = delete;

	static Control& get_instance();
	void loop(std::filesystem::path);
};