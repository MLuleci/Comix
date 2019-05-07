#include <iostream>
#include <SDL_image.h>
#include "controller.h"

Controller::Controller(char *Path) : Running(true), Update(true), Window(NULL), Image(NULL), width(0), height(0), mod(1)
{
	// Create window
	int Flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED;
	Window = SDL_CreateWindow("ImageViewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, Flags);
	if (Window == NULL) {
		std::cerr << "Couldn't create window: " << SDL_GetError() << std::endl;
		exit(-1);
	}

	// Create renderer
	Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_ACCELERATED);
	if (Renderer == NULL) {
		std::cerr << "Couldn't create renderer: " << SDL_GetError() << std::endl;
		exit(-1);
	}

	SDL_SetRenderDrawColor(Renderer, 0xFF, 0xFF, 0xFF, 0xFF);

	// Set-up path & directories
	fs::path File = fs::path(Path);
	if (ValidateFile(File)) {
		Image = LoadTexture(File);
		fs::current_path(File.parent_path()); // Change working directory
		for (auto &p : fs::directory_iterator(fs::current_path())) { // Add all files to vector
			if (ValidateFile(p.path())) {
				Paths.push_back(p.path());
				if (fs::equivalent(p.path(), File)) Index = Paths.size() - 1;
			}
		}
	} else {
		std::cerr << "Path must point to an image (jpeg/jpg/png)" << std::endl;
		exit(-1);
	}
}

Controller::~Controller() 
{
	Running = false;
	SDL_DestroyTexture(Image);
	SDL_DestroyRenderer(Renderer);
	SDL_DestroyWindow(Window);
	IMG_Quit();
	SDL_Quit();
}

int Controller::Loop() 
{
	SDL_Event Event;
	while (Running) {
		while (SDL_PollEvent(&Event)) {
			OnEvent(&Event);
		}
		if (Update) {
			Render();
			Update = false;
		}
	}

	return 0;
}

void Controller::OnEvent(SDL_Event *Event) 
{
	switch (Event->type) {
		case SDL_QUIT:
			Running = false;
			break;
		case SDL_KEYDOWN:
			SDL_Keysym sym = Event->key.keysym;
			if (sym.scancode == SDL_SCANCODE_LEFT) {
				if (Index == 0) Index = Paths.size();
				Index--;
				Image = LoadTexture(Paths.at(Index));
				mod = 1.f;
				Update = true;
			} else if (sym.scancode == SDL_SCANCODE_RIGHT) {
				if (++Index == Paths.size()) Index = 0;
				Image = LoadTexture(Paths.at(Index));
				mod = 1.f;
				Update = true;
			} else if (sym.mod & KMOD_CTRL) {
				if (sym.scancode == SDL_SCANCODE_EQUALS) {
					ZoomIn();
				} else if (sym.scancode == SDL_SCANCODE_MINUS) {
					ZoomOut();
				} else if (sym.scancode == SDL_SCANCODE_0) {
					mod = 1.f;
					Update = true;
				}
			}
			break;
		case SDL_MOUSEWHEEL:
			if (Event->wheel.y > 0) { // Up
				ZoomIn();
			} else if (Event->wheel.y < 0) { // Down
				ZoomOut();
			}
			break;
		case SDL_WINDOWEVENT:
			Update = true;
			break;
	}
}

void Controller::Render() 
{
	SDL_RenderClear(Renderer);
	
	// Calculate destination rectangle
	int w, h;
	float scale = 1.f;
	SDL_Rect rect;
	SDL_GetWindowSize(Window, &w, &h);

	if (w < width || h < height) scale = fmin((float)w / (float)width, (float)h/(float)height);
	rect.w = static_cast<int>(width * scale * mod);
	rect.h = static_cast<int>(height * scale * mod);
	rect.x = (w - rect.w) / 2;
	rect.y = (h - rect.h) / 2;

	SDL_RenderCopy(Renderer, Image, NULL, &rect);
	SDL_RenderPresent(Renderer);
}


SDL_Texture *Controller::LoadTexture(fs::path Path)
{
	if (Image) SDL_DestroyTexture(Image);
	SDL_Surface *surface = IMG_Load(Path.generic_string().c_str()); // Load surface
	SDL_Texture *texture = SDL_CreateTextureFromSurface(Renderer, surface); // Load texture
	if (surface) {
		width = surface->w;
		height = surface->h;
		SDL_FreeSurface(surface);
	}
	if (texture == NULL || surface == NULL) {
		std::cerr << "Couldn't load texture/surface: " << SDL_GetError() << std::endl;
		exit(-1);
	}
	return texture;
}

bool Controller::ValidateFile(fs::path Path) 
{
	if (!Path.empty() && !fs::is_directory(Path) && fs::is_regular_file(Path)) {
		std::string ext = Path.extension().generic_string();
		if (!ext.empty()) {
			ext = ext.substr(1, ext.length());
			for (std::string e : Extensions)
				if (!ext.compare(e)) return true;
		}
	}
	return false;
}

void Controller::ZoomIn() 
{
	mod += 0.1f;
	if (mod > 2.f) mod = 2.f;
	Update = true;
}

void Controller::ZoomOut() 
{
	mod -= 0.1f;
	if (mod < 0.1f) mod = 0.1f;
	Update = true;
}