#include "AppState.h"
#ifdef __linux__
#include "utils/url_handler.h" 
#endif
#include <filesystem>
int main(int argc, char** argv) {
#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
#endif
#ifdef DEC_MAC_PATH_BASE
  std::string argAsString(argv[0]);
  std::filesystem::path basePath = argAsString;
  auto cwd = basePath.parent_path();
#else
  auto cwd = std::filesystem::current_path();
#endif
  AppState app(cwd);
  if(argc > 1) {
    #ifdef __linux__
      if(UrlHandler::maybeSend(argv[1])) {
        return 0;
      }
    #endif
      app.start_url = std::string(argv[1]);
  }
  app.start();
  return 0;
}