#pragma once
#include <SDL.h>

class Drawable {
protected:
	SDL_Texture* _texture;
	SDL_Surface* _surface;
	bool _uflag;

public:
	Drawable() = default;
	Drawable(Drawable&&);
	~Drawable();
	void update();
};