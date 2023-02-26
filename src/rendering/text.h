#ifndef DEC_TEXT
#define DEC_TEXT

#include "../utils/unicode_utils.h"
#include "la.h"
class TextWithState {
 public:
  std::vector<int32_t> data;
  uint32_t last_point = 0;
  uint32_t text_x = 0;
  TextWithState(std::string data);
  TextWithState();
  Vec4f getCursorPosition();
  void append(int32_t code);
  void append(std::string data);
  void setData(std::string content);
  void remove();
  void moveLeft();
  void moveRight();
  void clearData();
  uint32_t countChar(int32_t cp);
  std::string getUtf8Value();
};

#endif