#ifndef DEC_URL_HANDLER_H
#define DEC_URL_HANDLER_H
#include "../irc/irc_event_handler.h"
#if defined(__linux__) || defined(_WIN32)
#ifdef _WIN32
#include <afunix.h>
#include <fileapi.h>
#include <winsock2.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#endif
#endif

class DecConfig;

class UrlHandler {
private:
    IrcEventHandler* client;
    DecConfig* config;
#if defined(__linux__) || defined(_WIN32)
    std::string socket_path = "/tmp/decirc.sock";
    int sock_fd;
#ifdef _WIN32
     sockaddr_un local, remote;
#else
    struct sockaddr_un local, remote;
#endif
#endif
public:
  UrlHandler(IrcEventHandler* client_, DecConfig* config_);
  ~UrlHandler();
  void handle(const char*);
#if defined(__linux__) || defined(_WIN32)
  void tick();
  static bool maybeSend(const char*);
#endif
};


#endif