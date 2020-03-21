#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <functional>
#include <SDL.h>
#include <SDL_ttf.h>
#include "widget.h"

class Text final : public Widget {
	static std::unique_ptr<TTF_Font, std::function<void(TTF_Font*)>> _font;
	friend int main(int, char**);

	SDL_Texture* _texture;
	SDL_Color _color;
	std::string _string;
public:
	Text(const std::string&);
	Text(Text&&);
	~Text();

	void set_string(const std::string&);
	std::string get_string() const;

	void set_color(Uint8, Uint8, Uint8, Uint8);
	SDL_Color get_color() const;

	void draw() const override;
};