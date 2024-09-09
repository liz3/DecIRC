#ifndef DEC_MESSAGE_LIST
#define DEC_MESSAGE_LIST
#include <stdint.h>
#include "../rendering/box.h"
#include "../rendering/image.h"
#include "../rendering/text.h"
#include <mutex>
#include "text_receiver.h"
#include "mouse_receiver.h"
#include "component.h"

#include "../rendering/text_box.h"
class MessageHolder;

class EmbedRender {
 public:
  Image* image = nullptr;
  TextWithState title, description, footer;
  TextBox t_box, d_box, f_box;
  float height = 0.0;
  float ah;
  float getHeight(float w, float atlas_height);
  EmbedRender();
  ~EmbedRender();
  void render(float x, float y, float w);
};
class RenderMessage : public TextReceiver{
 public:
  TextWithState text;
  TextBox box;
  TextWithState title;
  TextBox title_box;
  MessageHolder* m_holder;
  bool hasFocus = false;
  float render_x = 0, render_y = 0, render_w = 0;
  RenderMessage(MessageHolder* holder);
  std::vector<Image*> images;
  std::vector<std::string> links;
  std::vector<EmbedRender*> embeds;
  ~RenderMessage();
  int getHeight(float w, float atlas_height);
  void render(float x, float y, float w, bool selected = false);
  float atlas_height;
  float start = 0.0;
  float height = 0.0;
  float content_height = 0.0;
  void fetchImage(std::string url);
  void fetchImages();
  void disposeImages();
  bool initial_loading = false;
  bool canFocus() override;
  void onFocus(bool focus) override;
  void render(float x, float y, float w, float h) override;
  void onEnter() override;
  void onCodePoint(int32_t cp) override;
  void onKey(GLFWwindow* window,
             int key,
             int scancode,
             int action,
             int mods) override;
  bool isImage(std::string& url);

  void addText(std::string text) override;
  std::string getText() override;
 
};

class MessageList : public Component,  public MouseReceiver  {
 public:
  std::vector<RenderMessage*> messages;
  float height = 0.0;
  float current_offset = 0.0;
  float available_height = 0.0;
  float width = 500;
  int32_t selected_index = -1;
  RenderMessage* addContent(MessageHolder* h, bool prepend = false);
  std::vector<MessageHolder*>* msg_load_ptr = nullptr;
  bool hasFocus = false;
  void clearList();
  bool canFocus() override;
  void onFocus(bool focus) override;
  void render(float x, float y, float w, float h) override;
  void recomputeHeights();
  void setWidth(float width);
  void setAvailableHeight(float y);
  void changeScroll(float offset);
  void scrollEnd();
  void selectIndex(int32_t diff);
  void updateEntry(MessageHolder*);
  void removeEntry(MessageHolder*);
  void onMousePress(double x, double y, int button, int action) override;
  void onMouseWheel(double xoffset, double yoffset) override;
};
#endif