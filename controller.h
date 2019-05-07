#pragma once
#include <filesystem>
#include <vector>
#include <string>
#include <SDL.h>
#define WIDTH 640
#define HEIGHT 480

namespace fs = std::filesystem;

const std::string Extensions[] = {"jpg", "jpeg", "png"};

class Controller {
private:
	bool Running, Update;
	SDL_Window *Window;
	SDL_Renderer *Renderer;
	
	SDL_Texture *Image;
	int width, height;
	float mod;

	std::vector<fs::path> Paths;
	size_t Index;

	void ZoomIn();
	void ZoomOut();
	SDL_Texture *LoadTexture(fs::path Path);
	bool ValidateFile(fs::path Path);
public:
	Controller(char *Path);
	~Controller();
	int Loop();
	void OnEvent(SDL_Event *Event);
	void Render();
};