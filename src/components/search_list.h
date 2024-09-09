#ifndef DEC_SEARCH_LIST
#define DEC_SEARCH_LIST

#include "../rendering/text.h"
#include "text_receiver.h"
#include "mouse_receiver.h"

#include <string>
#include <vector>
#include <functional>
#ifdef __linux__
#include <cstdint>
#endif

struct SearchItem {
  std::string name;
  std::vector<uint8_t> icon;
  uint32_t icon_width;
  uint32_t icon_height;
  Vec4f color = vec4fs(1);
  void* user_data = nullptr;
  float x = 0, y = 0, w = 0;
};
using OnSelectionCallback = std::function<void(const SearchItem*)>;

class SearchList : public TextReceiver, public MouseReceiver {
 public:
  SearchList();

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
  void addText(std::string text) override;

  std::string getText() override;
  TextWithState text;
  SearchItem* current_selected = nullptr;
  TextWithState title;
  int selected_index = -1;
  void setItems(std::vector<SearchItem>* items);
  void setCallback(const OnSelectionCallback& cb);
  void recompute();
  void onMousePress(double x, double y, int button, int action) override;
  void onMouseWheel(double xoffset, double yoffset) override;
 private:
  std::vector<SearchItem>* items = nullptr;
  std::vector<SearchItem*> filtered;
  OnSelectionCallback select_cb;
  bool focused = false;
};
#endif