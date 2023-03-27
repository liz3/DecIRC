#include "text_field.h"
#include "../../third-party/glfw/include/GLFW/glfw3.h"
#include "../AppState.h"

TextField::TextField() : box(text) {}
bool TextField::canFocus() {
  return true;
}
void TextField::onFocus(bool focus) {
  box.renderCursor = focus;
}
void TextField::onEnter() {
  enter_cb(text.getUtf8Value());
}
void TextField::render(float x, float y, float w, float h) {
  auto absolute = AppState::gState->getPositionAbsolute(x, y, w, h);
  box.render(absolute.x, absolute.y, w, h);
}

void TextField::onCodePoint(int32_t cp) {
  text.append(cp);
  if (has_change_cb)
    on_change_cb(text.getUtf8Value());
}
void TextField::onKey(GLFWwindow* window,
                      int key,
                      int scancode,
                      int action,
                      int mods) {
  bool is_press = action == GLFW_PRESS || action == GLFW_REPEAT;
  bool ctrl_pressed = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                      glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
  if (is_press) {
    if (key == GLFW_KEY_BACKSPACE) {
      text.remove();
    } else if (key == GLFW_KEY_ENTER) {
      text.append('\n');
    } else if (key == GLFW_KEY_LEFT) {
      text.moveLeft();
    } else if (key == GLFW_KEY_RIGHT) {
      text.moveRight();
    } else if (key == GLFW_KEY_UP) {
      text.moveUp();
    } else if (key == GLFW_KEY_DOWN) {
      text.moveDown();
    } else if (key == GLFW_KEY_K && ctrl_pressed) {
      text.clearData();
    }
  }
}
void TextField::addText(std::string newContent) {
  text.append(newContent);
}
std::string TextField::getText() {
  return text.getUtf8Value();
}
void TextField::setOnChangeCb(const OnEnterCallback& cb) {
  has_change_cb = true;
  on_change_cb = cb;
}