#include "url_handler.h"
#include "global-protocol-handler.h"
#include <string>
#include "../irc/stream_reader.h"
#include "url_parser.h"
#include "config.h"
#include <filesystem>

UrlHandler* g_url_handler_instance = nullptr;

void global_handle_protocol(const char* url) {
  if (g_url_handler_instance)
    g_url_handler_instance->handle(url);
}

UrlHandler::UrlHandler(IrcEventHandler* client_, DecConfig* config_)
    : client(client_), config(config_) {
  if (!g_url_handler_instance) {
#ifdef __APPLE__
    global_start_protocol_handler();
#endif
#ifdef __linux__
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, socket_path.c_str());
    unlink(local.sun_path);
    if (bind(sock_fd, (struct sockaddr*)&local,
             strlen(local.sun_path) + sizeof(local.sun_family)) != 0) {
      return;
    }
    if (listen(sock_fd, 5) != 0) {
      return;
    }
#endif
  }
  g_url_handler_instance = this;
}
UrlHandler::~UrlHandler() {
#ifdef __linux__
  close(sock_fd);
  unlink(socket_path.c_str());
#endif
}
#ifdef __linux__
void UrlHandler::tick() {
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 1000 * 35;
  fd_set set;
  FD_ZERO(&set);
  FD_SET(sock_fd, &set);
  if (select(sock_fd+1, &set, NULL, NULL, &tv) > 0) {
    unsigned int sock_len = 0;
    int client = accept(sock_fd, (struct sockaddr*)&remote, &sock_len);
    char recv_buffer[4096];
    int data_recv = recv(client, recv_buffer, sizeof(recv_buffer), 0);
    if (data_recv) {
      handle(recv_buffer);
      send(client, "received", 8, 0);
    }
    close(client);
  }
}
bool UrlHandler::maybeSend(const char* url) {
  std::string socket_path = "/tmp/decirc.sock";
  if (!std::filesystem::exists(socket_path)) {
    return false;
  }
  int sock = 0;
  int data_len = 0;
  struct sockaddr_un remote;
  if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    std::cout << "socket\n";
    return false;
  }
  remote.sun_family = AF_UNIX;
  strcpy(remote.sun_path, socket_path.c_str());
  if (connect(sock, (struct sockaddr*)&remote,
              strlen(remote.sun_path) + sizeof(remote.sun_family)) == -1) {
    std::cout << "connect\n";
    return false;
  }
  send(sock, url, strlen(url) + 1, 0);

  char recv_data[9];
  recv_data[8] = 0x00;
  if (recv(sock, recv_data, 8, 0) != 8) {
    close(sock);
    return false;
  }
  close(sock);
  std::string str(recv_data);
  return str == "received";
}
#endif

void UrlHandler::handle(const char* url) {
  std::string str(url);
  std::string protocol = "", host = "", path = "", query = "", fragment = "";
  int port = 0;
  if (UrlParser::parse(str, protocol, host, path, query, fragment, port)) {
    if (!host.length())
      return;
    bool withSSL = protocol == "ircs";
    std::vector<std::string> channels;

    if (port == -1) {
      port = withSSL ? 6697 : 6667;
    }
    if (fragment.length() > 0) {
      fragment = UrlParser::urlDecode(fragment);
      StreamReader reader(fragment);
      while (reader.rem() > 0) {
        std::string ch = reader.readUntil(',');
        if (ch.length())
          channels.push_back(ch);
        reader.skip(1);
      }
    }
    for (auto* e : client->active_networks) {
      if (e->host == host && e->port == port) {
        for (auto& ch : channels) {
          if (!e->getJoinedChannels().count(ch)) {
            e->write({"JOIN", ch});
          }
        }
        return;
      }
    }
    IrcClient* nc = new IrcClient(host, withSSL);
    nc->preJoin = channels;
    nc->networkInfo.given_name = host;
    nc->nick = config->getFallbackUserName();
    nc->username = config->getFallbackUserName();
    nc->realname = config->getFallbackUserName();
    nc->port = port;
    nc->verifySSL = false;
    nc->autoConnect = true;
    client->addNetwork(nc);
  }
}