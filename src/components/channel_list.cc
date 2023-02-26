#include "channel_list.h"
#include "../AppState.h"

ChannelList::ChannelList() : queryBox(queryText) {
  queryBox.color = vec4fs(0.8);
  searchList.setCallback([this](const SearchItem* item) {
    if (network) {
      IrcChannelSearchEntry* e =
          reinterpret_cast<IrcChannelSearchEntry*>(item->user_data);
      if (!network->joinedChannels.count(e->name)) {
        std::vector<std::string> cmds = {"JOIN", e->name};
        network->write(cmds);
      }
    }
  });
}
bool ChannelList::canFocus() {
  return true;
}
void ChannelList::onFocus(bool focus) {
  if (focus)
    AppState::gState->setTextReceiver(&searchList);
  else
    network = nullptr;
}
void ChannelList::render(float x, float y, float w, float h) {}
void ChannelList::render(float width, float height) {
  auto atlas_height = AppState::gState->atlas.effective_atlas_height;
  auto render_width = width * 0.9;
  auto render_height = height * 0.9;
  auto x = width - render_width;
  auto y = height - render_height;
  auto abs = AppState::gState->getPositionAbsolute(x, y, 0, 0);
  Box::render(abs.x, abs.y, render_width - x, -(render_height - y),
              vec4f(0.1, 0.1, 0.15, 1));
  if (queryText.data.size())
    queryBox.render(abs.x, abs.y - atlas_height, render_width - x,
                    atlas_height + 10);
  searchList.render(x, y + ((atlas_height * 2) + 45), render_width - x,
                    render_height - (atlas_height * 2) - 65 - y);
}
void ChannelList::initFrom(IrcClient* client) {
  items.clear();
  network = client;
  for (auto& entry : client->channelSearch) {
    SearchItem item;
    item.name = entry.name + "[" + std::to_string(entry.user_count) +
                "]: " + entry.topic;
    item.user_data = &entry;
    items.push_back(item);
  }
  searchList.setItems(&items);
  queryText.setData("Search: " + client->searchQuery + ": " +
                    std::to_string(items.size()));
}
