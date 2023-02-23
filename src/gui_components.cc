#include "gui_components.h"
#include "AppState.h"

void GuiComponents::init() {
  chat_input.text.setData("");
  chat_input.box.background = vec4f(0.3, 0.3, 0.3, 1.0);
  chat_input.box.allowGrow = true;
  chat_input.list = &message_list;
  status_text.setData("Ready");
  auto* t = this;
  chat_input.enterCb = [t](std::string data) {
    t->state->client->sendChannelMessage(data);
  };
  header_comp.background = vec4f(0.2, 0.2, 0.2, 1);
  header_comp.style = "bold";
  dm_list.title.setData("Channels");
  network_list.title.setData("Networks");
  channel_list.title.setData("Channels");
  dm_list.setCallback([t](const SearchItem* item) {
    t->state->client->itemSelected("dm", item);
  });
  user_list.getList().setCallback([t](const SearchItem* item) {
    size_t off = 0;
    std::string name = item->name;
    for(const char& e : name) {
      if((e >= 'a' && e <= 'z') || (e >= 'A' && e <= 'Z'))
        break;
      off++;
    }
    std::string sub = name.substr(off);
    t->state->client->query(sub);
  });
  network_list.setCallback([t](const SearchItem* item) {
    t->state->client->itemSelected("network", item);
  });
  channel_list.setCallback([t](const SearchItem* item) {
    t->state->client->itemSelected("channel", item);
  });
}
void GuiComponents::render() {
  if (locked) {
    std::unique_lock lk(mtx);
    for (auto& t : tasks) {
      (*t)();
      delete t;
    }
    tasks.clear();
    lk.unlock();
    locked = false;
  }

  auto window_width = state->window_width;
  auto window_height = state->window_height;
  message_list.setWidth(window_width * 0.70);
  message_list.setAvailableHeight(window_height - 65 - 140);
  message_list.render(window_width - (window_width * 0.7), 80, 0, 0);
  chat_input.render(window_width - (window_width * 0.7), window_height - 85,
                    window_width * 0.69, 70);
  if (header_text.data.size()) {
    auto abs = state->getPositionAbsolute(window_width - (window_width * 0.7),
                                          -30, window_width * 0.7, 65);
    header_comp.render(abs.x, abs.y, window_width * 0.7, 65);
  }

  network_list.render(50, 50, 400, 400);
  channel_list.render(50, 400, 400, 400);

  if (status_text.data.size()) {
    auto abs = state->getPositionAbsolute(50, window_height - 150, 400, 65);
    status_comp.render(abs.x, abs.y, 400, 65);
  }
  if (active_popover) {
    active_popover->render(window_width, window_height);
  }
}
GuiComponents::GuiComponents(AppState* _state)
    : header_comp(header_text), status_comp(status_text) {
  state = _state;
}
GuiComponents::GuiComponents()
    : header_comp(header_text), status_comp(status_text) {}