#include "web_util.h"
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif
#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CFBundle.h>
#endif
void dec_open_in_browser(std::string& url) {
#ifdef _WIN32
ShellExecute(0, 0, url.c_str(), 0, 0 , SW_SHOW );
#endif
#ifdef __APPLE__
  CFURLRef cfurl = CFURLCreateWithBytes (
      NULL,                        
      (UInt8*)url.c_str(),     
      url.length(),            
      kCFStringEncodingASCII,      
      NULL                         
    );
  LSOpenCFURLRef(cfurl,0);
  CFRelease(cfurl);
#endif
#ifdef __linux__
std::string cmd = "xdg-open " + url;
system(cmd.c_str());
#endif
}