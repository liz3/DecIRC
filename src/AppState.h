#ifndef DEC_APP_STATE
#define DEC_APP_STATE
class GuiComponents;
class TextReceiver;
class GLFWwindow;
#ifndef _WIN32
#include <unistd.h>
#endif
#include <codecvt>
#include <iostream>
#include <string>
#include "../third-party/freetype2/include/ft2build.h"
#include "rendering/opengl_state.h"
#include "components/text_receiver.h"
#include "components/mouse_receiver.h"
#include "irc/irc_event_handler.h"
#include "rendering/font_atlas.h"
#ifndef __APPLE__
#include <algorithm>
#endif
#include "utils/config.h"

class UrlHandler;

class AppState {
 public:
  GLFWwindow* window;
  FontAtlas atlas;
  IrcEventHandler* client;
  GuiComponents* components;
  void runGuiLoop(UrlHandler*);
  uint32_t window_height = 720;
  uint32_t window_width = 1280;
  double mouse_x = 0, mouse_y = 0;
  float window_scale = 1;
  bool focused = true;
  DecConfig config;
  static AppState* gState;
  OpenGLState opengl_state;
  std::filesystem::path cwd;
  std::string start_url;
  AppState(std::filesystem::path cwd);
  void start();
  Vec2f getPositionAbsolute(float x, float y, float w, float h);
  void setTextReceiver(TextReceiver* recv, MouseReceiver* mouse_recv = nullptr);
  void emptyEvent();
  // Receivers
  TextReceiver* current_text_receiver = nullptr;
  MouseReceiver* current_mouse_receiver = nullptr;
};

#endif