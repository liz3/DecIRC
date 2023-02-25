#ifndef DEC_TEXT_BOX
#define DEC_TEXT_BOX
#include "font_atlas.h"
#include "shader.h"
#include "text.h"
#include "../irc/irc_messages.hpp"

struct RichChar {
  uint8_t type;
  uint32_t cp;
  std::string emote;
  std::string style;
  bool strikethrough = false;
  bool underline = false;
  Vec4f color = vec4fs(0);
};

struct Marker {
  float startX;
  bool active = false;
};

class TextBox {
 private:
  ShaderInstance* m_shader;
  FontAtlas* atlas;

 public:
  TextWithState& text;
  TextBox(TextWithState& text);
  void render(float x, float y, float w, float h);
  int computeHeight(float w, int preclude = 0);
  float computeHeightAbsolute(float w, int preclude = 0);
  void renderLine(float xStart, float xEnd, float y);
  bool renderCursor = false;
  bool allowGrow = false;
  bool growDown = false;
  Vec4f background = vec4fs(0.0);
  Vec4f color = vec4fs(1.0);
  std::string style = "normal";
  float scale = 1;
  bool richRender = false;
  IrcMessageMsg* msg_ref = nullptr;
  std::vector<RichChar> rich_cache;
  uint32_t last_data_point = -1;
  std::vector<RichChar>& preprocess();
};
#endif