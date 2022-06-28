#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <fstream>
#include <sstream>
namespace fs = std::filesystem;

class FileUtils {
 public:
  static std::string file_to_string(std::string path) {
    std::ifstream stream(path);
    std::stringstream ss;
    ss << stream.rdbuf();
    stream.close();
    return ss.str();
  }

  static bool string_to_file(std::string path, std::string content) {
    std::ofstream stream(path);
    if (!stream.is_open())
      return false;
    stream.write(content.c_str(), content.length());
    stream.close();
    return true;
  }
    static fs::path* getHomeFolder() {
#ifdef _WIN32
    const char* home = getenv("USERPROFILE");
#else
    const char* home = getenv("HOME");
#endif
    if(home)
      return new fs::path(home);
    return nullptr;

  }
};
#endif
