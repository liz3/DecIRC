#ifndef DEC_APP_STATE
#define DEC_APP_STATE
class GuiComponents;
class TextReceiver;
class GLFWwindow;
#include <unistd.h>
#include <codecvt>
#include <iostream>
#include <string>
#include "../third-party/freetype2/include/ft2build.h"
#include "rendering/opengl_state.h"
#include "components/text_receiver.h"
#include "discord/discord_client.h"
#include "rendering/font_atlas.h"
#ifndef __APPLE__
#include <algorithm>
#endif

class AppState {
 public:
  GLFWwindow* window;
  FontAtlas atlas;
  DiscordClient* client;
  GuiComponents* components;
  void runGuiLoop();
  uint32_t window_height = 720;
  uint32_t window_width = 1280;

  static AppState* gState;
  OpenGLState opengl_state;
  AppState();
  void start();
  Vec2f getPositionAbsolute(float x, float y, float w, float h);
  void setTextReceiver(TextReceiver* recv);
  void emptyEvent();
  // Receivers
  TextReceiver* current_text_receiver = nullptr;
};

#endif