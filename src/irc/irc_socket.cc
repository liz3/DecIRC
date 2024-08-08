#include "irc_socket.h"
#include "preclude.h"

IrcSocket::IrcSocket(std::string host,
                     uint16_t port,
                     bool useTLS,
                     bool verifySSL)
    : host(host), port(port), useTLS(useTLS), verifySSL(verifySSL), cancelled(false) {
  setupAndConnect();
}
IrcSocket::IrcSocket() {}
void IrcSocket::close() {
  if (sock) {
    sock->close();
    sock = nullptr;
  }
  connected = false;
}
bool IrcSocket::isConnected() {
  return connected;
}
void IrcSocket::setupAndConnect() {
  ix::SocketTLSOptions opts;
  if (!verifySSL)
    opts.caFile = "NONE";

  auto cancelled =
      ix::makeCancellationRequestWithTimeout(15, this->cancelled);
 auto isCancellationRequested = [&]() {
            return cancelled();
  };
  sock = ix::createSocket(useTLS, -1, err, opts);
  connected = sock->connect(host, port, err, isCancellationRequested);
}
bool IrcSocket::write(std::string& data) {
  auto cancelled =
      ix::makeCancellationRequestWithTimeout(15, this->cancelled);
 auto isCancellationRequested = [&]() {
            return cancelled();
  };
  bool out = sock->writeBytes(data, cancelled);
  std::cout << ">" << data;
  return out;
}
std::string IrcSocket::read(size_t len) {
  while(true) {
      char* buf = new char[len];
  std::string value;
  ssize_t res = sock->recv(buf, len);
  if (res > 0) {
    value = std::string(buf, res);
  } 
  delete[] buf;
  if(res <= 0) {
    if(ix::Socket::isWaitNeeded())
      continue;
  }
  return value;
  }
}
