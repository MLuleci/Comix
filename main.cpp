#include <iostream>
#include <SDL_image.h>
#include "controller.h"

int main(int argc, char *argv[]) 
{
	// Initialize SDL & check args
	int Flags = IMG_INIT_JPG | IMG_INIT_PNG;
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <path>" << std::endl;
		return -1;
	} else if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		std::cerr << "Couldn't initialise SDL: " << SDL_GetError() << std::endl;
		return -1;
	} else if (!(IMG_Init(Flags) & Flags)) {
		std::cerr << "Couldn't initialise SDL image: " << IMG_GetError() << std::endl;
		return -1;
	}

	Controller control(argv[1]);

	return control.Loop();
}