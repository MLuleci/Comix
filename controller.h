#pragma once
#include <filesystem>
#include <vector>
#include <string>
#include <SDL.h>
#define WIDTH 640
#define HEIGHT 480
#define sign(x) (x > 0 ? 1 : (x < 0 ? -1 : 0))

namespace fs = std::filesystem;

class Controller {
private:
	bool m_run, m_update;
	SDL_Window *m_window;
	SDL_Renderer *m_render;
	
	SDL_Texture *m_img;
	int m_img_w, m_img_h;
	int m_win_w, m_win_h;
	float m_scale;

	std::vector<fs::path> m_list;
	size_t m_index;

	SDL_Cursor *m_cursor;
	SDL_Rect m_rect;
	int m_sx, m_sy;

	const std::string m_ext[3] = { "jpg", "jpeg", "png" };

	bool Validate(fs::path path);
	void UpdateDim();
public:
	Controller(char *path);
	~Controller();
	
	int Loop();
	void Event(SDL_Event *event);
	void Render();

	void Zoom(float scale);
	void Move(int x, int y);
	SDL_Texture *LoadImage(fs::path path);
};