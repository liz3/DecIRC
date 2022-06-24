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
  dm_list.title.setData("Direct Messages");
  guilds_list.title.setData("Servers");
  channel_list.title.setData("Channels");
  dm_list.setCallback([t](const SearchItem* item) {
    t->state->client->itemSelected("dm", item);
  });
  guilds_list.setCallback([t](const SearchItem* item) {
    t->state->client->itemSelected("guild", item);
  });
  channel_list.setCallback([t](const SearchItem* item) {
    t->state->client->itemSelected("channel", item);
  });
}
void GuiComponents::render() {

  if(locked) {
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
  message_list.setWidth(window_width * 0.64);
  message_list.setAvailableHeight(window_height - 65 - 200);
  message_list.render(window_width - (window_width * 0.65), 80, 0, 0);
  chat_input.render(window_width - (window_width * 0.65), window_height - 120,
                    window_width * 0.64, 70);
  if (header_text.data.size()) {
    //	header_text.setData("this is a longer test to see if this renders
    // correctly");
    auto abs = state->getPositionAbsolute(window_width - (window_width * 0.65),
                                          -30, window_width * 0.64, 65);
    header_comp.render(abs.x, abs.y, window_width * 0.64, 65);
  }
  if (root_list_display == 1 || root_list_display == 0)
    dm_list.render(50, 50, 400, 400);
  else if (root_list_display == 2 || root_list_display == 3)
    guilds_list.render(50, 50, 400, 400);
  if (root_list_display == 3)
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