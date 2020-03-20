#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <SDL_image.h>
#include "image.h"
#include "render.h"
#include "util.h"

using namespace std;
namespace fs = std::filesystem;

void Image::load() 
{
	// Destroy old surface
	SDL_FreeSurface(_surface);

	// Load image into surface
	_surface = IMG_Load(_path.c_str());
	if (!_surface) {
		cerr << "Failed to load surface: " << _path << endl;
		exit(1);
	}

	// Get dimensions
	_w = _surface->w;
	_h = _surface->h;

	// Update texture
	update();
}

void Image::update()
{
	// Destroy old texture
	SDL_DestroyTexture(_texture);

	// Create texture from surface
	_texture = SDL_CreateTextureFromSurface(
		RenderWindow::get_instance().get_renderer(),
		_surface
	);
	if (!_texture) {
		cerr << "Failed to create texture" << endl;
		exit(1);
	}
}

Image::Image(const fs::path& p)
	: _path(p.string())
{
	if (!is_image(p)) {
		cerr << "Internal error: " << p << " is not an image" << endl;
		exit(1);
	}

	load();
}

Image::~Image()
{
	SDL_FreeSurface(_surface);
	SDL_DestroyTexture(_texture);
}

void Image::reset() 
{
	load();
}

void Image::draw(const SDL_Rect& dst) const
{
	RenderWindow::get_instance().render(_texture, NULL, &dst);
}

void Image::get_size(int *w, int *h) const
{
	if (w) *w = _w;
	if (h) *h = _h;
}

void Image::flip_x()
{
	// Get pixel format
	SDL_PixelFormat* fmt = _surface->format;

	// Create new surface w/ identical dimensions
	SDL_Surface *out = SDL_CreateRGBSurface(
		0,
		_w,
		_h,
		fmt->BitsPerPixel,
		fmt->Rmask,
		fmt->Gmask,
		fmt->Bmask,
		fmt->Amask
	);

	// Lock both surfaces
	if (SDL_LockSurface(_surface) < 0 || SDL_LockSurface(out) < 0) {
		cerr << "Failed to lock surface: " << SDL_GetError() << endl;
		exit(1);
	}

	// Iterate over rows, copying pixels
	void* pixels = _surface->pixels;
	void* npixels = out->pixels;
	int pitch = _surface->pitch;

	for (int y = 0; y < _h; ++y) {
		memcpy(
			(Uint8*)npixels + y * pitch,
			(Uint8*)pixels + (_h - y) * pitch,
			pitch
		);
	}

	// Unlock surfaces
	SDL_UnlockSurface(_surface);
	SDL_UnlockSurface(out);

	// Swap & free old surface
	SDL_FreeSurface(_surface);
	_surface = out;

	// Update texture
	update();
}

void Image::flip_y()
{
	// Get pixel format
	SDL_PixelFormat* fmt = _surface->format;
	int size = fmt->BytesPerPixel;

	// Create new surface w/ identical dimensions
	SDL_Surface *out = SDL_CreateRGBSurface(
		0,
		_w,
		_h,
		fmt->BitsPerPixel,
		fmt->Rmask,
		fmt->Gmask,
		fmt->Bmask,
		fmt->Amask
	);

	// Lock both surfaces
	if (SDL_LockSurface(_surface) < 0 || SDL_LockSurface(out) < 0) {
		cerr << "Failed to lock surface: " << SDL_GetError() << endl;
		exit(1);
	}

	// Iterate over columns, copying pixels
	void* pixels = _surface->pixels;
	void* npixels = out->pixels;
	int pitch = _surface->pitch;

	for (int y = 0; y < _h; ++y) {
		int row = y * pitch;
		for (int x = 0; x < _w; ++x) {
			memcpy(
				(Uint8*) npixels + x * size + row,
				(Uint8*) pixels + (_w - x) * size + row,
				size
			);
		}
	}

	// Unlock surfaces
	SDL_UnlockSurface(_surface);
	SDL_UnlockSurface(out);

	// Swap & free old surface
	SDL_FreeSurface(_surface);
	_surface = out;

	// Update texture
	update();
}

void Image::rotate_cw() 
{
	// Get pixel format
	SDL_PixelFormat* fmt = _surface->format;
	int size = fmt->BytesPerPixel;

	// Create new surface w/ flipped dimensions
	SDL_Surface *out = SDL_CreateRGBSurface(
		0,
		_h,
		_w,
		fmt->BitsPerPixel,
		fmt->Rmask,
		fmt->Gmask,
		fmt->Bmask,
		fmt->Amask
	);

	// Lock both surfaces
	if (SDL_LockSurface(_surface) < 0 || SDL_LockSurface(out) < 0) {
		cerr << "Failed to lock surface: " << SDL_GetError() << endl;
		exit(1);
	}

	// Per-pixel copy (inefficient)
	void* pixels = _surface->pixels;
	void* npixels = out->pixels;
	int pitch = _surface->pitch;
	int npitch = size * _h;

	for (int y = 0; y < _h; ++y) {
		for (int x = 0; x < _w; ++x) {

			// Translation: _surface(x, y) -> out(h - y, x)
			memcpy(
				(Uint8*) npixels + (_h - y) * size + x * npitch,
				(Uint8*) pixels + x * size + y * pitch,
				size
			);
		}
	}

	// Unlock surfaces
	SDL_UnlockSurface(_surface);
	SDL_UnlockSurface(out);

	// Swap & free old surface
	SDL_FreeSurface(_surface);
	_surface = out;

	// Update dimensions
	_w = _surface->w;
	_h = _surface->h;

	// Update texture
	update();
}

void Image::rotate_ccw() 
{
	// Get pixel format
	SDL_PixelFormat* fmt = _surface->format;
	int size = fmt->BytesPerPixel;

	// Create new surface w/ flipped dimensions
	SDL_Surface *out = SDL_CreateRGBSurface(
		0,
		_h,
		_w,
		fmt->BitsPerPixel,
		fmt->Rmask,
		fmt->Gmask,
		fmt->Bmask,
		fmt->Amask
	);

	// Lock both surfaces
	if (SDL_LockSurface(_surface) < 0 || SDL_LockSurface(out) < 0) {
		cerr << "Failed to lock surface: " << SDL_GetError() << endl;
		exit(1);
	}

	// Per-pixel copy (inefficient)
	void* pixels = _surface->pixels;
	void* npixels = out->pixels;
	int pitch = _surface->pitch;
	int npitch = size * _h;

	for (int y = 0; y < _h; ++y) {
		for (int x = 0; x < _w; ++x) {

			// Translation: _surface(x, y) -> out(y, w - x)
			memcpy(
				(Uint8*)npixels + y * size + (_w - x) * npitch,
				(Uint8*)pixels + x * size + y * pitch,
				size
			);
		}
	}

	// Unlock surfaces
	SDL_UnlockSurface(_surface);
	SDL_UnlockSurface(out);

	// Swap & free old surface
	SDL_FreeSurface(_surface);
	_surface = out;

	// Update dimensions
	_w = _surface->w;
	_h = _surface->h;

	// Update texture
	update();
}