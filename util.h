#pragma once
#include <filesystem>
#include <string>
#include <mutex>
#include <SDL.h>
#define sign(x) (x > 0 ? 1 : (x < 0 ? -1 : 0))
#define aquire(x) std::scoped_lock<std::recursive_mutex> __lk(x);

class Util {
	static std::filesystem::path _respath;
	friend int main(int, char**);
public:
	static bool is_image(const std::filesystem::path&);
	static std::filesystem::path get_respath(const char*);
};