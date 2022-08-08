#include "message_list.h"
#include "../AppState.h"

#include "../discord/message_state.h"
#include "../gui_components.h"
#include <algorithm>
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
    if (end < currentOffset) {
      msg->disposeImages();
      continue;
    }
    if (msg->start > currentOffset + availableHeight) {
           msg->disposeImages();
      continue;
    }
    if (!start) {
      offset = -(currentOffset - msg->start);
      start = true;
    }
    msg->fetchImages();
    msg->render(x, -(y + offset), width - 50, selected_i == i - 1 && msg->hasFocus);
    offset += msg->height;
  }
}
void RenderMessage::disposeImages() {
  if(!initiatedLoading)
    return;
  for (auto*& image : images) {
    image->remove();
    delete image;
  }
  initiatedLoading = false;
  images.clear();
}
void RenderMessage::fetchImages() {
  if(initiatedLoading)
    return;
  initiatedLoading = true;
    for (std::map<std::string, DiscordMessageAttachment>::iterator it =
           m_holder->message.attachments.begin();
       it != m_holder->message.attachments.end(); ++it) {
    if (it->second.content_type.find("image") != -1) {
      fetchImage(it->second.proxy_url);
    }
  }

}
void RenderMessage::render(float x, float y, float w, bool selected) {
  if (selected) {
    Box::render(x, y + (atlas_height), w, (-height), vec4f(0.1, 0.1, 0.1, 1));
  }
  if (m_holder->message.type == 0) {
    title_box.render(x + 50, y, 0, 0);
    y -= atlas_height;
  }
  box.render(x + 50, y, w, 0);
  y -= (content_height);
    if(m_holder->message.edited_timestamp.length()) {
        y -= atlas_height;

    TextWithState edited;
    TextBox edit_box(edited);
    edited.setData("edited");
    edit_box.color = vec4f(0.4,0.4,0.4, 1);
    edit_box.scale = 0.6;
    edit_box.render(x+50, y, 0,0);
    y -= (atlas_height * 0.6);
  }
  for (auto* em : embeds) {
    y -= 15 + atlas_height;
    em->render(x + 70, y, w - 20);
    y -= (em->height);
  }
  for (auto* image : images) {
    auto h = image->height;
    float scale = 1;
    if (image->width > w) {
       float ww = w > 650.0 ? 650.0 : w;
      scale = ww / image->width;
    }
      y -= 15;
    image->render(x + 50, -y, scale);

    y -= h * scale;
  }
  y -= (20 + atlas_height);
  auto baseOffset = x + 50;
  for (auto& reaction : m_holder->message.reactions) {
      if (!reaction.second.emote_id.length())
          continue;
    auto* img = AppState::gState->client->image_cache.getEmote(
        reaction.second.emote_id);
    auto& atlas = AppState::gState->atlas;
    if (img) {
      float imgScale = (atlas_height * 0.8) / (float)img->width;
      std::string num_text = std::to_string(reaction.second.count);
      TextWithState ts(num_text);
      TextBox num_b(ts);
      auto textWidth = atlas.getAdvance(ts.data);
      Box::render(baseOffset - 3, y - 3, atlas_height + 5 + textWidth + 3,
                  atlas_height + 3, vec4f(0, 0, 0, 1));
      img->render(baseOffset, -y - atlas_height + (atlas_height * 0.2),
                  imgScale);
      baseOffset += atlas_height + 5;
      num_b.render(baseOffset, y, 0, 0);
      baseOffset += textWidth + 15;
    }
  }
}
RenderMessage* MessageList::addContent(MessageHolder* content, bool prepend) {
  RenderMessage* msg = new RenderMessage(content);
  if (prepend) {
    float atlas_height = AppState::gState->atlas.effective_atlas_height;
    messages.insert(messages.begin(), msg);
    currentOffset += (msg->getHeight(width, atlas_height)) + atlas_height + 45;
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
  availableHeight = y;
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
  bool bottom = currentOffset == height - availableHeight && currentOffset != 0;
  float atlas_height = AppState::gState->atlas.effective_atlas_height;
  for (auto* e : messages) {
    float lHeight = (e->getHeight(width, atlas_height)) + atlas_height + 45;
    e->start = current;
    e->height = lHeight;
    e->atlas_height = atlas_height;
    current += lHeight;
  }

  height = current;
  if (height < availableHeight) {
    currentOffset = 0.0;
  } else {
    if (bottom) {
      currentOffset = height - availableHeight;
    }
  }
}
void MessageList::scrollEnd() {
  currentOffset = height - availableHeight;
}
void MessageList::setWidth(float width) {
  this->width = width;
  recomputeHeights();
}
void MessageList::changeScroll(float offset) {
  auto out = currentOffset + offset;
  if (out <= 0) {
    if (currentOffset == 0) {
      AppState::gState->client->fetchMore();
    }
    currentOffset = 0.0;
  } else if (out > height - availableHeight) {
    currentOffset = height - availableHeight;
  } else {
    currentOffset = out;
  }
}
void MessageList::clearList() {
  for (auto* e : messages) {
    delete e;
  }
  messages.clear();
  selected_index = -1;
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
  if(selected_index < 0 || selected_index >= messages.size()) {
    AppState::gState->setTextReceiver(&AppState::gState->components->chat_input);
    return;
  }
  int32_t selected_i = messages.size() - selected_index - 1;
  AppState::gState->setTextReceiver(messages[selected_i]);
}

RenderMessage::RenderMessage(MessageHolder* holder)
    : m_holder(holder), box(text), title_box(title) {
  box.allowGrow = true;
  box.growDown = true;
  box.color = vec4f(0.7, 0.7, 0.7, 1);
  box.discordMode = true;
  box.msg_ref = &holder->message;
  if (holder->message.type == 0) {
    title.setData(holder->message.author.username);
    text.setData(holder->message.content);
  } else {
    title.setData("");
    text.setData(holder->message.author.username + ": System message");
    box.color = vec4f(0.5, 0.5, 0.7, 1);
  }
  for (auto& em : holder->message.embeds) {
    embeds.push_back(new EmbedRender(em));
  }
  std::string cpy = holder->message.content;
  while (true) {
      int res = -1; 
    auto index = cpy.find("http://", 0);
    if (index != std::string::npos) {
        res = index;
    }
    else {
        index = cpy.find("https://", 0);
        if (index != std::string::npos) {
            res = index;
        }
    }
    if (res == -1)
        break;
    std::string start = cpy.substr(res);
    bool f = false;
    for (char c : " \n \t") {
        auto i = start.find(c, 0);
        if (i != std::string::npos) {
            f = true;
            std::string link = start.substr(0, i);
            links.push_back(link);
            cpy = start.substr(i);
            break;
        }
    }
    if (!f) {
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
  if (m_holder->message.attachments.size()) {
    AppState::gState->client->image_cache.reportDead(this);
  }
  for (auto* em : embeds) {
    AppState::gState->client->image_cache.reportDead(em);
    delete em;
  }
  embeds.clear();
  for (auto*& image : images) {
    image->remove();
    delete image;
  }
  if(AppState::gState->current_text_receiver == this)
    AppState::gState->setTextReceiver(nullptr);
}
int RenderMessage::getHeight(float w, float ah) {
  if (m_holder->message.type != 0) {
    return 0;
  }
  auto base = (box.computeHeight(w - 50))* ah;
  content_height = base;
  if(m_holder->message.edited_timestamp.length())
    base += (ah * 0.6) + ah;

  for (auto* em : embeds) {
    base += em->getHeight(w - 70, ah) + 15;
  }
  for (std::map<std::string, DiscordMessageAttachment>::iterator it =
           m_holder->message.attachments.begin();
       it != m_holder->message.attachments.end(); ++it) {
    DiscordMessageAttachment& at = it->second;
    if (at.height == 0)
      continue;
    base += 15;
    if (at.width > w - 50) {
      float ww = (w-50) > 700.0 ? 700.0 : w;
      float val =(ww - ((float)50)) / at.width;
      base += at.height * val;
    } else {
      base += at.height;
    }
  }
  if (m_holder->message.reactions.size())
    base += ah + 20;
  return base;
}

float EmbedRender::getHeight(float w, float atlas_height) {
  float base = 15 + atlas_height;
  auto h = t_box.computeHeight(w) * atlas_height;
  auto d = d_box.computeHeight(w) * atlas_height;
  auto f = f_box.computeHeight(w) * atlas_height;
  if (h)
    base += h + atlas_height;
  if (d)
    base += d + atlas_height;
  if (f)
    base += f + atlas_height;

  if (embed.image_url.size()) {
    if (embed.image_width > w) {
      float val = (w) / embed.image_width;
      base += embed.image_height * val;
    } else {
      base += embed.image_height;
    }
    base += 15;
  }
  height = base;
  ah = atlas_height;
  return base;
}
EmbedRender::EmbedRender(DiscordMessageEmbed embed)
    : embed(embed), t_box(title), d_box(description), f_box(footer) {
  title.setData(embed.title);
  t_box.growDown = true;
  t_box.allowGrow = true;
  d_box.growDown = true;
  d_box.allowGrow = true;

  f_box.growDown = true;
  f_box.allowGrow = true;

  t_box.style = "bold";
  description.setData(embed.description);
  footer.setData(embed.footer_text);
  if (embed.image_url.size()) {
    auto* t = this;
    AppState::gState->client->image_cache.fetchImage(
        embed.image_url, t, [t](bool success, ImageCacheEntry* image) {
          if (!success || image == nullptr)
            return;

          auto* f = new std::function([image, t]() {
            Image* instance = new Image(image);
            t->image = instance;
          });
          AppState::gState->components->runLater(f);
        });
  }
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
  void RenderMessage::onEnter() {
  };
  void RenderMessage::onCodePoint(int32_t cp) {
  };
  void RenderMessage::onKey(GLFWwindow* window,
             int key,
             int scancode,
             int action,
             int mods) {

      if (action != GLFW_PRESS)
          return;
      bool d_pressed = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
    if(key >= 49 && key < 58) {
      int num = key - 49;
      if (d_pressed) {
          if (links.size() > num) {
              std::string lnk = links[num];
              dec_open_in_browser(lnk);
          }
          return;
      }

      std::vector<Image*> all;
      for(auto* e : images) {
        all.push_back(e);
      }
            for(auto* e : embeds) {
              if(e->image)
        all.push_back(e->image);
      }
      if(all.size() > num) {

        AppState::gState->components->imageOverlay.initFrom(all[num]);
        AppState::gState->components->setActivePopover(&AppState::gState->components->imageOverlay);
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