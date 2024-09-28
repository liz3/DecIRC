#ifndef DEC_URL_HANDLER_H
#define DEC_URL_HANDLER_H
#include "../irc/irc_event_handler.h"
#ifdef __linux__
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#endif

class DecConfig;

class UrlHandler {
private:
    IrcEventHandler* client;
    DecConfig* config;
#ifdef __linux__
    std::string socket_path = "/tmp/decirc.sock";
    int sock_fd;
    struct sockaddr_un local, remote;
#endif
public:
  UrlHandler(IrcEventHandler* client_, DecConfig* config_);
  ~UrlHandler();
  void handle(const char*);
#ifdef __linux__
  void tick();
  static bool maybeSend(const char*);
#endif
};


#endif