#ifndef DEC_SEARCH_LIST
#define DEC_SEARCH_LIST

#include "../rendering/text.h"
#include "text_receiver.h"

#include <string>
#include <vector>

struct SearchItem {
  std::string name;
  std::vector<uint8_t> icon;
  uint32_t icon_width;
  uint32_t icon_height;
  void* user_data = nullptr;
};
using OnSelectionCallback = std::function<void(const SearchItem*)>;

class SearchList : public TextReceiver {
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
  TextWithState text;
  SearchItem* current_selected = nullptr;
  TextWithState title;
  int selected_index = -1;
  void setItems(std::vector<SearchItem>* items);
  void setCallback(const OnSelectionCallback& cb);
  void recompute();

 private:
  std::vector<SearchItem>* items = nullptr;
  std::vector<SearchItem*> filtered;
  OnSelectionCallback select_cb;
  bool focused = false;
};
#endif