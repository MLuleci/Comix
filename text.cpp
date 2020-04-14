#include <iostream>
#include <utility>
#include "text.h"
#include "render.h"
#include "control.h"
#include "util.h"

using namespace std;

unique_ptr<TTF_Font, function<void(TTF_Font*)>> Text::_font(
	nullptr, 
	[](TTF_Font* p) {
		TTF_CloseFont(p);
		TTF_Quit();
	}
);

Text::Text(const string& s) 
	: Widget()
	, _color{ 0xFF, 0xFF, 0xFF, 0xFF }
{
	set_string(s);
}

Text::Text(Text&& other) 
	: Widget(forward<Widget>(other))
	, _color(exchange(other._color, {0}))
	, _string(move(other._string))
{}

void Text::set_string(const string& s)
{
	aquire(_mut);
	_string = s;

	// Free old surface
	SDL_FreeSurface(_surface);

	// Load text into surface
	_surface = TTF_RenderUTF8_Blended(
		Text::_font.get(),
		_string.c_str(), 
		_color
	);
	if (!_surface) {
		cerr << "Failed to create surface: " << TTF_GetError() << endl;
		exit(1);
	}

	// Set widget size
	_w = _surface->w;
	_h = _surface->h;
	_uflag = true;
}

string Text::get_string() const
{
	aquire(_mut);
	return _string;
}

void Text::set_color(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	aquire(_mut);
	_color = { r, g, b, a };
	set_string(_string);
}

SDL_Color Text::get_color() const
{
	aquire(_mut);
	return _color;
}

void Text::draw() const
{
	aquire(_mut);
	SDL_Rect dst = { _x, _y, _w, _h };
	RenderWindow::get_instance().render(_texture, &dst);
}