#include <iostream>
#include <utility>
#include <cstdlib>
#include "drawable.h"
#include "render.h"

using namespace std;

Drawable::Drawable(Drawable&& other)
	: _texture(exchange(other._texture, nullptr))
	, _surface(exchange(other._surface, nullptr))
{}

Drawable::~Drawable()
{
	SDL_FreeSurface(_surface);
	SDL_DestroyTexture(_texture);
}

void Drawable::update()
{
	if (!_uflag)
		return;
	_uflag = false;

	if (!_surface) {
		cerr << "Invalid surface" << endl;
		exit(1);
	}

	// Destroy old texture
	if (_texture)
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