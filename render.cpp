#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <SDL_image.h>
#include "render.h"
#include "util.h"

using namespace std;
namespace fs = std::filesystem;

RenderWindow::RenderWindow()
{
	// Initialize SDL & SDL_image subsystem
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		cerr << "Failed to initialize SDL: " << SDL_GetError() << endl;
		exit(1);
	}

	int flags = IMG_INIT_JPG | IMG_INIT_PNG;
	if (IMG_Init(flags) ^ flags) {
		cerr << "Failed to initialise SDL_image: " << IMG_GetError() << endl;
		exit(1);
	}

	// Create window
	_window = SDL_CreateWindow(
		"",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		0,
		0,
		_winflags
	);
	if (!_window) {
		cerr << "Failed to create SDL_Window: " << SDL_GetError() << endl;
		exit(1);
	}
	SDL_SetWindowMinimumSize(_window, 412, 334);

	// Load & set icon
	fs::path p = Util::get_respath("icon.png");
	_icon = IMG_Load(p.string().c_str());
	if (!_icon) {
		cerr << "Failed to load surface: " << p << endl;
		exit(1);
	}
	SDL_SetWindowIcon(_window, _icon);

	// Create renderer
	_renderer = SDL_CreateRenderer(
		_window,
		-1,
		_renflags
	);
	if (!_renderer) {
		cerr << "Failed to create SDL_Renderer: " << SDL_GetError() << endl;
		exit(1);
	}
}

RenderWindow::~RenderWindow() 
{
	SDL_DestroyRenderer(_renderer);
	SDL_DestroyWindow(_window);
	SDL_FreeSurface(_icon);
	IMG_Quit();
	SDL_Quit();
}

void RenderWindow::render(SDL_Texture *tex, const SDL_Rect *dst) const
{
	SDL_RenderCopy(_renderer, tex, NULL, dst);
}

void RenderWindow::render(SDL_Texture *tex, const SDL_Rect *src, const SDL_Rect *dst) const
{
	SDL_RenderCopy(_renderer, tex, src, dst);
}

void RenderWindow::clear(Uint8 r = 0xFF, Uint8 g = 0xFF, Uint8 b = 0xFF) const
{
	SDL_SetRenderDrawColor(_renderer, r, g, b, 0xFF);
	SDL_RenderClear(_renderer);
}

void RenderWindow::display() const
{
	SDL_RenderPresent(_renderer);
}

RenderWindow& RenderWindow::get_instance()
{
	static RenderWindow instance;
	return instance;
}

SDL_Renderer* RenderWindow::get_renderer() const
{
	return _renderer;
}

void RenderWindow::get_size(int *w, int *h) const
{
	SDL_GetWindowSize(_window, w, h);
}

void RenderWindow::get_position(int *x, int *y) const
{
	SDL_GetWindowPosition(_window, x, y);
}

void RenderWindow::set_title(const string& s) 
{
	SDL_SetWindowTitle(_window, s.c_str());
}