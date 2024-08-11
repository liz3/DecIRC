#include "channel_list.h"
#include "../AppState.h"

ChannelList::ChannelList() : query_box(query_text) {
  query_box.color = vec4fs(0.8);
  search_list.setCallback([this](const SearchItem* item) {
    if (network) {
      if(mode == List) {
 IrcChannelSearchEntry* e =
          reinterpret_cast<IrcChannelSearchEntry*>(item->user_data);
      if (!network->joinedChannels.count(e->name)) {
        std::vector<std::string> cmds = {"JOIN", e->name};
        network->write(cmds);
      }
      } else if (mode == Names) {
         IrcNameSearchEntry* e =
          reinterpret_cast<IrcNameSearchEntry*>(item->user_data);
          AppState::gState->client->query(network, e->name);
      } 
    }else if (mode == QuickSearch) {
        IrcChannel* ch = reinterpret_cast<IrcChannel*>(item->user_data);
        AppState::gState->client->loadChannel(ch, true);


      }
  });
}
bool ChannelList::canFocus() {
  return true;
}
void ChannelList::onFocus(bool focus) {
  if (focus)
    AppState::gState->setTextReceiver(&search_list);
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
  if (query_text.data.size())
    query_box.render(abs.x, abs.y - atlas_height, render_width - x,
                    atlas_height + 10);
  search_list.render(x, y + ((atlas_height * 2) + 45), render_width - x,
                    render_height - (atlas_height * 2) - 65 - y);
}
void ChannelList::initQuickSearch(std::vector<IrcChannel*> channels) {
  mode = QueryPopulateType::QuickSearch;
  items.clear();
  for(auto * channel : channels) {
    SearchItem item;
    item.name = "[" + channel->client->networkInfo.given_name + "] " + channel->name;
    item.user_data = channel;
    items.push_back(item);
  }
 search_list.setItems(&items);
  query_text.setData("alt+k: " + std::to_string(items.size()));
}
void ChannelList::initFrom(IrcClient* client, QueryPopulateType type) {
  this->mode = type;
  items.clear();
  network = client;
  if(type == List) {
      for (auto& entry : client->channelSearch) {
    SearchItem item;
    item.name = entry.name + "[" + std::to_string(entry.user_count) +
                "]: " + entry.topic;
    item.user_data = &entry;
    items.push_back(item);
  }
  search_list.setItems(&items);
  query_text.setData("Search: " + client->searchQuery + ": " +
                    std::to_string(items.size()));
  }

  if(type == Names) {
   for (auto& entry : client->nameSearch.entries) {
    SearchItem item;
    item.name = entry.channel + (entry.mode.length() ? ("[" + entry.mode +
                        "]: " ) : ": ")+ entry.name;
    item.user_data = &entry;
    items.push_back(item);
  }
  std::string channels = "";
  for(size_t i = 0; i < client->nameSearch.channels.size(); i++) {
    channels += client->nameSearch.channels[i];
    if(i < client->nameSearch.channels.size()-1)
      channels += ", ";
  }
  search_list.setItems(&items);
  query_text.setData("Names: " + channels + ": " +
                    std::to_string(items.size()));
  }
}
