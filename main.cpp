#include <filesystem>
#include <iostream>
#include <string>
#include <clocale>
#include "util.h"
#include "text.h"
#include "control.h"

using namespace std;
namespace fs = std::filesystem;

int main(int argc, char **argv) 
{
	// Enable UTF-8 multibyte encoding
	setlocale(LC_ALL, "en_US.UTF-8");

	// Set respath
	Util::_respath = fs::path(argv[0]).parent_path() / "res";

	// Load font
	Text::_font.reset(
		[]() -> TTF_Font* {
			// Initialize SDL_ttf subsystem
			if (TTF_Init() < 0) {
				cerr << "Failed to initialise SDL_ttf: " << TTF_GetError() << endl;
				exit(1);
			}

			// Load the default font
			fs::path p = Util::get_respath("estre.ttf");
			TTF_Font* f = TTF_OpenFont(p.string().c_str(), 15);
			if (!f) {
				cerr << "Failed to load font: " << TTF_GetError() << endl;
				exit(1);
			}
			return f;
		}()
	);

    // Print usage
    if (argc != 2) {
		cerr << "Usage: " << argv[0] << " <path>" << endl;
        return 1;
    }

	// Initialize controller & loop
	Control::get_instance().loop(argv[1]);
    return 0;
}