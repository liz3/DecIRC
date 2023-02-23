#ifndef DEC_IRC_SOCKET_H
#define DEC_IRC_SOCKET_H

#include <string>
#include <netdb.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../../third-party/IXWebSocket/ixwebsocket/IXSocketFactory.h"
#include "../../third-party/IXWebSocket/ixwebsocket/IXSocket.h"

struct addrinfo;
#ifdef _WIN32
typedef SOCKET socket_t;
#else
typedef int socket_t;
#endif

class IrcSocket {
 public:
  IrcSocket(std::string host, uint16_t port, bool useTLS, bool verifySSL);
  IrcSocket();
  bool write(std::string& data);
  std::string read(size_t len = 4096);
  void close();
  bool isConnected();

 private:
  void setupAndConnect();
  bool useTLS;
  bool verifySSL;
  std::string err;

  std::string host;

  uint16_t port;
  bool connected;
  std::unique_ptr<ix::Socket> sock;
};
#endif