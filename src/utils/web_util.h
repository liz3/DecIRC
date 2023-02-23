#ifndef DEC_WEB_UTILS
#define DEC_WEB_UTILS
#include <string>
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif
void dec_open_in_browser(std::string& url) {
#ifdef _WIN32
ShellExecute(0, 0, url.c_str(), 0, 0 , SW_SHOW );
#endif
#ifdef __APPLE__
std::string cmd = "open " + url;
#endif
#ifdef __LINUX__
std::string cmd = "xdg-open " + url;
system(cmd);
#endif
}

#endif