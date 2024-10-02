#include "message_list.h"
#include "../AppState.h"

#include "../irc/message_state.h"
#include "../gui_components.h"
#include "../utils/web_util.h"

bool MessageList::canFocus() {
  return true;
}
void MessageList::onFocus(bool focus) {}
void MessageList::render(float x, float y, float w, float h) {
  if (msg_load_ptr != nullptr) {
    for (auto* e : *msg_load_ptr) {
      addContent(e, true);
    }
    delete msg_load_ptr;
    msg_load_ptr = nullptr;
  }
  auto window_width = AppState::gState->window_width;
  auto window_height = AppState::gState->window_height;
  Search* search = &AppState::gState->components->search;
  auto search_updated = false;
  if (AppState::gState->components->active_popover == search &&
      search->text.data.size()) {
    std::string s = search->getText();
    bool next = search->getNext() && search_index > -1;
    if (s != lastSearch || next) {
      size_t start = next && s == lastSearch ? search_index + 1 : 0;
      bool found = false;
      while (true) {
        for (size_t i = start; i < messages.size(); i++) {
          auto* current = messages[i];
          if (current->m_holder->message.content.find(s) != std::string::npos) {
            search_index = i;
            search_result = current;
            selected_index = messages.size() - i - 1;
            found = true;
            search_updated = true;
            break;
          }
        }
        if (!found && next) {
          next = false;
          start = 0;
          continue;
        }
        break;
      }
      if (!found) {
        search_index = -1;
        search_result = nullptr;
      }
      lastSearch = s;
    }
  } else {
    lastSearch = "";
    search_index = -1;
    search_result = nullptr;
  }
  if(search_result && search_updated) {
    current_offset = search_result->start;
    if(current_offset > height - available_height)
      current_offset = height - available_height;
  }
  Vec2f abs = vec2f(x - (window_width / 2), (y) - (window_height / 4));
  x = abs.x;
  y = abs.y * 2;
  float offset = 0;
  bool start = false;
  uint32_t i = 0;
  int32_t selected_i = messages.size() - selected_index - 1;
  for (auto* msg : messages) {
    auto end = msg->start + msg->height;
    i++;
    if (end < current_offset) {
      msg->disposeImages();
      continue;
    }
    if (msg->start > current_offset + available_height) {
      msg->disposeImages();
      continue;
    }
    if (!start) {
      offset = -(current_offset - msg->start);
      start = true;
    }
    msg->fetchImages();
    msg->render(x, -(y + offset), width, selected_i == i - 1 && (msg->hasFocus || search_result == msg));
    offset += msg->height;
  }
}
void RenderMessage::disposeImages() {
  render_x = 0;
  render_y = 0;
  if (!initial_loading)
    return;
  for (auto*& image : images) {
    image->unref();
  }
  initial_loading = false;
}
void RenderMessage::fetchImages() {
  if (initial_loading)
    return;
  initial_loading = true;
}
void RenderMessage::render(float x, float y, float w, bool selected) {
  if (selected) {
    Box::render(x, y + (atlas_height * 0.7), w, (-height),
                vec4f(0.25, 0.25, 0.25, 0.8));
  }
  render_w = w;
  render_y = y + atlas_height;
  title_box.render(x, y, 0, 0);
  // y -= atlas_height;
  size_t offset = 300;
  render_x = x;

  box.render(x + offset, y, w - offset, 0);
  if (selected) {
    TextWithState timeState(m_holder->message.getTimeFormatted());
    TextBox timeBox(timeState);
    timeBox.color = vec4fs(0.6);
    timeBox.scale = 0.6;
    timeBox.style = "bold";

    Box::render((x + w) - 75, y, 70, atlas_height * 0.6,
                vec4f(0.1, 0.1, 0.1, 1));
    timeBox.render((x + w) - 70, y + 5, 70, 0);
  }
  y -= (content_height);

  for (auto* image : images) {
    auto h = image->height;
    float scale = 1;
    if (image->width > w - offset) {
      float ww = w - offset > 650.0 ? 650.0 : w - offset;
      scale = ww / image->width;
    }
    y -= 10;
    image->render(x + offset, -y, scale);

    y -= h * scale;
  }
}
RenderMessage* MessageList::addContent(MessageHolder* content, bool prepend) {
  RenderMessage* msg = new RenderMessage(content);
  if (prepend) {
    float atlas_height = AppState::gState->atlas.effective_atlas_height;
    messages.insert(messages.begin(), msg);
    current_offset += (msg->getHeight(width, atlas_height)) + atlas_height + 45;
    if (selected_index != -1)
      selected_index--;
  } else {
    messages.push_back(msg);
    if (selected_index != -1)
      selected_index++;
  }

  recomputeHeights();
  return msg;
}
void MessageList::setAvailableHeight(float y) {
  available_height = y;
  recomputeHeights();
}
void MessageList::updateEntry(MessageHolder* h) {
  for (int i = 0; i < messages.size(); ++i) {
    if (messages[i]->m_holder == h) {
      RenderMessage* old = messages[i];
      delete old;
      messages[i] = new RenderMessage(h);
    }
  }
}
void MessageList::removeEntry(MessageHolder* h) {
  int x = -1;
  for (int i = 0; i < messages.size(); ++i) {
    if (messages[i]->m_holder == h) {
      x = i;
      break;
    }
  }
  if (x == -1)
    return;
  delete messages[x];
  messages.erase(messages.begin() + x);
  selected_index = -1;
}
void MessageList::recomputeHeights() {
  float current = 0.0;
  bool bottom =
      current_offset == height - available_height && current_offset != 0;
  float atlas_height = AppState::gState->atlas.effective_atlas_height;
  for (auto* e : messages) {
    float lHeight = (e->getHeight(width, atlas_height)) + 15;
    e->start = current;
    e->height = lHeight;
    e->atlas_height = atlas_height;
    current += lHeight;
  }

  height = current;
  if (height < available_height) {
    current_offset = 0.0;
  } else {
    if (bottom) {
      current_offset = height - available_height;
    }
  }
}
void MessageList::scrollEnd() {
  current_offset = height - available_height;
}
void MessageList::setWidth(float width) {
  this->width = width;
  recomputeHeights();
}
void MessageList::changeScroll(float offset) {
  auto out = current_offset + offset;
  if (out <= 0) {
    if (current_offset == 0) {
      AppState::gState->client->fetchMore();
    }
    current_offset = 0.0;
  } else if (out > height - available_height) {
    current_offset = height - available_height;
  } else {
    current_offset = out;
  }
}
void MessageList::clearList() {
  for (auto* e : messages) {
    delete e;
  }
  messages.clear();
  selected_index = -1;
  search_index = -1;
  search_result = nullptr;
}
void MessageList::selectIndex(int32_t diff) {
  int target = selected_index;
  if (diff < 0)
    target--;
  else
    target++;
  if (target == -2 || target == messages.size())
    return;
  selected_index = target;
  if (selected_index < 0 || selected_index >= messages.size()) {
    AppState::gState->setTextReceiver(
        &AppState::gState->components->chat_input);
    return;
  }
  int32_t selected_i = messages.size() - selected_index - 1;
  AppState::gState->setTextReceiver(messages[selected_i]);
}
bool RenderMessage::isImage(std::string& url) {
  for (const auto& entry : {".png", ".jpeg", ".jpg", ".webp", ".gif"}) {
    if (url.find(entry) != std::string::npos)
      return true;
  }

  return false;
}
void MessageList::onMousePress(double x, double y, int button, int action) {
  if (action != 0)
    return;
  auto* st = AppState::gState;
  auto sc = st->window_scale;
  for (size_t i = 0; i < messages.size(); i++) {
    auto* message = messages[i];
    if (message->render_x == 0 || message->render_y == 0)
      continue;
    float corrected_y =
        ((-message->render_y) + ((float)st->window_height / 2)) / sc;
    float corrected_height = (message->height) / sc;
    float corrected_x =
        (message->render_x + ((float)st->window_width / 2)) / sc;
    if (x >= corrected_x && x <= corrected_x + message->render_w &&
        y >= corrected_y && y <= corrected_y + corrected_height) {
      selected_index = messages.size() - i - 1;
      AppState::gState->setTextReceiver(message);
      break;
    }
  }
}
void MessageList::onMouseWheel(double xoffset, double yoffset) {
  if (yoffset != 0)
    changeScroll(-(yoffset * 5));
}
RenderMessage::RenderMessage(MessageHolder* holder)
    : m_holder(holder), box(text), title_box(title) {
  box.allowGrow = true;
  box.growDown = true;
  box.richRender = true;
  if (holder->message.action)
    box.style = "bold";
  box.color = vec4f(0.7, 0.7, 0.7, 1);
  box.msg_ref = &holder->message;
  if (!holder->message.action)
    title.setData(holder->message.source.getName());
  if (holder->message.action)
    text.setData("*" + holder->message.source.getName() + " " +
                 holder->message.content);
  else
    text.setData(holder->message.content);
  if (holder->message.type == 1) {
    box.color = vec4f(0.8, 0.3, 0.3, 1);
  }

  std::string cpy = holder->message.content;
  while (true) {
    int res = -1;
    auto index = cpy.find("http://", 0);
    if (index != std::string::npos) {
      res = index;
    } else {
      index = cpy.find("https://", 0);
      if (index != std::string::npos) {
        res = index;
      }
    }
    if (res == -1)
      break;
    std::string start = cpy.substr(res);
    bool f = false;
    for (size_t i = 0; i < start.length(); i++) {
      char c = start[i];
      if (c < 33 || c > 126) {
        f = true;
        std::string link = start.substr(0, i);
        if (isImage(link))
          fetchImage(link);
        links.push_back(link);
        cpy = start.substr(i);
        break;
      }
    }
    if (!f) {
      if (isImage(start))
        fetchImage(start);
      links.push_back(start);
      break;
    }
  }
}
void RenderMessage::fetchImage(std::string url) {
  auto* t = this;
  AppState::gState->client->image_cache.fetchImage(
      url, t, [t](bool success, ImageCacheEntry* image) {
        if (!success || image == nullptr) {
          return;
        }
        auto* f = new std::function([image, t]() {
          Image* instance = new Image(image);

          t->images.push_back(instance);
        });
        AppState::gState->components->runLater(f);
      });
}
RenderMessage::~RenderMessage() {
  for (auto* em : embeds) {
    AppState::gState->client->image_cache.reportDead(em);
    delete em;
  }
  embeds.clear();
  for (auto*& image : images) {
    image->remove();
    delete image;
  }
  if (AppState::gState->current_text_receiver == this)
    AppState::gState->setTextReceiver(nullptr);
}
int RenderMessage::getHeight(float w, float ah) {
  if (m_holder->message.type != 0) {
    return 0;
  }
  auto base = (box.computeHeight(w - 300)) * ah;
  content_height = base;
  // if(m_holder->message.edited_timestamp.length())
  //   base += (ah * 0.6) + ah;

  for (auto* image : images) {
    auto h = image->height;
    float scale = 1;
    if (image->width > w - 300) {
      float ww = w - 300 > 650.0 ? 650.0 : w - 300;
      scale = ww / image->width;
    }
    base += (h * scale) + 10;
  }
  // for (std::map<std::string, DiscordMessageAttachment>::iterator it =
  //          m_holder->message.attachments.begin();
  //      it != m_holder->message.attachments.end(); ++it) {
  //   DiscordMessageAttachment& at = it->second;
  //   if (at.height == 0)
  //     continue;
  //   base += 15;
  //   if (at.width > w - 50) {
  //     float ww = (w-50) > 700.0 ? 700.0 : w;
  //     float val =(ww - ((float)50)) / at.width;
  //     base += at.height * val;
  //   } else {
  //     base += at.height;
  //   }
  // }
  // if (m_holder->message.reactions.size())
  base += 20;
  return base;
}

