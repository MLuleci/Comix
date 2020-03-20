#pragma once
#include <string>
#include <SDL.h>

class RenderWindow {
	static const Uint32 _winflags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED;
	static const Uint32 _renflags = SDL_RENDERER_ACCELERATED;
	SDL_Window* _window;
	SDL_Renderer* _renderer;
	SDL_Surface* _icon;

	RenderWindow();
public:
	RenderWindow(const RenderWindow&) = delete;
	RenderWindow& operator=(const RenderWindow&) = delete;
	~RenderWindow();

	void render(SDL_Texture*, const SDL_Rect*) const;
	void render(SDL_Texture*, const SDL_Rect*, const SDL_Rect*) const;
	void clear(Uint8, Uint8, Uint8) const;
	void display() const;

	static RenderWindow& get_instance();
	SDL_Renderer* get_renderer() const;
	void get_size(int*, int*) const;
	void get_position(int*, int*) const;

	void set_title(const std::string&);
};