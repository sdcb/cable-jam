#include "core/WinFile.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace cable::core {

std::string ReadTextFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return {};
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

bool WriteTextFile(const std::string& path, const std::string& text) {
    const std::filesystem::path file(path);
    if (file.has_parent_path()) {
        std::filesystem::create_directories(file.parent_path());
    }
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        return false;
    }
    output << text;
    return static_cast<bool>(output);
}

bool DirectoryExists(const std::string& path) {
    return std::filesystem::is_directory(path);
}

bool CreateDirectories(const std::string& path) {
    return std::filesystem::create_directories(path) || std::filesystem::is_directory(path);
}

std::string CurrentDirectory() {
    return std::filesystem::current_path().string();
}

} // namespace cable::core
