#pragma once
#include <filesystem>
#include <string>
#include <SDL.h>

class Image {
	const std::string _path;
	SDL_Surface* _surface;
	SDL_Texture* _texture;
	int _w;
	int _h;

	void load();
	void update();
public:
	Image(const std::filesystem::path&);
	~Image();

	void reset();
	void draw(const SDL_Rect&) const;
	void get_size(int*, int*) const;

	void flip_x();
	void flip_y();

	void rotate_cw();
	void rotate_ccw();
};