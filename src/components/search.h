#ifndef DEC_SEARCH_H
#define DEC_SEARCH_H

#include "popover.h"
#include "text_field.h"
#include "../../third-party/glfw/include/GLFW/glfw3.h"

class Search : public TextField, public Popover {
private:
    bool next = false;
public:
  Search() {
    enter_cb = [&](std::string c) {
      next = true;
    };
  }
bool getNext() {
  bool v = next;
  next = false;
  return v;
}
void render(float window_width, float window_height) override {
    TextField::render(470, window_height - 85,
                    window_width - 490, 60);
}
};

#endif