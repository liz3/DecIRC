#include "search_list.h"
#include "../AppState.h"
#include "../../third-party/glfw/include/GLFW/glfw3.h"
#include "../rendering/box.h"
#include "../rendering/text_box.h"

SearchList::SearchList() {}
bool SearchList::canFocus() {
  return true;
}
void SearchList::onFocus(bool focus) {
  focused = focus;
}
void SearchList::onEnter() {
  if (current_selected) {
    select_cb(current_selected);
  }
}
void SearchList::render(float x, float y, float w, float h) {
  FontAtlas* atlas = &AppState::gState->atlas;
  auto absolute = AppState::gState->getPositionAbsolute(x, y, w, h);
  x = absolute.x;
  y = absolute.y + h;
  if (title.data.size()) {
    TextBox box(title);
    box.color = vec4f(0.65, 0.65, 0.65, 1);
    box.render(x, y, w, atlas->effective_atlas_height);
    y -= (atlas->effective_atlas_height + 15);
  }

  float offset = 0;
  if (focused) {
    TextBox box(text);
    box.render(x, y, w, atlas->effective_atlas_height);
    Box::render(x, y - 4, w, 2, vec4f(0.6, 0.6, 0.6, 1));
  }
  offset += atlas->effective_atlas_height + 15;
  for (auto* entry : filtered) {
    if (offset > h)
      break;
    TextWithState entry_text(entry->name);
    TextBox box(entry_text);
    box.render(x, y - offset, w, atlas->effective_atlas_height);
    if (current_selected == entry && focused) {
      Box::render(x, y - (offset + 4), w, 4, vec4f(0.2, 0.2, 0.8, 0.8));
    }
    offset += atlas->effective_atlas_height + 10;
  }
}

void SearchList::onCodePoint(int32_t cp) {
  text.append(cp);
  recompute();
}
void SearchList::onKey(GLFWwindow* window,
                       int key,
                       int scancode,
                       int action,
                       int mods) {
  bool isPress = action == GLFW_PRESS || action == GLFW_REPEAT;
  if (isPress) {
    if (key == GLFW_KEY_BACKSPACE) {
      text.remove();
      recompute();
    } else if (key == GLFW_KEY_UP) {
      if (selected_index > 0) {
        current_selected = filtered[--selected_index];
      }
    } else if (key == GLFW_KEY_DOWN) {
      if (selected_index < filtered.size() - 1) {
        current_selected = filtered[++selected_index];
      }
    }
  }
}
void SearchList::setItems(std::vector<SearchItem>* im) {
  items = im;
  recompute();
}
void SearchList::recompute() {
  filtered.clear();
  if (items == nullptr)
    return;
  int c = 0;
  selected_index = -1;
  if (text.data.size() == 0) {
    for (auto& e : (*items)) {
      filtered.push_back(&e);
      if (current_selected && &e == current_selected) {
        selected_index = c;
      }
      c++;
    }

  } else {
    std::string value = text.getUtf8Value();
    for (auto& e : (*items)) {
      if (e.name == value || e.name.find(value) != -1) {
        filtered.push_back(&e);
        if (current_selected && &e == current_selected) {
          selected_index = c;
        }
        c++;
      }
    }
  }
  if (current_selected && selected_index == -1) {
    current_selected = nullptr;
  }
  if (current_selected == nullptr && selected_index == -1 && filtered.size()) {
    selected_index = 0;
    current_selected = filtered[0];
  }
}
void SearchList::addText(std::string newContent) {
  text.append(newContent);
  recompute();
    
}
std::string SearchList::getText() {
  return text.getUtf8Value();
}
void SearchList::setCallback(const OnSelectionCallback& cb) {
  select_cb = cb;
}