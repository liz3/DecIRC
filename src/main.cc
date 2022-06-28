#include "AppState.h"
#include <filesystem>
#include <iostream>

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
  AppState app;
  app.start();
  return 0;
}