#pragma once
#include <filesystem>

namespace Control 
{
    void init(std::filesystem::path);
    void loop();
}