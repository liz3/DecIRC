#ifndef DEC_EVENT_RECEIVER
#define DEC_EVENT_RECEIVER
#include "../third-party/glfw/include/GLFW/glfw3.h"
#include "../third-party/png/lodepng.h"

#include "AppState.h"
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
  glViewport(0, 0, width, height);
  float xscale, yscale;
  glfwGetWindowContentScale(window, &xscale, &yscale);
  AppState::gState->window_scale = xscale;
#ifdef _WIN32
  AppState::gState->window_width = width * xscale;
  AppState::gState->window_height = height * yscale;
#else
  AppState::gState->window_width = width;
  AppState::gState->window_height = height;
#endif

};
void window_focus_callback(GLFWwindow* window, int focused) {
  AppState::gState->focused = focused;
  if(!focused) {
    AppState::gState->client->persistChannels();
  }
    if (AppState::gState->current_text_receiver) {
    AppState::gState->current_text_receiver->onFocus(focused);
  }
};

void character_callback(GLFWwindow* window, unsigned int codepoint) {
  auto* st = AppState::gState;
  if (st->current_text_receiver) {
     bool ctrl_pressed = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                      glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
         bool alt_pressed =
          glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS;
    if(ctrl_pressed || alt_pressed)
      return;
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
  bool cmd =
#ifdef __APPLE__
   glfwGetKey(window, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS;
#else
      glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS;
#endif
  bool x_pressed = glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS;

  if (key == GLFW_KEY_ESCAPE && isPress) {
    if (st->components->active_popover) {
      st->components->setActivePopover(nullptr);
      st->setTextReceiver(&st->components->chat_input);
      return;
    } else if (st->client->editMode) {
      st->client->cancelEdit();
      return;
    }
  }
  if (isPress && !ctrl_pressed && !shift_pressed && key == GLFW_KEY_ENTER) {
    if (st->current_text_receiver)
      st->current_text_receiver->onEnter();
    return;
  }
  if(cmd && key == GLFW_KEY_K && isPress) {
    st->client->initQuickSearch();
    return;
  }
  if (ctrl_pressed) {
    if (x_pressed) {

      if (isPress && key == GLFW_KEY_E) {
      st->components->message_list.scrollEnd();
    }  else if (isPress && key == GLFW_KEY_U) {
      st->client->renderUserList();
      return;
    } else if (isPress && key == GLFW_KEY_D) {
      st->components->root_list_display = 1;
      st->setTextReceiver(&st->components->channel_list, &st->components->channel_list);
      return;
    } else if (isPress && key == GLFW_KEY_G) {
      st->components->root_list_display = 2;

      st->setTextReceiver(&st->components->network_list, &st->components->network_list);
      return;
    } 
      return;
    }
    if (shift_pressed && isPress && (key == GLFW_KEY_P || key == GLFW_KEY_N)) {
      st->components->message_list.selectIndex(key == GLFW_KEY_P ? 1 : -1);
      return;
    }
    if (isPress && key == GLFW_KEY_M) {
            st->current_mouse_receiver = &st->components->message_list;
      st->setTextReceiver(&st->components->chat_input);

      return;
    }else if (isPress && key == GLFW_KEY_U) {
      st->components->message_list.changeScroll(-25);
      return;
    } else if (isPress && key == GLFW_KEY_D) {
      st->components->message_list.changeScroll(25);
      return;
    } else if (isPress && key == GLFW_KEY_C) {
      if (st->current_text_receiver) {
        std::string content = st->current_text_receiver->getText();
        glfwSetClipboardString(window, content.c_str());
        return;
      }
    } else if (isPress && key == GLFW_KEY_V) {
      if (st->current_text_receiver) {
        {
          GLFWimage size;
          const char* data = glfwGetClipboardPng(&size);
          if (data != nullptr) {
#ifdef _WIN32
            std::vector<uint8_t> in(data, data + size.size);
            std::vector<uint8_t> vecData;
            unsigned error =
                lodepng::encode(vecData, in, size.width, size.height);

#else
            uint8_t* dPtr = (uint8_t*)data;
            std::vector<uint8_t> vecData(dPtr, dPtr + size.size);
#endif
            HttpFileEntry* e = new HttpFileEntry();
            *e = {"unknown.png", "", "image/png", "", vecData};
            st->client->sendFiles.push_back(e);
            return;
          }
        }
        const char* content = glfwGetClipboardString(window);
        std::string str(content);
        st->current_text_receiver->addText(str);
        return;
      }
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

  auto* st = AppState::gState;
  auto sc = st->window_scale;
  if(action == 0) {
    if(st->mouse_x >= (470.0 / sc) && st->mouse_x <= (470.0 /sc)+(((float)st->window_width-490) / sc) && st->mouse_y >= ((float)st->window_height/sc)-40 &&  st->mouse_y <= ((float)st->window_height / sc)-7) {
          st->setTextReceiver(&st->components->chat_input);
          return;
    }
  }
  if(st->current_mouse_receiver) {
    st->current_mouse_receiver->onMousePress(st->mouse_x, st->mouse_y, button, action);
  }
};
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    auto* st = AppState::gState;
  if(st->current_mouse_receiver) {
    st->current_mouse_receiver->onMouseWheel(xoffset, yoffset);
  }
}

void mouse_position_callback(GLFWwindow* window,
                           double x, double y){
  auto* st = AppState::gState;
  st->mouse_x = x;
  st->mouse_y = y;
};
#endif