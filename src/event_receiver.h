#ifndef DEC_EVENT_RECEIVER
#define DEC_EVENT_RECEIVER
#include "../third-party/glfw/include/GLFW/glfw3.h"
#include "AppState.h"
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
  glViewport(0, 0, width, height);
  AppState::gState->window_width = width;
  AppState::gState->window_height = height;

  //    if(gState != nullptr) {
  //       gState->invalidateCache();
  //  #ifdef _WIN32
  //       float xscale, yscale;
  //       glfwGetWindowContentScale(window, &xscale, &yscale);
  //       gState->WIDTH = (float)width * xscale;
  //       gState->HEIGHT = (float)height * yscale;
  // #else
  //       gState->WIDTH = (float)width;
  //       gState->HEIGHT = (float)height;
  // #endif
  //     }
};
void window_focus_callback(GLFWwindow* window, int focused){

};

void character_callback(GLFWwindow* window, unsigned int codepoint) {
  auto* st = AppState::gState;
  if (st->current_text_receiver) {
    st->current_text_receiver->onCodePoint(codepoint);
  }
}
void key_callback(GLFWwindow* window,
                  int key,
                  int scancode,
                  int action,
                  int mods) {
  bool isPress = action == GLFW_PRESS || action == GLFW_REPEAT;
  auto* st = AppState::gState;
  bool ctrl_pressed = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                      glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
  bool shift_pressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
  bool cmd = glfwGetKey(window, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS;
  bool x_pressed = glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS;

  if (key == GLFW_KEY_ESCAPE && isPress) {
    if (st->components->active_popover) {
      st->components->setActivePopover(nullptr);
      return;
    }
  }
  if (isPress && !ctrl_pressed && !shift_pressed && key == GLFW_KEY_ENTER) {
    if (st->current_text_receiver)
      st->current_text_receiver->onEnter();
    return;
  }
  if (ctrl_pressed) {
    if (x_pressed) {
      if (isPress && key == GLFW_KEY_D) {
        st->client->tryDelete();
      }
      if (isPress && key == GLFW_KEY_E) {
        st->client->startDmCall();
      }
      return;
    }
    if (shift_pressed && isPress && (key == GLFW_KEY_P || key == GLFW_KEY_N)) {
      st->components->message_list.selectIndex(key == GLFW_KEY_P ? 1 : -1);
      return;
    }
    if (isPress && key == GLFW_KEY_M) {
      st->setTextReceiver(&st->components->chat_input);

      return;
    } else if (isPress && key == GLFW_KEY_E) {
      st->components->message_list.scrollEnd();
      return;
    } else if (isPress && key == GLFW_KEY_U) {
      st->client->renderUserInfo();
      return;
    } else if (isPress && key == GLFW_KEY_P) {
      st->components->message_list.changeScroll(-25);
      return;
    } else if (isPress && key == GLFW_KEY_N) {
      st->components->message_list.changeScroll(25);
      return;
    } else if (isPress && key == GLFW_KEY_D) {
      st->components->root_list_display = 1;
      st->setTextReceiver(&st->components->dm_list);
      return;
    } else if (isPress && key == GLFW_KEY_G) {
      st->components->root_list_display = 2;
      st->setTextReceiver(&st->components->guilds_list);
      return;
    }
  }
  if (st->current_text_receiver) {
    st->current_text_receiver->onKey(window, key, scancode, action, mods);
  }
};
void mouse_button_callback(GLFWwindow* window,
                           int button,
                           int action,
                           int mods){

};
#endif