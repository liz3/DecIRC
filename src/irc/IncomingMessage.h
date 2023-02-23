#ifndef DEC_INCOMING_MESSAGE_H
#define DEC_INCOMING_MESSAGE_H
#include <string>
#include "irc_messages.hpp"

struct Source {
  std::string nick;
  std::string username;
  std::string host;
  bool onlyHost;
  const std::string& getName() const {
    if (onlyHost) {
      return host;
    }
    return nick;
  }
};
class IncomingMessage {
 public:
  std::string raw;
  std::string tag;
  Source source;
  std::string command;
  std::string parameters;
  bool forUser = false;
  bool isNumericCommand;
  uint32_t numericCommand = -1;
};

#endif