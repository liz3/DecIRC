#include "message_util.h"
#include <sstream>


std::string IrcMessageUtil::stripMessage(const std::string& input) {
    std::stringstream ss;
  for (int i = 0; i < input.size(); ++i) {
    auto cp = input[i];
      if (cp == 0x02) {
        continue;
      }
      if (cp == 0x0F) {
        continue;
      }
      if (cp == 0x01D) {
        continue;
      }
      if (cp == 0x01F) {
        continue;
      }
      if (cp == 0x01E) {
        continue;
      }
      if (cp == 0x011) {
        continue;
      }
      if (cp == 0x03) {
        auto next = input[i + 1];
        if (next == ',') {
          continue;
        }
        if (next >= '0' && next <= '9') {
          auto sec = input[i + 2];
          if (sec == ',') {
            continue;
          } else if (sec >= '0' && sec <= '9') {
            std::string fg(1, (char)next);
            fg += (char)sec;
            auto third = input[i + 3];
            if (third == ',') {
              i += 5;
            } else {
              i += 2;
            }
            continue;
          } else {
            i += 1;
          }
        } else {
          continue;
        }
      }
      ss << cp;
  }

    return ss.str();
}