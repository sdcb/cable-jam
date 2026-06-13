#pragma once

#include <string>
#include <vector>

namespace cable::core {

std::string ReadTextFile(const std::string& path);
bool WriteTextFile(const std::string& path, const std::string& text);
bool DirectoryExists(const std::string& path);
bool CreateDirectories(const std::string& path);
std::string CurrentDirectory();

} // namespace cable::core