float EmbedRender::getHeight(float w, float atlas_height) {
  float base = 15 + atlas_height;
  // auto h = t_box.computeHeight(w) * atlas_height;
  // auto d = d_box.computeHeight(w) * atlas_height;
  // auto f = f_box.computeHeight(w) * atlas_height;
  // if (h)
  //   base += h + atlas_height;
  // if (d)
  //   base += d + atlas_height;
  // if (f)
  //   base += f + atlas_height;

  // if (embed.image_url.size()) {
  //   if (embed.image_width > w) {
  //     float val = (w) / embed.image_width;
  //     base += embed.image_height * val;
  //   } else {
  //     base += embed.image_height;
  //   }
  //   base += 15;
  // }
  // height = base;
  // ah = atlas_height;
  return base;
}
EmbedRender::EmbedRender() : t_box(title), d_box(description), f_box(footer) {
  // title.setData(embed.title);
  // t_box.growDown = true;
  // t_box.allowGrow = true;
  // d_box.growDown = true;
  // d_box.allowGrow = true;

  // f_box.growDown = true;
  // f_box.allowGrow = true;

  // t_box.style = "bold";
  // description.setData(embed.description);
  // footer.setData(embed.footer_text);
  // if (embed.image_url.size()) {
  //   auto* t = this;
  //   AppState::gState->client->image_cache.fetchImage(
  //       embed.image_url, t, [t](bool success, ImageCacheEntry* image) {
  //         if (!success || image == nullptr)
  //           return;

  //         auto* f = new std::function([image, t]() {
  //           Image* instance = new Image(image);
  //           t->image = instance;
  //         });
  //         AppState::gState->components->runLater(f);
  //       });
  // }
}
EmbedRender::~EmbedRender() {
  if (image) {
    image->remove();
    delete image;
    image = nullptr;
  }
}
void EmbedRender::render(float x, float y, float w) {
  if (title.data.size()) {
    t_box.render(x, y, w, 0);
    y -= t_box.computeHeight(w) * ah;
    y -= ah;
  }
  if (description.data.size()) {
    d_box.render(x, y, w, 0);
    y -= d_box.computeHeight(w) * ah;
    y -= ah;
  }
  if (image) {
    auto h = image->height;
    float scale = 1;
    if (image->width > w) {
      scale = w / image->width;
    }
    image->render(x, -y, scale);
    y -= h * scale;
    y -= 15;
  }
  if (footer.data.size()) {
    y -= ah;
    f_box.render(x, y, w, 0);
    y -= f_box.computeHeight(w) * ah;
  }
}
void RenderMessage::onEnter() {};
void RenderMessage::onCodePoint(int32_t cp) {};
void RenderMessage::onKey(GLFWwindow* window,
                          int key,
                          int scancode,
                          int action,
                          int mods) {
  if (action != GLFW_PRESS)
    return;
  bool ctrl_pressed = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                      glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
  bool alt_pressed = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS;
  if (key == GLFW_KEY_Q) {
    if (!ctrl_pressed && !alt_pressed)
      return;
    auto* st = AppState::gState;
    if (ctrl_pressed)
      st->components->chat_input.text.setData("> " + getText() + " ");
    else
      st->components->chat_input.text.setData(
          m_holder->message.source.getName() + ": ");
    st->setTextReceiver(&st->components->chat_input);
    return;
  }
  bool d_pressed = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
  if (key >= 49 && key < 58) {
    int num = key - 49;
    if (d_pressed) {
      if (links.size() > num) {
        std::string lnk = links[num];
        dec_open_in_browser(lnk);
      }
      return;
    }

    std::vector<Image*> all;
    for (auto* e : images) {
      all.push_back(e);
    }
    for (auto* e : embeds) {
      if (e->image)
        all.push_back(e->image);
    }
    if (all.size() > num) {
      AppState::gState->components->imageOverlay.initFrom(all[num]);
      AppState::gState->components->setActivePopover(
          &AppState::gState->components->imageOverlay);
    }
  }
};
bool RenderMessage::canFocus() {
  return true;
};
void RenderMessage::onFocus(bool focus) {
  hasFocus = focus;
};
void RenderMessage::render(float x, float y, float w, float h) {

};

void RenderMessage::addText(std::string text) {
  // noop
}
std::string RenderMessage::getText() {
  return m_holder->message.content;
}
