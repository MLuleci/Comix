#include <filesystem>
#include <iostream>
#include <string>
#include <clocale>
#include "util.h"
#include "text.h"
#include "control.h"

using namespace std;
namespace fs = std::filesystem;

bool is_image(const fs::path& p)
{
	if (!fs::exists(p) || !fs::is_regular_file(p))
		return false;

	string ext = p.extension().string();
	return !ext.compare(".jpeg") || !ext.compare(".jpg") || !ext.compare(".png");
}

int main(int argc, char **argv) 
{
	// Enable UTF-8 multibyte encoding
	setlocale(LC_ALL, "en_US.UTF-8");

    // Print usage
    if (argc != 2) {
		cerr << "Usage: " << argv[0] << " <path>" << endl;
        return 1;
    }

	// Initialize controller & loop
	Control::get_instance().loop(argv[1]);
    return 0;
}