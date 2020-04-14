#pragma once
#include <filesystem>
#include <string>
#include <mutex>
#include "drawable.h"

class Image : public Drawable {
	const std::string _path;
	mutable std::recursive_mutex _mut;
	int _w;
	int _h;

	void load();
public:
	Image(const std::filesystem::path&);

	void reset();
	void draw(const SDL_Rect&) const;
	void get_size(int*, int*) const;

	void flip_x();
	void flip_y();

	void rotate_cw();
	void rotate_ccw();
};