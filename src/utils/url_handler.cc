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
#if defined(__linux__) || defined(_WIN32)
#ifdef _WIN32
    socket_path = std::string(std::getenv("TEMP")) + "\\decirc.sock";
#endif
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, socket_path.c_str());
    unlink(local.sun_path);
#ifdef _WIN32
    if (bind(sock_fd, (struct sockaddr*)&local, sizeof(local)) < 0) {
#else
    if (bind(sock_fd, (struct sockaddr*)&local,
             strlen(local.sun_path) + sizeof(local.sun_family)) != 0) {
#endif
      std::cout << "Bind failed\n";
      return;
    }
    if (listen(sock_fd, 5) != 0) {
      std::cout << "listen failed\n";
      return;
    }
#endif
  }
  g_url_handler_instance = this;
}
UrlHandler::~UrlHandler() {
#if defined(__linux__) || defined(_WIN32)
 #ifdef _WIN32
  closesocket(sock_fd);
#else
  close(sock_fd);
 #endif
  unlink(socket_path.c_str());
#endif
}
#if defined(__linux__) || defined(_WIN32)
void UrlHandler::tick() {
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 1000 * 35;
  fd_set set;
  FD_ZERO(&set);
  FD_SET(sock_fd, &set);
  if (select(sock_fd+1, &set, NULL, NULL, &tv) > 0) {
#ifdef _WIN32
      int sock_len = 0;
    SOCKET client = accept(sock_fd, nullptr, nullptr);
#else
      unsigned int sock_len = 0;
    int client = accept(sock_fd, (struct sockaddr*)&remote, &sock_len);

#endif
    char recv_buffer[4096];

    int data_recv = recv(client, recv_buffer, sizeof(recv_buffer), 0);
    if (data_recv > 0) {
      handle(recv_buffer);
      send(client, "received", 8, 0);
    }
#ifdef _WIN32
    closesocket(client);
#else
    close(client);
#endif
  }
}
bool UrlHandler::maybeSend(const char* url) {
#ifdef _WIN32
  std::string socket_path = std::string(std::getenv("TEMP")) + "\\decirc.sock";
  if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(socket_path.c_str()) &&
      GetLastError() == ERROR_FILE_NOT_FOUND) {
    return false;
  }
#else
  std::string socket_path = "/tmp/decirc.sock";
  if (!std::filesystem::exists(socket_path)) {
    return false;
  }
#endif
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
  int res = recv(sock, recv_data, 8, 0);
  if (res != 8) {
    std::cout << "receive\n";
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    return false;
  }
#ifdef _WIN32
  closesocket(sock);
#else
  close(sock);
#endif
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