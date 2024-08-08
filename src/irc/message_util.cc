#include "message_util.h"
#include <sstream>


std::string IrcMessageUtil::stripMessage(const std::string& input) {
    std::stringstream ss;
  for (int i = 0; i < input.size(); ++i) {
    auto cp = input[i];
      if (cp == 0x02) {
        continue;
      }
      if (cp == 0x04) {
        i += 6;
        continue;
      }
      if(cp == 0x16) {
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
        if (next < '0' || next > '9')
          continue;
        i += 1;
        next = input[i + 1];
        if (next >= '0' && next <= '9') {
          i += 1;
          next = input[i + 1];
        }
        if (next == ',') {
          char after =  input[i + 2];
          if (after >= '0' && after <= '9') {
              i += 2;
              after = input[i + 1];
              if (after >= '0' && after <= '9') {
                i += 1;
              }
          }
        }
        continue;

      }

      ss << cp;
  }

    return ss.str();
}