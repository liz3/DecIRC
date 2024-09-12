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
  {
    TextBox box(text);
    if (!focused)
      box.color = vec4fs(0.5);
    box.render(x, y, w, atlas->effective_atlas_height);
  }
  if (focused) {
    Box::render(x, y - 4, w, 2, vec4f(0.6, 0.6, 0.6, 1));
  }
  offset += atlas->effective_atlas_height + 15;
  size_t render_amount = (h - offset) / (atlas->effective_atlas_height + 10);
  size_t skip = 0;
  if (selected_index > render_amount)
    skip = selected_index - render_amount;
  for (auto* entry : filtered) {
    if (skip > 0) {
      skip--;
      entry->y = 0;
      continue;
    }
    if (offset > h)
      break;
    TextWithState entry_text(entry->name);
    TextBox box(entry_text);
    box.richRender = true;
    box.color = entry->color;
    box.render(x, y - offset, w, atlas->effective_atlas_height);
    if (current_selected == entry && focused) {
      Box::render(x, y - (offset + 4), w, 4, vec4f(0.2, 0.2, 0.8, 0.8));
    }
          entry->x = x;
      entry->y = y - (offset + 4);
      entry->w = w;
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
  bool ctrl_pressed = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                      glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
  bool alt_pressed = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS;
  if (isPress) {
    if (key == GLFW_KEY_BACKSPACE) {
      if (alt_pressed)
        text.removeWord();
      else
        text.remove();
      recompute();
    } else if (key == GLFW_KEY_UP || (ctrl_pressed && key == GLFW_KEY_P)) {
      if (selected_index > 0) {
        current_selected = filtered[--selected_index];
      }
    } else if (key == GLFW_KEY_DOWN || (ctrl_pressed && key == GLFW_KEY_N)) {
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
void SearchList::onMousePress(double x, double y, int button, int action) {
   if(action != 0)
    return;
  auto* st = AppState::gState;
  auto sc = st->window_scale;

  for (size_t i = 0; i < filtered.size(); i++) {
    auto* entry = filtered[i];
    if(entry->y == 0)
      continue;
       float corrected_y = ((-entry->y) + ((float)st->window_height / 2)) /sc;
      float corrected_x =  (entry-> x + ((float)st->window_width / 2)) / sc;
    if (x >= corrected_x && x <= corrected_x + entry->w && y <= corrected_y &&
        y >= corrected_y - st->atlas.effective_atlas_height + 10) {
      selected_index = i;
      current_selected = filtered[i];
      break;
    }
  }
}
void SearchList::onMouseWheel(double xoffset, double yoffset) {
  if (!filtered.size())
    return;
  if (yoffset > 0) {
    int next = selected_index + 1;
    if(next == selected_index)
      next++;
    if (next < filtered.size()) {
      selected_index = next;
      current_selected = filtered[next];
    } else {
      selected_index = filtered.size() - 1;
      current_selected = filtered[filtered.size() - 1];
    }
  } else if (yoffset < 0) {
    int next = selected_index -1;

    if (next >= 0) {
      selected_index = next;
      current_selected = filtered[next];
    } else {
      selected_index = 0;
      current_selected = filtered[0];
    }
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