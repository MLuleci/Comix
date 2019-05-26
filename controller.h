#pragma once
#include <filesystem>
#include <vector>
#include <string>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#define sign(x) (x > 0 ? 1 : (x < 0 ? -1 : 0))

namespace fs = std::filesystem;

class Controller {
private:
	bool m_run, m_update;
	SDL_Window *m_window;
	SDL_Renderer *m_render;
	
	TTF_Font *m_font;
	SDL_Texture *m_text[2]; // path, zoom + index
	SDL_Rect m_text_rect[2];
	SDL_Rect m_bar;
	int m_font_size;

	SDL_Texture *m_img;
	int m_img_w, m_img_h;
	int m_win_w, m_win_h;
	float m_scale;

	std::vector<fs::path> m_list;
	size_t m_index;

	SDL_Cursor *m_cursor;
	SDL_Rect m_rect;
	int m_sx, m_sy, m_bx, m_by;

	bool m_keep_zoom;
	int m_color[3] = { 0xFF, 0xFF, 0xFF };
	SDL_Scancode m_next, m_prev;

	const std::string m_ext[3] = { "jpg", "jpeg", "png" };

	bool Validate(fs::path path);
	void GetWinDim();
	void CenterImage();
public:
	Controller(char *path);
	~Controller();
	
	int Loop();
	void Event(SDL_Event *event);
	void Render();

	void Zoom(float scale);
	void Move(int x, int y);
	SDL_Texture *LoadImage(fs::path path);
	SDL_Texture *LoadText(std::string text, SDL_Color color, SDL_Rect *rect = NULL);
};