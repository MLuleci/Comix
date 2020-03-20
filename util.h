#pragma once
#include <filesystem>
#include <string>
#define sign(x) (x > 0 ? 1 : (x < 0 ? -1 : 0))

bool is_image(const std::filesystem::path&);

bool starts_with(std::string&, const char*);
bool ends_with(std::string&, const char*);