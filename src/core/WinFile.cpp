#include "core/WinFile.h"

#include <fstream>
#include <sstream>

#include <windows.h>

namespace cable::core {
namespace {

std::string ParentPath(const std::string& path) {
    const std::size_t slash = path.find_last_of("\\/");
    if (slash == std::string::npos) {
        return {};
    }
    if (slash == 0) {
        return path.substr(0, 1);
    }
    if (slash == 2 && path[1] == ':') {
        return path.substr(0, 3);
    }
    return path.substr(0, slash);
}

} // namespace

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
    const std::string parent = ParentPath(path);
    if (!parent.empty()) {
        CreateDirectories(parent);
    }
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        return false;
    }
    output << text;
    return static_cast<bool>(output);
}

bool DirectoryExists(const std::string& path) {
    const DWORD attributes = GetFileAttributesA(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool CreateDirectories(const std::string& path) {
    if (path.empty() || DirectoryExists(path)) {
        return true;
    }
    const std::string parent = ParentPath(path);
    if (!parent.empty() && parent != path && !DirectoryExists(parent) && !CreateDirectories(parent)) {
        return false;
    }
    if (CreateDirectoryA(path.c_str(), nullptr)) {
        return true;
    }
    return GetLastError() == ERROR_ALREADY_EXISTS && DirectoryExists(path);
}

std::string CurrentDirectory() {
    const DWORD required = GetCurrentDirectoryA(0, nullptr);
    if (required == 0) {
        return {};
    }
    std::string path(required, '\0');
    const DWORD written = GetCurrentDirectoryA(required, path.data());
    if (written == 0) {
        return {};
    }
    path.resize(written);
    return path;
}

} // namespace cable::core
