#include "irc_socket.h"
#include "preclude.h"

IrcSocket::IrcSocket(std::string host,
                     uint16_t port,
                     bool useTLS,
                     bool verifySSL)
    : host(host), port(port), useTLS(useTLS), verifySSL(verifySSL) {
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
  std::atomic<bool> requestInitCancellation(false);

  auto isCancellationRequested =
      ix::makeCancellationRequestWithTimeout(15, requestInitCancellation);

  sock = ix::createSocket(useTLS, -1, err, opts);
  connected = sock->connect(host, port, err, isCancellationRequested);
}
bool IrcSocket::write(std::string& data) {
  #ifndef _WIN32
  auto result = sock->isReadyToWrite(1);
  while (result == ix::PollResultType::Timeout) {
    result = sock->isReadyToWrite(1);
  }
  if (result != ix::PollResultType::ReadyForWrite)
    return false;
#endif
  std::cout << ">" << data;
  sock->send((char*)data.c_str(), data.length());
  return true;
}
std::string IrcSocket::read(size_t len) {
  #ifndef _WIN32
  auto result = sock->isReadyToRead(25);
  while (result == ix::PollResultType::Timeout) {
    result = sock->isReadyToRead(25);
  }
  if (result != ix::PollResultType::ReadyForRead)
    return "";
  #endif
  char* buf = new char[len];
  std::string value;
  ssize_t res = sock->recv(buf, len);
  if (res > 0) {
    value = std::string(buf, res);
  }
  delete[] buf;
  return value;
}
