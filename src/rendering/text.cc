#include "text.h"

TextWithState::TextWithState() {}
TextWithState::TextWithState(std::string content) {
  setData(content);
}
void TextWithState::setData(std::string content) {
  data = UnicodeUtils::utf8_to_codepoint(content);
  text_x = data.size();
  last_point++;
}
Vec4f TextWithState::getCursorPosition() {
  uint32_t x = 0;
  uint32_t y = 0;
  uint32_t lastX = 0;
  for (int i = 0; i < text_x; ++i) {
    if (data[i] == (int32_t)'\n') {
      y++;
      lastX = i;
      x = 0;
    } else {
      x++;
    }
  }
  return vec4f(x, y, lastX, 0);
}
void TextWithState::append(int32_t code) {
  data.insert(data.begin() + text_x, code);
  text_x++;
  last_point++;
}
void TextWithState::append(std::string str) {
  auto pts = UnicodeUtils::utf8_to_codepoint(str);
  data.insert(data.begin() + text_x, pts.begin(), pts.end());
  text_x += pts.size();
  last_point++;
}
void TextWithState::remove() {
  if (text_x == 0)
    return;
  data.erase(data.begin() + text_x - 1);
  text_x--;
  last_point++;
}
void TextWithState::clearData() {
  data.clear();
  text_x = 0;
  last_point++;
}
void TextWithState::moveLeft() {
  if (text_x > 0)
    text_x--;
}
void TextWithState::moveRight() {
  if (text_x < data.size())
    text_x++;
}
std::string TextWithState::getUtf8Value() {
  return UnicodeUtils::unicode_to_utf8(data);
}
uint32_t TextWithState::countChar(int32_t cp) {
  int c = 0;
  for (int32_t cc : data) {
    if (cc == cp) {
      c++;
    }
  }
  return c;
}