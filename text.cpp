#include <iostream>
#include <cstdlib>
#include <utility>
#include "text.h"
#include "render.h"
#include "control.h"
#include "util.h"

using namespace std;
namespace fs = std::filesystem;

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
	, _texture(exchange(other._texture, nullptr))
	, _color(exchange(other._color, {0}))
	, _string(move(other._string))
{}

Text::~Text()
{
	SDL_DestroyTexture(_texture);
}

void Text::set_string(const string& s)
{
	aquire(_mut);

	// Free old texture
	SDL_DestroyTexture(_texture);
	_texture = nullptr;

	// Don't bother if string is empty
	_string = s;
	if (_string.empty())
		return;

	// Load text into surface
	SDL_Surface *surface = TTF_RenderUTF8_Blended(
		Text::_font.get(),
		_string.c_str(), 
		_color
	);
	if (!surface) {
		cerr << "Failed to create surface: " << TTF_GetError() << endl;
		exit(1);
	}

	// Set widget size
	_w = surface->w;
	_h = surface->h;

	// Create texture from surface
	_texture = SDL_CreateTextureFromSurface(
		RenderWindow::get_instance().get_renderer(),
		surface
	);
	SDL_FreeSurface(surface);
	if (!_texture) {
		cerr << "Failed to create texture" << endl;
		exit(1);
	}
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
	RenderWindow::get_instance().render(_texture, NULL, &dst);
}