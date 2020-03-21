#include "util.h"

using namespace std;
namespace fs = std::filesystem;

bool Util::is_image(const fs::path& p)
{
	if (!fs::exists(p) || !fs::is_regular_file(p))
		return false;

	string ext = p.extension().string();
	return !ext.compare(".jpeg") || !ext.compare(".jpg") || !ext.compare(".png");
}


std::filesystem::path Util::get_respath(const char* f)
{
	return _respath / f;
}

fs::path Util::_respath;