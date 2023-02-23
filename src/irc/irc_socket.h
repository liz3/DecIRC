#ifndef DEC_IRC_SOCKET_H
#define DEC_IRC_SOCKET_H

#include <string>
#include <fcntl.h>
#include "../../third-party/IXWebSocket/ixwebsocket/IXSocketFactory.h"
#include "../../third-party/IXWebSocket/ixwebsocket/IXSocket.h"

struct addrinfo;


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